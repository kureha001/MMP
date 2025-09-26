// LedScope.h
#pragma once
#include "MmpContext.h"

struct LedColor { uint8_t r,g,b; };

class LedScope {
  MmpContext& ctx;
public:
  LedScope(MmpContext& c, LedColor col) : ctx(c){
    ctx.pixels->setPixelColor(0, ctx.pixels->Color(col.g, col.r, col.b));
    ctx.pixels->show();
  }
  ~LedScope(){
    ctx.pixels->clear();
    ctx.pixels->show();
  }
};
