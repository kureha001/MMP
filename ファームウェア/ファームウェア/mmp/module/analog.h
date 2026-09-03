// filename : module/analog.h
//========================================================
// 機能モジュール：アナログ入力
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/03) 
//========================================================
//┬
//■インクルード
//┴

//########################################################
//# クラス：機能モジュール（アナログ入力）
//########################################################
class ModuleAnalog : public ModuleBase {
//--------------------------------------------------------
private:
//━━━━━━━━━━━━━━━
// ＧＰＩＯピンアサイン
//━━━━━━━━━━━━━━━
    const int g_ADDR_PINS[4] = {10, 9, 8, 7}; // アドレス・バス
    const int g_DATA_PINS[4] = { 4, 3, 2, 1}; // データ・バス

//━━━━━━━━━━━━━━━━━
// ユーザ別データ
//━━━━━━━━━━━━━━━━━
    struct UserData {
        int Values[16*4]; // チャンネル別の入力信号
        int SwitchCnt   ; // 使用範囲(スイッチ数;デバイス数)
        int PlayerCnt   ; // 使用範囲(プレイヤ数;チャンネル数)
    };
    //────────────────
    UserData* g_USR_DAT = nullptr;

//--------------------------------------------------------
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
  ModuleAnalog(MmpContext& ctx, const char* name, const char* desc)
  : ModuleBase(ctx, name, desc) {

    Serial.println(" [ANALOG：HC4067]");

    // ユーザ別データのメモリ確保
    void* p = calloc(ctx.accIDS, sizeof(UserData)); // 全要素0で初期化して確保
    if (!p) {
      Serial.println(String("　 [NG] メモリ不足です"));
      return;
    }
    g_USR_DAT = static_cast<UserData*>(p);

    // 既定設定
    for (int i = 0; i < ctx.accIDS; ++i) {
      g_USR_DAT[i].SwitchCnt = 4; // 使用範囲(スイッチ数;デバイス数)
      g_USR_DAT[i].PlayerCnt = 1; // 使用範囲(プレイヤ数;チャンネル数)
    }

    Serial.println(String("　 [OK] Device  ID : 0 ～ 3 "));
    Serial.println(String("　 [OK] Channel ID : 0 ～ 16"));
    Serial.println("");
  }

  //========================================================
  // コマンド・パーサー(実装)
  //========================================================
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

    //━━━━━━━━━━━━━━━━━
    // ユーザデータのスロットを特定
    //━━━━━━━━━━━━━━━━━
    if (!g_USR_DAT || ctx.accID < 0 || ctx.accID >= ctx.accIDS){_ResIniErr(); return;}
    UserData& SLOT = g_USR_DAT[ctx.accID];

    // ───────────────────────────────
    // 機能 : セットアップ
    // 書式 : ANALOG/SETUP
    // 引数 : ① プレイヤー数        ※IDより１つ大きい
    // 　　　 ② スイッチ数          ※IDより１つ大きい
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SETUP") == 0){

      // １．前処理
        // 1.1. 書式
        if (dat_cnt != 3){_ResChkErr(); return;}

        // 1.2. 単項目チェック
        int plCnt, swCnt;
        if (!_Str2Int(dat[1], plCnt, 1, 16) ||
            !_Str2Int(dat[2], swCnt, 1,  4) ){_ResChkErr(); return;}

      // ２．処理
      SLOT.PlayerCnt = plCnt;
      SLOT.SwitchCnt = swCnt;

      // ３．応答
      _ResOK();
      return;
    }

    // ───────────────────────────────
    // 機能 : 信号入力（入力バッファに更新）
    // 書式 : ANALOG/IN
    // 引数 : なし
    // 戻り : _ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"INPUT") == 0){

      // １．前処理：
        // 1.1. 書式
        if (dat_cnt != 1){_ResChkErr(); return;}

      // ２．処理
      for (int ch = 0; ch < SLOT.PlayerCnt; ch++) {

        // アドレスバスをセット
        for (int i = 0; i < 4; i++) {
          pinMode(g_ADDR_PINS[i], OUTPUT);
          digitalWrite(g_ADDR_PINS[i], (ch>>i) & 1);
        }

        delayMicroseconds(10); //時間調整

        // データバスから読取り
        for (int dev = 0; dev < SLOT.SwitchCnt; dev++) {
          const int pin = g_DATA_PINS[dev];
          SLOT.Values[ch*4 + dev] = analogRead(pin);
        }
      }

      // ３．後処理：
      _ResOK();
      return;
    }

    // ───────────────────────────────
    // 機能 : 入力バッファ参照
    // 書式 : ANALOG/READ
    // 引数 : ① プレイヤーID
    // 　　　 ② スイッチID
    // 戻り : _ResValue
    // ───────────────────────────────
    if (strcmp(Cmd,"READ") == 0){

      // １．前処理
        // 1.1. 書式（第3引数は任意）
        if (dat_cnt != 3){_ResChkErr(); return;}

        // 1.2. 単項目チェック
        int pl, sw;
        if (!_Str2Int(dat[1], pl, 0, SLOT.PlayerCnt - 1) ||
            !_Str2Int(dat[2], sw, 0, SLOT.SwitchCnt - 1) )
            {_ResChkErr(); return;}

        // 1.4.機能チェック
        if (pl >= SLOT.PlayerCnt || sw >= SLOT.SwitchCnt){_ResChkErr(); return;}

      // ２．処理
      const int idx = pl * 4 + sw;        // 値のデータ位置
      int res = SLOT.Values[idx];

      // ３．後処理：
      _ResValue(res);
      return;
    }

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
