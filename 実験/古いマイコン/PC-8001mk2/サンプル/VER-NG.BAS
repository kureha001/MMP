'==========================================
' PC-8001mkII  RS-232C 9600/8N1（USR併用）
' 送信: 末尾'!'付与/30B上限/改行なし
' 送信前: USR2で受信バッファ全消去
' 受信: USR1で5B固定・5B目'!'のみ採用
' END入力で終了
'==========================================

10 DEFUSR0=&H8000  ' TX 1byte  (後でPOKEで配置)
20 DEFUSR1=&H8010  ' RX 1byte  (後でPOKEで配置)
30 DEFUSR2=&H8020  ' Flush RX  (後でPOKEで配置)
40 MAXLEN%=30

'--- (オプション) 初期化USRL（9600/8N1設定）を追加予定ならここで呼ぶ ---
'50 DEFUSR3=&H8030: X=USR3(0)

60 PRINT "Type message (END to quit). No CR/LF is sent."
70 PRINT "Max length including '!' = 30."
80 PRINT

90 LINE INPUT "> "; MSG$
100 IF UCASE$(MSG$)="END" THEN PRINT "Bye.": END
110 IF LEN(MSG$)=0 THEN PRINT "(empty skipped)": GOTO 90

'--- 末尾'!'保証 + 30B上限 ---
120 IF RIGHT$(MSG$,1)<>"!" THEN MSG$=MSG$+"!"
130 IF LEN(MSG$)>MAXLEN% THEN MSG$=RIGHT$(MSG$,MAXLEN%)

'--- 送信前に受信バッファ全消去 ---
140 D=USR2(0)

'--- 改行なしで送信（1文字ずつ） ---
150 FOR I%=1 TO LEN(MSG$)
160   C%=ASC(MID$(MSG$,I%,1))
170   X=USR0(C%)
180 NEXT I%
190 PRINT "TX [";MSG$;"] (";LEN(MSG$);"B)"

'--- 5バイト固定受信（5B目'!'必須） ---
200 RX$=""
210 FOR I%=1 TO 5
220   R%=USR1(0)
230   RX$=RX$+CHR$(R%)
240 NEXT I%
250 IF RIGHT$(RX$,1)<>"!" THEN
260   PRINT "RX [invalid frame]"; " -> [";RX$;"]"
270 ELSE
280   PRINT "RX [";RX$;"]"
290 END IF

300 GOTO 90
