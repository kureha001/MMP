// ModuleBase.h
#pragma once
#include "MmpContext.h"
#include "LedScope.h"

class ModuleBase {
protected:
  MmpContext& ctx;
  LedColor led;
public:
  ModuleBase(MmpContext& c, LedColor col): ctx(c), led(col) {}
  virtual ~ModuleBase() {}
  virtual bool owns(const char* cmd) const = 0;
  virtual void handle(char dat[][10], int dat_cnt) = 0;
};
