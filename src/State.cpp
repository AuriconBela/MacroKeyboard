#include "State.h"
#include "StateMachine.h"

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
  // TODO: LCD inicializáló grafika megjelenítése
  // Például: "Initializing..." vagy animáció
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
  // TODO: Gomb nevek megjelenítése
  // Például grid layout a 12 gombhoz
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
  // TODO: Színbeállítás kijelzése
  // Például: színkerék vagy RGB értékek
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
  // TODO: "Parancs fut..." kijelzése
  // Például: progress bar vagy spinner animáció
}
