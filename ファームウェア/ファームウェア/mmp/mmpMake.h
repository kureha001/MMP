// filename : mmpMake.h
//========================================================
// システム構築
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
#pragma once

//========================================================
//§コンパイルオプション（ユーザが設定する）
//========================================================
//┬
//□┐コンパイルオプション
  //□【動作モード】選択スイッチ
  #define MODE_MAIN    0 // 選択肢：メインモード
  #define MODE_SUB     1 // 選択肢：サブモード
  #define MODE_BRIDGE  2 // 選択肢：ブリッジモード
  #define MODE MODE_BRIDGE // ★選択肢のいずれかをセットする
  //│
  //□【高速化】スイッチ
  // Stream型を単一スロット＋パケット単位で処理する。
  // [Dep_Connect/adapter/_index_.h]にて分岐定義する。
  //【利用した場合の制限】
  // ・メ イ ン：サブ連携が不可
  // ・サ　　ブ：GPIOのUARTは使用不可(USB-CDC,メイン連携は可能)
  // ・ブリッジ：GPIOのUARTは使用不可(USB-CDCは可能)
  #define TURBO_ON  true  // 選択肢：利用する
  #define TURBO_OFF false // 選択肢：利用しない
  #define TURBO TURBO_ON // ★選択肢のいずれかをセットする
  //│
  //□【通信アダプタ】選択スイッチ
    //│・選択肢：利用する  (true  をセット)
    //│・選択肢：利用しない(false をセット)
    //│各モードの通信アダプタを設定(選択肢のいずれかをセット)する
    //□★メインモード用
    #if   MODE == MODE_MAIN
      #define ADP_UART true // 未使用ではサブ連携が不可、UARTモジュールは動作可能
      #define ADP_UDP  true
      #define ADP_TCP  true
      #define ADP_WSOC true
      #define ADP_HTTP true
      #define ADP_ESPN true
      #define ADP_BLE  true
      #define ADP_IIC  true // Wire1を利用|WireはPWMで利用
    //□★サブモード用
    #elif MODE == MODE_SUB
      #define ADP_UART true // 未使用でもメイン連携が可能
      #define ADP_UDP  true
      #define ADP_TCP  true
      #define ADP_WSOC true
      #define ADP_HTTP true
      #define ADP_ESPN true
      #define ADP_BLE  true
      #define ADP_IIC  false // Wire1を使用|Wireは空き
    //□★ブリッジモード用
    #elif MODE == MODE_BRIDGE
      #define ADP_UART true  //※強制的に有効化される
      #define ADP_UDP  true
      #define ADP_TCP  true
      #define ADP_WSOC true
      #define ADP_HTTP true
      #define ADP_ESPN true
      #define ADP_BLE  true
      #define ADP_IIC  false //※変更禁止：現在未対応
    #endif
//┴┴┴

//========================================================
//§通信デバイス有効化スイッチ（ユーザによる変更は禁止）
//========================================================
//┬
//□┐通信デバイス有効化スイッチ
  //□UARTデバイス ※常に有効
  //│
  //□IICデバイス
  //│※メイン機はIICモジュールを装備しているので必要
  #if (MODE==MODE_MAIN) || ADP_IIC
  #define USE_IIC
  #endif
  //│
  //□WiFiデバイス
  //│※WiFiを基盤とする通信アダプタが有効な場合は必要
  #if ADP_UDP || ADP_TCP || ADP_WSOC || ADP_HTTP || ADP_ESPN
  #define USE_WiFi
  #endif
  //│  
  //□BLEデバイス
  //│※BlueToothを基盤とする通信アダプタが有効な場合は必要
  #if ADP_BLE
  #define USE_BLUETOOTH
  #endif
//┴┴
