#include "8ball.h"
#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern TinyScreen display;

// ---------------- Colors (RGB565) ----------------
const uint16_t COL_WHITE       = RGB565(255,255,255);
const uint16_t COL_RING_DIM    = RGB565(150,170,200);
const uint16_t COL_TEXT_LIGHT  = RGB565(230,234,245);
const uint16_t COL_TEXT_DARK   = RGB565(30,30,40);

// High-contrast triangle fills + text shadows
const uint16_t COL_TRI_CLASSIC = RGB565(16,40,96);     // deep navy for Classic
const uint16_t COL_TRI_INVERT  = RGB565(250,220,120);  // light gold for Inverted
const uint16_t COL_SHADOW_DARK  = RGB565(10,10,14);
const uint16_t COL_SHADOW_LIGHT = RGB565(235,240,255);

// theme
Theme theme = Classic;

// -------------- Tiny 5x7 transparent font ---------------
// ASCII 32..90 subset (space..'Z'). Each glyph 5 columns x 7 rows, MSB top.
const uint8_t FONT5x7[][5] = {
  // 32 ' ' .. 47 '/'
  {0x00,0x00,0x00,0x00,0x00}, // ' '
  {0x00,0x00,0x5F,0x00,0x00}, // '!'
  {0x00,0x07,0x00,0x07,0x00}, // '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
  {0x23,0x13,0x08,0x64,0x62}, // '%'
  {0x36,0x49,0x55,0x22,0x50}, // '&'
  {0x00,0x05,0x03,0x00,0x00}, // '''
  {0x00,0x1C,0x22,0x41,0x00}, // '('
  {0x00,0x41,0x22,0x1C,0x00}, // ')'
  {0x14,0x08,0x3E,0x08,0x14}, // '*'
  {0x08,0x08,0x3E,0x08,0x08}, // '+'
  {0x00,0x50,0x30,0x00,0x00}, // ','
  {0x08,0x08,0x08,0x08,0x08}, // '-'
  {0x00,0x60,0x60,0x00,0x00}, // '.'
  {0x20,0x10,0x08,0x04,0x02}, // '/'

  // 48 '0' .. '9'
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9

  // 58 ':' .. 64 '@'
  {0x00,0x36,0x36,0x00,0x00}, // ':'
  {0x00,0x56,0x36,0x00,0x00}, // ';'
  {0x08,0x14,0x22,0x41,0x00}, // '<'
  {0x14,0x14,0x14,0x14,0x14}, // '='
  {0x00,0x41,0x22,0x14,0x08}, // '>'
  {0x02,0x01,0x51,0x09,0x06}, // '?'
  {0x32,0x49,0x79,0x41,0x3E}, // '@'

  // 65 'A' .. 90 'Z'
  {0x7E,0x11,0x11,0x11,0x7E}, // A
  {0x7F,0x49,0x49,0x49,0x36}, // B
  {0x3E,0x41,0x41,0x41,0x22}, // C
  {0x7F,0x41,0x41,0x22,0x1C}, // D
  {0x7F,0x49,0x49,0x49,0x41}, // E
  {0x7F,0x09,0x09,0x09,0x01}, // F
  {0x3E,0x41,0x49,0x49,0x7A}, // G
  {0x7F,0x08,0x08,0x08,0x7F}, // H
  {0x00,0x41,0x7F,0x41,0x00}, // I
  {0x20,0x40,0x41,0x3F,0x01}, // J
  {0x7F,0x08,0x14,0x22,0x41}, // K
  {0x7F,0x40,0x40,0x40,0x40}, // L
  {0x7F,0x02,0x0C,0x02,0x7F}, // M
  {0x7F,0x04,0x08,0x10,0x7F}, // N
  {0x3E,0x41,0x41,0x41,0x3E}, // O
  {0x7F,0x09,0x09,0x09,0x06}, // P
  {0x3E,0x41,0x51,0x21,0x5E}, // Q
  {0x7F,0x09,0x19,0x29,0x46}, // R
  {0x46,0x49,0x49,0x49,0x31}, // S
  {0x01,0x01,0x7F,0x01,0x01}, // T
  {0x3F,0x40,0x40,0x40,0x3F}, // U
  {0x1F,0x20,0x40,0x20,0x1F}, // V
  {0x7F,0x20,0x18,0x20,0x7F}, // W
  {0x63,0x14,0x08,0x14,0x63}, // X
  {0x07,0x08,0x70,0x08,0x07}, // Y
  {0x61,0x51,0x49,0x45,0x43}  // Z
};

void drawChar5x7(int x, int y, char c, uint16_t col){
  if(c < 32 || c > 90) c = '?';
  const uint8_t* g = FONT5x7[c - 32];
  for (uint8_t cx=0; cx<5; cx++){
    uint8_t colBits = g[cx];
    for (uint8_t cy=0; cy<7; cy++){
      if (colBits & (1 << cy)){
        display.drawPixel(x + cx, y + cy, col);
      }
    }
  }
}
inline int textWidth5x7_fast(const char* s){
  size_t n = strlen(s);
  return n ? (int)(n*6 - 1) : 0; // 5 px glyph + 1 px spacing, minus last gap
}
void drawString5x7(int x, int y, const char* s, uint16_t col){
  int cx=x; for(const char* p=s; *p; ++p){ drawChar5x7(cx,y,*p,col); cx+=6; }
}
void drawString5x7_shadow(int x, int y, const char* s, uint16_t fg, uint16_t sh){
  drawString5x7(x+1, y+1, s, sh); // 1 px shadow
  drawString5x7(x,   y,   s, fg);
}

// ---------------- Answers ----------------
const char* ANSWERS[] = {
  "IT IS CERTAIN","IT IS DECIDEDLY SO","WITHOUT A DOUBT","YES DEFINITELY",
  "YOU MAY RELY ON IT","AS I SEE IT YES","MOST LIKELY","OUTLOOK GOOD",
  "YES","SIGNS POINT TO YES","REPLY HAZY TRY AGAIN","ASK AGAIN LATER",
  "BETTER NOT TELL YOU NOW","CANNOT PREDICT NOW","CONCENTRATE AND ASK AGAIN",
  "DON'T COUNT ON IT","MY REPLY IS NO","MY SOURCES SAY NO",
  "OUTLOOK NOT SO GOOD","VERY DOUBTFUL"
};
const uint8_t N_ANS = sizeof(ANSWERS)/sizeof(ANSWERS[0]);

// -------------- State --------------
bool shaking=false;
uint32_t shakeStart=0;
uint16_t shakeDur=800;
uint8_t ansIdx=0;

// -------------- Fast primitives --------------
void ringFast(int cx, int cy, int r, uint16_t col){
  int x = r, y = 0, err = 1 - r;
  while (x >= y){
    display.drawPixel(cx + x, cy + y, col);
    display.drawPixel(cx + y, cy + x, col);
    display.drawPixel(cx - y, cy + x, col);
    display.drawPixel(cx - x, cy + y, col);
    display.drawPixel(cx - x, cy - y, col);
    display.drawPixel(cx - y, cy - x, col);
    display.drawPixel(cx + y, cy - x, col);
    display.drawPixel(cx + x, cy - y, col);
    y++;
    if (err < 0) err += 2*y + 1;
    else { x--; err += 2*(y - x) + 1; }
  }
}

static inline int xAtY(int x1,int y1,int x2,int y2,int y){
  if (y2 == y1) return x1;
  return x1 + (int)((int32_t)(x2 - x1) * (y - y1) / (y2 - y1));
}

void fillTriangleFast(int ax,int ay,int bx,int by,int cx,int cy,uint16_t col){
  if (ay > by){ int t=ay; ay=by; by=t; t=ax; ax=bx; bx=t; }
  if (by > cy){ int t=by; by=cy; cy=t; t=bx; bx=cx; cx=t; }
  if (ay > by){ int t=ay; ay=by; by=t; t=ax; ax=bx; bx=t; }

  for (int y = ay; y <= by; ++y){
    int xl = xAtY(ax,ay,bx,by,y);
    int xr = xAtY(ax,ay,cx,cy,y);
    if (xl > xr){ int t=xl; xl=xr; xr=t; }
    display.drawLine(xl, y, xr, y, col);
  }
  for (int y = by; y <= cy; ++y){
    int xl = xAtY(bx,by,cx,cy,y);
    int xr = xAtY(ax,ay,cx,cy,y);
    if (xl > xr){ int t=xl; xl=xr; xr=t; }
    display.drawLine(xl, y, xr, y, col);
  }
}

// Word wrap into up to 3 lines inside a max width
uint8_t wrapLines(const char* src, char out[3][22], uint8_t maxLines, int maxWidth){
  char buf[96]; strncpy(buf, src, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
  uint8_t lines=0;
  char* token = strtok(buf, " ");
  char line[64]; line[0]=0;

  while (token){
    char tryLine[64];
    if (line[0]==0) {
      strncpy(tryLine, token, sizeof(tryLine)-1); tryLine[sizeof(tryLine)-1]=0;
    } else {
      snprintf(tryLine, sizeof(tryLine), "%s %s", line, token);
      tryLine[sizeof(tryLine)-1]=0;
    }
    if (textWidth5x7_fast(tryLine) <= maxWidth){
      strncpy(line, tryLine, sizeof(line)-1); line[sizeof(line)-1]=0;
    } else {
      if (lines < maxLines){
        strncpy(out[lines], line, 21); out[lines][21]=0;
        lines++; line[0]=0;
        strncpy(line, token, sizeof(line)-1); line[sizeof(line)-1]=0;
      }
    }
    token = strtok(NULL, " ");
  }
  if (line[0] && lines < maxLines){
    strncpy(out[lines], line, 21); out[lines][21]=0; lines++;
  }
  return lines;
}

// -------------- Shake --------------
const int8_t SIN32[32] = {
   0,  8, 16, 23, 29, 33, 36, 38,
  39, 38, 36, 33, 29, 23, 16,  8,
   0, -8,-16,-23,-29,-33,-36,-38,
 -39,-38,-36,-33,-29,-23,-16, -8
};

void startShake(){
  shaking = true; shakeStart = millis();
}
void updateShake(){
  uint32_t t = millis() - shakeStart;
  if (t >= shakeDur){
    shaking = false;
    ansIdx = random(N_ANS);
    drawAnswer();
    return;
  }

  // Smooth, decaying wobble
  const uint8_t speedPhase = 4; 
  uint8_t phase = (uint8_t)((t * speedPhase) >> 5) & 31;

  const int16_t maxAmp = 8;  
  const int16_t minAmp = 2;   
  int16_t amp = maxAmp - (int32_t)(maxAmp - minAmp) * t / shakeDur;

  int dx = (amp * SIN32[phase]) / 40;
  int dy = (amp * SIN32[(phase + 8) & 31]) / 40;

  // Parallax for inner window/8
  int inx = -dx / 2;
  int iny = -dy / 2;

  display.clearScreen();
  ringFast(48 + dx, 32 + dy, 26, COL_RING_DIM);
  ringFast(48 + inx, 26 + iny, 11,
           (theme==Classic)? RGB565(210,220,235) : RGB565(40,40,40));
  uint16_t eightCol = (theme==Classic)? COL_TEXT_LIGHT : COL_TEXT_DARK;
  drawChar5x7(48 - 3 + inx, 26 - 3 + iny, '8', eightCol);
}

// -------------- Draw screens --------------
void drawIdle(){
  display.clearScreen();
  ringFast(48,32,26, COL_RING_DIM);
  ringFast(48,26,11, (theme==Classic)? RGB565(210,220,235) : RGB565(40,40,40));
  uint16_t eightCol = (theme==Classic)? COL_TEXT_LIGHT : COL_TEXT_DARK;
  drawChar5x7(48-3, 26-3, '8', eightCol);
}

void drawAnswer(){
  display.clearScreen();

  // outer ring
  ringFast(48,32,26, COL_RING_DIM);

  // triangle points
  int ax=48, ay=16;          // top
  int bx=24, by=48;          // bottom-left
  int cx=72, cy=48;          // bottom-right

  // theme-aware triangle fill and outline
  uint16_t triCol     = (theme==Classic)? COL_TRI_CLASSIC : COL_TRI_INVERT;
  uint16_t triOutline = (theme==Classic)? COL_WHITE      : COL_TEXT_DARK;

  fillTriangleFast(ax,ay,bx,by,cx,cy, triCol);
  display.drawLine(ax,ay,bx,by,triOutline);
  display.drawLine(ax,ay,cx,cy,triOutline);
  display.drawLine(bx,by,cx,cy,triOutline);

  // wrap and draw text with shadow for readability
  char lines[3][22]; for (int i=0;i<3;i++) lines[i][0]=0;
  uint8_t L = wrapLines(ANSWERS[ansIdx], lines, 3, 60);

  int blockH = L * 8; // 7 px glyph + 1 px gap
  int y0 = 32 - blockH/2;

  uint16_t textFG = (theme==Classic)? COL_TEXT_LIGHT : COL_TEXT_DARK;
  uint16_t textSH = (theme==Classic)? COL_SHADOW_DARK: COL_SHADOW_LIGHT;

  for (uint8_t i=0; i<L; ++i){
    int w = textWidth5x7_fast(lines[i]);
    int x = 48 - w/2;
    int y = y0 + i*8;
    drawString5x7_shadow(x, y, lines[i], textFG, textSH);
  }
}

// -------------- Buttons --------------
#define BTN_UL TSButtonUpperLeft
#define BTN_UR TSButtonUpperRight
#define BTN_LL TSButtonLowerLeft
#define BTN_LR TSButtonLowerRight

uint8_t read8BallButtons(){ return display.getButtons(); }

// -------------- Arduino --------------
void setup8Ball(TinyScreen &display){
  Wire.begin();
  display.begin();
  display.setFlip(0);
  display.setBrightness(12);   // lower avoids flicker
  randomSeed(analogRead(0));
  drawIdle();
}

void run8Ball(TinyScreen &display, bool &exitToMenu){
  uint8_t b = read8BallButtons();

  if (b & BTN_UR){           // Ask -> shake, then answer
    if (!shaking){ startShake(); }
  }
  if (b & BTN_UL){           // Reset
    shaking=false; drawIdle();
  }
  if (b & BTN_LR){           // Theme toggle
    theme = (theme==Classic)? Inverted : Classic;
    if (!shaking) drawIdle();
    delay(200); // debounce
  }
  
  if (b & BTN_LL){           // Return to app menu
    exitToMenu = true;  // signal to exit to main menu

  }

  if (shaking) updateShake();

  delay(16); // ~60 fps pacing
}