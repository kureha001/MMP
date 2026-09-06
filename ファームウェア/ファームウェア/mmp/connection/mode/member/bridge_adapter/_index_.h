// filename : connection/mode/member/_index_.h
//========================================================
// 経路アダプタ／動作モード／ブリッジモード：経路アダプタ
//--------------------------------------------------------
// Ver 1.2.2 (2026/09/04) 
//========================================================
#pragma once

//========================================================
// 役割
//========================================================
 namespace TRANS{
  // TCP RAW
  namespace TCP{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace TCP */

  // WEB Socket
  namespace WSOC{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace WSOC */

  // WEB Socket
  namespace WAPI{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace WAPI */

  // BLE
  namespace BLE{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace BLE */

  // ESP-NOW
  namespace ESPN{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace ESPN */

  // IIC
  namespace IIC{
    bool begin() {return false;}
    bool send()  {return false;}
    bool end()   {return false;}
  } /* namespace IIC */
} /* namespace TRANS */