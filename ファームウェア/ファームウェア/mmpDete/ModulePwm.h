// ModulePwm.h
#pragma once
#include "ModuleBase.h"
#include "Util.h"

class ModulePwm : public ModuleBase {
public:
  using ModuleBase::ModuleBase;
  bool owns(const char* cmd) const override {
    return strcmp(cmd,"PWM")==0 || strcmp(cmd,"PWA")==0 || strcmp(cmd,"PWI")==0 || strcmp(cmd,"PWX")==0;
  }

  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    Stream& sp = MMP_SERIAL(ctx);

    if (strcmp(dat[0],"PWM")==0){
      if (dat_cnt < 3) { printCmdError(sp, "PWM"); return; }
      long ch, val;
      if (!parseHexStrict(dat[1], ch, 0, 0xFFFF) ||
          !parseHexStrict(dat[2], val, 0, 4095)){ printCmdError(sp, "PWM"); return; }
      int pwmID = (int)ch / 16; int pwmCh = (int)ch % 16;
      if (pwmID >= ctx.pwmCount || !ctx.pwmConnected[pwmID]){ printCmdError(sp, "PWM"); return; }
      ctx.pwm[pwmID].setPWM(pwmCh, 0, (int)val);
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"PWA")==0){
      if (dat_cnt < 3) { printCmdError(sp, "PWA"); return; }
      long ch, angle;
      if (!parseHexStrict(dat[1], ch, 0, 0xFFFF) ||
          !parseHexStrict(dat[2], angle, 0, 180)){ printCmdError(sp, "PWA"); return; }
      int pwmID = (int)ch / 16; int pwmCh = (int)ch % 16;
      if (pwmID >= ctx.pwmCount || !ctx.pwmConnected[pwmID]){ printCmdError(sp, "PWA"); return; }
      int rdiff = *ctx.servoRE - *ctx.servoRS;
      int pdiff = *ctx.servoPE - *ctx.servoPS;
      if (rdiff<=0 || pdiff<=0){ printCmdError(sp, "PWA"); return; }
      int val = *ctx.servoPS + int((float)((int)angle - *ctx.servoRS) / (float)rdiff * (float)pdiff);
      ctx.pwm[pwmID].setPWM(pwmCh, 0, val);
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"PWI")==0){
      if (dat_cnt < 5) { printCmdError(sp, "PWI"); return; }
      long rs,re,ps,pe;
      if (!parseHexStrict(dat[1], rs, 0, 360) ||
          !parseHexStrict(dat[2], re, 0, 360) ||
          !parseHexStrict(dat[3], ps, 0, 4095) ||
          !parseHexStrict(dat[4], pe, 0, 4095)){ printCmdError(sp, "PWI"); return; }
      if (!(rs<re && ps<pe)){ printCmdError(sp, "PWI"); return; }
      *ctx.servoRS=rs; *ctx.servoRE=re; *ctx.servoPS=ps; *ctx.servoPE=pe;
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"PWX")==0){
      if (dat_cnt < 2) { printCmdError(sp, "PWX"); return; }
      long id;
      if (!parseHexStrict(dat[1], id, 0, ctx.pwmCount-1)){ printCmdError(sp, "PWX"); return; }
      if (!ctx.pwmConnected[id]){ printCmdError(sp, "PWX"); return; }
      printHex4Bang(sp, 1);
    }
  }
};
