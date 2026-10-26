#include "Animated_Face.h"

Animated_Face::Animated_Face(int width, int height) {
  _screenWidth = _prevScreenWidth = width;
  _screenHeight = _prevScreenHeight = height;
}

Animated_Face::Animated_Face() {}


//*********************************************************************************************
//  GENERAL METHODS
//*********************************************************************************************

Animated_Face::~Animated_Face() {
  end();
}

void Animated_Face::begin(lv_obj_t *parent, bool dbug) {
  if (_obj) return;
  _debug = dbug;

  _obj = lv_obj_create(parent);
  lv_obj_remove_style_all(_obj);
  lv_obj_set_size(_obj, lv_pct(100), lv_pct(100));
  lv_obj_remove_flag(_obj, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
  lv_obj_add_flag(_obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_set_style_bg_opa(_obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(_obj, toColor(_debug ? _FakebgColor : _bgColor), 0);
  lv_obj_add_event_cb(_obj, drawCb, LV_EVENT_DRAW_MAIN, this);
  lv_obj_add_event_cb(_obj, deletedCb, LV_EVENT_DELETE, this);

  lv_obj_update_layout(_obj);
  _screenWidth = lv_obj_get_width(_obj);
  _screenHeight = lv_obj_get_height(_obj);
  init();

  _timer = lv_timer_create(timerCb, _frameInterval, this);
}

void Animated_Face::end() {
  if (_timer) {
    lv_timer_delete(_timer);
    _timer = nullptr;
  }
  if (_obj) {
    lv_obj_t *obj = _obj;
    _obj = nullptr;  // deletedCb then sees nothing left to do
    lv_obj_delete(obj);
  }
}

void Animated_Face::deletedCb(lv_event_t *e) {
  // The parent screen went away: drop the dangling pointer and stop animating.
  Animated_Face *self = static_cast<Animated_Face *>(lv_event_get_user_data(e));
  if (self->_obj) {
    self->_obj = nullptr;
    if (self->_timer) {
      lv_timer_delete(self->_timer);
      self->_timer = nullptr;
    }
  }
}

void Animated_Face::timerCb(lv_timer_t *t) {
  static_cast<Animated_Face *>(lv_timer_get_user_data(t))->update();
}

lv_color_t Animated_Face::toColor(uint16_t c) {
  return lv_color_make((c >> 8) & 0xF8, (c >> 3) & 0xFC, (c << 3) & 0xF8);
}

void Animated_Face::init() {

  // Init Display
  // canvasGfx->begin();
  // canvasGfx->fillScreen(_bgColor);
  // canvasGfx->flush();
  //canvasGfx->setDirectUseColorIndex(true);
  //eyeLheightCurrent = 1;    // start with closed eyes
  //eyeRheightCurrent = 1;    // start with closed eyes
  //setFramerate(frameRate);  // calculate frame interval based on defined frameRate

  // BUILD DEFAULT FACE
  // DEFAULT Position
  _faceCenter_x = _faceCenter_xNext = _screenWidth / 2;
  _faceCenter_y = _faceCenter_yNext = _screenHeight / 2;

  // BUILD EYE USING DEFAULTS
  // Start eyes as closed
  for (uint8_t i = 0; i < 6; i++) {
    _eyes[i].width = eyeWidthDefault;
    _eyes[i].height = eyeHeightDefault;
    _eyes[i].heightOffset = eyeHeightOffsetDefault;
    _eyes[i].radius = eyeBorderRadiusDefault;
    _eyes[i].distanceCenterX = spaceBetweenDefault;
    if (mouthActive) _eyes[i].distanceCenterY = spaceHeightDefault;
    else _eyes[i].distanceCenterY = 0;
    _eyes[i].color = _eyeColorDefault;
    _eyes[i].open = false;
    _eyes[i].blinking = false;

    for (uint8_t j = 0; j < 2; j++) {
      _eyes[i].lids[j].y1 = 0;
      _eyes[i].lids[j].y2 = 0;
      _eyes[i].lids[j].x3 = 0;
      _eyes[i].lids[j].y3 = 0;
      _eyes[i].lids[j].height = 0;
      _eyes[i].lids[j].radius = 0;
    }
  }

  // EYE - Coordinates
  // Left
  _eyes[LEFT].cX = _faceCenter_x - (eyeWidthDefault + spaceBetweenDefault);
  _eyes[LEFT].cY = _faceCenter_y;

  // Right
  _eyes[RIGHT].cX = _faceCenter_x + spaceBetweenDefault + 1;
  _eyes[RIGHT].cY = _faceCenter_y;

  // SET Eyes to open
  open();

  //Mouth Init
  mouthLx = mouthLx_Next = _faceCenter_x;
  mouthLy = _faceCenter_y;
  mouthLy_Next = _faceCenter_y + mouthPaddingDefault;

  mouthWidth = mouthWidthNext = mouthWidthDefault;
  mouthHeight = mouthHeightNext = mouthHeightDefault;
  mouthRadius = mouthRadiusNext = mouthRadiusDefault;
  mouthPadding = mouthPaddingNext = mouthPaddingDefault;

  // #ifdef FACE_DEBUG
  //   debug();
  // #endif

  // Init Touch
  // if (touch) {
  //   _touchDrv = touch;
  //   _touchDrv->init();
  // }
}

void Animated_Face::update() {
  stepFace();
  setup_complete = true;
  if (_obj) invalidateChanged();
}

void Animated_Face::invalidateChanged() {
  if (cross_hair || mouthCrosshair) {
    lv_obj_invalidate(_obj);  // debug overlays: not worth tracking
    return;
  }

  const int sides = cyclops ? 1 : 2;
  lv_area_t now[3];
  bool changed[3] = {false, false, false};
  bool used[3] = {false, false, false};

  const int mouthState[8] = {mouthLx, mouthLy, mouthWidth, mouthHeight, mouthRadius, topMouthLip, bottomMouthLip, surprisedPaddingNext};

  for (int s = 0; s < sides; s++) {
    const eyes &e = _eyes[s];
    now[s] = {e.cX - 2, e.cY - surprisedEyePadding - 2, e.cX + e.width + 2, e.cY + e.height + surprisedEyePadding + 2};
    used[s] = true;
    changed[s] = !_snapValid || memcmp(&_snapEyes[s], &e, sizeof(eyes)) != 0;
    memcpy(&_snapEyes[s], &e, sizeof(eyes));
  }
  if (mouthActive) {
    now[2] = {mouthLx - mouthWidth / 2 - 2, mouthLy - surprisedPaddingNext - 2, mouthLx + mouthWidth / 2 + mouthWidth % 2 + 2, mouthLy + mouthHeight + surprisedPaddingNext + 2};
    used[2] = true;
    changed[2] = !_snapValid || memcmp(_snapMouth, mouthState, sizeof(mouthState)) != 0;
    memcpy(_snapMouth, mouthState, sizeof(mouthState));
  }

  for (int i = 0; i < 3; i++) {
    if (!used[i] || !changed[i]) continue;
    lv_area_t dirty = now[i];
    if (_snapValid) {  // also repaint where it used to be
      dirty.x1 = min(dirty.x1, _prevRect[i].x1);
      dirty.y1 = min(dirty.y1, _prevRect[i].y1);
      dirty.x2 = max(dirty.x2, _prevRect[i].x2);
      dirty.y2 = max(dirty.y2, _prevRect[i].y2);
    }
    lv_obj_invalidate_area(_obj, &dirty);
    _prevRect[i] = now[i];
  }
  _snapValid = true;
}

// void Animated_Face::setFramerate(uint8_t fps) {
//   _frameInterval = 1000 / fps;
// }


void Animated_Face::close(bool left, bool right) {
  if (left) {
    _eyes[LEFT_NEXT].height = 1;  // blinking left eye
    _eyes[LEFT].open = false;     // left eye not opened (=closed)
    _eyes[LEFT].blinking = true;
  }
  if (right) {
    _eyes[RIGHT_NEXT].height = 1;  // blinking right eye
    _eyes[RIGHT].open = false;     // right eye not opened (=closed)
    _eyes[RIGHT].blinking = true;
  }
}

// Open eye(s)
void Animated_Face::open(bool left, bool right) {
  if (left) {
    _eyes[LEFT].open = true;  // left eye opened - if true, drawEyes() will take care of opening eyes again
    _eyes[LEFT].blinking = true;
  }
  if (right) {
    _eyes[RIGHT].open = true;  // right eye opened
    _eyes[RIGHT].blinking = true;
  }
}

// Trigger eyeblink(s) animation
void Animated_Face::blink(bool left, bool right) {
  close(left, right);
  open(left, right);
}

void Animated_Face::setFaceHorizontalOffset(int16_t offset) {
  _horizontal_padding = offset;
}
void Animated_Face::setFaceVerticalOffset(int16_t offset) {
  _vertical_padding = offset;
}

//*********************************************************************************************
//  SETTERS METHODS
//*********************************************************************************************
// Set automated eye blinking, minimal blink interval in full seconds and blink interval variation range in full seconds
void Animated_Face::setAutoblinker(bool active, int interval, int variation) {
  autoblinker = active;
  blinkInterval = interval;
  blinkIntervalVariation = variation;
}
void Animated_Face::setAutoblinker(bool active) {
  autoblinker = active;
}

void Animated_Face::setEyeWidth(int both) {
  _eyes[LEFT_NEXT].width = both;
  _eyes[RIGHT_NEXT].width = both;
  _eyes[LEFT_DEFAULT].width = both;
  _eyes[RIGHT_DEFAULT].width = both;

  if (!setup_complete) {
    _eyes[LEFT].width = both;
    _eyes[RIGHT].width = both;
  }
}


void Animated_Face::setEyeWidth(int leftEye, int rightEye) {
  _eyes[LEFT_NEXT].width = leftEye;
  _eyes[RIGHT_NEXT].width = rightEye;
  _eyes[LEFT_DEFAULT].width = leftEye;
  _eyes[RIGHT_DEFAULT].width = rightEye;

  if (!setup_complete) {
    _eyes[LEFT].width = leftEye;
    _eyes[RIGHT].width = rightEye;
  }
  //eyeRwidthDefault = rightEye;

  //eyeLxNext = center_x - (eyeLwidthDefault + spaceBetweenDefault);
  //eyeRxNext = center_x + spaceBetweenDefault+2;
}

void Animated_Face::setEyeHeight(int both) {
  _eyes[LEFT_NEXT].height = both;
  _eyes[RIGHT_NEXT].height = both;
  _eyes[LEFT_DEFAULT].height = both;
  _eyes[RIGHT_DEFAULT].height = both;

  if (!setup_complete) {
    _eyes[LEFT].height = both;
    _eyes[RIGHT].height = both;
  }
}

void Animated_Face::setEyeHeight(int leftEye, int rightEye) {
  _eyes[LEFT_NEXT].height = leftEye;
  _eyes[RIGHT_NEXT].height = rightEye;
  _eyes[LEFT_DEFAULT].height = leftEye;
  _eyes[RIGHT_DEFAULT].height = rightEye;

  if (!setup_complete) {
    _eyes[LEFT].height = leftEye;
    _eyes[RIGHT].height = rightEye;
  }
  //eyeRheightDefault = rightEye;

  //eyeLyNext = center_y - (eyeLheightDefault/2);
  //eyeRyNext = center_y - (eyeRheightDefault/2);
}

void Animated_Face::setEyeHeightOffset(int leftEye, int rightEye) {
  _eyes[LEFT_NEXT].heightOffset = leftEye;
  _eyes[RIGHT_NEXT].heightOffset = rightEye;
  _eyes[LEFT_DEFAULT].heightOffset = leftEye;
  _eyes[RIGHT_DEFAULT].heightOffset = rightEye;

  if (!setup_complete) {
    _eyes[LEFT].heightOffset = leftEye;
    _eyes[RIGHT].heightOffset = rightEye;
  }
}

void Animated_Face::setEyeBorderradius(int both) {
  _eyes[LEFT_NEXT].radius = both;
  _eyes[RIGHT_NEXT].radius = both;
  _eyes[LEFT_DEFAULT].radius = both;
  _eyes[RIGHT_DEFAULT].radius = both;

  if (!setup_complete) {
    _eyes[LEFT].radius = both;
    _eyes[RIGHT].radius = both;
  }
}

// Set border radius for left and right eye
void Animated_Face::setEyeBorderradius(int leftEye, int rightEye) {
  _eyes[LEFT_NEXT].radius = leftEye;
  _eyes[RIGHT_NEXT].radius = rightEye;
  _eyes[LEFT_DEFAULT].radius = leftEye;
  _eyes[RIGHT_DEFAULT].radius = rightEye;

  if (!setup_complete) {
    _eyes[LEFT].radius = leftEye;
    _eyes[RIGHT].radius = rightEye;
  }
  //eyeRborderRadiusDefault = rightEye;
}

// Set space between the eyes, can also be negative
void Animated_Face::setEyeSpacebetween(int space) {
  _eyes[LEFT_NEXT].distanceCenterX = space;
  _eyes[RIGHT_NEXT].distanceCenterX = space;
  _eyes[LEFT_DEFAULT].distanceCenterX = space;
  _eyes[RIGHT_DEFAULT].distanceCenterX = space;

  if (!setup_complete) {
    _eyes[LEFT].distanceCenterX = space;
    _eyes[RIGHT].distanceCenterX = space;
  }
}

void Animated_Face::setEyeSpacebetween(int leftEye, int rightEye) {
  _eyes[LEFT_NEXT].distanceCenterX = leftEye;
  _eyes[RIGHT_NEXT].distanceCenterX = rightEye;
  _eyes[LEFT_DEFAULT].distanceCenterX = leftEye;
  _eyes[RIGHT_DEFAULT].distanceCenterX = rightEye;

  if (!setup_complete) {
    _eyes[LEFT].distanceCenterX = leftEye;
    _eyes[RIGHT].distanceCenterX = rightEye;
  }
}

void Animated_Face::setEyeColor(uint16_t color) {
  _eyes[LEFT].color = _eyes[RIGHT].color = color;
}

void Animated_Face::setEyeColor(uint16_t l_color, uint16_t r_color) {
  _eyes[LEFT].color = l_color;
  _eyes[RIGHT].color = r_color;
}

void Animated_Face::setBGColor(uint16_t color) {
  _bgColor = color;
  if (_obj && !_debug) lv_obj_set_style_bg_color(_obj, toColor(color), 0);
}

void Animated_Face::setCuriosity(bool active, int interval, int variation) {
  idle = active;
  idleInterval = interval;
  idleIntervalVariation = variation;
}

void Animated_Face::setCuriosity(bool active) {
  idle = active;
}

void Animated_Face::setCuriosityOffsets(int width_change, int height_change) {
  idleWidthChange = width_change;
  idleHeightChange = height_change;
}

void Animated_Face::setPositionDistance(unsigned int distance) {
  idleDistance = distance;
}

void Animated_Face::setPosition(cardinal_direction direction) {
  switch (direction) {
    case N:
      _faceCenter_xNext = (_screenWidth / 2);
      _faceCenter_yNext = (_screenHeight / 2) + idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + idleHeightChange;
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + idleHeightChange;
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + idleWidthChange;
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + idleWidthChange;
      break;
    case NE:
      _faceCenter_xNext = (_screenWidth / 2) + idleDistance;
      _faceCenter_yNext = (_screenHeight / 2) + idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + (idleWidthChange / 2);
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + (idleWidthChange / 2);
      break;
    case E:
      _faceCenter_xNext = (_screenWidth / 2) + idleDistance;
      _faceCenter_yNext = (_screenHeight / 2);
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height;
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + idleHeightChange;
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width;
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + idleWidthChange;
      break;
    case SE:
      _faceCenter_xNext = (_screenWidth / 2) + idleDistance;
      _faceCenter_yNext = (_screenHeight / 2) - idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + (idleWidthChange / 2);
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + (idleWidthChange / 2);
      break;
    case S:
      _faceCenter_xNext = (_screenWidth / 2);
      _faceCenter_yNext = (_screenHeight / 2) - idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + idleHeightChange;
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + idleHeightChange;
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + idleWidthChange;
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + idleWidthChange;
      break;
    case SW:
      _faceCenter_xNext = (_screenWidth / 2) - idleDistance;
      _faceCenter_yNext = (_screenHeight / 2) - idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + (idleWidthChange / 2);
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + (idleWidthChange / 2);
      break;
    case W:
      _faceCenter_xNext = (_screenWidth / 2) - idleDistance;
      _faceCenter_yNext = (_screenHeight / 2);
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + idleHeightChange;
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height;
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + idleWidthChange;
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width;
      break;
    case NW:
      _faceCenter_xNext = (_screenWidth / 2) - idleDistance;
      _faceCenter_yNext = (_screenHeight / 2) + idleDistance;
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height + (idleHeightChange / 2);
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width + (idleWidthChange / 2);
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width + (idleWidthChange / 2);
      break;
    default:
      _faceCenter_xNext = (_screenWidth / 2);
      _faceCenter_yNext = (_screenHeight / 2);
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height;
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height;
      _eyes[LEFT_NEXT].width = _eyes[LEFT_DEFAULT].width;
      _eyes[RIGHT_NEXT].width = _eyes[RIGHT_DEFAULT].width;
      break;
  }
}

//*********************************************************************************************
//  MOUTH ADDITIONS
//*********************************************************************************************
void Animated_Face::addMouth(bool active) {
  mouthActive = active;
}

void Animated_Face::addMouth(bool active, int padding) {
  mouthActive = active;
  mouthPaddingNext = padding;
  mouthPaddingDefault = padding;
}

void Animated_Face::setMouthBorderRadius(int mouth) {
  mouthRadiusNext = mouth;
  mouthRadiusDefault = mouth;
}

void Animated_Face::setMouthWidth(int sizeX) {
  mouthWidthNext = sizeX;
  mouthWidthDefault = sizeX;
}

void Animated_Face::setMouthHeight(int sizeY) {
  mouthHeightNext = sizeY;
  mouthHeightDefault = sizeY;
}

void Animated_Face::setMouthPadding(int padding) {
  mouthPaddingNext = padding;
  mouthPaddingDefault = padding;
}

//*********************************************************************************************
//  PRE-CALCULATIONS AND ACTUAL DRAWINGS
//*********************************************************************************************
void Animated_Face::stepFace() {
  if (idle) {
    if (millis() >= idleAnimationTimer) {
      unsigned int move_from_center = random(3);
      if (move_from_center == 1) {
        cardinal_direction selected = (cardinal_direction)random(9);
        setPosition(selected);
      } else {
        setPosition(C);
      }
      idleAnimationTimer = millis() + (idleInterval * 1000) + (random(idleIntervalVariation) * 1000);  // calculate next time for eyes repositioning
    }
  }

  // _faceCenter_x = (_faceCenter_x + _faceCenter_xNext) / 2;
  // _faceCenter_y = (_faceCenter_y + _faceCenter_yNext) / 2;

  // _faceCenter_x = _faceCenter_x + (abs(_faceCenter_xNext - _faceCenter_x) / _framedivisor);
  // _faceCenter_y = _faceCenter_y + (abs(_faceCenter_yNext - _faceCenter_y) / _framedivisor);

  _faceCenter_x = (_faceCenter_x * _cur_value) + ((_faceCenter_xNext + _horizontal_padding) * _nex_value);
  _faceCenter_y = (_faceCenter_y * _cur_value) + ((_faceCenter_yNext + _vertical_padding) * _nex_value);

  if (mouthActive)
    stepMouth();

  stepEyes();
}

void Animated_Face::setAnimiationSpeed(float value) {
  if (value >= 0.50 and value <= 0.99) {
    _cur_value = value;
    _nex_value = 1.0 - value;
  }
}



void Animated_Face::stepEyes() {

  // Left eye width
  _eyes[LEFT].width = (_eyes[LEFT].width + _eyes[LEFT_NEXT].width) / 2;
  _eyes[LEFT].height = (_eyes[LEFT].height * _cur_value) + (_nex_value * (_eyes[LEFT_NEXT].height + _eyes[LEFT].heightOffset));
  _eyes[LEFT].distanceCenterX = (_eyes[LEFT].distanceCenterX + _eyes[LEFT_NEXT].distanceCenterX) / 2;
  _eyes[LEFT].distanceCenterY = (_eyes[LEFT].distanceCenterY + _eyes[LEFT_NEXT].distanceCenterY) / 2;
  _eyes[LEFT].radius = (_eyes[LEFT].radius + _eyes[LEFT_NEXT].radius) / 2;
  // lids
  // _eyes[LEFT].lids[TOP].y1 = (_eyes[LEFT].lids[TOP].y1 + _eyes[LEFT_NEXT].lids[TOP].y1) / 2;
  // _eyes[LEFT].lids[TOP].y2 = (_eyes[LEFT].lids[TOP].y2 + _eyes[LEFT_NEXT].lids[TOP].y2) / 2;
  // _eyes[LEFT].lids[TOP].x3 = (_eyes[LEFT].lids[TOP].x3 + _eyes[LEFT_NEXT].lids[TOP].x3) / 2;
  _eyes[LEFT].lids[TOP].y3 = (_eyes[LEFT].lids[TOP].y3 + _eyes[LEFT_NEXT].lids[TOP].y3) / 2;
  _eyes[LEFT].lids[TOP].height = (_eyes[LEFT].lids[TOP].height + _eyes[LEFT_NEXT].lids[TOP].height) / 2;
  _eyes[LEFT].lids[TOP].radius = (_eyes[LEFT].lids[TOP].radius + _eyes[LEFT_NEXT].lids[TOP].radius) / 2;
  // _eyes[LEFT].lids[BOTTOM].y1 = (_eyes[LEFT].lids[BOTTOM].y1 + _eyes[LEFT_NEXT].lids[BOTTOM].y1) / 2;
  // _eyes[LEFT].lids[BOTTOM].y2 = (_eyes[LEFT].lids[BOTTOM].y2 + _eyes[LEFT_NEXT].lids[BOTTOM].y2) / 2;
  // _eyes[LEFT].lids[BOTTOM].x3 = (_eyes[LEFT].lids[BOTTOM].x3 + _eyes[LEFT_NEXT].lids[BOTTOM].x3) / 2;
  _eyes[LEFT].lids[BOTTOM].y3 = (_eyes[LEFT].lids[BOTTOM].y3 + _eyes[LEFT_NEXT].lids[BOTTOM].y3) / 2;
  _eyes[LEFT].lids[BOTTOM].height = (_eyes[LEFT].lids[BOTTOM].height + _eyes[LEFT_NEXT].lids[BOTTOM].height) / 2;
  _eyes[LEFT].lids[BOTTOM].radius = (_eyes[LEFT].lids[BOTTOM].radius + _eyes[LEFT_NEXT].lids[BOTTOM].radius) / 2;

  // Right eye width
  _eyes[RIGHT].width = (_eyes[RIGHT].width + _eyes[RIGHT_NEXT].width) / 2;
  _eyes[RIGHT].height = (_eyes[RIGHT].height * _cur_value) + (_nex_value * (_eyes[RIGHT_NEXT].height + _eyes[RIGHT].heightOffset));
  _eyes[RIGHT].distanceCenterX = (_eyes[RIGHT].distanceCenterX + _eyes[RIGHT_NEXT].distanceCenterX) / 2;
  _eyes[RIGHT].distanceCenterY = (_eyes[RIGHT].distanceCenterY + _eyes[RIGHT_NEXT].distanceCenterY) / 2;
  _eyes[RIGHT].radius = (_eyes[RIGHT].radius + _eyes[RIGHT_NEXT].radius) / 2;
  // lids
  // _eyes[RIGHT].lids[TOP].y1 = (_eyes[RIGHT].lids[TOP].y1 + _eyes[RIGHT_NEXT].lids[TOP].y1) / 2;
  // _eyes[RIGHT].lids[TOP].y2 = (_eyes[RIGHT].lids[TOP].y2 + _eyes[RIGHT_NEXT].lids[TOP].y2) / 2;
  // _eyes[RIGHT].lids[TOP].x3 = (_eyes[RIGHT].lids[TOP].x3 + _eyes[RIGHT_NEXT].lids[TOP].x3) / 2;
  _eyes[RIGHT].lids[TOP].y3 = (_eyes[RIGHT].lids[TOP].y3 + _eyes[RIGHT_NEXT].lids[TOP].y3) / 2;
  _eyes[RIGHT].lids[TOP].height = (_eyes[RIGHT].lids[TOP].height + _eyes[RIGHT_NEXT].lids[TOP].height) / 2;
  _eyes[RIGHT].lids[TOP].radius = (_eyes[RIGHT].lids[TOP].radius + _eyes[RIGHT_NEXT].lids[TOP].radius) / 2;
  // _eyes[RIGHT].lids[BOTTOM].y1 = (_eyes[RIGHT].lids[BOTTOM].y1 + _eyes[RIGHT_NEXT].lids[BOTTOM].y1) / 2;
  // _eyes[RIGHT].lids[BOTTOM].y2 = (_eyes[RIGHT].lids[BOTTOM].y2 + _eyes[RIGHT_NEXT].lids[BOTTOM].y2) / 2;
  // _eyes[RIGHT].lids[BOTTOM].x3 = (_eyes[RIGHT].lids[BOTTOM].x3 + _eyes[RIGHT_NEXT].lids[BOTTOM].x3) / 2;
  _eyes[RIGHT].lids[BOTTOM].y3 = (_eyes[RIGHT].lids[BOTTOM].y3 + _eyes[RIGHT_NEXT].lids[BOTTOM].y3) / 2;
  _eyes[RIGHT].lids[BOTTOM].height = (_eyes[RIGHT].lids[BOTTOM].height + _eyes[RIGHT_NEXT].lids[BOTTOM].height) / 2;
  _eyes[RIGHT].lids[BOTTOM].radius = (_eyes[RIGHT].lids[BOTTOM].radius + _eyes[RIGHT_NEXT].lids[BOTTOM].radius) / 2;

  surprisedEyePadding = (surprisedEyePadding + surprisedEyePaddingNext) / 2;


  //EYE Coordinates Default prior to modifications
  if (!mouthActive) {
    // Left eye
    _eyes[LEFT].cX = _faceCenter_x - (_eyes[LEFT].width + _eyes[LEFT].distanceCenterX);
    _eyes[LEFT].cY = _faceCenter_y - (_eyes[LEFT].height / 2);
    // Right eye
    _eyes[RIGHT].cX = _faceCenter_x + _eyes[RIGHT].distanceCenterX + 1;
    _eyes[RIGHT].cY = _faceCenter_y - (_eyes[RIGHT].height / 2);
  } else {
    _eyes[LEFT].cX = _faceCenter_x - (_eyes[LEFT].width + _eyes[LEFT].distanceCenterX);
    _eyes[LEFT].cY = _faceCenter_y - (_eyes[LEFT_DEFAULT].height - (_eyes[LEFT_DEFAULT].height - _eyes[LEFT].height) / 2) - _padding;
    // Right eye
    _eyes[RIGHT].cX = _faceCenter_x + (_eyes[RIGHT].distanceCenterX + 1);
    _eyes[RIGHT].cY = _faceCenter_y - (_eyes[RIGHT_DEFAULT].height - (_eyes[RIGHT_DEFAULT].height - _eyes[RIGHT].height) / 2) - _padding;
  }

  // Open eyes again after closing them
  if (_eyes[LEFT].open && _eyes[LEFT].blinking) {
    if (_eyes[LEFT].height <= 1 + _eyes[LEFT].heightOffset) {
      _eyes[LEFT_NEXT].height = _eyes[LEFT_DEFAULT].height;
      _eyes[LEFT].blinking = false;
    }
  }
  if (_eyes[RIGHT].open && _eyes[RIGHT].blinking) {
    if (_eyes[RIGHT].height <= 1 + _eyes[RIGHT].heightOffset) {
      _eyes[RIGHT_NEXT].height = _eyes[RIGHT_DEFAULT].height;
      _eyes[RIGHT].blinking = false;
    }
  }

  if (autoblinker) {
    if (millis() >= blinktimer) {
      blink();
      blinktimer = millis() + (blinkInterval * 1000) + (random(blinkIntervalVariation) * 1000);  // calculate next time for blinking
    }
  }

}

//*********************************************************************************************
//  MOUTH FUNCTIONS
//*********************************************************************************************
void Animated_Face::setMouthCrosshair(bool active) {
  mouthCrosshair = active;
}

void Animated_Face::stepMouth() {

  mouthLx = _faceCenter_x;
  mouthLy = _faceCenter_y + mouthPaddingDefault;

  mouthWidth = (mouthWidth + mouthWidthNext) / 2;
  mouthHeight = (mouthHeight + mouthHeightNext) / 2;
  mouthRadius = (mouthRadius + mouthRadiusNext) / 2;
  mouthPadding = (mouthPadding + mouthPaddingNext) / 2;

  // mouth_sad = 0;
  // mouth_surprised = 0;

  if (mouth_surprised) {
    surprisedPaddingNext = 8;
    mouthRadiusNext = mouthRadiusDefault + 10;
  } else {
    surprisedPaddingNext = 0;
    mouthRadiusNext = mouthRadiusDefault;
  }

  if (mouth_happy) {
    topMouthRadius = 0;
    topMouthLipNext = mouthHeight / 2;
  } else {
    topMouthLipNext = 0;
    topMouthRadius = mouthRadiusDefault;
  }

  if (mouth_sad) {
    bottomMouthLipNext = ((4 * mouthHeight) / 5) + 2;
  } else {
    bottomMouthLipNext = 0;
  }

  if (!mouth_happy && !mouth_surprised && !mouth_sad) {
    topMouthLipNext = (4 * mouthHeight) / 5;
  }


  surprisedPadding = (surprisedPadding * _cur_value) + (surprisedPaddingNext * _nex_value);
  topMouthLip = (topMouthLip * _cur_value) + (topMouthLipNext * _nex_value);
  bottomMouthLip = (bottomMouthLip * _cur_value) + (bottomMouthLipNext * _nex_value);

}

//*********************************************************************************************
//  ADDITIONAL TEST FUNCTIONS
//*********************************************************************************************
void Animated_Face::set_eye_lid_position(bool eye, bool side, int y1, int y2, int x3, int y3, int height, int radius) {
  if (!eye) {
    _eyes[LEFT].lids[side].y1 = y1;
    _eyes[LEFT].lids[side].y2 = y2;
    _eyes[LEFT].lids[side].x3 = x3;
    _eyes[LEFT_NEXT].lids[side].y3 = y3;
    _eyes[LEFT_NEXT].lids[side].height = height;
    _eyes[LEFT_NEXT].lids[side].radius = radius;
  } else {
    _eyes[RIGHT].lids[side].y1 = y1;
    _eyes[RIGHT].lids[side].y2 = y2;
    _eyes[RIGHT].lids[side].x3 = x3;
    _eyes[RIGHT_NEXT].lids[side].y3 = y3;
    _eyes[RIGHT_NEXT].lids[side].height = height;
    _eyes[RIGHT_NEXT].lids[side].radius = radius;
  }
}


void Animated_Face::setCrosshair(bool active) {
  cross_hair = active;
}

//*********************************************************************************************
//  RENDERING (LVGL primitives; eye shapes first, then lids in the background colour on top)
//*********************************************************************************************
void Animated_Face::drawCb(lv_event_t *e) {
  Animated_Face *self = static_cast<Animated_Face *>(lv_event_get_user_data(e));
  lv_area_t coords;
  lv_obj_get_coords(self->_obj, &coords);
  lv_layer_t *layer = lv_event_get_layer(e);
  if (self->mouthActive) self->renderMouth(layer, coords.x1, coords.y1);
  self->renderEyes(layer, coords.x1, coords.y1);
}

void Animated_Face::fillRect(lv_layer_t *layer, int x, int y, int w, int h, int radius, uint16_t color) {
  if (w <= 0 || h <= 0) return;
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = toColor(color);
  dsc.bg_opa = LV_OPA_COVER;
  dsc.radius = radius < 0 ? 0 : radius;  // LVGL clamps to half the short side
  lv_area_t a = {x, y, x + w - 1, y + h - 1};
  lv_draw_rect(layer, &dsc, &a);
}

void Animated_Face::fillTri(lv_layer_t *layer, int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) {
  lv_draw_triangle_dsc_t dsc;
  lv_draw_triangle_dsc_init(&dsc);
  dsc.p[0] = {(lv_value_precise_t)x1, (lv_value_precise_t)y1};
  dsc.p[1] = {(lv_value_precise_t)x2, (lv_value_precise_t)y2};
  dsc.p[2] = {(lv_value_precise_t)x3, (lv_value_precise_t)y3};
  dsc.bg_color = toColor(color);
  dsc.bg_opa = LV_OPA_COVER;
  lv_draw_triangle(layer, &dsc);
}

void Animated_Face::drawCross(lv_layer_t *layer, int x, int y, uint16_t color) {
  lv_draw_line_dsc_t line;
  lv_draw_line_dsc_init(&line);
  line.color = toColor(color);
  line.width = 1;
  line.p1 = {(lv_value_precise_t)(x - 5), (lv_value_precise_t)y};
  line.p2 = {(lv_value_precise_t)(x + 5), (lv_value_precise_t)y};
  lv_draw_line(layer, &line);
  line.p1 = {(lv_value_precise_t)x, (lv_value_precise_t)(y - 5)};
  line.p2 = {(lv_value_precise_t)x, (lv_value_precise_t)(y + 5)};
  lv_draw_line(layer, &line);
}

void Animated_Face::renderEyes(lv_layer_t *layer, int ox, int oy) {
  const int sides = cyclops ? 1 : 2;
  const uint16_t bg = _debug ? _FakebgColor : _bgColor;

  // Basic eye shapes
  for (int s = 0; s < sides; s++) {
    const eyes &eye = _eyes[s];
    fillRect(layer, ox + eye.cX, oy + eye.cY - surprisedEyePadding, eye.width, eye.height + (2 * surprisedEyePadding), eye.radius, eye.color);
  }

  // Eye lids: a rounded rect cut from the top/bottom plus a triangle for the slanted lids
  if (!surprise && !cyclops) {
    for (int s = 0; s < 2; s++) {
      const eyes &eye = _eyes[s];
      if (eye.blinking) continue;
      const int x = ox + eye.cX;
      const int y = oy + eye.cY;

      // TOP
      fillRect(layer, x - 1, y - 1, eye.width + 2, eye.lids[TOP].height, eye.lids[TOP].radius, bg);
      fillTri(layer, x, y + eye.lids[TOP].y1 - 1, x + eye.width, y + eye.lids[TOP].y2 - 1, x + eye.lids[TOP].x3, y + eye.lids[TOP].y3 - 1, bg);

      // BOTTOM
      fillRect(layer, x - 1, y + eye.height - eye.lids[BOTTOM].height, eye.width + 2, eye.lids[BOTTOM].height + 2, eye.lids[BOTTOM].radius, bg);
      fillTri(layer, x, y + eye.lids[BOTTOM].y1 - 1, x + eye.width, y + eye.lids[BOTTOM].y2 - 1, x + eye.lids[BOTTOM].x3, y + eye.lids[BOTTOM].y3 - 1, bg);
    }
  }

  if (cross_hair) drawCross(layer, ox + _faceCenter_x, oy + _faceCenter_y, RGB565(242, 242, 33));
}

void Animated_Face::renderMouth(lv_layer_t *layer, int ox, int oy) {
  const uint16_t bg = _debug ? _FakebgColor : _bgColor;
  const int x = ox + mouthLx - (mouthWidth / 2);
  const int y = oy + mouthLy;

  // Default mouth
  fillRect(layer, x, y - surprisedPaddingNext, mouthWidth, mouthHeight + (2 * surprisedPaddingNext), mouthRadius, _mouthColor);
  // Top expression
  fillRect(layer, x - 1, y, mouthWidth + 2, topMouthLip, topMouthRadius, bg);
  // Bottom expression
  fillRect(layer, x - 1, y + (mouthHeight / 5), mouthWidth + 2, bottomMouthLip, topMouthRadius, bg);

  if (mouthCrosshair) drawCross(layer, ox + mouthLx, y, RGB565(255, 196, 0));
}
