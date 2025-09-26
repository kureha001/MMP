// ModuleAnalog.h
#pragma once
#include "ModuleBase.h"
#include "Util.h"

class ModuleAnalog : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  bool owns(const char* cmd) const override {
    return strcmp(cmd,"ANS")==0 || strcmp(cmd,"ANU")==0 || strcmp(cmd,"ANR")==0;
  }

  // Helpers to pick client-scoped slots
  int* values() {
    return (*ctx.currentClient==0) ? ctx.anaValuesClient0 : ctx.anaValuesClient1;
  }
  int& swCnt() {
    return (*ctx.currentClient==0) ? *ctx.anaSwitchCntClient0 : *ctx.anaSwitchCntClient1;
  }
  int& plCnt() {
    return (*ctx.currentClient==0) ? *ctx.anaPlayerCntClient0 : *ctx.anaPlayerCntClient1;
  }

  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    Stream& sp = MMP_SERIAL(ctx);

    if (strcmp(dat[0],"ANS")==0){
      if (dat_cnt < 3) { printCmdError(sp, "ANS"); return; }
      long newPl, newSw;
      if (!parseHexStrict(dat[1], newPl, 1, 16) ||
          !parseHexStrict(dat[2], newSw, 1, 4)){
        printCmdError(sp, "ANS"); return;
      }
      plCnt() = (int)newPl;
      swCnt() = (int)newSw;
      for (int i=0;i<16;i++) for (int j=0;j<4;j++) values()[i*4+j]=0;
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"ANU")==0){
      for (int ch=0; ch<plCnt(); ch++){
        for (int i=0;i<4;i++){
          pinMode(ctx.addressBusGPIOs[i], OUTPUT);
          digitalWrite(ctx.addressBusGPIOs[i], (ch>>i)&1);
        }
        delayMicroseconds(10);
        for (int sw=0; sw<swCnt(); sw++){
          int pin = ctx.analogGPIOs[sw];
          values()[ch*4+sw] = analogRead(pin);
        }
      }
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"ANR")==0){
      if (dat_cnt < 3) { printCmdError(sp, "ANR"); return; }
      long pl, sw;
      if (!parseHexStrict(dat[1], pl, 0, 15) ||
          !parseHexStrict(dat[2], sw, 0, 3)){
        printCmdError(sp, "ANR"); return;
      }
      char rets[6]; snprintf(rets, sizeof(rets), "%04X!", values()[((int)pl)*4 + (int)sw] & 0xFFFF);
      sp.print(rets);
    }
  }
};
