// filename : module.h
//========================================================
// 各モジュールの共通機能・抽象基底クラス
//-------------------------------------------------------- 
// 変更履歴: Ver 0.4.01 (2025/10/07)
// - 関連ファイルを統合
//========================================================
#pragma once

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
//━━━━━━━━━━━━━━━━━
struct MmpContext {
  int                 clientID      ; // 対象クライアントID: 0=USB, 1=UART0
  int                 clientID_max  ; // クライアントIDの上限
  Stream**            serial        ; // ストリームのハンドル
  Adafruit_NeoPixel*  pixels        ; // コマンド別のRGB-LED発行用
  const char*         version       ;
};
inline Stream& MMP_SERIAL(MmpContext& ctx){ return **ctx.serial; }


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
  virtual bool owns(const char* cmd) const          = 0;  // コマンド在籍確認
  virtual void handle(char dat[][10], int dat_cnt)  = 0;  // コマンド・パーサー
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
    //───── コンストラクタ(点灯) ─────
    LedScope(MmpContext& c, LedColor col) : ctx(c){
      ctx.pixels->setPixelColor(0, ctx.pixels->Color(col.g, col.r, col.b));
      ctx.pixels->show();
    }
    //───── デストラクタ(消灯) ─────
    ~LedScope(){
      ctx.pixels->clear();
      ctx.pixels->show();
    }
  };

  // ───────────────────────
  // 16進数(string)→10進数(long)パース
  // ───────────────────────
  inline bool strHex2long(const char* s, long& out, long minv, long maxv){

    if (!s || !*s)                return false; // 文字列チェック

    char* end = nullptr;
    out = strtol(s, &end, 16);

    if (*end != '\0'            ) return false; // 改行コード無しチェック
    if (minv > maxv             ) return false; // 大小チェック
    if (out < minv || out > maxv) return false; // 範囲チェック

    return true;
  }

  // ───────────────────────
  // 16進数変換（5バイト固定: 末尾は '!' で埋める）
  // ───────────────────────
  inline void ResHex4(Stream& sp, int16_t v) {
    char buf[6]; //終端NUL(\0)が必要なので 5 + 1 = 6
    snprintf(buf, sizeof(buf), "%04X!", (uint16_t)v);
    sp.print(buf);
  }
  // ───────────────────────
  // コマンドエラー "<CMD>!!"
  // ───────────────────────
  inline void ResCmdErr(Stream& sp, const char* cmd3){
    char err[6]; //終端NUL(\0)が必要なので 5 + 1 = 6
    strncpy(err, cmd3, 3);  // コマンド名
    err[3]='!';
    err[4]='!';
    err[5]='\0';
    sp.print(err);
  }