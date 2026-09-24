// filename : Dep_Connect/template/_api2.h
//========================================================
// 接続部門／業務設計：抽象基底クラス（接続スロット型）
//--------------------------------------------------------
// Ver 1.4.0 (2026/09/24)
//========================================================
#ifndef CONN_ADP_API2_H
#define CONN_ADP_API2_H
#pragma once
//┬
//□┐インクルード
//┴┴

//########################################################
template <typename T>
class AdapterSlotBase :
  virtual public AdapterBase<T>
//########################################################
{
//========================================================
//§公開機能
//========================================================
public:
  using AdapterBase<T>::AdapterBase;

protected:
//========================================================
//§接続管理
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
  struct T_SLOT {
    bool          used      = false; // 使用状況
    T             CONN             ; // 接続識別子(テンプレート)
    unsigned long timeStamp = 0    ; // タイムスタンプ
  };

  int     SLOTs = 0;
  T_SLOT* TBL   = nullptr;

  //─────────────────
  // スロット内容をセット
  //─────────────────
  void SLOT_SET(
    int argSID, // スロットID
    T   argConn // 接続識別子(テンプレート)
  ) {
    //┬
    //○スロット内容をセット
    TBL[argSID].used      = true    ; // 有効性[ON]
    TBL[argSID].CONN      = argConn ; // 接続識別子を反映
    TBL[argSID].timeStamp = millis(); // タイムスタンプを更新
    //┴
  } /* SLOT_SET() */

  //─────────────────
  // スロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：正常終了（スロットID）
  // ・-1 ：該当なし
  //─────────────────
  int SLOT_GET_FREE() {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //◎┐スロットを走査
      int  ID = 0;
      for (ID = 0; ID < SLOTs; ID++) {
        //│＼（すべて走査し終えた場合）
        //│ ▽完了：走査を終了
        //│
        //○スロット状態を確認
        if (!TBL[ID].used) break;
        //│＼（該当するスロットにヒットした場合）
        //│ ▽完了：走査を終了
        //┴
      } /* for */
      //┴
    //│
    //○┐【後処理】
      //▼返却
      return (ID < SLOTs) ? ID : -1;
    //┴
  } /* SLOT_GET_FREE() */

  //─────────────────
  // 古いスロットIDを取得
  //----------------------------------
  // 戻り値：スロットID（数値型）
  // ・0～：スロットID
  //─────────────────
  int SLOT_GET_OLD() {
    //┬
    //○┐【前処理】
      //┴
    //│
    //○┐【主処理】
      //◎┐最古のスロットIDを取得
      int oldID = 0;
      unsigned long oldTime = TBL[0].timeStamp;
      for (int ID = 1; ID < SLOTs; ID++) {
        //│＼（すべて走査し終えた場合）
        //│ ▽完了：走査を終了
        //│
        //◇┐スロット状態を確認
        if (TBL[ID].timeStamp < oldTime) {
            oldID   = ID;
            oldTime = TBL[ID].timeStamp;
          //┴
        //┴
        } /* if */
      } /* for */
      //┴
    //│
    //○┐【後処理】
      //▼返却：該当なし
      return oldID;
    //┴
  } /* SLOT_GET_OLD() */

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//§受信処理
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  //─────────────────
  // タスク関数
  //─────────────────
  virtual void ON_RECIVE() = 0;

  //─────────────────
  // スレッド処理の定義（FreeRTOS用）
  //─────────────────
  static void StreamQueue(void *pvParameters) {
    auto* self = static_cast<AdapterSlotBase<T>*>(pvParameters);
    for (;;) {
      if (self) self->ON_RECIVE();        // タスク関数
      vTaskDelay(1 / portTICK_PERIOD_MS); // 短いウェイト
    } // for
  } // StreamQueue()

  //─────────────────
  // 並列処理の開始
  //─────────────────
  TaskHandle_t MY_TASK = NULL;  // タスク識別(並列処理)
  void RUN_TASK(int argAID) {
    //┬
    //○受信タスクをFreeRTOSの別スレッドとして起動（自動コア割当）
    xTaskCreate(
      StreamQueue           , // 実行するタスク関数
      String(argAID).c_str(), // タスク名（デバッグ用）
      4096                  , // スタックサイズ（バイト単位）
      this                  , // パラメータ
      2                     , // 優先度
      &MY_TASK                // タスク識別を取得
    );
    //┴
  } // RUN_TASK()

}; /* class AdapterSlotBase */
#endif