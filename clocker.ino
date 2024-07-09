/*

Repository:
  https://github.com/hamidb80/arduino_ultrasonic_radar

Descriptions:
  an Arduino Uno R3 uses a 180-degree servo motor to rotate 2 ultra sonic sensors. 
  it then measures the distances of frontside and backside and plot them in a 128x160 TFT display.

Acknowledgements:
  Videos:
  - Arduino Tutorial: Using the ST7735 1.8" Color TFT Display with Arduino: https://youtube.com/watch?v=boagCpb6DgY
  - Arduino Tutorial: 1.8" TFT Color Display ST7735 128x160:                https://youtube.com/watch?v=NAyt5kQcn-A
  - How to Control a Servo With an Arduino:                                 https://youtube.com/watch?v=QbgTl6VSA9Y
  - The Simplest Way to Wire a Button to Arduino (with Internal Pull-Up)    https://youtube.com/watch?v=Xz4iVpdMd-w

  Contents:
  - AdaFruitGFX GFX Graphics Library - Graphics Primitives:                 https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
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

// --- TFT :: Monitor ---

// #define TFT_VDD  5v
// #define TFT_GND  0v
#define TFT_CS   10
#define TFT_RST  8
#define TFT_DC   9
#define TFT_SDA  11
#define TFT_CSK  13
// #define TFT_LED  3.3v

// --- Ultra Sonic :: Distance Meter ---

// #define US_VDD  5v
#define US_trig_1    4
#define US_trig_2    5
#define US_echoPin_1 7
#define US_echoPin_2 2
// #define US_GND  0v

// --- Engine :: Servo ---
#define ServoMotorPin  3

// --- Others ---
#define BtnPin         A0

#define speed          1

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

#define themes_len  4

Servo servoMotor;

auto tft = 
  #if defined(UGClib)        
    TFT            (TFT_CS, TFT_DC, TFT_RST);
  #elif defined(AdaFruitGFX) 
    Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
  #endif

int 
  W    =  tft.width(),
  H    =  tft.height(),
  
  max_dist        = 100, // in cm
  min_dist        = 20,

  deg             =  0,
  dir             = +1,
  
  theme_of_choice =  0,

  degree_offset   = +30,
  degree_calibre  = -1
  ;

bool
  stopped         = false 
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
  purple      = {110,  6,  100},
  light_blue  = {60,   20, 255}
;

Theme themes[themes_len] = {
  {lemon,       red},
  {cyan,        purple},
  {light_blue,  red},
  {really_dark, white},
};

// Math ----------------------------------------------------

double deg2rad(int deg){
  return ((double)deg) / 180.0 * PI;
}
double sind   (int deg){
  return sin(deg2rad(deg));
}
double cosd   (int deg){
  return cos(deg2rad(deg));
}

long ms2cm(long microseconds) {
  // micro-seconds to centi-meters
  return microseconds / 29 / 2;
}

// Utils ----------------------------------------------------

Color gradient(int select, int floor, int top, Color head, Color tail){
  return {
    map(select, floor, top, head.r, tail.r),
    map(select, floor, top, head.g, tail.g),
    map(select, floor, top, head.b, tail.b),
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
  return ms2cm(pulseIn(echo_pin, HIGH));
}
void sendWave(int trigPort){
  digitalWrite(trigPort, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPort, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPort, LOW);
}
Distances getDistance(){
  sendWave(US_trig_1);
  int d1 = readDistance(US_echoPin_1);
  sendWave(US_trig_2);
  int d2 = readDistance(US_echoPin_2);
  return {d1, d2};
}

// setup ----------------------------------------------------

void setup_ultrasonic(int trigPin, int echoPin){
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void setup_tft_screen(){
  #if defined(UGClib)
    tft.begin();
    tft.background(0, 0, 0);
  #elif defined(AdaFruitGFX)
    tft.initR(INITR_BLACKTAB); 
    tft.fillScreen(ST7735_BLACK); 
  #endif
}

void setup_serial(){
  Serial.begin(9600);
}

void setup_servo(int pwnPin){
  servoMotor.attach(pwnPin);
}

void setup_buttons(){
  pinMode(BtnPin, INPUT_PULLUP);
}

void setup() {
  setup_serial();
  
  setup_servo(ServoMotorPin);
  setup_buttons();

  setup_ultrasonic(US_trig_1, US_echoPin_1);
  setup_ultrasonic(US_trig_2, US_echoPin_2);
  
  setup_tft_screen();
}

// run ----------------------------------------------------

void loop() {
    servoMotor.write(deg);
    Distances dists = getDistance();
    int
      virtual_deg = degree_calibre * deg + degree_offset,

      mf     = constrain(dists.forward,  min_dist, max_dist),
      mb     = constrain(dists.backward, min_dist, max_dist)
    ;      

    // log
    // if (false)
    {
      Serial.print(deg);
      Serial.print(", ");
      Serial.print(0);
      Serial.print(", ");
      Serial.print(max_dist);
      Serial.print(", ");
      Serial.print(mb);
      Serial.print(", ");
      Serial.print(mf);
      Serial.println();
    }

    int
      radius = min(W, H),
      cx     = W / 2,
      cy     = H / 2,
      rx    = cosd(virtual_deg) * radius / 2,
      ry    = sind(virtual_deg) * radius / 2,
      
      fdx   = rx * +mf / max_dist,
      fdy   = ry * +mf / max_dist,
      bdx   = rx * -mb / max_dist,
      bdy   = ry * -mb / max_dist
    ;
    Theme 
      t = themes[theme_of_choice % themes_len]
    ;
    Color 
      cf = gradient(constrain(mf, 10, max_dist), 10, max_dist, t.close, t.far),
      cb = gradient(constrain(mb, 10, max_dist), 10, max_dist, t.close, t.far)
    ;

    drawLineScreen(black, cx-rx, cy-ry, cx+rx,  cy+ry);  
    drawLineScreen(cf,    cx,    cy,    cx+fdx, cy+fdy);
    drawLineScreen(cb,    cx,    cy,    cx+bdx, cy+bdy);

    deg += dir * speed;
    if     (deg >= 180 && dir == +1) dir = -1;
    else if(deg <= 0   && dir == -1) dir = +1;
}
