// ModuleInfo.h
#pragma once
#include "ModuleBase.h"

class ModuleInfo : public ModuleBase {
public:
  using ModuleBase::ModuleBase;
  bool owns(const char* cmd) const override { return strcmp(cmd,"VER")==0; }
  void handle(char dat[][10], int dat_cnt) override {
    LedScope _led(ctx, led);
    MMP_SERIAL(ctx).print(ctx.versionStr);
  }
};
