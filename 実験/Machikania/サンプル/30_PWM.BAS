1000 '<<SETTING>>
1010   TITLE="PWM(PCA9865)"
1020   WAIT1=0
1030   WAIT2=250
1040   gosub 200
3000 '=================================
2010 '<<COMMAND>>
2020   cmd="PWM:1:0FFF!": gosub 300
2030   cmd="PWM:0:0D00!": gosub 300
2040   pause WAIT2
2050   cmd="PWM:1:-1!"  : gosub 300
2060   cmd="PWM:0:-1!"  : gosub 300