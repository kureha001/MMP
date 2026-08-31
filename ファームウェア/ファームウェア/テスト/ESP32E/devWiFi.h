#pragma once
#include <WiFi.h>

//=====================================================
// ＷｉＦｉ
//=====================================================
namespace devWiFi {
  //─────────────────
  // デバイス起動
  //─────────────────
  bool START(String argSSID, String argPSWD) {

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

    Serial.printf   ("  [OK] Connected IP:[%s]\n", WiFi.localIP().toString().c_str());
    return true;
  } /* START() */

} /* namespace devBLE */
