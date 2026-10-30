# SCServo -- local RTOS patches

Copied from workloads/SCServo 1.0.1 (waveshareteam/ugv_base_general). Changes:

* `Bus::Servo` recursive mutex (lib/BusLock) is held for every bus transaction
  and every method that touches shared state (`Err`, `Error`, `Mem[]`). `SCSCL`
  and `SMS_STS` share one UART, so the lock is per bus, not per object.
  Lock order: module mutex -> SPI -> I2C -> Servo.
* `SCSerial::readSCS()` busy-waited up to `IOTimeOut` with `millis()`. It now
  spins for `SCS_SPIN_US` and then blocks a tick at a time.
* `Err` is initialised in the constructors.
* `syncReadPacketTx/Rx` + `syncReadRxPacketTo*` keep per-object state across
  calls; a caller that uses them must hold `BusLock(Bus::Servo)` across all of them.
