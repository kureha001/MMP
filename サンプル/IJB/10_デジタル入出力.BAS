100 CLS:CLV
110 ?"MMP Digital I/O"
120 ?"-------------------"
130 UART1
140 ?"VER!";:GSB900
150 '----------
200 ? "POW:0a:1!";:GSB900
210 ? "POR:0a!"  ;:GSB900
700 '----------
800 ?"VER!";:GSB900
810 UART0:?"":?"- END -":END
820 '----------
900 WAIT5
910 FOR I=0 TO 4:[I]=INKEY():NEXT
920 UART0:?" = ";
930 FOR I=0 TO 4:?CHR$([I]);:NEXT
940 ?"":UART1:RTN
