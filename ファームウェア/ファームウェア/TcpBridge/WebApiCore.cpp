// ============================================================
// WebApiCore.cpp (GET-only, paths aligned with MmpClient hierarchy)
// - すべて GET クエリで操作
// - ルート直下に /digital/* /analog/* /pwm/* /audio/* /i2c/* /info/* /conn/*
// - ハンドラは MmpCore 経由で UART0 の先機器を制御（常時オープン前提）
// - CORS/OPTIONS 対応（ブラウザからのアクセス向け）
// ============================================================

#include "WebApiCore.h"
#include "MmpCore.h"
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
  WebServer* g_srv = nullptr;

  // ---- CORS ----
  inline void add_cors() {
    if (!g_srv) return;
    g_srv->sendHeader("Access-Control-Allow-Origin", "*");
    g_srv->sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    g_srv->sendHeader("Access-Control-Allow-Headers", "Content-Type, X-Requested-With, Authorization");
    g_srv->sendHeader("Access-Control-Max-Age", "600");
  }
  inline void send_json(const String& s, int code=200) {
    add_cors();
    g_srv->send(code, "application/json", s);
  }
  inline void send_err(const String& msg, int code=400) {
    add_cors();
    g_srv->send(code, "application/json", String("{\"ok\":false,\"error\":\"")+msg+"\"}");
  }
  inline void send_204() { add_cors(); g_srv->send(204); }

  // ---- arg helpers ----
  inline bool argI(const char* key, int& out, int def=0, bool required=false) {
    if (!g_srv->hasArg(key)) { if (required) return false; out = def; return true; }
    out = g_srv->arg(key).toInt(); return true;
  }

  // ---------- 基本 ----------
  void h_root() {
    send_json(F("{\"ok\":true,\"service\":\"MMP-WebAPI\",\"note\":\"GET-only\"}"));
  }
  void h_info_version() {
    String v = MmpCore::Info::Version();
    send_json(String("{\"ok\":true,\"version\":\"")+v+"\"}");
  }
  void h_info_pwm() {
    int dev=0; argI("dev", dev, 0, false);
    uint16_t id = MmpCore::Info::DevPwm(dev);
    send_json(String("{\"ok\":true,\"dev\":")+dev+",\"pwm\":\"0x"+String(id,HEX)+"\"}");
  }
  void h_info_audio() {
    int dev=1; argI("dev", dev, 1, false);
    uint16_t id = MmpCore::Info::DevAudio(dev);
    send_json(String("{\"ok\":true,\"dev\":")+dev+",\"audio\":\"0x"+String(id,HEX)+"\"}");
  }

  // ---------- Conn ----------
  void h_conn_isOpen()   { send_json(String("{\"ok\":true,\"isOpen\":") + (MmpCore::Conn::IsOpen()?"true":"false")+"}"); }
  void h_conn_baud()     { send_json(String("{\"ok\":true,\"baud\":")    + MmpCore::Conn::Baud()     + "}"); }
  void h_conn_port()     { send_json(String("{\"ok\":true,\"port\":\"")  + MmpCore::Conn::Port()     + "\"}"); }
  void h_conn_lastError(){ send_json(String("{\"ok\":true,\"lastError\":\"") + MmpCore::Conn::LastError() + "\"}"); }

  // ---------- Digital ----------
  void h_digital_in() {
    int id; if (!argI("id", id, 0, true)) { send_err("missing:id"); return; }
    int v = MmpCore::Digital::In(id);
    send_json(String("{\"ok\":true,\"id\":")+id+",\"value\":"+v+"}");
  }
  void h_digital_out() {
    int id,val; if (!argI("id", id, 0, true) || !argI("val", val, 0, true)) { send_err("missing:id/val"); return; }
    bool ok = MmpCore::Digital::Out(id, val);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"id\":"+id+",\"val\":"+val+"}");
  }

  // ---------- Analog (HC4067) ----------
  void h_analog_configure() {
    int chTtl, devTtl;
    if (!argI("chTtl", chTtl, 0, true) || !argI("devTtl", devTtl, 0, true)) { send_err("missing:chTtl/devTtl"); return; }
    bool ok = MmpCore::Analog::Configure(chTtl, devTtl);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"chTtl\":"+chTtl+",\"devTtl\":"+devTtl+"}");
  }
  void h_analog_update() {
    bool ok = MmpCore::Analog::Update();
    send_json(String("{\"ok\":")+(ok?"true":"false")+"}");
  }
  void h_analog_read() {
    int ch, dev;
    if (!argI("ch", ch, 0, true) || !argI("dev", dev, 0, true)) { send_err("missing:ch/dev"); return; }
    int v = MmpCore::Analog::Read(ch, dev);
    send_json(String("{\"ok\":true,\"ch\":")+ch+",\"dev\":"+dev+",\"value\":"+v+"}");
  }

  // ---------- PWM ----------
  void h_pwm_out() {
    int ch,val;
    if (!argI("ch", ch, 0, true) || !argI("val", val, 0, true)) { send_err("missing:ch/val"); return; }
    bool ok = MmpCore::Pwm::Out(ch, val);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"ch\":"+ch+",\"val\":"+val+"}");
  }
  void h_pwm_angle_init() {
    int angleMin, angleMax, pwmMin, pwmMax;
    if (!argI("angleMin", angleMin, 0, true) ||
        !argI("angleMax", angleMax, 0, true) ||
        !argI("pwmMin"  , pwmMin  , 0, true) ||
        !argI("pwmMax"  , pwmMax  , 0, true)) { send_err("missing:angleMin/angleMax/pwmMin/pwmMax"); return; }
    bool ok = MmpCore::Pwm::AngleInit(angleMin, angleMax, pwmMin, pwmMax);
    send_json(String("{\"ok\":")+(ok?"true":"false")+"}");
  }
  void h_pwm_angle() {
    int ch,angle;
    if (!argI("ch", ch, 0, true) || !argI("angle", angle, 0, true)) { send_err("missing:ch/angle"); return; }
    bool ok = MmpCore::Pwm::AngleOut(ch, angle);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"ch\":"+ch+",\"angle\":"+angle+"}");
  }

  // ---------- Audio (DFPlayer) ----------
  void h_audio_volume() {
    int dev,value;
    if (!argI("dev", dev, 1, true) || !argI("value", value, 20, true)) { send_err("missing:dev/value"); return; }
    bool ok = MmpCore::Audio::Volume(dev, value);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"dev\":"+dev+",\"value\":"+value+"}");
  }
  void h_audio_setEq() {
    int dev,mode;
    if (!argI("dev", dev, 1, true) || !argI("mode", mode, 0, true)) { send_err("missing:dev/mode"); return; }
    bool ok = MmpCore::Audio::SetEq(dev, mode);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"dev\":"+dev+",\"mode\":"+mode+"}");
  }
  void h_audio_play_start() {
    int dev,dir,file;
    if (!argI("dev", dev, 1, true) || !argI("dir", dir, 1, true) || !argI("file", file, 1, true)) { send_err("missing:dev/dir/file"); return; }
    bool ok = MmpCore::Audio::Play::Start(dev, dir, file);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"dev\":"+dev+",\"dir\":"+dir+",\"file\":"+file+"}");
  }
  void h_audio_play_setLoop() {
    int dev,mode;
    if (!argI("dev", dev, 1, true) || !argI("mode", mode, 0, true)) { send_err("missing:dev/mode"); return; }
    bool ok = MmpCore::Audio::Play::SetLoop(dev, mode != 0);
    send_json(String("{\"ok\":")+(ok?"true":"false")+",\"dev\":"+dev+",\"mode\":"+mode+"}");
  }
  void h_audio_play_stop()   { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} bool ok=MmpCore::Audio::Play::Stop(dev);   send_json(String("{\"ok\":")+(ok?"true":"false")+"}"); }
  void h_audio_play_pause()  { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} bool ok=MmpCore::Audio::Play::Pause(dev);  send_json(String("{\"ok\":")+(ok?"true":"false")+"}"); }
  void h_audio_play_resume() { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} bool ok=MmpCore::Audio::Play::Resume(dev); send_json(String("{\"ok\":")+(ok?"true":"false")+"}"); }

  void h_audio_read_state()      { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} int v=MmpCore::Audio::Read::State(dev);       send_json(String("{\"ok\":true,\"state\":")+v+"}"); }
  void h_audio_read_volume()     { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} int v=MmpCore::Audio::Read::Volume(dev);      send_json(String("{\"ok\":true,\"volume\":")+v+"}"); }
  void h_audio_read_eq()         { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} int v=MmpCore::Audio::Read::Eq(dev);          send_json(String("{\"ok\":true,\"eq\":")+v+"}"); }
  void h_audio_read_fileCounts() { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} int v=MmpCore::Audio::Read::FileCounts(dev);  send_json(String("{\"ok\":true,\"fileCounts\":")+v+"}"); }
  void h_audio_read_fileNumber() { int dev; if(!argI("dev",dev,1,true)){send_err("missing:dev");return;} int v=MmpCore::Audio::Read::FileNumber(dev);  send_json(String("{\"ok\":true,\"fileNumber\":")+v+"}"); }

  // ---------- I2C ----------
  void h_i2c_write() {
    int addr,reg,val;
    if (!argI("addr",addr,0,true) || !argI("reg",reg,0,true) || !argI("val",val,0,true)) { send_err("missing:addr/reg/val"); return; }
    bool ok = MmpCore::I2c::Write(addr, reg, val);
    send_json(String("{\"ok\":")+(ok?"true":"false")+"}");
  }
  void h_i2c_read() {
    int addr,reg;
    if (!argI("addr",addr,0,true) || !argI("reg",reg,0,true)) { send_err("missing:addr/reg"); return; }
    int v = MmpCore::I2c::Read(addr, reg);
    send_json(String("{\"ok\":true,\"value\":")+v+"}");
  }

  // ---- OPTIONS helper ----
  void allow_options(const char* path) { g_srv->on(path, HTTP_OPTIONS, [](){ send_204(); }); }

} // namespace

namespace WebApiCore {

void begin(WebServer& server) {
  g_srv = &server;

  // ルーティング登録
  server.on("/", HTTP_GET, h_root);

  // Info
  server.on("/info/version", HTTP_GET, h_info_version);
  server.on("/info/pwm",     HTTP_GET, h_info_pwm);
  server.on("/info/audio",   HTTP_GET, h_info_audio);

  // Conn
  server.on("/conn/isOpen",    HTTP_GET, h_conn_isOpen);
  server.on("/conn/baud",      HTTP_GET, h_conn_baud);
  server.on("/conn/port",      HTTP_GET, h_conn_port);
  server.on("/conn/lastError", HTTP_GET, h_conn_lastError);

  // Digital
  server.on("/digital/in",  HTTP_GET, h_digital_in);
  server.on("/digital/out", HTTP_GET, h_digital_out);

  // Analog
  server.on("/analog/configure", HTTP_GET, h_analog_configure);
  server.on("/analog/update",    HTTP_GET, h_analog_update);
  server.on("/analog/read",      HTTP_GET, h_analog_read);

  // PWM
  server.on("/pwm/out",       HTTP_GET, h_pwm_out);
  server.on("/pwm/angleInit", HTTP_GET, h_pwm_angle_init);
  server.on("/pwm/angle",     HTTP_GET, h_pwm_angle);

  // Audio
  server.on("/audio/volume",          HTTP_GET, h_audio_volume);
  server.on("/audio/setEq",           HTTP_GET, h_audio_setEq);
  server.on("/audio/play/start",      HTTP_GET, h_audio_play_start);
  server.on("/audio/play/setLoop",    HTTP_GET, h_audio_play_setLoop);
  server.on("/audio/play/stop",       HTTP_GET, h_audio_play_stop);
  server.on("/audio/play/pause",      HTTP_GET, h_audio_play_pause);
  server.on("/audio/play/resume",     HTTP_GET, h_audio_play_resume);

  server.on("/audio/read/state",      HTTP_GET, h_audio_read_state);
  server.on("/audio/read/volume",     HTTP_GET, h_audio_read_volume);
  server.on("/audio/read/eq",         HTTP_GET, h_audio_read_eq);
  server.on("/audio/read/fileCounts", HTTP_GET, h_audio_read_fileCounts);
  server.on("/audio/read/fileNumber", HTTP_GET, h_audio_read_fileNumber);

  // I2C
  server.on("/i2c/write", HTTP_GET, h_i2c_write);
  server.on("/i2c/read",  HTTP_GET, h_i2c_read);

  // OPTIONS for all paths (CORS preflight safety)
  const char* paths[] = {
    "/", "/info/version", "/info/pwm", "/info/audio",
    "/conn/isOpen", "/conn/baud", "/conn/port", "/conn/lastError",
    "/digital/in", "/digital/out",
    "/analog/configure", "/analog/update", "/analog/read",
    "/pwm/out", "/pwm/angleInit", "/pwm/angle",
    "/audio/volume", "/audio/setEq",
    "/audio/play/start", "/audio/play/setLoop", "/audio/play/stop", "/audio/play/pause", "/audio/play/resume",
    "/audio/read/state", "/audio/read/volume", "/audio/read/eq", "/audio/read/fileCounts", "/audio/read/fileNumber",
    "/i2c/write", "/i2c/read"
  };
  for (auto p: paths) allow_options(p);

  // 404
  server.onNotFound([](){
    if (!g_srv) return;
    if (g_srv->method() == HTTP_OPTIONS) { send_204(); return; }
    send_err(String("not-found: ")+g_srv->uri(), 404);
  });

  // 起動ログ
  IPAddress ip = (WiFi.getMode()==WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
  Serial.printf("[WebApi] GET routes registered on %s\n", ip.toString().c_str());
}

void handle(WebServer& server) {
  server.handleClient();
}

} // namespace WebApiCore
