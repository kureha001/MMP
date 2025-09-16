'========================================================
' PC-98 N88-BASIC (Disk) 9600/8N1 送受信ユーティリティ
'  - 送信: 末尾'!'付与、総バイト<=30、改行なし（;）
'  - 送信前: 受信バッファ全消去
'  - 受信: 5バイト固定、5バイト目'!'のみ採用（簡易タイムアウト）
'  - 入力に END で終了
'========================================================
10 DEV$ = "COM1:"                ' 必要なら "COM2:" に変更
20 SPD$ = "9600,N,8,1"
30 TIMEOUT! = 1.5
40 MAXTOTAL% = 30
50 OPEN DEV$ + SPD$ AS #1
60 PRINT "Type message then [Enter] to send. Type END to quit."
70 PRINT "No CR/LF is sent. Max length (including '!') = 30."
80 PRINT

90 INPUT LINE "> "; MSG$
100 IF MSG$="END" OR MSG$="end" THEN GOTO 900
110 IF LEN(MSG$)=0 THEN PRINT "(empty skipped)": GOTO 90

'--- 末尾'!'保証＋30バイト以内 ---
120 S$ = MSG$
130 IF RIGHT$(S$,1) <> "!" THEN S$ = S$ + "!"
140 IF LEN(S$) > MAXTOTAL% THEN S$ = RIGHT$(S$, MAXTOTAL%)

'--- 送信前：受信バッファ全消去 ---
150 GOSUB 400

'--- 改行なし送信 ---
160 PRINT #1, S$;
170 PRINT "TX ["; S$; "] ("; STR$(LEN(S$)); "B)"

'--- 5バイト固定受信（末尾'!'必須） ---
180 GOSUB 600
190 IF RX$="" THEN PRINT "RX [timeout]" ELSE PRINT "RX ["; RX$; "]"
200 GOTO 90

'==== 受信バッファ全消去 ====
400 N% = LOC(1)
410 IF N% > 0 THEN D$ = INPUT$(N%, #1): GOTO 400
420 RETURN

'==== 5バイト固定受信（末尾'!'必須・簡易タイムアウト） ====
600 DEADLINE! = TIMER + TIMEOUT!
610 BUF$ = ""
620 IF LOC(1) > 0 THEN BUF$ = BUF$ + INPUT$(LOC(1), #1)
630 IF LEN(BUF$) >= 5 THEN
640   F$ = LEFT$(BUF$,5)
650   IF RIGHT$(F$,1) = "!" THEN RX$ = F$: RETURN
660   BUF$ = MID$(BUF$,6)       ' ずれた5Bは破棄して再同期
670 END IF
680 IF TIMER < DEADLINE! THEN GOTO 620
690 RX$ = ""
700 RETURN

'==== 終了処理 ====
900 CLOSE #1
910 PRINT "Bye."
920 END
