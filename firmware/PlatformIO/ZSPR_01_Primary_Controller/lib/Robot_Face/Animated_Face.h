#ifndef ANIMATED_FACE_H
#define ANIMATED_FACE_H

#include <Arduino.h>

#if __has_include(<Arduino_GFX_Library.h>)
  #include <Arduino_GFX_Library.h>
  #define ARDUINO_GFX_AVAILABLE 1
#else
  #define ARDUINO_GFX_AVAILABLE 0
  #define RED 0xF800
  #define GREEN 0x07E0
  #define BLUE 0x001F
  #define WHITE 0xFFFF
  #define BLACK 0x0000
  #define YELLOW 0xFFE0
  #define CYAN 0x07FF
  #define MAGENTA 0xF81F
  #define ORANGE 0xFC00
  #define PINK 0xF81F
  #define PURPLE 0x780F
#endif

#include <TFT_eSPI.h>
#define TFT_ESPI_AVAILABLE 1

#ifndef RGB565
  #define RGB565(r, g, b) ((uint16_t)((((r)&0xF8) << 8) | (((g)&0xFC) << 3) | ((b) >> 3)))
#endif

//#define FACE_DEBUG

// For Identifying eye numbers
#define LEFT 0
#define RIGHT 1
#define LEFT_NEXT 2
#define RIGHT_NEXT 3
#define LEFT_DEFAULT 4
#define RIGHT_DEFAULT 5

#define TOP 0
#define BOTTOM 1

enum cardinal_direction {
  N,
  NE,
  E,
  SE,
  S,
  SW,
  W,
  NW,
  C,
};

struct gridPoint {
  int x;
  int y;
};

struct eyelid {
  int y1;
  int y2;
  int x3;
  int y3;
  int height;
  int radius;
};

struct eyes {
  int width;
  int height;
  int radius;
  uint16_t color;
  int distanceCenterX;
  int distanceCenterY;
  int heightOffset;
  int cX;
  int cY;
  bool open;
  bool blinking;
  eyelid lids[2];
};



class Animated_Face {
public:
  Animated_Face(int width, int height);
  Animated_Face();

#if ARDUINO_GFX_AVAILABLE
  Arduino_RGB_Display *canvasGfx = NULL;
#endif

#if TFT_ESPI_AVAILABLE
  TFT_eSprite *_spr = NULL;
#endif

  //*********************************************************************************************
  //  GENERAL METHODS
  //*********************************************************************************************
#if ARDUINO_GFX_AVAILABLE
  void begin(Arduino_RGB_Display *display, bool dbug = false);
#endif
#if TFT_ESPI_AVAILABLE
  void begin(TFT_eSprite *sprite, bool dbug = false);
#endif
  //void setFramerate(uint8_t fps);
  void update();
  void setAnimiationSpeed(float value);

  //*********************************************************************************************
  //  FACE SETTERS
  //*********************************************************************************************
  void setEyeColor(uint16_t color);
  void setEyeColor(uint16_t l_color, uint16_t r_color);
  void setBGColor(uint16_t color);

  void setCrosshair(bool active);
  //void setSectorLines(bool active);
  //void setFPScounter(bool active);

  void setFaceHorizontalOffset(int16_t offset);
  void setFaceVerticalOffset(int16_t offset);

  void setCuriosity(bool active);
  void setCuriosity(bool active, int interval, int variation);
  void setCuriosityOffsets(int width_change, int height_change);

  void setPositionDistance(unsigned int distance);
  void setPosition(cardinal_direction direction);

  //*********************************************************************************************
  //  EYE FUNCTIONS
  //*********************************************************************************************
  // Set automated eye blinking, minimal blink interval in full seconds and blink interval variation range in full seconds
  void setAutoblinker(bool active, int interval, int variation);
  void setAutoblinker(bool active);

  void setEyeWidth(int both);
  void setEyeWidth(int leftEye, int rightEye);
  void setEyeHeight(int both);
  void setEyeHeight(int leftEye, int rightEye);
  void setEyeHeightOffset(int leftEye, int rightEye);
  void setEyeBorderradius(int both);
  void setEyeBorderradius(int leftEye, int rightEye);
  void setEyeSpacebetween(int space);
  void setEyeSpacebetween(int leftEye, int rightEye);

  //*********************************************************************************************
  //  MOUTH FUNCTIONS
  //*********************************************************************************************
  void addMouth(bool active);
  void addMouth(bool active, int padding);
  void setMouthCrosshair(bool active);

  void setMouthBorderRadius(int mouth);
  void setMouthWidth(int sizeX);
  void setMouthHeight(int sizeY);
  void setMouthPadding(int padding);

private:
  //Arduino_GFX *_display;
  //Arduino_GFX *_display;

  void init();

  //TFT eSPI graphics fixes
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);

  //*********************************************************************************************
  //  PRE-CALCULATIONS AND ACTUAL DRAWINGS
  //*********************************************************************************************
  void drawFace();
  void drawEyes();
  void drawMouth();
  void debug();
  void close(bool left, bool right);
  void open(bool left, bool right);
  void blink(bool left, bool right);
  void close() {
    _eyes[LEFT_NEXT].height = 1;   // closing left eye
    _eyes[RIGHT_NEXT].height = 1;  // closing right eye
    _eyes[LEFT].open = false;      // left eye not opened (=closed)
    _eyes[RIGHT].open = false;     // right eye not opened (=closed)
    _eyes[LEFT].blinking = true;
    _eyes[RIGHT].blinking = true;
  }

  // Open both eyes
  void open() {
    _eyes[LEFT].open = true;   // left eye opened - if true, drawEyes() will take care of opening eyes again
    _eyes[RIGHT].open = true;  // right eye opened
    //_eyes[LEFT_NEXT].eyeLidTop = _eyes[LEFT_NEXT].eyeLidBottom = 0;
    //_eyes[RIGHT_NEXT].eyeLidTop = _eyes[RIGHT_NEXT].eyeLidBottom = 0;
  }

  // Trigger eyeblink animation
  void blink() {
    close();
    open();
  }

  // For general setup - screen size and max. frame rate
  int _screenWidth, _prevScreenWidth;    // OLED display width, in pixels
  int _screenHeight, _prevScreenHeight;  // OLED display height, in pixels
  int _frameInterval = 10;               // default value for 50 frames per second (1000/50 = 20 milliseconds)
  unsigned long fpsTimer = 0;            // for timing the frames per second
  uint8_t _framedivisor = 4;

  //FaceSector _sectors[9];

  float _faceCenter_x, _faceCenter_xNext = 0;
  float _faceCenter_y, _faceCenter_yNext = 0;
  uint16_t _eyeColorDefault = WHITE;
  uint16_t _bgColor = BLACK;
  // bool circular_constrain = false;
  // int radius_con;
  uint8_t _padding = 0;

  int16_t _vertical_padding = 0;
  int16_t _horizontal_padding = 0;
  uint16_t _sector_sizeX;
  uint16_t _sector_sizeY;
  float _cur_value = 0.75;
  float _nex_value = 0.25;


  bool cyclops = false;  // if true, draw only one eye
  //bool eyeL_open = false;  // left eye opened or closed?
  //bool eyeR_open = false;  // right eye opened or closed?
  bool idle_center = false;
  bool cross_hair = false;
  bool sector_lines = false;


  //*********************************************************************************************
  //  Eyes Geometry
  //*********************************************************************************************

  //DEFAULTS
  int eyeWidthDefault = 50;
  int eyeHeightDefault = 50;
  int eyeHeightClosedDefault = 1;
  int eyeHeightOffsetDefault = 0;
  int eyeBorderRadiusDefault = 8;
  int spaceBetweenDefault = 5;
  int spaceHeightDefault = 10;

  bool surprise = false;
  int surprisedEyePadding = 0;
  int surprisedEyePaddingNext;

  //*********************************************************************************************
  //  Mouth Geometry
  //*********************************************************************************************

  // Mouth Defaults
  int mouthLx, mouthLx_Next = 0;
  int mouthLy, mouthLy_Next = 0;

  bool mouthActive = false;
  bool mouthCrosshair = false;

  int mouthPaddingDefault = -15;
  int mouthWidthDefault = 50;
  int mouthHeightDefault = 50;
  int mouthRadiusDefault = 25;
  uint16_t _mouthColor = WHITE;

  int mouthWidth, mouthWidthNext;
  int mouthHeight, mouthHeightNext;
  int mouthRadius, mouthRadiusNext;
  int mouthPadding, mouthPaddingNext;

  bool mouth_happy = 0;
  bool mouth_sad = 0;
  bool mouth_surprised = 0;

  int topMouthLip = 0;
  int topMouthLipNext;
  int topMouthRadius = 0;

  int bottomMouthLip = 0;
  int bottomMouthLipNext;

  int surprisedPadding = 0;
  int surprisedPaddingNext;


  //*********************************************************************************************
  //  Macro Animations
  //*********************************************************************************************

  // Animation - horizontal flicker/shiver
  bool hFlicker = 0;
  bool hFlickerAlternate = 0;
  int hFlickerAmplitude = 2;

  // Animation - vertical flicker/shiver
  bool vFlicker = 0;
  bool vFlickerAlternate = 0;
  int vFlickerAmplitude = 10;

  // Animation - auto blinking
  bool autoblinker = 0;            // activate auto blink animation
  int blinkInterval = 1;           // basic interval between each blink in full seconds
  int blinkIntervalVariation = 4;  // interval variaton range in full seconds, random number inside of given range will be add to the basic blinkInterval, set to 0 for no variation
  unsigned long blinktimer = 0;    // for organising eyeblink timing

  // Animation - idle mode: eyes looking in random directions
  bool idle = false;
  int idleInterval = 1;                  // basic interval between each eye repositioning in full seconds
  int idleIntervalVariation = 3;         // interval variaton range in full seconds, random number inside of given range will be add to the basic idleInterval, set to 0 for no variation
  unsigned long idleAnimationTimer = 0;  // for organising eyeblink timing
  unsigned int idleDistance = 5;
  int idleWidthChange = 0;
  int idleHeightChange = 0;

  // // Animation - eyes confused: eyes shaking left and right
  // bool confused = 0;
  // unsigned long confusedAnimationTimer = 0;
  // int confusedAnimationDuration = 500;
  // bool confusedToggle = 1;

  // // Animation - eyes laughing: eyes shaking up and down
  // bool laugh = 0;
  // unsigned long laughAnimationTimer = 0;
  // int laughAnimationDuration = 500;
  // bool laughToggle = 1;

protected:
  bool setup_complete = false;
  bool _debug = false;
  uint16_t _FakebgColor = RED;
  bool _fps = false;
  uint32_t _lastFps = 0;
  eyes _eyes[6];  //Create default structure for eyes

  void setSurpriseEmotion(bool active) {
    surprise = active;
    if (active)
      surprisedEyePaddingNext = _eyes[LEFT].height / 4;
    else
      surprisedEyePaddingNext = 0;
  }

  void set_eye_lid_position(bool eye, bool side, int y1, int y2, int x3, int y3, int height, int radius);
  //void set_mouth_lid_position(int x1, int x2, int height, int radius);

  // // Bottom happy eyelids offset
  // int eyelidsHappyBottomOffsetMax;
  // int eyelidsHappyBottomOffset = 0;
  // int eyelidsHappyBottomOffsetNext = 0;
};
#endif