100 '<<COMMON:INITIALIZE>>
110   CLS
120   UART 115200,0,1024
130   goto 1000
200 '=================================
210 '<<COMMON:SETUP>>
220   TITLE=" " + TITLE + " "
230   strline=""
240   for I=1 to 5+2
250     strline=strline + "-"
260   next
270   ? strline: ? TITLE: ? strline
280   return
300 '=================================
310 '<<COMMON:UART>>
320   uartput 1,cmd
330   ret=""
340   for I=1 to 5
350     ret=ret + uartget(1)
360   next
370   ? cmd + " : " + ret
380   pause WAIT1
390   return
900 '=================================
