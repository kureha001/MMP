// SerialScope.h
#pragma once
#include "MmpContext.h"

// Switch ctx.serial to a given Stream* and set currentClient index while in scope
class SerialScope {
  MmpContext& ctx;
  Stream* prev;
  int prevClient;
public:
  SerialScope(MmpContext& c, Stream* s, int clientIndex) : ctx(c){
    prev = *ctx.serial;
    prevClient = *ctx.currentClient;
    *ctx.serial = s;
    *ctx.currentClient = clientIndex;
  }
  ~SerialScope(){
    *ctx.serial = prev;
    *ctx.currentClient = prevClient;
  }
};
