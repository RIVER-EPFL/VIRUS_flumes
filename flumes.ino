#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Relay logic macros (active-LOW module)
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Relay pins
const int relay1 = 9;   // Flume A Pump1 (IN2)
const int relay2 = 8;   // Flume A Pump2 (IN1)
const int relay3 = 11;  // Flume B Pump3 (IN4)
const int relay4 = 10;  // Flume B Pump4 (IN3)

// Button pins (INPUT_PULLUP)
const int btnAuto = 5;
const int btnA    = 6;
const int btnB    = 7;

// Timing
unsigned long pumpInterval = 1UL*3600UL*1000UL;   // 1h (use 1UL*3600UL*1000UL for 1 h)
const unsigned long pauseDur   = 2000UL; //  2 s all-off pause

// Modes
enum Mode { MODE_OFF, MODE_AUTO, MODE_A_ONLY, MODE_B_ONLY, MODE_AB_MANUAL };
Mode mode = MODE_OFF;

// State for display & cycles
unsigned long prevA = 0, prevB = 0, cdA = 0, cdB = 0;
bool a1 = false, a2 = false, b3 = false, b4 = false;

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed"));
    while (1);
  }
  display.clearDisplay();

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  pinMode(btnAuto, INPUT_PULLUP);
  pinMode(btnA,    INPUT_PULLUP);
  pinMode(btnB,    INPUT_PULLUP);

  setMode(MODE_OFF);
}

void loop() {
  unsigned long now = millis();
  bool up = digitalRead(btnAuto) == LOW;
  bool a  = digitalRead(btnA   ) == LOW;
  bool b  = digitalRead(btnB   ) == LOW;

  // 1) A+B → ALL OFF
  if (a && b) {
    setMode(MODE_OFF);
    drawDisplay(); delay(50); return;
  }
  // 2) AUTO → reset & start cycles
  if (up) {
    setMode(MODE_AUTO);
    drawDisplay(); delay(50); return;
  }
  // 3) A alone
  if (a) {
    if (mode == MODE_B_ONLY) {
      // B then A → manual A & B
      digitalWrite(relay1, RELAY_ON); a1 = true;
      mode = MODE_AB_MANUAL;
      drawDisplay(); delay(50); return;
    }
    if (mode == MODE_AUTO) {
      // kill all
      digitalWrite(relay1, RELAY_OFF);
      digitalWrite(relay2, RELAY_OFF);
      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay4, RELAY_OFF);
      a1=a2=b3=b4=false;
    }
    // manual A only
    digitalWrite(relay1, RELAY_ON);  a1 = true;
    digitalWrite(relay2, RELAY_OFF); a2 = false;
    mode = MODE_A_ONLY;
    drawDisplay(); delay(50); return;
  }
  // 4) B alone
  if (b) {
    if (mode == MODE_A_ONLY) {
      // A then B → manual A & B
      digitalWrite(relay3, RELAY_ON); b3 = true;
      mode = MODE_AB_MANUAL;
      drawDisplay(); delay(50); return;
    }
    if (mode == MODE_AUTO) {
      digitalWrite(relay1, RELAY_OFF);
      digitalWrite(relay2, RELAY_OFF);
      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay4, RELAY_OFF);
      a1=a2=b3=b4=false;
    }
    // manual B only
    digitalWrite(relay3, RELAY_ON); b3 = true;
    digitalWrite(relay4, RELAY_OFF);b4 = false;
    mode = MODE_B_ONLY;
    drawDisplay(); delay(50); return;
  }

  // 5) AUTO cycles
  if (mode == MODE_AUTO) {
    if (now - prevA >= pumpInterval) {
      bool was1 = a1;
      digitalWrite(relay1, RELAY_OFF);
      digitalWrite(relay2, RELAY_OFF);
      a1 = a2 = false;
      delay(pauseDur);
      toggleA(was1 ? 2 : 1);
      prevA = millis();
    }
    cdA = pumpInterval - (now - prevA);

    if (now - prevB >= pumpInterval) {
      bool was3 = b3;
      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay4, RELAY_OFF);
      b3 = b4 = false;
      delay(pauseDur);
      toggleB(was3 ? 4 : 3);
      prevB = millis();
    }
    cdB = pumpInterval - (now - prevB);
  }

  // 6) Display
  drawDisplay();
  delay(50);
}

void setMode(Mode m) {
  // kill all
  digitalWrite(relay1, RELAY_OFF);
  digitalWrite(relay2, RELAY_OFF);
  digitalWrite(relay3, RELAY_OFF);
  digitalWrite(relay4, RELAY_OFF);
  a1=a2=b3=b4=false;

  switch (m) {
    case MODE_OFF:
      mode = MODE_OFF;
      break;
    case MODE_AUTO:
      mode = MODE_AUTO;
      prevA = prevB = millis();
      toggleA(1);
      toggleB(3);
      break;
    case MODE_A_ONLY:
      mode = MODE_A_ONLY;
      digitalWrite(relay1, RELAY_ON); a1 = true;
      break;
    case MODE_B_ONLY:
      mode = MODE_B_ONLY;
      digitalWrite(relay3, RELAY_ON); b3 = true;
      break;
    case MODE_AB_MANUAL:
      mode = MODE_AB_MANUAL;
      break;
  }
}

void toggleA(int p) {
  digitalWrite(relay1, p==1 ? RELAY_ON  : RELAY_OFF);
  digitalWrite(relay2, p==2 ? RELAY_ON  : RELAY_OFF);
  a1 = (p==1);  a2 = (p==2);
}
void toggleB(int p) {
  digitalWrite(relay3, p==3 ? RELAY_ON  : RELAY_OFF);
  digitalWrite(relay4, p==4 ? RELAY_ON  : RELAY_OFF);
  b3 = (p==3);  b4 = (p==4);
}

// 5-row OLED: row0=countdown/mode, row1=Flume A:, row2=P1/P2, row3=Flume B:, row4=P3/P4
void drawDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Row 0
  display.setCursor(0, 0);
  if (mode == MODE_AUTO) {
    unsigned long secs = min(cdA, cdB) / 1000UL;
    display.print(F("Next switch in "));
    if      (secs >= 3600) display.print(secs/3600.0,1), display.println(F(" h"));
    else if (secs >=   60) display.print(secs/60),      display.println(F(" m"));
    else                   display.print(secs),         display.println(F(" s"));
  } else if (mode == MODE_AB_MANUAL) {
    display.println(F("MANUAL FLUMES A & B"));
  } else if (mode == MODE_A_ONLY) {
    display.println(F("MANUAL FLUME A"));
  } else if (mode == MODE_B_ONLY) {
    display.println(F("MANUAL FLUME B"));
  } else {
    display.println(F("ALL FLUMES OFF"));
  }

  // Row 1: Flume A label
  display.setCursor(0, 16);
  display.println(F("FLUME A:"));
  // Row 2: P1 and P2
  display.setCursor(0, 27);
  display.print(F("P1: ")); display.print(a1 ? F("ON ") : F("OFF"));
  display.print(F("  P2: ")); display.print(a2 ? F("ON ") : F("OFF"));

  // Row 3: Flume B label
  display.setCursor(0, 42);
  display.println(F("FLUME B:"));
  display.setCursor(0, 53);
  display.print(F("P3: ")); display.print(b3 ? F("ON ") : F("OFF"));
  display.print(F("  P4: ")); display.print(b4 ? F("ON ") : F("OFF"));

  display.display();
}
