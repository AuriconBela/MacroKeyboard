#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

// Forward deklaráció
class StateMachine;

// Absztrakt State osztály
class State {
public:
  virtual ~State() {}
  
  // Állapot belépési logika
  virtual void enter(StateMachine* context) {}
  
  // Állapot kilépési logika
  virtual void exit(StateMachine* context) {}
  
  // Encoder gomb kezelése
  virtual void handleEncoderButton(StateMachine* context) {}
  
  // Billentyű lenyomás kezelése
  virtual void handleKeyPress(StateMachine* context, int keyIndex) {}
  
  // Volume kontroll kezelése
  virtual void handleVolumeControl(StateMachine* context, int direction) {}
  
  // Serial üzenet feldolgozása
  virtual void processSerialMessage(StateMachine* context, const String& message) {}
  
  // Timeout kezelése
  virtual void handleTimeout(StateMachine* context) {}
  
  // LCD frissítése
  virtual void updateLCD(StateMachine* context) {}
  
  // Állapot neve (debug céljából)
  virtual String getName() const = 0;
};

// Konkrét állapot osztályok

// Inicializáló állapot
class InitState : public State {
public:
  void enter(StateMachine* context) override;
  void processSerialMessage(StateMachine* context, const String& message) override;
  void updateLCD(StateMachine* context) override;
  String getName() const override { return "INIT"; }
};

// Normál állapot
class NormalState : public State {
private:
  unsigned long lastEncoderPress;
  bool waitingForSecondClick;
  static const unsigned long doubleClickWindow = 300;
  
public:
  NormalState() : lastEncoderPress(0), waitingForSecondClick(false) {}
  
  void enter(StateMachine* context) override;
  void handleEncoderButton(StateMachine* context) override;
  void handleKeyPress(StateMachine* context, int keyIndex) override;
  void handleVolumeControl(StateMachine* context, int direction) override;
  void handleTimeout(StateMachine* context) override;
  void updateLCD(StateMachine* context) override;
  String getName() const override { return "NORMAL"; }
};

// Háttérvilágítás módosító állapot
class BacklightState : public State {
private:
  unsigned long lastEncoderPress;
  bool waitingForSecondClick;
  static const unsigned long doubleClickWindow = 300;
  
public:
  BacklightState() : lastEncoderPress(0), waitingForSecondClick(false) {}
  
  void enter(StateMachine* context) override;
  void handleEncoderButton(StateMachine* context) override;
  void handleTimeout(StateMachine* context) override;
  void updateLCD(StateMachine* context) override;
  String getName() const override { return "BACKLIGHT"; }
};

// Parancs állapot
class CommandState : public State {
private:
  unsigned long commandSentTime;
  State* previousState;
  static const unsigned long commandTimeout = 5000;
  
public:
  CommandState() : commandSentTime(0), previousState(nullptr) {}
  
  void enter(StateMachine* context) override;
  void processSerialMessage(StateMachine* context, const String& message) override;
  void handleTimeout(StateMachine* context) override;
  void updateLCD(StateMachine* context) override;
  void setPreviousState(State* state) { previousState = state; }
  State* getPreviousState() const { return previousState; }
  String getName() const override { return "COMMAND"; }
};

// Globális állapot példányok
extern InitState initState;
extern NormalState normalState;
extern BacklightState backlightState;
extern CommandState commandState;

#endif // STATE_H
