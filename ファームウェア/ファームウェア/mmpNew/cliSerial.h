// filename : cliSerial.h
//========================================================
// クライアント：シリアル通信
//-------------------------------------------------------- 
// Ver 1.0.0 (2025/11/11) 初版
//========================================================
#include <Adafruit_NeoPixel.h>

// 統一入口(fnPerser.h)
extern String MMP_REQUEST(const String& wire, int clientID);

// NwoPixel(個数=1) コンテクストのメンバ
#define NEOPIXEL_PIN 38   // Waveshare ESP32-S3-Tiny: WS2812 DIN=GPIO38
Adafruit_NeoPixel g_pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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

    pinMode(SW_PIN_A, INPUT_PULLUP);
    pinMode(SW_PIN_B, INPUT_PULLUP);
    pinMode(SW_PIN_C, INPUT_PULLUP);
    delay(10);

    int A = (digitalRead(SW_PIN_A) == LOW) ? 1 : 0;
    int B = (digitalRead(SW_PIN_B) == LOW) ? 1 : 0;
    int C = (digitalRead(SW_PIN_C) == LOW) ? 1 : 0;

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
    g_pixels.begin();        // RGB-LEDを点灯
    g_pixels.clear();
    g_pixels.setPixelColor(0, g_pixels.Color(c.g, c.r, c.b));
    g_pixels.show();

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
// 受信バッファリング → コマンド実行 → レスポンス送信
//========================================================
namespace {

  // USB=0 / UART0=1 の各受信バッファ
  String g_rxUsb;
  String g_rxUart0;

//━━━━━━━━━━━━━━━━━
// ルート別処理
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // ＭＭＰコマンド
  //─────────────────
  inline void routeMMP(Stream& port, String& rx, int clientID){

    // フレーミング
    while (port.available()){

      // 1) 受信バッファリング
        char ch = (char)port.read();
        rx += ch;

        // ‘!’ までのフレームを取り出して順次処理
        int p = rx.indexOf('!');
        if (p < 0) continue;

        // 末尾'!'を削除
        String wire = rx.substring(0, p+1);
        rx.remove(0, p+1);

      // 2) コマンドパーサーへリクエスト
      String resp = MMP_REQUEST(wire, clientID);

      // 3) レスポンスを編集
      // (該当処理なし)

      // 4) 通信経路にレスポンスする
      port.print(resp);
    } // while

  } // routeMMP()

//━━━━━━━━━━━━━━━━━
// ルーティング登録
//━━━━━━━━━━━━━━━━━
// (該当処理なし)

} // namespace (No Name)


//========================================================
// ハンドラ関連処理
//========================================================
namespace srvSerial {
  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  // - cliNet.hから実行
  //━━━━━━━━━━━━━━━━━
  bool start() {

    // 1) 前処理
      // 二重起動チェック： → 起動済みの場合は中断

      // スロット情報を確保
      // (該当処理なし)

    // 2) サーバを起動

    // 3) 正常終了
    return true;
  } // start()

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

    // 3) 全スロットを処理
    routeMMP(Serial , g_rxUsb  , 0);
    routeMMP(Serial1, g_rxUart0, 1);
  } // handle()

} // namespace srvSerial
