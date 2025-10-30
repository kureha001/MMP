# filename : Function.py
# =========================================
# Ｆ層（ファンクション層）
# シーケンス制御に関わるコマンドを
# WEB-APIで提供する
# -----------------------------------------
#  Phase 2.8 : WebAPIサーバー起動（スタブ）
# =========================================
from   time import ticks_ms, sleep_ms
import ujson    # JSONからプレイヤー名取得で使用
import socket   # ポーリングのソケット通信で使用
import _thread  # ポーリングで使用

import  Data    as D層  # D層(データベース層)
import  Presen  as P層  # P層(プレゼンテーション層)

# =========================================
# ＡＰＩ実行
# =========================================
# ------------------------
# HTTPレスポンス送信(API共通)
# ------------------------
def レスポンス送信(conn, code, body_dict):
    #┬
    #○ボディ(JSON)を用意
    #　※サイズはバイト長で算出
    body       = ujson.dumps(body_dict)
    body_bytes = body.encode()
    #│
    #○ステータス行の理由句を用意
    reason = {
        200: "OK"                       ,
        201: "Created"                  ,
        204: "No Content"               ,
        400: "Bad Request"              ,
        404: "Not Found"                ,
        405: "Method Not Allowed"       ,
        500: "Internal Server Error"    ,
    }.get(code, "OK")
    #│
    #○ヘッダ「必要最小限 + CORS」を用意
    header = (
        "HTTP/1.1 {} {}\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: {}\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
    ).format(code, reason, len(body_bytes))
    #│
    #○レスポンス「ヘッダ＋ボディ」を返す
    conn.send(header.encode() + body_bytes)
    #┴
# ------------------------
# 参加者の情報を要求
# ------------------------
# 対応ＡＰＩ：/info
# リクエスト：なし
# レスポンス：① プレイヤー名
# エフェクト：なし
# ステータス：idle
# ------------------------
def api参加者情報要求(conn):
    #┬
    #①ログ表示
    print("[API] 参加者情報要求")
    #│
    #②リクエストのデータを反映
    #│
    #③主催者にレスポンス送信
    レス = {"name" : D層.基本["名前"]}
    レスポンス送信(conn, 200, レス)
    #│
    #④エフェクトを与える
    #│
    #⑤状態を更新
    D層.状況["状態"] = "idle"
    #┴
# ------------------------
# 参加が受理されたことを通知
# ------------------------
# 対応ＡＰＩ：/assign
# リクエスト：① 背番号
# レスポンス：① OK(Ture)
# エフェクト：あり
# ステータス：standby
# ------------------------
def api参加受理通知(conn, body):
    #┬
    #①ログ表示
    print("[API] 参加受理通知：", body)
    #│
    #②リクエストのデータを反映
    try   : data = ujson.loads(body) if body else {}
    except: data = {}
    D層.基本["背番号"] = data.get("no", -1)
    #│
    #③主催者にレスポンス送信
    レス = {"ok": True}
    レスポンス送信(conn, 200, レス)
    #│
    #④エフェクトを与える
    P層.api参加受理通知()
    #│
    #⑤状態を更新
    D層.状況["状態"] = "standby"
    #┴
# ------------------------
# ゲームの開始を通知
# ------------------------
# 対応ＡＰＩ：/begin
# リクエスト：① 背番号
# レスポンス：① OK(Ture)
# エフェクト：あり
# ステータス：playing
# ------------------------
def apiゲーム開始通知(conn, body):
    #┬
    #①ログ表示
    print("[API] ゲーム開始通知：", body)
    #│
    #②リクエストのデータを反映
    try   : data = ujson.loads(body) if body else {}
    except: data = {}
    D層.出題["問題"      ] = data.get("seq"     , [])
    D層.出題["制限時間"  ] = data.get("lim_time",  0)
    D層.出題["ミス上限数"] = data.get("lim_miss", -1)
    D層.出題["ペナルティ"] = data.get("penalty" ,  0)
    #│
    #③主催者にレスポンス送信
    レス = {"ok": True}
    レスポンス送信(conn, 200, レス)
    #│
    #④エフェクトを与える
    P層.apiゲーム開始通知()
    #│
    #⑤状態を更新
    D層.状況["状態"      ] = "playing"
    D層.状況["ステップ数"] = 0
    D層.状況["ミス回数"  ] = 0
    D層.状況["開始時刻"  ] = ticks_ms()
    #┴
# ------------------------
# ゲームの状況を要求
# ------------------------
# 対応ＡＰＩ：/status
# リクエスト：なし
# レスポンス：① 状態
#             ② ステップ数
#             ③ ミス回数
# エフェクト：なし
# ステータス：なし
# ------------------------
def apiゲーム状況要求(conn):
    #┬
    #①ログ表示
    print("[API] ゲーム状況要求")
    #│
    #②リクエストのデータを反映
    #│
    #③レスポンス送信
    レス = {
        "state": D層.状況["状態"      ],
        "step" : D層.状況["ステップ数"],
        "miss" : D層.状況["ミス回数"  ],
        "miss" : D層.状況["所要時間"  ],
    }
    レスポンス送信(conn, 200, レス)
    #│
    #④エフェクトを与える
    #│
    #⑤状態を更新（プレイ中)
    #┴
# ------------------------
# ゲーム結果通知
# ------------------------
# 対応ＡＰＩ：/result
# リクエスト：① 順位
#             ② 参加人数
# レスポンス：① OK(Ture)
# エフェクト：あり
# ステータス：ending
# ------------------------
def apiゲーム結果通知(conn, body):
    #┬
    #①ログ表示
    print("[API] ゲーム結果通知")
    #│
    #②リクエストのデータを反映
    try   : data = ujson.loads(body) if body else {}
    except: data = {}
    D層.結果["順位"  ] = data.get("rank", -1)
    D層.結果["参加数"] = data.get("nums", -1)
    #│
    #③レスポンス送信
    レスポンス送信(conn, 200, {"ok": True})
    #│
    #④エフェクトを与える
    P層.apiゲーム結果通知()
    #│
    #⑤状態を更新（エンディング）
    D層.状況["状態"] = "ending"
    #┴

# =========================================
# ポーリング処理
# =========================================
# ------------------------
# リクエスト分解
# ------------------------
def リクエスト分解(req):
    try:
        line0 = req.split("\r\n")[0]
        method, path, _ = line0.split(" ")
        body = req.split("\r\n\r\n", 1)[1]
        return method, path, body
    except Exception:
        return None, None, None

# ------------------------
# ポーリングのスレッド作成
# ------------------------
def ポーリング():
    #┬
    #○ソケット通信を確保する。
    addr = socket.getaddrinfo(  # アドレス取得
        D層.ネット["IP"],       # サーバーIPアドレス
        D層.ネット["PORT"],     # サーバーポート番号
        )[0][-1]
    s = socket.socket()         # ソケット作成  
    s.setsockopt(               # オプション設定
        socket.SOL_SOCKET,      # ソケットオプション
        socket.SO_REUSEADDR,    # アドレス再利用
        1                       # 有効化
        )
    s.bind(addr)                # バインド
    s.listen(1)                 # 接続待ち
    #│
    #◎└┐ポーリングし続ける
    while True:
        #○└┐正常系
        try:
            #○リクエスト内容を処理できるに分解
            #　＼(空の場合)
            #　 ▼RETURN
            conn, _ = s.accept()
            req     = conn.recv(1024).decode()
            if not req: return
            method, path, body = リクエスト分解(req)
            #│
            #◇┐リクエストに応じたAPIを実行：
            #　├→(APIが[参加者情報要求]の場合):該当するAPIを実行
            #　├→(APIが[参加受理通知]の場合　):該当するAPIを実行
            #　├→(APIが[ゲーム開始通知]の場合):該当するAPIを実行
            #　├→(APIが[ゲーム状況要求]の場合):該当するAPIを実行
            #　├→(APIが[ゲーム結果通知]の場合):該当するAPIを実行
            if   path=="/info"   and method=="GET" : api参加者情報要求(conn      )
            elif path=="/assign" and method=="POST": api参加受理通知  (conn, body)
            elif path=="/begin"  and method=="POST": apiゲーム開始通知(conn, body)
            elif path=="/status" and method=="GET" : apiゲーム状況要求(conn      )
            elif path=="/result" and method=="POST": apiゲーム結果通知(conn, body)
            else:
            #　└┐(その他)
                #○エラー(404:Not Found)をレスポンス
                print("[API] 不明なAPIリクエスト:", method, path)
                レスポンス送信(conn, 404, {"error": "not found"})
                #┴
        #│
        #○└┐正常系・異常系の共通処理
        finally:
            #○ソケット通信を切断
            #○小時間待つ(消費電力を軽減のため)
            conn.close()
            sleep_ms(100)
    #┴  ┴  ┴
# ------------------------
# サーバーを起動
# ------------------------
def サーバー起動():
    #┬
    #○ポーリングのスレッドを作成
    _thread.start_new_thread(ポーリング, ())
    print("　PORT  :", D層.ネット["PORT"])
    #┴
