'==================================================================
' 2) Core（プロトコル共通）
'==================================================================
Option Explicit

Private mAdp As Object ' cVisaComAdapter 等（I/O 抽象）
Private mReadChunk As Long
Private mTimeoutMs As Long

' 公開状態
Public 参加総人数 As Long
Public スイッチ数 As Long
Public 丸め As Long
Public mmpAnaVal() As Variant
Public PWM機器状況(0 To 15) As Variant
Public version As String
Public 接続済 As Boolean

Public Sub InitCore(ByVal adapter As Object, _
                    Optional ByVal 読込調整 As Long = 64, _
                    Optional ByVal timeoutMs As Long = 100)
    Set mAdp = adapter
    mReadChunk = IIf(読込調整 > 0, 読込調整, 64)
    mTimeoutMs = timeoutMs
    接続済 = False
End Sub

' === 時間系 ===
Private Declare PtrSafe Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)
Private Function NowMs() As Double
    NowMs = Timer * 1000#
End Function
Private Function TimeLeftMs(ByVal deadline As Double) As Double
    TimeLeftMs = deadline - NowMs()
End Function

' === 受信 ===
Private Function _受信() As String
    Dim deadline As Double: deadline = NowMs() + mTimeoutMs
    Dim buf As String: buf = ""

    Do While TimeLeftMs(deadline) > 0
        Dim chunk As String
        chunk = mAdp.ReadUpTo(mReadChunk)   ' まとめ読み
        If LenB(chunk) = 0 Then
            Sleep 1&
        Else
            buf = Right$(buf & chunk, 5)    ' 末尾5バイトだけ保持
            If Len(buf) = 5 And Right$(buf, 1) = "!" Then
                _受信 = buf
                Exit Function
            End If
        End If
        DoEvents
    Loop
    _受信 = vbNullString
End Function

Private Function _送信(ByVal s As String) As String
    If Not mAdp.WriteText(s) Then Exit Function
    _送信 = _受信()
End Function

' === バージョン ===
Public Function バージョン確認() As Boolean
    Dim r$: r = _送信("VER!")
    If Len(r) = 5 And Right$(r, 1) = "!" Then
        version = Mid$(r, 1, 1) & "." & Mid$(r, 2, 1) & "." & Mid$(r, 3, 2)
        バージョン確認 = True
    Else
        version = "?.?.??"
        バージョン確認 = False
    End If
End Function

' === アナログ ===
Public Sub アナログ設定(ByVal ス As Long, ByVal 参 As Long, ByVal 丸 As Long)
    スイッチ数 = ス: 参加総人数 = 参: 丸め = 丸
    ReDim mmpAnaVal(0 To 参 - 1, 0 To ス - 1)
    Call _送信("ANS:" & Hex2(参) & ":" & Hex2(ス) & "!")
End Sub

Public Sub アナログ読取()
    Dim _ As String: _ = _送信("ANU!")
    Dim ch As Long, bd As Long, r$, v&
    For ch = 0 To 参加総人数 - 1
        For bd = 0 To スイッチ数 - 1
            r = _送信("ANR:" & Hex2(ch) & ":" & Hex2(bd) & "!")
            If Len(r) = 5 And Right$(r, 1) = "!" Then
                v = Val("&H" & Left$(r, 4))
                If 丸め > 0 Then v = (v \ 丸め) * 丸め
                mmpAnaVal(ch, bd) = v
            Else
                mmpAnaVal(ch, bd) = Empty
            End If
        Next
    Next
End Sub

' === PWM ===
Public Function PWM_VALUE(ByVal ch As Long, ByVal val As Long) As Boolean
    Dim r$: r = _送信("PWM:" & Hex2(ch) & ":" & Hex4(val) & "!")
    PWM_VALUE = (r = "!!!!!")
End Function

Public Function PWM_INIT(ByVal aMin As Long, ByVal aMax As Long, ByVal pMin As Long, ByVal pMax As Long) As Boolean
    Dim r$: r = _送信("PWI:" & Hex3(aMin) & ":" & Hex3(aMax) & ":" & Hex4(pMin) & ":" & Hex4(pMax) & "!")
    PWM_INIT = (r = "!!!!!")
End Function

Public Function PWM_ANGLE(ByVal ch As Long, ByVal deg As Long) As Boolean
    Dim r$: r = _送信("PWA:" & Hex2(ch) & ":" & Hex3(deg) & "!")
    PWM_ANGLE = (r = "!!!!!")
End Function

Public Sub PWM_POWER(ByVal ch As Long, ByVal sw As Boolean)
    Call PWM_VALUE(ch, IIf(sw, &HFFF, 0))
End Sub

' === デジタル / MP3 / I2C も同様に実装（省略） ===

' === HEX 整形 ===
Private Function Hex2(ByVal v As Long) As String: Hex2 = Right$("00" & Hex$(v And &HFF), 2): End Function
Private Function Hex3(ByVal v As Long) As String: Hex3 = Right$("000" & Hex$(v And &H3FF), 3): End Function
Private Function Hex4(ByVal v As Long) As String: Hex4 = Right$("0000" & Hex$(v And &HFFFF), 4): End Function
