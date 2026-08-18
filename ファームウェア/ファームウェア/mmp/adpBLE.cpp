// filename : adpBLE.cpp
//========================================================
// 通信アダプタ：ＢＬＥブリッジ
//--------------------------------------------------------
//【常時接続：切断時スロット開放型】
//・基本処理       ：接続確認→受信(Writeイベント)→コマンド実行→結果返却
//・スロット構成   ：BLE接続毎に空スロットに動的割り当て
//・ポーリング跨ぎ：対応 ※通信状態は継続的に保持
//・ユーザ認証     ：行わない ※TCPと同様
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/17) α版 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <BLEDevice.h>
  #include <BLEServer.h>
  #include <BLEUtils.h>
  #include <BLE2902.h>
  //│
  //■ＭＭＰシステム
  #include "adp.h" // 通信アダプタ共通
  #include "dev.h" // 統合デバイス定義（devBLEの公開資源を含む）
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace adpBLE {
//========================================================
// Ａ．基本情報
//========================================================
  static constexpr int ROUTE_ID = ROUTE_ID_BLE; // 経路IDを定義
  bool ENABLED = false; // ハンドル有効判定：有効：true、無効：false

//========================================================
// Ｂ．ユーザ認証
//========================================================
  //─────────────────
  // スロット
  //----------------------------------
  // 戻り値：なし
  //─────────────────
  void AU_CREATE_TBL(){
  // ➡【該当処理なし】※常時接続は対象外
  } /* AU_CREATE_TBL() */

//========================================================
// Ｃ．接続情報
//========================================================
  //━━━━━━━━━━━━━━━━━
  // スロット
  //----------------------------------
  // 資源：接続状態および送信用キャラクタリスティック
  // 所有：BLE接続のオブジェクトを所有する
  // 割当：BLE接続ごとに１スロット
  // 持続：接続中に生成／切断で破棄
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // 領域確保：構造体の派生→実体化
    //─────────────────
    struct T_SS_SLOT : T_SS_BASE{
      bool               connected = false  ; // 接続判定フラグ
      String             conn_rx   = ""     ; // 受信（onWriteイベントでフレーム上書）
      BLECharacteristic* conn_tx   = nullptr; // 送信（参照）
    };
    static T_SS_SLOT* ssTBL = nullptr; // 領域確保
    int    SS_SLOTS         = 0      ; // 確保する領域サイズ

    //─────────────────
    // スロット初期化：関数の派生
    //─────────────────
    void INI_SS_SLOT(T_SS_SLOT& argSlot){
      INI_SS_SLOT_BASE(argSlot);
      argSlot.connected = false  ; // 接続判定フラグを切断
      argSlot.conn_rx   = ""     ; // 受信をクリア
      argSlot.conn_tx   = nullptr; // 送信の参照解除
    } /* INI_SS_SLOT */

  //─────────────────
  // 接続情報TBLを作る
  //----------------------------------
  void SS_CREATE_TBL(){
    //┬
    //●確保する領域サイズを取得
    SS_SLOTS = GET_SS_SLOTS();
    //│
    //○領域を確保
    ssTBL = new T_SS_SLOT[SS_SLOTS]; 
    //┴
  } /* SS_CREATE_TBL() */

  //─────────────────
  // スロットを開放
  //----------------------------------
  void SS_DETACH_SLOT(T_SS_SLOT& argSlot){
    // 接続を切断は不要？
    INI_SS_SLOT(argSlot);
  } /* SS_DETACH_SLOT() */

  //─────────────────
  // 空きスロットを照会
  //----------------------------------
  int SS_GET_FREE_ID() {
  //　➡【該当処理なし】※マルチ接続系が対象
  } /* SS_GET_FREE_ID() */

  //─────────────────
  // 接続を登録（BLE接続開始時）
  //─────────────────
  void SS_ATTACH_SLOT(){
    //┬
    //●空きスロットを探す
    if (ssTBL[0].used) return;
    //│ ＼（空きスロットがない）
    //│  ▼次を探す
    //│
    //●スロットを初期化
    INI_SS_SLOT(ssTBL[0]);
    //│
    //○スロットに新規接続を登録
    ssTBL[0].used      = true; // 使用中
    ssTBL[0].connected = true; // 接続判定フラグを接続
    ssTBL[0].conn_rx   = ""  ; // 受信をクリア
    ssTBL[0].conn_tx   = devBLE::BLE_TX; // 送信の参照
    //┴
  } /* SS_ATTACH_SLOT() */


//========================================================
// Ｄ．レスポンス
//========================================================
  //─────────────────
  // スロットの受付資源に送信
  //─────────────────
  void SEND_CONN(
    T_SS_SLOT&    argSS, // 送信先
    const String& argMSG // 送信メッセージ
  ){
    //┬
    //○メッセージをレスポンス
    if (ctx.logLevel >= 0) { LOG_PRINT(argMSG); }
    else {
      if (argSS.connected && argSS.conn_tx != nullptr) {
        argSS.conn_tx->setValue(argMSG.c_str());
        argSS.conn_tx->notify();
      } /* END-if */
    };
    //│
    //○フレームをクリア
    ctx.strFrame = "";
    //┴
  } /* SEND_CONN() */


//========================================================
// Ｅ．プロセス部品
//========================================================
  //─────────────────
  // １．接続状態を確認
  //----------------------------------
  //【詳細】
  // イベントドリブンなので確認は不要
  //----------------------------------
  // 戻り値：接続状態（論理値）
  // ・false：接続中
  // ・true ：切断中
  //─────────────────
  bool P1_CONNECT(T_SS_SLOT&  argSS){return false;}

  //─────────────────
  // ２．フレームを取得(データ受信)
  // ※データ受信：Writeイベントから呼び出し
  //----------------------------------
  // 受信イベントが先回りして実行する。
  // そのため、P2_MAKE_FRAME()を介さない。
  // 終端文字を見つけた場合、フレームをコンテクストに反映する。
  // 受信継続の判定は、一律で不可能とする。
  //----------------------------------
  //【詳細】
  // データ受信単位  ：フレーム単位
  // 受信バッファ    ：しない
  // 受信継続判定    ：しない
  // フレーム終端判定：する
  //----------------------------------
  // 戻り値：受信継続の要否（論理値）
  // ・true ：不要
  // ・false：必要
  //─────────────────
  bool P21_RECEIVE(T_SS_SLOT& argSS){
    //┬
    //○受信データを受信バッファに加える
    argSS.rx += argSS.conn_rx;
    //│
    //○受信バッファのオーバーフローを確認
    if (argSS.rx.length() > SS_RX_SIZE) {
    //│ ＼（オーバーフローになった場合）
        //●エラーコードをレスポンス
        //▼RETURN:不可能(オーバーフローが発生)
        argSS.isOverflow = true;
        argSS.rx = "";
        SEND_CONN(argSS, "#DFL!");
        return true;
    } /* END-if */
    //│
    //○取り込み状態を確認
    if (!argSS.rx.endsWith("!")) {
    //│ ＼（終端に達していない場合）
        //●エラーコードをレスポンス
        //▼RETURN:不可能(フレームが未完成)
        SEND_CONN(argSS, "#CMD!");
        return true;
    } /* END-if */
    //│
    //●受信バッファをURI形式に変換
    //○コンテクストにフレームをセット
    P2_FORMAT_URI(argSS.rx);
    ctx.strFrame = argSS.rx;
    //│
    //▼RETURN:不可能
    argSS.rx = "";
    return true ; // フレーム完成
    //┴
  } /* P21_RECEIVE() */

  //─────────────────
  // ２．フレームを取得
  //----------------------------------
  // フレームの作成状況を判定する。
  // ※受信イベントが先回りしてP21_RECEIVE()を実行済み。
  //----------------------------------
  //【詳細】
  // フレーム化処理  ：する
  // 受信継続判定    ：しない
  //----------------------------------
  // 戻り値：フレーム作成状況（論理値）
  // ・true ：未完成
  // ・false：完成
  //─────────────────
  bool P2_MAKE_FRAME(T_SS_SLOT& argSS){
    //┬
    //○受信バッファの内容を破棄
    //▼処理継続の判定を返す
    return (ctx.strFrame == "" ? true : false);
  } /* P2_MAKE_FRAME() */

  //─────────────────
  // ３．基本情報を取得
  //----------------------------------
  //【詳細】
  // フレーム書式    ：{コマンドパス}!
  //─────────────────
  void P3_MAKE_INFO(){
    //┬
    //〇受信待ちデータの取り込み
    ctx.cmdPath = ctx.strFrame;
    //┴
  } /* P3_MAKE_INFO() */

  //─────────────────
  // ４．認証を実施
  //----------------------------------
  //【詳細】
  // 常時接続のため、認証は行わない
  //----------------------------------
  // 戻り値：論理値
  // ・false：認証に成功
  // ・true ：認証に失敗
  //─────────────────
  bool P4_AUTH(){return false;}


//========================================================
// Ｆ．ルーティング処理
//========================================================
  void routeMMP(T_SS_SLOT& argSS){
    //┬
    //○１．接続状態を確認
    if (P1_CONNECT(argSS)) return;
    //│ ＼（切断の場合）
    //│  ▼処理を中断
    //│
    //●２．フレームを取得
    if (P2_MAKE_FRAME(argSS)) return;
    //│ ＼（フレームが未完成の場合）
    //│  ▼処理を中断
    //│
    //○３．基本情報を取得
    P3_MAKE_INFO();
    //│
    //○４．認証を実施
    if (P4_AUTH()) return;
    //│ ＼（認証に失敗した場合）
    //│  ▼処理を中断
    //│
    //●５．MMPコマンドを実行
    String resMMP = P5_RUN();
    //│
    //●６．実行結果をレスポンス
    SEND_CONN(argSS, resMMP);
    //┴
  } /* routeMMP() */


//========================================================
// Ｇ．初期化・ポーリング
//========================================================
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // BLEイベントコールバック：サーバ用
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  class Callback_Server : public BLEServerCallbacks {
    //─────────────────
    // 接続イベント
    //─────────────────
    void onConnect(BLEServer* pServer) override {
      //┬
      //○接続状況を確認
      if (ssTBL[0].connected) return;
      //│ ＼（参加済みの場合）
          //▼RETURN：これ以上は参加させない
      //│
      //○アドバタイジングを停止(新規の侵入を物理的に防ぐ)
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->getAdvertising()->stop();
      //│
      //●スロットを割り当て
      SS_ATTACH_SLOT();
      //┴
    } /* onConnect() */

    //─────────────────
    // 切断イベント
    //─────────────────
    void onDisconnect(BLEServer* pServer) override {
      //┬
      //●スロットを破棄
      if (ssTBL != nullptr) SS_DETACH_SLOT(ssTBL[0]);
      //│
      //○アドバタイジングを再開
      if (devBLE::MY_SRV != nullptr) devBLE::MY_SRV->startAdvertising();
      //┴
    } /* onDisconnect() */
  }; /* Callback_Server */

  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // BLEイベントコールバック：クライアント用
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  class Callback_Client : public BLECharacteristicCallbacks {
    //─────────────────
    // 受信イベント
    //─────────────────
    void onWrite(BLECharacteristic *pCharacteristic) override {
      //┬
      //○未取り込みデータを受信（getValue()参照後は消費されない）
      ssTBL[0].conn_rx = pCharacteristic->getValue();
      if (ssTBL[0].conn_rx.length() < 1) return;
      //│ ＼（空の場合）
          //▼RETURN
      //│
      //○状態を確認
      if (!ssTBL[0].used || !ssTBL[0].connected) return;
      //│ ＼（スロットが使用不可 または 切断中の場合）
          //▼RETURN
      //│
      //●コンテクストをセットアップ
      P0_SETUP_CTX(ROUTE_ID, 0);
      //│
      //◇┐データ受信
        if (P21_RECEIVE(ssTBL[0])) {
        //├┐（フレームを取り込めた場合）
          //●スロット処理を指示
          routeMMP(ssTBL[0]);
          ctx.strFrame = "";
        //└┐（その他）
          //┴
        } /* END-if  */
    //┴
    } /* onWrite() */
  }; /* Callback_Client */

  static Callback_Server ON_CONNECTION; // 接続・切断
  static Callback_Client ON_RECIVE    ; // データ受信

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //----------------------------------
  // ロジックで明示的に呼び出す ※handle()参照
  //━━━━━━━━━━━━━━━━━
  void start() {
    //┬
    //○１．前準備の完了状態を確認
    if (!devBLE::ENABLED) devBLE::start();
    if (!devBLE::ENABLED || !devBLE::MY_SRV) {
    //│ ＼（通信デバイスが起動していない場合）
        //○エラーメッセージを表示
        //○無効化
        //▼異常終了
      Serial.println(" BLE Bridge : Bluetoothサーバが起動していません ");
      ENABLED = false;
      return;
    } /* END-if */
    //│
    //○２．サーバ資源生成
    //　➡【該当処理なし】※BLEサーバが対象
    //│
    //○３．接続情報TBLを作成
    SS_CREATE_TBL();
    //│
    //○４．ルーティング登録
    // ➡【該当処理なし】※Webサーバが対象
    //│
    //○５．サーバ開始
    devBLE::MY_SRV->setCallbacks(&ON_CONNECTION);
    if (devBLE::BLE_RX != nullptr) {
      devBLE::BLE_RX->setCallbacks(&ON_RECIVE);
    };
    //│
    //○┐６．成功終了
      //○成功メッセージ
      //○有効化
      Serial.println(" BLE Bridge : OK");
      ENABLED = true;
    //┴
  } /* start() */

  //━━━━━━━━━━━━━━━━━
  // ハンドラ入口（ポーリング入口）
  //━━━━━━━━━━━━━━━━━
  void handle() {
    if (!ENABLED) return;
    // BLEはイベント駆動のため定期ポーリングでの処理は不要
  } /* handle() */

} /* namespace adpBLE */