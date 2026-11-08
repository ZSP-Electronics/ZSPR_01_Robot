#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "peripheral_config.h"
#include "Servo_Module.h" // MAX_SERVO_NUM

class HAL_Robot_Module;

// ---------------------------------------------------------------------------
// ZSPR-01 joint table. A "slot" is an index into the servo module's table; the order is
// that of the servo ID defines in Servo_Module.h.
// ---------------------------------------------------------------------------
static constexpr const char *SERVO_SLOT_NAME[MAX_SERVO_NUM] = {
    "ST Left",             // ST_SERVO_ID_LEFT
    "ST Right",            // ST_SERVO_ID_RIGHT
    "Front Right",         // FR_SERVO_ID
    "Front Left",          // FL_SERVO_ID
    "Arm Extension Right", // AER_SERVO_ID
    "Arm Shoulder Right",  // ASR_SERVO_ID
    "Arm Rotate Right",    // ARR_SERVO_ID
    "Arm Extension Left",  // AEL_SERVO_ID
    "Arm Shoulder Left",   // ASL_SERVO_ID
    "Arm Rotate Left",     // ARL_SERVO_ID
    "Eye Arm Right",       // EAR_SERVO_ID
    "Eye Arm Left",        // EAL_SERVO_ID
};
// Slots 0 and 1 are the ST wheel servos. They are driven by wheel code, not joint
// control: configuration screens leave them out, and they have no calibration or limits.
static constexpr uint8_t SERVO_FIRST_CONFIGURABLE_SLOT = 2;
// Joints with calibration and limits, and so the size of the stored lists.
static constexpr uint8_t SERVO_CAL_COUNT = MAX_SERVO_NUM - SERVO_FIRST_CONFIGURABLE_SLOT;

// Calibration reference position for each slot, as a raw SC servo position (0..1023,
// middle 511): the pose the horns are meant to be assembled in. Edit to suit.
static constexpr int16_t SERVO_CAL_POSITION[MAX_SERVO_NUM] = {
    511, 511, // ST Left, ST Right (unused)
    511,      // Front Right
    511,      // Front Left
    511,      // Arm Extension Right
    511,      // Arm Shoulder Right
    511,      // Arm Rotate Right
    511,      // Arm Extension Left
    511,      // Arm Shoulder Left
    511,      // Arm Rotate Left
    511,      // Eye Arm Right
    511,      // Eye Arm Left
};

// Joint limits used until a different range is stored (the range Servo_Module::setMode
// gives an SC servo).
#define ROBOT_JOINT_DEFAULT_MIN 20
#define ROBOT_JOINT_DEFAULT_MAX 1003

#define ROBOT_CAL_SETTLE_MS 2000 // time allowed for the servos to reach the reference pose

// Robot control for the ZSPR-01: joint naming, calibration, joint limits and joint
// targets, all in one task. The servo module only moves and reads servos; this class
// decides where they should go.
//
// Positions in this class are CALIBRATED units: the reference pose (SERVO_CAL_POSITION)
// is where a calibrated joint reads when it is physically at the reference. The raw
// servo position sent to the bus is  pos + calibration offset , clamped to the joint's
// limits first. Offset, minimum and maximum are stored in NVS (keys servo_ofs, servo_min,
// servo_max: SERVO_CAL_COUNT int16 each, in slot order starting at slot 2).
//
// Joint targets: any task may call setJointTarget(); the robot task sends changed
// targets to the servos each period. Kinematics goes in Robot_Procedure::step(), which
// would turn a body-level goal into joint targets.
//
// Calibration (driven by the task so the UI never blocks):
//   calibrationStart()    Moving -> Settling -> Releasing -> Released
//                         (servos at the reference pose, torque off: set the horns by hand)
//   calibrationConfirm()  Finishing: torque back on holding position, positions read,
//                         offset = read - reference stored -> Done, or Failed
//   calibrationAbort()    torque back on, nothing stored
//   calibrationReset()    Done/Failed -> Idle, after the caller has seen the result
// ST-type servos (the wheels, or any ST-type servo in a joint slot) are not calibrated.
class Robot_Procedure
{
public:
    enum class CalState : uint8_t
    {
        Idle,
        Moving,    // sending each servo to the reference pose, one per period
        Settling,  // waiting for them to arrive
        Releasing, // torque off, one per period
        Released,  // complete; torque is off, waiting for calibrationConfirm()
        Finishing, // torque on, reading positions, saving
        Done,      // offsets stored
        Failed     // a servo didn't answer or the save failed (torque is on)
    };

    explicit Robot_Procedure(HAL_Robot_Module &robot);

    // Loads the stored calibration and limits, then starts the task. Call after the
    // servo module's setup().
    void begin();

    // --- names / table ---------------------------------------------------------
    static const char *slotName(uint8_t slot) { return slot < MAX_SERVO_NUM ? SERVO_SLOT_NAME[slot] : ""; }
    // A joint slot holding an SC-type servo that was found on the bus.
    bool calibratable(uint8_t slot) const;

    // --- calibration -----------------------------------------------------------
    bool calibrationAvailable() const; // servo module up and Bottango not holding the bus
    bool calibrationStart();           // false if unavailable or already running
    void calibrationConfirm();         // only from Released
    void calibrationAbort();
    void calibrationReset();
    CalState calibrationState() const { return _cal; }
    bool calibrationActive() const { return _cal != CalState::Idle && _cal != CalState::Done && _cal != CalState::Failed; }

    // Stored offset (raw SC units) for a joint slot; 0 if never calibrated.
    int16_t calibrationOffset(uint8_t slot) const;

    // --- limits ----------------------------------------------------------------
    int16_t jointMin(uint8_t slot) const;
    int16_t jointMax(uint8_t slot) const;
    // Stores a new range for a joint slot (0 <= min < max <= 1023). False if invalid or
    // the save failed.
    bool setJointLimits(uint8_t slot, int16_t minPos, int16_t maxPos);

    // --- limit capture ---------------------------------------------------------
    // For setting a joint's range by hand. limitCaptureBegin() turns every joint's torque
    // off so the horns move freely (and blocks joint targets and calibration);
    // limitCapture() records a joint's current position as its minimum or maximum in a
    // pending list; limitCaptureEnd(true) stores the pending list as the joint limits
    // (NVS keys servo_min / servo_max) and puts torque back, each joint holding where it
    // is. Positions are calibrated units. A captured min above max is swapped, and a
    // joint whose min equals its max keeps its old limits.
    bool limitCaptureBegin();
    bool limitCaptureActive() const { return _freeMove; }
    bool limitCapture(uint8_t slot, bool isMax, int16_t *captured = nullptr); // false if the servo did not answer
    int16_t pendingMin(uint8_t slot) const;
    int16_t pendingMax(uint8_t slot) const;
    bool limitCaptureEnd(bool save);

    // --- joint control ---------------------------------------------------------
    // Queues a target for a joint slot, in calibrated units; it is clamped to the
    // joint's limits. Ignored while calibrating or in Bottango mode. speed 0 = default.
    bool setJointTarget(uint8_t slot, int16_t pos, uint16_t speed = 0);

private:
    HAL_Robot_Module &_robot;
    TaskHandle_t _task = nullptr;
    SemaphoreHandle_t _mutex;

    volatile CalState _cal = CalState::Idle;
    volatile bool _abortRequested = false;
    volatile bool _freeMove = false; // limit capture: torque off, hands off the servos
    int16_t _pendMin[SERVO_CAL_COUNT];
    int16_t _pendMax[SERVO_CAL_COUNT];
    uint8_t _calSlot = 0;
    uint32_t _calT0 = 0;

    int16_t _offset[SERVO_CAL_COUNT] = {};
    int16_t _min[SERVO_CAL_COUNT];
    int16_t _max[SERVO_CAL_COUNT];

    int16_t _target[SERVO_CAL_COUNT] = {};
    uint16_t _targetSpeed[SERVO_CAL_COUNT] = {};
    uint16_t _dirty = 0; // bit n: joint slot n+SERVO_FIRST_CONFIGURABLE_SLOT has an unsent target

    void loadStored();
    uint8_t nextSlot(uint8_t from) const;
    bool controlAllowed() const;

    void run();     // task body, never returns
    void step();    // one period: procedures, then joint targets
    void stepCalibration();
    void sendTargets();
    bool finishCalibration(bool store);
    static void s_task(void *arg);
};
