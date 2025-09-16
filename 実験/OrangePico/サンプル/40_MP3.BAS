1000 '<<SETTING>>
1010   TITLE="mp3(DFPlayer)"
1020   WAIT1=30000
1030   WAIT12=0
1040   gosub 200
3000 '=================================
2010 '<<COMMAND>>
2020   cmd="VOL:20!"   : gosub 300
2030   cmd="DIR:1,4,1!": gosub 300
2040   cmd="DIR:1,4,2!": gosub 300
2050   cmd="DIR:1,4,3!": gosub 300
2060   WAIT1=0
2070   cmd="DSP!"      : gosub 300