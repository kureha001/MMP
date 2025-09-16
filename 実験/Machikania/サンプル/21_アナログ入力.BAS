1000 '<<SETTING>>
1010   TITLE="JOY PAD(HC4067)"
1020   WAIT1=0
1030   WAIT2=250
1040   gosub 200
3000 '=================================
2010 '<<COMMAND>>
2020   cmd="ANS:1:4!"   : gosub 300
2030   FOR X=1 TO 50
2040     cmd="ANU!"     : gosub 300
2050    cmd="ANR:00:0!": gosub 300
2060     cmd="ANR:00:1!": gosub 300
2070     cmd="ANR:00:2!": gosub 300
2080     cmd="ANR:00:3!": gosub 300
2090     pause WAIT2
2100   NEXT