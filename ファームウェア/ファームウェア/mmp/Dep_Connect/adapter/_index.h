// filename : Dep_Connect/adapter/__index.h
//========================================================
// 接続部門／業務課／担当：担当名簿
//--------------------------------------------------------
// Ver 1.3.2 (2026/09/21)
//========================================================
#pragma once

//========================================================
//§担当名簿
//========================================================
#if TURBO
  #include "UART_TURBO.cpp"
#else
  #include "UART.cpp"
#endif
  #include "UDP.cpp"
  #include "TCP.cpp"
  #include "WEB_Socket.cpp"
  #include "HTTP.cpp"
  #include "ESP_NOW.cpp"
  #include "BLE.cpp"
  #include "IIC.cpp"
