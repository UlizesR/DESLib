; Test PUSH/POP stack operations
start:
; Push some values to stack
MOV R0, 42
PUSH R0
MOV R1, 100
PUSH R1
MOV R2, 200
PUSH R2

; Pop values back (should be in reverse order)
POP R3        ; R3 should be 200
SYS R3, PRINT ; Print R3

POP R4        ; R4 should be 100
SYS R4, PRINT ; Print R4

POP R5        ; R5 should be 42
SYS R5, PRINT ; Print R5

; Test stack operations with arithmetic
MOV R6, 10
PUSH R6
MOV R7, 20
PUSH R7

; Add values on stack
POP R8        ; R8 = 20
POP R9        ; R9 = 10
ADD R10, R8, R9  ; R10 = 20 + 10 = 30
SYS R10, PRINT   ; Print R10 (should be 30)

HALT
