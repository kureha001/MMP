// filename : connection/member/_API0_.h
//========================================================
// 経路アダプタAPI：基本型
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#ifndef ADAPTER_BASE_H
#define ADAPTER_BASE_H
#pragma once

//========================================================
// 経路アダプタの抽象基底クラス
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
  //┴
//┴
};

#endif // ADAPTER_BASE_H