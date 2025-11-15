#ifndef GLOBALS_H
#define GLOBALS_H

#include <TinyScreen.h>

const int SCREEN_WIDTH = 96;
const int SCREEN_HEIGHT = 64;
const int LANES = 3;
const int LANE_WIDTH = SCREEN_WIDTH / LANES;
const int FRAME_DELAY = 250;
const int SCROLL_SPEED = 2;

extern bool gameOver;
extern int scrollOffset;
extern unsigned long lastFrame;

#endif
