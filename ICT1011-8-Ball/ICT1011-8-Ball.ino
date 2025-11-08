#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>

TinyScreen display = TinyScreen(TinyScreenPlus);

// -------- RGB565 helper --------
static inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// -------- Colors --------
uint16_t COL_BG   = RGB565(0,0,0);          // true black background
uint16_t COL_FG   = RGB565(255,255,255);    // white lines/text
uint16_t COL_DIM  = RGB565(150,150,170);    // dim outer ring
uint16_t COL_TRI  = RGB565(60,125,255);     // blue triangle (classic)
uint16_t COL_TRI_INV = RGB565(220,60,50);   // red triangle (inverted)

// -------- Screen geometry --------
const uint8_t W=96, H=64;
const uint8_t CX=W/2, CY=H/2;

// -------- Buttons --------
#define BTN_UL TSButtonUpperLeft
#define BTN_UR TSButtonUpperRight
#define BTN_LL TSButtonLowerLeft
#define BTN_LR TSButtonLowerRight

// -------- State --------
bool showAnswer=false, shaking=false, inverted=false;
uint32_t shakeStart=0;
const uint32_t SHAKE_MS=650;
uint8_t ansIndex=0;

const char* ANSWERS[] = {
  "It is certain","It is decidedly so","Without a doubt","Yes, definitely",
  "You may rely on it","As I see it, yes","Most likely","Outlook good",
  "Yes","Signs point to yes","Reply hazy, try again","Ask again later",
  "Better not tell you now","Cannot predict now","Concentrate and ask again",
  "Don't count on it","My reply is no","My sources say no",
  "Outlook not so good","Very doubtful"
};

// ---------- tiny draw helpers (no GFX) ----------
void clear(uint16_t col){
  display.clearScreen(); // clears to black; if you want another color, fill manually
  if(col != RGB565(0,0,0)){
    for(uint8_t y=0;y<H;y++) for(uint8_t x=0;x<W;x++) display.drawPixel(x,y,col);
  }
}

void drawCircleOutline(int cx,int cy,int r,uint16_t col){
  // Bresenham
  int x=0, y=r, d=3-2*r;
  auto P=[&](int px,int py){ display.drawPixel(px,py,col); };
  while(y>=x){
    P(cx+x,cy-y); P(cx-x,cy-y); P(cx+x,cy+y); P(cx-x,cy+y);
    P(cx+y,cy-x); P(cx-y,cy-x); P(cx+y,cy+x); P(cx-y,cy+x);
    x++;
    if(d>0){ y--; d += 4*(x-y)+10; } else { d += 4*x+6; }
  }
}

static inline int edge(int x1,int y1,int x2,int y2,int px,int py){
  return (px-x1)*(y2-y1) - (py-y1)*(x2-x1);
}

void fillTriangle(int x1,int y1,int x2,int y2,int x3,int y3,uint16_t col){
  int minX = min(x1,min(x2,x3)), maxX = max(x1,max(x2,x3));
  int minY = min(y1,min(y2,y3)), maxY = max(y1,max(y2,y3));
  for(int y=minY;y<=maxY;y++){
    for(int x=minX;x<=maxX;x++){
      int e1 = edge(x1,y1,x2,y2,x,y);
      int e2 = edge(x2,y2,x3,y3,x,y);
      int e3 = edge(x3,y3,x1,y1,x,y);
      if((e1>=0 && e2>=0 && e3>=0) || (e1<=0 && e2<=0 && e3<=0)){
        display.drawPixel(x,y,col);
      }
    }
  }
}

void drawText(const char* s,int x,int y,uint16_t fg,uint16_t bg){
  display.fontColor(fg, bg);   // IMPORTANT: set background to black so glyphs are readable
  display.setCursor(x,y);
  display.print(s);
}

// simple word-wrap into up to 3 lines of ~13 chars for 96x64
void drawWrappedCentered(const char* msg, uint16_t fg, uint16_t bg){
  // copy to buffer we can edit
  char buf[64]; strncpy(buf, msg, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
  const int maxChars=13, maxLines=3;
  const char* p = buf;
  char lines[3][22]; int lc=0;

  while(*p && lc<maxLines){
    int len=0, lastSpace=-1;
    while(p[len] && len<maxChars){ if(p[len]==' ') lastSpace=len; len++; }
    int take = (p[len]=='\0'||len==maxChars)? len : (lastSpace>=0? lastSpace : len);
    strncpy(lines[lc], p, take); lines[lc][take]='\0';
    p += take;
    while(*p==' ') p++;
    lc++;
  }

  int lineH=8;
  int totalH = lc*lineH;
  int y0 = CY - totalH/2;
  for(int i=0;i<lc;i++){
    int w = strlen(lines[i])*6;                      // 6x8 default font width
    int x = CX - w/2;
    drawText(lines[i], x, y0 + i*lineH, fg, bg);
  }
}

// ---------- screens ----------
void drawIdle(){
  clear(COL_BG);
  drawCircleOutline(CX, CY, 30, COL_DIM);
  drawCircleOutline(CX, CY, 29, COL_FG);
  drawText("8", CX-3, CY-4, COL_FG, COL_BG);        // small centered “8”
}

void drawAnswer(){
  clear(inverted ? COL_BG : COL_BG);                // both cases keep black background
  // ball outline
  drawCircleOutline(CX, CY, 30, COL_DIM);
  drawCircleOutline(CX, CY, 29, COL_FG);

  // answer triangle touching top & base inside ring
  int ax=CX,   ay=CY-18;
  int bx=CX-24,by=CY+16;
  int cx=CX+24,cy=CY+16;
  fillTriangle(ax,ay,bx,by,cx,cy, inverted ? COL_TRI_INV : COL_TRI);
  // triangle outline
  display.drawLine(ax,ay,bx,by,COL_FG);
  display.drawLine(bx,by,cx,cy,COL_FG);
  display.drawLine(cx,cy,ax,ay,COL_FG);

  // wrapped, centered text in white
  drawWrappedCentered(ANSWERS[ansIndex], COL_FG, COL_BG);
}

void drawShake(){
  // simple wobble transform: redraw idle but offset slightly
  clear(COL_BG);
  // wobble center
  uint32_t t = millis() - shakeStart;
  int dx = (int)(sinf(t*0.045f)*2.0f);
  int dy = (int)(cosf(t*0.052f)*2.0f);
  // ball outline with offset
  drawCircleOutline(CX+dx, CY+dy, 30, COL_DIM);
  drawCircleOutline(CX+dx, CY+dy, 29, COL_FG);
  drawText("8", CX-3+dx, CY-4+dy, COL_FG, COL_BG);
}

// ---------- input & main loop ----------
void setup(){
  Wire.begin();
  display.begin();
  display.setFlip(0);
  display.setBrightness(15);
  display.clearScreen();
  randomSeed(analogRead(0));
  drawIdle();
}

void handleButtons(){
  static uint8_t last=0;
  uint8_t b = display.getButtons();

  // UL = reset
  if(!(last & BTN_UL) && (b & BTN_UL)){
    shaking=false; showAnswer=false; drawIdle();
  }
  // UR = ask (shake then reveal)
  if(!(last & BTN_UR) && (b & BTN_UR)){
    if(!shaking){
      ansIndex = random(0, (int)(sizeof(ANSWERS)/sizeof(ANSWERS[0])));
      shaking = true; showAnswer=false; shakeStart = millis();
    }
  }
  // LR = theme toggle (classic/inverted visual for triangle color only)
  if(!(last & BTN_LR) && (b & BTN_LR)){
    inverted = !inverted;
    if(showAnswer) drawAnswer(); else drawIdle();
  }

  last = b;
}

void loop(){
  handleButtons();

  if(shaking){
    if(millis() - shakeStart >= SHAKE_MS){
      shaking=false; showAnswer=true; drawAnswer();
    }else{
      drawShake();
    }
  }
  // small frame delay
  delay(16);
}
