#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
//#include <Keyboard.h>
#include "StateMachine.h"
#include "State.h"

// OLED Display konfigurációs konstansok
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1    // Arduino Micro-n nincs reset pin
#define SCREEN_ADDRESS 0x3C // vagy 0x3D, attól függ az OLED címzése

// OLED display objektum
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RGB LED pinjei (PWM képes pinek, I2C pinektől eltérően)
const int redPin = 5;    // PWM pin
const int greenPin = 6;  // PWM pin  
const int bluePin = 10;  // PWM pin
const int redPin2 = 11;  // PWM pin
const int greenPin2 = 9; // PWM pin
const int bluePin2 = 12; // Digitális pin

// Encoder pinjei (I2C-től eltérő pinek)
const int clkPin = 7;   // Encoder CLK (A pin)
const int dtPin = 8;    // Encoder DT (B pin)  
const int swPin = 4;    // Encoder gomb (SW pin)

const int rowPins[3] = {A0, A1, A2};       // Sor pinek (OUTPUT) - aktiváljuk őket LOW-val
const int colPins[4] = {A3, A4, A5, 1};    // Oszlop pinek (INPUT_PULLUP) - TX pin helyett A6

// Array méretek konstansai (dinamikus számítás)
const int NUM_ROWS = sizeof(rowPins) / sizeof(rowPins[0]);
const int NUM_COLS = sizeof(colPins) / sizeof(colPins[0]);
const int NUM_KEYS = NUM_ROWS * NUM_COLS;

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

// Mátrix billentyűzet kezeléshez (dinamikus méret)
bool keyStates[NUM_KEYS] = {false};
bool lastKeyStates[NUM_KEYS] = {false};

// Encoder gomb kezeléshez
bool lastEncoderButtonState = false;

// HSV (0-360, 0-100, 0-100) → RGB (0-255) konverzió
void hsvToRGB(int hue, int saturation, int value, int& r, int& g, int& b) {
  // Normalizálás
  float h = (hue % 360) / 60.0;  // 0-6 tartomány
  float s = saturation / 100.0;  // 0-1 tartomány  
  float v = value / 100.0;       // 0-1 tartomány
  
  int i = (int)h;
  float f = h - i;
  float p = v * (1 - s);
  float q = v * (1 - s * f);
  float t = v * (1 - s * (1 - f));
  
  float r1, g1, b1;
  
  switch(i) {
    case 0: r1 = v; g1 = t; b1 = p; break;
    case 1: r1 = q; g1 = v; b1 = p; break;
    case 2: r1 = p; g1 = v; b1 = t; break;
    case 3: r1 = p; g1 = q; b1 = v; break;
    case 4: r1 = t; g1 = p; b1 = v; break;
    default: r1 = v; g1 = p; b1 = q; break;
  }
  
  r = (int)(r1 * 255);
  g = (int)(g1 * 255);
  b = (int)(b1 * 255);
}

// Kompatibilitási wrapper a régi hueToRGB-hez (telített, fényes színek)
void hueToRGB(int hue, int& r, int& g, int& b) {
  hsvToRGB(hue, 100, 100, r, g, b); // 100% telítettség, 100% világosság
}

// Változó telítettségű színek generálása
void hueToRGBWithSaturation(int hue, int saturation, int& r, int& g, int& b) {
  hsvToRGB(hue, saturation, 100, r, g, b); // 100% világosság, változó telítettség
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

// Mátrix billentyűk olvasása és kezelése (dinamikus méretekkel)
void handleKeys() {
  for (int row = 0; row < NUM_ROWS; row++) {
    // Összes sor HIGH-ra állítása (inaktív)
    for (int i = 0; i < NUM_ROWS; i++) {
      digitalWrite(rowPins[i], HIGH);
    }
    
    // Aktuális sor aktiválása (LOW)
    digitalWrite(rowPins[row], LOW);
    
    // Kis késleltetés a jel stabilizálódásához
    delayMicroseconds(10);
    
    // Oszlopok olvasása
    for (int col = 0; col < NUM_COLS; col++) {
      int keyIndex = row * NUM_COLS + col; // Dinamikus számítás
      bool currentState = !digitalRead(colPins[col]); // Pull-up miatt invertált

      keyStates[keyIndex] = currentState;
      
      // Billentyű lenyomás detektálása (rising edge)
      if (keyStates[keyIndex] && !lastKeyStates[keyIndex]) {
        stateMachine.handleKeyPress(keyIndex);
        #ifndef USE_MINIMAL_DISPLAY
        Serial.print(F("Matrix key pressed: "));
        Serial.print(keyIndex);
        Serial.print(F(" (row: "));
        Serial.print(row);
        Serial.print(F(", col: "));
        Serial.print(col);
        Serial.println(F(")"));
        #endif
      }
      
      lastKeyStates[keyIndex] = keyStates[keyIndex];
    }
    
    // Sor deaktiválása (HIGH)
    digitalWrite(rowPins[row], HIGH);
    delayMicroseconds(500);
  }
  
  // Összes sor deaktiválása (HIGH) a szkennelés után
  for (int i = 0; i < NUM_ROWS; i++) {
    digitalWrite(rowPins[i], HIGH);
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
    } else if (currentState == &backlightState || currentState == &initState) {
      // Hue változtatás háttérvilágítás módban
      hue += encoderDirection * 5;
      if (hue >= 360) hue -= 360;
      if (hue < 0) hue += 360;
    }
  }
}

void setup() {
  #ifndef USE_MINIMAL_DISPLAY
  Serial.begin(9600);
  while (!Serial) ; // Wait for Serial to be ready
  Serial.println(F("MacroKeyboard Starting..."));
  #endif

  Wire.begin();
  //Keyboard.begin();
  
  // // I2C eszközök keresése
  // Serial.println("Scanning I2C devices...");
  // byte error, address;
  // int nDevices = 0;
  // for(address = 1; address < 127; address++) {
  //   Wire.beginTransmission(address);
  //   error = Wire.endTransmission();
  //   if (error == 0) {
  //     Serial.print("I2C device found at address 0x");
  //     if (address < 16) Serial.print("0");
  //     Serial.print(address, HEX);
  //     Serial.println(" !");
  //     nDevices++;
  //   }
  // }
  // if (nDevices == 0) {
  //   Serial.println("No I2C devices found - check wiring!");
  // } else {
  //   Serial.print("Found ");
  //   Serial.print(nDevices);
  //   Serial.println(" I2C devices");
  // }
  
  // OLED display inicializálása
  #ifndef USE_MINIMAL_DISPLAY  
  Serial.println(F("Initializing OLED display..."));
  #endif

  // OLED inicializálás próbálkozás többféle címmel
  bool displayInitialized = false;
  
  // Próbáljuk meg 0x3C címmel
  if(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayInitialized = true;
    #ifndef USE_MINIMAL_DISPLAY
    Serial.println(F("OLED initialized at 0x3C"));
    #endif
  }
  
  if (!displayInitialized) {
    #ifndef USE_MINIMAL_DISPLAY
    Serial.println(F("SSD1306 allocation failed"));
    #endif
    // LED hibajelzés
    digitalWrite(redPin, HIGH);
    for(;;); // Végtelen ciklus hiba esetén
  }
  
  // Encoder pinek
  pinMode(clkPin, INPUT_PULLUP);
  pinMode(dtPin, INPUT_PULLUP);
  pinMode(swPin, INPUT_PULLUP);
  
  // Pin teszt - kezdeti állapotok kiolvasása
  #ifndef USE_MINIMAL_DISPLAY
  Serial.print(F("CLK pin initial state: "));
  Serial.println(digitalRead(clkPin));
  Serial.print(F("DT pin initial state: "));
  Serial.println(digitalRead(dtPin));
  Serial.print(F("SW pin initial state: "));
  Serial.println(digitalRead(swPin));
  #endif
  
  // RGB LED pinek (hibajelzéshez is)
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT);
  pinMode(bluePin2, OUTPUT);
  
  digitalWrite(greenPin, LOW);  
  
  // Mátrix billentyűzet pinek inicializálása (dinamikus méretekkel)
  // Sor pinek (OUTPUT, kezdetben HIGH - inaktív)
  for (int i = 0; i < NUM_ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); // Inaktív állapot
  }
  
  // Oszlop pinek (INPUT_PULLUP)
  for (int i = 0; i < NUM_COLS; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }
  
  #ifndef USE_MINIMAL_DISPLAY
  Serial.print(F("Matrix keyboard pins configured ("));
  Serial.print(NUM_ROWS);
  Serial.print(F("x"));
  Serial.print(NUM_COLS);
  Serial.print(F(" matrix, "));
  Serial.print(NUM_KEYS);
  Serial.println(F(" keys total):"));
  
  Serial.print(F("Row pins (OUTPUT): "));
  for (int i = 0; i < NUM_ROWS; i++) {
    Serial.print(rowPins[i]);
    if (i < NUM_ROWS - 1) Serial.print(F(", "));
  }
  Serial.println(F(")"));
  
  Serial.print(F("Column pins (INPUT_PULLUP): "));
  for (int i = 0; i < NUM_COLS; i++) {
    Serial.print(colPins[i]);
    if (i < NUM_COLS - 1) Serial.print(F(", "));
  }
  Serial.println(F(")"));
  #endif
  
  // Encoder interrupt beállítása mindkét pinre
  attachInterrupt(digitalPinToInterrupt(clkPin), onEncoderChange, CHANGE);
  
  // Kezdeti encoder állapot beállítása
  lastClkState = digitalRead(clkPin);
  
  #ifndef USE_MINIMAL_DISPLAY
  Serial.println(F("Interrupts attached, starting state machine..."));
  #endif
  
  // Állapotgép inicializálása
  stateMachine.initialize();
  
  #ifndef USE_MINIMAL_DISPLAY
  Serial.println(F("Setup complete!"));
  #endif
}

void loop() {  
  // Serial kommunikáció feldolgozása
  stateMachine.processSerialInput();
  
  // Encoder gomb kezelése
  bool currentEncoderButton = !digitalRead(swPin);
  if (currentEncoderButton && !lastEncoderButtonState) {
    stateMachine.handleEncoderButton();
    #ifndef USE_MINIMAL_DISPLAY
    Serial.println(F("Encoder button pressed!"));
    #endif
  }
  lastEncoderButtonState = currentEncoderButton;
  
  // Állapot függő logika
  State* currentState = stateMachine.getCurrentState();
  if (currentState == &initState) {
    #ifdef IMITATE_PC_ANSWER
    // Várakozás a PC válaszára - automatikus válasz szimuláció 3 másodperc után
    static unsigned long initStartTime = 0;
    static bool initTimerStarted = false;
    
    // Timer indítása az első alkalommal
    if (!initTimerStarted) {
      initStartTime = millis();
      initTimerStarted = true;
      Serial.println(F("Init state entered - waiting for PC response or 3 second timeout..."));
    }
    
    // 3 másodperc után automatikus válasz
    if (millis() - initStartTime > 3000) {
      Serial.println(F("Simulating PC response: 'READY'"));
      // Szimuláljuk az állapot válasz feldolgozását közvetlenül
      #endif
      if (currentState) {
        currentState->processSerialMessage(&stateMachine, "READY");
      }
      #ifdef IMITATE_PC_ANSWER
      initTimerStarted = false; // Reset timer for next time
    }
    #endif
  } else if (currentState == &normalState) {
    handleKeys();
    processEncoderRotation(); // Javított encoder kezelés
  } else if (currentState == &backlightState) {
    processEncoderRotation(); // Javított encoder kezelés
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


