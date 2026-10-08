# ユーザ認証
最終更新日：2026年08月11日

## はじめに

機能モジュールのなかには、ユーザ個別のデータを持つものがあります。

ユーザの識別には**アクセスID**を用います。
ユーザは初めに`アクセスID`と対になる**認証CD**の発行を要求します。
次回以降は取得した`認証CD`を用いることで、ユーザ個別のデータを活用できます。

要求に**認証CD**を添付すると、該当する`アクセスID`が≪コンテキスト≫に格納されます。
機能モジュールは≪コンテキスト≫を通じて`アクセスID`を取得し、ユーザ個別のデータを特定します。

## 処理シーケンス

```mermaid
sequenceDiagram
    autonumber
    actor Client as 接続元
    participant Adapter as 通信アダプタ
    participant Context as コンテキスト
    participant AuthMod as ユーザ認証モジュール
    participant AuthTable as ユーザ認証TBL
    participant Module as 機能モジュール
    participant UserData as ユーザ個別データTBL

    Note over Client, AuthTable: 1. 初回：認証CDの発行要求
    Client->>Adapter: 認証CD発行要求 (アクセスID含む)
    Adapter->>Context: リクエストを格納
    Context->>AuthMod: 認証CD発行処理を依頼
    AuthMod->>AuthTable: 新規認証ユーザをINSERT (認証CD発行)
    AuthTable-->>AuthMod: 登録完了
    AuthMod-->>Context: 発行結果を格納
    Context-->>Adapter: レスポンスデータ取得
    Adapter-->>Client: 発行した認証CDを返却

    Note over Client, Module: 2. 次回以降：認証CDを用いた機能要求
    Client->>Adapter: リクエスト ＋ 認証CD添付
    Adapter->>Context: 要求と認証CDを格納
    Context->>AuthMod: 認証確認を依頼
    AuthMod->>AuthTable: ユーザ認証TBLを照会 (認証CDチェック)
    alt 認証CDが存在しない場合
        AuthTable-->>AuthMod: 該当なし (エラー)
        AuthMod-->>Context: 認証エラー情報を格納
        Context-->>Adapter: エラーレスポンス取得
        Adapter-->>Client: 認証エラーを返却 (※機能モジュールに到達せず終了)
    else 認証CDが存在する場合
        AuthTable-->>AuthMod: 認証OK (アクセスID特定)
        AuthMod-->>Context: 認証OK・アクセスIDを反映
        Context->>Module: 機能実行
        Module->>UserData: アクセスIDでユーザ個別データTBLを照会<br/>(※原則存在するため不在チェック不要)
        UserData-->>Module: ユーザ個別データを取得
        Module-->>Context: 処理結果を格納
        Context-->>Adapter: 処理結果を取得
        Adapter-->>Client: レスポンス返却
    end
```