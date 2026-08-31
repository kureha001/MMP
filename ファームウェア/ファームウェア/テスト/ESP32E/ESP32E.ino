#include <SPI.h>
#include <TFT_eSPI.h>

#include "devWiFi.h" // 通信機器初期化
#include "devBLE.h"  // 通信機器初期化
#include "mode.h"    // 通信モード
#include "cmd.h"     // コマンド関連

//=====================================================
// 基本情報
//=====================================================
const char* SRV_IP = "192.168.2.99";
TFT_eSPI tft = TFT_eSPI();

//=====================================================
// ボタンの構造体定義
//=====================================================
struct Button {
  int16_t      x, y, w, h;
  uint16_t    baseColor;
  uint16_t    touchColor;
  uint16_t    textColor;
  const char* label;
  uint8_t     textSize;
  bool        isPressed;
};

//----------------------------------------------------
// トグルボタンの構造体定義（選択状態を持つ）
//----------------------------------------------------
struct ToggleButton {
    int16_t x, y, w, h;
    const char* label;
    bool isSelected;
};

//=====================================================
// ＵＩ部品の定義
//=====================================================
    //----------------------------------------------------
    // 上下ボタン
    //----------------------------------------------------
    Button btnUp   = {380, 50,  80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "^", 3, false};
    Button btnDown = {380, 120, 80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "v", 3, false};
    Button* buttons[] = { &btnUp, &btnDown };
    const int numButtons = 2;

    //----------------------------------------------------
    // モード選択ボタン定義
    //----------------------------------------------------
    const int modeBTN_Y  = 38;
    const int modeBTN_H  = modeBTN_Y - 8;
    const int numModeBTN = 6;
    ToggleButton modeBTN[numModeBTN] = {
      {10,  4, 45, modeBTN_H, "UART",   true},   
      {58,  4, 42, modeBTN_H, "TCP",    false},
      {103, 4, 52, modeBTN_H, "WebSoc", false},
      {158, 4, 52, modeBTN_H, "WebAPI", false},
      {213, 4, 42, modeBTN_H, "BLE",    false},
      {258, 4, 42, modeBTN_H, "IIC",    false}
    };

    //----------------------------------------------------
    // コマンド切替ボタン
    //----------------------------------------------------
    const int numFilterBTN = 6;
    ToggleButton filterBTN[numFilterBTN] = {
      {10,  285, 55, 26, "SYSTEM",  true},   
      {70,  285, 60, 26, "DIGITAL", false},
      {135, 285, 60, 26, "ANALOG",  false},
      {200, 285, 45, 26, "PWM",     false},
      {250, 285, 45, 26, "MP3",     false},
      {300, 285, 45, 26, "IIC",     false}
    };

    // フィルタリング後のインデックスを保持する配列
    int       filteredIndices[37]; //
    int       filteredCount   = 0; //
    int       scrollIndex     = 0; //          
    const int maxVisibleRows  = 8; // 表示行数を微調整

//=====================================================
// 各種ヘルパー
//=====================================================
    //----------------------------------------------------
    // コマンドリストをフィルタリング
    //----------------------------------------------------
    void CommandFilter() {

    filteredCount = 0;
    const char* activeLabel = "SYSTEM";

    for (int i = 0; i < numFilterBTN; i++) {
      if (filterBTN[i].isSelected) {
        activeLabel = filterBTN[i].label;
        break;
      }
    }

    for (int i = 0; i < TOTAL_CMDs; i++) {
      const char* cmd = COMMAND_TBL[i];
      bool match = false;

      if (strcmp(activeLabel, "SYSTEM") == 0) {
        if (cmd[0] == '_' || strncmp(cmd, "SYS", 3)        == 0) match = true;
      } else {
        if (strncmp(cmd, activeLabel, strlen(activeLabel)) == 0) match = true;
      } /* END-if */

      if (match && filteredCount < 37) filteredIndices[filteredCount++] = i;
    } /* END-for */

    // カーソル位置を先頭にセット
    scrollIndex = 0;

 } /* CommandFilter() */

//=====================================================
// ＵＩ部品の描画
//=====================================================
    //----------------------------------------------------
    // ボタン部品（モード切替、コマンド入替）
    //----------------------------------------------------
    void Draw_BTN(Button &btn) {
      uint16_t fillColor = btn.isPressed ? btn.touchColor : btn.baseColor;
      tft.fillRect(btn.x, btn.y, btn.w, btn.h, fillColor);
      tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
      
      tft.setTextColor(btn.isPressed ? TFT_BLACK : btn.textColor, fillColor);
      tft.setTextSize(btn.textSize);
      
      int16_t charWidth  = (btn.textSize == 3) ? 18 : 12;
      int16_t charHeight = (btn.textSize == 3) ? 24 : 16;
      int len = strlen(btn.label);
      
      int16_t textX = btn.x + (btn.w - (len * charWidth)) / 2;
      int16_t textY = btn.y + (btn.h - charHeight) / 2;
      
      tft.setCursor(textX, textY);
      tft.print(btn.label);
    }

    //----------------------------------------------------
    // ボタン部品のトグル演出
    //----------------------------------------------------
    void Draw_TglBTN(ToggleButton &btn) {
      uint16_t fillColor = btn.isSelected ? TFT_WHITE : TFT_DARKGREY;
      uint16_t textColor = btn.isSelected ? TFT_BLACK : TFT_WHITE;
      
      tft.fillRect(btn.x, btn.y, btn.w, btn.h, fillColor);
      tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_LIGHTGREY);
      
      tft.setTextColor(textColor, fillColor);
      tft.setTextSize(1);
      
      int len = strlen(btn.label);
      int16_t textX = btn.x + (btn.w - (len * 6)) / 2;
      int16_t textY = btn.y + (btn.h - 8) / 2;
      
      tft.setCursor(textX, textY);
      tft.print(btn.label);
    } /* Draw_TglBTN() */

    //----------------------------------------------------
    // レスポンス部品（テキスト）
    //----------------------------------------------------
    void Draw_Response(const char* text) {
      int16_t x = 380;
      int16_t y = 200;
      int16_t w = 80;
      int16_t h = 60;

      tft.fillRect(x, y, w, h, TFT_BLACK);
      tft.drawRect(x, y, w, h, TFT_BLUE);
      
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      
      int len = strlen(text);
      int16_t textX = x + (w - (len * 12)) / 2;
      int16_t textY = y + (h - 16) / 2;
      
      tft.setCursor(textX, textY);
      tft.print(text);
    } /* Draw_Response() */

    //----------------------------------------------------
    // コマンド選択リスト部品
    //----------------------------------------------------
    void Draw_CmdList() {
      tft.fillRect(10, 45, 345, 235, TFT_BLACK);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);

      int y = 48;
      for (int i = 0; i < maxVisibleRows; i++) {
        int idx = scrollIndex + i;
        if (idx < filteredCount) {
          tft.setCursor(15, y);
          tft.print(COMMAND_TBL[filteredIndices[idx]]);
        } /* END-if */
      y += 29;
      } /* END-for */
    } /* Draw_CmdList() */

    //----------------------------------------------------
    // 選択コマンドの演出
    //----------------------------------------------------
    void Draw_TapCmd(int argRow, bool argHighlight) {

      if (argRow < 0 || argRow >= maxVisibleRows) return;
      int idx = scrollIndex + argRow;
      if (idx >= filteredCount) return;

      int y = 45 + (argRow * 29);
      
      uint16_t bg = argHighlight ? TFT_WHITE : TFT_BLACK;
      uint16_t fg = argHighlight ? TFT_BLACK : TFT_WHITE;

      tft.fillRect(10, y, 345, 27, bg);
      tft.setTextColor(fg, bg);
      tft.setTextSize(2);
      tft.setCursor(15, y + 3);
      tft.print(COMMAND_TBL[filteredIndices[idx]]);
    }

    //----------------------------------------------------
    // 接続状態表示
    //----------------------------------------------------
    void Draw_NetStat2(int argNo, bool isON, String argMsg) {
      uint16_t colBG   = (isON) ? TFT_GREEN : TFT_RED;
      uint16_t colText = (isON) ? TFT_BLACK : TFT_WHITE;
      tft.setTextSize (1                   ); // フォントサイズ
      tft.setTextColor(colText, colBG      ); // 配色
      tft.setCursor   (384,  3 + 11 * argNo); // 表示座標
      tft.print       (argMsg.c_str()      ); // メッセージ表示
    } /* Draw_NetStat2() */
    //----------------------------------------------------
    void Draw_NetStat(bool argIsConnect) {
      bool isON = argIsConnect;
      Draw_NetStat2(0, isON, " CONNECTION  ");

      isON = (WiFi.status() == WL_CONNECTED);
      Draw_NetStat2(1, isON, " WiFi Server ");

      isON = (BLE_CLIENT != nullptr && BLE_CLIENT->isConnected());
      Draw_NetStat2(2, isON, " BLE  Server ");
    } /* Draw_NetStat() */


//=====================================================
// 画面全体の描画
//=====================================================
    //----------------------------------------------------
    // 上部タイトルバーの描画
    //----------------------------------------------------
    void Draw_TOP() {
      tft.fillRect(0, 0, 480, modeBTN_Y, TFT_NAVY);
      for (int i = 0; i < numModeBTN; i++) Draw_TglBTN(modeBTN[i]);
    }

    //----------------------------------------------------
    // 通常ボタン
    //----------------------------------------------------
    void DrawUIFrame() {
      tft.fillScreen(TFT_BLACK);

      Draw_TOP();
      for (int i = 0; i < numFilterBTN; i++) Draw_TglBTN(filterBTN[i]);

      tft.drawFastVLine(360, 30, 250, TFT_DARKGREY);

      for (int i = 0; i < numButtons; i++) Draw_BTN(*buttons[i]);

      Draw_NetStat(false);
      Draw_Response("-----");
      CommandFilter();
      Draw_CmdList();
    }


//=====================================================
// タッチ用ヘルパー
//=====================================================
  //---------------------------------------------------
  // タッチ範囲を判定
  //---------------------------------------------------
  bool isTouchedInside(Button &btn, uint16_t x, uint16_t y) {
    return (x >= btn.x && x <= (btn.x + btn.w) && y >= btn.y && y <= (btn.y + btn.h));
  } /* isTouchedInside() */

  //---------------------------------------------------
  // ボタンに触れたかを確認
  //---------------------------------------------------
  bool CheckTouch(ToggleButton &btn, uint16_t x, uint16_t y) {
    return (x >= btn.x && x <= (btn.x + btn.w) && y >= btn.y && y <= (btn.y + btn.h));
  } /* CheckTouch() */


//=====================================================
// 初期化処理
//=====================================================
void setup() {

    // ログ出力用にシリアルを初期化
    Serial.begin(115200);
    delay(1000);

    // 画面を初期化
    tft.init();
    tft.setRotation(1); // 横長 (480x320)

    DrawUIFrame();

    // Wi-Fiを接続する
    int retWifi = devWiFi::START("Buffalo-G-7050","etnxhurnecbs7");

    // BLEを接続する
    int retBLE  = devBLE::START("MMP-ESP32S3");

    // BLEを接続する
    Serial.println("\n========<< READY to Start >>========\n");

    Draw_NetStat(true);

} /* setup() */


//=====================================================
// 繰返し処理
//=====================================================
void loop() {

  uint16_t    t_x = 0;
  uint16_t    t_y = 0;
  bool        isTouch   = tft.getTouch(&t_x, &t_y);
  static bool lastTouch = false;

  // 座標を補正
  if (isTouch) t_y = 320 - t_y; // Y軸反転

  // トグル状態の履歴を更新
  bool prevUpPressed   = btnUp.isPressed;
  bool prevDownPressed = btnDown.isPressed;

  // ボタンの状態更新
  for (int i = 0; i < numButtons; i++) {
    bool nextState = isTouch && isTouchedInside(*buttons[i], t_x, t_y);
    if (buttons[i]->isPressed != nextState) {
      buttons[i]->isPressed = nextState;
      Draw_BTN(*buttons[i]);
    } /* END-for */
  } /* END-for */

  // ▲ボタンのスクロール
  if (!prevUpPressed && btnUp.isPressed &&  scrollIndex > 0) {
    scrollIndex--;
    Draw_CmdList();
  } /* END-if */

  // ▼ボタンのスクロール
  if (!prevDownPressed && btnDown.isPressed && scrollIndex < filteredCount - maxVisibleRows) {
    scrollIndex++;
    Draw_CmdList();
  } /* END-if */

  // タッチ開始時を判定
  if (isTouch && !lastTouch) {

    // Ｙ軸の範囲を判定
    if (t_y >= 0 && t_y <= modeBTN_Y) {
    //→（モード切替の場合）

      // 各ボタンを走査
      for (int modeID = 0; modeID < numModeBTN; modeID++) {

        // ボタンに触れたかを確認
        if (!CheckTouch(modeBTN[modeID], t_x, t_y)) continue;

        bool prevStates[numModeBTN];

        // 各ボタンの選択状態を取得
        for (int j = 0; j < numModeBTN; j++) {
          prevStates[j] = modeBTN[j].isSelected;
        } /* END-for */

        // 今回のモードボタンを選択済みにセット
        for (int j = 0; j < numModeBTN; j++) {
          modeBTN[j].isSelected = (j == modeID);
          Draw_TglBTN(modeBTN[j]); // ボタンを選択済みで表示
        } /* END-for */

        // 接続状態を画面表示
        bool modStat = false;
        Draw_NetStat(false);
        delay(500);

        // ボタン別にアクション
        if (prevStates[modeID]) {
        // 前回選択したモードの場合
          // 終了処理を実行
          if (modeID ==0) modStat = modeUART  ::END();
          if (modeID ==1) modStat = modeTCP   ::END();
          if (modeID ==2) modStat = modeWebSoc::END();
          if (modeID ==3) modStat = modeWebAPI::END();
          if (modeID ==4) modStat = modeBLE   ::END();
          if (modeID ==5) modStat = modeIIC   ::END();

        } else {
        // 今回選択したモードの場合
          // ステータスを取得
          if (modeID ==0) modStat = modeUART  ::IS_CONNECT;
          if (modeID ==1) modStat = modeTCP   ::IS_CONNECT;
          if (modeID ==2) modStat = modeWebSoc::IS_CONNECT;
          if (modeID ==3) modStat = modeWebAPI::IS_CONNECT;
          if (modeID ==4) modStat = modeBLE   ::IS_CONNECT;
          if (modeID ==5) modStat = modeIIC   ::IS_CONNECT;
          Draw_NetStat(modStat);

          // 初期化処理を実行
          if (modeID ==0) modStat = modeUART  ::BEGIN();
          if (modeID ==1) modStat = modeTCP   ::BEGIN(8081);
          if (modeID ==2) modStat = modeWebSoc::BEGIN(8082);
          if (modeID ==3) modStat = modeWebAPI::BEGIN(8080);
          if (modeID ==4) modStat = modeBLE   ::BEGIN();
          if (modeID ==5) modStat = modeIIC   ::BEGIN();
          Draw_NetStat(modStat);
        } /* END-if（ボタン別にアクション）*/
      } /* END-for（各ボタンを走査）*/

    } else if (t_y >= 282 && t_y <= 315) {
    //→（コマンド入替の場合）

      // 各ボタンを走査
      for (int subID = 0; subID < numFilterBTN; subID++) {

        // ボタンに触れたかを確認
        if (!CheckTouch(filterBTN[subID], t_x, t_y)) continue;

        // 各ボタンの選択状態を取得
        for (int j = 0; j < numFilterBTN; j++) {
          filterBTN[j].isSelected = (j == subID);
          Draw_TglBTN(filterBTN[j]);
        } /* END-for（各ボタンの選択状態を取得）*/

        // コマンドリストを選択
        // コマンドリストを画面表示
        CommandFilter();
        Draw_CmdList();
      } /* END-for（各ボタンを走査）*/

    } else if (t_x >= 10 && t_x <= 355 && t_y >= 45 && t_y <= 270) {
    //→（コマンドリストの場合）

      // コマンド位置を特定
      int intRow = (t_y - 45) / 29;     // 画面内の行位置
      int listRow = scrollIndex + intRow; // コマンドリストの行位置
      if (intRow >= 0 && intRow < maxVisibleRows && listRow < filteredCount) {

        // コマンド位置を画面演出
        Draw_TapCmd(intRow, true);

        // コマンドを実行
        String thisCmd = COMMAND_TBL[filteredIndices[listRow]];
        String retMSG = "";
        if (modeBTN[0].isSelected) retMSG = modeUART  ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[1].isSelected) retMSG = modeTCP   ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[2].isSelected) retMSG = modeWebSoc::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[3].isSelected) retMSG = modeWebAPI::RUN(thisCmd.c_str()      );
        if (modeBTN[4].isSelected) retMSG = modeBLE   ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[5].isSelected) retMSG = modeIIC   ::RUN(thisCmd.c_str()      );

        // ログ表示する
        if (retMSG.length() == 5) {
          Serial.printf("Received Response : [%s]\n", retMSG);
        } else {
          Serial.printf("Timeout.\n");
          retMSG = "TIME";
        } /* END-if */

        delay(80);

        // レスポンスを画面表示
        // コマンド位置を画面演出
        Draw_Response(retMSG.c_str());
        Draw_TapCmd(intRow, false);
      } /* END-if（コマンド位置を特定）*/
    } /* END-if（Ｙ軸の範囲を判定 */
  } /* END-if（タッチ開始時を判定）*/

  // 最終タッチを更新
  lastTouch = isTouch;
  delay(30);

} /* loop() */