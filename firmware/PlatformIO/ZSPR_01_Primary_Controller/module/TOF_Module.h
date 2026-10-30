#pragma once

#include <Arduino.h>
#include "Universal_Module.h"
#include <vl53l8cx.h>
#include "hardware_config.h"
#include "board_io.h"

#define VL53L8CX_NB_SPAD 64

// Property / command IDs for TOF_Module::get/set/command.
enum class TofProp : uint16_t
{
    Ready,         // get  BOOL   new ranging frame available
    Resolution,    // get/set UINT  16 (4x4) or 64 (8x8)
    Distances,     // get  BYTES  distance_mm[], resolution * VL53L8CX_NB_TARGET_PER_ZONE entries
    SignalAmbient, // get/set BOOL  report signal + ambient in print_result
};

enum class TofCmd : uint16_t
{
    ToggleResolution = 0U,
    ToggleSignalAmbient,
};

class TOF_Module : public Universal_Module
{
public:
    TOF_Module(bool enable) : Universal_Module(enable)
    {
        sensor_vl53l8cx_top = new VL53L8CX(&SPI, BOARD_VLx_NCS);
        _buses = BUS_MASK_SPI; // chip select goes through the expander, which locks I2C itself
    }

    return_codes_t setup() override
    {
        return_codes_t code = ERROR;
        if (_enabled)
        {
            board_digitalWrite(BOARD_VLx_SPI_N, HIGH);
            board_digitalWrite(BOARD_VLx_LPN, HIGH);

            // Configure VL53L8CX component.
            sensor_vl53l8cx_top->begin();
            _status = sensor_vl53l8cx_top->init();

            if (_status == VL53L8CX_STATUS_OK)
            {
                // Start Measurements
                _status = sensor_vl53l8cx_top->start_ranging();
                _initialized = (_status == VL53L8CX_STATUS_OK);
            }

            uint8_t isDataReady = 0;
            _status = sensor_vl53l8cx_top->check_data_ready(&isDataReady);

            if (_status == VL53L8CX_STATUS_OK)
                code = SUCCESS;
            else if (_status == VL53L8CX_STATUS_TIMEOUT_ERROR)
                code = TIMEOUT;  
                
           NewDataReady = isDataReady;
        }
        return code;
    }

    return_codes_t update() override
    {
        return_codes_t code = ERROR;
        if (_enabled)
        {
            if (!_initialized)
                return code;

            uint8_t isDataReady = 0;

            // do
            // {
            //     _status = sensor_vl53l8cx_top->check_data_ready(&NewDataReady);
            // } while (!NewDataReady);
            _status = sensor_vl53l8cx_top->check_data_ready(&isDataReady);
            if (!isDataReady)
                return TIMEOUT;

            if ((!_status) && (isDataReady != 0))
            {
                _status = sensor_vl53l8cx_top->get_ranging_data(&_resultData);
            }

            if (_status == VL53L8CX_STATUS_OK)
                code = SUCCESS;
            else if (_status == VL53L8CX_STATUS_TIMEOUT_ERROR)
                code = TIMEOUT;

            NewDataReady = isDataReady;
        }

        return code;
    }

    /****************/
    /* TOF COMMANDS */
    /****************/
    uint8_t is_data_ready(void)
    {
        // Serial.printf("TOF is Data ready value: %d\r\n", NewDataReady);
        return NewDataReady;
    }

    const VL53L8CX_ResultsData &get_results(void) const
    {
        return _resultData;
    }

    uint8_t get_resolution(void) const
    {
        return _res;
    }

    void print_result(VL53L8CX_ResultsData *Result)
    {
        int8_t i, j, k;
        uint8_t l, zones_per_line;
        uint8_t number_of_zones = _res;

        zones_per_line = (number_of_zones == 16) ? 4 : 8;

        // display_commands_banner();

        Serial.print("Cell Format :\n\n");

        for (l = 0; l < VL53L8CX_NB_TARGET_PER_ZONE; l++)
        {
            snprintf(report, sizeof(report), " \033[38;5;10m%20s\033[0m : %20s\n", "Distance [mm]", "Status");
            Serial.print(report);

            if (_EnableAmbient || _EnableSignal)
            {
                snprintf(report, sizeof(report), " %20s : %20s\n", "Signal [kcps/spad]", "Ambient [kcps/spad]");
                Serial.print(report);
            }
        }

        Serial.print("\n\n");

        for (j = 0; j < number_of_zones; j += zones_per_line)
        {
            for (i = 0; i < zones_per_line; i++)
            {
                Serial.print(" -----------------");
            }
            Serial.print("\n");

            for (i = 0; i < zones_per_line; i++)
            {
                Serial.print("|                 ");
            }
            Serial.print("|\n");

            for (l = 0; l < VL53L8CX_NB_TARGET_PER_ZONE; l++)
            {
                // Print distance and status
                for (k = (zones_per_line - 1); k >= 0; k--)
                {
                    if (Result->nb_target_detected[j + k] > 0)
                    {
                        snprintf(report, sizeof(report), "| \033[38;5;10m%5ld\033[0m  :  %5ld ",
                                 (long)Result->distance_mm[(VL53L8CX_NB_TARGET_PER_ZONE * (j + k)) + l],
                                 (long)Result->target_status[(VL53L8CX_NB_TARGET_PER_ZONE * (j + k)) + l]);
                        Serial.print(report);
                    }
                    else
                    {
                        snprintf(report, sizeof(report), "| %5s  :  %5s ", "X", "X");
                        Serial.print(report);
                    }
                }
                Serial.print("|\n");

                if (_EnableAmbient || _EnableSignal)
                {
                    // Print Signal and Ambient
                    for (k = (zones_per_line - 1); k >= 0; k--)
                    {
                        if (Result->nb_target_detected[j + k] > 0)
                        {
                            if (_EnableSignal)
                            {
                                snprintf(report, sizeof(report), "| %5ld  :  ", (long)Result->signal_per_spad[(VL53L8CX_NB_TARGET_PER_ZONE * (j + k)) + l]);
                                Serial.print(report);
                            }
                            else
                            {
                                snprintf(report, sizeof(report), "| %5s  :  ", "X");
                                Serial.print(report);
                            }
                            if (_EnableAmbient)
                            {
                                snprintf(report, sizeof(report), "%5ld ", (long)Result->ambient_per_spad[j + k]);
                                Serial.print(report);
                            }
                            else
                            {
                                snprintf(report, sizeof(report), "%5s ", "X");
                                Serial.print(report);
                            }
                        }
                        else
                        {
                            snprintf(report, sizeof(report), "| %5s  :  %5s ", "X", "X");
                            Serial.print(report);
                        }
                    }
                    Serial.print("|\n");
                }
            }
        }
        for (i = 0; i < zones_per_line; i++)
        {
            Serial.print(" -----------------");
        }
        Serial.print("\n");
    }

    void toggle_resolution(void)
    {
        _status = sensor_vl53l8cx_top->stop_ranging();

        switch (_res)
        {
        case VL53L8CX_RESOLUTION_4X4:
            _res = VL53L8CX_RESOLUTION_8X8;
            break;

        case VL53L8CX_RESOLUTION_8X8:
            _res = VL53L8CX_RESOLUTION_4X4;
            break;

        default:
            break;
        }
        _status = sensor_vl53l8cx_top->set_resolution(_res);
        _status = sensor_vl53l8cx_top->start_ranging();
    }

    void toggle_signal_and_ambient(void)
    {
        _EnableAmbient = (_EnableAmbient) ? false : true;
        _EnableSignal = (_EnableSignal) ? false : true;
    }

protected:
    return_codes_t onGet(uint16_t id, Value &out) override
    {
        switch ((TofProp)id)
        {
        case TofProp::Ready:
            out = Value(NewDataReady != 0);
            return SUCCESS;
        case TofProp::Resolution:
            out = Value((uint32_t)_res);
            return SUCCESS;
        case TofProp::Distances:
            out = Value(_resultData.distance_mm,
                        (uint16_t)(_res * VL53L8CX_NB_TARGET_PER_ZONE * sizeof(_resultData.distance_mm[0])));
            return SUCCESS;
        case TofProp::SignalAmbient:
            out = Value(_EnableAmbient && _EnableSignal);
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onSet(uint16_t id, const Value &in) override
    {
        switch ((TofProp)id)
        {
        case TofProp::Resolution:
        {
            uint32_t want = in.asUInt();
            if (want != VL53L8CX_RESOLUTION_4X4 && want != VL53L8CX_RESOLUTION_8X8)
                return INVALID_PARAM;
            if (want != _res)
                toggle_resolution(); // only two resolutions, so a change is a toggle
            return SUCCESS;
        }
        case TofProp::SignalAmbient:
        {
            bool on = in.asBool();
            _EnableAmbient = on;
            _EnableSignal = on;
            return SUCCESS;
        }
        default:
            return NOT_SUPPORTED;
        }
    }

    return_codes_t onCommand(uint16_t id, const Value &in, Value &out) override
    {
        (void)in;
        (void)out;
        switch ((TofCmd)id)
        {
        case TofCmd::ToggleResolution:
            toggle_resolution();
            return SUCCESS;
        case TofCmd::ToggleSignalAmbient:
            toggle_signal_and_ambient();
            return SUCCESS;
        default:
            return NOT_SUPPORTED;
        }
    }

private:
    VL53L8CX *sensor_vl53l8cx_top;

    uint8_t NewDataReady = 0;
    bool _EnableAmbient = false;
    bool _EnableSignal = false;
    uint8_t _res = VL53L8CX_RESOLUTION_4X4;
    char report[256];
    uint8_t _status;
    VL53L8CX_ResultsData _resultData;
};