// filename : Dev_Network/device/WiFi.cpp
//========================================================
// 通信部門／担当：デバイス（WiFiサーバ）
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
//┬
//■┐インクルード
  //■Arduinoシステム
  #include <WiFi.h>
  #include <LittleFS.h>
  #include <ArduinoJson.h>
  //┴
//┴

//########################################################
//# 処理詳細
//########################################################
namespace devWiFi {
//========================================================
// 基本設計
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 設定ファイルの選定
  //━━━━━━━━━━━━━━━━━
  #if   (MODE == MODE_MAIN  )
    String FILE_PATH = "/wifi_main.json"  ;
  #elif (MODE == MODE_SUB   )
    String FILE_PATH = "/wifi_sub.json"   ;
  #elif (MODE == MODE_BRIDGE)
    String FILE_PATH = "/wifi_bridge.json";
  #else
    String FILE_PATH = "/wifi.json"  ;
  #endif

//========================================================
// 共通資源
//========================================================
  //─────────────────
  // 設定ファイルのスペック
  //─────────────────
  constexpr int HOSTs   = 4;  // アイテム登録数：ホスト情報
  constexpr int ROUTERs = 6;  // アイテム登録数：Wifiルーター情報

  //─────────────────
  // 接続のスペック
  //─────────────────
  IPAddress IP_AP = IPAddress(192,168,254,254); // APモード用のIPアドレス
  constexpr int WAIT_MS      = 15000          ; // SSID接続待ち時間ms
  constexpr int WAIT_MS_INT  = WAIT_MS / 10   ; // SSID接続待ち時間ms(間隔)
  constexpr int WAIT_MS_DIS  = 500            ; // 切断後の待ち時間ms

  //━━━━━━━━━━━━━━━━━
  // 設定ファイルＤＢ
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
      String ssid ;
      String pass ;
      bool   isDefault = false;  // JSONの "default"
    };

    //─────────────────
    //  型：接続情報
    //─────────────────
    struct typeRecord {
      typeHost   hostList[HOSTs]  ; // ホスト一覧
      typeRouter candList[ROUTERs]; // ルーター一覧
      int        hostNum = 0      ;
      int        candNum = 0      ;
    };

    //━━━━━━━━━━━━━━━
    // ＤＢを実体化
    //━━━━━━━━━━━━━━━
    typeRecord DB;


//========================================================
// 各種ヘルパ
//========================================================
  //─────────────────
  // host 配列からtype一致のものを取得
  // （"sta" / "ap"）
  //─────────────────
  static const typeHost* GET_HOST(
    const char* argMode  // "ap" または "sta"
  ) {
    for (int i=0; i<DB.hostNum; ++i)
      if (DB.hostList[i].type == argMode) return &DB.hostList[i];
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
  //━━━━━━━━━━━━━━━━━
  // 設定ファイル読込
  //----------------------------------
  //【戻り値】読込結果（論理値）
  //・true ：読込に成功
  //・false：読込に失敗
  //━━━━━━━━━━━━━━━━━
  bool READ_JSON() {
    // jsonファイル読取を開始
    File f = LittleFS.open(FILE_PATH, "r");
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
    DB.hostNum = 0;
    if (doc["host"].is<JsonArray>()) {
      for (JsonObject c : doc["host"].as<JsonArray>()) {
        // 上限チェック：配列長から算出
        if (DB.hostNum >= (int)(sizeof(DB.hostList)/sizeof(DB.hostList[0]))) break;

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
        auto& h = DB.hostList[DB.hostNum];
        h.type  = type;
        h.name  = name;
        h.ip    = ip;
        DB.hostNum++;
      }
    }

    // 情報読取：Wi-Fi候補
    DB.candNum = 0;
    if (doc["wifi"].is<JsonArray>()) {
      for (JsonObject c : doc["wifi"].as<JsonArray>()) {
        if (DB.candNum >= (int)(sizeof(DB.candList)/sizeof(DB.candList[0]))) break;
        auto& w  = DB.candList[DB.candNum];
        w.label     = String((const char*)(c["label" ] | ""));
        w.ssid      = String((const char*)(c["ssid"  ] | ""));
        w.pass      = String((const char*)(c["pass"  ] | ""));
        w.isDefault = (bool)(c["default"] | false);  // "default" → isDefault
        if (w.ssid.length()) DB.candNum++;
      }
    }

    // 正常でリターン
    return true;
  } /* READ_JSON() */


//========================================================
// IPアドレス編集のパーツ
//========================================================
  //━━━━━━━━━━━━━━━━━
  // ＳＴＡモード用
  //----------------------------------
  //・DHCPで得たIPアドレス[第1〜3オクテット]+指定の第4オクテット
  //・どの方法でも作成できない場合は[0.0.0.0]
  //━━━━━━━━━━━━━━━━━
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

  //━━━━━━━━━━━━━━━━━
  // ＡＰモード用
  //----------------------------------
  //・フル書式[x.x.x.x]ならそのまま
  //・上記でなければ第4オクテットを差替[x.x.x.0]
  //━━━━━━━━━━━━━━━━━
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
        return IPAddress(IP_AP[0], IP_AP[1], IP_AP[2],oct4);
      //└┐（その他）
        //┴
    } /* END-if */
    //│
    //▼返却:エラー時は[0.0.0.0]
    return IP_AP;
  } /* GET_IP_AP() */


//========================================================
// Wifi接続を実施
//========================================================
  //─────────────────
  // 処理結果表示ヘルパ
  //─────────────────
  void RUN_INFO(String pSSID, String pName, String pIP) {
    Serial.println(String("      [OK] SSID: ") + pSSID.c_str());
    Serial.println(String("      [OK] HOST: ") + pName.c_str());
    Serial.println(String("      [OK] IP  : ") + pIP.c_str()  );
  }

  //━━━━━━━━━━━━━━━━━
  // ＳＴＡモード接続
  //----------------------------------
  // DHCP → 必要なら静的IPへ再接続
  // 要件：hList.ipが空ならDHCPのまま採用
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //━━━━━━━━━━━━━━━━━
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
      WiFi.disconnect(false, false); delay(WAIT_MS_DIS);
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
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < WAIT_MS){Serial.print("."); delay(WAIT_MS_INT);}
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
      WiFi.disconnect(false, false); delay(WAIT_MS_DIS);
      //│
      //○WiFiサーバを起動（静的IP）
      WiFi.config(newIP, gatewayIP, subnetMask, dnsIP1, dnsIP2);
      WiFi.begin(pSSID.c_str(), pPass.c_str());
      //│
    //○接続を確認
      t0 = millis();
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < WAIT_MS){Serial.print("."); delay(WAIT_MS_INT);}
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

  //━━━━━━━━━━━━━━━━━
  // ＡＰモード接続
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //━━━━━━━━━━━━━━━━━
  bool RUN_AP(String pSSID, String pName,IPAddress pIP) {
    //┬
    //○切断して少し待つ
    WiFi.disconnect(false, false); delay(WAIT_MS_DIS);
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
// 処理プロセス
//========================================================
  //━━━━━━━━━━━━━━━━━
  // P1.設定ファイル読込
  //----------------------------------
  //【戻り値】読込結果（論理値）
  //・true ：読込に成功
  //・false：読込に失敗
  //━━━━━━━━━━━━━━━━━
  bool P1_ReadConfig(){
    if (!LittleFS.begin(true)        ){Serial.println("     [NG] 初期化に失敗"  );return false;}
    if (!LittleFS.exists(FILE_PATH)){Serial.println("     [NG] ファイルが無い");return false;}
    if (!READ_JSON()                 ){Serial.println("     [NG] 読込に失敗"    );return false;}
    return true;
} /* P1_ReadConfig() */

  
  //━━━━━━━━━━━━━━━━━
  // P2-1.Wifi起動(STAモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //━━━━━━━━━━━━━━━━━
  bool P21_MODE_STA(){
    //┬
    //◎┐WiFi情報の候補を順に試行
    bool isRun = false;
    for (int i=0; i < DB.candNum && !isRun; i++){
      //│＼（[SSIDリストの最後に達した]または[起動できた]の場合）
      //│ ▽完了：走査終了
      //│
      //●WiFiサーバを起動
      String pLabel = DB.candList[i].label.c_str();
      String pSSID  = DB.candList[i].ssid.c_str();
      String pPass  = DB.candList[i].pass.c_str();
      isRun = RUN_STA(pLabel, pSSID, pPass);
      //┴
    } /* END-for */
    //│
    //▼RETRUN:成功でリターン
    return isRun;
  } /* P21_MODE_STA() */

  //━━━━━━━━━━━━━━━━━
  // P2-2.Wifi起動(APモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //━━━━━━━━━━━━━━━━━
  bool P22_MODE_AP(){
    //┬
    //○APホスト情報をJSONから取得（無ければデフォルト）
    const typeHost* hList = GET_HOST("ap");
    String    pName = hList ? hList->name : String("MMP_AP");
    String    pSSID = String("MMP_AP-MODE");
    IPAddress pIP   = hList ? GET_IP_AP(hList->ip) : IP_AP;
    //│
    //●ＡＰモードで起動
    //▼返却：接続結果
    return RUN_AP(pSSID, pName, pIP);
  } /* P22_MODE_AP() */

  //━━━━━━━━━━━━━━━━━
  // P3.Wifi起動(緊急APモード)
  //----------------------------------
  //【戻り値】接続結果（論理値）
  //・true ：接続に成功
  //・false：接続に失敗
  //━━━━━━━━━━━━━━━━━
  bool P3_MODE_ALTERNATIVE(){
    //┬
    //○パラメータ値を用意 ※固定IPアドレス
    String    pName = String("MMP");
    String    pSSID = String("MMP_ALT-MODE");
    IPAddress pIP   = IP_AP;
    //│
    //●緊急モードで起動
    //▼返却：接続結果
    return RUN_AP(pSSID,pName,pIP);
  } /* P3_MODE_ALTERNATIVE() */


//========================================================
// 担務（公開機能）
//========================================================
  //━━━━━━━━━━━━━━━━━
  // 基本情報
  //━━━━━━━━━━━━━━━━━
  bool ENABLED = false; // 有効判定：有効：true、無効：false

  //━━━━━━━━━━━━━━━━━
  // 初期化処理
  //━━━━━━━━━━━━━━━━━
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
        isOK = P3_MODE_ALTERNATIVE();
    } /* END-if */
    //│
    //○有効性セット
    ENABLED = isOK;
    //┴
  } /* START() */
} /* namespace devWiFi */