#as:
#objdump: -dr

.*:[    ]+file format .*


Disassembly of section .text:

.* <.text>:
[ 	]+0:[ 	]+1e000001[ 	]+pcaddu18i[ 	]+\$ra, 0
[ 	]+0: R_LARCH_CALL36[ 	]+a
[ 	]+4:[ 	]+4c000021[ 	]+jirl[ 	]+\$ra, \$ra, 0
[ 	]+8:[ 	]+1e00000c[ 	]+pcaddu18i[ 	]+\$t0, 0
[ 	]+8: R_LARCH_CALL36[ 	]+a
[ 	]+c:[ 	]+4c000180[ 	]+jr[ 	]+\$t0
