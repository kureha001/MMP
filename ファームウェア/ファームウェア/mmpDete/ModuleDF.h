// ModuleDF.h
#pragma once
#include "ModuleBase.h"
#include "Util.h"

class ModuleDF : public ModuleBase {
public:
  using ModuleBase::ModuleBase;
  bool owns(const char* cmd) const override {
    return strcmp(cmd,"DIR")==0 || strcmp(cmd,"DLP")==0 || strcmp(cmd,"DSP")==0 ||
           strcmp(cmd,"DPA")==0 || strcmp(cmd,"DPR")==0 || strcmp(cmd,"VOL")==0 ||
           strcmp(cmd,"DEF")==0 || strcmp(cmd,"DST")==0 || strcmp(cmd,"DPX")==0;
  }

  bool checkDev(const char* cmd, const char* s, int& idx, Stream& sp){
    long dev;
    if (!parseHexStrict(s, dev, 1, 2)) { printCmdError(sp, cmd); return false; }
    idx = (int)dev - 1;
    if (idx<0 || idx>=2 || !ctx.dfConnected[idx]){ printCmdError(sp, cmd); return false; }
    return true;
  }

  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    Stream& sp = MMP_SERIAL(ctx);

    if (strcmp(dat[0],"DIR")==0){
      if (dat_cnt<4){ printCmdError(sp,"DIR"); return; }
      int idx; if (!checkDev("DIR", dat[1], idx, sp)) return;
      long folder, track;
      if (!parseHexStrict(dat[2], folder, 0, 0xFFFF) ||
          !parseHexStrict(dat[3], track,  0, 0xFFFF)){ printCmdError(sp,"DIR"); return; }
      ctx.df[idx].playFolder((int)folder,(int)track);
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"DLP")==0){
      if (dat_cnt<3){ printCmdError(sp,"DLP"); return; }
      int idx; if (!checkDev("DLP", dat[1], idx, sp)) return;
      long loop;
      if (!parseHexStrict(dat[2], loop, 0, 1)){ printCmdError(sp,"DLP"); return; }
      if (loop==1) ctx.df[idx].enableLoop(); else ctx.df[idx].disableLoop();
      sp.print("!!!!!");

    } else if (strcmp(dat[0],"DSP")==0){
      if (dat_cnt<2){ printCmdError(sp,"DSP"); return; }
      int idx; if (!checkDev("DSP", dat[1], idx, sp)) return;
      ctx.df[idx].stop(); sp.print("!!!!!");

    } else if (strcmp(dat[0],"DPA")==0){
      if (dat_cnt<2){ printCmdError(sp,"DPA"); return; }
      int idx; if (!checkDev("DPA", dat[1], idx, sp)) return;
      ctx.df[idx].pause(); sp.print("!!!!!");

    } else if (strcmp(dat[0],"DPR")==0){
      if (dat_cnt<2){ printCmdError(sp,"DPR"); return; }
      int idx; if (!checkDev("DPR", dat[1], idx, sp)) return;
      ctx.df[idx].start(); sp.print("!!!!!");

    } else if (strcmp(dat[0],"VOL")==0){
      if (dat_cnt<3){ printCmdError(sp,"VOL"); return; }
      int idx; if (!checkDev("VOL", dat[1], idx, sp)) return;
      long v; if (!parseHexStrict(dat[2], v, 0, 30)){ printCmdError(sp,"VOL"); return; }
      ctx.df[idx].volume((int)v); sp.print("!!!!!");

    } else if (strcmp(dat[0],"DEF")==0){
      if (dat_cnt<3){ printCmdError(sp,"DEF"); return; }
      int idx; if (!checkDev("DEF", dat[1], idx, sp)) return;
      long mode; if (!parseHexStrict(dat[2], mode, 0, 5)){ printCmdError(sp,"DEF"); return; }
      ctx.df[idx].EQ((int)mode); sp.print("!!!!!");

    } else if (strcmp(dat[0],"DST")==0){
      if (dat_cnt<3){ printCmdError(sp,"DST"); return; }
      int idx; if (!checkDev("DST", dat[1], idx, sp)) return;
      long sel; if (!parseHexStrict(dat[2], sel, 1, 5)){ printCmdError(sp,"DST"); return; }
      int state = -1;
      switch((int)sel){
        case 1: state = ctx.df[idx].readState(); break;
        case 2: state = ctx.df[idx].readVolume(); break;
        case 3: state = ctx.df[idx].readEQ(); break;
        case 4: state = ctx.df[idx].readFileCounts(); break;
        case 5: state = ctx.df[idx].readCurrentFileNumber(); break;
      }
      if (state < 0) { printCmdError(sp,"DST"); return; }
      printHex4Bang(sp, state);

    } else if (strcmp(dat[0],"DPX")==0){
      if (dat_cnt<2){ printCmdError(sp,"DPX"); return; }
      long dev;
      if (!parseHexStrict(dat[1], dev, 1, 2)) { printCmdError(sp,"DPX"); return; }
      int idx = (int)dev - 1;
      if (idx<0 || idx>=2) { printCmdError(sp,"DPX"); return; }
      if (!ctx.dfConnected[idx]){ printCmdError(sp,"DPX"); return; } // per spec: invalid/unconnected -> error
      printHex4Bang(sp, 1);
    }
  }
};
