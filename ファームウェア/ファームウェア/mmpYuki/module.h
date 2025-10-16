// filename : module.h
//========================================================
// 各モジュールの共通機能・抽象基底クラス
//-------------------------------------------------------- 
// 変更履歴: Ver0.5.01 (2025/10/16)
// - 10進数統一
// 変更履歴: Ver0.5.00 (2025/10/13)
// - クライアントからのリクエスト条件をperse.hより移管
// - コマンド応答ヘルパーを一新
//========================================================
#pragma once

// クライアントからのリクエスト条件
#define REQUEST_LENGTH  96  // リクエスト全体のバッファ長
#define DAT_COUNT       10  // コマンド＋引数の個数
#define DAT_LENGTH      20  // 上記1個あたりの上限バイト数

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
  // 文字列→10進数パース
  // ───────────────────────
  inline bool _Str2Int(const char* s, int& out, int minv, int maxv){

    if (!s || !*s)                return false; // 文字列チェック

    char* end = nullptr;
    out = int(strtol(s, &end, 10));

    if (*end != '\0'             ) return false; // 改行コード無しチェック
    if (minv > maxv              ) return false; // 大小チェック
    if (out  < minv || out > maxv) return false; // 範囲チェック

    return true;
  }
  // ───────────────────────
  // 16進数変換（5バイト固定: 末尾は '!' で埋める）
  // ───────────────────────
  inline void _ResValue(Stream& sp, int v) {

    // 桁あふれ
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
  // 先頭トークンを削除したコマンドを取得
  // ───────────────────────
  const char* _Remove1st(const char* s) {
      if (!s) return s;
      while (*s==' ' || *s=='\t') ++s;        // 先頭の空白を無視（任意）
      const char* slash = strchr(s, '/');
      if (slash && slash > s) return slash + 1; // 先頭カテゴリ＋'/'があれば、その直後
      return s;                                 // なければそのまま
  }
  
  //━━━━━━━━━━━━━━━━━
  // レスポンス
  //━━━━━━━━━━━━━━━━━
  inline void _ResOK    (Stream& sp){sp.print("!!!!!");} // 正常終了
  inline void _ResNotCmd(Stream& sp){sp.print("#CMD!");} // コマンド名違反
  inline void _ResChkErr(Stream& sp){sp.print("#CHK!");} // チェック違反
  inline void _ResIniErr(Stream& sp){sp.print("#INI!");} // 初期化違反
  inline void _ResDevErr(Stream& sp){sp.print("#DEV!");} // デバイス違反