'==================================================================
' 1) Adapter（VISA-COM）の最小実装
'==================================================================
' 参照設定: VISA COM x.x
Option Explicit

Private rm As VisaComLib.ResourceManager
Private io As VisaComLib.FormattedIO488  ' 便宜上、共通 I/O ラッパを使用
Private msg As Object                    ' 実体は IMessage

Private mTimeoutMs As Long
Private mIsOpen As Boolean

Public Function OpenRsrc(ByVal resource As String, ByVal baud As Long, ByVal timeoutMs As Long) As Boolean
    On Error GoTo ERR_
    Set rm = New VisaComLib.ResourceManager
    Set io = New VisaComLib.FormattedIO488
    Set msg = rm.Open(resource)            ' 例: "ASRL5::INSTR"
    Set io.IO = msg

    mTimeoutMs = timeoutMs
    msg.Timeout = mTimeoutMs               ' ms

    ' --- シリアル属性（8N1, ボーレート, TermChar無効） ---
    Call msg.SetAttribute(&H3FFF0021, baud)       ' VI_ATTR_ASRL_BAUD
    Call msg.SetAttribute(&H3FFF0018, 8)          ' VI_ATTR_ASRL_DATA_BITS
    Call msg.SetAttribute(&H3FFF0011, 0)          ' VI_ATTR_ASRL_PARITY = NONE
    Call msg.SetAttribute(&H3FFF0013, 10)         ' VI_ATTR_ASRL_STOP_BITS = ONE(10)
    Call msg.SetAttribute(&H3FFF0038, 0)          ' VI_ATTR_TERMCHAR_EN = 0 (無効)

    mIsOpen = True
    OpenRsrc = True
    Exit Function
ERR_:
    OpenRsrc = False
End Function

Public Sub CloseRsrc()
    On Error Resume Next
    If Not io Is Nothing Then Set io.IO = Nothing
    If Not msg Is Nothing Then Set msg = Nothing
    If Not rm Is Nothing Then Set rm = Nothing
    mIsOpen = False
End Sub

Public Property Let TimeoutMs(ByVal v As Long)
    mTimeoutMs = v
    On Error Resume Next
    If Not msg Is Nothing Then msg.Timeout = mTimeoutMs
End Property

Public Function WriteText(ByVal s As String) As Boolean
    On Error GoTo ERR_
    io.WriteString s
    WriteText = True
    Exit Function
ERR_:
    WriteText = False
End Function

' VISA-COM は in_waiting 相当が乏しいため、ReadUpTo 方式でまとめて取得
Public Function ReadUpTo(ByVal count As Long) As String
    On Error GoTo ERR_
    ' バイナリ読み（終端文字非依存）
    Dim data() As Byte
    ReDim data(1 To count)
    Dim n As Long
    n = io.IO.Read(data, count)            ' 読めたバイト数を返す
    If n <= 0 Then
        ReadUpTo = vbNullString
    Else
        ReadUpTo = StrConv(LeftB$(data, n), vbUnicode)
    End If
    Exit Function
ERR_:
    ReadUpTo = vbNullString
End Function

Public Sub ClearBuffer()
    On Error Resume Next
    io.IO.Clear
End Sub

Public Property Get IsOpen() As Boolean
    IsOpen = mIsOpen
End Property
