// filename : fnPerser.h
//========================================================
// コマンド パーサーのフロント・対象資源の登録
//--------------------------------------------------------
// Ver0.6.00 (2025/xx/xx)
//========================================================
#pragma once
#include <vector>
#include <string.h>
#include "mod.h"

// 監視対象のモジュール
#include "modINF.h"
#include "modANA.h"
#include "modDIG.h"
#include "modPWM.h"
#include "modI2C.h"
#include "modMP3.h"

//========================================================
// 外部参照（本体側で定義）
//========================================================
extern MmpContext    ctx;        // グローバル・コンテクスト
class Perser;
extern Perser*       g_perser;   // パーサー本体へのポインタ

//========================================================
// 前方宣言：統一入口（runCommand から参照するため）
//========================================================
String MMP_REQUEST(const String& wire, int clientID);

//========================================================
/** 返信先(RAIIガード) : ctx.reply と clientID を一時差替 */
class ReplyScope {
  MmpContext& ctxRef;
  Stream*     prevReply;
  int         prevClientID;
public:
  ReplyScope(MmpContext& c, Stream* argReply, int argClientID)
  : ctxRef(c), prevReply(*c.reply), prevClientID(c.clientID){
    *ctxRef.reply   = argReply;
    ctxRef.clientID = argClientID;
  }
  ~ReplyScope(){
    *ctxRef.reply   = prevReply;
    ctxRef.clientID = prevClientID;
  }
};

//========================================================
// パーサー
//========================================================
class Perser {

  // ストリーム(クライアント)定義
  struct Source {
    Stream*     s;                      // ストリーム オブジェクト
    const char* name;                   // 名称
    int         clientIndex;            // 0=USB, 1=UART0, 2=TCP, 3=HTTP...
    char        buf[ REQUEST_LENGTH ];  // リクエスト受信バッファ
    int         len = 0;                // リクエスト長カウンタ
  };

  // 依存性注入
  MmpContext&               ctxRef;     // コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  mods;       // 監視対象のモジュール
  std::vector<Source>       sources;    // 監視対象のクライアント

  // 仮想出力ストリーム（モジュールのprintを蓄積）
  class StringStream : public Stream {
    String out;
  public:
    size_t write(uint8_t c) override { out += (char)c; return 1; }
    int    available()      override { return  0; }
    int    read()           override { return -1; }
    int    peek()           override { return -1; }
    void   flush()          override {}
    String str()            const    { return out; }
  };

  //━━━━━━━━━━━━━━━━━
  // 既存：コマンド実行（内部は ExecuteString に統一）
  //━━━━━━━━━━━━━━━━━
  void runCommand(Source& src){
    // src.buf は '!' を '\0' に置換済み
    String wire = String(src.buf);
    // 統一入口へ（経路番号付き：USB=0 / UART0=1）
    MMP_REQUEST(wire, src.clientIndex);
    // viaSerial() が実ポートへ 5バイトを出力するため、ここでは戻るのみ
  }

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Perser(MmpContext& c): ctxRef(c) {}

  //━━━━━━━━━━━━━━━━━
  // 対象資源の登録
  //━━━━━━━━━━━━━━━━━
  void Init(){
    // 監視対象のモジュールを登録
    mods.push_back(new ModuleInfo   (ctxRef, RGB_INFO   ));
    mods.push_back(new ModuleAnalog (ctxRef, RGB_ANALOG ));
    mods.push_back(new ModuleDigital(ctxRef, RGB_DIGITAL));
    mods.push_back(new ModulePwm    (ctxRef, RGB_PWM    ));
    mods.push_back(new ModuleI2C    (ctxRef, RGB_I2C    ));
    mods.push_back(new ModuleDF     (ctxRef, RGB_MP3    ));

    // クライアント：USB(CDC) / GPIO(UART0)
    addSource(&Serial , "USB"  , 0);
    addSource(&Serial1, "UART0", 1);
  }

  // ───────────────────────
  void addSource(Stream* s, const char* name, int clientIndex){
    Source src;
    src.s           = s;
    src.name        = name;
    src.clientIndex = clientIndex;
    src.buf[0]      = 0;
    src.len         = 0;
    sources.push_back(src);

    if (clientIndex > ctxRef.clientID_max) ctxRef.clientID_max = clientIndex;
  }

  //━━━━━━━━━━━━━━━━━
  // 追加：任意文字列を直接実行（I/O非依存・5バイト応答）
  //   wire 例: "DIGITAL:OUTPUT:12:1!" 末尾'!'は無くても可
  //━━━━━━━━━━━━━━━━━
  String ExecuteString(const String& wire){
    // 1) ワークバッファへ安全コピー
    char buf[ REQUEST_LENGTH ];
    {
      size_t n = wire.length();
      if (n >= sizeof(buf)) n = sizeof(buf) - 1;
      memcpy(buf, wire.c_str(), n);
      buf[n] = '\0';
    }
    // 2) 末尾 '!' は除去（あっても無くてもOKにする）
    {
      size_t n = strlen(buf);
      if (n > 0 && buf[n-1] == '!') buf[n-1] = '\0';
    }

    // 3) ':' 区切りでトークン分解
    char dat[ DAT_COUNT ][ DAT_LENGTH ];
    int  dat_cnt = 0;
    {
      char* tok = strtok(buf, ":");
      while (tok && dat_cnt < DAT_COUNT){
        strncpy(dat[dat_cnt], tok, sizeof(dat[0])-1);
        dat[dat_cnt][sizeof(dat[0])-1] = '\0';
        dat_cnt++;
        tok = strtok(nullptr, ":");
      }
      if (dat_cnt == 0) return "#CMD!";
    }

    // 4) 仮想ストリームに出力を蓄積し、モジュールに処理させる
    StringStream ss;
    {
      // ctxRef.clientID は呼び出し元（MMP_REQUEST）で設定済みを想定
      ReplyScope scope(ctxRef, &ss, ctxRef.clientID);
      for (auto* m : mods){
        if (m->owns(dat[0])){
          m->handle(dat, dat_cnt);
          return ss.str(); // 5バイト（!!!!!, XXXX!, #CMD! など）
        }
      }
    }
    // 5) 未登録コマンド
    return "#CMD!";
  }

  //━━━━━━━━━━━━━━━━━
  // すべてのクライアントを待ち受ける（Serial経路）
  //━━━━━━━━━━━━━━━━━
  void pollAll(){
    for (auto& src : sources){
      while (src.s && src.s->available()){
        char c = (char)src.s->read();
        if (src.len < (int)sizeof(src.buf)-1) src.buf[src.len++] = c;

        if (c=='!'){
          src.buf[src.len-1] = '\0';   // 末尾置換
          runCommand(src);             // ← 統一入口へ
          src.len = 0;
        }
      }
    }
  }
}; // ←←← ここで必ずクラスを閉じる！ (重要)

//========================================================
// 経路別ラッパ関数群
//   - viaSerial : 5バイト応答を実ポートへ出力
//   - viaTcp    : 戻り値として5バイト応答を返す（送信は呼び元）
//   - viaHttp   : 同上（JSON化はHTTP層で）
//========================================================
inline String viaSerial(const String& wire, Stream& port){
  // ctx.clientID は MMP_REQUEST() で設定済み
  String res = g_perser->ExecuteString(wire);
  port.print(res);
  return res;
}
inline String viaTcp(const String& wire){
  return g_perser->ExecuteString(wire);
}
inline String viaHttp(const String& wire){
  return g_perser->ExecuteString(wire);
}

//========================================================
// 統一呼び出し：MMP_REQUEST()
//   - 全経路からここを通す（Serial/TCP/HTTP）
//   - ctx.clientID のみで経路識別（出力先の実体は関数内で扱う）
//   - 戻り値は常に5バイト応答
//========================================================
inline String MMP_REQUEST(const String& wire, int clientID){
  ctx.clientID = clientID;
  switch (clientID){
    case 0: return viaSerial(wire, Serial ); // USB CDC
    case 1: return viaSerial(wire, Serial1); // GPIO UART
    case 2: return viaTcp   (wire);          // TCP（送信は呼び元）
    case 3: return viaHttp  (wire);          // HTTP（送信/JSON化は呼び元）
    default:
      // 未定義の経路ID
      return "#DEV!";
  }
}

// 互換（旧API）
inline String MMP_REQUEST(const String& wire){
  // 経路不明扱い：DEVエラーに倒す（旧呼び出しの早期検出用）
  // 必要であればここを 0(USB) にフォールバックすることも可能
  return MMP_REQUEST(wire, -1);
}
