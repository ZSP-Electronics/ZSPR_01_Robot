#include "Animated_Face.h"

Animated_Face::Animated_Face(int width, int height) {
  _screenWidth = _prevScreenWidth = width;
  _screenHeight = _prevScreenHeight = height;
}

Animated_Face::Animated_Face() {}


//*********************************************************************************************
//  GENERAL METHODS
//*********************************************************************************************

#if ARDUINO_GFX_AVAILABLE
void Animated_Face::begin(Arduino_RGB_Display *display, bool dbug) {
  _debug = dbug;
  canvasGfx = display;
  init();
}
#endif

#if TFT_ESPI_AVAILABLE
void Animated_Face::begin(TFT_eSprite *sprite, bool dbug) {
  _debug = dbug;
  _spr = sprite;

  _screenWidth = _spr->width();
  _screenHeight = _spr->height();
  init();
}
#endif

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
  // Limit drawing updates to defined max framerate
  if (millis() - fpsTimer >= _frameInterval) {
    drawFace();

    //if (flush)
    //canvasGfx->flush();

    fpsTimer = millis();
    setup_complete = true;
    //_fps = millis();
  }
  //_fps = millis();
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
void Animated_Face::drawFace() {
#if ARDUINO_GFX_AVAILABLE
  if (canvasGfx != NULL) {
    if (_debug)
      canvasGfx->fillScreen(_FakebgColor);  // start with a blank screen
    else
      canvasGfx->fillScreen(_bgColor);  // start with a blank screen
  }
#endif

#if TFT_ESPI_AVAILABLE
  if (_spr != NULL) {
    if (_debug)
      _spr->fillSprite(_FakebgColor);  // start with a blank screen
    else
      _spr->fillSprite(_bgColor);
    //_spr->fillSprite(RED);
  }
#endif


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
    drawMouth();

  drawEyes();

  // Draw sector lines if enabled
  // if (sector_lines) {
  //   //Draw Vertical Lines
  //   for (uint8_t i = 0; i <= 5; i++) {
  //     canvasGfx->drawLine(_sector_sizeX * i, 0, _sector_sizeX * i, _screenHeight, RGB565(242, 242, 33));
  //   }
  //   //Draw Horizontal Lines
  //   for (uint8_t j = 0; j <= 5; j++) {

  //     canvasGfx->drawLine(0, _sector_sizeY * j, _screenWidth, _sector_sizeY * j, RGB565(242, 242, 33));
  //   }

  //   if (idleRadius > 0)
  //     canvasGfx->drawCircle((_screenWidth / 2), (_screenHeight / 2), idleRadius, RGB565(255, 0, 255));
  // }

  //Draw center point of face
  if (cross_hair) {
#if ARDUINO_GFX_AVAILABLE
    if (canvasGfx != NULL) {
      canvasGfx->drawLine(_faceCenter_x - 5, _faceCenter_y, _faceCenter_x + 5, _faceCenter_y, RGB565(242, 242, 33));
      canvasGfx->drawLine(_faceCenter_x, _faceCenter_y - 5, _faceCenter_x, _faceCenter_y + 5, RGB565(242, 242, 33));
    }
#endif

#if TFT_ESPI_AVAILABLE
    if (_spr != NULL) {
      _spr->drawLine(_faceCenter_x - 5, _faceCenter_y, _faceCenter_x + 5, _faceCenter_y, RGB565(242, 242, 33));
      _spr->drawLine(_faceCenter_x, _faceCenter_y - 5, _faceCenter_x, _faceCenter_y + 5, RGB565(242, 242, 33));
    }
#endif
  }

  if (mouthCrosshair) {
    if (mouthActive) {
#if ARDUINO_GFX_AVAILABLE
      if (canvasGfx != NULL) {
        canvasGfx->drawLine(mouthLx - 5, mouthLy, mouthLx + 5, mouthLy, RGB565(255, 196, 0));
        canvasGfx->drawLine(mouthLx, mouthLy - 5, mouthLx, mouthLy + 5, RGB565(255, 196, 0));
      }
#endif

#if TFT_ESPI_AVAILABLE
      if (_spr != NULL) {
        _spr->drawLine(mouthLx - 5, mouthLy, mouthLx + 5, mouthLy, RGB565(255, 196, 0));
        _spr->drawLine(mouthLx, mouthLy - 5, mouthLx, mouthLy + 5, RGB565(255, 196, 0));
      }
#endif
    }
  }

  // if (_fps) {
  //   canvasGfx->setCursor((_screenWidth / 2), 20);
  //   //canvasGfx->setFont(&FreeSansBold10pt7b);
  //   canvasGfx->setTextColor(GREEN);
  //   //Serial.printf("FPS counter: %ld   Last FPS %ld\n", millis(), _lastFps);
  //   canvasGfx->print(1000 / (millis() - _lastFps - _frameInterval));
  //   canvasGfx->println(" FPS");
  //   _lastFps = millis();
  // }
}

void Animated_Face::setAnimiationSpeed(float value) {
  if (value >= 0.50 and value <= 0.99) {
    _cur_value = value;
    _nex_value = 1.0 - value;
  }
}



void Animated_Face::drawEyes() {

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

  /*************************/
  /* DRAW EYES and lidsS */
  /*************************/
#if ARDUINO_GFX_AVAILABLE
  if (canvasGfx != NULL) {
    // Draw basic eye shapes
    canvasGfx->fillRoundRect(_eyes[LEFT].cX, _eyes[LEFT].cY, _eyes[LEFT].width, _eyes[LEFT].height + surprisedEyePadding, _eyes[LEFT].radius, _eyes[LEFT].color);  // left eye
    if (!cyclops) {
      canvasGfx->fillRoundRect(_eyes[RIGHT].cX, _eyes[RIGHT].cY, _eyes[RIGHT].width, _eyes[RIGHT].height + surprisedEyePadding, _eyes[RIGHT].radius, _eyes[RIGHT].color);  // right eye
    }

    if (!_eyes[LEFT].blinking)
      canvasGfx->fillRoundRect(_eyes[LEFT].cX - 1, _eyes[LEFT].cY - 1, _eyes[LEFT].width + 2, 1, _eyes[LEFT].radius, _bgColor);
    if (!cyclops) {
      if (!_eyes[RIGHT].blinking)
        canvasGfx->fillRoundRect(_eyes[RIGHT].cX - 1, _eyes[RIGHT].cY - 1, _eyes[RIGHT].width + 2, 1, _eyes[RIGHT].radius, _bgColor);  // right eye
    }

    // Draw eye lids
    if (!cyclops) {
      if (!_eyes[LEFT].blinking) {
        //TOP
        canvasGfx->fillRoundRect(_eyes[LEFT].cX, _eyes[LEFT].cY - 1, _eyes[LEFT].width + 2, _eyes[LEFT].lids[TOP].height, _eyes[LEFT].lids[TOP].radius, _bgColor);
        canvasGfx->fillTriangle(_eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y1) - 1, _eyes[LEFT].cX + _eyes[LEFT].width, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y2) - 1, _eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y3) - 1, _bgColor);

        // BOTTOM
        canvasGfx->fillRoundRect(_eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].height) - (_eyes[LEFT].lids[BOTTOM].height + 1), _eyes[LEFT].width + 2, _eyes[LEFT].lids[BOTTOM].height + 2, _eyes[LEFT].lids[BOTTOM].radius, _bgColor);
        canvasGfx->fillTriangle(_eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y1) - 1, _eyes[LEFT].cX + _eyes[LEFT].width, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y2) - 1, _eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y3) - 1, _bgColor);
      }
      if (!_eyes[RIGHT].blinking) {
        //TOP
        canvasGfx->fillRoundRect(_eyes[RIGHT].cX, _eyes[RIGHT].cY - 1, _eyes[RIGHT].width + 2, _eyes[RIGHT].lids[TOP].height, _eyes[RIGHT].lids[TOP].radius, _bgColor);
        canvasGfx->fillTriangle(_eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y1) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].width, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y2) - 1, _eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y3) - 1, _bgColor);

        // BOTTOM
        canvasGfx->fillRoundRect(_eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].height) - (_eyes[RIGHT].lids[BOTTOM].height + 1), _eyes[RIGHT].width + 2, _eyes[RIGHT].lids[BOTTOM].height + 2, _eyes[RIGHT].lids[BOTTOM].radius, _bgColor);
        canvasGfx->fillTriangle(_eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y1) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].width, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y2) - 1, _eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y3) - 1, _bgColor);
      }
    } else {
      //canvasGfx->fillTriangle(_eyes[LEFT].cX, _eyes[LEFT].cY - 1, _eyes[LEFT].cX + (_eyes[LEFT].width / 2), _eyes[LEFT].cY - 1, _eyes[LEFT].cX, _eyes[LEFT].cY + _emotions[VALUE].sadness - 1, _bgColor);                                          // left lids half
      //canvasGfx->fillTriangle(_eyes[LEFT].cX + (_eyes[LEFT].width / 2), _eyes[LEFT].cY - 1, _eyes[LEFT].cX + _eyes[LEFT].width, _eyes[LEFT].cY - 1, _eyes[LEFT].cX + _eyes[LEFT].width, _eyes[LEFT].cY + _emotions[VALUE].sadness - 1, _bgColor);  // right lids half
    }
  }
#endif

#if TFT_ESPI_AVAILABLE
  if (_spr != NULL) {
    // Draw basic eye shapes
    fillRoundRect(_eyes[LEFT].cX, _eyes[LEFT].cY - surprisedEyePadding, _eyes[LEFT].width, _eyes[LEFT].height + (2 * surprisedEyePadding), _eyes[LEFT].radius, _eyes[LEFT].color);  // left eye
    if (!cyclops) {
      fillRoundRect(_eyes[RIGHT].cX, _eyes[RIGHT].cY - surprisedEyePadding, _eyes[RIGHT].width, _eyes[RIGHT].height + (2 * surprisedEyePadding), _eyes[RIGHT].radius, _eyes[RIGHT].color);  // right eye
    }

    // lidssTiredHeight = (lidssTiredHeight + lidssTiredHeightNext) / 2;
    // if (!_eyes[LEFT].blinking)
    //   fillRoundRect(_eyes[LEFT].cX - 1, _eyes[LEFT].cY - 1, _eyes[LEFT].width + 2, 1, _eyes[LEFT].radius, _bgColor);
    // if (!cyclops) {
    //   if (!_eyes[RIGHT].blinking)
    //     fillRoundRect(_eyes[RIGHT].cX - 1, _eyes[RIGHT].cY - 1, _eyes[RIGHT].width + 2, 1, _eyes[RIGHT].radius, _bgColor);  // right eye
    // }

    // Draw eye lids
    if (!surprise) {
      if (!cyclops) {
        if (!_eyes[LEFT].blinking) {
          //TOP
          fillRoundRect(_eyes[LEFT].cX - 1, _eyes[LEFT].cY - 1, _eyes[LEFT].width + 2, _eyes[LEFT].lids[TOP].height, _eyes[LEFT].lids[TOP].radius, _bgColor);
          _spr->fillTriangle(_eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y1) - 1, _eyes[LEFT].cX + _eyes[LEFT].width, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y2) - 1, _eyes[LEFT].cX + _eyes[LEFT].lids[TOP].x3, (_eyes[LEFT].cY + _eyes[LEFT].lids[TOP].y3) - 1, _bgColor);

          // BOTTOM
          fillRoundRect(_eyes[LEFT].cX - 1, (_eyes[LEFT].cY + _eyes[LEFT].height) - (_eyes[LEFT].lids[BOTTOM].height), _eyes[LEFT].width + 2, _eyes[LEFT].lids[BOTTOM].height + 2, _eyes[LEFT].lids[BOTTOM].radius, _bgColor);
          _spr->fillTriangle(_eyes[LEFT].cX, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y1) - 1, _eyes[LEFT].cX + _eyes[LEFT].width, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y2) - 1, _eyes[LEFT].cX + _eyes[LEFT].lids[BOTTOM].x3, (_eyes[LEFT].cY + _eyes[LEFT].lids[BOTTOM].y3) - 1, _bgColor);
        }
        if (!_eyes[RIGHT].blinking) {
          //TOP
          fillRoundRect(_eyes[RIGHT].cX - 1, _eyes[RIGHT].cY - 1, _eyes[RIGHT].width + 2, _eyes[RIGHT].lids[TOP].height, _eyes[RIGHT].lids[TOP].radius, _bgColor);
          _spr->fillTriangle(_eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y1) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].width, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y2) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].lids[TOP].x3, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[TOP].y3) - 1, _bgColor);

          // BOTTOM
          fillRoundRect(_eyes[RIGHT].cX - 1, (_eyes[RIGHT].cY + _eyes[RIGHT].height) - (_eyes[RIGHT].lids[BOTTOM].height), _eyes[RIGHT].width + 2, _eyes[RIGHT].lids[BOTTOM].height + 2, _eyes[RIGHT].lids[BOTTOM].radius, _bgColor);
          _spr->fillTriangle(_eyes[RIGHT].cX, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y1) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].width, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y2) - 1, _eyes[RIGHT].cX + _eyes[RIGHT].lids[BOTTOM].x3, (_eyes[RIGHT].cY + _eyes[RIGHT].lids[BOTTOM].y3) - 1, _bgColor);
        }
      } else {
      }
    }
  }
#endif
}

//*********************************************************************************************
//  MOUTH FUNCTIONS
//*********************************************************************************************
void Animated_Face::setMouthCrosshair(bool active) {
  mouthCrosshair = active;
}

void Animated_Face::drawMouth() {

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

#if ARDUINO_GFX_AVAILABLE
  if (canvasGfx != NULL) {
    // Default mouth
    canvasGfx->fillRoundRect(mouthLx - (mouthWidth / 2), mouthLy - surprisedPaddingNext, mouthWidth, mouthHeight + (2 * surprisedPaddingNext), mouthRadius, _mouthColor);

    // Top Expression
    canvasGfx->fillRoundRect(mouthLx - (mouthWidth / 2) - 1, mouthLy, mouthWidth + 2, topMouthLip, topMouthRadius, _bgColor);
    // Bottom Expression
    canvasGfx->fillRoundRect(mouthLx - (mouthWidth / 2) - 1, mouthLy + (mouthHeight / 5), mouthWidth + 2, bottomMouthLip, topMouthRadius, _bgColor);
  }
#endif

#if TFT_ESPI_AVAILABLE
  if (_spr != NULL) {
    // Default mouth
    fillRoundRect(mouthLx - (mouthWidth / 2), mouthLy - surprisedPaddingNext, mouthWidth, mouthHeight + (2 * surprisedPaddingNext), mouthRadius, _mouthColor);

    // Top Expression
    fillRoundRect(mouthLx - (mouthWidth / 2) - 1, mouthLy, mouthWidth + 2, topMouthLip, topMouthRadius, _bgColor);
    // Bottom Expression
    fillRoundRect(mouthLx - (mouthWidth / 2) - 1, mouthLy + (mouthHeight / 5), mouthWidth + 2, bottomMouthLip, topMouthRadius, _bgColor);
  }
#endif
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

void Animated_Face::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
  if (_spr != NULL) {
    int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
    if (r > max_radius)
      r = max_radius;

    _spr->fillRect(x, y + r, w, h - (r << 1), color);
    _spr->fillCircleHelper(x + r, y + h - r - 1, r, 1, w - r - r - 1, color);
    _spr->fillCircleHelper(x + r, y + r, r, 2, w - r - r - 1, color);
  }
}
