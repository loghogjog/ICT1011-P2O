/*
  TinyZero + TinyScreen (96x64) — Spin The Wheel
  - UL Subtract, UR Add, LR Spin / Stop, LL Back to Main Menu

  Requires: TinyScreen library by TinyCircuits
*/

#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

extern TinyScreen display;

// -------- Colors (RGB565) --------
static inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t COL_BG    = RGB565(10,12,20);
const uint16_t COL_FG    = RGB565(255,255,255);
const uint16_t COL_DIM   = RGB565(120,130,150);
const uint16_t COL_HAND  = RGB565(255,220,70);
const uint16_t COL_HILITE= RGB565(90,110,160);

// -------- Geometry --------
const uint8_t  SCR_W = 96, SCR_H = 64;
const uint8_t  CX = SCR_W/2, CY = SCR_H/2;
const uint8_t  R_OUT = 22, R_IN = 14, R_LABEL = 26;

static inline float TAU(){ return 6.28318530718f; }
static inline float stepSize(uint8_t N){ return TAU()/N; }
static inline float normAngle(float a){ float t=fmodf(a,TAU()); return t<0?t+TAU():t; }

static inline float thetaForSectorAtTop(uint8_t k, uint8_t N){
  return -((float)k - 0.5f) * stepSize(N);
}

static inline uint8_t sectorIndexAtPointer(float theta, uint8_t N){
  float s = stepSize(N);
  float m = normAngle(-theta) / s;
  uint8_t k = (uint8_t)floorf(m) + 1;  // 1..N
  if (k > N) k = 1;
  return k;
}

// -------- Buttons --------
#define BTN_UPPER_LEFT   TSButtonUpperLeft
#define BTN_UPPER_RIGHT  TSButtonUpperRight
#define BTN_LOWER_LEFT   TSButtonLowerLeft
#define BTN_LOWER_RIGHT  TSButtonLowerRight

// -------- State --------
uint8_t  maxN=10, value=1;
bool     spinning=false, skipSpin=false;
float    wheelAngle=0.0f;
uint32_t spinStartMs=0, spinDurationMs=0;
float    spinStartAngle=0, spinTotal=0;

// -------- Tiny 3×5 digits --------
const uint8_t DIGIT[10][5] = {
  {0b111,0b101,0b101,0b101,0b111}, //0
  {0b010,0b110,0b010,0b010,0b111}, //1
  {0b111,0b001,0b111,0b100,0b111}, //2
  {0b111,0b001,0b111,0b001,0b111}, //3
  {0b101,0b101,0b111,0b001,0b001}, //4
  {0b111,0b100,0b111,0b001,0b111}, //5
  {0b111,0b100,0b111,0b101,0b111}, //6
  {0b111,0b001,0b001,0b001,0b001}, //7
  {0b111,0b101,0b111,0b101,0b111}, //8
  {0b111,0b101,0b111,0b001,0b111}  //9
};

void drawDigit3x5(uint8_t dx, uint8_t dy, uint8_t d, uint16_t col){
  if(d>9) d=9;
  for(uint8_t y=0;y<5;y++){
    uint8_t row = DIGIT[d][y];
    for(uint8_t x=0;x<3;x++){
      if(row & (0b100>>x)) display.drawRect(dx+ x*2, dy+ y*2, 2, 2, 1, col);
    }
  }
}

void drawNumberCentered(uint8_t n, uint16_t col){
  if(n<10){
    uint8_t w = 3*2, h = 5*2;
    drawDigit3x5(CX - w/2, CY - h/2, n, col);
  }else{ // 10
    uint8_t w = (3*2)*2 + 2;
    uint8_t x = CX - w/2;
    drawDigit3x5(x,           CY - 5, 1, col);
    drawDigit3x5(x + 3*2 + 2, CY - 5, 0, col);
  }
}

// -------- Drawing helpers --------
void drawHandTop(){
  float a = -TAU()/4; // up
  int x = (int)(CX + cosf(a)*(R_IN-1));
  int y = (int)(CY + sinf(a)*(R_IN-1));
  display.drawLine(CX, CY-6, CX, y, COL_HAND);
  display.drawLine(x, y, x-1, y+3, COL_HAND);
  display.drawLine(x, y, x+1, y+3, COL_HAND);
}

void clearScreen(uint16_t col){
  display.drawRect(0,0,SCR_W,SCR_H, 1, col);
}

// --- OPTIMIZED DRAW WHEEL ---
void drawWheel(uint8_t N, int8_t showIdx){
  int8_t ticks[20][4];
  int8_t labels[20][2];
  uint8_t labelNums[20];

  for(uint8_t i=0;i<N;i++){
    float aTick = -TAU()/4 + i*(TAU()/N) + wheelAngle;
    ticks[i][0] = (int)(CX + cosf(aTick)*R_IN);
    ticks[i][1] = (int)(CY + sinf(aTick)*R_IN);
    ticks[i][2] = (int)(CX + cosf(aTick)*R_OUT);
    ticks[i][3] = (int)(CY + sinf(aTick)*R_OUT);

    float aCenter = -TAU()/4 + (i+0.5f)*(TAU()/N) + wheelAngle;
    labels[i][0] = (int)(CX + cosf(aCenter)*R_LABEL);
    labels[i][1] = (int)(CY + sinf(aCenter)*R_LABEL);
    labelNums[i] = i + 1;
  }

  uint8_t idx = sectorIndexAtPointer(wheelAngle, N);
  uint8_t centerNum = (showIdx>0) ? (uint8_t)showIdx : idx;

  display.drawRect(0, 0, SCR_W, SCR_H, 1, COL_BG);

  for(uint8_t i=0; i<N; i++){
     display.drawLine(ticks[i][0], ticks[i][1], ticks[i][2], ticks[i][3], COL_FG);
     int lx = labels[i][0];
     int ly = labels[i][1];
     if(labelNums[i]<10){
       drawDigit3x5(lx-3, ly-3, labelNums[i], COL_FG);
     } else {
       drawDigit3x5(lx-5, ly-3, 1, COL_FG);
       drawDigit3x5(lx+1,  ly-3, 0, COL_FG);
     }
  }
  drawHandTop();
  drawNumberCentered(centerNum, COL_FG);
}

// -------- Spin logic --------
void beginSpin(){
  spinning = true; skipSpin = false;
  value = 1 + (uint8_t)(rand() % maxN);
  float s = stepSize(maxN);
  float margin = s * 0.08f;
  float maxOff = s*0.5f - margin;
  float offset = ((float)rand()/RAND_MAX)*2.0f*maxOff - maxOff;
  float targetTheta = thetaForSectorAtTop(value, maxN) + offset;
  spinStartAngle = wheelAngle;
  float cur = normAngle(wheelAngle);
  float desired = normAngle(targetTheta);
  float delta = desired - cur; if(delta<0) delta += TAU();
  uint8_t turns = 2 + (rand()%3);
  spinTotal = turns*TAU() + delta;
  spinDurationMs = 1500 + turns*300;
  spinStartMs = millis();
}

void updateSpin(){
  if(!spinning) return;
  if(skipSpin){ wheelAngle = normAngle(spinStartAngle + spinTotal); spinning=false; return; }
  uint32_t elapsed = millis() - spinStartMs;
  if(elapsed >= spinDurationMs){ wheelAngle = normAngle(spinStartAngle + spinTotal); spinning=false; return; }
  float t = (float)elapsed / (float)spinDurationMs;
  float ease = 1.0f - powf(1.0f - t, 3.0f);
  wheelAngle = normAngle(spinStartAngle + spinTotal * ease);
}

// -------- Buttons --------
struct BtnState { uint8_t last=0; uint32_t tUL=0, tUR=0; } btn;
uint8_t readButtons(){ return display.getButtons(); }

void handleButtons(bool &quitGame){
  uint8_t m = readButtons();
  uint32_t now = millis();
  bool ulPrev = btn.last & BTN_UPPER_LEFT;
  bool urPrev = btn.last & BTN_UPPER_RIGHT;
  bool ul = m & BTN_UPPER_LEFT;
  bool ur = m & BTN_UPPER_RIGHT;

  if(!ulPrev && ul) btn.tUL = now;
  if(ulPrev && !ul){
    int delta = (now - btn.tUL > 500) ? -5 : -1;
    if(!spinning){
      int v = (int)maxN + delta;
      while(v<1) v += 10; while(v>10) v -= 10;
      maxN = (uint8_t)v;
      if(value>maxN) value=maxN;
      wheelAngle = thetaForSectorAtTop(value, maxN);
    }
  }
  if(!urPrev && ur) btn.tUR = now;
  if(urPrev && !ur){
    int delta = (now - btn.tUR > 500) ? +5 : +1;
    if(!spinning){
      int v = (int)maxN + delta;
      while(v<1) v += 10; while(v>10) v -= 10;
      maxN = (uint8_t)v;
      if(value>maxN) value=maxN;
      wheelAngle = thetaForSectorAtTop(value, maxN);
    }
  }

  // ---- LOWER LEFT → QUIT GAME ----
  static bool latchLL = false;
  static bool latchLR = false;

  if (m & BTN_LOWER_LEFT) {
      if (!latchLL) {
          latchLL = true;
          quitGame = true;
      }
  } else latchLL = false;

  // ---- LOWER RIGHT → SPIN / STOP ----
  if (m & BTN_LOWER_RIGHT) {
      if (!latchLR) {
          latchLR = true;
          if (!spinning) beginSpin();
          else skipSpin = true;
      }
  } else latchLR = false;

  btn.last = m;
}

// -------- Arduino --------
void setupSpinTheWheel(TinyScreen &display){
  Wire.begin();
  display.begin();
  display.setFlip(0);
  display.setBrightness(10); 
  randomSeed(analogRead(0));
  wheelAngle = thetaForSectorAtTop(value, maxN);
  drawWheel(maxN, -1);
}

void runSpinTheWheel(TinyScreen &display, bool &quitGame){
  handleButtons(quitGame);
  updateSpin();
  int8_t showIdx = spinning ? (int8_t)sectorIndexAtPointer(wheelAngle, maxN) : (int8_t)value;
  drawWheel(maxN, showIdx);
}
