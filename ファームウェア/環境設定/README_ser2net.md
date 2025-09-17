# ser2net 運用ドキュメント<br>（Raspberry Pi 向け）

本書は Raspberry Pi をモデルに、ser2net を使ってシリアルデバイスを TCP にブリッジする手順をまとめたものです。

---

## 1. インストール

Raspberry Pi OS (Lite/GUI どちらでも可) 上で以下を実行します。

```bash
sudo apt update
sudo apt install ser2net
```

インストール後、バージョンを確認します。

```bash
ser2net -v
```

- 3.x 系 → `/etc/ser2net.conf` を使用
- 4.x 系 → `/etc/ser2net.yaml` を使用

ここでは **v3.5** を前提とします。

---

## 2. 設定ファイル編集

設定ファイルは `/etc/ser2net.conf` です。  
サンプルの 2000/2001/3000/3001 の定義は削除またはコメントアウトしてください。

例：`/dev/ttyACM0` を 115200bps で公開する場合:

```conf
3331:raw:0:/dev/ttyACM0:115200 8DATABITS NONE 1STOPBIT -XONXOFF -RTSCTS -LOCAL
```

複数ポートを公開する例（ttyACM0〜ttyACM3）:

```conf
3331:raw:0:/dev/ttyACM0:115200 8DATABITS NONE 1STOPBIT -XONXOFF -RTSCTS -LOCAL
3332:raw:0:/dev/ttyACM1:115200 8DATABITS NONE 1STOPBIT -XONXOFF -RTSCTS -LOCAL
3333:raw:0:/dev/ttyACM2:115200 8DATABITS NONE 1STOPBIT -XONXOFF -RTSCTS -LOCAL
3334:raw:0:/dev/ttyACM3:115200 8DATABITS NONE 1STOPBIT -XONXOFF -RTSCTS -LOCAL
```

保存後にサービスを再起動します。

---

## 3. サービスの実行と再起動

サービス起動状態を確認:

```bash
systemctl status ser2net --no-pager
```

再起動する場合:

```bash
sudo systemctl restart ser2net
```

ブート時に自動起動するよう有効化:

```bash
sudo systemctl enable ser2net
```

---

## 4. 動作確認

待ち受けポートを確認:

```bash
ss -tlnp | grep 333
```

ログ確認（エラーや設定読み込みを確認）:

```bash
sudo journalctl -u ser2net -n50 --no-pager
```

---

## 5. トラブルシューティング

### ポートが LISTEN しない
- サンプル設定 (2000/3000) が残っていないか確認
- `/etc/ser2net.conf` の文法ミスを確認（特にスペース）
- 設定変更後は必ず `sudo systemctl restart ser2net`

### デバイスファイルが見つからない
- USB接続を確認
- `/dev/ttyACM*` の番号は再接続で変わることがある
- 固定したい場合は `/dev/serial/by-id/...` のシンボリックリンクを利用

### IP指定で接続できない
- v3 系は IP バインドをサポートしていないため、常に全インターフェイスで待受
- LANアクセス制御は UFW/iptables 等で行う

---

## 6. クライアントからの接続例

### Linux/Windows (pySerial)
```python
import serial
s = serial.serial_for_url("socket://192.168.2.113:3331", timeout=1)
s.write(b"VER!")
print(s.read(5))
s.close()
```

### 簡易確認 (telnet)
```bash
telnet 192.168.2.113 3331
```

---

以上で、ser2net を使ったシリアルデバイスの TCP ブリッジ環境を構築できます。
