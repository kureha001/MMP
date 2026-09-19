import serial
import socket
import asyncio
import websockets
import time
import urllib.request
from bleak import BleakScanner, BleakClient

# ==========================================
# 測定・パラメータ設定
# ==========================================
iterations   = 10
MAX_CHANNELS = 3
MAX_PINS     = 4   

# ==========================================
# ログ出力の設定
# ==========================================
LOG_PYTHON  = False  # Pyhon側のログ
LOG_MMP     = False  # MMP側のログ

# ==========================================
# 動作モード・通信パラメータ設定
# ==========================================
CONNECTION_MODE = 'COM'   # シリアル
#CONNECTION_MODE = 'TCP'   # TCP
#CONNECTION_MODE = 'WS'    # WebSocket
#CONNECTION_MODE = 'HTTP'  # HTTP
#CONNECTION_MODE = 'BLE'   # Bluetooth LE

# COM（シリアル通信）設定
BAUDRATE = 921600

#-------------------------------------------------------------------------------------------------
#■メイン<<USB-CDC>>
#COM_PORT = 'COM50' # 全部          : 1.809 ms (100%) ★故障★
#COM_PORT = 'COM53' # 上記 GPIO PORT: 2.338 ms (129%) 921600
#COM_PORT = 'COMxx' # UART          : 1.103 ms ( 61%)
#COM_PORT = 'COM49' # ターボ&ESP-NOW: 0.551 ms ( 30%)
#COM_PORT = 'COM48' # ターボ        : 0.434 ms ( 28%)
#-------------------------------------------------------------------------------------------------
#■サブ<<USB-CDC>>
#COM_PORT = 'COM59' ### 全部 ###
  # 全部   ＋メイン(全部)            : 3.131 ms (100%)
  #        ＋メイン(UART)            : 1.993 ms ( 70%)
#COM_PORT = 'COM45' ### ESP-NOW ###
  # ターボ&ESP-NOW＋メイン(全部)     : 2.093 ms ( 67%) ★UART&SP-NOW:2.8ms
  #               ＋メイン(UART)     : 1.133 ms ( 36%)
#-------------------------------------------------------------------------------------------------
#■ブリッジ<<ESP-NOW>>
#COM_PORT = 'COM47'
  # ESP-NOW＋メイン(全部          )  : 6.293 ms (100%)
  #        ＋メイン(ターボ&ESP-NOW)  : 5.328 ms ( 85%)
  #-----------------------------------------------------------------------------------------------
  # ESP-NOW＋サブ(全部   )＋メイン(全部): 7.780 ms (100%)
  #                       ＋メイン(UART): 6.238 ms ( 80%)
  #        ＋サブ(ESP-NOW)＋メイン(全部): 5.295 ms ( 68%)
  #                       ＋メイン(UART): 5.228 ms ( 67%)
#-------------------------------------------------------------------------------------------------
COM_TRANS = ""
#■MAIN
#COM_TRANS = "BRIDGE/TCP:192.168.2.99:8081!"
#COM_TRANS = "BRIDGE/WSOC:192.168.2.99:8082!"
#COM_TRANS = "BRIDGE/ESPN:50787D18448C!" # COM50:全部
#COM_TRANS = "BRIDGE/ESPN:50787D17BE20!" # COM49 :ターボ+ESP-NOW
#COM_TRANS = "BRIDGE/BLE:MMP-ESP32S3!"   # ×：繋がらない
#■SUB
#COM_TRANS = "BRIDGE/TCP:192.168.2.147:8081!"
#COM_TRANS = "BRIDGE/WSOC:192.168.2.147:8082!"
#COM_TRANS = "BRIDGE/ESPN:90DA72734DE4!" # COM59:全部
#COM_TRANS = "BRIDGE/ESPN:90DA72741AF0!" # COM45:ESP-NOW
#COM_TRANS = "BRIDGE/BLE:MMP-SUB-001!"   # ×：繋がらない
#-------------------------------------------------------------------------------------------------

# ネットワーク設定
TCP_IP    = '192.168.2.99'
TCP_PORT  = 8081
WS_PORT   = 8082
HTTP_PORT = 8080

# BLE 設定
BLE_DEVICE_NAME  = 'MMP-ESP32S3'
BLE_UART_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # 書き込み用 (例: Nordic UART Service)
BLE_UART_TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # 受信/通知用

setup_cmd    = f"ANALOG/SETUP:{MAX_CHANNELS}:{MAX_PINS}!"
cmds_per_set = 1 + (MAX_CHANNELS * MAX_PINS)


class DeviceConnection:
    """COM / TCP / WebSocket / HTTP / BLE の差分を吸収する全機能抽象化クラス"""
    def __init__(self, mode, com_port=None, baudrate=None, tcp_ip=None, 
                 tcp_port=None, ws_port=None, http_port=None, 
                 ble_name=None, timeout=2.0):
        self.mode = mode.upper()
        self.timeout = timeout
        self.conn = None
        self.com_port = com_port
        self.baudrate = baudrate
        self.tcp_ip = tcp_ip
        self.tcp_port = tcp_port
        self.ws_port = ws_port
        self.http_port = http_port
        self.http_base_url = f"http://{self.tcp_ip}:{self.http_port}/"
        self.last_http_cmd = ""
        
        # BLE用パラメータ
        self.ble_name = ble_name
        self.ble_rx_queue = asyncio.Queue()

    async def connect(self):
        """非同期接続処理"""
        if self.mode == 'COM':
            self.conn = serial.Serial()
            self.conn.port     = self.com_port
            self.conn.baudrate = self.baudrate
            self.conn.timeout  = self.timeout
            self.conn.dtr      = False
            self.conn.rts      = False
            self.conn.open()
            await asyncio.sleep(2)  # 接続安定化待ち

        elif self.mode == 'TCP':
            self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.conn.settimeout(self.timeout)
            self.conn.connect((self.tcp_ip, self.tcp_port))

        elif self.mode == 'WS':
            uri = f"ws://{self.tcp_ip}:{self.ws_port}"
            self.conn = await websockets.connect(uri)

        elif self.mode == 'HTTP':
            # ステートレスのため事前接続なし
            pass

        elif self.mode == 'BLE':
            print(f"BLEデバイス [{self.ble_name}] をスキャン中... (タイムアウト: 10秒)")
            
            device = await BleakScanner.find_device_by_name(self.ble_name, timeout=10.0)
            
            if not device:
                print(f"[{self.ble_name}] の直接検出に失敗。周辺デバイスを一覧検索します...")
                devices = await BleakScanner.discover(timeout=5.0)
                for d in devices:
                    if d.name and self.ble_name in d.name:
                        device = d
                        print(f"一覧から発見: {d.name} ({d.address})")
                        break
            
            if not device:
                raise ConnectionError(f"BLEデバイス '{self.ble_name}' が見つかりませんでした。")
            
            print(f"発見: {device.name} ({device.address}) - 接続中...")
            self.conn = BleakClient(device)
            await self.conn.connect()
            await self.conn.start_notify(BLE_UART_TX_UUID, self._ble_notification_handler)
        else:
            raise ValueError("無効な CONNECTION_MODE が指定されました。")

    def _ble_notification_handler(self, sender, data):
        """BLE受信データのキュー保存コールバック"""
        self.ble_rx_queue.put_nowait(data)

    async def _fetch_http(self, command_str):
        """HTTP GET リクエストの非同期実行（生のコマンドパスをそのまま送信）"""
        clean_cmd = command_str.lstrip('/')
        url = f"{self.http_base_url}{clean_cmd}"
        
        req = urllib.request.Request(url, method='GET')
        
        def _sync_request():
            with urllib.request.urlopen(req, timeout=self.timeout) as response:
                return response.read()

        return await asyncio.to_thread(_sync_request)

    async def send(self, data_str):
        """データ送信"""
        if self.mode == 'COM':
            self.conn.write(data_str.encode('utf-8'))
            self.conn.flush()
        elif self.mode == 'TCP':
            self.conn.sendall(data_str.encode('utf-8'))
        elif self.mode == 'WS':
            await self.conn.send(data_str)
        elif self.mode == 'HTTP':
            self.last_http_cmd = data_str
        elif self.mode == 'BLE':
            await self.conn.write_gatt_char(BLE_UART_RX_UUID, data_str.encode('utf-8'), response=False)

    async def read_exact(self, num_bytes=5):
        """固定5バイト受領"""
        if self.mode == 'COM':
            return self.conn.read(num_bytes)
        elif self.mode == 'TCP':
            chunks = []
            bytes_recd = 0
            while bytes_recd < num_bytes:
                chunk = self.conn.recv(num_bytes - bytes_recd)
                if not chunk:
                    raise ConnectionError("TCPソケットが切断されました。")
                chunks.append(chunk)
                bytes_recd += len(chunk)
            return b''.join(chunks)
        elif self.mode == 'WS':
            res = await self.conn.recv()
            return res.encode('utf-8')[:num_bytes] if isinstance(res, str) else res[:num_bytes]
        elif self.mode == 'HTTP':
            res_bytes = await self._fetch_http(self.last_http_cmd)
            return res_bytes[:num_bytes]
        elif self.mode == 'BLE':
            received = bytearray()
            while len(received) < num_bytes:
                chunk = await asyncio.wait_for(self.ble_rx_queue.get(), timeout=self.timeout)
                received.extend(chunk)
            return bytes(received[:num_bytes])

    async def reset_input_buffer(self):
        """受信バッファクリア"""
        if self.mode == 'COM':
            self.conn.reset_input_buffer()
        elif self.mode == 'TCP':
            self.conn.setblocking(False)
            try:
                while True:
                    if not self.conn.recv(1024): break
            except (BlockingIOError, socket.error):
                pass
            finally:
                self.conn.setblocking(True)
                self.conn.settimeout(self.timeout)
        elif self.mode == 'WS':
            try:
                while True:
                    await asyncio.wait_for(self.conn.recv(), timeout=0.001)
            except (asyncio.TimeoutError, Exception):
                pass
        elif self.mode == 'HTTP':
            pass
        elif self.mode == 'BLE':
            while not self.ble_rx_queue.empty():
                self.ble_rx_queue.get_nowait()

    async def close(self):
        """接続終了"""
        if self.conn:
            if self.mode == 'WS':
                await self.conn.close()
            elif self.mode == 'BLE':
                await self.conn.stop_notify(BLE_UART_TX_UUID)
                await self.conn.disconnect()
            elif self.mode != 'HTTP':
                self.conn.close()


async def execute_cmd_5bytes(dev, cmd_str):
    await dev.reset_input_buffer()
    await dev.send(cmd_str)
    return await dev.read_exact(5)


# ==========================================
# メイン処理
# ==========================================
async def main():
    try:
        print(f"接続試行中... [モード: {CONNECTION_MODE}]")
        dev = DeviceConnection(
            mode=CONNECTION_MODE,
            com_port=COM_PORT,
            baudrate=BAUDRATE,
            tcp_ip=TCP_IP,
            tcp_port=TCP_PORT,
            ws_port=WS_PORT,
            http_port=HTTP_PORT,
            ble_name=BLE_DEVICE_NAME
        )
        await dev.connect()
        print("接続完了。")

        # ＭＭＰのログをオフ
        if LOG_MMP:
            res_log = await execute_cmd_5bytes(dev, "SYS/SET/LOG:1!")
            print(f"ログ出力制御: [ON] ... {res_log}")
        else:
            res_log = await execute_cmd_5bytes(dev, "SYS/SET/LOG:0!")
            print(f"ログ出力制御: [OFF] ... {res_log}")

        # COM_TRANS が設定されている場合、接続後に最初に一度だけ実行
        if COM_TRANS:
            res_trans = await execute_cmd_5bytes(dev, COM_TRANS)
            print(f"転送設定送信: {COM_TRANS} ... {res_trans}")

        # 初期設定・ログオフ
        res_setup = await execute_cmd_5bytes(dev, setup_cmd)
        print(f"アナログ設定: {setup_cmd} ... {res_setup}")

        durations = []

        print(f"\n--- {iterations}回の繰り返し処理を開始 (1セット: {cmds_per_set} コマンド) ---")
        for i in range(iterations):
            start_time = time.perf_counter()

            # 1. ANALOG/INPUT! 実行
            await execute_cmd_5bytes(dev, "ANALOG/INPUT!")

            # 2. ANALOG/READ ループ
            for ch in range(1, MAX_CHANNELS + 1):
                for pin in range(1, MAX_PINS + 1):
                    read_cmd = f"ANALOG/READ:{ch - 1}:{pin - 1}!"
                    res = await execute_cmd_5bytes(dev, read_cmd)
                    
                    if LOG_PYTHON:
                        print(f" [{ch}][{pin}] (ID:{ch-1},{pin-1}) : {res}")

            end_time = time.perf_counter()
            elapsed = end_time - start_time
            durations.append(elapsed)

            if LOG_PYTHON:
                avg_cmd_ms = (elapsed / cmds_per_set) * 1000
                print(f"進捗: {i + 1}/{iterations} 回完了 | セット時間: {elapsed:.3f} s | 1コマンド平均: {avg_cmd_ms:.3f} ms")

        # 集計処理
        avg_set_duration = sum(durations) / len(durations)
        avg_single_cmd_duration = (avg_set_duration / cmds_per_set) * 1000

        print("\n--- 計測結果 ---")
        print(f"通信モード        : {CONNECTION_MODE}")
        print(f"動作パラメータ    : チャンネル [1〜{MAX_CHANNELS}] / ピン [1〜{MAX_PINS}] (設定: {setup_cmd})")
        print(f"全繰り返し回数    : {iterations} 回")
        print(f"1セットのコマンド数: {cmds_per_set} 回")
        print(f"セット平均処理時間 : {avg_set_duration:.3f} s / セット")
        print(f"最速セット処理時間 : {min(durations):.3f} s")
        print(f"最悪セット処理時間 : {max(durations):.3f} s")
        print(f"----------------------------------------")
        print(f"1コマンド平均時間  : {avg_single_cmd_duration:.3f} ms / コマンド")

    except Exception as e:
        print(f"エラーが発生しました: {e}")

if __name__ == '__main__':
    asyncio.run(main())