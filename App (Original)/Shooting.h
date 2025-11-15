#ifndef SHOOTING_H
#define SHOOTING_H

#include <TinyScreen.h>

extern TinyScreen display;

void setupShooting(TinyScreen &display);
void runShooting(TinyScreen &display, bool &exitToMenu);

#endif
