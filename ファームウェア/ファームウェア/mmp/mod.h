// filename : mod.h
//========================================================
// 機能モジュール：共通
//  - コンテキストの提供
//  - 抽象基底クラスの提供
//  - RAIIガード機能の提供
//  - ユーティリティの提供
//--------------------------------------------------------
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#pragma once

// クライアントからのリクエスト条件
#define REQUEST_LENGTH  96  // リクエスト全体のバッファ長
#define DAT_COUNT       10  // コマンド＋引数の個数
#define DAT_LENGTH      20  // 上記1個あたりの上限バイト数

//─────────────────
// モジュール別のRGB-LED点灯色
//─────────────────
struct typeColor { uint8_t r,g,b; };
static constexpr typeColor RGB_INFO    = {  5,  5,  5};
static constexpr typeColor RGB_ANALOG  = { 10,  0, 10};
static constexpr typeColor RGB_DIGITAL = { 10,  0,  0};
static constexpr typeColor RGB_PWM     = {  0,  0, 50};
static constexpr typeColor RGB_I2C     = { 10, 10,  0};
static constexpr typeColor RGB_MP3     = {  0, 10,  0};

//━━━━━━━━━━━━━━━━━
// グローバル資源(定義)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 仮想出力ストリーム
  //─────────────────
  class StringStream : public Stream {
    String out;
  public:
      // ストリームをオーバーライド
      size_t  write(uint8_t c) override { out += (char)c; return 1; }
      int     available()      override { return  0;  }
      int     read()           override { return -1;  }
      int     peek()           override { return -1;  }
      void    flush()          override {             }
      // 独自メソッドを追加
      void    clear()                   { out = "";   }
      String  str()            const    { return out; }
  }; /* class StringStream */

  //─────────────────
  // コンテクスト
  // - 実　装：スケッチ
  //─────────────────
  struct MmpContext {
    StringStream        vStream     ; // メンバとして保持
    int                 clientID    ; // クライアントID
    Adafruit_NeoPixel*  pixels      ; // RGB-LED発光用
    const char*         version     ; // システムのバージョン
  };

//━━━━━━━━━━━━━━━━━
// モジュール(抽象基底クラス)
//━━━━━━━━━━━━━━━━━
class ModuleBase {
protected:

  // 依存性注入
  MmpContext& ctx;  //コンテクスト

  // スコープ
  typeColor   led;  // モジュール別ＬＥＤ色

public:
  //─────コンストラクタ─────
  ModuleBase(
    MmpContext& argCtx, // コンテクスト
    typeColor   argCol  // モジュール別LED色
  ):
  ctx(argCtx),  // コンテクスト
  led(argCol)   // モジュール別LED色
  {} // 処理なし
  //───── デストラクタ ─────
  virtual ~ModuleBase()
  {} // 処理なし
  //─────────────────

  //─────────────────
  // 抽象基底クラス
  // (モジュール インターフェース)
  //─────────────────
  virtual bool owns(const char* cmd) const = 0;                   // コマンド在籍確認
  virtual void handle(char dat[][ DAT_LENGTH ], int dat_cnt) = 0; // コマンド・パーサー

}; /* class ModuleBase */


//━━━━━━━━━━━━━━━━━
// モジュール用：前処理
//━━━━━━━━━━━━━━━━━
  // ───────────────────────
  // LED点滅(RAIIガード)
  // ───────────────────────
  class LedScope {

    // データ退避ワーク
    MmpContext& ctx;  // コンテクスト

  public:
    //─── コンストラクタ(点灯) ───
    LedScope(
      MmpContext& argCtx, // ※ModuleBase経由で指定
      typeColor   argCol  // ※ModuleBase経由で指定
    ) :
    ctx(argCtx) //コンテクスト
    {
      ctx.pixels->setPixelColor(0, ctx.pixels->Color(argCol.g, argCol.r, argCol.b));
      ctx.pixels->show();
    }
    //────デストラクタ(消灯)────
    ~LedScope()
    {
      ctx.pixels->clear();
      ctx.pixels->show();
    }
    //─────────────────
  }; /* class LedScope */


//━━━━━━━━━━━━━━━━━
// モジュール用：ユーティリティ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 文字列→10進数パース
  //─────────────────
  inline bool _Str2Int(const char* s, int& out, int minv, int maxv){
    if (!s || !*s)                return false; // 空チェック
    char* end = nullptr;
    out = int(strtol(s, &end, 10));
    if (*end != '\0'             ) return false; // 終端チェック
    if (minv > maxv              ) return false; // 大小チェック
    if (out  < minv || out > maxv) return false; // 範囲チェック
    return true;
  } /* _Str2Int() */

  //─────────────────
  // 文字列→論理値パース
  //─────────────────
  inline bool _Str2bool(const char* s, bool& out){
    if (!s || !*s) return false;          // 空チェック
    char* end = nullptr;
    int num   = int(strtol(s, &end, 10)); // 数値変換
    out       = (num > 0) ? true : false; // 数値変換
    if (*end != '\0') return false;       // 終端チェック
    return true;
  } /* _Str2bool() */

  //─────────────────
  // 十進数変換
  //  - 末尾は '!' で埋める）
  //  - v ∈ [-999, 9999] 以外は #FLW!
  //─────────────────
  inline void _ResValue(Stream& sp, int v) {
    if (v < -999 || v > 9999) { sp.print("#FLW!"); return; }
    char buf[6];  // 5文字 + NUL
    if (v < 0) {
      int a = -v;  // 安全に絶対値化
      snprintf(buf, sizeof(buf), "-%03ld!", a);
    } else {
      snprintf(buf, sizeof(buf), "%04d!"  , v);
    } /* if */
    sp.print(buf);
  } /* _ResValue() */

  //─────────────────
  // 先頭トークンを削除したコマンド取得
  //─────────────────
  inline const char* _Remove1st(const char* s) {
      if (!s) return s;
      while (*s==' ' || *s=='\t') ++s;
      const char* slash = strchr(s, '/');
      if (slash && slash > s) return slash + 1;
      return s;
  } /* _Remove1st() */
  
  //─────────────────
  // クライアントへレスポンス
  //─────────────────
  inline void _ResOK    (Stream& sp){sp.print("!!!!!");} // 正常終了
  inline void _ResNotCmd(Stream& sp){sp.print("#CMD!");} // コマンド名が不正
  inline void _ResChkErr(Stream& sp){sp.print("#CHK!");} // 引数チェックで不正
  inline void _ResIniErr(Stream& sp){sp.print("#INI!");} // データが未初期化
  inline void _ResDevErr(Stream& sp){sp.print("#DEV!");} // 使用不可のデバイス
  inline void _ResFilErr(Stream& sp){sp.print("#FIL!");} // ファイル操作が異常終了
  inline void _ResNoDErr(Stream& sp){sp.print("#NOD!");} // データ項目名が不正  
