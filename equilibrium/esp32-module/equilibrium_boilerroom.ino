

#include "OOCSI.h"
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// --- WiFi credentials ---
const char* ssid = "...";
const char* password = "...";

// --- OOCSI connection settings ---
// A random suffix is appended at boot so re-uploading never collides with
// the previous connection still registered under the same name.
String OOCSIName;
const char* hostserver = "oocsi.id.tue.nl";
OOCSI oocsi = OOCSI();
const char* CHANNEL = "OOCSI-things/team-...";

// --- LED strip ---
#define STRIP_PIN 5
#define NUM_LEDS  30
Adafruit_NeoPixel strip(NUM_LEDS, STRIP_PIN, NEO_GRBW + NEO_KHZ800);

// --- OLED display (I2C, SH1106 driver) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 6
#define OLED_SCL 7
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Direction thresholds (calibrated to the mounted valve's real readings) ---
// Axis range is roughly -2.0 to 2.0, wrapping at the ends.
const float D1_EAST = -1.0;
const float D1_WEST = 1.0;
const float D1_MAX_ABS = 2.0;  // South sits at the wrap point, near +2.0 or -2.0
const float D1_TOL = 0.25;     // adjust if steps feel too strict or too loose

enum Direction { DIR_EAST, DIR_SOUTH, DIR_WEST };

bool isAtDirection(Direction dir, float d1v) {
  switch (dir) {
    case DIR_EAST:  return fabs(d1v - D1_EAST) <= D1_TOL;
    case DIR_WEST:  return fabs(d1v - D1_WEST) <= D1_TOL;
    case DIR_SOUTH: return fabs(d1v) >= (D1_MAX_ABS - D1_TOL);
  }
  return false;
}

// --- Sequence (edit to your liking) ---
const int SEQUENCE_LEN = 3;
const float SEQUENCE_PRESSURE[SEQUENCE_LEN] = {1.0, 2.0, 3.0};   // East, South, West pressures
const Direction SEQUENCE_DIR[SEQUENCE_LEN]  = {DIR_EAST, DIR_SOUTH, DIR_WEST};

const unsigned long HOLD_DURATION = 3000;
const unsigned long LED_START_DELAY = 500;

int currentStep = 0;
bool revealed = false;
bool awaitingUnlock = true;
bool inZone = false;
unsigned long zoneStartTime = 0;
bool isSolved = false;
float d1 = 0;

void setStripColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(r, g, b, 0));
  strip.show();
}

void updateProgressBar() {
  int segPerStep = NUM_LEDS / SEQUENCE_LEN;
  int litFromSteps = currentStep * segPerStep;

  float holdProgress = 0;
  if (revealed && inZone) {
    unsigned long elapsed = millis() - zoneStartTime;
    if (elapsed > LED_START_DELAY) {
      holdProgress = constrain((float)(elapsed - LED_START_DELAY) / (HOLD_DURATION - LED_START_DELAY), 0.0, 1.0);
    }
  }
  int litFromHold = holdProgress * segPerStep;
  int totalLit = litFromSteps + litFromHold;

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < totalLit) strip.setPixelColor(i, strip.Color(0, 200, 100, 0));
    else strip.setPixelColor(i, strip.Color(255, 30, 0, 0));
  }
  strip.show();
}

void showWaiting() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 10);
  display.println("AWAITING SIGNAL");
  display.println("FROM CONTROL ROOM...");
  display.display();
}

void showPressure(float p, int stepNum) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print("STEP "); display.print(stepNum); display.print("/"); display.println(SEQUENCE_LEN);
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print(p, 1);
  display.println(" BAR");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println("HOLD 3s AT POSITION");
  display.display();
}

void showStepComplete() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 20);
  display.println("STEP COMPLETE");
  display.println("AWAITING NEXT SIGNAL...");
  display.display();
}

void showStabilized() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 20);
  display.println("SYSTEM STABILIZED");
  display.println("PRESSURE NORMAL");
  display.display();
}

void processOOCSI() {
  d1 = oocsi.getFloat("mr-kip_d1", d1);
  Serial.print("mr-kip_d1 = ");
  Serial.println(d1);

  if (oocsi.has("unlock_next")) {
    int u = oocsi.getInt("unlock_next", 0);
    if (u == currentStep + 1 && awaitingUnlock && !revealed) {
      revealed = true;
      awaitingUnlock = false;
      showPressure(SEQUENCE_PRESSURE[currentStep], currentStep + 1);

      oocsi.newMessage(CHANNEL);
      oocsi.addInt("revealed_step", currentStep + 1);
      oocsi.addFloat("revealed_pressure", SEQUENCE_PRESSURE[currentStep]);
      oocsi.sendMessage();
    }
  }

  if (revealed && !awaitingUnlock && !isSolved) {
    bool nowInZone = isAtDirection(SEQUENCE_DIR[currentStep], d1);

    if (nowInZone && !inZone) zoneStartTime = millis();
    if (!nowInZone) zoneStartTime = 0;
    inZone = nowInZone;

    if (inZone && millis() - zoneStartTime >= HOLD_DURATION) {
      oocsi.newMessage(CHANNEL);
      oocsi.addInt("step_done", currentStep + 1);
      oocsi.sendMessage();

      currentStep++;
      revealed = false;
      awaitingUnlock = true;
      inZone = false;
      zoneStartTime = 0;

      if (currentStep >= SEQUENCE_LEN) {
        isSolved = true;
        setStripColor(0, 150, 255);
        showStabilized();
        oocsi.newMessage(CHANNEL);
        oocsi.addInt("temp", -10);
        oocsi.sendMessage();
      } else {
        showStepComplete();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  strip.begin();
  strip.setBrightness(100);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(0x3C, true);
  showWaiting();

  OOCSIName = "Equilibrium_Boiler_" + String(esp_random() % 10000);
  oocsi.connect(OOCSIName.c_str(), hostserver, ssid, password, processOOCSI);
  oocsi.subscribe(CHANNEL);
}

void loop() {
  oocsi.check();
  if (!isSolved) updateProgressBar();
  delay(10);
}
