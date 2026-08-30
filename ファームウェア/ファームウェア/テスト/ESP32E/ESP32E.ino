#include <SPI.h>
#include <TFT_eSPI.h>

#include "iniWiFi.h" // WiFi初期化
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

//----------------------------------------------------
// 右側コントロールパネルのボタン定義
//----------------------------------------------------
Button btnUp    = {380, 50,  80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "^", 3, false};
Button btnDown  = {380, 120, 80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "v", 3, false};

Button* buttons[] = { &btnUp, &btnDown };
const int numButtons = 2;

//----------------------------------------------------
// タイトル部（上部バー y:0~30）に配置する6つのトグルボタン定義
//----------------------------------------------------
ToggleButton modeBTN[6] = {
    {10,  4, 45, 22, "UART",   true},   
    {58,  4, 42, 22, "TCP",    false},
    {103, 4, 52, 22, "WebSoc", false},
    {158, 4, 52, 22, "WebAPI", false},
    {213, 4, 42, 22, "BLE",    false},
    {258, 4, 42, 22, "IIC",    false}
};
const int numToggles = 6;

//----------------------------------------------------
// 画面下部に配置する6つのサブカテゴリ切替ボタン定義
//----------------------------------------------------
ToggleButton subModeBTN[6] = {
    {10,  285, 55, 26, "SYSTEM",  true},   
    {70,  285, 60, 26, "DIGITAL", false},
    {135, 285, 60, 26, "ANALOG",  false},
    {200, 285, 45, 26, "PWM",     false},
    {250, 285, 45, 26, "MP3",     false},
    {300, 285, 45, 26, "IIC",     false}
};
const int numSubToggles = 6;

// フィルタリング後のインデックスを保持する配列
int filteredIndices[37];
int filteredCount = 0;

int scrollIndex = 0;             
const int maxVisibleRows = 7;    // 下部ボタン配置に伴い表示行数を微調整

//----------------------------------------------------
// 選択中のサブモードに応じてコマンドをフィルタリングする関数
//----------------------------------------------------
void UpdateFilteredCommands() {
  filteredCount = 0;
  const char* activeLabel = "SYSTEM";
  for (int i = 0; i < numSubToggles; i++) {
    if (subModeBTN[i].isSelected) {
      activeLabel = subModeBTN[i].label;
      break;
    }
  }

  for (int i = 0; i < TOTAL_CMDs; i++) {
    const char* cmd = COMMAND_TBL[i];
    bool match = false;

    if (strcmp(activeLabel, "SYSTEM") == 0) {
      if (cmd[0] == '_' || strncmp(cmd, "SYS", 3) == 0) {
        match = true;
      }
    } else {
      if (strncmp(cmd, activeLabel, strlen(activeLabel)) == 0) {
        match = true;
      }
    }

    if (match) {
      if (filteredCount < 37) {
        filteredIndices[filteredCount++] = i;
      }
    }
  }
  scrollIndex = 0;
}

//----------------------------------------------------
//----------------------------------------------------
void Draw_BTN(Button &btn) {
  uint16_t fillColor = btn.isPressed ? btn.touchColor : btn.baseColor;
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, fillColor);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
  
  tft.setTextColor(btn.isPressed ? TFT_BLACK : btn.textColor, fillColor);
  tft.setTextSize(btn.textSize);
  
  int16_t charWidth = (btn.textSize == 3) ? 18 : 12;
  int16_t charHeight = (btn.textSize == 3) ? 24 : 16;
  int len = strlen(btn.label);
  
  int16_t textX = btn.x + (btn.w - (len * charWidth)) / 2;
  int16_t textY = btn.y + (btn.h - charHeight) / 2;
  
  tft.setCursor(textX, textY);
  tft.print(btn.label);
}

//----------------------------------------------------
// トグルボタンを描画する関数
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
}

//----------------------------------------------------
// レスポンス表示エリア
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
}

//----------------------------------------------------
// リスト表示エリアを描画
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
    }
    y += 29;
  }
}

//----------------------------------------------------
// 指定した行をハイライト描画する関数
//----------------------------------------------------
void Draw_TapCmd(int clickedRow, bool highlight) {
  if (clickedRow < 0 || clickedRow >= maxVisibleRows) return;
  int idx = scrollIndex + clickedRow;
  if (idx >= filteredCount) return;

  int y = 45 + (clickedRow * 29);
  
  uint16_t bg = highlight ? TFT_WHITE : TFT_BLACK;
  uint16_t fg = highlight ? TFT_BLACK : TFT_WHITE;

  tft.fillRect(10, y, 345, 27, bg);
  tft.setTextColor(fg, bg);
  tft.setTextSize(2);
  tft.setCursor(15, y + 3);
  tft.print(COMMAND_TBL[filteredIndices[idx]]);
}

//----------------------------------------------------
// 接続状態表示
//----------------------------------------------------
void Draw_NetStatus(bool isWiFi, bool argIsConnect) {
  tft.fillRect(300, 4, 175, 22, TFT_NAVY);

  uint16_t connBgColor   = argIsConnect ? TFT_GREEN : TFT_RED;
  uint16_t connTextColor = argIsConnect ? TFT_BLACK : TFT_WHITE;
  
  const char* connStr = argIsConnect ? "CONNECT" : "       ";
  int connW = strlen(connStr) * 6 + 6; 
  int connX = 410 - connW;             
  
  tft.fillRect(connX, 4, connW, 22, connBgColor);
  tft.setTextColor(connTextColor, connBgColor);
  tft.setTextSize(1);
  tft.setCursor(connX + 3, 10);
  tft.print(connStr);

  uint16_t onlineBgColor   = isWiFi ? TFT_GREEN : TFT_RED;
  uint16_t onlineTextColor = isWiFi ? TFT_BLACK : TFT_WHITE;
  
  tft.fillRect(415, 4, 58, 22, onlineBgColor);
  tft.setTextColor(onlineTextColor, onlineBgColor);
  tft.setTextSize(1);
  tft.setCursor(425, 10);

  tft.print( isWiFi ? "WiFi ok" : "WiFi ng");

} /* Draw_NetStatus() */

//----------------------------------------------------
// 上部タイトルバーの描画
//----------------------------------------------------
void Draw_TOP() {
  tft.fillRect(0, 0, 480, 30, TFT_NAVY);
  
  for (int i = 0; i < numToggles; i++) {
    Draw_TglBTN(modeBTN[i]);
  }
}

  //----------------------------------------------------
  // 通常ボタン
  //----------------------------------------------------
  void DrawUIFrame() {
    tft.fillScreen(TFT_BLACK);

    Draw_TOP();
    
    // 下部サブモードボタンの描画
    for (int i = 0; i < numSubToggles; i++) {
      Draw_TglBTN(subModeBTN[i]);
    }

    Draw_NetStatus(false, false);

    tft.drawFastVLine(360, 30, 250, TFT_DARKGREY);

    for (int i = 0; i < numButtons; i++) Draw_BTN(*buttons[i]);

    Draw_Response("OK...");
    UpdateFilteredCommands();
    Draw_CmdList();
  }

//=====================================================
// 初期化処理
//=====================================================
  //---------------------------------------------------
  // 通常ボタン
  //---------------------------------------------------
  bool isTouchedInside(Button &btn, uint16_t x, uint16_t y) {
    return (x >= btn.x && x <= (btn.x + btn.w) && y >= btn.y && y <= (btn.y + btn.h));
  } /* isTouchedInside() */

  //---------------------------------------------------
  // トグルボタン
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

    // ログ出力用にシリアルを初期化
    tft.init();
    tft.setRotation(1); // 横長 (480x320)

    DrawUIFrame();

    // Wi-Fiを接続する
    Draw_NetStatus(INIT_WiFi(), false);
} /* setup() */


//=====================================================
// 繰返し処理
//=====================================================
void loop() {

  uint16_t t_x = 0, t_y = 0;
  static bool lastTouched = false;
  bool touched = tft.getTouch(&t_x, &t_y);

  if (touched) t_y = 320 - t_y; // Y軸反転

  bool prevUpPressed   = btnUp.isPressed;
  bool prevDownPressed = btnDown.isPressed;

  // ボタンの状態更新
  for (int i = 0; i < numButtons; i++) {
    bool nextState = touched && isTouchedInside(*buttons[i], t_x, t_y);
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

  // タッチ開始時の判定
  if (touched && !lastTouched) {
    if (t_y >= 0 && t_y <= 30) {
      for (int modeID = 0; modeID < numToggles; modeID++) {

        if (CheckTouch(modeBTN[modeID], t_x, t_y)) {

          bool prevStates[numToggles];

          // 画面演出
          for (int j = 0; j < numToggles; j++) {
            prevStates[j] = modeBTN[j].isSelected;
          } /* END-for */

          // 画面演出
          for (int j = 0; j < numToggles; j++) {
            modeBTN[j].isSelected = (j == modeID);
            Draw_TglBTN(modeBTN[j]);
          } /* END-for */

          // モード切替時のアクション
          bool modStat = false;
          Draw_NetStatus((WiFi.status() == WL_CONNECTED), modStat);

          if (prevStates[modeID]) {
            if (modeID ==0) modStat = modeUART  ::END();
            if (modeID ==1) modStat = modeTCP   ::END();
            if (modeID ==2) modStat = modeWebSoc::END();
            if (modeID ==3) modStat = modeWebAPI::END();
            if (modeID ==4) modStat = modeBLE   ::END();
            if (modeID ==5) modStat = modeIIC   ::END();
          } else {
            if (modeID ==0) modStat = modeUART  ::IS_CONNECT;
            if (modeID ==1) modStat = modeTCP   ::IS_CONNECT;
            if (modeID ==2) modStat = modeWebSoc::IS_CONNECT;
            if (modeID ==3) modStat = modeWebAPI::IS_CONNECT;
            if (modeID ==4) modStat = modeBLE   ::IS_CONNECT;
            if (modeID ==5) modStat = modeIIC   ::IS_CONNECT;
            Draw_NetStatus((WiFi.status() == WL_CONNECTED), modStat);

            if (modeID ==0) modStat = modeUART  ::BEGIN();
            if (modeID ==1) modStat = modeTCP   ::BEGIN(8081);
            if (modeID ==2) modStat = modeWebSoc::BEGIN(8082);
            if (modeID ==3) modStat = modeWebAPI::BEGIN(8080);
            if (modeID ==4) modStat = modeBLE   ::BEGIN("MMP-ESP32S3");
            if (modeID ==5) modStat = modeIIC   ::BEGIN();
          } /* END-if */

          Draw_NetStatus((WiFi.status() == WL_CONNECTED), modStat);

          break;
        } /* END-if */
      } /* END-for */

    } else if (t_y >= 282 && t_y <= 315) {
      // 下部サブモードボタンのタッチ判定
      for (int subID = 0; subID < numSubToggles; subID++) {
        if (CheckTouch(subModeBTN[subID], t_x, t_y)) {
          for (int j = 0; j < numSubToggles; j++) {
            subModeBTN[j].isSelected = (j == subID);
            Draw_TglBTN(subModeBTN[j]);
          }
          UpdateFilteredCommands();
          Draw_CmdList();
          break;
        } /* END-if */
      } /* END-for */

    } else if (t_x >= 10 && t_x <= 355 && t_y >= 45 && t_y <= 270) {

      int clickedRow = (t_y - 45) / 29;
      int targetIdx = scrollIndex + clickedRow;

      if (clickedRow >= 0 && clickedRow < maxVisibleRows && targetIdx < filteredCount) {

        // 画面演出
        Draw_TapCmd(clickedRow, true);

        // モード切替時のトリガー
        String thisCmd = COMMAND_TBL[filteredIndices[targetIdx]];
        String retMSG = "";
        if (modeBTN[0].isSelected) retMSG = modeUART  ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[1].isSelected) retMSG = modeTCP   ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[2].isSelected) retMSG = modeWebSoc::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[3].isSelected) retMSG = modeWebAPI::RUN(thisCmd.c_str()      );
        if (modeBTN[4].isSelected) retMSG = modeBLE   ::RUN(thisCmd.c_str(), 2000);
        if (modeBTN[5].isSelected) retMSG = modeIIC   ::RUN(thisCmd.c_str()      );

        // 取得結果を確認する
        if (retMSG.length() == 5) {
            Serial.printf("Received 5 bytes: %s\n", retMSG);
        } else {
            Serial.printf("Timeout.\n");
            retMSG = "TIME";
        } /* END-if */

        delay(80);

        // 画面演出
        Draw_Response(retMSG.c_str());
        Draw_TapCmd(clickedRow, false);
      } /* END-if */
    } /* END-if */
  } /* END-if */

  lastTouched = touched;
  delay(30);

} /* loop() */