'==================================================================
' 3) Wrapper（CPython と同じ UX）
'==================================================================
cMmpVba.cls
Option Explicit

Private adp As cVisaComAdapter
Private core As cMmpCore

Public Sub Init(Optional ByVal 読込調整 As Long = 64, _
                Optional ByVal 速度 As Long = 115200, _
                Optional ByVal timeoutMs As Long = 200)
    Set adp = New cVisaComAdapter
    Set core = New cMmpCore
    core.InitCore adp, 読込調整, timeoutMs
End Sub

Public Function 通信接続(Optional ByVal resource As String = vbNullString, _
                        Optional ByVal 速度 As Long = 115200, _
                        Optional ByVal timeoutMs As Long = 200) As Boolean
    If resource = vbNullString Then
        ' ASRL* 自動探索（最初の1件）
        Dim rm As New VisaComLib.ResourceManager
        Dim lst As Variant: lst = rm.FindRsrc("ASRL?*INSTR")
        If IsArray(lst) Then resource = CStr(lst(0)) Else resource = "ASRL5::INSTR"
    End If
    If Not adp.OpenRsrc(resource, 速度, timeoutMs) Then GoTo NG
    If Not core.バージョン確認() Then GoTo NG
    core.接続済 = True
    通信接続 = True: Exit Function
NG:
    Call 通信切断
    通信接続 = False
End Function

Public Sub 通信切断()
    On Error Resume Next
    adp.CloseRsrc
    core.接続済 = False
End Sub

' --- コア API をそのまま露出 ---
Public Property Get Core() As cMmpCore: Set Core = core: End Property
