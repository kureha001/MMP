// filename : Dep_Connect/adapter/_template_/_API0_.h
//========================================================
// 接続部門／業務課／作業標準：抽象基底クラス（基本型）
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/09) 
//========================================================
#ifndef CONN_ADP_API0_H
#define CONN_ADP_API0_H
#pragma once

//========================================================
// 作業標準：抽象基底クラス（基本型）
//========================================================
class AdapterBase {
protected:
  //┬
  //□┐メンバ
    //□コンテクスト(参照)
    MmpContext& ctx; // コンテクスト(参照)
    //┴

public:
  //┬
  //○コンストラクタ
  AdapterBase(MmpContext& argCtx): ctx(argCtx){}
  //┴
  //┬
  //○デストラクタ
  virtual ~AdapterBase(){}
  //┴
  
  //┬
  //□┐共通インタフェイス
    //│
    //□実行ハンドル
    virtual void handle() = 0;
    //│
    //□転送受付（未登録はエラーをレスポンス）
    virtual void trans() {Serial.print("#TRS!");};
    //│
    //□レスポンス（ブリッジ用）
    virtual void SEND_CONN_BRIDGE() {
      Serial.print(ctx.resMSG);
      adpFnBase::SHOW_LOG();
    };
  //┴
//┴
};

#endif