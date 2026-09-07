# -*- coding: utf-8 -*-
#============================================================
# ＭＭＰ通信モジュール (mmp_comm.py)
# - WebAPI / WebSocket / TCP(RAW) / COM / BLE の各通信ラッパー
#============================================================
import json
import sys
import time
import socket
import random
import base64

# ====== 通信モジュール用の設定保持用 ======
COMM_CONFIG = {
    "IP": "192.168.2.99",
    "PORT_WAPI": 8080,
    "PORT_TCPR": 8081,
    "PORT_WSOC": 8082,
    "COMP_PORT": "COM148",
    "COMP_BAUD": 115200,
    "BLE_NAME": "MMP-ESP32S3",
    "BLE_CHAR_RX": "6e400002-b5a3-f393-e0a9-e50e24dcca9e",
    "BLE_CHAR_TX": "6e400003-b5a3-f393-e0a9-e50e24dcca9e",
    "VERBOSE_API": False,
    "CURRENT_MODE": "WAPI"
}

#============================================================
# BLE (Bleak) 用の簡易ラッパー
#============================================================
class SimpleBle:
    def __init__(self, device_name, rx_uuid, tx_uuid):
        self.device_name = device_name
        self.rx_uuid = rx_uuid
        self.tx_uuid = tx_uuid
        self.client = None

    def connect(self, timeout=10.0):
        from bleak import BleakClient, BleakScanner
        import asyncio

        async def _connect():
            print(f"[{self.device_name}] をスキャン中...")
            device = await BleakScanner.find_device_by_name(self.device_name, timeout=timeout)
            
            if not device:
                raise RuntimeError(f"デバイス '{self.device_name}' が見つかりませんでした。")

            print(f"-> 発見しました ({device.address})。接続中...")
            self.client = BleakClient(device)
            await self.client.connect()

        asyncio.run(_connect())

    def send_recv(self, message, timeout=10.0):
        import asyncio
        if not message.endswith('!'):
            message += '!'

        async def _send():
            # コマンドを書き込み
            await self.client.write_gatt_char(self.rx_uuid, message.encode('utf-8'), response=False)
            # 必要に応じて応答待ちのウェイト
            await asyncio.sleep(0.2) 
            return "" # 必要であれば読み込み処理を追加可能ですが、今回は空文字でOK

        return asyncio.run(_send())

    def close(self):
        import asyncio
        async def _close():
            if self.client and self.client.is_connected:
                try:
                    await self.client.disconnect()
                except Exception:
                    pass

        try:
            if self.client:
                asyncio.run(_close())
        except Exception:
            pass
        self.client = None


#============================================================
# COMポート (Serial) 用の簡易ラッパー
#============================================================
class SimpleComPort:
    def __init__(self, port_name, baudrate):
        self.port_name = port_name if port_name.upper().startswith("COM") else f"COM{port_name}"
        self.baudrate = baudrate
        self.ser = None

    def connect(self, timeout=10.0):
        import serial
        self.ser = serial.Serial(
            port=self.port_name,
            baudrate=self.baudrate,
            timeout=timeout
        )

    def send_recv(self, message, timeout=10.0):
        if not message.endswith('!'):
            message += '!'
        self.ser.timeout = timeout
        self.ser.write(message.encode('utf-8'))
        
        try:
            data = self.ser.readline()
            return data.decode('utf-8', 'errors=replace')
        except Exception:
            return ""

    def close(self):
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except:
            pass
        self.ser = None


#============================================================
# WebSocket 用の簡易ラッパー
#============================================================
class SimpleWebSocket:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self, timeout=10.0):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)
        self.sock.connect((self.host, self.port))
        
        key = base64.b64encode(bytes([random.randint(0, 255) for _ in range(16)])).decode('utf-8')
        req = (
            f"GET / HTTP/1.1\r\n"
            f"Host: {self.host}:{self.port}\r\n"
            f"Upgrade: websocket\r\n"
            f"Connection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\n"
            f"Sec-WebSocket-Version: 13\r\n\r\n"
        )
        self.sock.sendall(req.encode('utf-8'))
        resp = self.sock.recv(1024)
        if b"101 Switching Protocols" not in resp:
            raise RuntimeError("WebSocket handshake failed")

    def send_recv(self, message, timeout=10.0):
        if not message.endswith('!'):
            message += '!'
        data = message.encode('utf-8')
        length = len(data)
        
        frame = bytearray()
        frame.append(0x81)
        if length <= 125:
            frame.append(length | 0x80)
        elif length <= 65535:
            frame.append(126 | 0x80)
            frame.extend(length.to_bytes(2, 'big'))
        else:
            frame.append(127 | 0x80)
            frame.extend(length.to_bytes(8, 'big'))
            
        mask = bytes([random.randint(0, 255) for _ in range(4)])
        frame.extend(mask)
        
        masked_data = bytearray(b ^ mask[i % 4] for i, b in enumerate(data))
        frame.extend(masked_data)
        
        self.sock.settimeout(timeout)
        self.sock.sendall(frame)
        
        header = self.sock.recv(2)
        if not header:
            return ""
        length = header[1] & 0x7F
        if length == 126:
            length = int.from_bytes(self.sock.recv(2), 'big')
        elif length == 127:
            length = int.from_bytes(self.sock.recv(8), 'big')
            
        payload = self.sock.recv(length)
        return payload.decode('utf-8', 'errors=replace')

    def close(self):
        try:
            if self.sock:
                self.sock.close()
        except:
            pass
        self.sock = None


#============================================================
# TCP(RAW) 用の簡易ラッパー
#============================================================
class SimpleTcpRaw:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self, timeout=10.0):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)
        self.sock.connect((self.host, self.port))

    def send_recv(self, message, timeout=10.0):
        if not message.endswith('!'):
            message += '!'
        self.sock.settimeout(timeout)
        self.sock.sendall(message.encode('utf-8'))
        try:
            data = self.sock.recv(4096)
            return data.decode('utf-8', 'errors=replace')
        except socket.timeout:
            return ""

    def close(self):
        try:
            if self.sock:
                self.sock.close()
        except:
            pass
        self.sock = None


#============================================================
# HTTP 互換レイヤ
#============================================================
IS_MICRO   = (sys.implementation.name == "micropython")
IS_CIRCUIT = (sys.implementation.name == "circuitpython")

if IS_MICRO:
    import urequests as _requests
    def _http_get(url, timeout=10.0):
        try:
            try:
                resp = _requests.get(url, timeout=timeout)
            except TypeError:
                resp = _requests.get(url)
            try:
                status = getattr(resp, "status", getattr(resp, "status_code", 0))
                text = resp.text if hasattr(resp, "text") else resp.content.decode("utf-8", "replace")
                return status, text
            finally:
                try: resp.close()
                except: pass
        except Exception as e:
            raise RuntimeError("MicroPython GET failed: {}".format(e))

elif IS_CIRCUIT:
    import ssl
    import wifi
    import socketpool
    import adafruit_requests as _ada_req

    _CP_SESSION = None
    def _get_cp_session():
        global _CP_SESSION
        if _CP_SESSION is None:
            pool = socketpool.SocketPool(wifi.radio)
            try:
                ctx = ssl.create_default_context()
            except Exception:
                ctx = None
            _CP_SESSION = _ada_req.Session(pool, ctx)
            try:
                _CP_SESSION.timeout = 10
            except Exception:
                pass
        return _CP_SESSION

    def _http_get(url, timeout=10.0):
        s = _get_cp_session()
        try:
            try:
                resp = s.get(url, timeout=timeout)
            except TypeError:
                resp = s.get(url)
            status = getattr(resp, "status_code", getattr(resp, "status", 0))
            text = resp.text
            return status, text
        finally:
            try: resp.close()
            except: pass

else:
    from urllib import request as _urlreq, error as _urlerr

    def _http_get(url, timeout=10.0):
        try:
            with _urlreq.urlopen(url, timeout=timeout) as resp:
                status = getattr(resp, "status", 0)
                text = resp.read().decode("utf-8", errors="replace")
                return status, text
        except _urlerr.URLError as e:
            raise RuntimeError("URLError: {}".format(e))


#============================================================
# セッション管理・APIリクエスト共通関数
#============================================================
_ACTIVE_CONN = None

def session_connect():
    global _ACTIVE_CONN
    mode = COMM_CONFIG["CURRENT_MODE"]
    try:
        if mode == "WSOC":
            _ACTIVE_CONN = SimpleWebSocket(COMM_CONFIG["IP"], COMM_CONFIG["PORT_WSOC"])
            _ACTIVE_CONN.connect()
        elif mode == "TCPR":
            _ACTIVE_CONN = SimpleTcpRaw(COMM_CONFIG["IP"], COMM_CONFIG["PORT_TCPR"])
            _ACTIVE_CONN.connect()
        elif mode == "COMP":
            _ACTIVE_CONN = SimpleComPort(COMM_CONFIG["COMP_PORT"], COMM_CONFIG["COMP_BAUD"])
            _ACTIVE_CONN.connect()
        elif mode == "BLE":
            _ACTIVE_CONN = SimpleBle(COMM_CONFIG["BLE_NAME"], COMM_CONFIG["BLE_CHAR_RX"], COMM_CONFIG["BLE_CHAR_TX"])
            _ACTIVE_CONN.connect()
    except Exception as e:
        print(f"[{mode}] 接続エラー: {e}")
        _ACTIVE_CONN = None

def session_disconnect():
    global _ACTIVE_CONN
    if _ACTIVE_CONN:
        _ACTIVE_CONN.close()
        _ACTIVE_CONN = None

def api_get(path, timeout=10.0, argLog=False):
    global _ACTIVE_CONN
    mode = COMM_CONFIG["CURRENT_MODE"]
    body_text = ""
    status = 200
    ok = False
    j = None

    if mode in ("WSOC", "TCPR", "COMP", "BLE"):
        if not _ACTIVE_CONN:
            session_connect()
        
        try:
            cmd = path if path.startswith('/') else '/' + path
            body_text = _ACTIVE_CONN.send_recv(cmd, timeout=timeout)

            try:
                j = json.loads(body_text)
            except:
                j = {"value": body_text, "result": True, "ok": True}
            ok = True
            status = 200
        except Exception as e:
            ok = False
            status = 500
            body_text = json.dumps({"error": str(e)})
            j = {"error": str(e)}

    else:
        # WebAPI通信の場合
        url = f"http://{COMM_CONFIG['IP']}:{COMM_CONFIG['PORT_WAPI']}{path}"
        status, body_text = _http_get(url, timeout=timeout)
        ok = (200 <= status < 300)
        try:
            j = json.loads(body_text)
        except Exception:
            j = None

        if COMM_CONFIG["VERBOSE_API"] or argLog:
            print("\n=== API ===\nGET {}\nHTTP {}".format(url, status))
            if j is not None:
                try:
                    print(json.dumps(j, ensure_ascii=False, indent=2))
                except Exception:
                    print(body_text or "(空)")
            else:
                print(body_text or "(空)")
            print("=== /API ===\n")

    return ok, status, j, body_text