#ifndef PLAYER_H
#define PLAYER_H

#include <TinyScreen.h>

extern int playerLane;
extern int playerFrame;
extern const int PLAYER_Y;
extern const int PLAYER_SCALE;
extern unsigned long lastPlayerFrame;
extern const int PLAYER_FRAME_DELAY;

void drawStickman(int x, int y, int frame);
void readButtons();

#endif
