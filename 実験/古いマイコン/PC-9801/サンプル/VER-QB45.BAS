'========================================================
' PC-98 QuickBasic 4.5  (9600 / 8N1)
' 送信: 末尾'!'を必ず付与、総バイト<=30、改行なし
' 送信前: 受信バッファを無条件で全消去
' 受信: 5バイト固定、5バイト目が'!'のみ有効（簡易TO）
' 入力に END で終了
'========================================================
CONST PORT$      = "COM1:"            ' 例: "COM2:" に変更可
CONST BAUD$      = "9600,N,8,1"       ' 速度を変えるときはここ
CONST TIMEOUT!   = 1.5                ' 秒（必要に応じ調整）
CONST MAX_TOTAL% = 30

OPEN PORT$ + BAUD$ FOR RANDOM AS #1

PRINT "Type message then [Enter] to send. Type END to quit."
PRINT "No CR/LF is sent. Max length (including '!') = 30."
PRINT

DO
  LINE INPUT "> "; msg$
  IF UCASE$(RTRIM$(msg$)) = "END" THEN EXIT DO
  IF LEN(msg$) = 0 THEN PRINT "(empty skipped)": GOTO ContinueLoop

  out$ = EnsureBangAndClamp$(msg$, MAX_TOTAL%)
  IF out$ = "" THEN PRINT "Too long.": GOTO ContinueLoop

  '--- 送信直前: 受信バッファ全消去 ---
  FlushRxAll 1

  '--- 改行なし送信（末尾 ; が重要） ---
  PRINT #1, out$;
  PRINT "TX ["; out$; "] ("; LEN(out$); "B)"

  '--- 5バイト固定応答（5B目が '!' のみ採用） ---
  r$ = Recv5Frame$(TIMEOUT!)
  IF r$ = "" THEN PRINT "RX [timeout]" ELSE PRINT "RX ["; r$; "]"

ContinueLoop:
LOOP

CLOSE #1
PRINT "Bye."
END

'----------------- 受信バッファ全消去 -------------------
SUB FlushRxAll (chn%)
  DO
    n% = LOC(chn%)
    IF n% > 0 THEN dummy$ = INPUT$(n%, #chn%) ELSE EXIT DO
  LOOP
END SUB

'----------------- 末尾'!'保証＋30B上限 -----------------
FUNCTION EnsureBangAndClamp$ (body$, maxTotal%)
  s$ = body$
  IF LEN(s$) = 0 THEN EnsureBangAndClamp$ = "!" : EXIT FUNCTION
  IF RIGHT$(s$, 1) <> "!" THEN s$ = s$ + "!"
  IF LEN(s$) > maxTotal% THEN
     EnsureBangAndClamp$ = RIGHT$(s$, maxTotal%)
  ELSE
     EnsureBangAndClamp$ = s$
  END IF
END FUNCTION

'----------------- 5バイト固定受信（末尾'!'必須） -------
FUNCTION Recv5Frame$ (timeoutSec!)
  deadline! = TIMER + timeoutSec!
  buf$ = ""
  DO
    IF LOC(1) > 0 THEN
      buf$ = buf$ + INPUT$(LOC(1), #1)
      IF LEN(buf$) >= 5 THEN
        f$ = LEFT$(buf$, 5)
        IF RIGHT$(f$, 1) = "!" THEN Recv5Frame$ = f$: EXIT FUNCTION
        buf$ = MID$(buf$, 6)   ' ずれた5Bを捨てて再同期
      END IF
    END IF
    IF TIMER >= deadline! THEN EXIT DO
  LOOP
  Recv5Frame$ = ""
END FUNCTION
