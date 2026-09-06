// filename : devWiFi.h

#pragma once
#include <WiFi.h>

extern String SRV_IP;

//=====================================================
// ＷｉＦｉ
//=====================================================
namespace devWiFi {
  //─────────────────
  // デバイス起動
  //─────────────────
  bool START(
    String argSSID,  // ルータのSSID
    String argPSWD,  // ルータのパスワード
    String argSrvIP4 // MMPのIPアドレス(第4オクテット)
) {

    Serial.println("\n========== [WiFi] START() ==========");

    Serial.printf(" 1.Connecting to %s", argSSID.c_str());
    WiFi.begin(argSSID, argPSWD);

    int intDelayMs    = 500;
    int intRetryCount = 40;
    int intNowCount   = 0;

    while (WiFi.status() != WL_CONNECTED && intNowCount < intRetryCount) {
      delay(intDelayMs);
      Serial.print(".");
      intNowCount++;
    } /* END-while */

    Serial.println("");

    bool isConnect = (WiFi.status() == WL_CONNECTED);
    if (!isConnect) {
      Serial.println("  [NG] Connection Failed");
      return false;
    } /* END-if */

    IPAddress localIP = WiFi.localIP();
    SRV_IP = String(localIP[0]) + "." + 
             String(localIP[1]) + "." + 
             String(localIP[2]) + "." + 
             argSrvIP4;

    Serial.printf   ("  [OK] My  IP:[%s]\n", WiFi.localIP().toString().c_str());
    Serial.printf   ("  [OK] MMP IP:[%s]\n", SRV_IP);

    return true;
  } /* START() */

} /* namespace devBLE */
