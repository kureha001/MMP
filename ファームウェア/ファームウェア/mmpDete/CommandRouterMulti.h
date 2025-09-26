// CommandRouterMulti.h
#pragma once
#include <vector>
#include "ModuleBase.h"
#include "SerialScope.h"
#include "MmpConfig.h"

class CommandRouterMulti {
  struct Source {
    Stream* s;
    const char* name;
    int clientIndex; // 0=USB, 1=UART0
    char buf[MMP_INPUT_MAX];
    int  len = 0;
  };

  MmpContext& ctx;
  std::vector<ModuleBase*> modules;
  std::vector<Source> sources;

  void processLine(Source& src){
    char dat[5][10]; int dat_cnt=0;
    char* tok = strtok(src.buf, ":");
    while(tok && dat_cnt<5){
      strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
      dat[dat_cnt][sizeof(dat[0])-1] = '\0';
      dat_cnt++;
      tok = strtok(nullptr, ":");
    }
    if (dat_cnt==0) return;
    SerialScope out(ctx, src.s, src.clientIndex);
    for (auto* m: modules){
      if (m->owns(dat[0])){ m->handle(dat, dat_cnt); return; }
    }
    // Unknown -> "CMD?!"
    char resp[6] = "!!!?!";
    strncpy(resp, dat[0], 3);
    MMP_SERIAL(ctx).print(resp);
  }

public:
  CommandRouterMulti(MmpContext& c): ctx(c) {}

  void addModule(ModuleBase* m){ modules.push_back(m); }

  void addSource(Stream* s, const char* name, int clientIndex){
    Source src; src.s=s; src.name=name; src.clientIndex=clientIndex; src.len=0; src.buf[0]=0;
    sources.push_back(src);
  }

  void pollAll(){
    for (auto& src : sources){
      while (src.s && src.s->available()){
        char c = (char)src.s->read();
        if (src.len < (int)sizeof(src.buf)-1) src.buf[src.len++] = c;
        if (c=='!'){
          src.buf[src.len-1] = '\0';
          processLine(src);
          src.len = 0;
        }
      }
    }
  }
};
