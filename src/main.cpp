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
volatile bool encoderChanged = false;
volatile int encoderDirection = 0;

// Encoder debounce változók
volatile unsigned long lastEncoderTime = 0;
const unsigned long encoderDebounceTime = 2; // 2ms debounce

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

// Javított encoder interrupt kezelés (minden állapotban működik)
void onEncoderChange() {
  unsigned long currentTime = micros();
  
  // Debounce ellenőrzés
  if (currentTime - lastEncoderTime < encoderDebounceTime * 1000) {
    return;
  }
  lastEncoderTime = currentTime;
  
  int clkState = digitalRead(clkPin);
  int dtState = digitalRead(dtPin);

  // Csak élek detektálása (LOW->HIGH vagy HIGH->LOW)
  if (clkState != lastClkState) {
    // Irány meghatározása
    if (clkState == HIGH) {
      // Csak rising edge-en értékelünk
      if (dtState != clkState) {
        encoderDirection = 1;  // Pozitív irány
      } else {
        encoderDirection = -1; // Negatív irány
      }
      encoderChanged = true;
    }
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

// Encoder forgatás feldolgozása (centralizált)
void processEncoderRotation() {
  if (encoderChanged) {
    encoderChanged = false; // Reset flag
    
    State* currentState = stateMachine.getCurrentState();
    
    if (currentState == &normalState) {
      // Volume kontroll normál állapotban
      stateMachine.handleVolumeControl(encoderDirection);
      Serial.print("Encoder volume: ");
      Serial.println(encoderDirection > 0 ? "UP" : "DOWN");
      
    } else if (currentState == &backlightState || currentState == &initState) {
      // Hue változtatás háttérvilágítás módban
      hue += encoderDirection * 5;
      if (hue >= 360) hue -= 360;
      if (hue < 0) hue += 360;
      Serial.print("Encoder hue: ");
      Serial.print(hue);
      Serial.print(" (");
      Serial.print(encoderDirection > 0 ? "CW" : "CCW");
      Serial.println(")");
    }
  }
}

void setup() {
  Serial.begin(9600);
  
  // Debug üzenet
  Serial.println("MacroKeyboard Starting...");
  Serial.println("Testing encoder pins...");
  
  // Encoder pinek
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  
  // Pin teszt - kezdeti állapotok kiolvasása
  Serial.print("CLK pin initial state: ");
  Serial.println(digitalRead(clkPin));
  Serial.print("DT pin initial state: ");
  Serial.println(digitalRead(dtPin));
  Serial.print("SW pin initial state: ");
  Serial.println(digitalRead(swPin));
  
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
  
  // Encoder interrupt beállítása mindkét pinre
  attachInterrupt(digitalPinToInterrupt(clkPin), onEncoderChange, CHANGE);
  
  // Kezdeti encoder állapot beállítása
  lastClkState = digitalRead(clkPin);
  
  Serial.println("Interrupts attached, starting state machine...");
  
  // Állapotgép inicializálása
  stateMachine.initialize();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Diagnosztikai számláló (encoder teszt)
  static unsigned long lastDiagnostic = 0;
  static unsigned long loopCounter = 0;
  loopCounter++;
  
  // Serial kommunikáció feldolgozása
  stateMachine.processSerialInput();
  
  // Encoder gomb kezelése
  bool currentEncoderButton = !digitalRead(swPin);
  if (currentEncoderButton && !lastEncoderButtonState) {
    stateMachine.handleEncoderButton();
    Serial.println("Encoder button pressed!");
  }
  lastEncoderButtonState = currentEncoderButton;
  
  // Állapot függő logika
  State* currentState = stateMachine.getCurrentState();
  if (currentState == &initState) {
    // Várakozás a PC válaszára
    processEncoderRotation(); // Javított encoder kezelés
  } else if (currentState == &normalState) {
    handleKeys();
    processEncoderRotation(); // Javított encoder kezelés
  } else if (currentState == &backlightState) {
    processEncoderRotation(); // Javított encoder kezelés
  } else if (currentState == &commandState) {
    stateMachine.handleCommandTimeout();
  }
  processEncoderRotation();
  // RGB LED frissítése
  updateRGBLeds();
  
  // LCD frissítése
  updateLCD();
  
  // Timeout kezelések
  stateMachine.handleDoubleClickTimeout();
  
  // Diagnosztikai kimenet (5 másodpercenként)
  if (millis() - lastDiagnostic > 5000) {
    lastDiagnostic = millis();
    Serial.print("Loop count: ");
    Serial.print(loopCounter);
    Serial.print(" | Encoder changes: ");
    Serial.print(encoderChanged ? "ACTIVE" : "IDLE");
    Serial.print(" | CLK: ");
    Serial.print(digitalRead(clkPin));
    Serial.print(" | DT: ");
    Serial.print(digitalRead(dtPin));
    Serial.print(" | Current hue: ");
    Serial.println(hue);
    loopCounter = 0;
  }
  
  delay(10);
}


