# 📘 保守マニュアル

> バージョン：0.4
> コードネーム：Date(デーテ)

本書はMMPに搭載するマイコンのファームウェアを保守する為のマニュアルです。

---

## 1. システム概要

**mmpDete** は、新たに2つのシリアルポートを同日使用できるようになりました。ソースコードのモジュール分割も行い、保守しやすいスタイルに生まれ変わりました。  

- **通信ポート**
  - USBシリアル（`Serial`）
  - UART0（`Serial1`）
  - DFPlayer専用 UART（`Serial2`）

- **制御対象デバイス**
  - **PWM出力**（PCA9685 × 最大16台）
  - **アナログ入力**（HC4067 × 最大4台）
  - **DFPlayer Mini MP3モジュール**（1系統固定）
  - **I2Cデバイス**（任意）
  - **デジタルIO**（GPIO）
  - **情報コマンド**（バージョン照会など）

- **プロトコル**
  - コマンド：終端文字 `'!'`  
  - 正常応答：`!!!!!`（固定5バイト）  
  - エラー応答：`<CMD>!!`（例: `PWM!!`）  

---

## 2. モジュール構成

各機能はモジュールとして分離されています。  
モジュールはすべて `ModuleBase` を継承し、共通のインターフェースを持ちます。  

- **ModuleAnalog.h** : アナログ入力（ANS/ANU/ANR）
- **ModulePwm.h** : PWM出力、サーボ角度指定、接続状態確認（PWM/PWA/PWI/PWX）
- **ModuleI2C.h** : I2Cの読み書き（I2R/I2W）
- **ModuleDigital.h** : GPIOデジタル出力（POW）
- **ModuleDF.h** : DFPlayer操作（DIR/DLP/DSP/DPA/DPR/VOL/DEF/DST/DPX）
- **ModuleInfo.h** : 情報コマンド（VER）
- **共通部品**
  - **MmpContext.h** : 全体の共有状態
  - **LedScope.h / LedPalette.h** : RGB LED 制御と色管理
  - **SerialScope.h** : 応答先シリアルの切替管理
  - **Util.h** : 共通ユーティリティ（16進数変換、エラー応答等）
  - **CommandRouterMulti.h** : 複数ポート入力のコマンドパーサ

---

## 3. ファイル一覧と役割

|ファイル名|役割|
|----------|----|
|`mmp_main.ino`|エントリーポイント。モジュール登録、シリアル/ポート初期化|
|`MmpConfig.h`|定数設定（リクエストバッファ長 `MMP_INPUT_MAX` など）|
|`MmpContext.h`|システム全体の共有状態（各モジュールから参照）|
|`ModuleBase.h`|モジュール基底クラス|
|`ModuleAnalog.h`|アナログ入力機能|
|`ModulePwm.h`|PWM/サーボ制御機能|
|`ModuleI2C.h`|I2C制御機能|
|`ModuleDigital.h`|デジタルIO制御機能|
|`ModuleDF.h`|DFPlayer制御機能|
|`ModuleInfo.h`|システム情報取得機能|
|`CommandRouterMulti.h`|マルチポートのコマンドルーティング|
|`LedScope.h`|LEDスコープ制御（点灯区間管理）|
|`LedPalette.h`|モジュールごとのLED色定義|
|`SerialScope.h`|シリアルスコープ制御（出力ポート管理）|
|`Util.h`|共通ユーティリティ関数群|

---

## 4. コマンドの追加／修正／削除方法

### 既存モジュールへのコマンド追加
1. 対象の `ModuleXXXX.h` を開く。  
2. `owns()` に新コマンドを追加。  
3. `handle()` に `else if (...)` を追加し処理を記述。  
4. 成功時は `"!!!!!"`、エラー時は `<CMD>!!` を返す。  

### 修正
- `handle()` 内の既存処理を修正。  
- ライブラリ側の影響を確認すること。  

### 削除
- `owns()` と `handle()` 内の対象ブロックを削除。  
- 応答フォーマットの整合性を維持する。  

---

## 5. モジュール単位の追加／削除方法

### モジュール追加
1. `ModuleXXXX.h` を作成し、`ModuleBase` を継承するクラスを実装。  
2. `mmp_main.ino` に `#include "ModuleXXXX.h"` を追加。  
3. `setup()` で `g_router.addModule(new ModuleXXXX(g_ctx, LED_PALETTE_XXXX));` を追加。  
4. `LedPalette.h` に新しい色を追加。  

### モジュール削除
1. `mmp_main.ino` の `setup()` 内から該当の `addModule(...)` を削除。  
2. 不要な `#include` と `LedPalette.h` の色定義を削除。  

---

## 6. 注意点

- **応答形式統一**：必ず `"!!!!!"` または `<CMD>!!`。  
- **LED色**：必ず `LedPalette.h` で定義してから使用。  
- **シリアル出力**：必ず `MMP_SERIAL(ctx)` 経由で行う。  
- **16進数パース**：必ず `parseHexStrict()` を利用。  
- **バッファ長調整**：`MmpConfig.h` の `MMP_INPUT_MAX` を変更。  
