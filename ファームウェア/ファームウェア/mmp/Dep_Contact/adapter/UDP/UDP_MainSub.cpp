// filename : Dep_Contact/adapter/UDP/UDP_MainSub.cpp
//========================================================
// 接客部門／担当(通信アダプタ)：UDP（メイン／サブ）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/30)
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
    //▼返却：正常終了（起動ログMSG[OK]）
    char msg[100];
    snprintf(msg, sizeof(msg), " [OK] UDP        (PORT %d)", MY_PORT);
    return String(msg);
  //┴
  } /* CONSTRACT() */

  //───────────────────────────
  // 終了処理：接続元に処理結果を送信する
  //───────────────────────────
  void SEND_RESULT(String argConn) override final {
  //┬
  //○┐【前処理】
    //●初期化の健全性を確認する
    if (SETUP()) return;
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
    //○接続元に処理結果を送信する
    MY_NET.beginPacket(ip, port);
    MY_NET.write((const uint8_t*)ctx.base.Result.c_str(), ctx.base.Result.length());
    MY_NET.endPacket();
    //┴
  //│
  //○┐【後処理】
    //●コンテクスト・ログを出力する
    adpFnBase::LOG_CTX();
  //┴┴
  } /* SEND_RESULT() */