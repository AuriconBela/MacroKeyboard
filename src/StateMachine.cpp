#include "StateMachine.h"
#include "State.h"

// Globális állapotgép példány
StateMachine stateMachine;

// Konstruktor
StateMachine::StateMachine() : 
  currentState(nullptr),
  initComplete(false),
  waitingForCommandResponse(false),
  currentVolume(50),
  isMuted(false)
{
  initKeyNames();
}

// Billentyű nevei inicializálása
void StateMachine::initKeyNames() {
  for (int i = 0; i < 12; i++) {
    keyNames[i] = "";
    keyAssigned[i] = false;
  }
}

// Billentyű név lekérdezése
String StateMachine::getKeyName(int index) const {
  if (index >= 0 && index < 12) {
    return keyNames[index];
  }
  return "";
}

// Billentyű hozzárendelés ellenőrzése
bool StateMachine::isKeyAssigned(int index) const {
  if (index >= 0 && index < 12) {
    return keyAssigned[index];
  }
  return false;
}

// Serial üzenet küldése
void StateMachine::sendSerialMessage(const String& message) {
  Serial.println(message);
}

// Állapotváltás kezelése
void StateMachine::changeState(State* newState) {
  if (currentState) {
    currentState->exit(this);
  }
  
  currentState = newState;
  
  if (currentState) {
    currentState->enter(this);
  }
}

// Encoder gomb kezelése (delegálás az aktuális állapotnak)
void StateMachine::handleEncoderButton() {
  if (currentState) {
    currentState->handleEncoderButton(this);
  }
}

// Volume kezelés (delegálás az aktuális állapotnak)
void StateMachine::handleVolumeControl(int direction) {
  if (currentState) {
    currentState->handleVolumeControl(this, direction);
  }
}

// Billentyű lenyomás kezelése (delegálás az aktuális állapotnak)
void StateMachine::handleKeyPress(int keyIndex) {
  if (currentState) {
    currentState->handleKeyPress(this, keyIndex);
  }
}

// Konfiguráció parse-olása
void StateMachine::parseKeyConfig(const String& config) {
  // Formátum: 0,ButtonName|1,Button2|2,Button3|...
  int startPos = 0;
  int commaPos = config.indexOf(',');
  
  while (commaPos != -1) {
    int keyIndex = config.substring(startPos, commaPos).toInt();
    
    int pipePos = config.indexOf('|', commaPos);
    if (pipePos == -1) pipePos = config.length();
    
    String keyName = config.substring(commaPos + 1, pipePos);
    
    if (keyIndex >= 0 && keyIndex < 12) {
      keyNames[keyIndex] = keyName;
      keyAssigned[keyIndex] = true;
    }
    
    startPos = pipePos + 1;
    commaPos = config.indexOf(',', startPos);
  }
}

// Serial üzenetek feldolgozása (delegálás az aktuális állapotnak)
void StateMachine::processSerialInput() {
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    
    if (currentState) {
      currentState->processSerialMessage(this, message);
    }
  }
}

// Timeout kezelése (delegálás az aktuális állapotnak)
void StateMachine::handleCommandTimeout() {
  if (currentState) {
    currentState->handleTimeout(this);
  }
}

// Dupla kattintás timeout kezelése (delegálás az aktuális állapotnak)
void StateMachine::handleDoubleClickTimeout() {
  if (currentState) {
    currentState->handleTimeout(this);
  }
}

// LCD frissítése (delegálás az aktuális állapotnak)
void StateMachine::updateLCD() {
  if (currentState) {
    currentState->updateLCD(this);
  }
}

// Inicializálás
void StateMachine::initialize() {
  changeState(&initState);
}
