// filename : Dep_Command/module/system.h
//========================================================
// コマンド部門／モジュール課：システム管理 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/11) 
//========================================================

//########################################################
//# 処理詳細
//########################################################
class ModuleSystem : public ModuleBase {
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
  using ModuleBase::ModuleBase;

  //========================================================
  // コマンド・パーサー(実装)
  //========================================================
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    // ───────────────────────────────
    // VER  : バージョン
    // 引数 : なし
    // 戻り :
    //  ・正常 [NNNN!]：メジャー1桁 マイナー1桁 リビジョン2桁
    //  ・異常 [VER!!]
    // ───────────────────────────────
    if (strcmp(Cmd,"VERSION") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(); return; }

      // ２．バージョン(文字列)を返す
      ctx.resMSG = ctx.sysVer;

      // ３．後処理：
      return;
    }

    // ───────────────────────────────
    // MODE : モードID
    // 引数 : なし
    // 戻り : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"MODE") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(); return; }

      // ２．モードIDを返す
      int res = MODE;

      // ３．後処理：
      _ResValue(res);
      return;
    }

    // ───────────────────────────────
    // BOOT : 再起動
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"BOOT") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 1){_ResChkErr(); return; }

      // ２．再起動
      _ResOK();
      ESP.restart();

      // ３．後処理：
      return;
    }

    // ───────────────────────────────
    // SET/LOG : ログ出力レベル設定
    // 引数 : ① 出力レベル {-1:なし | 0:レスポンス時 | n:処理プロセスn時 }
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/LOG") == 0){

      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt != 2){_ResChkErr(); return; }

        // 1.2.単項目チェック
        int intLv;
        if (!_Str2Int(dat[1], intLv, 0, 1)){_ResChkErr(); return;}

      // ２．出力レベルをセット
      ctx.sysLog = (intLv == 0 ? false : true);

      // ３．後処理：
      _ResOK();
      return;
    }

    // ───────────────
    // UPDATE/IIC : IICピン設定の更新＆保存
    // ───────────────
    if (strcmp(Cmd, "UPDATE/IIC") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 3) { _ResChkErr(); return; } // Cmd, SDA, SCL の3要素

      // 1.2.単項目チェック
      int sda, scl;
      if (!_Str2Int(dat[1], sda, 0, 49)){_ResChkErr(); return;}
      if (!_Str2Int(dat[2], scl, 0, 49)){_ResChkErr(); return;}

      // 1.3.相関チェック
      if(sda == scl){_ResChkErr(); return;}

      // ２．IICを再起動
      bool res = devIIC::UPDATE(sda, scl);

      // ３．後処理：
      if (res) _ResOK();
      else     _ResChkErr();
      return;
    }

#if ADP_BLE
    // ───────────────
    // UPDATE/BLE : BLEデバイス名の更新＆保存
    // ───────────────
    if (strcmp(Cmd, "UPDATE/BLE") == 0){

      // １．前処理：
        // 1.1.書式チェック
      if (dat_cnt != 2) { _ResChkErr(); return; } // Cmd, Name の2要素

        // 1.2.単項目チェック (1〜31文字)
      size_t len = strlen(dat[1]);
      if (len == 0 || len > 31) { _ResChkErr(); return; }

      // ２．BLEデバイス名を再起動
      bool res = devBLE::UPDATE(dat[1]);

      // ３．後処理：
      if (res) _ResOK();
      else     _ResChkErr();
      return;
    }
#endif

  //━━━━━━━━━━━━━━━━━
  // コマンド名エラー
  //━━━━━━━━━━━━━━━━━
  _ResNotCmd();
  return;
  }

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
// なし
};
