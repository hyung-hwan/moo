.386P

NAME HLT

PROG_CODE SEGMENT ER USE32 'CODE'
ASSUME CS:PROG_CODE

PUBLIC _halt_cpu

_halt_cpu PROC NEAR
	HLT ; this will cause #GP(0) as it's privileged. TODO: find a way to raise privilege level.
	RET
_halt_cpu ENDP

PROG_CODE ENDS

END
