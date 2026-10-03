#include "HAL_Robot_Module.h"
#include <cstring>

HAL_Robot_Module *HAL_Robot_Module::_instance = nullptr;

// The one true definition of the global board_io.h expects (see its
// "extern KTS1622_IO_Expander expander;") -- every board_pinMode/
// board_digitalWrite/board_digitalRead call in the codebase resolves here.
namespace
{
    const uint8_t kExpanderAddrs[2] = {KTS1622_ADDR_VDD, KTS1622_ADDR_GND}; // up to 4
}
KTS1622_IO_Expander expander(2, kExpanderAddrs, Wire);

// ---------------------------------------------------------------------------
// Generic Print sink that captures bytes into a fixed caller-owned buffer
// instead of writing to a stream -- lets SDCard_Module::print_root_listing()
// (which only knows how to write to a Print) feed a HostLink response
// payload instead of Serial.
// ---------------------------------------------------------------------------
class BufferPrint : public Print
{
public:
    BufferPrint(uint8_t *buf, size_t cap) : _buf(buf), _cap(cap) {}

    size_t write(uint8_t c) override
    {
        if (_len >= _cap)
            return 0;
        _buf[_len++] = c;
        return 1;
    }

    size_t write(const uint8_t *data, size_t len) override
    {
        size_t avail = _cap - _len;
        size_t n = (len < avail) ? len : avail;
        memcpy(_buf + _len, data, n);
        _len += n;
        return n;
    }

    uint8_t length() const { return (uint8_t)_len; }

private:
    uint8_t *_buf;
    size_t _cap;
    size_t _len = 0;
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void HAL_Robot_Module::begin()
{
    _instance = this;

    _eventQueue = xQueueCreate(HAL_EVENT_QUEUE_LEN, sizeof(RobotEvent));

    hardware_setup();
    cliSerial = &Serial;
    cli_setup();
}

void HAL_Robot_Module::update()
{
    hostLink.poll();

    RobotEvent evt;
    if (_eventQueue && xQueueReceive(_eventQueue, &evt, 0) == pdTRUE)
    {
        uint8_t respPayload[PROTOCOL_MAX_PAYLOAD - 1];
        uint8_t respLen = 0;
        uint8_t status = hostLink.dispatch(evt.command, evt.data, evt.dataLen, respPayload, respLen);
#ifdef DEBUG
        Serial.printf("[event %lu \"%s\"] dispatched cmd=0x%02X status=0x%02X\r\n",
                      (unsigned long)evt.id, evt.title, (uint8_t)evt.command, status);
#endif
        (void)status; // queued events are fire-and-forget: result isn't pushed back to the host
    }

    hardware_update();
}

bool HAL_Robot_Module::post_event(const char *title, uint8_t titleLen,
                                   PacketCmd command, const uint8_t *data, uint8_t dataLen,
                                   uint32_t &outId)
{
    if (!_eventQueue)
        return false;
    if (dataLen > sizeof(RobotEvent::data))
        return false;

    RobotEvent evt{};
    evt.id = _nextEventId;
    uint8_t copyLen = (titleLen < sizeof(evt.title) - 1) ? titleLen : (uint8_t)(sizeof(evt.title) - 1);
    memcpy(evt.title, title, copyLen);
    evt.title[copyLen] = '\0';
    evt.command = command;
    evt.dataLen = dataLen;
    if (dataLen)
        memcpy(evt.data, data, dataLen);

    if (xQueueSend(_eventQueue, &evt, 0) != pdTRUE)
        return false; // queue full

    outId = _nextEventId++;
    return true;
}

// ---------------------------------------------------------------------------
// Hardware setup/update -- moved from src/system_functions.h, reaching
// members instead of globals. Bodies otherwise unchanged.
// ---------------------------------------------------------------------------

void HAL_Robot_Module::hardware_setup()
{
    /* Initialize serial communication */
    Serial.begin(115200); // USB CDC Connection

    SPI.begin(BOARD_SCLK, BOARD_MISO, BOARD_MOSI);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(400000); // Set I2C frequency to 400kHz

    // ESP IO Config
    board_pinMode(BOARD_IO2_INT, INPUT);
    board_pinMode(BOARD_IO2_RST, OUTPUT);
    board_digitalWrite(BOARD_IO2_RST, HIGH);

    expander_ok = expander.begin();

    board_pinMode(BOARD_IO1_INT, INPUT);
    board_pinMode(BOARD_VLx_SPI_N, OUTPUT);
    board_pinMode(BOARD_VLx_NCS, OUTPUT);
    board_pinMode(BOARD_VLx_SYNC, OUTPUT);
    board_pinMode(BOARD_VLx_INT, INPUT);
    board_pinMode(BOARD_VLx_LPN, OUTPUT);
    board_pinMode(BOARD_SENSE_RST, OUTPUT);
    board_pinMode(BOARD_SENSE_CHG, OUTPUT);
    board_pinMode(BOARD_TFT_RST, OUTPUT);
    board_pinMode(BOARD_TFT_CS, OUTPUT);
    board_pinMode(BOARD_TOUCH_IRQ, INPUT);
    board_pinMode(BOARD_TOUCH_RST, OUTPUT);
    board_pinMode(BOARD_SDMMC_CS, OUTPUT);
    board_pinMode(BOARD_SDMMC_DET, INPUT);
    board_pinMode(BOARD_SYS_SPI_CS, INPUT);
    board_pinMode(BOARD_SYS_PWR_BUT, INPUT);
    board_pinMode(BOARD_SYS_PWR_EN, OUTPUT);
    board_pinMode(BOARD_SYS_PWR_SHTDN, INPUT);
    board_pinMode(BOARD_IMU_INT1, INPUT);
    board_pinMode(BOARD_IMU_INT2, INPUT);
    board_pinMode(BOARD_LED, OUTPUT);
    board_pinMode(BOARD_TFT_BL, OUTPUT);
    board_pinMode(BOARD_CHG_EN, OUTPUT);
    board_pinMode(BOARD_CHG_DETECT, INPUT);
    board_pinMode(BOARD_CURR_ALERT1, INPUT);
    board_pinMode(BOARD_CURR_ALERT2, INPUT);
    board_pinMode(BOARD_REG_EN, OUTPUT);

#if TEST
    board_digitalWrite(BOARD_REG_EN, HIGH);
    board_digitalWrite(BOARD_LED, LOW);
#endif

#if ENABLE_CURRENT_SENSE
    current_module_ok = current_module.setup();
#endif

#if ENABLE_SERVO
    servo_module_ok = servo_module.setup();
#endif

#if ENABLE_BOTTANGO
    // After the servo bus is up: in offline mode this registers the exported
    // animations' servos immediately.
    bottango_module.setup();
#endif

#if ENABLE_IMU_COMPASS
    imu_module_ok = imu_module.setup();
#endif

#if ENABLE_TOF
    tof_module_ok = tof_module.setup();
#endif

#if ENABLE_SD
    sdcard_module_ok = sdcard_module.setup();
#endif

#if ENABLE_BATTERY
#if ENABLE_CURRENT_SENSE
    battery_module.attach_current_source(&HAL_Robot_Module::s_battery_current_source);
#endif
    battery_module_ok = battery_module.setup();
#endif
}

void HAL_Robot_Module::hardware_update()
{
#if ENABLE_CURRENT_SENSE
    current_module.update();
#endif

#if ENABLE_SERVO
    servo_module.update();
#endif

#if ENABLE_IMU_COMPASS
    imu_module.update();
#endif

#if ENABLE_TOF
    tof_module.update();
#endif

#if ENABLE_SD
    sdcard_module.update();
#endif

#if ENABLE_BATTERY
    battery_module.update();
#endif
}

#if ENABLE_BATTERY && ENABLE_CURRENT_SENSE
Battery_Module::CurrentReadings HAL_Robot_Module::battery_current_source()
{
    return {
        (uint16_t)current_module.currSys->readBusVoltage_mV(),
        (int16_t)current_module.currSys->readCurrent_mA(),
        (uint16_t)current_module.currSys->readPower_mW(),
        (uint16_t)current_module.currMotor->readBusVoltage_mV(),
        (int16_t)current_module.currMotor->readCurrent_mA(),
        (uint16_t)current_module.currMotor->readPower_mW(),
    };
}

Battery_Module::CurrentReadings HAL_Robot_Module::s_battery_current_source()
{
    return _instance->battery_current_source();
}
#endif

// ---------------------------------------------------------------------------
// CLI -- moved from src/system_functions.h
// ---------------------------------------------------------------------------

void HAL_Robot_Module::cli_setup()
{
    cli.setOnError(&HAL_Robot_Module::s_errorCallback);
    cmdHelp = cli.addCommand("help", &HAL_Robot_Module::s_helpCallback);
    cmdHelp.setDescription("Prints this help message");
    cmdHelp.addFlagArgument("-v");

    cmdPheriph_Test = cli.addCommand("test_periph/erials", &HAL_Robot_Module::s_testPeriphCallback);
    cmdPheriph_Test.setDescription("Test peripherials in report or individually stream data");
    cmdPheriph_Test.addFlagArgument("current");
    cmdPheriph_Test.addFlagArgument("servo");
    cmdPheriph_Test.addFlagArgument("tof");
    cmdPheriph_Test.addFlagArgument("imu");
    cmdPheriph_Test.addFlagArgument("touch");
    cmdPheriph_Test.addFlagArgument("buzzer");
    cmdPheriph_Test.addFlagArgument("sdcard");
    cmdPheriph_Test.addFlagArgument("battery");

    cli.addCommand("ping", &HAL_Robot_Module::s_pingCallback);

#if ENABLE_BOTTANGO
    cmdBottango = cli.addCommand("bottango", &HAL_Robot_Module::s_bottangoCallback);
    cmdBottango.setDescription("Bottango: run [animation [loop]] (blocks until quit) | status | live | offline | cal <id> <usMin> <usMax> <posMin> <posMax> [inv]");
    cmdBottango.addPosArg("action", "run");
    cmdBottango.addPosArg("a1", "");
    cmdBottango.addPosArg("a2", "");
    cmdBottango.addPosArg("a3", "");
    cmdBottango.addPosArg("a4", "");
    cmdBottango.addPosArg("a5", "");
    cmdBottango.addPosArg("a6", "");
#endif

    // Same command set answers identically whether it comes in as a typed
    // CLI line above or a binary PacketCmd frame from the Raspberry Pi.
    hostLink.begin(Serial, cli);
    hostLink.registerHandler(PacketCmd::PING, &HAL_Robot_Module::s_pingHandler);
    hostLink.registerHandler(PacketCmd::CAPS_QUERY, &HAL_Robot_Module::s_capsQueryHandler);
    hostLink.registerHandler(PacketCmd::EVENT_POST, &HAL_Robot_Module::s_eventPostHandler);
    hostLink.registerHandler(PacketCmd::SERVO_SET_POS, &HAL_Robot_Module::s_servoSetPosHandler);
    hostLink.registerHandler(PacketCmd::SERVO_SET_SPEED, &HAL_Robot_Module::s_servoSetSpeedHandler);
    hostLink.registerHandler(PacketCmd::SERVO_SET_TORQUE, &HAL_Robot_Module::s_servoSetTorqueHandler);
    hostLink.registerHandler(PacketCmd::SERVO_READ, &HAL_Robot_Module::s_servoReadHandler);
    hostLink.registerHandler(PacketCmd::TOF_READ, &HAL_Robot_Module::s_tofReadHandler);
    hostLink.registerHandler(PacketCmd::IMU_READ, &HAL_Robot_Module::s_imuReadHandler);
    hostLink.registerHandler(PacketCmd::MAG_READ, &HAL_Robot_Module::s_magReadHandler);
    hostLink.registerHandler(PacketCmd::COMPASS_HEADING, &HAL_Robot_Module::s_compassHeadingHandler);
    hostLink.registerHandler(PacketCmd::CURRENT_READ_SYS, &HAL_Robot_Module::s_currentReadSysHandler);
    hostLink.registerHandler(PacketCmd::CURRENT_READ_MOTOR, &HAL_Robot_Module::s_currentReadMotorHandler);
    hostLink.registerHandler(PacketCmd::TOUCH_READ, &HAL_Robot_Module::s_touchReadHandler);
    hostLink.registerHandler(PacketCmd::SD_LIST, &HAL_Robot_Module::s_sdListHandler);
    hostLink.registerHandler(PacketCmd::SD_READ, &HAL_Robot_Module::s_sdReadHandler);
    hostLink.registerHandler(PacketCmd::SD_WRITE, &HAL_Robot_Module::s_sdWriteHandler);
    hostLink.registerHandler(PacketCmd::SD_DELETE, &HAL_Robot_Module::s_sdDeleteHandler);
}

void HAL_Robot_Module::errorCallback(cmd_error *errorPtr)
{
    CommandError e(errorPtr);

    cliSerial->println("> ERROR: " + e.toString());

    if (e.hasCommand())
    {
        cliSerial->println("> Did you mean? " + e.getCommand().toString());
    }
    else
    {
        String list = cli.toString();
        list.replace("\r\n", "\r\n> ");
        cliSerial->println("> " + list);
    }
}

void HAL_Robot_Module::helpCallback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument argVer = cmd.getArgument("-v");
    bool _version = argVer.isSet();

    if (_version)
    {
        cliSerial->println("> ZSPR Controller Version: " + String(ZSPR_CONTROLLER_VERSION));
    }
    else
    {
        cliSerial->println("> Commands:");
        String list = cli.toString();
        list.replace("\r\n", "\r\n> ");
        cliSerial->println("> " + list);
    }
}

void HAL_Robot_Module::pingCallback(cmd *cmdPtr)
{
    cliSerial->println("> pong");
}

void HAL_Robot_Module::flushSerial()
{
    while (cliSerial->available())
        cliSerial->read();
}

void HAL_Robot_Module::drainSerialAndStop()
{
    while (cliSerial->available())
        cliSerial->read();
    cliSerial->println("> [stream] stopped");
}

void HAL_Robot_Module::testPeriphCallback(cmd *cmdPtr)
{
    Command cmd(cmdPtr);
    Argument argCur = cmd.getArgument("current");
    bool _current = argCur.isSet();

    Argument argSer = cmd.getArgument("servo");
    bool _servo = argSer.isSet();

    Argument argTOF = cmd.getArgument("tof");
    bool _tof = argTOF.isSet();

    Argument argIMU = cmd.getArgument("imu");
    bool _imu = argIMU.isSet();

    Argument argTCH = cmd.getArgument("touch");
    bool _touch = argTCH.isSet();

    Argument argBuz = cmd.getArgument("buzzer");
    bool _buzzer = argBuz.isSet();

    Argument argSDC = cmd.getArgument("sdcard");
    bool _sdcard = argSDC.isSet();

    Argument argBat = cmd.getArgument("battery");
    bool _battery = argBat.isSet();

    if ((uint8_t)(_current + _servo + _tof + _imu + _touch + _buzzer + _sdcard + _battery) > 1)
    {
        cliSerial->println("> [WARNING] Test only one peripherial at a time");
    }
    else if (_current)
    {
#if ENABLE_CURRENT_SENSE
        cliSerial->println("> [current] streaming, send any byte to stop");
        flushSerial();
        while (!cliSerial->available())
        {
            cliSerial->printf("> [SYS]   bus=%.1fmV current=%.1fmA power=%.1fmW\t\t[MTR] bus=%.1fmV current=%.1fmA power=%.1fmW\r\n",
                               current_module.currSys->readBusVoltage_mV(),
                               current_module.currSys->readCurrent_mA(),
                               current_module.currSys->readPower_mW(),
                               current_module.currMotor->readBusVoltage_mV(),
                               current_module.currMotor->readCurrent_mA(),
                               current_module.currMotor->readPower_mW());
            delay(200);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_servo)
    {
#if ENABLE_SERVO
        cliSerial->println("> [servo] streaming, send any byte to stop");
        flushSerial();
        while (!cliSerial->available())
        {
            uint8_t found = servo_module.get_found_servos();
            if (found == 0)
            {
                cliSerial->println("> [servo] no servos found");
            }
            for (uint8_t i = 0; i < found; i++)
            {
                int id = servo_module.get_servo_id(i);
                bool isST = (servo_module.st.FeedBack(id) != -1);
                bool isSC = !isST && (servo_module.sc.FeedBack(id) != -1);

                if (isST)
                {
                    cliSerial->printf("> [servo %d] pos=%d speed=%d load=%d voltage=%u current=%d temp=%d\r\n",
                                       id, servo_module.st.ReadPos(id), servo_module.st.ReadSpeed(id),
                                       servo_module.st.ReadLoad(id), servo_module.st.ReadVoltage(id),
                                       servo_module.st.ReadCurrent(id), servo_module.st.ReadTemper(id));
                }
                else if (isSC)
                {
                    cliSerial->printf("> [servo %d] pos=%d speed=%d load=%d voltage=%u current=%d temp=%d\r\n",
                                       id, servo_module.sc.ReadPos(id), servo_module.sc.ReadSpeed(id),
                                       servo_module.sc.ReadLoad(id), servo_module.sc.ReadVoltage(id),
                                       servo_module.sc.ReadCurrent(id), servo_module.sc.ReadTemper(id));
                }
                else
                {
                    cliSerial->printf("> [servo %d] no response\r\n", id);
                }
            }
            delay(200);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_tof)
    {
#if ENABLE_TOF
        cliSerial->println("> [tof] streaming, send any byte to stop");
        flushSerial();
        while (!cliSerial->available())
        {
            tof_module.update();
            if (tof_module.is_data_ready())
            {
                const VL53L8CX_ResultsData &r = tof_module.get_results();
                uint8_t zones = tof_module.get_resolution();
                cliSerial->print("> [tof] mm:");
                for (uint8_t z = 0; z < zones; z++)
                {
                    cliSerial->printf(" %4ld", (long)r.distance_mm[z]);
                }
                cliSerial->println();
            }
            else
            {
                cliSerial->println("> [tof] waiting for data...");
            }
            delay(100);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_imu)
    {
#if ENABLE_IMU_COMPASS
        cliSerial->println("> [imu] streaming, send any byte to stop");
        flushSerial();
        while (!cliSerial->available())
        {
            imu_module.update();

            float ax, ay, az, gx, gy, gz, mx, my, mz;
            imu_module.get_accel(ax, ay, az);
            imu_module.get_gyro(gx, gy, gz);
            imu_module.get_mag(mx, my, mz);

            cliSerial->printf("> [imu raw] accel(g)=%.2f,%.2f,%.2f%s gyro(dps)=%.2f,%.2f,%.2f%s mag(uT)=%.1f,%.1f,%.1f%s\r\n",
                               ax, ay, az, imu_module.is_accel_ok() ? "" : "(stale)",
                               gx, gy, gz, imu_module.is_gyro_ok() ? "" : "(stale)",
                               mx, my, mz, imu_module.is_mag_ok() ? "" : "(stale)");

            if (imu_module.is_heading_ok())
            {
                cliSerial->printf("> [imu filtered] heading=%.1fdeg\r\n", imu_module.get_heading());
            }
            else
            {
                cliSerial->println("> [imu filtered] heading=unavailable");
            }
            delay(150);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_touch)
    {
#if ENABLE_TOUCH
        cliSerial->println("> [touch] no touch driver wired up yet, send any byte to stop");
        flushSerial();
        while (!cliSerial->available())
        {
            delay(200);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_buzzer)
    {
#if ENABLE_BUZZER
        cliSerial->println("> [buzzer] sounding test pattern, send any byte to stop");
        flushSerial();
        const int notes[] = {262, 330, 392, 523};
        uint8_t idx = 0;
        while (!cliSerial->available())
        {
            cliSerial->printf("> [buzzer] tone=%dHz\r\n", notes[idx]);
            buzzer.sound(notes[idx], 200);
            idx = (idx + 1) % (sizeof(notes) / sizeof(notes[0]));
            delay(300);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_sdcard)
    {
#if ENABLE_SD
        cliSerial->println("> [sdcard] streaming, send any byte to stop");
        flushSerial();

        if (sdcard_module.is_mounted())
        {
            cliSerial->printf("> [sdcard] mounted, fatType=%u totalKB=%llu freeKB=%llu\r\n",
                               sdcard_module.fat_type(),
                               (unsigned long long)sdcard_module.total_space_kb(),
                               (unsigned long long)sdcard_module.free_space_kb());
            cliSerial->println("> [sdcard] root listing:");
            uint16_t entries = sdcard_module.print_root_listing(*cliSerial);
            cliSerial->printf("> [sdcard] %u entries\r\n", entries);
        }
        else
        {
            cliSerial->println("> [sdcard] not mounted (init failed or no card)");
        }

        while (!cliSerial->available())
        {
            cliSerial->printf("> [sdcard raw] mounted=%s detect_pin=%s\r\n",
                               sdcard_module.is_mounted() ? "yes" : "no",
                               sdcard_module.get_card_detect_raw() ? "HIGH" : "LOW");
            delay(500);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else if (_battery)
    {
#if ENABLE_BATTERY
        cliSerial->println("> [battery] streaming, send any byte to stop");
        flushSerial();

        uint32_t lastVoltReqMs = 0;
        while (!cliSerial->available())
        {
            uint32_t now = millis();
            if (now - lastVoltReqMs >= 1000)
            {
                lastVoltReqMs = now;
                battery_module.request_battery_voltage();
            }

            cliSerial->printf("> [battery] link=%s pingFails=%u pushSuspended=%s",
                               battery_module.is_link_up() ? "UP" : "DOWN",
                               battery_module.get_consecutive_ping_failures(),
                               battery_module.current_push_suspended() ? "yes" : "no");

            if (battery_module.has_battery_voltage())
                cliSerial->printf(" voltage=%.2fV\r\n", battery_module.get_battery_voltage_mV() / 1000.0f);
            else
                cliSerial->println(" voltage=unavailable");

            delay(250);
        }
        drainSerialAndStop();
#else
        cliSerial->println("> [ERROR] Peripherial not Supported");
#endif
    }
    else
    {
        cliSerial->println("\n===== TEST self-check =====");

#if ENABLE_SERVO
        cliSerial->print("[servo] responding IDs:");
        uint8_t respondCount = 0;
        for (uint8_t i = 0; i < servo_module.get_found_servos(); i++)
        {
            uint8_t id = servo_module.get_servo_id(i);
            if (servo_module.st.Ping(id))
            {
                cliSerial->printf(" %u", id);
                respondCount++;
            }
        }
        if (respondCount == 0)
            cliSerial->print(" NONE");
        cliSerial->printf(" (%u/%u)\r\n", respondCount, servo_module.get_found_servos());
#endif

#if ENABLE_TOF
        cliSerial->printf("[tof] init=%s dataReady=%s\r\n",
                           tof_module_ok == SUCCESS ? "OK" : "FAILED",
                           tof_module.is_data_ready() ? "yes" : "no");
#endif

#if ENABLE_CURRENT_SENSE
        cliSerial->printf("[curr sys]   init=%s bus=%.1fmV current=%.1fmA power=%.1fmW\r\n",
                           current_module_ok == SUCCESS ? "OK" : "FAILED",
                           current_module.currSys->readBusVoltage_mV(), current_module.currSys->readCurrent_mA(), current_module.currSys->readPower_mW());
        cliSerial->printf("[curr motor] init=%s bus=%.1fmV current=%.1fmA power=%.1fmW\r\n",
                           current_module_ok == SUCCESS ? "OK" : "FAILED",
                           current_module.currMotor->readBusVoltage_mV(), current_module.currMotor->readCurrent_mA(), current_module.currMotor->readPower_mW());
#endif

#if ENABLE_IMU_COMPASS
        imu_module.update();
        float ax, ay, az, gx, gy, gz, mx, my, mz;
        imu_module.get_accel(ax, ay, az);
        imu_module.get_gyro(gx, gy, gz);
        imu_module.get_mag(mx, my, mz);
        cliSerial->printf("[imu]     init=%s accel(g)=%.2f,%.2f,%.2f gyro(dps)=%.2f,%.2f,%.2f mag(uT)=%.1f,%.1f,%.1f\r\n",
                           imu_module_ok == SUCCESS ? "OK" : "FAILED",
                           ax, ay, az, gx, gy, gz, mx, my, mz);
        if (imu_module.is_heading_ok())
        {
            cliSerial->printf("[compass] heading=%.1fdeg\r\n", imu_module.get_heading());
        }
        else
        {
            cliSerial->println("[compass] heading=FAILED");
        }
#endif

#if ENABLE_SD
        cliSerial->printf("[sd] init=%s mounted=%s freeKB=%llu\r\n",
                           sdcard_module_ok == SUCCESS ? "OK" : "FAILED",
                           sdcard_module.is_mounted() ? "yes" : "no",
                           (unsigned long long)sdcard_module.free_space_kb());
#endif

#if ENABLE_DISPLAY
        Serial.println("[display] enabled");
#endif

#if ENABLE_BATTERY
        cliSerial->printf("[battery] init=%s link=%s pingFails=%u\r\n",
                           battery_module_ok == SUCCESS ? "OK" : "FAILED",
                           battery_module.is_link_up() ? "UP" : "DOWN",
                           battery_module.get_consecutive_ping_failures());
#endif

        cliSerial->println("============================\n");
    }
}

void HAL_Robot_Module::s_errorCallback(cmd_error *errorPtr) { _instance->errorCallback(errorPtr); }
void HAL_Robot_Module::s_helpCallback(cmd *cmdPtr) { _instance->helpCallback(cmdPtr); }
void HAL_Robot_Module::s_pingCallback(cmd *cmdPtr) { _instance->pingCallback(cmdPtr); }
void HAL_Robot_Module::s_testPeriphCallback(cmd *cmdPtr) { _instance->testPeriphCallback(cmdPtr); }
void HAL_Robot_Module::s_bottangoCallback(cmd *cmdPtr) { _instance->bottangoCallback(cmdPtr); }

void HAL_Robot_Module::bottangoCallback(cmd *cmdPtr)
{
#if ENABLE_BOTTANGO
    Command cmd(cmdPtr);
    String action = cmd.getArgument("action").getValue();
    String a[6];
    for (int i = 0; i < 6; i++)
        a[i] = cmd.getArgument(String("a") + String(i + 1)).getValue();

    if (action == "run")
    {
        // Blocking by design: nothing else (HAL loop, CLI, HostLink) is
        // serviced until the host sends "quit".
        BottangoLink::setStartAnimation(a[0].length() ? (int)a[0].toInt() : -1, a[1] == "loop");
        cliSerial->println("> Bottango running. Send \"quit\" to exit.");
        cliSerial->flush();
        bottango_module.run();
        cliSerial->println("> Bottango stopped.");
    }
    else if (action == "status")
    {
        cliSerial->println("> mode: " + String(BottangoLink::isOffline() ? "offline (exported animations)" : "live (Bottango app)"));
        cliSerial->println("> animations: " + String(BottangoLink::animationCount()));
        cliSerial->println("> servo moves sent: " + String(BottangoServoBridge::movesSent()));
    }
    else if (action == "live" || action == "offline")
    {
        cliSerial->println("> switching to " + action + ", rebooting...");
        cliSerial->flush();
        BottangoLink::setOffline(action == "offline"); // does not return
    }
    else if (action == "cal")
    {
        BottangoServoCal c;
        uint8_t id = (uint8_t)a[0].toInt();
        c.usMin = (uint16_t)a[1].toInt();
        c.usMax = (uint16_t)a[2].toInt();
        c.posMin = a[3].length() ? (int16_t)a[3].toInt() : -1;
        c.posMax = a[4].length() ? (int16_t)a[4].toInt() : -1;
        c.invert = a[5] == "inv";
        if (!BottangoServoBridge::setCalibration(id, c))
            cliSerial->println("> ERROR: bad id or usMin == usMax");
    }
    else
    {
        cliSerial->println("> ERROR: unknown action");
    }
#else
    cliSerial->println("> Bottango disabled (ENABLE_BOTTANGO)");
#endif
}

// ---------------------------------------------------------------------------
// PacketCmd handlers
// ---------------------------------------------------------------------------

uint8_t HAL_Robot_Module::capsQueryHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
    uint16_t caps = 0;
#if ENABLE_SERVO
    caps |= (uint16_t)RobotCapability::SERVO;
#endif
#if ENABLE_TOF
    caps |= (uint16_t)RobotCapability::TOF;
#endif
#if ENABLE_CURRENT_SENSE
    caps |= (uint16_t)RobotCapability::CURRENT_SENSE;
#endif
#if ENABLE_TOUCH
    caps |= (uint16_t)RobotCapability::TOUCH;
#endif
#if ENABLE_IMU_COMPASS
    caps |= (uint16_t)RobotCapability::IMU_COMPASS;
#endif
#if ENABLE_SD
    caps |= (uint16_t)RobotCapability::SD;
#endif
#if ENABLE_DISPLAY
    caps |= (uint16_t)RobotCapability::DISPLAY_PANEL;
#endif
#if ENABLE_BUZZER
    caps |= (uint16_t)RobotCapability::BUZZER;
#endif
#if ENABLE_BATTERY
    caps |= (uint16_t)RobotCapability::BATTERY;
#endif
    putU16(resp, caps);
    return ok(respLen, 2);
}

uint8_t HAL_Robot_Module::eventPostHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen)
{
    if (len < 1)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);

    uint8_t titleLen = p[0];
    uint16_t off = 1;
    if ((uint16_t)(off + titleLen + 2 + 1) > len)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);

    const uint8_t *title = p + off;
    off += titleLen;
    uint16_t cmdRaw = getU16(p + off);
    off += 2;
    uint8_t dataLen = p[off];
    off += 1;
    if (off + dataLen != len)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    const uint8_t *data = p + off;

    uint32_t id;
    if (!post_event((const char *)title, titleLen, (PacketCmd)cmdRaw, data, dataLen, id))
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);

    resp[0] = (uint8_t)(id & 0xFF);
    resp[1] = (uint8_t)((id >> 8) & 0xFF);
    resp[2] = (uint8_t)((id >> 16) & 0xFF);
    resp[3] = (uint8_t)((id >> 24) & 0xFF);
    return ok(respLen, 4);
}

uint8_t HAL_Robot_Module::servoSetPosHandler(const uint8_t *p, uint8_t len, uint8_t *, uint8_t &respLen)
{
#if ENABLE_SERVO
    if (len != 6)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    uint8_t id = p[0];
    int16_t pos = getS16(p + 1);
    uint16_t speed = getU16(p + 3);
    uint8_t acc = p[5];
    servo_module.writePosition(id, pos, speed, acc);
    return ok(respLen, 0);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::servoSetSpeedHandler(const uint8_t *p, uint8_t len, uint8_t *, uint8_t &respLen)
{
#if ENABLE_SERVO
    if (len != 4)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    uint8_t id = p[0];
    int16_t speed = getS16(p + 1);
    uint8_t acc = p[3];

    Servo_Module::Feedback fb;
    if (!servo_module.get_feedback(id, fb))
        return fail(respLen, PacketStatus::ERR_HW_FAULT);

    // Driver has no speed-only primitive -- hold the servo's last cached
    // position, just change its cruise speed.
    servo_module.writePosition(id, fb.pos, (uint16_t)speed, acc);
    return ok(respLen, 0);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::servoSetTorqueHandler(const uint8_t *p, uint8_t len, uint8_t *, uint8_t &respLen)
{
#if ENABLE_SERVO
    if (len != 2)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    servo_module.servoTorque(p[0], p[1]);
    return ok(respLen, 0);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::servoReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_SERVO
    if (len != 1)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    Servo_Module::Feedback fb;
    if (!servo_module.get_feedback(p[0], fb))
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    putS16(resp + 0, fb.pos);
    putS16(resp + 2, fb.speed);
    putS16(resp + 4, fb.load);
    resp[6] = fb.voltage;
    putS16(resp + 7, fb.temp);
    return ok(respLen, 9);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::tofReadHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_TOF
    if (!tof_module.is_data_ready())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    const VL53L8CX_ResultsData &r = tof_module.get_results();
    uint8_t zones = tof_module.get_resolution(); // 16 (4x4) or 64 (8x8)
    for (uint8_t z = 0; z < zones; z++)
        putU16(resp + 2 * z, (uint16_t)r.distance_mm[z]);
    for (uint8_t z = zones; z < 64; z++)
        putU16(resp + 2 * z, 0);
    return ok(respLen, 128); // always 64 zones' worth, zero-padded past the active resolution
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::imuReadHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_IMU_COMPASS
    float ax, ay, az, gx, gy, gz;
    imu_module.get_accel(ax, ay, az);
    imu_module.get_gyro(gx, gy, gz);
    putF32(resp + 0, ax);
    putF32(resp + 4, ay);
    putF32(resp + 8, az);
    putF32(resp + 12, gx);
    putF32(resp + 16, gy);
    putF32(resp + 20, gz);
    return ok(respLen, 24);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::magReadHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_IMU_COMPASS
    if (!imu_module.is_mag_ok())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    float mx, my, mz;
    imu_module.get_mag(mx, my, mz);
    putF32(resp + 0, mx);
    putF32(resp + 4, my);
    putF32(resp + 8, mz);
    return ok(respLen, 12);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::compassHeadingHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_IMU_COMPASS
    if (!imu_module.is_heading_ok())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    putF32(resp, imu_module.get_heading());
    return ok(respLen, 4);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::currentReadSysHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_CURRENT_SENSE
    return current_module.readCurrentSensor(current_module.currSys, resp, respLen);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::currentReadMotorHandler(const uint8_t *, uint8_t, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_CURRENT_SENSE
    return current_module.readCurrentSensor(current_module.currMotor, resp, respLen);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::touchReadHandler(const uint8_t *, uint8_t, uint8_t *, uint8_t &respLen)
{
    return fail(respLen, PacketStatus::ERR_DISABLED); // ENABLE_TOUCH=0, no driver wired up yet
}

uint8_t HAL_Robot_Module::sdListHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_SD
    bool rootRequested = (len == 0) ||
                          (len == 1 && (p[0] == '\0' || p[0] == '/')) ||
                          (len == 2 && p[0] == '/' && p[1] == '\0');
    if (!rootRequested)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS); // only root listing is supported
    if (!sdcard_module.is_mounted())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);

    BufferPrint out(resp, PROTOCOL_MAX_PAYLOAD - 1);
    sdcard_module.print_root_listing(out);
    return ok(respLen, out.length());
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::sdReadHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_SD
    if (len == 0 || p[len - 1] != '\0')
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    if (!sdcard_module.is_mounted())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);

    SDCard_Module::File f = sdcard_module.open((const char *)p, O_RDONLY);
    if (!f)
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    int n = f.read(resp, PROTOCOL_MAX_PAYLOAD - 1);
    f.close();
    if (n < 0)
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    return ok(respLen, (uint8_t)n); // truncated past ~199 bytes -- no chunking protocol yet
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::sdWriteHandler(const uint8_t *p, uint8_t len, uint8_t *resp, uint8_t &respLen)
{
#if ENABLE_SD
    uint8_t pathLen = 0;
    while (pathLen < len && p[pathLen] != '\0')
        pathLen++;
    if (pathLen == len)
        return fail(respLen, PacketStatus::ERR_BAD_ARGS); // no NUL found
    if (!sdcard_module.is_mounted())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);

    const char *path = (const char *)p;
    const uint8_t *data = p + pathLen + 1;
    uint8_t dataLen = len - pathLen - 1;

    SDCard_Module::File f = sdcard_module.open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (!f)
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    size_t written = f.write(data, dataLen);
    f.close();

    uint32_t w = (uint32_t)written;
    resp[0] = (uint8_t)(w & 0xFF);
    resp[1] = (uint8_t)((w >> 8) & 0xFF);
    resp[2] = (uint8_t)((w >> 16) & 0xFF);
    resp[3] = (uint8_t)((w >> 24) & 0xFF);
    return ok(respLen, 4);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::sdDeleteHandler(const uint8_t *p, uint8_t len, uint8_t *, uint8_t &respLen)
{
#if ENABLE_SD
    if (len == 0 || p[len - 1] != '\0')
        return fail(respLen, PacketStatus::ERR_BAD_ARGS);
    if (!sdcard_module.is_mounted())
        return fail(respLen, PacketStatus::ERR_HW_FAULT);
    bool removed = sdcard_module.remove((const char *)p);
    return removed ? ok(respLen, 0) : fail(respLen, PacketStatus::ERR_HW_FAULT);
#else
    return fail(respLen, PacketStatus::ERR_DISABLED);
#endif
}

uint8_t HAL_Robot_Module::s_pingHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return pingHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_capsQueryHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->capsQueryHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_eventPostHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->eventPostHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_servoSetPosHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->servoSetPosHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_servoSetSpeedHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->servoSetSpeedHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_servoSetTorqueHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->servoSetTorqueHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_servoReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->servoReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_tofReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->tofReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_imuReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->imuReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_magReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->magReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_compassHeadingHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->compassHeadingHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_currentReadSysHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->currentReadSysHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_currentReadMotorHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->currentReadMotorHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_touchReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->touchReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_sdListHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->sdListHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_sdReadHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->sdReadHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_sdWriteHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->sdWriteHandler(p, l, r, rl); }
uint8_t HAL_Robot_Module::s_sdDeleteHandler(const uint8_t *p, uint8_t l, uint8_t *r, uint8_t &rl) { return _instance->sdDeleteHandler(p, l, r, rl); }
