// filename : Dep_Product/manager.cpp
//========================================================
//  製造部門：部門長
//--------------------------------------------------------
// Ver 1.4.0 (2026/10/01)
//========================================================
//┬
//□┐インクルード(機能モジュール群)
  //□Arduinoシステム
  #include <vector> // 登録コンテナが使用
//┴
//┬
//□┐ 製造部門
  //□業務設計：抽象基底クラス
  #define  DAT_LENGTH 20      // トークン最大長（未定義時のフォールバック）
  #include "template/api.h"   // ModuleBase
  //│
  //□担当：機能モジュール
  #include "module/system.h"  // システム管理
//──────────────────
//➡メイン
#if MODE == MODE_MAIN
//------------------------------------
  #include "module/analog.h"  // アナログ入力
  #include "module/digital.h" // デジタル入出力
  #include "module/pwm.h"     // PWM出力
  #include "module/IIC.h"     // IIC通信
  #include "module/MP3.h"     // MP3プレイヤー
//------------------------------------
#endif //➡メイン
//──────────────────
//┴┴

namespace DepProduct {
//========================================================
// 非公開機能
//========================================================
  //─────────────────
  // 基本情報
  //─────────────────
    //┬
    //□制限事項
    #define DAT_COUNT      10 // コマンド＋引数の個数
    #define REQUEST_LENGTH 96 // リクエスト全体のバッファ長
    //│
    //□担当名簿
    std::vector<ModuleBase*> MODULE;
    //┴

//========================================================
// 公開機能
//========================================================
  //━━━━━━━━━━━━━━━━━
  //（１）本部の始業指示に応じる
  // → 部下を招集・待機
  //━━━━━━━━━━━━━━━━━
  void INIT(){
    //┬
    //○動作モードを確認
    //│＼（動作モードがメイン以外場合）
    //│ ▼終了：早期リターン
    //│
    //○始業のあいさつ（開始）
    Log::prtln("<<機能モジュールの初期化>>");
    //│
    //○担当を招集
    MODULE.push_back(new ModuleSystem (ctx, "SYS"    , "System Management"   ));
//──────────────────
//➡メイン
#if MODE == MODE_MAIN
//------------------------------------
    MODULE.push_back(new ModuleAnalog (ctx, "ANALOG" , "Analog Input"        ));
    MODULE.push_back(new ModuleDigital(ctx, "DIGITAL", "Digital Input/Output"));
    MODULE.push_back(new ModulePwm    (ctx, "PWM"    , "PWM Output"          ));
    MODULE.push_back(new ModuleIIC    (ctx, "IIC"    , "IIC Read/Write"      ));
    MODULE.push_back(new ModuleMP3    (ctx, "MP3"    , "MP3 Player"          ));
//------------------------------------
#endif //➡メイン
//──────────────────
    //│
    //◎┐担当を点呼
    Log::prt(" Add In ->");
    for (auto* mod : MODULE){
      //│＼（全機能モジュールを走査し終えた場合）
      //│ ▼完了：走査を終える
      //│
      //●機能モジュール名を表示
      Log::prt(String(" [") + String(mod->getModName()) + String("]"));
      //┴
    } //～for
    //│
    //○始業のあいさつ（終了）
    Log::prt("\n\n");
    //┴
  } /* INIT() */

  //━━━━━━━━━━━━━━━━━
  //（２）接続部門のコマンド実行指示に応じる
  // → 部下に業務遂行を指示
  //━━━━━━━━━━━━━━━━━
  void HANDLE(){
    //┬
    //①┐コマンドパスを整形
    char pPath[ REQUEST_LENGTH ];
    {
      //◇超過分を削除
      size_t pLen = ctx.base.CmdPath.length();
      if (pLen >= sizeof(pPath)) pLen = sizeof(pPath) - 1;
      memcpy(pPath, ctx.base.CmdPath.c_str(), pLen);
      pPath[pLen] = '\0';
      //│
      //◇末尾'!'を除去
      pLen = strlen(pPath);
      if (pLen > 0 && pPath[pLen-1] == '!') pPath[pLen-1] = '\0';
      //┴
    }   /* ① */
    //│
    //②┐コマンドパラメータを取得
    char dat[ DAT_COUNT ][ DAT_LENGTH ]; // 登録バッファ（コマンド、引数１...引数n）
    int  regCount = 0                  ; // 登録数（コマンド名＋引数）
    {
      //○先頭のトークンを取得
      char* tok = strtok(pPath, ":");
      //│
      //◎┐トークン毎を登録バッファに登録
      while (tok && regCount < DAT_COUNT){
        //│＼（データ数の上限を超えた場合）
        //│ ▼ループ処理を中断
        //│
        //○当該トークンを登録バッファに登録
        strncpy(dat[regCount], tok, sizeof(dat[0])-1);
        dat[regCount][sizeof(dat[0])-1] = '\0';
        //│
        //○登録数をカウントアップ
        regCount++;
        //│
        //○次のトークンを取得
        tok = strtok(nullptr, ":");
      } //～while
        //┴
      //│
      //○エラーメッセージを返却
      if (regCount == 0)
      {ctx.base.Result = RCD::NotCmd; return;}
        // ＼（登録数がゼロの場合）
          //○処理結果にエラーCDをセットする
          //▼終了：早期リターン
      //┴
    }   /* ② */
    //│
    //③┐機能モジュール機能を実行
      //○処理結果を初期化
      ctx.base.Result = "";
      //│
      //◎┐モジュールを走査
      for (auto* m : MODULE){
        //│＼（全機能モジュールを走査し終えた場合）
        //│ ▼ループ処理を中断
        //│
        //◇┐当該モジュールを実行
        if (m->owns(dat[0])){
          //├→(コマンド所有者の場合)
            //○機能モジュールを実行
            m->handle(dat, regCount);          
            //│
            //▼実行結果をリターン
            return;
        } //～if
          //└┐（その他）
      //┴┴　┴
      } //～for
    //│
    //○処理結果にエラーCDをセットする
    ctx.base.Result = RCD::NotMod;
    //┴
  } /* HANDLE() */

}; /* namespace Command */