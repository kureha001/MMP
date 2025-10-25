// filename : fnPerser.h
//========================================================
// コマンド パーサーのフロント・対象資源の登録
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
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
#include "modNET.h"

//========================================================
// 外部参照(本体側で定義)
//========================================================
extern  MmpContext    ctx;        // グローバル・コンテクスト
class   Perser;
extern  Perser*       g_perser;   // パーサー本体へのポインタ

//========================================================
// 前方宣言：統一入口
//========================================================
String MMP_REQUEST(const String& wire, int clientID);

//========================================================
// 返信先(RAIIガード) : ctx.reply と clientID を一時差替
//========================================================
class ReplyScope {

  MmpContext& ctxRef;
  Stream*     prevReply;
  int         prevClientID;

public:
  //─────コンストラクタ─────
  ReplyScope(
    MmpContext& c,            // コンテキスト
    Stream*     argReply,     // 通信経路
    int         argClientID   // 通信経路ID
  ):
    ctxRef(c),
    prevReply(*c.reply),
    prevClientID(c.clientID){
      *ctxRef.reply   = argReply    ;
      ctxRef.clientID = argClientID ;
    } // prevClientID()
  //───── デストラクタ ─────
  ~ReplyScope(){
    *ctxRef.reply   = prevReply;
    ctxRef.clientID = prevClientID;
  }
  //─────────────────
};


//========================================================
// パーサー
//========================================================
class Perser {

  // ストリーム(クライアント)定義
  struct Source {
    const char* name;                   // 名称
    int         clientIndex;            // 0=USB, 1=UART0, 2=TCP, 3=HTTP...
    char        buf[ REQUEST_LENGTH ];  // リクエスト受信バッファ
    int         len = 0;                // リクエスト長カウンタ
  };

  // 依存性注入
  MmpContext&               ctxRef;     // コンテクスト

  // 保有情報
  std::vector<ModuleBase*>  mods;       // 監視対象の機能モジュール
  std::vector<Source>       sources;    // リクエストしてきたクライアント

  // 仮想出力ストリーム(モジュールのprintを蓄積)
  class StringStream : public Stream {

    String out;

    public:
      size_t write(uint8_t c) override { out += (char)c; return 1; }
      int    available()      override { return  0; }
      int    read()           override { return -1; }
      int    peek()           override { return -1; }
      void   flush()          override {}
      String  str()           const    { return out; }
  }; // class StringStream

public:
  //━━━━━━━━━━━━━━━━━
  // コンストラクタ
  //━━━━━━━━━━━━━━━━━
  Perser(MmpContext& c): ctxRef(c) {}

  //━━━━━━━━━━━━━━━━━
  // パーサーの初期化
  //━━━━━━━━━━━━━━━━━
  void Init(){
    // 機能モジュールの登録
    mods.push_back(new ModuleInfo   (ctxRef, RGB_INFO   ));
    mods.push_back(new ModuleAnalog (ctxRef, RGB_ANALOG ));
    mods.push_back(new ModuleDigital(ctxRef, RGB_DIGITAL));
    mods.push_back(new ModulePwm    (ctxRef, RGB_PWM    ));
    mods.push_back(new ModuleI2C    (ctxRef, RGB_I2C    ));
    mods.push_back(new ModuleMP3    (ctxRef, RGB_MP3    ));
    mods.push_back(new ModuleNetConf(ctxRef, RGB_INFO   ));

    // 通信経路の登録：0=USB, 1=UART0, 2=TCP, 3=HTTP...
    addSource("USB"  , 0);
    addSource("UART0", 1);
    addSource("TCP"  , 2);
    addSource("HTTP" , 3);
  } // Init()

  //━━━━━━━━━━━━━━━━━
  // 通信経路の追加
  //━━━━━━━━━━━━━━━━━
  void addSource(
    const char* name,         // 識別名は自由にユニークで
    int         clientIndex   // 0=USB, 1=UART0, 2=TCP, 3=HTTP
  ){
    Source src;
    src.name        = name;
    src.clientIndex = clientIndex;
    src.buf[0]      = 0;
    src.len         = 0;
    sources.push_back(src);

    // 通信経路数をインクリメント
    if (clientIndex > ctxRef.clientID_max) ctxRef.clientID_max = clientIndex;
  } // addSource()

  //━━━━━━━━━━━━━━━━━
  // 追加：任意文字列を直接実行
  //━━━━━━━━━━━━━━━━━
  String ExecuteString(const String& wire){

    // 1) ワークバッファへ安全コピー
    char buf[ REQUEST_LENGTH ];
    {
      size_t n = wire.length();
      if (n >= sizeof(buf)) n = sizeof(buf) - 1;
      memcpy(buf, wire.c_str(), n);
      buf[n] = '\0';
    } // --- 1) ---

    // 2) 末尾 '!' は除去(あっても無くてもOKにする)
    {
      size_t n = strlen(buf);
      if (n > 0 && buf[n-1] == '!') buf[n-1] = '\0';
    } // --- 2) ---

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
      } // while
      if (dat_cnt == 0) return "#CMD!";
    } // --- 3) ---

    // 4) 仮想ストリームに出力を蓄積し、モジュールに処理させる
    StringStream ss;
    {
      ReplyScope scope(ctxRef, &ss, ctxRef.clientID);
      for (auto* m : mods){
        if (m->owns(dat[0])){
          m->handle(dat, dat_cnt);
          return ss.str(); // 5バイト(!!!!!, XXXX!, #CMD! など)
        } // if
      }   // for
    } // --- 4) ---

    // 5) 未登録コマンド
    return "#CMD!";
  }

}; // class Perser


//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 統一呼び出し
// - 全経路はここを通る
// - ctx.clientID のみで経路識別
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
inline String MMP_REQUEST(const String& wire, int clientID){

  // 通信経路を変更
  ctx.clientID = clientID;

  // 通信経路別にコマンド・パース処理
  switch (clientID){
  case 0  : return g_perser->ExecuteString(wire); // シリアル通信(USB CDC)
  case 1  : return g_perser->ExecuteString(wire); // シリアル通信(GPIO UART)
  case 2  : return g_perser->ExecuteString(wire); // TCP(送信は呼び元)
  case 3  : return g_perser->ExecuteString(wire); // HTTP(送信/JSON化は呼び元)
  default : return "#DEV!"                  ; // 未定義の経路ID
  } // switch

} // MMP_REQUEST()