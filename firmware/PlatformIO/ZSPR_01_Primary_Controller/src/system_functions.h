#pragma once

#include <cstring>
#include <cstdlib>
#include <math.h>

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SimpleCLI.h"
#include "HostLink.h"
#include "protocol_handlers.h"
#include "KTS1622.h"
#include "Buzzer.h"

#include "peripheral_config.h"
#include "hardware_config.h"
#include "board_io.h"
#include "Current_Module.h"
#include "Servo_Module.h"
#include "Display_Module.h"
#include "IMU_Module.h"
#include "TOF_Module.h"
#include "SDCard_Module.h"
#include "Battery_Module.h"

#define ZSPR_CLI_VERSION "0.0.1"

SimpleCLI cli;
Stream *cliSerial;

Command cmdHelp;
Command cmdPheriph_Test;

const uint8_t addrs[] = {KTS1622_ADDR_VDD, KTS1622_ADDR_GND}; // up to 4
KTS1622_IO_Expander expander(2, addrs, Wire);
bool expander_ok = false;

#if ENABLE_BUZZER
Buzzer buzzer(BOARD_BUZZER);
#endif

#if ENABLE_CURRENT_SENSE
Current_Module current_module(ENABLE_CURRENT_SENSE);
return_codes_t current_module_ok = ERROR;
#endif

#if ENABLE_SERVO
Servo_Module servo_module(ENABLE_SERVO);
return_codes_t servo_module_ok = ERROR;
#endif

#if ENABLE_DISPLAY
Display_Module display_module(ENABLE_DISPLAY);
#endif

#if ENABLE_IMU_COMPASS
IMU_Module imu_module(ENABLE_IMU_COMPASS);
return_codes_t imu_module_ok = ERROR;
#endif

#if ENABLE_TOF
TOF_Module tof_module(ENABLE_TOF);
return_codes_t tof_module_ok = ERROR;
#endif

#if ENABLE_SD
SDCard_Module sdcard_module(ENABLE_SD);
return_codes_t sdcard_module_ok = ERROR;
#endif

#if ENABLE_BATTERY
Battery_Module battery_module(ENABLE_BATTERY);
return_codes_t battery_module_ok = ERROR;

#if ENABLE_CURRENT_SENSE
// Supplies live INA232 readings for Battery_Module's periodic 250ms
// SEND_CURRENT_POWER push -- keeps Battery_Module.h from needing to include
// Current_Module.h itself.
Battery_Module::CurrentReadings battery_current_source()
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
#endif
#endif

void hardware_setup(void)
{
  /* Initialize serial communication */
  Serial.begin(115200);                                                // USB CDC Connection

  SPI.begin(BOARD_SCLK, BOARD_MISO, BOARD_MOSI);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(400000); // Set I2C frequency to 400kHz

  // ESP IO Config
  board_pinMode(BOARD_IO2_INT, INPUT);
  // attachInterrupt(digitalPinToInterrupt(BOARD_IO2_INT), io_int_isr, FALLING);
  board_pinMode(BOARD_IO2_RST, OUTPUT);
  board_digitalWrite(BOARD_IO2_RST, HIGH);

  const uint8_t rst_pins[] = {BOARD_IO2_RST, BOARD_IO1_RST};
  // expander_ok = expander.begin(rst_pins);
  expander_ok = expander.begin();

  // This board's KTS1622 chain: module 0's INT -> native BOARD_IO_INT,
  // module 1's INT -> module 0's own BOARD_IO2_INT pin (see hardware_config.h).
  // board_configureInterruptLine(BOARD_IO1_INT);
  // board_setInterruptCascadePin(1, BOARD_IO1_INT);

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
  battery_module.attach_current_source(battery_current_source);
#endif
  battery_module_ok = battery_module.setup();
#endif
}

void hardware_update(void)
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

/**************************/
/* COMMAND LINE INTERFACE */
/**************************/

void errorCallback(cmd_error *errorPtr)
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

void helpCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Argument argVer = cmd.getArgument("-v");
  bool _version = argVer.isSet();

  if (_version)
  {
    cliSerial->println("> ZSPR CLI Version: " + String(ZSPR_CLI_VERSION));
  }
  else
  {
    cliSerial->println("> Commands:");
    String list = cli.toString();
    list.replace("\r\n", "\r\n> ");
    cliSerial->println("> " + list);
  }
}

/**************************/
/******** COMMANDS ********/
/**************************/
void pingCallback(cmd *cmdPtr)
{
  cliSerial->println("> pong");
}

// Consumes whatever byte(s) broke a streaming test loop out of Serial's RX
// buffer, so they don't get reprocessed as a stray CLI line, then confirms.
void flushSerial()
{
  while (cliSerial->available())
    cliSerial->read();
}
void drainSerialAndStop()
{
  while (cliSerial->available())
    cliSerial->read();
  cliSerial->println("> [stream] stopped");
}

// Testing Peripherials
void testPeriphCallback(cmd *cmdPtr)
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
        // battery_module.send_ping();
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

#if ENABLE_TOUCH
    Serial.printf("[touch] init=%s mask=0x%02X\r\n", touch_ok ? "OK" : "FAILED", touch.keyMask());
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
    Serial.printf("[display] panel=%ux%u\r\n", panel.width(), panel.height());
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

/**************************************/
/*            CLI SETUP               */
/**************************************/

void system_setup()
{
  hardware_setup();
  cliSerial = &Serial;

  cli.setOnError(errorCallback);
  cmdHelp = cli.addCommand("help", helpCallback);
  cmdHelp.setDescription("Prints this help message");
  cmdHelp.addFlagArgument("-v");

  cmdPheriph_Test = cli.addCommand("test_periph/erials", testPeriphCallback);
  cmdPheriph_Test.setDescription("Test peripherials in report or individually stream data");
  cmdPheriph_Test.addFlagArgument("current");
  cmdPheriph_Test.addFlagArgument("servo");
  cmdPheriph_Test.addFlagArgument("tof");
  cmdPheriph_Test.addFlagArgument("imu");
  cmdPheriph_Test.addFlagArgument("touch");
  cmdPheriph_Test.addFlagArgument("buzzer");
  cmdPheriph_Test.addFlagArgument("sdcard");
  cmdPheriph_Test.addFlagArgument("battery");

  cli.addCommand("ping", pingCallback);

  // Same command set answers identically whether it comes in as a typed
  // CLI line above or a binary PacketCmd frame from the Raspberry Pi.
  hostLink.begin(Serial, cli);
  hostLink.registerHandler(PacketCmd::PING, pingHandler);
}

void system_loop()
{
  hostLink.poll();
  // board_serviceInterrupts();
  hardware_update();
}