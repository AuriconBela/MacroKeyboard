#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "StateMachine.h"
#include "State.h"

// OLED Display konfigurációs konstansok
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1    // Arduino Micro-n nincs reset pin
#define SCREEN_ADDRESS 0x3C // vagy 0x3D, attól függ az OLED címzése

// OLED display objektum
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

// Hue érték lekérdezésére szolgáló függvény
int getCurrentHue() {
  return hue;
}

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
      #ifndef USE_MINIMAL_DISPLAY
      Serial.print("Encoder volume: ");
      Serial.println(encoderDirection > 0 ? "UP" : "DOWN");
      #endif
      
    } else if (currentState == &backlightState || currentState == &initState) {
      // Hue változtatás háttérvilágítás módban
      hue += encoderDirection * 5;
      if (hue >= 360) hue -= 360;
      if (hue < 0) hue += 360;
      #ifndef USE_MINIMAL_DISPLAY
      Serial.print("Encoder hue: ");
      Serial.print(hue);
      Serial.print(" (");
      Serial.print(encoderDirection > 0 ? "CW" : "CCW");
      Serial.println(")");
      #endif
    }
  }
}

void setup() {
  // Serial inicializálás késleltetéssel
  Serial.begin(9600);
  while (!Serial) ; // Wait for Serial to be ready
  Serial.println("MacroKeyboard Starting...");
  Serial.println("Serial communication initialized!");

  Wire.begin();
  // OLED display inicializálása
  Serial.println("Initializing OLED display...");
  
  // I2C inicializálás
  //Wire.begin();
  
  // OLED inicializálás próbálkozás többféle címmel
  bool displayInitialized = false;
  
  // Próbáljuk meg 0x3C címmel
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    displayInitialized = true;
    //#ifndef USE_MINIMAL_DISPLAY
    Serial.println("OLED initialized at 0x3C");
    //#endif
  }
  // Ha nem sikerült, próbáljuk meg 0x3D címmel
  else if(display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    displayInitialized = true;
    //#ifndef USE_MINIMAL_DISPLAY
    Serial.println("OLED initialized at 0x3D");
    //#endif
  }
  
  if (!displayInitialized) {
    //#ifndef USE_MINIMAL_DISPLAY
    Serial.println(F("SSD1306 allocation failed"));
    //#endif
    // LED hibajelzés
    digitalWrite(redPin, HIGH);
    for(;;); // Végtelen ciklus hiba esetén
  }
  
  // Display alapbeállítás és teszt
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);
  // display.println(F("MacroKeyboard"));
  // display.println(F("v2.0 Starting..."));
  // display.setCursor(0, 20);
  // display.println(F("OLED Test OK"));
  // display.display();
  // delay(2000); // 2 másodperc várakozás
  
  // Encoder pinek
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  
  // Pin teszt - kezdeti állapotok kiolvasása
  #ifndef USE_MINIMAL_DISPLAY
  Serial.print("CLK pin initial state: ");
  Serial.println(digitalRead(clkPin));
  Serial.print("DT pin initial state: ");
  Serial.println(digitalRead(dtPin));
  Serial.print("SW pin initial state: ");
  Serial.println(digitalRead(swPin));
  #endif
  
  // RGB LED pinek (hibajelzéshez is)
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT);
  pinMode(bluePin2, OUTPUT);
  
  // LED teszt - zöld jelzi a sikeres inicializálást
  digitalWrite(greenPin, HIGH);
  delay(500);
  digitalWrite(greenPin, LOW);  // Billentyűzet pinek
  for (int i = 0; i < 12; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }
  
  // Encoder interrupt beállítása mindkét pinre
  attachInterrupt(digitalPinToInterrupt(clkPin), onEncoderChange, CHANGE);
  
  // Kezdeti encoder állapot beállítása
  lastClkState = digitalRead(clkPin);
  
  #ifndef USE_MINIMAL_DISPLAY
  Serial.println("Interrupts attached, starting state machine...");
  #endif
  
  // Állapotgép inicializálása
  stateMachine.initialize();
  
  #ifndef USE_MINIMAL_DISPLAY
  Serial.println("Setup complete!");
  #endif
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
    #ifndef USE_MINIMAL_DISPLAY
    Serial.println("Encoder button pressed!");
    #endif
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
  //processEncoderRotation();
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


