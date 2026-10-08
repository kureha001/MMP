// filename : Dep_Contact/template/apiQueue.h
//========================================================
// 接客部門／操作手順書(抽象クラス)：非同期キュー型
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
#pragma once

//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// クラス：基本型を継承
//━━━━━━━━━━━━━━━━━━━━━━━━━━━━
template <typename T>
class          AD_API_Queue:
virtual public AD_API<T>
{
  public:
    using AD_API<T>::AD_API;

  // 非同期キューの機能を追加する
  #include "apiAddQueue.h"

}; /* class AD_API_Queue */