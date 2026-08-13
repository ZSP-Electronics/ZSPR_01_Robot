#ifndef RGB_ROBOFACE_H
#define RGB_ROBOFACE_H

#include <Arduino.h>
#include "Animated_Face.h"


struct EmotionLvls {
  uint8_t happiness;
  uint8_t surprise;
  uint8_t sadness;
  uint8_t tired;
  uint8_t anger;
  uint8_t fear;
  uint8_t disgust;
  uint8_t love;
};

enum eEmotions {
  Normal = 0,
  Happy,
  Excited,
  Surprised,
  Focused,
  Annoyed,
  Skeptic,
  // Frustrated,
  // Unimpressed,
  // Squint,
  // Suspicious,
  Angry,
  Furious,
  Disgust,
  Sad,
  // Worried,
  Tired,
  Sleep,
  // Scared,
  EMOTIONS_COUNT
};

class RGBroboFace : public Animated_Face {
public:
  RGBroboFace(int width, int height);
  RGBroboFace();

  //   void begin(Arduino_RGB_Display *display, bool dbug = false);
  //   void begin(TFT_eSprite *sprite, bool dbug = false);

  //void ClearVariations();

  void setEyeExpression(eEmotions emotion);




private:
  EmotionLvls _emotions;

  void GoTo_Normal();
};

#endif