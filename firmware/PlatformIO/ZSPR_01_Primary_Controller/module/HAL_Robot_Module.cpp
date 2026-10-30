#include "HAL_Robot_Module.h"
#include <cstring>
#include <esp_task_wdt.h>

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
// Lifecycle
// ---------------------------------------------------------------------------
HAL_Robot_Module::HAL_Robot_Module()
{
    _instance = this;
    // List of modules to add to CLI
    _modules[(uint8_t)Peripheral::Current] = new Current_Module((bool)ENABLE_CURRENT_SENSE);
    _modules[(uint8_t)Peripheral::IMU] = new IMU_Module((bool)ENABLE_IMU_COMPASS);
    _modules[(uint8_t)Peripheral::TOF] = new TOF_Module((bool)ENABLE_TOF);
    _modules[(uint8_t)Peripheral::SDCard] = new SDCard_Module((bool)ENABLE_SD);
    _modules[(uint8_t)Peripheral::Battery] = new Battery_Module((bool)ENABLE_BATTERY);
    _modules[(uint8_t)Peripheral::CapTouch] = new Cap_Touch_Module((bool)ENABLE_CAP_TOUCH);
}

void HAL_Robot_Module::init()
{
    _instance = this;
    // No ENABLE_ flag: these two are always present (the flags were removed
    // from peripheral_config.h).
    display = new Display_Module(true);
    servo = new Servo_Module(true);
    cli = new CLI_Module(true, &Serial);

    hardware_setup();

    // All module setup() has finished, so from here on the modules are shared
    // between tasks and go through their locks.
    BaseType_t created = xTaskCreatePinnedToCore(&HAL_Robot_Module::s_peripheralTask, "peripherals",
                                                 PERIPHERAL_TASK_STACK_BYTES, this,
                                                 PERIPHERAL_TASK_PRIORITY, &_peripheralTask,
                                                 PERIPHERAL_TASK_CORE);
    if (created != pdPASS)
    {
        Serial.println("[hal] failed to start peripheral task");
    }

    BaseType_t cli_created = xTaskCreatePinnedToCore(&HAL_Robot_Module::s_cliTask, "cli",
                                                     CLI_TASK_STACK_BYTES, this,
                                                     CLI_TASK_PRIORITY, &_cliTask,
                                                     CLI_TASK_CORE);
    if (cli_created != pdPASS)
    {
        Serial.println("[hal] failed to start CLI task");
    }

    if (display->initialized())
    {
        BaseType_t display_created = xTaskCreatePinnedToCore(&HAL_Robot_Module::s_displayTask, "display",
                                                             DISPLAY_TASK_STACK_BYTES, this,
                                                             DISPLAY_TASK_PRIORITY, &_displayTask,
                                                             DISPLAY_TASK_CORE);
        if (display_created != pdPASS)
        {
            Serial.println("[hal] failed to start display task");
        }
    }

    if (servo->enabled())
    {
        BaseType_t servo_created = xTaskCreatePinnedToCore(&HAL_Robot_Module::s_servoTask, "servo",
                                                           SERVO_TASK_STACK_BYTES, this,
                                                           SERVO_TASK_PRIORITY, &_servoTask,
                                                           SERVO_TASK_CORE);
        if (servo_created != pdPASS)
        {
            Serial.println("[hal] failed to start servo task");
        }
    }
}

void HAL_Robot_Module::update()
{
    // Peripherals run in their own task; don't spin loop() at full speed.
    vTaskDelay(pdMS_TO_TICKS(100));
}

void HAL_Robot_Module::s_peripheralTask(void *arg)
{
    static_cast<HAL_Robot_Module *>(arg)->hardware_update();
}

// ---------------------------------------------------------------------------
// Hardware setup/update -- moved from src/system_functions.h, reaching
// members instead of globals. Bodies otherwise unchanged.
// ---------------------------------------------------------------------------

void HAL_Robot_Module::hardware_setup()
{
    /* Initialize serial communication */
    Serial.begin(115200); // USB CDC Connection
    // Serial.setDebugOutput(true);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(400000); // Set I2C frequency to 400kHz

    // ESP IO Config
    board_pinMode(BOARD_IO_INT, INPUT);
    board_pinMode(BOARD_IO_RST, OUTPUT);
    board_digitalWrite(BOARD_IO_RST, HIGH);

    expander_ok = expander.begin();

    board_pinMode(BOARD_VLx_SPI_N, OUTPUT);
    board_pinMode(BOARD_VLx_NCS, OUTPUT);
    board_pinMode(BOARD_VLx_SYNC, OUTPUT);
    board_pinMode(BOARD_VLx_INT, INPUT);
    board_pinMode(BOARD_VLx_LPN, OUTPUT);
    board_pinMode(BOARD_SENSE_RST, OUTPUT);
    board_pinMode(BOARD_SENSE_CHG, OUTPUT);
    board_pinMode(BOARD_TFT_RST, OUTPUT);
    board_pinMode(BOARD_TFT_CS, OUTPUT);
    board_pinMode(BOARD_TFT_BL, OUTPUT);
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
    board_pinMode(BOARD_LED2, OUTPUT);
    board_pinMode(BOARD_CHG_EN, OUTPUT);
    board_pinMode(BOARD_CHG_DETECT, INPUT);
    board_pinMode(BOARD_CURR_ALERT1, INPUT);
    board_pinMode(BOARD_CURR_ALERT2, INPUT);
    board_pinMode(BOARD_REG_EN, OUTPUT);

    board_digitalWrite(BOARD_REG_EN, LOW);

#if TEST
    board_digitalWrite(BOARD_LED, LOW);
    board_digitalWrite(BOARD_LED2, LOW);
#endif

    // Keep the other SPI devices deselected while the display owns SCLK/MOSI.
    board_digitalWrite(BOARD_VLx_NCS, HIGH);
    board_digitalWrite(BOARD_SDMMC_CS, HIGH);

    display->run_setup();
    SPI.begin(BOARD_SCLK, BOARD_MISO, BOARD_MOSI);

    servo->run_setup();

    // Names are the test_periph flags; they must be registered before the
    // CLI's setup() so it can build the command.
    cli->addModule("current", module(Peripheral::Current));
    cli->addModule("imu", module(Peripheral::IMU));
    cli->addModule("tof", module(Peripheral::TOF));
    cli->addModule("sdcard", module(Peripheral::SDCard));
    cli->addModule("battery", module(Peripheral::Battery));
    cli->addModule("touch", module(Peripheral::CapTouch));
    cli->addModule("servo", servo);
    cli->run_setup();

#if ENABLE_BATTERY && ENABLE_CURRENT_SENSE
    static_cast<Battery_Module *>(module(Peripheral::Battery))->attach_current_source(&HAL_Robot_Module::s_battery_current_source);
#endif

    // Pins are configured above, so every enabled module can now come up.
    for (uint8_t i = 0; i < TOTAL_PERIPHERIALS; i++)
    {
        if (_modules[i] && _modules[i]->enabled())
        {
            _module_ok[i] = _modules[i]->run_setup();
        }
    }
}

// Peripheral task body. Fixed-rate loop so a slow module doesn't change how
// often the others run; each module takes its own locks inside run_update().
void HAL_Robot_Module::hardware_update()
{
    // Subscribe this task to the task watchdog. The system already runs the
    // TWDT (and watches the idle task), so adjust its timeout rather than
    // re-initialising it; fall back to init if it somehow isn't running.
    esp_task_wdt_config_t wdt = {};
    wdt.timeout_ms = PERIPHERAL_WDT_TIMEOUT_MS;
    wdt.trigger_panic = PERIPHERAL_WDT_PANIC;
#ifdef CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0
    wdt.idle_core_mask |= (1 << 0);
#endif
#ifdef CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1
    wdt.idle_core_mask |= (1 << 1);
#endif
    if (esp_task_wdt_reconfigure(&wdt) == ESP_ERR_INVALID_STATE)
    {
        esp_task_wdt_init(&wdt);
    }
    if (esp_task_wdt_add(nullptr) != ESP_OK)
    {
        Serial.println("[hal] peripheral task could not join the task watchdog");
    }

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(PERIPHERAL_TASK_PERIOD_MS);

    while (1)
    {
        for (uint8_t i = 0; i < TOTAL_PERIPHERIALS; i++)
        {
            if (_modules[i] && _modules[i]->enabled())
            {
                _modules[i]->run_update();
            }
        }

        // Toggle the LED to show the peripheral task is running every second
        board_digitalWrite(BOARD_LED, !board_digitalRead(BOARD_LED));
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second

        esp_task_wdt_reset(); // one feed per full pass

        // If the pass overran the period xTaskDelayUntil() returns without
        // blocking; yield a tick anyway so the idle task on this core still
        // runs (and feeds its watchdog) and lower-priority tasks get a turn.
        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}


/*******************/
/*    CLI TASK     */
/*******************/
void HAL_Robot_Module::s_cliTask(void *arg)
{
    static_cast<HAL_Robot_Module *>(arg)->cli_update();
}

// The TWDT is already configured by the peripheral task; this just subscribes
// the CLI task to it. Long-running commands (test_periph streaming) feed it
// themselves.
void HAL_Robot_Module::cli_update()
{
    if (esp_task_wdt_add(nullptr) != ESP_OK)
    {
        Serial.println("[hal] CLI task could not join the task watchdog");
    }

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(CLI_TASK_PERIOD_MS);

    while (1)
    {
        cli->run_update();
        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}

/***********************/
/*    DISPLAY TASK     */
/***********************/
void HAL_Robot_Module::s_displayTask(void *arg)
{
    static_cast<HAL_Robot_Module *>(arg)->display_update();
}

// Touch polling only -- drawing is the UI task's job (src/app/UI_Task).
void HAL_Robot_Module::display_update()
{
    if (esp_task_wdt_add(nullptr) != ESP_OK)
    {
        Serial.println("[hal] display task could not join the task watchdog");
    }

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(DISPLAY_TASK_PERIOD_MS);

    while (1)
    {
        display->run_update();
        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}

/*********************/
/*    SERVO TASK     */
/*********************/
void HAL_Robot_Module::s_servoTask(void *arg)
{
    static_cast<HAL_Robot_Module *>(arg)->servo_update();
}

// Polls servo feedback. Commands from other tasks (CLI, Bottango) interleave
// between servos through the module and servo-bus locks. The servo module
// also feeds the watchdog between servos, since a silent one stalls its read.
void HAL_Robot_Module::servo_update()
{
    if (esp_task_wdt_add(nullptr) != ESP_OK)
    {
        Serial.println("[hal] servo task could not join the task watchdog");
    }

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(SERVO_TASK_PERIOD_MS);

    while (1)
    {
        servo->run_update();
        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}

/***********************/
/*    BATTERY COMM     */
/***********************/
#if ENABLE_BATTERY && ENABLE_CURRENT_SENSE
// Called from the battery module's update (peripheral task). Reads the values
// Current_Module cached in its own update() through its locked get() instead
// of poking the INA232s directly.
Battery_Module::CurrentReadings HAL_Robot_Module::battery_current_source()
{
    Universal_Module *current = module(Peripheral::Current);
    auto read = [current](CurrentProp prop) -> int32_t
    {
        Value v;
        return current->get(prop, v) == SUCCESS ? v.asInt() : 0;
    };
    return {
        (uint16_t)read(CurrentProp::SysVoltage_mV),
        (int16_t)read(CurrentProp::SysCurrent_mA),
        (uint16_t)read(CurrentProp::SysPower_mW),
        (uint16_t)read(CurrentProp::MotorVoltage_mV),
        (int16_t)read(CurrentProp::MotorCurrent_mA),
        (uint16_t)read(CurrentProp::MotorPower_mW),
    };
}

Battery_Module::CurrentReadings HAL_Robot_Module::s_battery_current_source()
{
    return _instance->battery_current_source();
}
#endif
