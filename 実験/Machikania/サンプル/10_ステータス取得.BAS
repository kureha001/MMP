1000 '<<SETTING>>
1010   TITLE="INFOMATION"
1020   WAIT1=50
1030   WAIT12=0
1040   gosub 200
3000 '=================================
2010 '<<COMMAND>>
2020   cmd="VER!"  : gosub 300
2030   cmd="PWX:1!": gosub 300
2040   cmd="PWX:2!": gosub 300
2050   cmd="FDX:1!": gosub 300
2060   cmd="FDX:2!": gosub 300