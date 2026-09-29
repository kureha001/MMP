// filename : Dep_Connect/adapter/UDP/UDP_MainSub.cpp
//========================================================
// 接続部門／担当：UDP（メインモード・サブモード）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/29)
//========================================================

//========================================================
//§開始処理・終了処理
//========================================================
  //───────────────────────────
  // 開始処理：コンストラクタ
  //───────────────────────────
  String CONSTRACT() override final {
  //┬
  //○┐【前処理】
    //┴
  //│
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
    //▼：返却：起動ログ表示のMSG
    char msg[100];
    snprintf(msg, sizeof(msg), " [OK] UDP        (PORT %d)", MY_PORT);
    return String(msg);
  //┴
  } /* CONSTRACT() */

  //───────────────────────────
  // 終了処理：接続元にレスポンスMSGを送信する
  //───────────────────────────
  void SEND_MSG(String argConn) override final {
  //┬
  //○┐【前処理】
    //●初期化の健全性を確認する
    if (SETUP_NORMAL()) return;
    //│＼（問題がある場合）
    //│ ▼終了：早期リターンする
    //│
    //○転送先の情報を用意する
    int intPos = argConn.indexOf(':');
    IPAddress ip; ip.fromString(argConn.substring(0, intPos));
    uint16_t  port = argConn.substring(intPos + 1).toInt();
    //┴
  //│
  //○┐【主処理】
    //○接続元にレスポンスMSGを送信する
    MY_NET.beginPacket(ip, port);
    MY_NET.write((const uint8_t*)ctx.base.Msg.c_str(), ctx.base.Msg.length());
    MY_NET.endPacket();
    //┴
  //│
  //○┐【後処理】
    //●ログを出力する
    adpFnBase::SHOW_LOG();
  //┴┴
  } /* SEND_MSG() */