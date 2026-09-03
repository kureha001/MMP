// filename : dev.cpp
//========================================================
// デバイスの初期化
//--------------------------------------------------------
//【目的】
//・RGB-LEDデバイスを利用可能にする
//・通信デバイスを起動させる
//--------------------------------------------------------
//【公開資源】
//・INIT_DEVICE()：通信デバイスを起動させる
//--------------------------------------------------------
//【処理機能】
//・RGB-LEDデバイスを提供する
//・通信デバイスの初期化を指示する
//--------------------------------------------------------
// Ver 1.2.0 (2026/09/02) 
//========================================================
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "dev.h"
  //│
  //■ＭＭＰシステム(通信デバイス群）
  #include "device/UART.cpp"
  #include "device/WiFi.cpp"
  #include "device/BLE.cpp"
//┴

void INIT_DEVICE() {
    // --------------------------------------------------
    // 設定値の整合性チェック（ビルドガード）
    // --------------------------------------------------
    #if !defined(MMP_TYPE_MAIN) && !defined(ADP_COM_UART)
    #error "【設定エラー】サブ構成の場合は ADP_COM_UART の定義が必須です！"
    #endif
    #if defined(MMP_TYPE_MAIN) && !defined(ADP_COM_UART)
    #warning "【確認】サブ機を繋げる場合は UART を有効にしてください。"
    #endif

 //┬
  //○USB(CDC)ポートを起動
  Serial.begin(115200);          // USB(CDC)
  Serial.setDebugOutput(false);  // SDKデバッグ出力を抑止
  delay(2000);                   // 安定するまで待つ
  Serial.println("<<通信デバイスの初期化>>");
  Serial.println(" [Serial device]"  );
  Serial.println("　 [OK] USB (CDC) -> 115,200bps");
  //│
  //●通信デバイスを初期化
#if defined(ADP_COM_UART ) //----------------┨UART┠┐
  devUART::START();
#endif // -------------------------------------------┘

// -----------┨TcpRaw｜WebAPI｜WebSoc｜ESP-Now|WEB┠┐
#if defined(ADP_COM_TCP)||defined(ADP_COM_WAPI)||defined(ADP_COM_WSOC)||defined(ADP_COM_ESPN)
  devWiFi::START();
#endif // -------------------------------------------┘

#if defined(ADP_COM_BLE  ) // ------------┨ＢＬＥ┠-┐
  devBLE::START();
#endif // -------------------------------------------┘
  //┴
} /* INIT_DEVICE() */