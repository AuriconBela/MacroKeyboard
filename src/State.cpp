#include "State.h"
#include "StateMachine.h"

// PROGMEM string konstansok - RAM helyett Flash memóriában tárolva
const char INIT_STR[] PROGMEM = "MacroBoard";
const char LOADING_STR[] PROGMEM = "Loading";
const char MACRO_STR[] PROGMEM = "MacroKeyboard";
const char VOL_STR[] PROGMEM = "Vol:";
const char MUTED_STR[] PROGMEM = "[MUTED]";
const char RGB_STR[] PROGMEM = "2x=RGB";
const char BACKLIGHT_STR[] PROGMEM = "RGB Backlight";
const char HUE_STR[] PROGMEM = "HUE";
const char ROTATE_STR[] PROGMEM = "Rotate encoder";
const char EXIT_STR[] PROGMEM = "2x click = exit";
const char EXEC_STR[] PROGMEM = "EXECUTING";
const char TIME_STR[] PROGMEM = "Time: ";
const char WAIT_STR[] PROGMEM = "Please wait";

// Globális állapot példányok
InitState initState;
NormalState normalState;
BacklightState backlightState;
CommandState commandState;

// ===== InitState implementáció =====

void InitState::enter(StateMachine* context) {
  context->initKeyNames();
  context->sendSerialMessage("INIT_REQUEST");
}

void InitState::processSerialMessage(StateMachine* context, const String& message) {
  if (message.startsWith("KEY_CONFIG:")) {
    String config = message.substring(11);
    context->parseKeyConfig(config);
    context->setInitComplete(true);
    context->changeState(&normalState);
  }
}

void InitState::updateLCD(StateMachine* context) {
  #ifdef USE_MINIMAL_DISPLAY
  // Minimális inicializáló megjelenítés
  static unsigned long lastUpdate = 0;
  static int dotCount = 0;
  
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.print(F("MacroBoard"));
    
    // Egyszerű loading pontok
    display.setTextSize(1);
    display.setCursor(30, 45);
    display.print(F("Loading"));
    for (int i = 0; i < (dotCount % 4); i++) {
      display.print(F("."));
    }
    
    display.display();
    dotCount++;
  }
  #else
  // Teljes animáció (ha van elég hely)
  static unsigned long lastUpdate = 0;
  static int animFrame = 0;
  static int dotCount = 0;
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastUpdate > 200) {
    lastUpdate = currentTime;
    
    display.clearDisplay();
    
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 5);
    display.print((__FlashStringHelper*)INIT_STR);
    
    const char spinner[] = {'|', '/', '-', '\\'};
    int spinnerChar = animFrame % 4;
    display.setCursor(64, 25);
    display.setTextSize(3);
    display.print(spinner[spinnerChar]);
    
    String loadingText = (__FlashStringHelper*)LOADING_STR;
    for (int i = 0; i < (dotCount % 4); i++) {
      loadingText += ".";
    }
    display.setCursor(30, 45);
    display.setTextSize(1);
    display.print(loadingText);
    
    int progressWidth = (animFrame * 3) % 80;
    display.drawRect(24, 55, 80, 6, SSD1306_WHITE);
    display.fillRect(25, 56, progressWidth, 4, SSD1306_WHITE);
    
    animFrame++;
    if (animFrame % 3 == 0) {
      dotCount++;
    }
    
    display.display();
  }
  #endif
}

// ===== NormalState implementáció =====

void NormalState::enter(StateMachine* context) {
  waitingForSecondClick = false;
  lastEncoderPress = 0;
}

void NormalState::handleEncoderButton(StateMachine* context) {
  unsigned long currentTime = millis();
  
  if (waitingForSecondClick) {
    if (currentTime - lastEncoderPress <= doubleClickWindow) {
      // Dupla kattintás detektálva - váltás háttérvilágítás módba
      context->changeState(&backlightState);
      waitingForSecondClick = false;
    }
  } else {
    if (currentTime - lastEncoderPress > doubleClickWindow) {
      // Első kattintás - mute/unmute
      bool isMuted = context->getIsMuted();
      context->setIsMuted(!isMuted);
      String muteCmd = (!isMuted) ? "MUTE:ON" : "MUTE:OFF";
      context->sendSerialMessage(muteCmd);
      
      waitingForSecondClick = true;
      lastEncoderPress = currentTime;
    }
  }
}

void NormalState::handleKeyPress(StateMachine* context, int keyIndex) {
  if (context->isKeyAssigned(keyIndex)) {
    String command = "KEY:" + String(keyIndex);
    context->sendSerialMessage(command);
    commandState.setPreviousState(this);
    context->changeState(&commandState);
  }
}

void NormalState::handleVolumeControl(StateMachine* context, int direction) {
  int currentVolume = context->getCurrentVolume();
  currentVolume += direction * 5;
  currentVolume = constrain(currentVolume, 0, 100);
  context->setCurrentVolume(currentVolume);
  
  String volumeCmd = "VOL:" + String(currentVolume);
  context->sendSerialMessage(volumeCmd);
}

void NormalState::handleTimeout(StateMachine* context) {
  // Dupla kattintás timeout kezelése
  if (waitingForSecondClick && (millis() - lastEncoderPress > doubleClickWindow)) {
    waitingForSecondClick = false;
  }
}

void NormalState::updateLCD(StateMachine* context) {
  // Egyszerűsített normál állapot megjelenítés
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Fejléc
  display.setCursor(25, 2);
  display.print(F("MacroKeyboard"));
  
  // Volume és Mute (egyszerűsítve)
  int volume = context->getCurrentVolume();
  bool muted = context->getIsMuted();
  
  display.setCursor(2, 55);
  display.print(F("Vol:"));
  display.print(volume);
  if (muted) {
    display.setCursor(50, 55);
    display.print(F("[MUTE]"));
  }
  
  // Egyszerűsített 2x6 gomb layout (kisebb)
  const int buttonWidth = 20;
  const int buttonHeight = 10;
  const int startX = 4;
  const int startY = 15;
  const int spacingX = 21;
  const int spacingY = 12;
  
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < 6; col++) {
      int buttonIndex = row * 6 + col;
      int x = startX + col * spacingX;
      int y = startY + row * spacingY;
      
      if (context->isKeyAssigned(buttonIndex)) {
        // Aktív gomb - teli keret
        display.fillRect(x, y, buttonWidth, buttonHeight, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(x + 2, y + 2);
        display.print(buttonIndex);
        display.setTextColor(SSD1306_WHITE);
      } else {
        // Inaktív gomb - üres keret
        display.drawRect(x, y, buttonWidth, buttonHeight, SSD1306_WHITE);
        display.setCursor(x + 8, y + 2);
        display.print(F("-"));
      }
    }
  }
  
  // Encoder hint
  display.setCursor(85, 55);
  display.print(F("2x=RGB"));
  
  display.display();
}

// ===== BacklightState implementáció =====

void BacklightState::enter(StateMachine* context) {
  waitingForSecondClick = false;
  lastEncoderPress = 0;
}

void BacklightState::handleEncoderButton(StateMachine* context) {
  unsigned long currentTime = millis();
  
  if (waitingForSecondClick) {
    if (currentTime - lastEncoderPress <= doubleClickWindow) {
      // Dupla kattintás detektálva - vissza normál módba
      context->changeState(&normalState);
      waitingForSecondClick = false;
    }
  } else {
    waitingForSecondClick = true;
    lastEncoderPress = currentTime;
  }
}

void BacklightState::handleTimeout(StateMachine* context) {
  // Dupla kattintás timeout kezelése
  if (waitingForSecondClick && (millis() - lastEncoderPress > doubleClickWindow)) {
    waitingForSecondClick = false;
  }
}

void BacklightState::updateLCD(StateMachine* context) {
  // Egyszerűsített háttérvilágítás mód
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  int displayHue = getCurrentHue();
  
  // Fejléc
  display.setTextSize(1);
  display.setCursor(25, 2);
  display.print(F("RGB Backlight"));
  
  // Nagy HUE szám középen
  display.setTextSize(3);
  display.setCursor(30, 25);
  display.print(displayHue);
  
  // Egyszerű RGB értékek
  int r = (displayHue < 120) ? 255 : 0;
  int g = (displayHue >= 60 && displayHue < 240) ? 255 : 0;
  int b = (displayHue >= 180) ? 255 : 0;
  
  display.setTextSize(1);
  display.setCursor(10, 55);
  display.print(F("R:"));
  display.print(r);
  display.setCursor(50, 55);
  display.print(F("G:"));
  display.print(g);
  display.setCursor(90, 55);
  display.print(F("B:"));
  display.print(b);
  
  // Használati utasítás
  display.setCursor(15, 15);
  display.print(F("Rotate encoder"));
  
  display.display();
}

// ===== CommandState implementáció =====

void CommandState::enter(StateMachine* context) {
  commandSentTime = millis();
  context->setWaitingForCommandResponse(true);
}

void CommandState::processSerialMessage(StateMachine* context, const String& message) {
  if (message == "COMMAND_COMPLETE") {
    context->setWaitingForCommandResponse(false);
    if (previousState) {
      context->changeState(previousState);
    } else {
      context->changeState(&normalState);
    }
  }
}

void CommandState::handleTimeout(StateMachine* context) {
  if (millis() - commandSentTime > commandTimeout) {
    // Timeout - vissza az előző állapotba
    context->setWaitingForCommandResponse(false);
    if (previousState) {
      context->changeState(previousState);
    } else {
      context->changeState(&normalState);
    }
  }
}

void CommandState::updateLCD(StateMachine* context) {
  // Egyszerűsített parancs futás állapot
  static unsigned long lastUpdate = 0;
  static int animFrame = 0;
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - commandSentTime;
  
  if (currentTime - lastUpdate > 200) {
    lastUpdate = currentTime;
    
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Fejléc
    display.setTextSize(2);
    display.setCursor(15, 10);
    display.print(F("EXECUTING"));
    
    // Egyszerű spinner
    const char spinner[] = {'|', '/', '-', '\\'};
    int spinnerIndex = animFrame % 4;
    display.setTextSize(3);
    display.setCursor(55, 30);
    display.print(spinner[spinnerIndex]);
    
    // Időzítő
    display.setTextSize(1);
    display.setCursor(45, 55);
    display.print(F("Time: "));
    display.print(elapsed / 1000);
    display.print(F("s"));
    
    animFrame++;
    display.display();
  }
}
