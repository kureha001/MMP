'============================================================
' Wrapper（CPython と同じ UX）
'------------------------------------------------------------
' ファイル種別 ： クラス モジュール
' ファイル名称 ： cMmpVba.cls
'============================================================
Option Explicit

'#============================================================
'# モジュール・レベル
'#============================================================
Private mアダプタ As cVisaComAdapter
Private mMMPコア As cMmpCore
Private m通信速度 As Long
Private m時間切れ_ミリ秒 As Long


'==================================================================
' 初期化処理
'==================================================================
Public Sub 初期化( _
    Optional ByVal arg読込調整 As Long = 64, _
    Optional ByVal arg通信速度 As Long = 115200, _
    Optional ByVal arg時間切れ_ミリ秒 As Long = 200 _
    )

    Set mアダプタ = New cVisaComAdapter
    Set mMMPコア = New cMmpCore

    m通信速度 = arg通信速度
    m時間切れ_ミリ秒 = arg時間切れ_ミリ秒

    mMMPコア.InitCore mアダプタ, arg読込調整, arg時間切れ_ミリ秒
End Sub


'==================================================================
' UARTオープン
'  - リソース 省略時は ASRL?*::INSTR を自動探索（先頭1件）
'  - 通信速度/timeout 省略時は 初期化 時の既定値を使用
'  - ① 初期化 未実行時の自己回復を追加
'  - ② VISA ResourceManager の ProgID フォールバックを追加
'==================================================================
Public Function 通信接続( _
    Optional ByVal argリソース As String = vbNullString, _
    Optional ByVal arg通信速度 As Long = -1, _
    Optional ByVal arg時間切れ_ミリ秒 As Long = -1 _
    ) As Boolean
On Error GoTo ラベル_失敗時の後処理

    ' ① 初期化 忘れの自己回復（既定値で初期化）
    If mアダプタ Is Nothing Or mMMPコア Is Nothing Then
        初期化
    End If

    Dim 通信速度 As Long
    通信速度 = IIf(arg通信速度 >= 0, arg通信速度, m通信速度)
    
    Dim 時間切れ As Long
    時間切れ = IIf(arg時間切れ_ミリ秒 >= 0, arg時間切れ_ミリ秒, m時間切れ_ミリ秒)

    If LenB(argリソース) = 0 Then
        
        ' ② VISA を late-binding で取得（複数 ProgID を順次トライ）
        ' VISA.ResourceManager      ：一般的（Keysight/NI 等）
        ' VISA.Application          ：環境によってはこちら
        ' VisaComLib.ResourceManager： 参照設定がある環境の保険
        Dim リソースmgr As Object
        Dim 処理判定 As Boolean
        Dim プログラムID一覧 As Variant
        
        プログラムID一覧 = Array( _
            "VISA.ResourceManager", _
            "VISA.Application", _
            "VisaComLib.ResourceManager" _
        )
        
        
        '◎プログラムIDを走査しながらVISAを生成する
        Dim プログラムID As Long
        For プログラムID = LBound(プログラムID一覧) To UBound(プログラムID一覧)
            
            '○VISAを実体化する
            On Error Resume Next
                Set リソースmgr = CreateObject(プログラムID一覧(プログラムID))
            On Error GoTo 0
            
            '◇実体化に成功
            If Not リソースmgr Is Nothing Then
                処理判定 = True
                Exit For
            End If
        
        Next プログラムID
            
        '◇実体化に失敗
        If Not 処理判定 Then
            ' ③ 後始末（COM 解放）
            On Error Resume Next
            Set リソースmgr = Nothing
            On Error GoTo 0
            GoTo ラベル_失敗時の後処理
        End If

        '○ "::" を含む正規のサーチパターン
        Dim lst As Variant
        On Error Resume Next
            lst = リソースmgr.FindRsrc("ASRL?*::INSTR")
            Dim エラーNo As Long: エラーNo = Err.Number
        On Error GoTo 0
        
        '◇リソースの取得に失敗
        If エラーNo <> 0 Then
            ' ③ 後始末（COM 解放）
            On Error Resume Next
            Set リソースmgr = Nothing
            On Error GoTo 0
            GoTo ラベル_失敗時の後処理
        End If

        '◇書式別にリソースを抽出する（空配列も防御）
        If IsArray(lst) Then
            
            If UBound(lst) >= LBound(lst) Then
                argリソース = CStr(lst(LBound(lst)))
            Else
                ' ③ 後始末（COM 解放）
                On Error Resume Next
                Set リソースmgr = Nothing
                On Error GoTo 0
                GoTo ラベル_失敗時の後処理
            End If
        
        ElseIf VarType(lst) = vbString Then
            argリソース = CStr(lst)
        
        Else
            ' ③ 後始末（COM 解放）
            On Error Resume Next
            Set リソースmgr = Nothing
            On Error GoTo 0
            GoTo ラベル_失敗時の後処理
        
        End If

        ' ③ 後始末（COM 解放：正常経路）
        On Error Resume Next
        Set リソースmgr = Nothing
        On Error GoTo 0
    
    End If

    '●通信開始を試みる
    '●バージョン照合を試みる
    If Not mアダプタ.OpenRsrc(argリソース, 通信速度, 時間切れ) Then GoTo ラベル_失敗時の後処理
    If Not mMMPコア.バージョン確認() Then GoTo ラベル_失敗時の後処理

    '▼正常終了する
    ' （①の派生）成功接続時に既定値を更新してUX向上
    m通信速度 = 通信速度
    m時間切れ_ミリ秒 = 時間切れ

    mMMPコア.接続済 = True
    通信接続 = True
    Exit Function

ラベル_失敗時の後処理:
    '▼異常終了する
    通信切断
    通信接続 = False

End Function


'==================================================================
' UARTクローズ
'==================================================================
Public Sub 通信切断()
On Error Resume Next
    If Not mアダプタ Is Nothing Then mアダプタ.CloseRsrc
    If Not mMMPコア Is Nothing Then mMMPコア.接続済 = False
End Sub


'==================================================================
' コア API をそのまま露出
'  - 自己参照再帰を避けるため、内部フィールドは MMPコア に変更
'==================================================================
Public Property Get Core() As cMmpCore
    Set Core = mMMPコア
End Property


