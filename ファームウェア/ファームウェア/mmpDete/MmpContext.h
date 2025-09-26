// MmpContext.h
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <PCA9685.h>
#include <DFRobotDFPlayerMini.h>

struct MmpContext {
  // Current output stream pointer (points to one of the registered sources)
  Stream** serial;
  Adafruit_NeoPixel* pixels;

  // PCA9685
  PCA9685* pwm;
  bool*    pwmConnected;
  int      pwmCount;

  // DFPlayer (Serial2 only)
  DFRobotDFPlayerMini* df;
  bool*                dfConnected;

  // Analog (HC4067) shared hardware
  const int* addressBusGPIOs; // 4 pins
  const int* analogGPIOs;     // up to 4 pins

  // Client-scoped analog snapshots (2 clients: 0=USB, 1=UART0)
  int* anaValuesClient0; // [16*4]
  int* anaValuesClient1; // [16*4]
  int* anaSwitchCntClient0;
  int* anaSwitchCntClient1;
  int* anaPlayerCntClient0;
  int* anaPlayerCntClient1;

  // Servo rating
  int* servoRS;
  int* servoRE;
  int* servoPS;
  int* servoPE;

  // Info
  const char* versionStr;

  // Current client index (set by router): 0=USB, 1=UART0
  int* currentClient;
};

inline Stream& MMP_SERIAL(MmpContext& ctx){ return **ctx.serial; }
