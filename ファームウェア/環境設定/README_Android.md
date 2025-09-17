# Android 環境構築ドキュメント

本書は Android スマートフォンで Python を使ってシリアルデバイスと通信するための環境構築方法をまとめたものです。  
目的や制約に応じて、3つのアプローチを紹介します。

---

## 1. 基本編：Pydroid + PySerial

### 1.1 Pydroid の導入
1. Google Play ストアから **「Pydroid 3」** をインストールします。
2. 初回起動後、Python インタープリタが利用可能になります。

### 1.2 pyserial のインストール
1. Pydroid 内の「PIP」メニューを開きます。
2. `pyserial` を検索し、インストールします。
3. インストール後、以下で確認できます。

```python
import serial
print(serial.__version__)
```

### 1.3 動作確認（USBシリアル直接は不可）
- Pydroid 標準の Python では USB デバイスに直接アクセスできません。
- そのため **TCPブリッジ経由** か **usb4a ライブラリ**を利用する必要があります。

### 1.4 「Serial USB Terminal」での疎通確認
1. Google Play から **「Serial USB Terminal」** をインストールします。
2. USB-OTG ケーブルでシリアルモジュール（例：USB-UART変換器、RP2040 など）を接続します。
3. アプリを開くと自動的にデバイスが認識されます。
4. ボーレート等を設定し、送受信できることを確認してください。
   - 例: `VER!` を送信し、5バイト応答があるか確認。

---

## 2. スタンドアローン編：TCP ブリッジ

### 2.1 アプリ導入
- Google Play から **「TCP UART transparent bridge」** をインストールします。

### 2.2 使い方
1. スマホに USB シリアルモジュールを接続します。
2. アプリを起動し、シリアル設定（ボーレート/データビット/パリティ/ストップビット）を入力します。
3. TCP ブリッジ機能を有効化し、待受ポートを設定します（例: 3331）。
4. これにより **スマホが ser2net 相当のブリッジサーバー** になります。

### 2.3 クライアント側からのアクセス
- 同じ LAN 内の PC/Pydroid から以下で接続可能です。

```python
import serial
s = serial.serial_for_url("socket://<スマホのIP>:3331", timeout=1)
s.write(b"VER!")
print(s.read(5))
s.close()
```

---

## 3. スタンドアローン編：usb4a + usbserial4a

### 3.1 ライブラリの導入
Pydroid で`Menu`->`Pip`で`usbserial4a`をインストールします。<br>
または、 `Menu`->`Terminal`で`pip install usbserial4a`とします。<br>
`Kivy`は、すでに Pydroid に標準搭載されているので、インストールは不要です。

### 3.2 注意点
- **usb4a/usbserial4a** は Android の USB API をラップしたライブラリです。
- **Kivy 環境での実行が前提**です。
  - 通常の「Run」では JNI 初期化に失敗し、`No JNIEnv available in Terminal` エラーが出ます。
  - **「Run with Kivy」モード**で実行する必要があります。

### 3.3 サンプルコード
```python
from usb4a import usb
from usbserial4a import serial4a

usb_device_list = usb.get_usb_device_list()
if usb_device_list:
    serial_port = serial4a.get_serial_port(
        usb_device_list[0].getDeviceName(),
        9600, 8, 'N', 1)
    if serial_port and serial_port.is_open:
        serial_port.write(b'VER!')
        print(serial_port.read(5))
        serial_port.close()
```

### 3.4 制約
- 実行環境が Kivy 前提のため、**Tkinter や pygame とは共存できません**。
- Android 権限（USBアクセス許可ダイアログ）が必要です。
- PC 向けコードをそのまま流用することはできません。

---

## まとめ

- **最も簡単**: Pydroid + PySerial + ser2net 経由（Linuxがブリッジ）  
- **スマホ単体でブリッジ**: 「TCP UART transparent bridge」アプリを利用  
- **スマホ単体で直接通信**: usb4a + usbserial4a（Kivy必須、制約多い）  

アプリ開発者の目的に応じて、上記の方式を選んでください。
