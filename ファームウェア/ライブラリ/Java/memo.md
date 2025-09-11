# 補足（Java 特有の点）

## 可変長・オプショナル引数：
Java は C# のようなデフォルト引数を持たないため、使い勝手が良い最小限のオーバーロードを用意しています（bits を省略できる形を提供）。
bits=0 は Settings.AnalogBits にフォールバックします（C#/Arduino と同じ仕様）。

## シリアル依存：
jSerialComm は OS 標準ドライバの上で動作し、Windows/Mac/Linux すべて対応。
追加の native 配置は不要です。

## 厳密 5 文字受信：
ノイズや複数フレーム連結に対応するため、受信バッファの末尾5文字だけ保持して判定しています（C#/Arduino と同じ流儀）。

## エラー設計：
数値系の取得は "hhhh!" の HEX4 をパース。
出力系は "!!!!!" を boolean にマッピング。
FW のコマンド誤り "abc?!" は HEX4 として失敗するので安全に検知できます。

```
mmp-java/
├─ settings.gradle
├─ build.gradle
└─ mmp-core/
   ├─ build.gradle
   └─ src/main/java/mmp/core/MmpClient.java
```
