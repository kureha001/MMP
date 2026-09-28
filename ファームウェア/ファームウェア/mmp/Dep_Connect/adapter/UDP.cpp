// filename : Dep_Connect/adapter/UDP.cpp
//========================================================
// 接続部門／担当：UDP
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/27)
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFiUdp.h>
//┴┴

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス【非同期キュー型＋スロット型
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AD_UDP : // 接続識別子：String
public AD_API_Queue<String>,
public AD_API_Slot< String>
{
private:
//========================================================
//§基本情報
//========================================================
  WiFiUDP MY_NET        ; // クライアント・サーバ両用(実体)
  int     MY_PORT = 8083; // ポート番号

//========================================================
//§各種ヘルパ
//========================================================
  //───────────────────────────
  // 接続識別子を求める
  //───────────────────────────
  static String getConn(
    const IPAddress& argIP,
    uint16_t         argPORT
  ) {return argIP.toString() + ":" + String(argPORT);}

//========================================================
//§接続管理
//========================================================
  //───────────────────────────
  // 処理対象の接続スロットを割当てる
  //------------------------------------------------------
  // 戻り値 ：スロットID（数値）
  //───────────────────────────
  int SLOT_ASSIGN(const String& argConn) {
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //●既存スロットを求める
    int ID             = SLOT_GET_ACTIVE(argConn);
    if (ID<0) {
      // ＼（該当する既存スロットがない場合）
        //●空スロットから求める
        ID             = SLOT_GET_FREE();
        if (ID < 0) ID = SLOT_GET_OLD ();
        // ＼（該当する空スロットがない場合）
          //●古いスロットから求める
          //┴
        //│
        //●割当スロットに内容をセットする
        SLOT_SET(ID, argConn);
        //┴
    } //～if 
    //│
    //○タイムスタンプを更新する
    TBL[ID].timeStamp = millis();
    //┴
  //│
  //○┐【後処理】
    //▼返却：スロットID
    return ID;
  //┴
  } /* SLOT_ASSIGN() */

//========================================================
//§リクエスト終了
//========================================================
//──────────────────
//➡ブリッジ
//・転送受付で完了
#if (MODE == MODE_BRIDGE)
//------------------------------------
  //───────────────────────────
  // 転送依頼を受け付ける
  //───────────────────────────
  void TRANS() override final {
  //┬
  //○┐【前処理】
    //○転送先の情報を用意する
    String transIP = ctx.trans.Dat1;
    //┴
  //│
  //○┐【主処理】
    //○リクエストを転送する
    MY_NET.beginPacket(transIP.c_str(), MY_PORT);
    MY_NET.write((const uint8_t*)ctx.trans.Frame.c_str(), ctx.trans.Frame.length());
    MY_NET.endPacket();
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* TRANS() */
//──────────────────
//➡ブリッジ以外
//・接続元へのレスポンスで完了
#else
//------------------------------------
  //───────────────────────────
  // 接続元にMSGをレスポンスする
  //───────────────────────────
  void SEND_CONN(String argConn) override final {
  //┬
  //○┐【前処理】
    //●WiFiの接続状況を確認する
    if (!devWiFi::ENABLED_CONN(true)) return;
    //┴
  //│
  //○┐【主処理】
    //○接続元の宛先を求める
    int intPos = argConn.indexOf(':');
    IPAddress sendIP   ; sendIP.fromString(argConn.substring(0, intPos));
    uint16_t  sendPort = argConn.substring(intPos + 1).toInt();
    //│
    //○接続元宛にメッセージを送信する
    MY_NET.beginPacket(sendIP, sendPort);
    MY_NET.write((const uint8_t*)ctx.base.Msg.c_str(), ctx.base.Msg.length());
    MY_NET.endPacket();
    //┴
  //│
  //○┐【後処理】
    //●ログを出力する
    adpFnBase::SHOW_LOG();
  //┴┴
  } /* SEND_CONN() */
//------------------------------------
#endif //➡ブリッジ｜➡ブリッジ以外
//──────────────────

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 並列処理の内容を定義
  //───────────────────────────
  void ON_RECIVE() override final {
  //┬
  //○┐【前処理】
    //○受信内容を確認する
    int packetSize = MY_NET.parsePacket();
    if (packetSize < 1) return;
    //│＼（パケット内容が[空]の場合）
    //│ ▼終了：早期リターンする
    //┴
  //│
  //○┐【主処理】
    //○┐フレーム求める
      //●接続情報を用意する
      IPAddress qIP   = MY_NET.remoteIP()  ; // IPアドレス
      uint16_t  qPort = MY_NET.remotePort(); // ポート番号
      //│
      //●受信データを読み取る
      char pFrame[LIMIT::READ_LEN];
      int  getLen = MY_NET.read(pFrame, sizeof(pFrame) - 1);
      if (getLen < 1) return;
      //│＼（データ内容が[空]の場合）
      //│ ▼終了：早期リターンする
      //│
      //○受信データを末尾処理する
      pFrame[getLen] = '\0';
      //┴
    //│
    //○┐キュー情報を用意する
      //●接続識別子を求める
      //●スロットIDを求める
      String qCONN = getConn(qIP, qPort);
      int    qSID  = SLOT_ASSIGN(qCONN);
      //┴
    //│
    //●キューを登録する
    pushQueue(qCONN, String(pFrame), qSID);
    //┴
  //│
  //○┐【後処理】
  //┴┴
  } /* ON_RECIVE() */

//========================================================
//§ハンドルの事前処理と進行判定
//========================================================
  //───────────────────────────
  // 一般用
  //------------------------------------------------------
  //【戻り値】進行判定
  // true ：進行NG
  // false：進行OK
  //───────────────────────────
  bool SETUP_NORMAL() override final {
    //●WiFiの接続状況を確認する
    return !devWiFi::ENABLED_CONN(true);
  } /* SETUP_NORMAL() */

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ【非同期キュー型＋スロット型】
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_UDP(MmpContext& argCtx) : // 接続識別子：String
  AD_API(              argCtx, AID::UDP),
  AD_API_Queue<String>(argCtx, AID::UDP),
  AD_API_Slot< String>(argCtx, AID::UDP)
  {
  //┬
  //○┐【前処理】
    //●WiFiの接続状況を確認する
    if (!devWiFi::ENABLED_CONN(true)) return;
    //┴
  //│
//──────────────────
//➡ブリッジ
//・単一スロット
//・受信タスクが不要
#if (MODE == MODE_BRIDGE)
//------------------------------------
  //○┐【主処理】
    //○接続スロットを初期化する
    SLOTs = 1;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○サービスを開始する
    MY_NET.begin(MY_PORT);
    //┴
  //│
  //○┐【後処理】
    //○起動ログ表示のMSGを表示する
    Log::prtln(" [OK] UDP");
  //┴┴
//──────────────────
//➡ブリッジ以外
//・複数スロット
//・受信タスクが必要
#else
//------------------------------------
  //○┐【主処理】
    //○接続スロットを初期化する
    SLOTs = 10;
    TBL   = new T_SLOT[SLOTs];
    //│
    //○サービスを開始する
    MY_NET.begin(MY_PORT);
    //│
    //●データ受信のタスクを開始する
    RUN_TASK(MY_AID);
    //┴
  //│
  //○┐【後処理】
    //○起動ログ表示のMSGを表示する
    char msg[128];
    snprintf(msg, sizeof(msg), " [OK] UDP        (PORT %d)", MY_PORT);
    Log::prtln(String(msg));
  //┴┴
//------------------------------------
#endif //➡ブリッジ｜➡ブリッジ以外
//──────────────────
  } /* constractor AD_UDP() */

}; /* class AD_UDP */