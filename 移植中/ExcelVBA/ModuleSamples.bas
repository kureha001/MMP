'==================================================================
' 4) シート連携サンプル
'==================================================================
Option Explicit

Public Sub サンプル_接続_取得_切断()
    Dim dev As New cMmpVba
    dev.Init 読込調整:=64, 速度:=115200, timeoutMs:=200

    If Not dev.通信接続() Then
        MsgBox "接続失敗", vbExclamation
        Exit Sub
    End If

    ' 例: アナログ設定→1回取得→シートへ
    With dev.Core
        .アナログ設定 2, 2, 100
        .アナログ読取
        Range("B2").Value = "FW " & .version
        Range("B4").Resize(.参加総人数, .スイッチ数).Value = .mmpAnaVal
        ' PWM スイープ例
        .PWM_POWER 2, True
        Dim v As Long
        For v = 120 To 380 Step 4
            .PWM_VALUE 3, v
            DoEvents
        Next
        .PWM_VALUE 3, 0
        .PWM_POWER 2, False
    End With

    dev.通信切断
    MsgBox "完了", vbInformation
End Sub
