// ModuleDigital.h
#pragma once
#include "ModuleBase.h"
#include "Util.h"

class ModuleDigital : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  bool owns(const char* cmd) const override {
    return strcmp(cmd,"POW")==0; // POR is out of scope per spec
  }

  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    Stream& sp = MMP_SERIAL(ctx);

    if (strcmp(dat[0],"POW")==0){
      if (dat_cnt < 3) { printCmdError(sp, "POW"); return; }
      long port, val;
      if (!parseHexStrict(dat[1], port, 0, 0xFFFF) ||
          !parseHexStrict(dat[2], val,  0, 1)){ printCmdError(sp, "POW"); return; }
      pinMode((int)port, OUTPUT);
      digitalWrite((int)port, (int)val);
      sp.print("!!!!!");
    }
  }
};
