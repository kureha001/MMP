// filename : Dep_Connect/adapter/bridge/TCP.cpp
//========================================================
// 接続部門／業務課／ブリッジ係：TCP 担当
//--------------------------------------------------------
// Ver 1.3.0 (2026/09/07) 
//========================================================
//┬
//□┐インクルード
  //□Arduinoシステム
  #include <WiFi.h>
//┴┴

//########################################################
//# 処理詳細
//########################################################
namespace brdTCP {
//=====================================================
// 基本情報
//=====================================================
  bool          IS_CONNECT = false; // 接続状況
  WiFiClient    CONN              ; // クライアント(接続資源)
  String        MMP_IP     = ""   ; // MMPメインのIPアドレス
  uint16_t      MMP_PORT   = 0    ; // MMPメインのポート番号
  unsigned long TIMEOUT_MS = 2000 ; // タイムアウト(ミリ秒)

//=====================================================
// 接続する（初期化）
//=====================================================
void BEGIN(
  int      argMMP_IP4, // MMPメインのIPアドレス（第４オクテット）
  uint16_t argPort     // MMPメインのポート番号
) {

  bool isErr = false;
  if (WiFi.status() != WL_CONNECTED) isErr = true;
  if (isErr) {ctx.resMSG = "#BG1!"; return;}

  IPAddress localIP = WiFi.localIP();
  MMP_IP =
    String(localIP[0]) + "." + 
    String(localIP[1]) + "." + 
    String(localIP[2]) + "." + 
    String(argMMP_IP4);
  MMP_PORT = argPort;   
  CONN.setTimeout(TIMEOUT_MS);

  if (CONN.connect(MMP_IP.c_str(), MMP_PORT)) IS_CONNECT = true;
  else {IS_CONNECT = false; ctx.resMSG = "#BG2!";}
}

//=====================================================
// 切断する
//=====================================================
void END() {
  CONN.stop();
  IS_CONNECT = false;
}

//=====================================================
// データを送信する
//=====================================================
void SEND() {
  bool isErr = false;
  if      (!IS_CONNECT                  ) isErr = true;
  else if (WiFi.status() != WL_CONNECTED) isErr = true;
  else if (!CONN.connected()            ) isErr = true;
  if (isErr) {ctx.resMSG = "#SD1!"; return;}
  CONN.print(ctx.strFrame);
}

} /* namespace brdTCP */