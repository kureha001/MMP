// filename : cliSerial.h
//========================================================
// クライアント：シリアル通信
//  - シリアル通信のポーリング処理
//    (リクエスト→実行→レスポンス)
//-------------------------------------------------------- 
// Ver 1.0.0 (2025/11/14) α版
//========================================================
#include "cli.h"  // クライアント：共通ユーティリティ
#include "mod.h"  // 機能モジュール：抽象基底クラス

//━━━━━━━━━━━━━━━━━
// グローバル資源(宣言)
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // コンテクスト
  // - 型定義：mod.h
  // - 実　装：スケッチ
  //─────────────────
  extern MmpContext ctx;

  //─────────────────
  // 統一入口：fnPerser.hで定義
  //─────────────────
  extern String MMP_REQUEST(const String& path, int clientID);

//━━━━━━━━━━━━━━━━━
// 通信速度の定義・設定
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ボーレートのプリセット
  //─────────────────
  static const int BAUD_PRESETS[8] = {
    921600,57600,38400,19200,9600,4800,2400,300
  };

  //─────────────────
  // ボーレート変更スイッチのGPIO
  //─────────────────
  #define SW_PIN_A 18 // bit-0
  #define SW_PIN_B 14 // bit-1
  #define SW_PIN_C 13 // bit-2

  //─────────────────
  // ボーレート別のRGB-LED点灯色
  //─────────────────
  struct RGB { uint8_t r,g,b; };
  static const RGB BAUD_COLORS[8] = {
    /*0:白*/ {10,10,10},
    /*1:緑*/ { 0,10, 0},
    /*2:青*/ { 0, 0,10},
    /*3:水*/ { 0,10,10},
    /*4:黄*/ {10,10, 0},
    /*5:橙*/ {10, 3, 0},
    /*6:紫*/ {10, 0,10},
    /*7:赤*/ {10, 0, 0}
  };


//========================================================
// ルーティング内処理
//========================================================
//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド
  //─────────────────
  // (該当処理なし) ※fnPerser.hのhandle()に実装している


//========================================================
// ハンドラ関連処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  // - スケッチのsetup()から実行
  //━━━━━━━━━━━━━━━━━
  bool InitSerial(){

    //ボーレート設定ボタンのピンを定義
    pinMode(SW_PIN_A, INPUT_PULLUP);
    pinMode(SW_PIN_B, INPUT_PULLUP);
    pinMode(SW_PIN_C, INPUT_PULLUP);
    delay(10);

    //ボタンを読取
    int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;
    int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;
    int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;

    //ボーレートIDを取得
    int id = 7;
    if      (A==0 && B==0 && C==0) id = 0;
    else if (A==1 && B==0 && C==0) id = 1;
    else if (A==0 && B==1 && C==0) id = 2;
    else if (A==0 && B==0 && C==1) id = 3;
    else if (A==1 && B==1 && C==0) id = 4;
    else if (A==0 && B==1 && C==1) id = 5;
    else if (A==1 && B==0 && C==1) id = 6;

    // ボーレートに応じてRGB-LEDを点灯
    RGB c = BAUD_COLORS[id]; // 色パターンを取得
    ctx.pixels->begin();        // RGB-LEDを点灯
    ctx.pixels->clear();
    ctx.pixels->setPixelColor(0, ctx.pixels->Color(c.g, c.r, c.b));
    ctx.pixels->show();

    // シリアルポートを起動
    Serial.begin(BAUD_PRESETS[id]);                       // USB(CDC)
    Serial.setDebugOutput(false);                         // SDKデバッグ出力を抑止
    Serial1.begin(BAUD_PRESETS[id], SERIAL_8N1, 44, 43);  // GPIO Serial
    delay(100);
    Serial.flush();
    Serial1.flush();
    delay(100);
  
    // 起動メッセージを表示
    Serial.println("[Serial initialize]"  );
    Serial.println("　USB (CDC)      : OK");
    Serial.println("　UART(GPIO 0,1) : OK");

    return true;
  } // InitSerial()


//========================================================
// メイン処理
//========================================================
namespace {
//━━━━━━━━━━━━━━━━━
// 事前データ
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // サーバー
  //─────────────────
  // (該当処理なし)

  //─────────────────
  // 受信バッファ
  //─────────────────
  String g_BUF_USB  ; // USB(CDC)用
  String g_BUF_UART0; // UART0(GPIO)用

//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド実行
  //─────────────────
  inline void routeMMP(
    Stream& argStream , // 実ストリーム
    String& argBuf    , // 受信バッファ
    int argRouteID      // 経路ID
  ){
    //┬
    //◎┐受信バッファリング
    while (argStream.available()){
      //○バイト単位で取得
      char ch = (char)argStream.read();
      argBuf += ch;
      int p = argBuf.indexOf('!');
      if (p < 0) continue;
      // ＼（'!'ではない）
        //↓
        //▼０:スキップ
      //│
      //○末尾'!'を削除
      String path = argBuf.substring(0, p+1);
      argBuf.remove(0, p+1);
      //│
      //○コマンドパーサーへリクエスト(ルートID指定)
      String resp = MMP_REQUEST(path, argRouteID);
      //│
      //○通信経路にレスポンスする
      argStream.print(resp);
      //┴
    } /* while */
    //┴
  } /* routeMMP() */

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//━━━━━━━━━━━━━━━━━
// (該当処理なし)

} /* namespace(匿名) */


//========================================================
// ハンドラ関連処理
//========================================================
namespace srvSerial {
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  // - スケッチから実行
  //━━━━━━━━━━━━━━━━━
  bool start() {

    // 1) 前処理
      // 二重起動チェック： → 起動済みの場合は中断
      // (該当処理なし)

      // スロット情報を確保
      // (該当処理なし)

    // 2) サーバを起動
    // (該当処理なし)

    // 3) 正常終了
    return true;
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  // - スケッチのloop()から実行
  // - 実処理はxxx()
  //━━━━━━━━━━━━━━━━━
  void handle(){

    // 1) 起動チェック
    // (該当処理なし)

    // 2) 接続スロットの情報を更新
    // (該当処理なし)

    // 3) すべての接続スロットを処理
    routeMMP(Serial , g_BUF_USB  , ROUTE_ID_USB   );
    routeMMP(Serial1, g_BUF_UART0, ROUTE_ID_UART0 );
  } /* handle() */

} /* namespace srvSerial */
