/*

Repository:
  https://github.com/hamidb80/arduino_ultrasonic_radar

Descriptions:
  an Arduino Uno R3 uses a 270-degree servo motor to rotate 2 ultra sonic sensors. 
  it then measures the distances of frontside and backside and plot them in a 128x160 TFT display.

Acknowledgements:
  Videos:
  - Arduino Tutorial: Using the ST7735 1.8" Color TFT Display with Arduino: https://youtube.com/watch?v=boagCpb6DgY
  - Arduino Tutorial: 1.8" TFT Color Display ST7735 128x160:                https://youtube.com/watch?v=NAyt5kQcn-A
  - How to Control a Servo With an Arduino:                                 https://youtube.com/watch?v=QbgTl6VSA9Y

  Contents:
  - AdaFruitGFX GFX Graphics Library  Graphics Primitives:                  https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
  - convert RGB-888 to RGB-565:                                             https://stackoverflow.com/questions/11471122/rgb888-to-rgb565-bit-shifting
  - RGB-565 color picker:                                                   https://rgbcolorpicker.com/565

*/

// Configs ----------------------------------------------------

// --- choose one of the display libraries
#define AdaFruitGFX
// #define UGClib

// Libs ----------------------------------------------------

#if defined(UGClib)
  #include <TFT.h>  
  #include <SPI.h>
#elif defined(AdaFruitGFX)
  #include <Adafruit_GFX.h>
  #include <Adafruit_ST7735.h>
#endif

#include <Servo.h>

// Pins ----------------------------------------------------

// TODO explain about models of devices

// TFT :: Monitor
#define TFT_CS   10
#define TFT_DS   9
#define TFT_RST  8

// Ultra Sonic :: Distance Meter
#define US_commonTrigPin   4
#define US_echoPin_1       7
#define US_echoPin_2       2

// Engine :: Servo 
#define ServoMotorPin  5
#define ServoMaxDegree 270

// Data Structures ----------------------------------------------------

struct Color {
  uint8_t 
    r, g, b;
};
struct Theme {
  Color 
    far, 
    close;
};

struct Distances {
  long 
    forward, backward;
};

// globals ---------------------------------------------------------

Servo servoMotor;

#if defined(UGClib)
  TFT             tft = TFT            (TFT_CS, TFT_DS, TFT_RST);
#elif defined(AdaFruitGFX)
  Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DS, TFT_RST);
#endif


int 
  W    =  tft.width(),
  H    =  tft.height(),
  size  = min(W, H),
  cx   = W / 2,
  cy   = H / 2,

  max_dist        = 80, // in cm

  virtual_deg     =  0,
  dir             = +1,
  theme_of_choice = 0
  ;

Color 
  black       = {0,   0,   0  },
  dark        = {20,  20,  20 },
  really_dark = {8,   8 ,  8  },
  white       = {255, 255, 255},
  gray        = {100, 100, 100},
  yellow      = {255, 255, 100},
  green       = {61,  255, 0  },
  lemon       = {180, 255, 0  },
  dark_green  = {19,  100, 6  },
  red         = {255, 0,   0  },
  dark_red    = {100, 4,   27 },
  cyan        = {85,  237, 242},
  purple      = {110,  6,   100}
  ;

Theme themes[] = {
  {lemon,  red},
  {cyan,  purple},
  {really_dark, white},
};

// Math ----------------------------------------------------

double deg2rad(int deg){
  return ((double)deg) / 180.0 * PI;
}
double sind(int deg){
  return sin(deg2rad(deg));
}
double cosd(int deg){
  return cos(deg2rad(deg));
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

int pick(float percent, int a, int b){
  int 
    dir = b - a,
    mov = percent * dir;
  return a + mov;
}

// Utils ----------------------------------------------------

Color gradient(int select, int floor, int top, Color head, Color tail){
  // percent is a float number between 0 & 1
  float percent = (float)(select - floor) / (top - floor);
  return {
    pick(percent, head.r, tail.r),
    pick(percent, head.g, tail.g),
    pick(percent, head.b, tail.b),
  };
}

uint16_t toRgb555(Color c){
  return
    ((c.r & 0b11111000) << 8) | 
    ((c.g & 0b11111100) << 3) | 
     (c.b >> 3);
}

// Actions ----------------------------------------------------

void drawLineScreen(Color c, int x1, int y1, int x2, int y2){
  #if defined(UGClib)
    tft.stroke(c.b, c.g, c.r);
    tft.line(x1, y1, x2, y2);
  #elif defined(AdaFruitGFX)
    tft.drawLine(x1, y1, x2, y2, toRgb555(c));
  #endif
}

long readDistance(int echo_pin){
  return 
    microsecondsToCentimeters(
      pulseIn(
        echo_pin, 
        HIGH));
}

void sendWave(){
  digitalWrite(US_commonTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(US_commonTrigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(US_commonTrigPin, LOW);
}
Distances getDistance(){
  sendWave();
  int 
    d1 = readDistance(US_echoPin_1);

  return {d1, d1};
}

// Arduino's built-ins ----------------------------------------------------

void setup() {
  Serial.begin(9600);

  pinMode(US_commonTrigPin, OUTPUT);
  pinMode(US_echoPin_1, INPUT);
  pinMode(US_echoPin_2, INPUT);

  #if defined(UGClib)
    tft.begin();
    tft.background(0, 0, 0);
  #elif defined(AdaFruitGFX)
    tft.initR(INITR_BLACKTAB); 
    tft.fillScreen(ST7735_BLACK); 
  #endif
 
  servoMotor.attach(ServoMotorPin);
}

void loop() {
  int deg = map(virtual_deg, 0, ServoMaxDegree, 0, 180);
  
  servoMotor.write(deg);

  Distances dists = getDistance();
  int 
    mf    = constrain(dists.forward,  10, max_dist),
    mb    = constrain(dists.backward, 10, max_dist),
    rx    = cosd(deg) * size / 2,
    ry    = sind(deg) * size / 2,
    fdx   = rx        * +mf  / max_dist,
    fdy   = ry        * +mf  / max_dist,
    bdx   = rx        * -mb  / max_dist,
    bdy   = ry        * -mb  / max_dist
  ;
  Theme t = themes[theme_of_choice];
  Color 
    cf = gradient(constrain(mf, 10, max_dist), 10, max_dist, t.close, t.far),
    cb = gradient(constrain(mb, 10, max_dist), 10, max_dist, t.close, t.far)
  ;

  drawLineScreen(black, cx-rx, cy-ry, cx+rx,  cy+ry);  
  drawLineScreen(cf,    cx,    cy,    cx+fdx, cy+fdy);
  drawLineScreen(cb,    cx,    cy,    cx+bdx, cy+bdy);

  virtual_deg += dir;
  if     (deg >= 180 && dir == +1) dir = -1;
  else if(deg <= 0   && dir == -1) dir = +1;

  // Serial.println(millis());
  // delay(100);
}
