# 高度なシステム構成を学ぶ教材プログラム

複雑なオブジェクト構造を用いた、ゲームシステムの構築方法を学ぶための、教材プログラムです。

## 1. はじめに

実用的な保守・拡張が可能なアーキテクチャへとはなにか。
明確な責任分担をもった設計思想とはなにか。
これらを初学者が順を追って学べる内容にしています。

(注意)
初学者向の理解を深めるために、関数名・変数名など随所に日本語を多用しています。
その為、コーディングスタイルなどはPEP8に準拠しないことを理解ください。
以下に、設計上の主要な特徴を示します。

## 2. 使用技術

- 言語：Python、Pyxel  
- 設計技術：
  - オブジェクトのコンポーネント化
  - 処理のMVC化
  - データ構造の独立化

## 3. 想定ユーザー・利用シーン

- 書籍の学習者を対象としています。
- より高度な構造設計を学ぶための教材として最適です。
- 現場での開発にも通用する構造的な考え方の習得が目的です。

## 4. インストールと実行

このプロジェクトには Python と最新の Pyxel が必要です。

```bash
# リポジトリをクローン
git clone https://github.com/kureha001/Learn_Pyxel.git
cd Learn_Pyxel

# 必要なライブラリをインストール
pip install -U pyxel
pip install -U pyserial

# プログラムを実行
python main.py
```


# システム全体の設計思想

## 1. エンティティクラスとコンポジット構造

本ゲームシステムでは、複雑なオブジェクト構造を扱うために、「コンポジットクラス」と「要素クラス(FN〇〇)」という構成を採用しています。

- コンポジットクラス：キャラクタやシーンなどの大枠となるエンティティ
- 要素クラス(FN〇〇)：移動、衝突、発射、描画などの個別機能を分担

これにより、個別の処理の独立性を保ちつつ、共通の制御ロジックを活用できます。

---

## 2. クラス構造と情報の責務

すべてのエンティティ(コンポジットクラス)は、次の責務をもつクラスを内部に持つことがあります：

| クラス     | 説明                                         |
|------------|----------------------------------------------|
| 仕様クラス | 静的データ。データベースからの設定情報など。 |
| 情報クラス | 動的データ。現在の状態(座標・カウントなど)。 |

この2つのクラスは以下のように使い分けられます：

- 複数の要素で共有されるデータ → コンポジットクラス側に配置
- 要素内で閉じた処理でのみ使用 → 要素クラス側に配置

また、データベース(敵機DB・ボスDB・アイテムDB)などの配列添字によるアクセスは、仕様・情報クラスに複製することで、可読性・保守性を高めています。

---

## 3. プロセスと要素クラスの対応

コントローラは毎フレーム、特定の順序でプロセス(処理段階)を実行し、各プロセスに対応する「要素クラス(FN〇〇)」を呼び出します。これにより、処理の一貫性と拡張性が確保されます。

| 実行順序 | プロセス名   | 対応するクラス           | 説明                                        |
|----------|--------------|--------------------------|---------------------------------------------|
| ①        | 移動         | `FN移動`                 | キャラクタ・背景・特殊効果などの座標更新。 |
| ②        | 衝突         | `FN衝突`                 | 弾・敵・アイテムとの当たり判定。           |
| ③        | 発射         | `FN発射`                 | 自弾・敵弾の生成。                         |
| ④        | 出現         | なし(`__init__.py　・・・ このパッケージの初期化ファイル`)| 敵機・ボス・アイテムなど特定コンポジットクラスの初期化(生成)処理。 |
| ⑤        | 描画         | `FN描画`                 | 各オブジェクトの画面表示。                 |

### 出現プロセスの補足

- 出現プロセスは、他のプロセスと異なり、**対応する要素クラス(FN〇〇)は存在しません**。
- 出現対象となるのは、**敵機クラス**に属する「敵機」「ボス」「アイテム」などに限定されます。
- 対象のコンポジットクラスは `/キャラクタ/敵機/` 配下の各コンポーネントであり、主に `__init__.py　・・・ このパッケージの初期化ファイル` を通じて `本体` クラスを生成します。
- 他のコンポジットクラス(例：自機、背景、シーンなど)は、ゲームの準備や状態遷移時など、**出現プロセスとは別のタイミングで生成**されます。

## 4. エンティティクラスとプロセス制御の統合設計

複数のエンティティクラスが、Pyxelのフレーム更新処理に連動する「コントローラ(更新・描画)」によって制御されます。

### 主なエンティティクラス(コンポジットクラス)

- キャラクタ系(自機・敵機・弾・爆発)
- アイテムの効果を管理する特殊効果
- 画面を彩る背景
- ゲーム進行を制御する各種シーン(タイトル・プレイ・終了)

これらのクラスは、以下の特長を備えた構造です：

- 各コンポジットクラスは、特定の機能(移動・衝突・発射・描画など)を「要素クラス(FN〇〇)」として分離。
- コントローラがフレーム単位でプロセスを実行し、該当する要素クラスの処理を呼び出す構造を採用。

### プロセスとの対応関係

- プロセス名はすべて `P〇〇` という名称でコントローラ上に存在し、
- 各プロセスに対応する「要素クラス(FN〇〇)」が実行される

一方、出現プロセスに関しては、**対応する要素クラスが存在せず**、代わりに `__init__.py　・・・ このパッケージの初期化ファイル` 内で該当コンポジットクラスの初期化が行われます。

特に出現対象となるのは次の通りです：

- **敵機クラス**配下に属する「敵機」「ボス」「アイテム」
- 他のクラス(例：自機・背景・シーン)は、初期化やシーン遷移時など、別タイミングで生成される


# システム全体のディレクトリ構成

プロジェクトの全体構成は以下の通りです。
解説はMarkdown形式で、各フォルダ内に格納しています。(以下のツリーには記載していません)

```
【フォルダ】
├── main/
│   ├── コントローラ/
│   │   ├── 描画/
│   │   └── 更新/
│   ├── データセット/
│   ├── オブジェクト/
│   │   ├── キャラ/
│   │   │   ├── 弾/
│   │   │   ├── 敵機/
│   │   │   └── 自機/
│   │   ├── シーン/
│   │   ├── 演出/
│   │   └── 管理/
│   └── 汎用部品/
└── DFPlayer/
```

```
【ファイル構成】
├ main.py　・・・ 最上位モジュール
├ README.md　・・・ 説明ドキュメント
│
├── main/　・・・ 中核コントローラ
│   │ 
│   ├ __init__.py　　　・・・ このパッケージの初期化ファイル
│   ├ GAME.py　　　　　・・・ このゲームシステムの最上位クラス
│   ├ GAME共通.py　　　・・・ ゲーム共通の処理群
│   ├ リソース.pyxres　・・・ Pyxelのリソースファイル　
│   │ 
│   ├── コントローラ/
│   │　 │
│   │   ├── 更新/　・・・ 更新処理のコントローラ
│   │　 │　 │
│   │　 │　 ├ README.md
│   │　 │　 ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │　 │　 ├ main.py　・・・　このパッケージのコンポジットクラス
│   │　 │　 │
│   │　 │　 ├── P移動/　・・・ 更新の【移動】プロセス
│   │　 │　 │   ├ README.md
│   │　 │　 │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │　 │　 │   └ main.py　・・・　このパッケージのコンポジットクラス
│   │　 │　 │
│   │　 │　 ├── P衝突/　・・・ 更新の【衝突】プロセス
│   │　 │　 │   ├ Pアクション.py ・・・ サブプロセス
│   │　 │　 │   ├ P衝突判定.py ・・・ サブプロセス
│   │　 │　 ├ README.md
│   │　 │　 │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │　 │　 │   └ main.py　・・・　このパッケージのコンポジットクラス
│   │　 │　 │
│   │　 │　 ├── P発射/ ・・・ 更新の【発射】プロセス
│   │　 │　 │   ├ README.md
│   │　 │　 │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │　 │　 │   └ main.py　・・・　このパッケージのコンポジットクラス
│   │　 │　 │
│   │　 │　 └── P出現/　　・・・ 更新の【出現】プロセス
│   │　 │　　　  │
│   │　 │　　　　├ README.md
│   │　 │　　　　├── README(サブプロセス)/
│   │　 │　　　　│   ├ Pボス.md
│   │　 │　　　　│   ├ P敵機.md
│   │　 │　　　　│   ├ P特殊(詳細).md
│   │　 │　　　　│   ├ P特殊.md
│   │　 │　　　　│   ├ 機雷.md
│   │　 │　　　　│   └ 補給.md
│   │　 │　　　　│
│   │　 │　　　　├ __init__.py　・・・ このパッケージの初期化ファイル
│   │　 │　　　　├ main.py　・・・　このパッケージのコンポジットクラス
│   │　 │　　　　│
│   │　 │　　　　├ Pボス.py　・・・ ボスキャラ出現のサブプロセス
│   │　 │　　　　├ P敵機.py　・・・ 通常敵出現のサブプロセス
│   │　 │　　　　├ P機雷.py　・・・ 機雷出現のサブプロセス
│   │　 │　　　　├ P特殊.py　・・・ 特殊効果アイテム出現のサブプロセス
│   │　 │　　　　└ P補給.py　・・・ 補給系アイテム出現のサブプロセス
│   │　 │
│   │   └── 描画/　・・・ 更新処理のコントローラ(VVIEW)
│   │   　　 ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   　　 └ main.py　・・・　このパッケージのコンポジットクラス
│   │
│   ├── オブジェクト/
│   │   │ 
│   │   ├── キャラ/　・・・ キャラクタのエンティティ・クラス
│   │   │　 │ 
│   │   │   ├── 自機/　・・・ 自機のコンポーネント化クラス
│   │   │   │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │   │   ├ README.md
│   │   │   │   ├ README_自機共通.md
│   │   │   │   ├ main.py　・・・　このパッケージのコンポジットクラス
│   │   │   │   ├ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │   │   ├ FN衝突.py　・・・ 衝突プロセスで実行される要素クラス
│   │   │   │   ├ FN発射.py　・・・ 発射プロセスで実行される要素クラス
│   │   │   │   ├ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   │   └ 自機共通.py
│   │   │   │
│   │   │   ├── 敵機/　・・・ 敵機(通常敵/ボスキャラ/アイテム)のコンポーネント化クラス
│   │   │   │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │   │   ├ README.md
│   │   │   │   ├ DB_解説1.md
│   │   │   │   ├ DB_解説2.md
│   │   │   │   ├ DB.py　　　・・・ 敵機に関するデータベース
│   │   │   │   ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │   │   ├ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │   │   ├ FN衝突.py　・・・ 衝突プロセスで実行される要素クラス
│   │   │   │   ├ FN発射.py　・・・ 発射プロセスで実行される要素クラス
│   │   │   │   └ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   │
│   │   │   └── 弾/　・・・ 弾のコンポーネント化クラス
│   │   │        ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │        ├ README.md
│   │   │        ├ README(要素クラス)/
│   │   │   　　 ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │        ├ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │        ├ FN衝突.py　・・・ 衝突プロセスで実行される要素クラス
│   │   │        └ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │ 
│   │   ├── シーン/　・・・シーンののエンティティ・クラス
│   │   │   │ 
│   │   │   ├ DB.py　・・・ シーンに関するデータベース
│   │   │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │   │
│   │   │   ├── タイトル画面/　・・・ タイトル画面のコンポーネント化クラス
│   │   │   │   ├ README.md
│   │   │   │   ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │   │   ├ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   │   └ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │   │
│   │   │   ├── プレイ画面/　・・・ プレイ画面のコンポーネント化クラス
│   │   │   │   ├ README.md
│   │   │   │   ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │   │   ├ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   │   └ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │   │
│   │   │   └── 終了画面/　・・・ 終了画面のコンポーネント化クラス
│   │   │   　   ├ README.md
│   │   │   　　 ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │   　   ├ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   　   └ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │
│   │   ├── 演出/　・・・ 演出のエンティティ・クラス
│   │   │   │
│   │   │   ├── 爆発/　・・・ 爆発のコンポーネント化クラス
│   │   │   │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │   │   ├ README.md
│   │   │   │   ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │   │   ├ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │   │   └ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │   │
│   │   │   └── 背景/　・・・ 背景のコンポーネント化クラス
│   │   │        ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   │        ├ README.md
│   │   │        ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │   │        ├ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │   │        └ FN描画.py　・・・ 描画プロセスで実行される要素クラス
│   │   │
│   │   └── 管理/　・・・ 背景のコンポーネント化クラス
│   │        └── 特殊効果/　・・・ 特殊効果アイテムのコンポーネント化クラス
│   │             ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   　   　   ├ README.md
│   │             ├ DB_解説.md
│   │             ├ DB.py　　　・・・ 特殊効果に関するデータベース
│   │   　   　   ├ main.py　　・・・　このパッケージのコンポジットクラス
│   │             ├ FN移動.py　・・・ 移動プロセスで実行される要素クラス
│   │             └ FN衝突.py　・・・ 衝突プロセスで実行される要素クラス
│   │
│   ├── データセット/
│   │   ├ __init__.py　・・・ このパッケージの初期化ファイル
│   │   ├ データセット.py　・・・　このパッケージのコンポジットクラス
│   │   ├ DSコンテナ.py　　・・・ コンテナ関連の要素クラス
│   │   ├ DS仕様.py　　　　・・・ 使用関連の要素クラス
│   │   └ DS情報.py　　　　・・・ ゲーム情報関連の要素クラス
│   │
│   └── 汎用部品/　・・・ 描汎用的な部品群
│        ├ __init__.py　・・・ このパッケージの初期化ファイル
│        ├ mmpRottenmeier.py　・・・ MMP用ライブラリ
│        ├ テスト.py　　　　　・・・ MMP用テストプログラム
│        ├ 位置関係.py
│        ├ 入力操作.py
│        └ 座標変換.py
│
└── DFPlayer/　・・・ MP3のBGMファイル
　   ├ 01/　・・・　シーン別BGMファイル
　   │   ├ 001.mp3
　   │   ├ 002.mp3
　   │   └ 003.mp3
　   ├ 02/　・・・　プレイ画面(ザコ)別BGMファイル
　   │   ├ 001.mp3
　   │   ├ 002.mp3
　   │   ├ 003.mp3
　   │   ├ 004.mp3
　   │   ├ 005.mp3
　   │   ├ 006.mp3
　   │   ├ 007.mp3
　   │   ├ 008.mp3
　   │   ├ 009.mp3
　   │   ├ 010.mp3
　   │   ├ 011.mp3
　   │   ├ 012.mp3
　   │   ├ 013.mp3
　   │   ├ 014.mp3
　   │   ├ 015.mp3
　   │   └ 016.mp3
　   └ 03/　・・・　プレイ画面(ボス)別BGMファイル
　       ├ 001.mp3
　       ├ 002.mp3
　       ├ 003.mp3
　       ├ 004.mp3
　       ├ 005.mp3
　       ├ 006.mp3
　       ├ 007.mp3
　       ├ 008.mp3
　       ├ 009.mp3
　       ├ 010.mp3
　       ├ 011.mp3
　       ├ 012.mp3
　       ├ 013.mp3
　       ├ 014.mp3
　       ├ 015.mp3
　       └ 016.mp3
```