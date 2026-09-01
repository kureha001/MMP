// filename : adp-stream.cpp
//========================================================
// 通信アダプタ共通：ストリーム受信用
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■ＭＭＰシステム
  #include "adp.h"  // 通信アダプタ共通へ公開
  //┴
//┴

//========================================================
// ストリーム受信用の部品
//========================================================
  //─────────────────
  // スロット初期化
  //----------------------------------
  // 通信アダプタの名前空間で派生(名称:INIT_SLOT)
  //─────────────────
  void SS_INI_SLOT_BASE(SS_SLOT_TYPE& argSlot){
    argSlot.used   = false; // スロット有効性を「無効」
    argSlot.isOver = false; // 容量超過フラグを「OFF」
    argSlot.rx     = ""   ; // 受信バッファをクリア
    argSlot.rx.reserve(SS_RX_SIZE); // 容量確保
  } /* SS_INI_SLOT_BASE */

  //─────────────────
  // ストリーム型のデータ処理
  //----------------------------------
  // 引数：
  // ・受信バッファ
  // ・オーバーフローフラグ
  // ・エラーメッセージ
  //----------------------------------
  // 戻り値：受信継続の要否（論理値）
  // ・true ：受信継続が「不要」
  // ・false：受信継続が「必要」
  //─────────────────
  bool READ_STREAM(
    SS_SLOT_TYPE argBASE  , // スロット(ベース)
    String       &argFrame  // エラーMSG返却
){
    //┬
    //○オーバーフロー発生を確認
    if (argBASE.rx.length() > SS_RX_SIZE) {
    //│＼（発生した場合）
        //○オーバーフロー中へ移行
        //○受信バッファをクリア
        //▼返却：受信継続が「不要」
        argBASE.isOver = true;
        argBASE.rx     = ""  ;
        return true;
    } /* END-if */
    //│
    //○取り込み状態を確認
    if (!argBASE.rx.endsWith("!")) return false;
    //│＼（終端に達していない場合）
    //│ ▼返却：受信継続が「必要」
    //│
    //○オーバーフロー中を確認
    if (argBASE.isOver) {
    //│＼（オーバーフロー中の場合）
        //○オーバーフロー中を解除
        //○受信バッファをクリア
        //●エラーコードをフレームにセット
        //▼返却：受信継続が「不要」
        argBASE.isOver = false  ;
        argBASE.rx     = ""     ;
        argFrame       = "#DFL!";
        return true;
    } /* END-if */
    //│
    //●受信バッファをURI形式に変換
    P1_FORMAT_URI(argBASE.rx);
    //│
    //○フレームを作成
    argFrame       = argBASE.rx;
    argBASE.rx     = ""     ;
    //│
    //▼返却：受信継続が「不要」
    return true;
  } /* READ_STREAM() */

  //━━━━━━━━━━━━━━━━━
  // ストリームからフレームを取得
  //----------------------------------
  // 引数：(参照)接続管理スロット
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：完成
  // ・false：未完成
  //━━━━━━━━━━━━━━━━━
  String P2_STREAM(
    Stream&      argConn, // 通信資源
    SS_SLOT_TYPE argBASE  // スロット(ベース)
  ){
    String retFrame = "";
    bool   isStop = false;

    while (argConn.available()){             ; // 受信バッファあり
      argBASE.rx += (char)argConn.read()     ; // 1バイト受信
      isStop = READ_STREAM(argBASE, retFrame); // 処理判断
      if (isStop) break                      ; // 継続無し
    } /* END-while */

    return retFrame; // フレーム返却(エラーコード含む)
  }