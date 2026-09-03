// filename : adapter/_API_.h
#pragma once

class AdapterBase {
protected:
  MmpContext& ctx; // コンテクスト(参照)

public:
  AdapterBase(MmpContext& argCtx): ctx(argCtx) {}
  virtual ~AdapterBase() {}
  
  // アクションメソッド
  virtual void handle() = 0;
};