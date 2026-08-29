#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();

// Wi-Fi設定
const char* ssid     = "Buffalo-G-7050";
const char* password = "etnxhurnecbs7";

// 送信先TCPサーバー設定 (192.168.2.99:8081)
const char* targetIP = "192.168.2.99";
const uint16_t targetPort = 8081;

// TCP接続管理用クライアント
WiFiClient tcpClient;
bool isTcpConnected = false;

// ボタンの構造体定義
struct Button {
  int16_t x, y, w, h;
  uint16_t baseColor;
  uint16_t touchColor;
  uint16_t textColor;
  const char* label;
  uint8_t textSize;
  bool isPressed;
};

// トグルボタンの構造体定義（選択状態を持つ）
struct ToggleButton {
  int16_t x, y, w, h;
  const char* label;
  bool isSelected;
};

// 右側コントロールパネルのボタン定義
Button btnUp    = {380, 50,  80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "^", 3, false};
Button btnDown  = {380, 120, 80, 55, TFT_LIGHTGREY, TFT_DARKGREY, TFT_BLACK, "v", 3, false};

Button* buttons[] = { &btnUp, &btnDown };
const int numButtons = 2;

// タイトル部（上部バー y:0~30）に配置する6つのトグルボタン定義
ToggleButton toggleButtons[6] = {
  {10,  4, 45, 22, "UART",   true},   
  {58,  4, 42, 22, "TCP",    false},
  {103, 4, 52, 22, "WebSoc", false},
  {158, 4, 52, 22, "WebAPI", false},
  {213, 4, 42, 22, "BLE",    false},
  {258, 4, 42, 22, "IIC",    false}
};
const int numToggles = 6;

// 21件のコマンドリスト
const char* commandList[21] = {
  "_START_!",
  "SYS/VERSION!",
  "SYS/SET_LOG:0!",
  "SYS/SET_LOG:1!",
  "SYS/BOOT!",
  "DIGITAL/OUTPUT:17:1!",
  "DIGITAL/OUTPUT:17:0!",
  "PWM/OUTPUT:0:100!",
  "PWM/OUTPUT:0:600!",
  "MP3/TRACK/PLAY:1:1:1!",
  "MP3/TRACK/PLAY:1:1:2!",
  "MP3/TRACK/PLAY:1:1:3!",
  "MP3/TRACK/PLAY:1:1:101!",
  "MP3/TRACK/PLAY:1:1:202!",
  "MP3/TRACK/PLAY:1:2:1!",
  "MP3/TRACK/PLAY:1:2:2!",
  "MP3/TRACK/PLAY:1:2:3!",
  "MP3/TRACK/PLAY:1:3:1!",
  "MP3/TRACK/PLAY:1:3:2!",
  "MP3/TRACK/PLAY:1:3:3!",
  "MP3/TRACK/STOP:1!"
};
const int totalCommands = 21;
int scrollIndex = 0;             
const int maxVisibleRows = 8;    // フォントサイズ2（中間：高さ16px）で約8行収まるように調整

void drawButton(Button &btn) {
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

// トグルボタンを描画する関数
void drawToggleButton(ToggleButton &btn) {
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

// レスポンス表示エリア
void drawResponseArea(const char* text) {
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

// 下部ステータスバー更新
void updateBottomStatusBar(const char* cmdStr) {
  tft.fillRect(0, 290, 480, 30, TFT_DARKGREY);
  tft.setTextSize(1);

  const char* protoName = "UART";
  for (int i = 0; i < numToggles; i++) {
    if (toggleButtons[i].isSelected) {
      protoName = toggleButtons[i].label;
      break;
    }
  }

  tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
  tft.setCursor(10, 301);
  
  char fullReqBuf[90];
  snprintf(fullReqBuf, sizeof(fullReqBuf), "REQ: %s://%s:%d/%s", protoName, targetIP, targetPort, cmdStr);
  tft.print(fullReqBuf);
}

// リスト表示エリアを描画（フォントサイズ2：中間サイズを使用、行間29px）
void drawCommandList() {
  tft.fillRect(10, 45, 345, 235, TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  int y = 48;
  for (int i = 0; i < maxVisibleRows; i++) {
    int idx = scrollIndex + i;
    if (idx < totalCommands) {
      tft.setCursor(15, y);
      tft.print(commandList[idx]);
    }
    y += 29;
  }
}

// 指定した行をハイライト描画する関数
void highlightCommandLine(int clickedRow, bool highlight) {
  if (clickedRow < 0 || clickedRow >= maxVisibleRows) return;
  int idx = scrollIndex + clickedRow;
  if (idx >= totalCommands) return;

  int y = 45 + (clickedRow * 29);
  
  uint16_t bg = highlight ? TFT_WHITE : TFT_BLACK;
  uint16_t fg = highlight ? TFT_BLACK : TFT_WHITE;

  tft.fillRect(10, y, 345, 27, bg);
  tft.setTextColor(fg, bg);
  tft.setTextSize(2);
  tft.setCursor(15, y + 3);
  tft.print(commandList[idx]);
}

// 接続状態表示
void updateStatusDisplay(bool isOnline, bool connectedFlag) {
  tft.fillRect(300, 4, 175, 22, TFT_NAVY);

  uint16_t connBgColor = connectedFlag ? TFT_GREEN : TFT_DARKGREY;
  uint16_t connTextColor = connectedFlag ? TFT_BLACK : TFT_WHITE;
  
  const char* connStr = connectedFlag ? "CONNECT" : "DISCONECT";
  int connW = strlen(connStr) * 6 + 6; 
  int connX = 410 - connW;             
  
  tft.fillRect(connX, 4, connW, 22, connBgColor);
  tft.setTextColor(connTextColor, connBgColor);
  tft.setTextSize(1);
  tft.setCursor(connX + 3, 10);
  tft.print(connStr);

  uint16_t onlineBgColor = isOnline ? TFT_GREEN : TFT_RED;
  uint16_t onlineTextColor = isOnline ? TFT_BLACK : TFT_WHITE;
  
  tft.fillRect(415, 4, 58, 22, onlineBgColor);
  tft.setTextColor(onlineTextColor, onlineBgColor);
  tft.setTextSize(1);
  tft.setCursor(425, 10);
  if (isOnline) {
    tft.print("ONLINE");
  } else {
    tft.print("OFFLIN");
  }
}

// 上部タイトルバーの描画
void drawTitleBar() {
  tft.fillRect(0, 0, 480, 30, TFT_NAVY);
  
  for (int i = 0; i < numToggles; i++) {
    drawToggleButton(toggleButtons[i]);
  }
}

void drawUIFrame() {
  tft.fillScreen(TFT_BLACK);

  drawTitleBar();
  updateStatusDisplay(false, false);
  updateBottomStatusBar("-");

  tft.drawFastVLine(360, 30, 260, TFT_DARKGREY);

  for (int i = 0; i < numButtons; i++) {
    drawButton(*buttons[i]);
  }

  drawResponseArea("OK...");
  drawCommandList();
}

bool isTouchedInside(Button &btn, uint16_t x, uint16_t y) {
  return (x >= btn.x && x <= (btn.x + btn.w) && y >= btn.y && y <= (btn.y + btn.h));
}

bool isTouchedInsideToggle(ToggleButton &btn, uint16_t x, uint16_t y) {
  return (x >= btn.x && x <= (btn.x + btn.w) && y >= btn.y && y <= (btn.y + btn.h));
}

// TCP接続を試みる関数
void connectTcpServer() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.printf("Connecting to TCP server %s:%d...\n", targetIP, targetPort);
  tcpClient.setTimeout(2000);
  if (tcpClient.connect(targetIP, targetPort)) {
    isTcpConnected = true;
    Serial.println("TCP connected successfully.");
    updateStatusDisplay(true, true);
  } else {
    isTcpConnected = false;
    Serial.println("TCP connection failed.");
    updateStatusDisplay(true, false);
  }
}

// 指定したコマンドを送信し、5バイトのレスポンスを受信して画面に表示する関数
void sendCommandViaTcp(const char* cmdStr, int clickedRow) {
  highlightCommandLine(clickedRow, true);
  updateBottomStatusBar(cmdStr);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    drawResponseArea("ERR");
    delay(100);
    highlightCommandLine(clickedRow, false);
    return;
  }

  // ★修正：TCPプロトコル（toggleButtons[1]）が選択されている場合のみ自動接続・送信を行なう
  if (toggleButtons[1].isSelected) {
    if (!isTcpConnected || !tcpClient.connected()) {
      connectTcpServer();
    }

    if (isTcpConnected && tcpClient.connected()) {
      Serial.printf("Sending command via TCP: %s\n", cmdStr);
      tcpClient.print(cmdStr);

      uint8_t rxBuffer[6] = {0};
      unsigned long startTime = millis();
      int bytesRead = 0;

      while (bytesRead < 5 && (millis() - startTime) < 2000) {
        while (tcpClient.available() && bytesRead < 5) {
          rxBuffer[bytesRead++] = tcpClient.read();
        }
        delay(10);
      }

      if (bytesRead == 5) {
        rxBuffer[5] = '\0';
        Serial.printf("Received 5 bytes: %s\n", rxBuffer);
        drawResponseArea((char*)rxBuffer);
      } else {
        Serial.printf("Timeout. Read %d/5 bytes\n", bytesRead);
        drawResponseArea("TIME");
      }
    } else {
      drawResponseArea("FAIL");
    }
  } else {
    // TCP以外のモード（UART, WebSoc, WebAPI, BLE, IIC）のときはTCP通信を行わずダミー表示
    Serial.printf("Command tapped on %s mode: %s\n", toggleButtons[1].isSelected ? "TCP" : "Other", cmdStr);
    drawResponseArea("OK");
  }

  delay(80);
  highlightCommandLine(clickedRow, false);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(1); // 横長 (480x320)

  drawUIFrame();

  // Wi-Fi 接続開始
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);

  int connTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && connTimeout < 40) {
    delay(500);
    Serial.print(".");
    connTimeout++;
  }
  Serial.println("");

  bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed.");
  }

  updateStatusDisplay(connected, false);
}

void loop() {
  uint16_t t_x = 0, t_y = 0;
  static bool lastTouched = false;
  bool touched = tft.getTouch(&t_x, &t_y);

  if (touched) {
    t_y = 320 - t_y; // Y軸反転
  }

  bool prevUpPressed = btnUp.isPressed;
  bool prevDownPressed = btnDown.isPressed;

  // ボタンの状態更新
  for (int i = 0; i < numButtons; i++) {
    bool nextState = touched && isTouchedInside(*buttons[i], t_x, t_y);
    if (buttons[i]->isPressed != nextState) {
      buttons[i]->isPressed = nextState;
      drawButton(*buttons[i]);
    }
  }

  // ▲ボタンのスクロール
  if (!prevUpPressed && btnUp.isPressed) {
    if (scrollIndex > 0) {
      scrollIndex--;
      drawCommandList();
    }
  }

  // ▼ボタンのスクロール
  if (!prevDownPressed && btnDown.isPressed) {
    if (scrollIndex < totalCommands - maxVisibleRows) {
      scrollIndex++;
      drawCommandList();
    }
  }

  // タッチ開始時の判定
  if (touched && !lastTouched) {
    if (t_y >= 0 && t_y <= 30) {
      for (int i = 0; i < numToggles; i++) {
        if (isTouchedInsideToggle(toggleButtons[i], t_x, t_y)) {
          bool wasTcpSelected = toggleButtons[1].isSelected;
          
          for (int j = 0; j < numToggles; j++) {
            toggleButtons[j].isSelected = (j == i);
            drawToggleButton(toggleButtons[j]);
          }
          Serial.printf("Protocol selected: %s\n", toggleButtons[i].label);

          if (i == 1 && !wasTcpSelected) {
            if (WiFi.status() == WL_CONNECTED) {
              connectTcpServer();
            }
          } 
          else if (wasTcpSelected) {
            if (isTcpConnected) {
              tcpClient.stop();
              isTcpConnected = false;
              bool wifiOk = (WiFi.status() == WL_CONNECTED);
              updateStatusDisplay(wifiOk, false);
            }
          }
          break;
        }
      }
    }
    else if (t_x >= 10 && t_x <= 355 && t_y >= 45 && t_y <= 270) {
      int clickedRow = (t_y - 45) / 29;
      int targetIdx = scrollIndex + clickedRow;
      if (clickedRow >= 0 && clickedRow < maxVisibleRows && targetIdx < totalCommands) {
        Serial.printf("Command list tapped: index %d -> %s\n", targetIdx, commandList[targetIdx]);
        sendCommandViaTcp(commandList[targetIdx], clickedRow);
      }
    }
  }
  lastTouched = touched;

  delay(30);
}