// filename : Dep_Command/module/MP3_DFPlayerMini.h
//========================================================
// コマンド部門／モジュール課：MP3プレイヤー 担当
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/15)
// ・UARTポートの見直し 
//========================================================
//┬
//■┐インクルード
  //■サードパーティ
  #include <DFRobotDFPlayerMini.h>  // デバイス固有
//┴┴

//########################################################
//# 処理詳細
//########################################################
class ModuleMP3 : public ModuleBase {
//--------------------------------------------------------
private:
//━━━━━━━━━━━━━━━━━
// デバイス情報
//━━━━━━━━━━━━━━━━━
    bool IS_LOOP = true; // 自動リピート（初期値ON）
    int  WAIT_MS = 500 ; // 待ち時間(ms)

    static const int SER_MAX   = 2; // 将来拡張できる上限
    static const int SER_CNT   = 1; // 現在用意できている個数
    int              SER_START = 2; // Serial2を使うため(将来拡張予定)

    DFRobotDFPlayerMini MP3[SER_MAX]; // コンテナ
    HardwareSerial*     SER[SER_MAX]; // MP3プレイヤに割り当てるシリアルデバイス
    int  PIN_RX[SER_MAX] = {11, 0}  ; // シリアルデバイスのピン（RX） 
    int  PIN_TX[SER_MAX] = {12, 0}  ; // シリアルデバイスのピン（TX）
    bool ENABLE[SER_MAX] = {false, false}; // MP3プレイヤの有効性

//--------------------------------------------------------
public:
  //━━━━━━━━━━━━━━━━━
  // モジュール(抽象基底クラス)
  //━━━━━━━━━━━━━━━━━
  ModuleMP3(MmpContext& ctx, const char* name, const char* desc)
  : ModuleBase(ctx, name, desc) {
    //┬
    //○開始
    Log::prtln(" [MP3：DFPlayer mini]");
    //│
    //◎┐初期設定
    for (int ID = 0; ID < SER_MAX; ++ID) {
      //│
      //○シリアルボートを用意
      if (ID == SER_CNT) break;
      SER[ID] = new HardwareSerial(ID + SER_START); 
      SER[ID]->begin(9600, SERIAL_8N1, PIN_RX[ID], PIN_TX[ID]);
      //│
      //○MP3プレイヤーを生成
      if (MP3[ID].begin(*SER[ID])) ENABLE[ID] = true;
      //│
      //○結果表示
      char msg[128];
      snprintf(msg, sizeof(msg), 
        "　 [%s] Device  ID : %d (Serial%d)",
        (ENABLE[ID] ? "OK" : "NG"), ID, (ID + SER_START)
      );
      Log::prtln(String(msg));
    } /* END-for */
    //│
    //○終了
    Log::prtln("");
    //┴
  }; /* Constractor ModuleMP3 */

  //========================================================
  // コマンド・パーサー(実装)
  //========================================================
  void handle(char dat[][ DAT_LENGTH ], int dat_cnt) override {

    //━━━━━━━━━━━━━━━━━
    // 前処理
    //━━━━━━━━━━━━━━━━━
    const char* Cmd = _Remove1st(dat[0]); // コマンド名を補正

  //━━━━━━━━━━━━━━━━━
  // 対象トラックの制御
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：全体トラックID または ＭＰ３フォルダID で再生
    // 書式：MP3/{PLAY|PLAY3}:<機器番号0～1>!
    // 戻値：トラック状態CD
    // ───────────────────────────────
    if (strcmp(Cmd,"PLAY") == 0 || strcmp(Cmd,"PLAY3" ) == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 3) {_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

        // 1.2. 単項目チェック
        int folder, track;
        if (!_Str2Int(dat[2], track,  0, 255) ){_ResChkErr(); return;}

      // ２．コマンド実行
      if (strcmp(Cmd,"PLAY" ) == 0) MP3[idx].play(track);
      if (strcmp(Cmd,"PLAY3") == 0) MP3[idx].playMp3Folder(track);

      // ３．後処理：
      reTrackkState(idx);
      return;
    } /* MP3/{PLAY|PLAY3} */

    // ───────────────────────────────
    // 機能：指定フォルダ内トラックを再生開始
    // 書式：MP3/PLAYF:<機器番号0～1>!
    // 戻値：トラック状態CD
    // ───────────────────────────────
    if (strcmp(Cmd,"PLAYF") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 4) {_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

        // 1.2. 単項目チェック
        int folder, track;
        if (!_Str2Int(dat[2], folder, 0, 255) ||
            !_Str2Int(dat[3], track,  0, 255) ){_ResChkErr(); return;}

      // ２．コマンド実行
      MP3[idx].playFolder(folder,track);

      // ３．後処理：
      reTrackkState(idx);
      return;
    } /* PLAYF */

    // ───────────────────────────────
    // 機能：次曲を再生／前曲を再生／停止／一時停止／開始
    // 書式：MP3/{NEXT|PREV|STOP|PAUSE|START}:<機器番号0～1>!
    // 戻値：トラック状態CD
    // ───────────────────────────────
    if (
      strcmp(Cmd,"NEXT" ) == 0 || strcmp(Cmd,"PREV" ) == 0 ||
      strcmp(Cmd,"STOP" ) == 0 || strcmp(Cmd,"PAUSE") == 0 ||
      strcmp(Cmd,"START") == 0
    ){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 2){_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

      // ２．コマンド実行
      if (strcmp(Cmd,"NEXT" ) == 0) MP3[idx].next();
      if (strcmp(Cmd,"PREV" ) == 0) MP3[idx].previous();
      if (strcmp(Cmd,"STOP" ) == 0) MP3[idx].stop();
      if (strcmp(Cmd,"PAUSE") == 0) MP3[idx].pause();
      if (strcmp(Cmd,"START") == 0) MP3[idx].start();
            
      // ３．後処理：
      reTrackkState(idx);
      return;
    } /* NEXT|PREV|STOP|PAUSE|START */
    
  //━━━━━━━━━━━━━━━━━
  // デバイスの設定
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：ループ再生を設定
    // 書式：MP3/SET/LOOP:<機器番号0～1>:<設定 1(ON),0(OFF)>!
    // 戻値：トラック状態CD
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/LOOP") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt < 3){_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

        // 1.3. 単項目チェック
        int loop; if (!_Str2Int(dat[2], loop, 0, 1)){_ResChkErr(); return;}

      // ２．コマンド実行
      if (loop == 1) MP3[idx].enableLoop();
      else           MP3[idx].disableLoop();

      // ３．後処理：
      reTrackkState(idx);
      return;
    } /* SET/LOOP */
    
    // ───────────────────────────────
    // 機能：音量を設定
    // 書式：MP3/SET/VOLUME:<機器番号0～1>:<音量0～30>!
    // 制限 : とくになし
    // 戻値：_ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/VOLUME") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<3){_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

        // 1.3. 単項目チェック
        int v; if (!_Str2Int(dat[2], v, 0, 30)){_ResChkErr(); return;}

      // ２．コマンド実行
      MP3[idx].volume(v);

      // ３．後処理：
      _ResOK();
      return;
    } /* SET/VOLUME */
    
    // ───────────────────────────────
    // 機能：イコライザーを設定
    // 書式：MP3/SET/EQ:<機器番号0～1>:<タイプ0～5>!
    //        0: Normal, 1: Pop, 2: Rock,3: Jazz, 4: Classic, 5: Bass
    // 制限 : とくになし
    // 戻値：_ResOK
    // ───────────────────────────────
    if (strcmp(Cmd,"SET/EQ") == 0){
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt<3){_ResChkErr(); return;}

        // 1.2. 対象外チェック
        int idx;
        if (!checkDev(dat[1], idx)) return;

        // 1.3. 単項目チェック
        int mode; if (!_Str2Int(dat[2], mode, 0, 5)){_ResChkErr(); return;}

      // ２．コマンド実行
      MP3[idx].EQ(mode);

      // ３．後処理：
      _ResOK();
      return;
    } /* SET/EQ */
    
  //━━━━━━━━━━━━━━━━━
  // インフォメーション
  //━━━━━━━━━━━━━━━━━
    // ───────────────────────────────
    // 機能：機器の状態
    // 書式：X:<機器番号1〜4>!
    // 制限 : とくになし
    // 戻値："xyzz!" x:メジャー／y:マイナー／zz:リビジョン
    // ───────────────────────────────
    if (strcmp(Cmd,"INFO/CONNECT") == 0){
      // １．前処理
        // 1.1. 書式
        if (dat_cnt != 2){_ResChkErr(); return;}

        // 1.2. 単項目チェック
        int dev;
        if (!_Str2Int(dat[1], dev, 1, 2)){_ResChkErr(); return;}

      // ２．コマンド実行
      int res = ENABLE[dev - 1];

      // ３．後処理：
      _ResValue(res);
      return;
    } /* INFO/CONNECT */

    // ───────────────────────────────
    // 機能：各種状態取得
    // 書式：MP3/INFO/{TRACK|VOLUME|FILEID|FILES}:<機器番号0～1>!
    // 制限 : とくになし
    // 戻値：_ResValue
    // ───────────────────────────────
    if (
      strcmp(Cmd,"INFO/TRACK" ) == 0 ||
      strcmp(Cmd,"INFO/VOLUME") == 0 ||
      strcmp(Cmd,"INFO/EQ"    ) == 0 ||
      strcmp(Cmd,"INFO/FILEID") == 0 ||
      strcmp(Cmd,"INFO/FILES" ) == 0 )
    {
      // １．前処理：
        // 1.1.書式チェック
        if (dat_cnt != 2){_ResChkErr(); return;}

        // 1.3. 対象外チェック
        int idx = 0;
        if (!checkDev(dat[1], idx)) return;

        // ２．コマンド実行 ※エラーならリトライ
        int res = -1;
        for (int tries = 0; tries < 50 && res == -1; ++tries) {
          if      (strcmp(Cmd,"INFO/TRACK" ) == 0){res = MP3[idx].readState()            ;res = MP3[idx].readState()            ;} 
          else if (strcmp(Cmd,"INFO/VOLUME") == 0){res = MP3[idx].readVolume()           ;res = MP3[idx].readVolume()           ;}
          else if (strcmp(Cmd,"INFO/EQ"    ) == 0){res = MP3[idx].readEQ()               ;res = MP3[idx].readEQ()               ;}
          else if (strcmp(Cmd,"INFO/FILEID") == 0){res = MP3[idx].readCurrentFileNumber();res = MP3[idx].readCurrentFileNumber();}
          else if (strcmp(Cmd,"INFO/FILES" ) == 0){res = MP3[idx].readFileCounts()       ;res = MP3[idx].readFileCounts()       ;}
          if (res != -1) break;
        }

        // ３．後処理：
        _ResValue(res);
        return;
    } /* INFO/{TRACK|VOLUME|FILEID|FILES} */

    //━━━━━━━━━━━━━━━━━
    // コマンド名エラー
    //━━━━━━━━━━━━━━━━━
    _ResNotCmd();
    return;
  } /* handle() */

//━━━━━━━━━━━━━━━━━
// 内部ヘルパー
//━━━━━━━━━━━━━━━━━
  // ───────────────
  // デバイス違反チェッカー
  // ───────────────
  bool checkDev(const char* s, int idx){
    // 単項目チェック
    int dev;
    if (!_Str2Int(s, dev, 1, 2)){_ResChkErr(); return false;}

    // 対象外チェック
    idx = dev - 1;
    if (!ENABLE[idx]){_ResDevErr(); return false;}

    // 後処理
    return true;
  } /* checkDev() */

  // ───────────────
  // 対象トラック状況
  // ───────────────
  void reTrackkState(int idx){
    // ※エラーならリトライ
    int res = -1;
    for (int tries = 0; tries < 50 && res == -1; ++tries) {
      res = MP3[idx].readState();
      if (res != -1) break;
    } /* END-for*/
    _ResValue(res);
  } /* reTrackkState() */

}; /* class ModuleMP3 */