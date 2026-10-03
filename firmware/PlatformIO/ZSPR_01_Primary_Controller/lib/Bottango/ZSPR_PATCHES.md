# Local changes to the vendored Bottango driver

Upstream: https://github.com/EvanBottango/Bottango (commit dbbe5f3, driver 0.8.0d1, API v9), BSD-3-Clause (see LICENSE).

Re-vendoring: copy `BottangoArduinoDriver/src` over `src/`, then re-apply:

1. `src/BasicCommands.cpp`, `src/BottangoCore.cpp`, `src/Outgoing.cpp`:
   `Serial.` -> `BOTTANGO_SERIAL.` and the `Serial.begin(BAUD_RATE);` inside
   `initUSBSerialComms()` removed (HostLink owns/starts the port).
   `sed -i 's/\bSerial\.begin(BAUD_RATE);//; s/\bSerial\./BOTTANGO_SERIAL./g' <those 3 files>`
2. `BottangoArduinoConfig.h`: declares `Stream &bottangoSerial()` + `BOTTANGO_SERIAL`.
3. `BottangoArduinoModules.h`: `USE_CODE_COMMAND_STREAM`, `ENABLE_DYNAMIC_ANIMATION_SOURCE_SWITCH`
   and `USE_ADAFRUIT_PWM_LIBRARY` enabled (the last is satisfied by the shim in lib/BottangoLink).
4. `BottangoArduinoCallbacks.cpp` is NOT vendored; lib/BottangoLink implements the `Callbacks::` namespace.
5. `GeneratedCodeAnimations.{h,cpp}` is the file Bottango's "Save animations to code" export
   produces. Overwrite the stub in this folder with your export.
