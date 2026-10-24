#include "RGBRoboFace.h"

RGBroboFace::RGBroboFace(int width, int height)
  : Animated_Face(width, height) {}
RGBroboFace::RGBroboFace()
  : Animated_Face() {}

// void RGBroboFace::begin(Arduino_RGB_Display *display, bool dbug){
//   Animated_Face.begin(display, dbug);
// }

// void RGBroboFace::begin(TFT_eSprite *sprite, bool dbug){
//   Animated_Face.begin(sprite, dbug);
// }

void RGBroboFace::GoTo_Normal() {
  set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
  set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
  set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, 0, 0);
  set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
}

void RGBroboFace::setEyeExpression(eEmotions emotion) {
  if (emotion != Surprised) {
    setSurpriseEmotion(false);
    GoTo_Normal();
  }

  switch (emotion) {
    //set_eye_lid_position(eye, side, y1, y2, x3, y3, height, radius)
    case Normal:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
      break;
    case Angry:
      set_eye_lid_position(LEFT, TOP, 0, 0, _eyes[LEFT].width, _eyes[LEFT].height / 2, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
      break;
    case Disgust:
      set_eye_lid_position(LEFT, TOP, 0, 0, _eyes[LEFT].width, _eyes[LEFT].height / 3, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[LEFT].height / 2, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, _eyes[RIGHT].width, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, _eyes[RIGHT].height, _eyes[RIGHT].height, 0, _eyes[RIGHT].height / 2, 0, 0);
      break;
    case Happy:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[LEFT].height / 3, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 3, 0);
      break;
    case Excited:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[LEFT].height / 2, _eyes[LEFT].radius * 2);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 2, _eyes[RIGHT].radius * 2);
      break;
    case Sad:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, _eyes[LEFT].height / 2, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, _eyes[RIGHT].width, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
      break;
    // case Worried:
    //   break;
    case Focused:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, _eyes[LEFT].height / 3, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[LEFT].height / 2, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, _eyes[LEFT].height / 3, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 2, 0);
      break;
    case Annoyed:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, _eyes[LEFT].height / 2, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, _eyes[LEFT].height / 2, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[LEFT].height / 3, 0);
      break;
    case Surprised:
      setSurpriseEmotion(true);
      break;
    case Skeptic:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
      break;
    // case Frustrated:
    //   break;
    // case Unimpressed:
    //   break;
    case Tired:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 2, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, _eyes[RIGHT].width, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 2, 0);
      break;
    case Sleep:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, (_eyes[LEFT].height / 2) - 1, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, (_eyes[LEFT].height / 2) - 1, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, (_eyes[RIGHT].height / 2) - 1, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, (_eyes[RIGHT].height / 2) - 1, 0);
      break;
    // case Suspicious:
    //   break;
    // case Squint:
    //   break;
    case Furious:
      set_eye_lid_position(LEFT, TOP, 0, 0, _eyes[LEFT].width, _eyes[LEFT].height / 2, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 3, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, _eyes[RIGHT].height / 2, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, _eyes[RIGHT].height / 3, 0);
      break;
    // case Scared:
    //   break;
    default:
      set_eye_lid_position(LEFT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(LEFT, BOTTOM, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, TOP, 0, 0, 0, 0, 0, 0);
      set_eye_lid_position(RIGHT, BOTTOM, 0, 0, 0, 0, 0, 0);
      break;
  }
}
