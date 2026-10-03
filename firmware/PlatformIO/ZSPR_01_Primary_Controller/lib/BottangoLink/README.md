# BottangoLink

Runs the [Bottango](https://www.bottango.com) animation app against the robot's SC/ST bus servos, live over USB or offline from exported animations.

```
Bottango app --USB--> BottangoLink::run() --> Bottango driver (lib/Bottango)
exported animations (flash) -----------------------------------^         |
                                                      virtual PCA9685 (Adafruit_PWMServoDriver.h shim)
                                                                          v
                                          BottangoServoBridge --> Bottango_Module --> Servo_Module --> servos
```

## Libraries

| Library | Where | Why |
|---|---|---|
| Bottango Arduino driver (BSD-3, API v9) | `lib/Bottango` (vendored, see `ZSPR_PATCHES.md`) | Protocol, handshake, curve evaluation, offline playback |
| Adafruit PWM Servo Driver (+ BusIO) | **not needed** | The driver's PCA9685 code is satisfied by the shim here; no PCA9685 exists on this robot |
| SCServo | already in `lib_deps` | SC/ST bus protocol |

## Bottango app setup

1. In a serial terminal on the robot's USB port run `bottango run` (optionally `bottango run <animation> [loop]` in offline mode). This **blocks**: nothing else (HAL loop, CLI, HostLink, other peripherals) runs until the host sends the line `quit`. Close the terminal, then connect Bottango to the same port (any baud); to leave, send `quit` from a terminal (Bottango's own disconnect/STOP only deregisters effectors).
2. Add each servo as an **I2C servo (PCA9685)**: *channel = SC servo ID* (0-15), address ignored (use 0x40). Set the min/max pulse to the travel you want.
3. Pulse width -> position: `usMin..usMax` (default 500..2500 us) maps linearly onto the servo's range (SC 20..1003, ST 0..4095). Override per servo with `bottango cal <id> <usMin> <usMax> <posMin> <posMax> [inv]` (not persisted across reboot).

## Speed / timing

Bottango evaluates its curves on the robot and emits a new pulse per loop. Every 20 ms (`Bottango_Module::BOTTANGO_UPDATE_MS`) changed servos get one `SyncWrite`: SC servos get `time = ms since their last move`, ST servos get `speed = |delta pos| / that time`, so the servo glides between samples. The first move after connect/registration takes 300 ms. Max speed and acceleration limits set in Bottango still apply (the driver enforces them before the pulse reaches the shim).

## Offline animations

1. In Bottango: *Save animations to code*, copy the generated `GeneratedCodeAnimations.h/.cpp` over the stubs in `lib/Bottango/`, rebuild and flash.
2. `bottango offline` persists the mode and reboots; `bottango live` switches back; `bottango status` shows it. Animations only play while `bottango run` is active, started per their Bottango config (play on start, idle, pin trigger) or by `bottango run <n> [loop]`.

## Limits

- Pin servos, steppers, audio and relay are not wired to anything on this robot.
- SD-card playback is not enabled (code export only).
- Servos hold position after STOP; torque is not released.
