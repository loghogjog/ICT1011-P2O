#ifndef SPINTHEWHEEL_H
#define SPINTHEWHEEL_H

#include <TinyScreen.h>

extern TinyScreen display;

void setupSpinTheWheel(TinyScreen &display);
void runSpinTheWheel(TinyScreen &display, bool &exitToMenu);

#endif
