// filename : devNet.cpp
//========================================================
// 通信デバイス初期化：ＷｉＦｉ
//--------------------------------------------------------
//【目的】
// ＷｉＦｉサーバを初期化する
//--------------------------------------------------------
//【公開資源】
//・ENABLE ：このデバイスの有効性
//・START()：WiFiサーバを起動する
//--------------------------------------------------------
//【処理機能】
//・設定ファイルに応じたモードでWiFiサーバーを起動する
//・どのモードでも起動できない場合、緊急ＡＰモードで起動する
//・初期化の状況をシリアルに表示する
//--------------------------------------------------------
//【WiFiの設定ファイルの格納方法】
//  1. プロジェクトフォルダに/data/config.json を置く
//  2. Arduino IDE で[Ctrl][Shift][P]を同時押し
//  3. [Upload LittleFS to Pico/ESP8266/ESP32]を実行
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/23) 
//========================================================
#pragma once
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WiFi.h>
  #include <LittleFS.h>
  #include <ArduinoJson.h>
  //│
  //■ＭＭＰシステム
  //┴
//┴

//########################################################
//# 専用名の前空間
//########################################################
namespace devNetwork {
//========================================================
// 基本情報
//========================================================
  bool ENABLED = false; // 有効判定：有効：true、無効：false

//========================================================
// 共通資源
//========================================================
  //─────────────────
  // 設定ファイル
  //─────────────────
  constexpr int g_MAX_ITEM_HOST   = 4;  // アイテム登録数：ホスト情報
  constexpr int g_MAX_ITEM_ROUTER = 6;  // アイテム登録数：Wifiルーター情報
  String        g_FILE_PATH = "/config.json";  // SSID接続待ち時間ms(間隔)

  //─────────────────
  // 接続条件
  //─────────────────
  IPAddress g_IP = IPAddress(192,168,99,3); // APモードのデフォルトIP
  constexpr int g_WAIT      = 15000;        // SSID接続待ち時間ms
  constexpr int g_WAIT_INT  = g_WAIT / 10;  // SSID接続待ち時間ms(間隔)
  constexpr int g_WAIT_DIS  = 500 ;         // 切断後の待ち時間ms


//━━━━━━━━━━━━━━━━━
// 設定ファイル情報
//━━━━━━━━━━━━━━━━━
  //─────────────────
  // 型：ホスト（JSON: host[]）
  //─────────────────
  struct typeHost {
    String type;  // "sta" | "ap"
    String name;  // hostname
    String ip;    // STA:末尾オクテット or "", AP:フルIP
  };
  //─────────────────
  //  型：Wi-Fi候補（JSON: wifi[]）
  //─────────────────
  struct typeRouter {
    String label;
    String ssid;
    String pass;
    bool   isDefault = false;  // JSONの "default"
  };

  //─────────────────
  //  型：接続情報
  //─────────────────
  struct typeConnect {
    typeHost    hostList[g_MAX_ITEM_HOST];
    typeRouter  candList[g_MAX_ITEM_ROUTER];
    int         hostNum = 0;
    int         candNum = 0;
  };

//━━━━━━━━━━━━━━━
// グローバル変数
//━━━━━━━━━━━━━━━
typeConnect g_WIFI;


//========================================================
// ヘルパ
//========================================================
  //─────────────────
  // host 配列からtype一致のものを取得
  // （"sta" / "ap"）
  //─────────────────
  static const typeHost* GET_HOST(
    const char* argMode  // "ap" または "sta"
  ) {
    for (int i=0; i<g_WIFI.hostNum; ++i)
      if (g_WIFI.hostList[i].type == argMode) return &g_WIFI.hostList[i];
    return nullptr;
  } /* typeHost() */

  //─────────────────
  // "254" → 末尾オクテット
  // 空や不正は false
  //─────────────────
  static bool IS_OCTET(
    const String& argBase, // 評価対象
    uint8_t&      argVal   // デフォルト値をセット→正常時はargBaseで上書き
  ) {
    if (argBase.length()==0) return false               ; //空文字チェック
    for (char c: argBase) if (c<'0'||c>'9') return false; //○数字チェック
    long v = strtol(argBase.c_str(), nullptr, 10)       ; //○10進数に変換
    if (v<0 || v>254) return false                      ; //○数値範囲チェック
    argVal = (uint8_t)v;
    return true;
  } /* IS_OCTET() */


//========================================================
// 設定ファイル
//========================================================
  //━━━━━━━━━━━━━━━
  // 設定ファイル読込
  //----------------------------------
  //【戻り値】読込結果（論理値）
  //・true ：読込に成功
  //・false：読込に失敗
  //━━━━━━━━━━━━━━━
  bool READ_JSON() {
    // jsonファイル読取を開始
    File f = LittleFS.open(g_FILE_PATH, "r");
    if (!f) return false;

    // ファイルサイズに応じた余裕ある容量で
    size_t sz  = f.size();
    size_t cap = sz + 1024;
    if (cap < 4096 ) cap = 4096;
    if (cap > 16384) cap = 16384;

    DynamicJsonDocument doc(cap);
    DeserializationError iniErr = deserializeJson(doc, f);
    f.close();
    if (iniErr) { return false; }

    // 情報読取：サーバー
    //g_SRV_TCP.maxClients  = doc["server"]["max_clients"   ] | 4;
    //g_SRV_TCP.writeLock   = doc["server"]["write_lock"    ] | false;
    //g_SRV_TCP.writeLockMs = doc["server"]["write_lock_ms" ] | 30000;

    // 情報読取：ホスト
    g_WIFI.hostNum = 0;
    if (doc["host"].is<JsonArray>()) {
      for (JsonObject c : doc["host"].as<JsonArray>()) {
        // 上限チェック：配列長から算出
        if (g_WIFI.hostNum >= (int)(sizeof(g_WIFI.hostList)/sizeof(g_WIFI.hostList[0]))) break;

        // バリデーション
        String type = String((const char*)(c["type"] | ""));
        String name = String((const char*)(c["name"] | ""));
        String ip   = String((const char*)(c["ip"  ] | ""));

        // 追加：前後の空白を除去
        type.trim();
        name.trim();
        ip.trim();

        // ここでバリデーション
        if (!(type.equalsIgnoreCase("sta") || type.equalsIgnoreCase("ap"))) continue;
        if (!name.length()) continue;
        if (type.equalsIgnoreCase("sta")) {
            if (ip.length() > 0) {
              bool digits = true;
              for (size_t i = 0; i < ip.length(); ++i) {
                if (!isDigit((unsigned char)ip[i])) {digits = false; break;}
              }
              if (digits) {
                long v = strtol(ip.c_str(), nullptr, 10);
                if (v < 0 || v > 255) ip = "";
              } else ip = "";
            }
          }

        // 代入
        auto& h = g_WIFI.hostList[g_WIFI.hostNum];
        h.type  = type;
        h.name  = name;
        h.ip    = ip;
        g_WIFI.hostNum++;
      }
    }

    // 情報読取：Wi-Fi候補
    g_WIFI.candNum = 0;
    if (doc["wifi"].is<JsonArray>()) {
      for (JsonObject c : doc["wifi"].as<JsonArray>()) {
        if (g_WIFI.candNum >= (int)(sizeof(g_WIFI.candList)/sizeof(g_WIFI.candList[0]))) break;
        auto& w  = g_WIFI.candList[g_WIFI.candNum];
        w.label     = String((const char*)(c["label" ] | ""));
        w.ssid      = String((const char*)(c["ssid"  ] | ""));
        w.pass      = String((const char*)(c["pass"  ] | ""));
        w.isDefault = (bool)(c["default"] | false);  // "default" → isDefault
        if (w.ssid.length()) g_WIFI.candNum++;
      }
    }

    // 正常でリターン
    return true;
  } /* READ_JSON() */


//========================================================
// IPアドレスを作成
//========================================================
    //─────────────────
    // ＳＴＡモード用
    //----------------------------------
    //・DHCPで得たIPアドレス[第1〜3オクテット]+指定の第4オクテット
    //・どの方法でも作成できない場合は[0.0.0.0]
    //─────────────────
    static IPAddress GET_IP_STA(
      const IPAddress&  argIP,  // DHCP発行のIPアドレス
      const String&     argOct4 // 置き換えたい第4オクテット値(空の場合あり)
    ) {
      //┬
      //○ワーク変数を用意
      uint8_t oct4 = 0; // 失敗した場合のデフォルト値
      //│
      //◇┐ＩＰアドレスを作成
      if (IS_OCTET(argOct4, oct4)) {
        //├┐（引数が単一オクテットの場合）
          //▼返却:第4オクテットで置換
        return IPAddress(argIP[0], argIP[1], argIP[2], oct4);
        //└┐（その他）
          //┴
      } /* END-if */
      //│
      //▼返却：エラー時は[0.0.0.0]
      return IPAddress();
    } /* GET_IP_STA() */

    //─────────────────
    // ＡＰモード用
    //----------------------------------
    //・フル書式[x.x.x.x]ならそのまま
    //・上記でなければ第4オクテットを差替[x.x.x.0]
    //─────────────────
    static IPAddress GET_IP_AP(
        const String& argIP // ホストリストのIPアドレス または 第4オクテット
    ) {
      //┬
      //○ワーク変数を用意
      IPAddress ip;       // フル書式チェック用
      uint8_t   oct4 = 0; // 失敗した場合のデフォルト値
      //│
      //◇┐ＩＰアドレスを作成
      if (ip.fromString(argIP)) {
        //├┐（フル表記["x.x.x.x"]の場合）
          //▼返却:引数そのまま
          return ip;
        //│
      } else if (IS_OCTET(argIP, oct4)) {
        //├┐（末尾だけの場合）
          //▼返却:デフォルトを第4オクテットで置換
          return IPAddress(g_IP[0], g_IP[1], g_IP[2],oct4);
        //└┐（その他）
          //┴
        } /* END-if */
      //│
      //▼返却:エラー時は[0.0.0.0]
      return g_IP;
    } /* GET_IP_AP() */


//========================================================
// Wifi接続を実施
//========================================================
  void RUN_INFO(String pSSID, String pName, String pIP) {
    Serial.println(String("      [OK] SSID: ") + pSSID.c_str());
    Serial.println(String("      [OK] HOST: ") + pName.c_str());
    Serial.println(String("      [OK] IP  : ") + pIP.c_str()  );
  }

  //─────────────────
  // ＳＴＡモード用
  //----------------------------------
  // DHCP → 必要なら静的IPへ再接続
  // 要件：hList.ipが空ならDHCPのまま採用
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //─────────────────
  static bool RUN_STA(String pLabel, String pSSID, String pPass)
  {
    //┬
    if (pSSID.isEmpty()) return false;
    //│
    //○┐事前準備
      //○STA ホスト情報を JSON から取得（無ければデフォルト）
      const typeHost* hList = GET_HOST("sta");
      String pName  = hList ? hList->name : String("MMP_STA");
      String oct4   = hList ? hList->ip   : String("");  // 第4オクテット or 空
      //┴
    //│
    //○ヘッダ表示(ラベル名、SSID)
    Serial.print(String("    ") + pLabel.c_str() + String(" / ") + pSSID.c_str() + String(" "));
    //│
    //○┐仮接続
      //○切断して少し待つ
      WiFi.disconnect(false, false); delay(g_WAIT_DIS);
      //│
      //○パラメータをセット
      WiFi.mode(WIFI_STA);             // STAモード
      WiFi.setHostname(pName.c_str()); // ホスト名
      //│
      //○WiFiサーバを起動（DHCP）
      WiFi.begin(pSSID.c_str(), pPass.c_str()); // SSID,パスワード
      //│
      //○接続を確認
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < g_WAIT){Serial.print("."); delay(g_WAIT_INT);}
      if    (WiFi.status() != WL_CONNECTED) {
      //│＼（しばらく待っても接続できない場合）
          //○接続を切断
          //▼返却:接続に失敗
          Serial.println(" [NG] DHCP");
          return false;
      } /* END-if */
      //┴
    //│
    //○┐本接続の準備
      //○DHCP情報を退避
      IPAddress dhcpIP     = WiFi.localIP();    // IPアドレス
      IPAddress gatewayIP  = WiFi.gatewayIP();  // ゲートウェイアドレス
      IPAddress subnetMask = WiFi.subnetMask(); // サブネットマスク
      IPAddress dnsIP1     = WiFi.dnsIP(0);     // DNSサーバ１
      IPAddress dnsIP2     = WiFi.dnsIP(1);     // DNSサーバ２
      //│
      //○第4オクテット(JSON)を確認
      if (oct4.length() == 0) {
      //│ ＼（指定がない場合）
          //○正常処理を表示
          //●ステータスを表示
          //▼返却：接続に成功(DHCPのまま採用)
          Serial.println(" [OK] useing DHCP-IP(1)]");
          RUN_INFO(pSSID, pName, WiFi.localIP().toString());
          return true;
      } /* END-if */
      //│
      //○静的IPを取得(DHCP発行のIPアドレスの第4オクテットを変更)
      IPAddress newIP = GET_IP_STA(dhcpIP, oct4);
      if (!newIP) {
      //│＼（取得できない場合）
          //○正常処理を表示
          //●ステータスを表示
          //▼返却：接続に成功(DHCPのまま採用)
          Serial.println(" [OK] useing DHCP-IP(2)");
          RUN_INFO(pSSID, pName, WiFi.localIP().toString());
          return true;
      } /* END-if */
      //│
      //○サブネットは固定 /24
      // GW は DHCP 優先・無ければ x.y.z.1、DNS 未取得なら GW
      subnetMask = IPAddress(255,255,255,0);
      if (!gatewayIP) gatewayIP = IPAddress(newIP[0], newIP[1], newIP[2], 1);
      if (!dnsIP1   ) dnsIP1 = gatewayIP;
      //┴
    //│
    //○┐本接続
      //○切断して少し待つ
      WiFi.disconnect(false, false); delay(g_WAIT_DIS);
      //│
      //○WiFiサーバを起動（静的IP）
      WiFi.config(newIP, gatewayIP, subnetMask, dnsIP1, dnsIP2);
      WiFi.begin(pSSID.c_str(), pPass.c_str());
      //│
    //○接続を確認
      t0 = millis();
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < g_WAIT){Serial.print("."); delay(g_WAIT_INT);}
      if    (WiFi.status() != WL_CONNECTED) {
      //│ ＼（しばらく待っても接続できない場合）
          //○エラーを表示
          //▼返却:接続に成功
          Serial.println(" [NG] STA-IP");
          return false;
      } /* END-if */
      //┴
    //│
    //○接続情報を表示
    Serial.println(" Connected.");
    RUN_INFO(pSSID, pName, WiFi.localIP().toString());
    //│
    //▼返却:接続成功
    return true;
  } /* RUN_STA() */

  //─────────────────
  // ＡＰモード用
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //─────────────────
  bool RUN_AP(String pSSID, String pName,IPAddress pIP) {
    //┬
    //○切断して少し待つ
    WiFi.disconnect(false, false); delay(g_WAIT_DIS);
    //│
    //○パラメータをセット
    WiFi.mode(WIFI_AP);                                    // APモード
    WiFi.setHostname(pName.c_str());                       // ホスト名(JSON)
    WiFi.softAPConfig(pIP, pIP, IPAddress(255,255,255,0)); // SSID,パスワードなし
    //│
    //○WiFiサーバを起動（AP）
    if (!WiFi.softAP(pSSID.c_str())) {
    //│ ＼（起動に失敗した場合）
        //○エラーを表示
        //▼返却:起動に失敗
        Serial.println("     [NG] softAP");
        return false;
    } /* END-if*/
    //│
    //○接続情報を表示
    RUN_INFO(pSSID, pName, WiFi.softAPIP().toString());
    //│
    //▼返却:接続成功
    return true;
  }


//========================================================
// プロセス
//========================================================
  //─────────────────
  // P1.設定ファイル読込
  //----------------------------------
  //【戻り値】読込結果（論理値）
  //・true ：読込に成功
  //・false：読込に失敗
  //─────────────────
  bool P1_ReadConfig(){
    if (!LittleFS.begin(true)        ){Serial.println("     [NG] 初期化に失敗"  );return false;}
    if (!LittleFS.exists(g_FILE_PATH)){Serial.println("     [NG] ファイルが無い");return false;}
    if (!READ_JSON()                 ){Serial.println("     [NG] 読込に失敗"    );return false;}
    return true;
} /* P1_ReadConfig() */

  
  //─────────────────
  // P2-1.Wifi起動(STAモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //─────────────────
  bool P21_MODE_STA(){
    //┬
    //◎┐WiFi情報の候補を順に試行
    bool isRun = false;
    for (int i=0; i < g_WIFI.candNum && !isRun; i++){
      //│＼（[SSIDリストの最後に達した]または[起動できた]の場合）
      //│ ▽完了：走査終了
      //│
      //●WiFiサーバを起動
      String pLabel = g_WIFI.candList[i].label.c_str();
      String pSSID  = g_WIFI.candList[i].ssid.c_str();
      String pPass  = g_WIFI.candList[i].pass.c_str();
      isRun = RUN_STA(pLabel, pSSID, pPass);
      //┴
    } /* END-for */
    //│
    //▼RETRUN:成功でリターン
    return isRun;
  } /* P21_MODE_STA() */

  //─────────────────
  // P2-2.Wifi起動(APモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //─────────────────
  bool P22_MODE_AP(){
    //┬
    //○APホスト情報をJSONから取得（無ければデフォルト）
    const typeHost* hList = GET_HOST("ap");
    String    pName = hList ? hList->name : String("MMP_AP");
    String    pSSID = String("MMP_AP-MODE");
    IPAddress pIP   = hList ? GET_IP_AP(hList->ip) : g_IP;
    //│
    //●ＡＰモードで起動
    //▼返却：接続結果
    return RUN_AP(pSSID,pName,pIP);
  } /* P22_MODE_AP() */

  //─────────────────
  // P3.Wifi起動(緊急APモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //─────────────────
  bool P3_MODE__ALTERNATIVE(){
    //┬
    //○パラメータ値を用意 ※固定IPアドレス
    String    pName = String("MMP");
    String    pSSID = String("MMP_ALT-MODE");
    IPAddress pIP   = g_IP;
    //│
    //●緊急モードで起動
    //▼返却：接続結果
    return RUN_AP(pSSID,pName,pIP);
  } /* P3_MODE__ALTERNATIVE() */


//########################################################
//# メイン処理
//########################################################
  //─────────────────
  // 初期化処理
  //─────────────────
  void START(){
    //┬
    //○開始表示
    Serial.println(" [Wi-Fi Network device]");
    bool isOK = false;
    //│
    //●P1.設定ファイル読込
    // 【前提条件】無条件
    Serial.println("   1.設定ファイルの読込");
    isOK = P1_ReadConfig();
    //│
    //◇┐P2.設定ファイルに従い起動
    if (isOK) {
      //├┐（設定ファイルが読み込めた場合）
        Serial.println("   2.設定ファイルに従い起動します");
        //│
        //●P2-1.ＳＴＡモードでを起動
        Serial.println("   [STA mode]");
        isOK = P21_MODE_STA();
        //│
        //●P2-2.ＡＰモードで起動
        // 【前提条件】STAモードの起動に失敗
        if (!isOK) {
          Serial.println("   [AP mode]");
          isOK = P22_MODE_AP();
        }
        //┴
    } /* END-if */
    //│
    //●P3.緊急モードで起動
    // 【前提条件】設定ファイルの内容での起動に失敗
    if (!isOK) {
        Serial.println("   3.緊急モードで起動します");
        isOK = P3_MODE__ALTERNATIVE();
    } /* END-if */
    //│
    //○有効性セット
    ENABLED = isOK;
    //┴
  } /* START() */
} /* namespace devNetwork */