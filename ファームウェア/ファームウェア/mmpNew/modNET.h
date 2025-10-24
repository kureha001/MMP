// filename : modNET.h
//========================================================
// モジュール：Wifi設定
//--------------------------------------------------------
// Ver 0.6.0 (2025/xx/xx)
//========================================================
#pragma once
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "mod.h"

//━━━━━━━━━━━━━━━
// グローバル
//━━━━━━━━━━━━━━━
constexpr int g_MAX_ITEM_HOST   = 4;  // ホスト情報ののアイテム登録数
constexpr int g_MAX_ITEM_ROUTER = 6;  // Wifiルーター情報のアイテム登録数

//━━━━━━━━━━━━━━━━━
// 設定ファイル情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型：ホスト（JSON: host[]）
  //─────────────────
  struct typeHost {
    String type;  // "sta" | "ap"
    String name;  // hostname
    String ip;    // STA:末尾オクテット or "", AP:フルIP
  };
  //─────────────────
  //  型：Wi-Fi候補（JSON: wifi[]）
  //─────────────────
  struct typeRouter {
    String label;
    String ssid;
    String pass;
    bool   isDefault = false;  // JSONの "default"
  };
  //─────────────────
  //  型：接続情報
  //─────────────────
  struct typeConnect {
    typeHost    hostList[g_MAX_ITEM_HOST];
    typeRouter  candList[g_MAX_ITEM_ROUTER];
    int         hostNum = 0;
    int         candNum = 0;
  };
  //─────────────────
  //  型：サーバ情報
  //─────────────────
  struct typeServer {
    int      maxClients   = 0;
    bool     writeLock    = false;
    uint32_t writeLockMs  = 0;
  };

//━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━
typeConnect g_WIFI;                       // 規定値はここでは持たない
typeServer  g_SRV = { 4, false, 30000 };  // 既定値
static const char* g_FILENAME = "/config.json";

//━━━━━━━━━━━━━━━
// ユーティリティ
//━━━━━━━━━━━━━━━
  // ───────────────────────────────
  // 機能 : JSONを保存
  // 引数 : ① ＰＷＭ値      　；出力PWM値(引数値)
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  bool jsonSave(const JsonDocument& doc){
    File f = LittleFS.open(g_FILENAME, "w");
    if (!f) return false;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
  }
  // ───────────────────────────────
  // 機能 : JSONを新規作成
  // 引数 : なし
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  bool jsonCreate() {
      JsonDocument doc;
      doc["server"];                  // 空object
      doc["host"  ].to<JsonArray>();  // 空配列
      doc["wifi"  ].to<JsonArray>();  // 空配列
      return jsonSave(doc);
  }
  // ───────────────────────────────
  // 機能 : JSONを読込
  // 引数 : ① ＰＷＭ値      　；出力PWM値(引数値)
  // 戻値 : 合否；true=正常／false=違反
  // ───────────────────────────────
  bool jsonLoad(JsonDocument& doc) {

    if (!LittleFS.begin(true)) return false;

    if (!LittleFS.exists(g_FILENAME)) {
      // 空骨格を作る
      doc.clear();
      doc["server"];                 // object（空）
      doc["host"  ].to<JsonArray>();   // array
      doc["wifi"  ].to<JsonArray>();   // array
      File nf = LittleFS.open(g_FILENAME, "w");
      // → 作成に失敗
      if (!nf) return false;

      // → 作成に成功
      serializeJsonPretty(doc, nf);
      nf.close();
      return true;
    }

    // → 作成に成功
    File f = LittleFS.open(g_FILENAME, "r");
    if (!f) return false;

    DeserializationError err = deserializeJson(doc, f);
    f.close();
    return !err;
  }


//━━━━━━━━━━━━━━━━━
// メイン処理
//━━━━━━━━━━━━━━━━━
class ModuleNetConf : public ModuleBase {
public:
  using ModuleBase::ModuleBase;

  //━━━━━━━━━━━━━━━━━
  // コマンド在籍確認(実装)
  //━━━━━━━━━━━━━━━━━
  bool owns(const char* cmd) const override {
      return cmd && strncmp(cmd, "WIFI", 4) == 0;
  }

  //━━━━━━━━━━━━━━━━━
  // コマンド・パーサー(実装)
  //━━━━━━━━━━━━━━━━━
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    Stream&     sp = MMP_REPLY(ctx);      // 返信出力先（経路抽象）
    LedScope    scopeLed(ctx, led);       // コマンド色のLED発光
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正


  //━━━━━━━━━━━━━━━━━
  // ファイル
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : ファイル：新規作成
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"FILE/CREATE") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(sp); return;}

      // ２．処理：
        // ファイル作成 → 失敗すれば中断
        if (!jsonCreate()){_ResFilErr(sp); return;}

      // ３．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : ファイル：削除
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"FILE/DELETE") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(sp); return;}

      // ２．処理：
        // ファイルシステムが無効ならば中断
        if (!LittleFS.begin(true)) { _ResFilErr(sp); return; }

        // ファイルを削除
        if   (LittleFS.exists(g_FILENAME)) {
          if (!LittleFS.remove(g_FILENAME)) {
            _ResFilErr(sp);
            return;
          }
        }

      // ３．後処理：
      _ResOK(sp);
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // サーバー情報
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : サーバー情報：アイテムをセット
    // 引数 : ① 整数型：クライアント接続上限
    //        ② 整数型：書込ロック有無{有：1以上｜無：０以下}
    //        ③ 整数型：書込ロック時間(ミリ秒)
    // 制限 : ① １～９
    //        ② －９９９～９９９
    //　　　　③ ０～９９９９
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SERVER/SETUP") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 4){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      int  clientNum, lockTime;
      bool lockEnable;
      if (!_Str2Int (dat[1], clientNum, 1, 9    )  ||
          !_Str2bool(dat[2], lockEnable         )  ||
          !_Str2Int (dat[3], lockTime,  0, 9999 ) ){_ResChkErr(sp); return;}

      // ２．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        // サーバー情報をセット
        JsonObject row = doc["server"].to<JsonObject>();
        row["max_clients"  ] = clientNum;
        row["write_lock"   ] = lockEnable;
        row["write_lock_ms"] = lockTime;

        // ファイル保存 → 失敗の場合：ファイル異常！！
        if (!jsonSave(doc)){_ResFilErr(sp); return;}

      // ３．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : サーバー情報：アイテムを削除
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SERVER/RESET") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(sp); return;}

      // ２．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        // サーバー情報を削除
        doc.remove("server");

        // ファイル保存 → 失敗の場合：ファイル異常！！
        if (!jsonSave(doc)){_ResFilErr(sp); return;}

      // ３．後処理：
      _ResOK(sp);
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // ホスト情報
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : ホスト情報：アイテムをセット
    // 引数 : ① 文字型：タイプ{STAモード:"sta" | APモード:"ap"}
    //        ② 文字型： ホスト名
    //        ③ 文字型：STA:末尾オクテット or "", AP:フルIP
    // 制限 : ① {"sta"|"ap"}
    //　　　　② 空文字以外
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"HOST/SETUP") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 4){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      if (!( String(dat[1]).equalsIgnoreCase("sta") ||
             String(dat[1]).equalsIgnoreCase("ap")) ){_ResChkErr(sp); return;}
      if (dat[2][0] == '\0'                         ){_ResChkErr(sp); return;}

      // ２．処理準備：
      String strBase  = "host";
      String strKey   = "type";

      // ３．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        //◇グループ内のアイテムをキー捜査する：
        JsonArray docBase = doc[strBase].to<JsonArray>();
        bool      found   = false;
        for (JsonObject row : docBase){
          // → タイプと一致する場合：
          String col = String((const char*)(row[strKey] | ""));
          if (col.equalsIgnoreCase(dat[1])){
            row["name"] = dat[2];
            row["ip"  ] = dat[3];
            found = true;
            break;
          }
        }

        // → ひとつもヒットしない場合：
        if (!found){
          // アイテムを追加
          JsonObject row = docBase.add<JsonObject>();
          row["type"] = dat[1];
          row["name"] = dat[2];
          row["ip"  ] = dat[3];
        }

        // ファイル保存 → 失敗の場合：ファイル異常！！
        if (!jsonSave(doc)){_ResFilErr(sp); return;}

      // ４．後処理：
      _ResOK(sp);
      return;
    }

  //━━━━━━━━━━━━━━━━━
  // ルーター情報
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能 : ルーター情報：アイテムをセット
    // 引数 : ① 文字型：ラベル
    //        ② 文字型：SSID 
    //        ③ 文字型：パスワード
    //        ④ 整数型：優先接続{有：1以上｜無：０以下}
    // 制限 : ① 空文字以外
    //　　　　② 空文字以外
    //　　　　③ 空文字以外
    //　　　　④ －９９９～９９９９
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"ROUTER/SETUP") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 5){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      bool bValue;
      if  (dat[1][0]=='\0' || dat[2][0]=='\0' || dat[3][0]=='\0'){_ResChkErr(sp); return;}
      if  (!_Str2bool(dat[4], bValue)                           ){_ResChkErr(sp); return;}

      // ２．処理準備：
      String strBase  = "wifi";
      String strKey   = "label";

      // ３．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        //◇グループ内のアイテムをキー捜査する：
        JsonArray docBase = doc[strBase].to<JsonArray>();
        bool      found   = false;
        for (JsonObject row : docBase){
          String col = String((const char*)(row[strKey] | ""));
          if (col.equalsIgnoreCase(dat[1])){
          // → カレントのアイテムがヒットした場合：
            // 全ての項目を更新
            row["ssid"   ] = dat[2];
            row["pass"   ] = dat[3];
            row["default"] = bValue;
            found = true;
            break;
          }
        }

        // → ひとつもヒットしない場合：
        if (!found){
          // アイテムを追加
          JsonObject row = docBase.add<JsonObject>();
          row["label"  ] = dat[1];
          row["ssid"   ] = dat[2];
          row["pass"   ] = dat[3];
          row["default"] = bValue;
        }

        // ファイル保存 → 失敗の場合：ファイル異常！！
        if (!jsonSave(doc)){_ResFilErr(sp); return;}

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : ルーター情報：優先接続をセット
    // 引数 : ① 文字型：ラベル
    //        ② 整数型：優先接続{有：1以上｜無：０以下}
    // 制限 : ① 空文字以外
    //　　　　② －９９９～９９９９
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"ROUTER/DEFAULT") == 0){

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 3){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      bool bValue;
      if (dat[1][0] == '\0'         ){_ResChkErr(sp); return;}
      if (!_Str2bool(dat[2], bValue)){_ResChkErr(sp); return;}

      // ２．処理準備：
      String strBase  = "wifi";
      String strKey   = "label";

      // ３．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        //◇グループ内のアイテムをキー捜査する：
        JsonArray docBase = doc[strBase].to<JsonArray>();
        bool      found   = false;
        for (JsonObject row : docBase){
        // → カレントのアイテムがヒットした場合：
          // 優先接続の項目を更新
          String col = String((const char*)(row[strKey] | ""));
          if (col.equalsIgnoreCase(dat[1])){
            row["default"] = bValue;
            found = true;
            break;
          }
        }

        // → ひとつもヒットしない場合：データなし！！
        if (!found){_ResNoDErr(sp); return;}

        // ファイル保存 → 失敗の場合：ファイル異常！！
        if (!jsonSave(doc)){_ResFilErr(sp); return;}

      // ４．後処理：
      _ResOK(sp);
      return;
    }

    // ───────────────────────────────
    // 機能 : ホスト情報：アイテムを削除(タイプ指定)
    // 引数 : ① 文字型：タイプ
    // 制限 : ① 空文字以外
    // 戻り : _ResOK
    // ───────────────────────────────
    // 機能 : ホスト情報：アイテムを削除(タイプ指定)
    // 引数 : ① 文字型：ホスト名
    // 制限 : ① 空文字以外
    // 戻り : _ResOK
    // ───────────────────────────────
    // 機能 : ルーター情報：アイテムを削除
    // 引数 : ① 文字型：ラベル
    // 制限 : ① 空文字以外
    // 戻り : _ResOK
    // ───────────────────────────────
    if ( (strcmp(Cmd,"HOST/RESET/TYPE") == 0) ||
         (strcmp(Cmd,"HOST/RESET/NAME") == 0) ||
         (strcmp(Cmd,"ROUTER/RESET"   ) == 0) ) {

      // １．前処理：
      // 1.1.書式チェック
      if (dat_cnt != 2){_ResChkErr(sp); return;}

      // 1.2.単項目チェック
      if (dat[1] == ""){_ResChkErr(sp); return;}

      // ２．処理準備：
      String strBase,strKey;
      if      (strcmp(Cmd,"HOST/RESET/TYPE") == 0){strBase = "host"; strKey = "type";}
      else if (strcmp(Cmd,"HOST/RESET/NAME") == 0){strBase = "host"; strKey = "name";}
      else if (strcmp(Cmd,"ROUTER/RESET"   ) == 0){strBase = "wifi"; strKey = "label";}
      else { _ResNotCmd(sp); return; }

      // ３．処理：
        // ファイルを開く → 失敗の場合：ファイル異常！！
        JsonDocument doc;
        if (!jsonLoad(doc)){_ResFilErr(sp); return;}

        //◇グループ内のアイテムをキー捜査する：
        JsonArray docBase = doc[strBase].to<JsonArray>();
        bool      found   = false;
        for (size_t i = 0; i < docBase.size(); ++i){
          JsonObject row = docBase[i];
          String    col  = String((const char*)(row[strKey] | ""));
          if (col.equalsIgnoreCase(dat[1])){
          // → ヒットする場合：
            // アイテムを削除
            docBase.remove(i);
            // ファイル保存 → 失敗すれば中断
            if (!jsonSave(doc)){_ResFilErr(sp); return;}
            found = true;
            break;
          }
        }

        // → ひとつもヒットしない場合：
        if (!found){_ResChkErr(sp); return;}

    // ４．後処理：
      _ResOK(sp);
      return;
    }

    //━━━━━━━━━━━━━━━━━
    // コマンド名エラー
    //━━━━━━━━━━━━━━━━━
    _ResNotCmd(sp);
  }

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
// なし
};
