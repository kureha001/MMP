// filename : devNet.cpp
//========================================================
// 資源初期化：ネットワーク系
//--------------------------------------------------------
// ネットワーク通信が利用可能な状態にする
//--------------------------------------------------------
//【障害対策】
//・ 設定ファイル読込：失敗すると緊急ＡＰモードで起動
//・ Wi-Fi設定　　　 ：失敗すると緊急ＡＰモードで起動
//・ サービス起動    ：軌道に失敗したサービスが使えない
//--------------------------------------------------------
// Ver 1.1.0 (2026/08/15) α版 
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
  constexpr int g_MAX_ITEM_HOST   = 4;  // アイテム登録数：ホスト情報
  constexpr int g_MAX_ITEM_ROUTER = 6;  // アイテム登録数：Wifiルーター情報

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
    //○空文字チェック
    if (argBase.length()==0) return false;

    //○数字チェック
    for (char c: argBase) if (c<'0'||c>'9') return false;

    //○10進数に変換
    long v = strtol(argBase.c_str(), nullptr, 10);

    //○数値範囲チェック
    if (v<0 || v>254) return false;

    argVal = (uint8_t)v;
    return true;
  } /* IS_OCTET() */


//========================================================
// サブ処理
//========================================================
  //━━━━━━━━━━━━━━━━━
  // IPアドレスを作成
  //━━━━━━━━━━━━━━━━━
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
          //▼RETURN:第4オクテットで置換
        return IPAddress(argIP[0], argIP[1], argIP[2], oct4);
        //└┐（その他）
          //┴
      } /* END-if */
      //│
      //▼RETURN:エラー時は[0.0.0.0]
      return IPAddress();
      //┴
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
          //▼RETURN:引数そのまま
          return ip;
        //│
      } else if (IS_OCTET(argIP, oct4)) {
        //├┐（末尾だけの場合）
          //▼RETURN:第4オクテットで置換
          return IPAddress(192,168,254,oct4);
        //└┐（その他）
          //┴
        } /* END-if */
      //│
      //▼RETURN:エラー時は[0.0.0.0]
      return IPAddress();
      //┴
    } /* GET_IP_AP() */

  //━━━━━━━━━━━━━━━━━
  // Wifi接続
  //━━━━━━━━━━━━━━━━━
    //─────────────────
    // ＡＰモード用
    //----------------------------------
    //─────────────────
    //　➡【該当処理なし】※ＡＰモードは対象外

    //─────────────────
    // ＳＴＡモード用
    //----------------------------------
    // DHCP → 必要なら静的IPへ再接続
    // 要件：hList.ipが空ならDHCPのまま採用
    //─────────────────
    static bool CONNECT_STA(const typeRouter& argWifi)
    {
      if (argWifi.ssid.isEmpty()) return false;

      //○STA ホスト情報を JSON から取得（無ければデフォルト）
      const typeHost* hList = GET_HOST("sta");
      String hostName       = hList ? hList->name : String("mmp-sta-mode");
      String oct4           = hList ? hList->ip   : String("");  // 第4オクテット or 空
      //│
      //○DHCPで接続（第三オクテット把握のため）
      WiFi.mode(WIFI_STA);
      WiFi.setHostname(hostName.c_str());
      WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());
      //│
      //◎8秒間接続トライ
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000){delay(200);}
      if (WiFi.status() != WL_CONNECTED) {
      //│ ＼（タイムアウトの場合）
      //│  ▼RETURN:失敗
            WiFi.disconnect(true, true);
            delay(200);
            return false;
      } /* END-if */
      //│
      //○DHCP情報を退避
      IPAddress dhcpIP     = WiFi.localIP();
      IPAddress gatewayIP  = WiFi.gatewayIP();
      IPAddress subnetMask = WiFi.subnetMask();
      IPAddress dnsIP1     = WiFi.dnsIP(0);
      IPAddress dnsIP2     = WiFi.dnsIP(1);
      //│
      //○hList.ip が空なら DHCP のまま採用
      if (oct4.length() == 0) return true;
      //│
      //○新たに静的IPを作成(DHCP発行のIPアドレスの第4オクテットを変更)
      IPAddress newIP = GET_IP_STA(dhcpIP, oct4);
      if (!newIP) return true;
      // →（失敗なら DHCP のまま）
      //│
      // サブネットは固定 /24
      // GW は DHCP 優先・無ければ x.y.z.1、DNS 未取得なら GW
      subnetMask = IPAddress(255,255,255,0);
      if (!gatewayIP) gatewayIP = IPAddress(newIP[0], newIP[1], newIP[2], 1);
      if (!dnsIP1   ) dnsIP1 = gatewayIP;
      //│
      //○静的IPで再接続
      WiFi.disconnect(false, false);
      delay(100);
      WiFi.config(newIP, gatewayIP, subnetMask, dnsIP1, dnsIP2);
      WiFi.begin(argWifi.ssid.c_str(), argWifi.pass.c_str());
      //│
      //◎8秒間接続トライ
      t0 = millis();
      while (WiFi.status() != WL_CONNECTED && (millis()-t0) < 8000){delay(200);}
      if (WiFi.status() == WL_CONNECTED) return true;
      //│ ＼（タイムアウトの場合）
      //│  ▼RETURN:失敗
      //│
      //○
      WiFi.disconnect(true, true);
      delay(200);
      return false;
      //┴
    } /* CONNECT_STA() */

  //━━━━━━━━━━━━━━━
  // 設定ファイル読込
  //----------------------------------
  //【戻り値】実行結果（論理値）
  //・成功：false
  //・失敗：true
  //━━━━━━━━━━━━━━━
  bool InitNet_JsonMain() {
    // jsonファイル読取を開始
    File f = LittleFS.open("/config.json", "r");
    if (!f) return true;

    // ファイルサイズに応じた余裕ある容量で
    size_t sz  = f.size();
    size_t cap = sz + 1024;
    if (cap < 4096 ) cap = 4096;
    if (cap > 16384) cap = 16384;

    DynamicJsonDocument doc(cap);
    DeserializationError iniErr = deserializeJson(doc, f);
    f.close();
    if (iniErr) { return true; }

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
    return false;
  } /* InitNet_JsonMain() */


//========================================================
// プロセス
//========================================================
  //─────────────────
  // P1.設定ファイル読込
  //----------------------------------
  //【戻り値】実行結果（論理値）
  //・成功：false
  //・失敗：true
  //─────────────────
  bool InitNet_Json(){
    if (!LittleFS.begin(true)           ){Serial.println("　[NG] 初期化に失敗"  );return true;}
    if (!LittleFS.exists("/config.json")){Serial.println("　[NG] ファイルが無い");return true;}
    if (InitNet_JsonMain()              ){Serial.println("　[NG] 読込に失敗"    );return true;}
    return false;
} /* InitNet_Json() */

  
  //─────────────────
  // P2-1.Wifi起動(STAモード)
  //─────────────────
  bool InitNet_RUN_STA(){
    //┬
    //◎┐STAホスト情報から、候補を順に試行（仮実装：isDefault優先は後で統合時に実装）
    bool isRun = false;
    for (int i=0; i < g_WIFI.candNum && !isRun; i++){
      //│ ＼（[最後まで走査し終えた]または[起動できた]の場合）
      //│  ▼走査を終了する
      //│
      //○SSIDを表示
      Serial.println(String("　Try SSID=") + g_WIFI.candList[i].ssid.c_str());
      //│
      //○[ＳＴＡモード]で起動
      isRun = CONNECT_STA(g_WIFI.candList[i]);
    } /* END-for */
    //│
    //○起動状態を確認
    if (!isRun) {
    //│ ＼（起動していない場合）
    //│  ▼RETURN:起動に失敗
          Serial.println("　　[NG] STAモードの起動に失敗");
          return true;
    } /* END-if*/
    //│
    //○状況を画面に表示
    Serial.println("　　・STAモード");
    Serial.println(String("　- SSID: ") + WiFi.SSID().c_str());
    Serial.println(String("　- IP  : ") + WiFi.localIP().toString().c_str());
    //│
    //▼リターン
    return false;
    //┴
  } /* InitNet_RUN_STA() */


  //─────────────────
  // P2-2.Wifi起動(APモード)
  //─────────────────
  bool InitNet_RUN_AP(){
    //┬
    //○┐事前準備
      //○APホスト情報をJSONから取得（無ければデフォルト）
      const typeHost* hList = GET_HOST("ap");
      String    pSSID       = hList ? hList->name          : String("mmp-ap-mode");
      IPAddress pIP         = hList ? GET_IP_AP(hList->ip) : IPAddress(192,168,254,254);
      if (!pIP) pIP         = IPAddress(192,168,254,254);
      //│
      //○パラメータをセット
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(pIP, pIP, IPAddress(255,255,255,0));
      //┴
    //│
    //○[ＡＰモード]で起動
    if (!WiFi.softAP(pSSID.c_str())) {
    //│ ＼（起動に失敗した場合）
    //│  ▼RETURN:起動に失敗
          Serial.println("　　[NG] APモードの起動に失敗");
          return true;
    } /* END-if*/
    //│
    //○状況を画面に表示
    Serial.println("　　・APモード");
    Serial.println(String("　　- SSID: ") + pSSID.c_str());
    Serial.println(String("　　- IP  : ") + WiFi.softAPIP().toString().c_str());
    //│
    //▼正常でリターン
    return false;
    //┴
  } /* InitNet_RUN_AP() */

  //─────────────────
  // P3.Wifi起動(緊急APモード)
  //─────────────────
  bool InitNet_RUN_ALTERNATIVE(){
    //┬
    //○┐事前準備
      //○APモード固定でパラメータ値を用意
      String    pSSID = String("mmp-ap-mode");
      IPAddress pIP   = IPAddress(111,111,111,111);
      //│
      //○パラメータをセット
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(pIP, pIP, IPAddress(255,255,255,0));
      //┴
    //│
    //○[ＡＰモード]で起動
    if (!WiFi.softAP(pSSID.c_str())) {
    //│ ＼（起動に失敗した場合）
    //│  ▼RETURN:起動に失敗
          Serial.println("　　[NG] 緊急モードの起動に失敗");
          return true;
    } /* END-if*/
    //│
    //○状況を画面に表示
    Serial.println("　・緊急モード");
    Serial.println(String("　　- SSID: ") + pSSID.c_str());
    Serial.println(String("　　- IP  : ") + WiFi.softAPIP().toString().c_str());
    //│
    //▼正常でリターン
    return false;
    //┴
  } /* InitNet_RUN_AP() */


//########################################################
//# メイン処理
//########################################################
  //─────────────────
  // 初期化処理
  //----------------------------------
  // 戻り値：処理結果（論理値）
  // ・true ：失敗
  // ・false：成功
  //─────────────────
  void start(){
    //┬
    //○開始表示
    Serial.println(" [Wi-Fi Network device]");
    //│
    //●P1.設定ファイル読込
    // 【前提条件】無条件
     Serial.println(" １．設定ファイルの読込");
    bool isErr = InitNet_Json();
    //│
    //◇┐P2.設定ファイルに従い起動
    Serial.println("  ２．Wifiを起動設定");
    if (!isErr) {
      //├┐（エラーが残っいない場合）
        //●P2-1.ＳＴＡモードでを起動
        isErr = InitNet_RUN_STA();
        //│
        //●P2-2.ＡＰモードで起動
        // 【前提条件】エラーが残っいない
        if (isErr) isErr = InitNet_RUN_AP();
        //┴
    } /* END-if */
    //│
    //●P3.緊急モードで起動
    // 【前提条件】エラーが残ってる
    if (isErr) isErr = InitNet_RUN_ALTERNATIVE();
    //│
    //○終了表示
    if (!isErr) Serial.println("  [OK] 初期化が完了");
    Serial.println("");
    //│
    //○有効性セット
    ENABLED = !isErr;
    //┴
  } /* start() */
} /* namespace devNetwork */