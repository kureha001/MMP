
1000 '<<SETTING>>
1010   TITLE="Digital I/O"
1020   WAIT1=1000
1030   WAIT12=0
1040   gosub 200
3000 '=================================
2010 '<<COMMAND>>
2020 cmd="POW:0a:1!": gosub 300
2030 cmd="POR:0a!"  : gosub 300
