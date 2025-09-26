// Util.h
#pragma once
#include <Arduino.h>

// Strict hex parser: returns true if *entire* s is valid hex and fits in [minv, maxv].
inline bool parseHexStrict(const char* s, long& out, long minv, long maxv){
  if (!s || !*s) return false;
  char* end=nullptr;
  out = strtol(s, &end, 16);
  if (*end != '\0') return false;      // reject partial parses like "1G"
  if (out < minv || out > maxv) return false;
  return true;
}

// Utility to print 4-digit uppercase hex + '!' into a small buffer
inline void printHex4Bang(Stream& sp, long v){
  char buf[6];
  snprintf(buf, sizeof(buf), "%04X!", int(v) & 0xFFFF);
  sp.print(buf);
}

// Utility to print 5-byte error "<CMD>!!"
inline void printCmdError(Stream& sp, const char* cmd3){
  char err[6]; strncpy(err, cmd3, 3); err[3]='!'; err[4]='!'; err[5]='\0';
  sp.print(err);
}
