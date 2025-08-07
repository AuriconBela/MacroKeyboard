#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>

// Forward deklarációk
class State;

// Állapotgép osztály
class StateMachine {
private:
  State* currentState;
  
  // Serial kommunikáció változók
  bool initComplete;
  bool waitingForCommandResponse;
  
  // Billentyűzet változók
  String keyNames[12];
  bool keyAssigned[12];
  
  // Volume kontroll
  int currentVolume;
  bool isMuted;

public:
  // Konstruktor
  StateMachine();
  
  // Állapot kezelés
  void changeState(State* newState);
  State* getCurrentState() const { return currentState; }
  
  // Getter/Setter függvények
  bool isInitComplete() const { return initComplete; }
  void setInitComplete(bool value) { initComplete = value; }
  
  bool isWaitingForCommandResponse() const { return waitingForCommandResponse; }
  void setWaitingForCommandResponse(bool value) { waitingForCommandResponse = value; }
  
  String getKeyName(int index) const;
  bool isKeyAssigned(int index) const;
  
  int getCurrentVolume() const { return currentVolume; }
  void setCurrentVolume(int volume) { currentVolume = volume; }
  
  bool getIsMuted() const { return isMuted; }
  void setIsMuted(bool muted) { isMuted = muted; }
  
  // Fő interface függvények (delegálnak az aktuális állapotnak)
  void handleEncoderButton();
  void handleVolumeControl(int direction);
  void handleKeyPress(int keyIndex);
  void processSerialInput();
  void handleCommandTimeout();
  void handleDoubleClickTimeout();
  void updateLCD();
  
  // Segédfüggvények (publikusak, hogy az állapotok használhassák)
  void sendSerialMessage(const String& message);
  void initKeyNames();
  void parseKeyConfig(const String& config);
  
  // Inicializálás
  void initialize();
};

// Globális állapotgép példány deklaráció
extern StateMachine stateMachine;

#endif // STATEMACHINE_H
