// filename : module/_API_.h
//========================================================
// コマンドAPI（抽象基底クラス）
//--------------------------------------------------------
// Ver 1.2.0 (2026/09/02) 
//========================================================
#pragma once
//┬
//■インクルード
//┴

//━━━━━━━━━━━━━━━━━
// グローバル資源
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // クライアントからのリクエスト条件
  //─────────────────
  #define DAT_LENGTH 20 // 1トークンあたりの上限バイト数

//========================================================
// コマンド・モジュールの抽象基底クラス
//========================================================
class ModuleBase {
protected:
  //┬
  //□┐メンバ
    //□コンテクスト(参照)
    MmpContext& ctx;
    //│
    //□モジュール名
    const char* modName;
    //│
    //□モジュール説明
    const char* modDesc;
    //┴

public:
  //┬
  //○コンストラクタ
  ModuleBase(
    MmpContext& argCtx , // 引数：コンテクスト
    const char* argName, // 引数：モジュール名
    const char* argDesc  // 引数：モジュールの説明
  ):
  ctx(argCtx)          , // コンテクスト(参照メンバ)に結び付ける
  modName(argName)     , // モジュール名(メンバ)にセット
  modDesc(argDesc)       // モジュール説明(メンバ)にセット
  {} // 処理なし
  //┴

  //┬
  //○デストラクタ
  virtual ~ModuleBase()
  {} // 処理なし
  //┴

  //┬
  //□┐共通インタフェイス
    //│
    //□モジュール名の参照
    const char* getModName() const {return modName;}
    //│
    //□モジュールの説明の参照
    const char* getModDesc() const {return modDesc;}
    //│
    //□コマンド所有者の確認
    bool owns(const char* cmd) const {
      // 空なら早期リターン
      if (!cmd || !modName) return false;
      // コマンド名とモジュール名を比較し判定
      const size_t nameLen = strlen(modName);
      return strncmp(cmd, modName, nameLen) == 0
        && (cmd[nameLen] == '\0' || cmd[nameLen] == '/');
    }
    //│
    //□コマンド実行(実行結果は仮想ストリームに格納)
    virtual void handle(char dat[][ DAT_LENGTH ], int dat_cnt) = 0;
    //┴
}; /* class ModuleBase */


//========================================================
// ヘルパ
//========================================================
  //─────────────────
  // コンテクストへレスポンスをセット
  //─────────────────
    //─────────────────
    // 基本レスポンス
    //─────────────────
    inline void _ResOK    (){ctx.resMSG = "!!!!!";} // 正常終了
    inline void _ResNotCmd(){ctx.resMSG = "#CMD!";} // コマンド名が不正
    inline void _ResChkErr(){ctx.resMSG = "#CHK!";} // 引数チェックで不正
    inline void _ResIniErr(){ctx.resMSG = "#INI!";} // データが未初期化
    inline void _ResDevErr(){ctx.resMSG = "#DEV!";} // 使用不可のデバイス
    inline void _ResFilErr(){ctx.resMSG = "#FIL!";} // ファイル操作が異常終了
    inline void _ResNoDErr(){ctx.resMSG = "#NOD!";} // データ項目名が不正  
    inline void _ResValErr(){ctx.resMSG = "#VAL!";} // 数値変換エラー  

    //─────────────────
    // 戻値用：十進数変換
    //  - 末尾は '!' で埋める）
    //  - v ∈ [-999, 9999] 以外はエラー
    //─────────────────
    inline void _ResValue(int v) {
      if (v < -999 || v > 9999) { _ResValErr(); return; }
      char buf[6];  // 5文字 + NUL
      if (v < 0) {
        int a = -v;  // 安全に絶対値化
        snprintf(buf, sizeof(buf), "-%03ld!", a);
      } else {
        snprintf(buf, sizeof(buf), "%04d!"  , v);
      } /* if */
      ctx.resMSG = String(buf);
    } /* _ResValue() */


  //─────────────────
  // 各種編集
  //─────────────────
    //─────────────────
    // 引数用：文字列→10進数パース
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
    // 引数用：文字列→論理値パース
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
    // 先頭トークンを削除したコマンド取得
    //─────────────────
    inline const char* _Remove1st(const char* s) {
      if (!s) return s;
      while (*s==' ' || *s=='\t') ++s;
      const char* slash = strchr(s, '/');
      if (slash && slash > s) return slash + 1;
      return s;
    } /* _Remove1st() */