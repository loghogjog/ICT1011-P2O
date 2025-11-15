#ifndef EIGHTBALL_H
#define EIGHTBALL_H

#include <TinyScreen.h>

// Display object (defined in main .cpp/.ino)
extern TinyScreen display;

// Color definitions (RGB565)
static inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

extern const uint16_t COL_WHITE;
extern const uint16_t COL_RING_DIM;
extern const uint16_t COL_TEXT_LIGHT;
extern const uint16_t COL_TEXT_DARK;

extern const uint16_t COL_TRI_CLASSIC;
extern const uint16_t COL_TRI_INVERT;
extern const uint16_t COL_SHADOW_DARK;
extern const uint16_t COL_SHADOW_LIGHT;

// Theme enum and variable
enum Theme { Classic = 0, Inverted = 1 };
extern Theme theme;

// Function prototypes
void drawChar5x7(int x, int y, char c, uint16_t col);
int textWidth5x7_fast(const char* s);
void drawString5x7(int x, int y, const char* s, uint16_t col);
void drawString5x7_shadow(int x, int y, const char* s, uint16_t fg, uint16_t sh);

void drawIdle();
void drawAnswer();
void startShake();
void updateShake();

uint8_t read8BallButtons();
void setup8Ball(TinyScreen &display);
void run8Ball(TinyScreen &display, bool &exitToMenu);

#endif
