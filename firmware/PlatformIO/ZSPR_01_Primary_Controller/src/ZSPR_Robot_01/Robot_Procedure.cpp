#include "Robot_Procedure.h"
#include "HAL_Robot_Module.h"
#include "NVS_Store.h"
#include <esp_task_wdt.h>
#include <string.h>

static const char *KEY_OFFSET = "servo_ofs";
static const char *KEY_MIN = "servo_min";
static const char *KEY_MAX = "servo_max";

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

Robot_Procedure::Robot_Procedure(HAL_Robot_Module &robot) : _robot(robot)
{
    _mutex = xSemaphoreCreateMutex();
    for (uint8_t i = 0; i < SERVO_CAL_COUNT; i++)
    {
        _min[i] = ROBOT_JOINT_DEFAULT_MIN;
        _max[i] = ROBOT_JOINT_DEFAULT_MAX;
        _pendMin[i] = _min[i];
        _pendMax[i] = _max[i];
    }
}

void Robot_Procedure::begin()
{
    if (_task)
        return;
    loadStored();
    xTaskCreatePinnedToCore(&Robot_Procedure::s_task, "robot", ROBOT_TASK_STACK_BYTES, this,
                            ROBOT_TASK_PRIORITY, &_task, ROBOT_TASK_CORE);
}

void Robot_Procedure::loadStored()
{
    int16_t offsets[SERVO_CAL_COUNT], mins[SERVO_CAL_COUNT], maxs[SERVO_CAL_COUNT];
    const bool haveOfs = NVS::loadArray(KEY_OFFSET, offsets, SERVO_CAL_COUNT);
    const bool haveMin = NVS::loadArray(KEY_MIN, mins, SERVO_CAL_COUNT);
    const bool haveMax = NVS::loadArray(KEY_MAX, maxs, SERVO_CAL_COUNT);

    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (haveOfs)
        memcpy(_offset, offsets, sizeof(_offset));
    if (haveMin && haveMax)
    {
        for (uint8_t i = 0; i < SERVO_CAL_COUNT; i++)
        {
            // Keep the default for an entry that makes no sense.
            if (mins[i] >= 0 && maxs[i] <= 1023 && mins[i] < maxs[i])
            {
                _min[i] = mins[i];
                _max[i] = maxs[i];
            }
        }
    }
    memcpy(_pendMin, _min, sizeof(_pendMin));
    memcpy(_pendMax, _max, sizeof(_pendMax));
    xSemaphoreGive(_mutex);
}

// ---------------------------------------------------------------------------
// Table / state
// ---------------------------------------------------------------------------
bool Robot_Procedure::calibratable(uint8_t slot) const
{
    Servo_Module *servo = _robot.servo;
    if (!servo || !servo->enabled() || !servo->initialized())
        return false;
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return false;
    return servo->servoTypeAt(slot) == SCSERVOTYPE;
}

bool Robot_Procedure::calibrationAvailable() const
{
    Servo_Module *servo = _robot.servo;
    return servo && servo->enabled() && servo->initialized() && _robot.mode() != ControlMode::Bottango && !_freeMove;
}

// Joint control is allowed when nothing else owns the servos.
bool Robot_Procedure::controlAllowed() const
{
    return calibrationAvailable() && !calibrationActive();
}

uint8_t Robot_Procedure::nextSlot(uint8_t from) const
{
    for (uint8_t s = from < SERVO_FIRST_CONFIGURABLE_SLOT ? SERVO_FIRST_CONFIGURABLE_SLOT : from; s < MAX_SERVO_NUM; s++)
    {
        if (calibratable(s))
            return s;
    }
    return MAX_SERVO_NUM;
}

int16_t Robot_Procedure::calibrationOffset(uint8_t slot) const
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return 0;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t v = _offset[slot - SERVO_FIRST_CONFIGURABLE_SLOT];
    xSemaphoreGive(_mutex);
    return v;
}

int16_t Robot_Procedure::jointMin(uint8_t slot) const
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return 0;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t v = _min[slot - SERVO_FIRST_CONFIGURABLE_SLOT];
    xSemaphoreGive(_mutex);
    return v;
}

int16_t Robot_Procedure::jointMax(uint8_t slot) const
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return 0;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t v = _max[slot - SERVO_FIRST_CONFIGURABLE_SLOT];
    xSemaphoreGive(_mutex);
    return v;
}

bool Robot_Procedure::setJointLimits(uint8_t slot, int16_t minPos, int16_t maxPos)
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return false;
    if (minPos < 0 || maxPos > 1023 || minPos >= maxPos)
        return false;
    int16_t mins[SERVO_CAL_COUNT], maxs[SERVO_CAL_COUNT];
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _min[slot - SERVO_FIRST_CONFIGURABLE_SLOT] = minPos;
    _max[slot - SERVO_FIRST_CONFIGURABLE_SLOT] = maxPos;
    memcpy(mins, _min, sizeof(mins));
    memcpy(maxs, _max, sizeof(maxs));
    xSemaphoreGive(_mutex);
    return NVS::saveArray(KEY_MIN, mins, SERVO_CAL_COUNT) && NVS::saveArray(KEY_MAX, maxs, SERVO_CAL_COUNT);
}

// ---------------------------------------------------------------------------
// Limit capture
// ---------------------------------------------------------------------------
bool Robot_Procedure::limitCaptureBegin()
{
    if (_freeMove)
        return true;
    Servo_Module *servo = _robot.servo;
    if (!servo || !servo->enabled() || !servo->initialized() || _robot.mode() == ControlMode::Bottango ||
        calibrationActive())
        return false;

    _freeMove = true; // first: the task stops sending targets
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memcpy(_pendMin, _min, sizeof(_pendMin));
    memcpy(_pendMax, _max, sizeof(_pendMax));
    _dirty = 0;
    xSemaphoreGive(_mutex);

    for (uint8_t slot = SERVO_FIRST_CONFIGURABLE_SLOT; slot < MAX_SERVO_NUM; slot++)
    {
        if (calibratable(slot))
            servo->torqueAt(slot, false);
    }
    return true;
}

bool Robot_Procedure::limitCapture(uint8_t slot, bool isMax, int16_t *captured)
{
    if (!_freeMove || !calibratable(slot))
        return false;
    const int raw = _robot.servo->readPositionAt(slot);
    if (raw < 0)
        return false;
    const uint8_t i = slot - SERVO_FIRST_CONFIGURABLE_SLOT;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t pos = (int16_t)clampi(raw - _offset[i], 0, 1023); // calibrated units
    (isMax ? _pendMax : _pendMin)[i] = pos;
    xSemaphoreGive(_mutex);
    if (captured)
        *captured = pos;
    return true;
}

int16_t Robot_Procedure::pendingMin(uint8_t slot) const
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return 0;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t v = _pendMin[slot - SERVO_FIRST_CONFIGURABLE_SLOT];
    xSemaphoreGive(_mutex);
    return v;
}

int16_t Robot_Procedure::pendingMax(uint8_t slot) const
{
    if (slot < SERVO_FIRST_CONFIGURABLE_SLOT || slot >= MAX_SERVO_NUM)
        return 0;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    const int16_t v = _pendMax[slot - SERVO_FIRST_CONFIGURABLE_SLOT];
    xSemaphoreGive(_mutex);
    return v;
}

bool Robot_Procedure::limitCaptureEnd(bool save)
{
    if (!_freeMove)
        return true;
    Servo_Module *servo = _robot.servo;

    // Torque back on, each joint holding where it was left.
    for (uint8_t slot = SERVO_FIRST_CONFIGURABLE_SLOT; slot < MAX_SERVO_NUM; slot++)
    {
        if (!calibratable(slot))
            continue;
        const int pos = servo->readPositionAt(slot);
        if (pos >= 0)
            servo->writePositionAt(slot, (int16_t)pos, ServoInitSpeed_SC, ServoInitACC_SC);
        servo->torqueAt(slot, true);
        esp_task_wdt_reset();
    }

    bool ok = true;
    int16_t mins[SERVO_CAL_COUNT], maxs[SERVO_CAL_COUNT];
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (save)
    {
        for (uint8_t i = 0; i < SERVO_CAL_COUNT; i++)
        {
            int16_t lo = _pendMin[i], hi = _pendMax[i];
            if (lo > hi)
            {
                const int16_t t = lo;
                lo = hi;
                hi = t;
            }
            if (lo < hi)
            {
                _min[i] = lo;
                _max[i] = hi;
            }
        }
    }
    // The pending list starts from the live limits again either way.
    memcpy(_pendMin, _min, sizeof(_pendMin));
    memcpy(_pendMax, _max, sizeof(_pendMax));
    memcpy(mins, _min, sizeof(mins));
    memcpy(maxs, _max, sizeof(maxs));
    xSemaphoreGive(_mutex);

    if (save)
        ok = NVS::saveArray(KEY_MIN, mins, SERVO_CAL_COUNT) && NVS::saveArray(KEY_MAX, maxs, SERVO_CAL_COUNT);
    _freeMove = false;
    return ok;
}

// ---------------------------------------------------------------------------
// Joint control
// ---------------------------------------------------------------------------
bool Robot_Procedure::setJointTarget(uint8_t slot, int16_t pos, uint16_t speed)
{
    if (!controlAllowed() || !calibratable(slot))
        return false;
    const uint8_t i = slot - SERVO_FIRST_CONFIGURABLE_SLOT;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _target[i] = pos;
    _targetSpeed[i] = speed;
    _dirty |= (uint16_t)(1u << i);
    xSemaphoreGive(_mutex);
    return true;
}

void Robot_Procedure::sendTargets()
{
    if (!controlAllowed())
        return;

    uint16_t dirty;
    int16_t target[SERVO_CAL_COUNT], offset[SERVO_CAL_COUNT], mn[SERVO_CAL_COUNT], mx[SERVO_CAL_COUNT];
    uint16_t speed[SERVO_CAL_COUNT];
    xSemaphoreTake(_mutex, portMAX_DELAY);
    dirty = _dirty;
    _dirty = 0;
    memcpy(target, _target, sizeof(target));
    memcpy(offset, _offset, sizeof(offset));
    memcpy(mn, _min, sizeof(mn));
    memcpy(mx, _max, sizeof(mx));
    memcpy(speed, _targetSpeed, sizeof(speed));
    xSemaphoreGive(_mutex);

    for (uint8_t i = 0; i < SERVO_CAL_COUNT; i++)
    {
        if (!(dirty & (1u << i)))
            continue;
        const int pos = clampi(target[i], mn[i], mx[i]);
        const int raw = clampi(pos + offset[i], 0, 1023);
        _robot.servo->writePositionAt(i + SERVO_FIRST_CONFIGURABLE_SLOT, (int16_t)raw,
                                      speed[i] ? speed[i] : ServoInitSpeed_SC, ServoInitACC_SC);
    }
}

// ---------------------------------------------------------------------------
// Calibration
// ---------------------------------------------------------------------------
bool Robot_Procedure::calibrationStart()
{
    if (!calibrationAvailable() || calibrationActive())
        return false;
    _abortRequested = false;
    _calSlot = nextSlot(0);
    _cal = CalState::Moving; // last: the task starts on seeing this
    return true;
}

void Robot_Procedure::calibrationConfirm()
{
    if (_cal == CalState::Released)
        _cal = CalState::Finishing;
}

void Robot_Procedure::calibrationAbort()
{
    if (calibrationActive())
        _abortRequested = true;
}

void Robot_Procedure::calibrationReset()
{
    if (_cal == CalState::Done || _cal == CalState::Failed)
        _cal = CalState::Idle;
}

// Torque back on for every calibratable servo, each told to hold where it is first so it
// doesn't jump back to the reference goal. With `store`, offset = position read -
// reference, saved to NVS. False if a servo didn't answer (its offset is left as it was)
// or the save failed.
bool Robot_Procedure::finishCalibration(bool store)
{
    bool ok = true;
    int16_t offsets[SERVO_CAL_COUNT];
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memcpy(offsets, _offset, sizeof(offsets));
    xSemaphoreGive(_mutex);

    Servo_Module *servo = _robot.servo;
    for (uint8_t slot = SERVO_FIRST_CONFIGURABLE_SLOT; slot < MAX_SERVO_NUM; slot++)
    {
        if (!calibratable(slot))
            continue;
        const int pos = servo->readPositionAt(slot);
        if (pos >= 0)
        {
            servo->writePositionAt(slot, (int16_t)pos, ServoInitSpeed_SC, ServoInitACC_SC);
            if (store)
                offsets[slot - SERVO_FIRST_CONFIGURABLE_SLOT] = (int16_t)(pos - SERVO_CAL_POSITION[slot]);
        }
        else
        {
            ok = false;
        }
        servo->torqueAt(slot, true);
        esp_task_wdt_reset(); // a silent servo costs its bus timeout
    }

    if (!store)
        return ok;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    memcpy(_offset, offsets, sizeof(_offset));
    xSemaphoreGive(_mutex);
    return NVS::saveArray(KEY_OFFSET, offsets, SERVO_CAL_COUNT) && ok;
}

// One servo per period, so other tasks' bus traffic interleaves.
void Robot_Procedure::stepCalibration()
{
    if (_abortRequested || (calibrationActive() && _robot.mode() == ControlMode::Bottango))
    {
        _abortRequested = false;
        finishCalibration(false); // torque back on, store nothing
        _cal = CalState::Idle;
        return;
    }

    Servo_Module *servo = _robot.servo;
    switch (_cal)
    {
    case CalState::Moving:
        if (_calSlot >= MAX_SERVO_NUM)
        {
            _calT0 = millis();
            _cal = CalState::Settling;
        }
        else
        {
            servo->writePositionAt(_calSlot, SERVO_CAL_POSITION[_calSlot], ServoInitSpeed_SC, ServoInitACC_SC);
            _calSlot = nextSlot(_calSlot + 1);
        }
        break;
    case CalState::Settling:
        if (millis() - _calT0 >= ROBOT_CAL_SETTLE_MS)
        {
            _calSlot = nextSlot(0);
            _cal = CalState::Releasing;
        }
        break;
    case CalState::Releasing:
        if (_calSlot >= MAX_SERVO_NUM)
        {
            _cal = CalState::Released;
        }
        else
        {
            servo->torqueAt(_calSlot, false);
            _calSlot = nextSlot(_calSlot + 1);
        }
        break;
    case CalState::Finishing:
        _cal = finishCalibration(true) ? CalState::Done : CalState::Failed;
        break;
    default: // Idle, Released, Done, Failed: waiting on the caller
        break;
    }
}

// ---------------------------------------------------------------------------
// Task
// ---------------------------------------------------------------------------
void Robot_Procedure::step()
{
    stepCalibration();

    // Kinematics goes here: turn a body-level goal into joint targets with
    // setJointTarget() before they are sent.

    sendTargets();
}

void Robot_Procedure::s_task(void *arg)
{
    static_cast<Robot_Procedure *>(arg)->run();
}

void Robot_Procedure::run()
{
    esp_task_wdt_add(nullptr);

    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(ROBOT_TASK_PERIOD_MS);

    while (1)
    {
        step();
        esp_task_wdt_reset();

        if (xTaskDelayUntil(&lastWake, period) == pdFALSE)
        {
            vTaskDelay(1);
            lastWake = xTaskGetTickCount();
        }
    }
}
