#pragma once

#include <Arduino.h>
#include <cstring>
#include <esp_task_wdt.h>
#include "Universal_Module.h"
#include "SimpleCLI.h"
#include "peripheral_config.h"

// Property/command enums for the modules test_periph knows how to report on.
#include "Current_Module.h"
#include "Servo_Module.h"
#include "IMU_Module.h"
#include "TOF_Module.h"
#include "SDCard_Module.h"
#include "Battery_Module.h"
#include "Cap_Touch_Module.h"

// Typed serial command line. Runs in its own task and talks to every other
// module only through their public get/set/command (each locks per module), so
// it is safe alongside the peripheral task.
//
// Usage:
//     cli.addModule("tof", tof);      // before setup(): names become test_periph flags
//     cli.run_setup();
//     ... task loop: cli.run_update();
class CLI_Module : public Universal_Module
{
public:
    CLI_Module(bool enable, Stream *serial) : Universal_Module(enable), _cliSerial(serial)
    {
        instanceStorage() = this;
    }

    // Registers `module` under `name`, the flag test_periph accepts for it
    // ("test_periph tof"). Call before setup(). The module must outlive the CLI.
    bool addModule(const char *name, Universal_Module *module)
    {
        if (!name || !module || _moduleCount >= MAX_MODULES)
            return false;
        _registry[_moduleCount].name = name;
        _registry[_moduleCount].module = module;
        _moduleCount++;
        return true;
    }

    return_codes_t setup() override
    {
        if (!_enabled)
            return ERROR;

        cli.setOnError(CLI_Module::errorCallback);
        cmdHelp = cli.addCommand("help", CLI_Module::helpCallback);
        cmdHelp.setDescription("Prints this help message");
        cmdHelp.addFlagArgument("-v");

        cmdPheriph_Test = cli.addCommand("test_periph/erials", CLI_Module::testPeriphCallback);
        cmdPheriph_Test.setDescription("Test peripherials in report or individually stream data");
        for (uint8_t i = 0; i < _moduleCount; i++)
        {
            cmdPheriph_Test.addFlagArgument(_registry[i].name);
        }

        cli.addCommand("ping", CLI_Module::pingCallback);

        _initialized = true;
        return SUCCESS;
    }

    // Non-blocking: consumes whatever bytes are waiting, echoes them, and hands
    // each completed line to the parser.
    return_codes_t update() override
    {
        if (!_enabled || !_initialized)
            return ERROR;

        while (_cliSerial->available())
        {
            uint8_t b = (uint8_t)_cliSerial->read();

            if (_skipLF)
            {
                _skipLF = false;
                if (b == '\n') // second half of a CR LF pair
                    continue;
            }

            if (b == '\b' || b == 0x7F)
            {
                if (_lineLen > 0)
                {
                    _lineLen--;
                    _cliSerial->write((const uint8_t *)"\b \b", 3);
                }
                continue;
            }

            _cliSerial->write(b); // raw serial terminals don't echo for us

            if (b == '\r' || b == '\n')
            {
                if (b == '\r')
                {
                    _cliSerial->write('\n');
                    _skipLF = true;
                }
                if (_lineLen > 0)
                {
                    _line[_lineLen] = '\0';
                    cli.parse(_line, _lineLen);
                    _lineLen = 0;
                }
                continue;
            }

            if (_lineLen < CLI_LINE_LEN - 1)
                _line[_lineLen++] = (char)b;
        }
        return SUCCESS;
    }

protected:
    void flushSerial()
    {
        while (_cliSerial->available())
            _cliSerial->read();
    }

    void drainSerialAndStop()
    {
        while (_cliSerial->available())
            _cliSerial->read();
        _cliSerial->println("> [stream] stopped");
    }

    static void errorCallback(cmd_error *errorPtr)
    {
        CLI_Module *instance = instanceStorage();
        if (!instance || !instance->_cliSerial)
        {
            return;
        }

        CommandError e(errorPtr);

        instance->_cliSerial->println("> ERROR: " + e.toString());

        if (e.hasCommand())
        {
            instance->_cliSerial->println("> Did you mean? " + e.getCommand().toString());
        }
        else
        {
            String list = instance->cli.toString();
            list.replace("\r\n", "\r\n> ");
            instance->_cliSerial->println("> " + list);
        }
    }

    static void helpCallback(cmd *cmdPtr)
    {
        CLI_Module *instance = instanceStorage();
        if (!instance || !instance->_cliSerial)
        {
            return;
        }

        Command cmd(cmdPtr);
        Argument argVer = cmd.getArgument("-v");
        bool _version = argVer.isSet();

        if (_version)
        {
            instance->_cliSerial->println("> ZSPR Controller Version: " + String(ZSPR_CONTROLLER_VERSION));
        }
        else
        {
            instance->_cliSerial->println("> Commands:");
            String list = instance->cli.toString();
            list.replace("\r\n", "\r\n> ");
            instance->_cliSerial->println("> " + list);
        }
    }

    static void pingCallback(cmd *cmdPtr)
    {
        CLI_Module *instance = instanceStorage();
        if (!instance || !instance->_cliSerial)
        {
            return;
        }

        instance->_cliSerial->println("> pong");
    }

    static void testPeriphCallback(cmd *cmdPtr)
    {
        CLI_Module *self = instanceStorage();
        if (!self || !self->_cliSerial)
        {
            return;
        }

        Command cmd(cmdPtr);
        self->runTestPeriph(cmd);
    }

    // ------------------------------------------------------------------
    // test_periph
    // ------------------------------------------------------------------
    struct Entry
    {
        const char *name = nullptr;
        Universal_Module *module = nullptr;
    };

    using ReportFn = void (*)(CLI_Module &, Universal_Module &);
    struct Reporter
    {
        const char *name;
        ReportFn fn;
        uint16_t periodMs; // stream rate
    };

    static const Reporter *reporterFor(const char *name)
    {
        static const Reporter table[] = {
            {"current", &CLI_Module::reportCurrent, 200},
            {"servo", &CLI_Module::reportServo, 200},
            {"tof", &CLI_Module::reportTof, 100},
            {"imu", &CLI_Module::reportImu, 150},
            {"touch", &CLI_Module::reportCapTouch, 100},
            {"sdcard", &CLI_Module::reportSd, 500},
            {"battery", &CLI_Module::reportBattery, 250},
        };
        for (const Reporter &r : table)
        {
            if (strcmp(r.name, name) == 0)
                return &r;
        }
        return nullptr;
    }

    void runTestPeriph(Command &cmd)
    {
        const Entry *selected = nullptr;
        uint8_t count = 0;
        for (uint8_t i = 0; i < _moduleCount; i++)
        {
            if (cmd.getArgument(_registry[i].name).isSet())
            {
                selected = &_registry[i];
                count++;
            }
        }

        if (count > 1)
        {
            _cliSerial->println("> [WARNING] Test only one peripherial at a time");
        }
        else if (count == 1)
        {
            streamModule(*selected);
        }
        else
        {
            selfCheck();
        }
    }

    // Reports one module until a byte arrives on the serial port.
    void streamModule(const Entry &e)
    {
        const Reporter *r = reporterFor(e.name);
        if (!r)
        {
            _cliSerial->printf("> [%s] no test available\r\n", e.name);
            return;
        }
        if (!e.module->enabled())
        {
            _cliSerial->printf("> [%s] disabled\r\n", e.name);
            return;
        }

        _cliSerial->printf("> [%s] streaming, send any byte to stop\r\n", e.name);
        flushSerial();
        while (!_cliSerial->available())
        {
            r->fn(*this, *e.module);
            esp_task_wdt_reset(); // streaming blocks inside the CLI task; keep it fed
            vTaskDelay(pdMS_TO_TICKS(r->periodMs));
        }
        drainSerialAndStop();
    }

    // One report from every enabled module that has one.
    void selfCheck()
    {
        _cliSerial->println("\n===== TEST self-check =====");
        for (uint8_t i = 0; i < _moduleCount; i++)
        {
            const Reporter *r = reporterFor(_registry[i].name);
            if (r && _registry[i].module->enabled())
            {
                r->fn(*this, *_registry[i].module);
            }
        }
        _cliSerial->println("============================\n");
    }

    // ------------------------------------------------------------------
    // Reporters: read through get() only; an error from get() means the
    // module isn't initialised or the value isn't ready, and is shown as
    // "unavailable".
    // ------------------------------------------------------------------
    template <typename E>
    static bool readInt(Universal_Module &m, E id, int32_t &out)
    {
        Value v;
        if (m.get(id, v) != SUCCESS)
            return false;
        out = v.asInt();
        return true;
    }

    template <typename E>
    static bool readFloat(Universal_Module &m, E id, float &out)
    {
        Value v;
        if (m.get(id, v) != SUCCESS)
            return false;
        out = v.asFloat();
        return true;
    }

    static void reportCurrent(CLI_Module &c, Universal_Module &m)
    {
        static const CurrentProp props[6] = {
            CurrentProp::SysVoltage_mV, CurrentProp::SysCurrent_mA, CurrentProp::SysPower_mW,
            CurrentProp::MotorVoltage_mV, CurrentProp::MotorCurrent_mA, CurrentProp::MotorPower_mW};
        int32_t v[6];
        for (uint8_t i = 0; i < 6; i++)
        {
            if (!readInt(m, props[i], v[i]))
            {
                c._cliSerial->println("> [current] unavailable");
                return;
            }
        }
        c._cliSerial->printf("> [SYS]   bus=%ldmV current=%ldmA power=%ldmW\t\t[MTR] bus=%ldmV current=%ldmA power=%ldmW\r\n",
                             (long)v[0], (long)v[1], (long)v[2], (long)v[3], (long)v[4], (long)v[5]);
    }

    static void reportServo(CLI_Module &c, Universal_Module &m)
    {
        int32_t found;
        if (!readInt(m, Servo_Module::addr(ServoProp::Count, 0), found))
        {
            c._cliSerial->println("> [servo] unavailable");
            return;
        }
        if (found == 0)
        {
            c._cliSerial->println("> [servo] no servos found");
            return;
        }

        for (int32_t i = 0; i < found; i++)
        {
            int32_t id;
            if (!readInt(m, Servo_Module::addr(ServoProp::IdAt, (uint8_t)i), id))
                continue;

            int32_t pos, speed, load, volt, curr, temp;
            bool ok = readInt(m, Servo_Module::addr(ServoProp::Position, (uint8_t)id), pos) &&
                      readInt(m, Servo_Module::addr(ServoProp::Speed, (uint8_t)id), speed) &&
                      readInt(m, Servo_Module::addr(ServoProp::Load, (uint8_t)id), load) &&
                      readInt(m, Servo_Module::addr(ServoProp::Voltage, (uint8_t)id), volt) &&
                      readInt(m, Servo_Module::addr(ServoProp::Current, (uint8_t)id), curr) &&
                      readInt(m, Servo_Module::addr(ServoProp::Temp, (uint8_t)id), temp);
            if (ok)
            {
                c._cliSerial->printf("> [servo %ld] pos=%ld speed=%ld load=%ld voltage=%ld current=%ld temp=%ld\r\n",
                                     (long)id, (long)pos, (long)speed, (long)load, (long)volt, (long)curr, (long)temp);
            }
            else
            {
                c._cliSerial->printf("> [servo %ld] no response\r\n", (long)id);
            }
        }
    }

    static void reportTof(CLI_Module &c, Universal_Module &m)
    {
        Value v;
        if (m.get(TofProp::Ready, v) != SUCCESS)
        {
            c._cliSerial->println("> [tof] unavailable");
            return;
        }
        if (!v.asBool())
        {
            c._cliSerial->println("> [tof] waiting for data...");
            return;
        }

        int32_t zones;
        if (!readInt(m, TofProp::Resolution, zones))
            return;

        // Distances points into the module's own frame buffer: hold its lock
        // while we read it so update() can't overwrite it mid-print.
        Universal_Module::DataLock lock(m);
        if (m.get(TofProp::Distances, v) != SUCCESS || !v.data)
            return;
        const int16_t *d = (const int16_t *)v.data;

        c._cliSerial->print("> [tof] mm:");
        for (int32_t z = 0; z < zones; z++)
        {
            c._cliSerial->printf(" %4ld", (long)d[z * VL53L8CX_NB_TARGET_PER_ZONE]);
        }
        c._cliSerial->println();
    }

    static bool readVec3(Universal_Module &m, ImuProp prop, float (&dst)[3])
    {
        Value v;
        if (m.get(prop, v) != SUCCESS || v.type != Value::BYTES || v.len != sizeof(dst))
            return false;
        memcpy(dst, v.data, sizeof(dst));
        return true;
    }

    static void reportImu(CLI_Module &c, Universal_Module &m)
    {
        float a[3], g[3], mg[3];
        Value ok;
        bool aOk = false, gOk = false, mOk = false, hOk = false;
        float heading = 0;

        // One lock around the whole snapshot so all four values are from the
        // same update().
        {
            Universal_Module::DataLock lock(m);
            if (!readVec3(m, ImuProp::Accel, a) || !readVec3(m, ImuProp::Gyro, g) || !readVec3(m, ImuProp::Mag, mg))
            {
                c._cliSerial->println("> [imu] unavailable");
                return;
            }
            aOk = m.get(ImuProp::AccelOk, ok) == SUCCESS && ok.asBool();
            gOk = m.get(ImuProp::GyroOk, ok) == SUCCESS && ok.asBool();
            mOk = m.get(ImuProp::MagOk, ok) == SUCCESS && ok.asBool();
            hOk = m.get(ImuProp::HeadingOk, ok) == SUCCESS && ok.asBool();
            readFloat(m, ImuProp::Heading, heading);
        }

        c._cliSerial->printf("> [imu raw] accel(g)=%.2f,%.2f,%.2f%s gyro(dps)=%.2f,%.2f,%.2f%s mag(uT)=%.1f,%.1f,%.1f%s\r\n",
                             a[0], a[1], a[2], aOk ? "" : "(stale)",
                             g[0], g[1], g[2], gOk ? "" : "(stale)",
                             mg[0], mg[1], mg[2], mOk ? "" : "(stale)");
        if (hOk)
            c._cliSerial->printf("> [imu filtered] heading=%.1fdeg\r\n", heading);
        else
            c._cliSerial->println("> [imu filtered] heading=unavailable");
    }

    static void reportCapTouch(CLI_Module &c, Universal_Module &m)
    {
        int32_t mask;
        if (!readInt(m, CapTouchProp::Touched, mask))
        {
            c._cliSerial->println("> [touch] unavailable");
            return;
        }
        c._cliSerial->printf("> [touch] keys=0x%02lX%s\r\n", (unsigned long)mask, mask ? " (touched)" : "");
    }

    static void reportSd(CLI_Module &c, Universal_Module &m)
    {
        Value v;
        if (m.get(SdProp::Mounted, v) != SUCCESS)
        {
            c._cliSerial->println("> [sdcard] unavailable");
            return;
        }
        bool mounted = v.asBool();
        bool detect = m.get(SdProp::CardDetect, v) == SUCCESS && v.asBool();

        if (!mounted)
        {
            c._cliSerial->printf("> [sdcard] not mounted (init failed or no card) detect_pin=%s\r\n", detect ? "HIGH" : "LOW");
            return;
        }

        int32_t fat = 0, total = 0, freeKb = 0;
        readInt(m, SdProp::FatType, fat);
        readInt(m, SdProp::TotalKB, total);
        readInt(m, SdProp::FreeKB, freeKb);
        c._cliSerial->printf("> [sdcard] mounted fatType=%ld totalKB=%lu freeKB=%lu detect_pin=%s\r\n",
                             (long)fat, (unsigned long)total, (unsigned long)freeKb, detect ? "HIGH" : "LOW");
    }

    static void reportBattery(CLI_Module &c, Universal_Module &m)
    {
        // The voltage comes back asynchronously: ask once a second, show the
        // last answer.
        uint32_t now = millis();
        if (now - c._lastVoltReqMs >= 1000)
        {
            c._lastVoltReqMs = now;
            m.command(BatteryAction::RequestVoltage);
        }

        Value v;
        if (m.get(BatteryProp::LinkUp, v) != SUCCESS)
        {
            c._cliSerial->println("> [battery] unavailable");
            return;
        }
        bool link = v.asBool();
        int32_t fails = 0;
        readInt(m, BatteryProp::PingFailures, fails);
        bool suspended = m.get(BatteryProp::PushSuspended, v) == SUCCESS && v.asBool();

        c._cliSerial->printf("> [battery] link=%s pingFails=%ld pushSuspended=%s",
                             link ? "UP" : "DOWN", (long)fails, suspended ? "yes" : "no");

        int32_t mv;
        if (readInt(m, BatteryProp::VoltageMv, mv))
            c._cliSerial->printf(" voltage=%.2fV\r\n", mv / 1000.0f);
        else
            c._cliSerial->println(" voltage=unavailable");
    }

private:
    static CLI_Module *&instanceStorage()
    {
        static CLI_Module *instance = nullptr;
        return instance;
    }

    static constexpr uint8_t MAX_MODULES = 12;
    static constexpr size_t CLI_LINE_LEN = 128;

    Entry _registry[MAX_MODULES];
    uint8_t _moduleCount = 0;

    char _line[CLI_LINE_LEN];
    size_t _lineLen = 0;
    bool _skipLF = false;
    uint32_t _lastVoltReqMs = 0;

    SimpleCLI cli;
    Stream *_cliSerial = &Serial;

    Command cmdHelp;
    Command cmdPheriph_Test;
};
