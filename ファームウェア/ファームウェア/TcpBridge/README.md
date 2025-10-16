# TCPブリッジ 兼 WEB-API サーバ

## 1. はじめに（目的）
本ツールは **ESP32小型マイコン（M5Stamp S3など）** を利用して、  
PCやRaspberry Pi上で動作する `ser2net` を代替するシリアル⇄TCPブリッジを提供します。 また、MMPコマンドのWEB-APIも提供します。 

- 省スペース  
- 低コスト  
- 保守性の向上（Webから設定可能）  

を目的としています。  

---

## 2. できること
- **UART⇄TCPの透過ブリッジ（RAW）**  
- **他ライブラリ同様、フルセットのMMP機能をもつWEB-API**  
- **2系統のUARTをサポート**（例: TCPポート5331/5332）  
- **Web UI搭載**  
  - `/` : 設定ファイルアップロード  
  - `/config` : 現在の設定表示  
  - `/status` : 起動状況確認  
- **Wi-Fi候補を複数登録可能**（順に接続を試行）  
- **フェイルオーバー**  
  - 接続失敗時は APモード (`m5-bridge-setup`) に切替 → 設定投入可能  

---

## 3. 制約と前提
- **信号レベル**: 3.3V TTL（M5シリーズのGPIOは3.3Vロジック）。  
  - 5Vロジック機器と接続する場合は必ずレベルシフタを使用してください。  
- **GND共通化**: 接続先機器と必ずGNDを共有すること。  
- **速度**: 115200bps ×2系統での動作を想定。  
- **セキュリティ**: LAN用途を前提。Wi-FiパスワードはJSON内に平文で保存されます。  

---

## 4. クイックスタート
![トップ画面](./images/0_TOP.png)
1. **初回起動はAPモード**になっているので、WifiをSSID: `m5-bridge-setup`で接続します。
2. WEBブラウザで`http://192.168.4.1/`にアクセスすると、トップ画面が表示されます。 
3. `参照`ボタンをクリックし、設定ファイルを選択します。選択すると`参照`ボタンの横にファイル名が表示されます。
4. `Upload`ボタンをクリックし、取り込みます。
5. 自動再起動後、設定ファイルに従いWi-Fiに接続されます。この時点でWifiをもとのものに繋ぎ直してください。
6. シリアルモニタの接続情報でIPアドレスを確認し、WEBブラウザで開くと、再びTop画面が表示されます。
7. 現在の**接続状況**リンクを押すと、接続状況などを確認できます。
8. 念のため**現在の設定ファイルの中身**リンクを押して、取り込みした内容と一致するかを確認します。

---

## 5. セットアップ詳細
- **配線**:  
  - UART1 = G3 (RX), G1 (TX)  
  - UART2 = G10 (RX), G9 (TX)  
- **IDE設定**:  
  - ボード: `ESP32S3 Dev Module`  
  - Upload Speed: 115200 または 921600  
  - Flash Size: 4MB  
  - Partition Scheme: Default 4MB with spiffs  
  - USB CDC On Boot: Enabled  

---

## 6. 使用方法

### 6-1. TCPブリッジ編
シリアルモニターに現在のIPアドレスが表示されます。そのアドレスをWEBブラウザで開きます。<br>
（例）`http://192.168.1.110`<br>
WEB画面で、設定ファイルの選択とアップロードができます。
また、現在格納する設定ファイルの中身、現在の接続状況を表示できます。

アプリケーションで制御する場合、以下の用に使用します。
- `/upload` : 設定ファイルをアップロード（再起動して反映）。  
- `/config` : 現在の `/config.json` を表示。  
- `/status` : 現在の動作状況を表示。  
- **TCP接続** : `telnet <IP> 5331` または `5332`。  

### 6-2. WEB-API編
各種ライブラリと同様に、MMPの持つ機能をフルセットでWEB-APIで提供する。APIツリー構造とシグネチャは以下の通り。
```
作成中
```

---

## 7. シーケンス（処理順序）
設定ファイル(後述)に従い、wifiに接続する。
接続に失敗するとＡＰモードとして起動する。

### 1.ＡＰモード
初回起動（config.jsonなし）または Wifi接続不可の場合、ＡＰモードとなり、各クライアントはシリアルターミナルに表示したアドレスで接続し、同一セグメントに参加する。そのままＡＰモードで使用してもよいし、設定ファイルを更新して、wifiモードに移行しても良い。
```mermaid
sequenceDiagram
    participant ESP32
    participant User
    ESP32->>ESP32: 起動
    ESP32->>ESP32: /config.json を探す
    alt 存在しない
        ESP32->>ESP32: APモード起動 (SSID: m5-bridge-setup)
        User->>ESP32: Wi-Fi接続
        User->>ESP32: /upload で config.json 投入
        ESP32->>ESP32: 自動再起動
    end
```

### ＳＴＡモード
wifiに接続するとSTAモードに切り替わる。
```mermaid
sequenceDiagram
    participant ESP32
    participant WiFiAP
    ESP32->>ESP32: 起動
    ESP32->>ESP32: /config.json を読み込み
    ESP32->>WiFiAP: 候補SSIDへ接続試行
    alt 成功
        ESP32->>ESP32: STAモード (DHCPでIP取得)
    else 全候補失敗
        ESP32->>ESP32: APフォールバック (m5-bridge-setup)
    end
```

---

## 8. 設定ファイル（`config.json`）
### スキーマ
```json
{
  "wifi": {
    "hostname": "m5-bridge",
    "candidates": [
      {"ssid": "YOUR_SSID1", "pass": "YOUR_PASS1", "timeout_ms": 8000},
      {"ssid": "YOUR_SSID2", "pass": "YOUR_PASS2", "timeout_ms": 8000}
    ],
    "ap_fallback": {"enabled": true, "ssid": "m5-bridge-setup", "pass": "", "hold_seconds": 0}
  },
  "uart": [
    {"rx": 3, "tx": 1,  "baud": 115200, "tcp_port": 5331},
    {"rx":10, "tx": 9,  "baud": 115200, "tcp_port": 5332}
  ],
  "server": {"max_clients": 4, "write_lock": false, "write_lock_ms": 30000}
}
```

### 編集例
- Wifi情報を増減する → `candidates` のエントリを追加・削除する
    - **ssid:** wifiのSSID
    - **pass:** wifiのパスワード
    - **timeout_ms:** この時間を超えたら次のエントリに進む
- 通信速度を変える → `baud` を変更  
- UARTピンを変える → `rx` / `tx` を変更  
- TCPポート番号を変える → `tcp_port` を変更  
- 同時接続数を増やす → `max_clients` を変更  

---

## 9. 保守（ハード依存）
- ESP32-S3（M5Stamp S3）: UART3系統、小型ボードで実績あり  
- ESP32-C3: UART2系統、GPIO少ないが低消費電力  
- ESP32-S2: UART2系統、Wi-Fiのみ  
- ESP32 Classic: UART3系統、情報豊富  

### Arduino IDEでのコンパイル設定
- ボード: `ESP32S3 Dev Module`（M5Stamp S3 の場合）  
- Upload Speed: 115200 または 921600  
- Flash Size: 4MB  
- Partition Scheme: Default 4MB with spiffs  

### 比較表

| 項目             | ESP32-S3 (M5Stamp S3) | ESP32-C3 | ESP32-S2 | ESP32 Classic |
|------------------|------------------------|----------|----------|---------------|
| **UART数**       | 3                      | 2        | 2        | 3             |
| **Wi-Fi**        | ○                      | ○        | ○        | ○             |
| **BLE**          | ○                      | ○        | ×        | ○             |
| **GPIO自由度**   | 高い                   | 低い     | 中       | 中             |
| **小型ボード**   | M5Stamp S3 等          | M5Stamp C3 | 少なめ   | WROOM Mini 等 |
| **情報量/実績**  | 増加中                 | 少なめ   | 少なめ   | 豊富           |

---

## 10. トラブルシューティング
- **Wi-Fiに繋がらない** → `/status` 確認、APフォールバックを利用  
- **文字化け** → baud不一致、TX/RX誤接続、GND未共通  
- **TCP接続できない** → max_clients超過、Firewall設定を確認  
