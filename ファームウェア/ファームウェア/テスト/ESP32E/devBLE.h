// filename : devBLE.h

#pragma once
#include <BLEClient.h>

extern BLEClient* BLE_CLIENT = nullptr; // BLEクライアント

//=====================================================
// ＢＬＥ
//=====================================================
namespace devBLE {
  //─────────────────
  // デバイス起動
  //─────────────────
  bool START(String argDevName) {

    Serial.println("\n========== [BLE] START() ==========");

    if (!BLEDevice::getInitialized()) BLEDevice::init("");
    BLE_CLIENT = BLEDevice::createClient();

    Serial.printf(" 1.Scanning for BLE device: %s...\n", argDevName.c_str());

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    BLEScanResults*      foundDevices = pBLEScan->start(5, false);
    BLEAdvertisedDevice* targetDevice = nullptr;

    for (int i = 0; i < foundDevices->getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices->getDevice(i);
      if (device.haveName() && device.getName() == argDevName) {
      targetDevice = new BLEAdvertisedDevice(device);
      break;
      }
    } /* END-for */

    pBLEScan->clearResults();

    if (targetDevice == nullptr) {
      Serial.println("   [NG] Connected");
      return false;
    } /* END-if */

    Serial.println(" 2.Connecting to BLE Server...");
    if (!BLE_CLIENT->connect(targetDevice)) {
      Serial.println("   [NG] Connection Failed");
      delete targetDevice;
      return false;
    } /* END-if */

    delete targetDevice;

    Serial.println("   [OK] Connected to BLE Server.");
    return true;
  } /* START() */

} /* namespace devBLE */
