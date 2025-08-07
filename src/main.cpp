#include <Arduino.h>
#include "StateMachine.h"
#include "State.h"

// RGB LED pinjei
const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;
const int redPin2 = 5;
const int greenPin2 = 6;
const int bluePin2 = 7;

// Encoder pinjei
const int clkPin = 2;
const int dtPin = 3;
const int swPin = 4;  // Encoder gomb

// Billentyűzet pinjei (12 billentyű)
const int keyPins[12] = {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 8, 12};

// Hardware állapot változók
volatile int lastClkState = LOW;
volatile int hue = 0;

// Billentyűzet kezeléshez
bool keyStates[12] = {false};
bool lastKeyStates[12] = {false};

// Encoder gomb kezeléshez
bool lastEncoderButtonState = false;

// Hue (0–360) → RGB (0–255)
void hueToRGB(int hue, int& r, int& g, int& b) {
  float h = hue / 60.0;
  float x = 1 - abs(fmod(h, 2) - 1);
  float r1, g1, b1;

  if (h < 1) { r1 = 1; g1 = x; b1 = 0; }
  else if (h < 2) { r1 = x; g1 = 1; b1 = 0; }
  else if (h < 3) { r1 = 0; g1 = 1; b1 = x; }
  else if (h < 4) { r1 = 0; g1 = x; b1 = 1; }
  else if (h < 5) { r1 = x; g1 = 0; b1 = 1; }
  else { r1 = 1; g1 = 0; b1 = x; }

  r = int(r1 * 255);
  g = int(g1 * 255);
  b = int(b1 * 255);
}

// RGB LED frissítése
void updateRGBLeds() {
  int r, g, b;
  hueToRGB(hue, r, g, b);
  
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
  analogWrite(redPin2, r);
  analogWrite(greenPin2, g);
  analogWrite(bluePin2, b);
}

// Encoder változás kezelése (csak háttérvilágítás módban)
void onEncoderChange() {
  if (stateMachine.getCurrentState() != &backlightState) return;
  
  int clkState = digitalRead(clkPin);
  int dtState = digitalRead(dtPin);

  if (clkState != lastClkState) {
    if (dtState != clkState) {
      hue += 5;
    } else {
      hue -= 5;
    }

    if (hue >= 360) hue -= 360;
    if (hue < 0) hue += 360;
  }

  lastClkState = clkState;
}

// LCD kijelző frissítése (placeholder - most delegálunk az állapotgépnek)
void updateLCD() {
  stateMachine.updateLCD();
}

// Billentyűk olvasása és kezelése
void handleKeys() {
  for (int i = 0; i < 12; i++) {
    keyStates[i] = !digitalRead(keyPins[i]); // Pull-up miatt invertált
    
    // Billentyű lenyomás detektálása
    if (keyStates[i] && !lastKeyStates[i]) {
      stateMachine.handleKeyPress(i);
    }
    
    lastKeyStates[i] = keyStates[i];
  }
}

// Encoder forgatás kezelése normál állapotban (volume)
void handleEncoderRotation() {
  static int lastEncoderState = 0;
  int clkState = digitalRead(clkPin);
  int dtState = digitalRead(dtPin);
  
  if (stateMachine.getCurrentState() == &normalState) {
    if (clkState != lastEncoderState) {
      if (dtState != clkState) {
        stateMachine.handleVolumeControl(1); // Hangerő fel
      } else {
        stateMachine.handleVolumeControl(-1); // Hangerő le
      }
    }
    lastEncoderState = clkState;
  }
}

void setup() {
  Serial.begin(9600);
  
  // Encoder pinek
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  
  // RGB LED pinek
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT); 
  pinMode(bluePin2, OUTPUT);
  
  // Billentyűzet pinek
  for (int i = 0; i < 12; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }
  
  // Encoder interrupt beállítása
  attachInterrupt(digitalPinToInterrupt(clkPin), onEncoderChange, CHANGE);
  
  // Állapotgép inicializálása
  stateMachine.initialize();
}

void loop() {
  // Serial kommunikáció feldolgozása
  stateMachine.processSerialInput();
  
  // Encoder gomb kezelése
  bool currentEncoderButton = !digitalRead(swPin);
  if (currentEncoderButton && !lastEncoderButtonState) {
    stateMachine.handleEncoderButton();
  }
  lastEncoderButtonState = currentEncoderButton;
  
  // Állapot függő logika
  State* currentState = stateMachine.getCurrentState();
  if (currentState == &initState) {
    // Várakozás a PC válaszára
  } else if (currentState == &normalState) {
    handleKeys();
    handleEncoderRotation();
  } else if (currentState == &backlightState) {
    // RGB LED színek frissítése (interrupt-ban történik a hue változtatás)
  } else if (currentState == &commandState) {
    stateMachine.handleCommandTimeout();
  }
  
  // RGB LED frissítése
  updateRGBLeds();
  
  // LCD frissítése
  updateLCD();
  
  // Timeout kezelések
  stateMachine.handleDoubleClickTimeout();
  
  delay(10);
}


