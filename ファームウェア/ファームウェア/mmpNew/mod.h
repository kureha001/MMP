// filename : mod.h
//========================================================
// 各モジュールの共通機能・抽象基底クラス
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// クライアントからのリクエスト条件
#define REQUEST_LENGTH  96  // リクエスト全体のバッファ長
#define DAT_COUNT       10  // コマンド＋引数の個数
#define DAT_LENGTH      20  // 上記1個あたりの上限バイト数

// 経路ID（参考：perser.h の MMP_REQUEST と対応）
static constexpr int MMP_CLIENT_USB   = 0;
static constexpr int MMP_CLIENT_UART0 = 1;
static constexpr int MMP_CLIENT_TCP   = 2;
static constexpr int MMP_CLIENT_HTTP  = 3;

//─────────────────
// モジュール別のRGB-LED点灯色
//─────────────────
struct LedColor { uint8_t r,g,b; };
static constexpr LedColor RGB_INFO    = {  5,  5,  5};
static constexpr LedColor RGB_ANALOG  = { 10,  0, 10};
static constexpr LedColor RGB_DIGITAL = { 10,  0,  0};
static constexpr LedColor RGB_PWM     = {  0,  0, 50};
static constexpr LedColor RGB_I2C     = { 10, 10,  0};
static constexpr LedColor RGB_MP3     = {  0, 10,  0};

//━━━━━━━━━━━━━━━━━
// コンテクスト(共通の疎結合データ)
//  - clientID : 経路番号（0=USB,1=UART0,2=TCP,3=HTTP,...）
//  - reply    : 現在の“返信出力先”Stream（二重ポインタ；RAIIで一時差替）
//━━━━━━━━━━━━━━━━━
struct MmpContext {
  int                 clientID      ; // 対象クライアントID
  int                 clientID_max  ; // クライアントIDの上限
  Stream**            reply         ; // 返信出力先（MMP_REPLY(ctx) が参照）
  Adafruit_NeoPixel*  pixels        ; // コマンド別のRGB-LED発行用
  const char*         version       ;
};
inline Stream& MMP_REPLY(MmpContext& ctx){ return **ctx.reply; }

//━━━━━━━━━━━━━━━━━
// モジュール(抽象基底クラス)
//━━━━━━━━━━━━━━━━━
class ModuleBase {
protected:
  // 依存性注入
  MmpContext& ctx;  //コンテクスト
  // スコープ
  LedColor    led;  // モジュール別ＬＥＤ色
public:
  //───── コンストラクタ(実行開始) ─────
  ModuleBase(MmpContext& c, LedColor col): ctx(c), led(col) {}
  //───── デストラクタ(特になし) ─────
  virtual ~ModuleBase() {}
  // 抽象基底クラス(モジュール インターフェース)
  virtual bool owns(const char* cmd) const                    = 0;  // コマンド在籍確認
  virtual void handle(char dat[][ DAT_LENGTH ], int dat_cnt)  = 0;  // コマンド・パーサー
};

//━━━━━━━━━━━━━━━━━
// モジュール用ユーティリティ
//━━━━━━━━━━━━━━━━━
  // ───────────────────────
  // ＬＥＤ点滅(RAIIガード実行)
  // ───────────────────────
  class LedScope {
    // 依存性注入
    MmpContext& ctx;  // コンテクスト
  public:
    //─────コンストラクタ(点灯)─────
    LedScope(MmpContext& c, LedColor col) : ctx(c){
      ctx.pixels->setPixelColor(0, ctx.pixels->Color(col.g, col.r, col.b));
      ctx.pixels->show();
    }
    //───── デストラクタ(消灯) ─────
    ~LedScope(){
      ctx.pixels->clear();
      ctx.pixels->show();
    }
    //─────────────────
  };

  // ───────────────────────
  // 文字列→10進数パース
  // ───────────────────────
  inline bool _Str2Int(const char* s, int& out, int minv, int maxv){
    if (!s || !*s)                return false; // 空チェック
    char* end = nullptr;
    out = int(strtol(s, &end, 10));
    if (*end != '\0'             ) return false; // 終端チェック
    if (minv > maxv              ) return false; // 大小チェック
    if (out  < minv || out > maxv) return false; // 範囲チェック
    return true;
  }

  // ───────────────────────
  // 文字列→論理値パース
  // ───────────────────────
  inline bool _Str2bool(const char* s, bool& out){
    if (!s || !*s) return false;          // 空チェック
    char* end = nullptr;
    int num   = int(strtol(s, &end, 10)); // 数値変換
    out       = (num > 0) ? true : false; // 数値変換
    if (*end != '\0') return false;       // 終端チェック
    return true;
  }

  // ───────────────────────
  // 十進数変換（5バイト固定: 末尾は '!' で埋める）
  //  - v ∈ [-999, 9999] 以外は #FLW! を返す
  // ───────────────────────
  inline void _ResValue(Stream& sp, int v) {
    if (v < -999 || v > 9999) { sp.print("#FLW!"); return; }
    char buf[6];  // 5文字 + NUL
    if (v < 0) {
      int a = -v;  // 安全に絶対値化
      snprintf(buf, sizeof(buf), "-%03ld!", a);
    } else {
      snprintf(buf, sizeof(buf), "%04d!", v);
    }
    sp.print(buf);
  }

  // ───────────────────────
  // 先頭トークンを削除したコマンドを取得（任意ユーティリティ）
  // ───────────────────────
  inline const char* _Remove1st(const char* s) {
      if (!s) return s;
      while (*s==' ' || *s=='\t') ++s;
      const char* slash = strchr(s, '/');
      if (slash && slash > s) return slash + 1;
      return s;
  }
  
  //━━━━━━━━━━━━━━━━━
  // クライアントへレスポンス
  //━━━━━━━━━━━━━━━━━
  inline void _ResOK    (Stream& sp){sp.print("!!!!!");} // 正常終了
  inline void _ResNotCmd(Stream& sp){sp.print("#CMD!");} // コマンド名が不正
  inline void _ResChkErr(Stream& sp){sp.print("#CHK!");} // 引数チェックで不正
  inline void _ResIniErr(Stream& sp){sp.print("#INI!");} // データが未初期化
  inline void _ResDevErr(Stream& sp){sp.print("#DEV!");} // 使用不可のデバイス
  inline void _ResFilErr(Stream& sp){sp.print("#FIL!");} // ファイル操作が異常終了
  inline void _ResNoDErr(Stream& sp){sp.print("#NOD!");} // データ項目名が不正
  
