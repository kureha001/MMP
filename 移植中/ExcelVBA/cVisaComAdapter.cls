'#============================================================
'# Adapter（VISA-COM）の最小実装
'#------------------------------------------------------------
'# ファイル種別 ： クラス モジュール
'# ファイル名称 ： cVisaComAdapter.cls
'#============================================================
Option Explicit

'#============================================================
'# モジュール・レベル
'#============================================================
' 参照設定: VISA COM x.x
Private mVisaリソースmgr As VisaComLib.ResourceManager
Private mVisa入出力 As VisaComLib.FormattedIO488  ' 便宜上、共通 I/O ラッパを使用
Private mVisaメッセージ As Object                 ' 実体は IMessage

Private m時間切れ_ミリ秒 As Long
Private m接続状態 As Boolean


'#============================================================
'# UARTオープン
'#============================================================
Public Function OpenRsrc( _
    ByVal argリソース As String, _
    ByVal arg通信速度 As Long, _
    ByVal arg時間切れ_ミリ秒 As Long _
    ) As Boolean
On Error GoTo ラベル_失敗時の後処理
    
    Set mVisaリソースmgr = New VisaComLib.ResourceManager
    Set mVisa入出力 = New VisaComLib.FormattedIO488
    Set mVisaメッセージ = mVisaリソースmgr.Open(argリソース) ' 例: "ASRL5::INSTR"
    Set mVisa入出力.io = mVisaメッセージ

    m時間切れ_ミリ秒 = arg時間切れ_ミリ秒
    mVisaメッセージ.Timeout = m時間切れ_ミリ秒

    ' --- シリアル属性（8N1, ボーレート, TermChar無効） ---
    With mVisaメッセージ
        Call .SetAttribute(&H3FFF0021, arg通信速度) ' VI_ATTR_ASRL_BAUD
        Call .SetAttribute(&H3FFF0018, 8)           ' VI_ATTR_ASRL_DATA_BITS
        Call .SetAttribute(&H3FFF0011, 0)           ' VI_ATTR_ASRL_PARITY    = NONE
        Call .SetAttribute(&H3FFF0013, 10)          ' VI_ATTR_ASRL_STOP_BITS = ONE(10)
        Call .SetAttribute(&H3FFF0038, 0)           ' VI_ATTR_TERMCHAR_EN    = 0 (無効)
    End With

    '▼正常終了
    m接続状態 = True
    OpenRsrc = True
    Exit Function


ラベル_失敗時の後処理:
    '▼異常終了（部分生成オブジェクトを必ず解放）
    On Error Resume Next
    If Not mVisa入出力 Is Nothing Then Set mVisa入出力.io = Nothing
    Set mVisa入出力 = Nothing
    Set mVisaメッセージ = Nothing
    Set mVisaリソースmgr = Nothing
    m接続状態 = False
    OpenRsrc = False

End Function


'#============================================================
'# UARTクローズ
'#============================================================
Public Sub CloseRsrc()
On Error Resume Next
    If Not mVisa入出力 Is Nothing Then Set mVisa入出力.io = Nothing
    If Not mVisaメッセージ Is Nothing Then Set mVisaメッセージ = Nothing
    If Not mVisaリソースmgr Is Nothing Then Set mVisaリソースmgr = Nothing
    m接続状態 = False
End Sub


'==================================================================
' タイムアウト（ミリ秒）
'==================================================================
Public Property Let timeoutMs(ByVal argミリ秒 As Long)
On Error Resume Next
    m時間切れ_ミリ秒 = argミリ秒
    If Not mVisaメッセージ Is Nothing Then mVisaメッセージ.Timeout = m時間切れ_ミリ秒
End Property


'==================================================================
' 送信処理
'==================================================================
Public Function WriteText(ByVal art送信文字列 As String) As Boolean
On Error GoTo ラベル_失敗時の後処理
    
    '●送信
    mVisa入出力.WriteString art送信文字列
    
    '▼正常終了
    WriteText = True
    Exit Function


ラベル_失敗時の後処理:
    '▼異常終了
    WriteText = False

End Function


'==================================================================
' 受信処理
' ※VISA-COMは in_waiting相当が乏しく、ReadUpTo方式でまとめて取得
'==================================================================
Public Function ReadUpTo(ByVal arg読込調整 As Long) As String
On Error GoTo ラベル_失敗時の後処理
    
    '○防御：0以下は即返す
    If arg読込調整 <= 0 Then
        ReadUpTo = vbNullString
        Exit Function
    End If

    '○バイナリ読み（終端文字非依存）
    Dim data() As Byte
    ReDim data(0 To arg読込調整 - 1)  ' ★0始まりで確保（VISA-COM互換性向上）
    
    '○
    Dim n As Long
    n = mVisa入出力.io.Read(data, arg読込調整) ' 読めたバイト数を返す
    
    '◇
    If n <= 0 Then
        ReadUpTo = vbNullString
    Else
        ' ★読み取れたサイズ n に縮小してから文字列化
        If n < arg読込調整 Then ReDim Preserve data(0 To n - 1)
        ReadUpTo = StrConv(data, vbUnicode)
    End If
    
    '▼正常終了
    Exit Function

ラベル_失敗時の後処理:
    '▼異常終了
    ReadUpTo = vbNullString
End Function


'==================================================================
' 通信バッファを消去
'==================================================================
Public Sub ClearBuffer()
On Error Resume Next
    mVisa入出力.io.Clear
End Sub


'==================================================================
' 接続状態
'==================================================================
Public Property Get IsOpen() As Boolean
    IsOpen = m接続状態
End Property

