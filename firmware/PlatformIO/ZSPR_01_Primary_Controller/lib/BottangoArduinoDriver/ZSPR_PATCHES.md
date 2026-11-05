# ZSPR changes to the vendored Bottango driver

Upstream: https://github.com/EvanBottango/Bottango (driver 0.8.0d1, API v9), BSD-3-Clause.
Everything else under this folder is upstream, unmodified.

Build flags (platformio.ini) that switch the patches on:
`-DZSPR_SERVO_SHIM -DZSPR_NO_REBOOT_ON_STOP -Ilib/BottangoArduinoDriver -Ilib/ZSPR_Bottango_Driver`

Re-vendoring: copy upstream over this folder, then re-apply:

1. `src/PinServoEffector.h`: include `ZSPR_Servo_Shim.h` instead of `ESP32Servo.h` when
   `ZSPR_SERVO_SHIM` is defined. The shim maps a Bottango pin servo to the SC09 bus servo
   with the same ID.
2. `src/BottangoCore.cpp`, `stop()`: skip `BasicCommands::reboot(false)` when
   `ZSPR_NO_REBOOT_ON_STOP` is defined. Upstream resets the board when the app sends "stop";
   here "stop" returns the robot to CLI mode instead (Bottango_Module / HAL set_mode()).
3. `library.json` (ZSPR): compiles `src/` but not `src/lib/ESP32Servo` (its `Servo` would clash
   with the shim's) and not `BottangoArduinoCallbacks.cpp` (ZSPR_Bottango_Callbacks.cpp
   provides the `Callbacks::` functions).

Note: `BottangoCore::uninitialize()` clears `initialized` but not `handshake`, and the app's
second handshake is ignored while it is set. Bottango_Module clears it after stopping.
