// ModuleI2C.h
#pragma once
#include "ModuleBase.h"
#include "Util.h"
#include <Wire.h>

class ModuleI2C : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  bool owns(const char* cmd) const override {
    return strcmp(cmd,"I2W")==0 || strcmp(cmd,"I2R")==0;
  }

  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    Stream& sp = MMP_SERIAL(ctx);

    if (strcmp(dat[0],"I2W")==0){
      if (dat_cnt < 4) { printCmdError(sp, "I2W"); return; }
      long addr, reg, val;
      if (!parseHexStrict(dat[1], addr, 0x00, 0x7F) ||
          !parseHexStrict(dat[2], reg,  0x00, 0xFF) ||
          !parseHexStrict(dat[3], val,  0x00, 0xFF)){ printCmdError(sp, "I2W"); return; }
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.write((int)val);
      Wire.endTransmission();
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"I2R")==0){
      if (dat_cnt < 3) { printCmdError(sp, "I2R"); return; }
      long addr, reg;
      if (!parseHexStrict(dat[1], addr, 0x00, 0x7F) ||
          !parseHexStrict(dat[2], reg,  0x00, 0xFF)){ printCmdError(sp, "I2R"); return; }
      Wire.beginTransmission((int)addr);
      Wire.write((int)reg);
      Wire.endTransmission(false);
      int n = Wire.requestFrom((int)addr, 1);
      if (n != 1) { printCmdError(sp, "I2R"); return; }
      int v = Wire.read();
      printHex4Bang(sp, v);
    }
  }
};
