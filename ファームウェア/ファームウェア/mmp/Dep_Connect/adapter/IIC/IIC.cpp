// filename : Dep_Connect/adapter/IIC/IIC.cpp
//========================================================
// 接続部門／担当：IIC（ベース）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/29)
//========================================================

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス【非同期キュー型＋接続スロット型】
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
class  AD_IIC : // 接続識別子：uint8_t
public AD_API_Queue<uint8_t>, // 非同期キュー型
public AD_API_Slot< uint8_t>  // 接続スロット型
{
private:
//========================================================
//§基本情報
//========================================================
  const int            DATA_LENGTH  = 80; // データ長制限
  static const uint8_t IIC_ADDR_MIN = 0xA0;
  static const uint8_t IIC_ADDR_MAX = 0xA4;
  String CONN_TX[IIC_ADDR_MAX - IIC_ADDR_MIN + 1]; // 返送バッファ

//========================================================
//§受信処理
//========================================================
  //───────────────────────────
  // 並列処理の内容を定義
  //───────────────────────────
  void ON_RECIVE() override final {
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐【主処理】
    //◎┐スレーブ（IICアドレス）を走査
    for (uint8_t pConn = IIC_ADDR_MIN; pConn <= IIC_ADDR_MAX; pConn++) {
      //│＼（最後のアドレスに達した場合）
      //│ ▽完了：走査を終了
      //│
      //○┐【前処理（走査単位）】
        //送受信バッファを初期化
        String qFrame = "";
        int    ID     = pConn - IIC_ADDR_MIN;
        String resMSG = (CONN_TX[ID] == "") ? RCD::OK : CONN_TX[ID];
        CONN_TX[ID]   = "";
        //┴
      //│
      //○┐受信データを取得
        //○スレーブへレスポンス
        Wire1.beginTransmission(pConn);
        Wire1.write((const uint8_t*)resMSG.c_str(),resMSG.length());
        Wire1.endTransmission(false);
        //│
        //○スレーブから受信データを取得
        Wire1.requestFrom(pConn, DATA_LENGTH);
        while (Wire1.available()) qFrame += (char)Wire1.read();
        //┴
      //│
      //○┐キュー情報を取得
        //○フレームを取得
        int idx = qFrame.indexOf('!')         ;if (idx < 0      ) continue;
        qFrame  = qFrame.substring(0, idx + 1);if (qFrame == "!") continue;
        //│＼（書式あやまりの場合）
        //│ ▽次へ：スキップ
        //┴
      //│
      //●キューを登録
      pushQueue(pConn, qFrame, 0);
      //┴
    } //～for
    //┴
  } /* ON_RECIVE() */

//========================================================
//§モード別実装のインクルード
//========================================================
#if MODE == MODE_BRIDGE
#else
  #include "IIC_MainSub.cpp"
#endif

//========================================================
//§公開機能
//========================================================
public:
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  // コンストラクタ【非同期キュー型＋接続スロット型】
  //━━━━━━━━━━━━━━━━━━━━━━━━━━━
  AD_IIC(   MmpContext& argCtx) : // 接続識別子：uint8_t
  AD_API<      uint8_t>(argCtx, AID::IIC), // 基本型
  AD_API_Queue<uint8_t>(argCtx, AID::IIC), // 非同期キュー型
  AD_API_Slot< uint8_t>(argCtx, AID::IIC)  // 接続スロット型
  {
  //┬
  //○┐【前処理】
    //┴
  //│
  //○┐主処理
    //●受信タスクを登録
    RUN_TASK(MY_AID); // 並列処理で登録
    //┴
  //│
  //○┐【後処理】
    //○メッセージ表示
      char msg[128];
      snprintf(msg, sizeof(msg), " [OK] IIC #1 / ADR.%d->%d", IIC_ADDR_MIN, IIC_ADDR_MAX);
      Log::prtln(String(msg));
  //┴┴
  } /* constractor AD_IIC() */

}; /* class AD_IIC */