// filename : Dep_Connect/adapter/_template_/_API0_.h
//========================================================
// 接続部門／業務課／作業標準：抽象基底クラス（基本型）
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/14)
// ・[handle_Begin()]を[AdapterQueueBase]から移動
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
    //□ポーリングの前処理
    virtual bool handle_Begin() {return false;}
    //│
    //□実行ハンドル
    virtual void handle() = 0;
    //│
    //□ポーリングの後処理
    virtual void handle_End() {}
    //│
    //□転送受付（未登録はコンテクストにエラーCDをセット）
    virtual void trans() {ctx.strFrame = "#BR0!";};
  //┴
//┴
};

#endif