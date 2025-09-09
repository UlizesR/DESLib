; Test file for control flow operations
; This file tests JMP, JZ, and JNZ instructions

; Test unconditional jump
MOV R0, #10
JMP skip1
MOV R0, #20      ; This should be skipped
skip1:
PRINT R0         ; Should print 10

; Test conditional jump (JZ) - should not jump
MOV R1, #5
CMP R1, #0       ; R1 != 0, so zero flag not set
JZ skip2
MOV R1, #15      ; This should execute
skip2:
PRINT R1         ; Should print 15

; Test conditional jump (JZ) - should jump
MOV R2, #0
CMP R2, #0       ; R2 == 0, so zero flag is set
JZ skip3
MOV R2, #25      ; This should be skipped
skip3:
PRINT R2         ; Should print 0

; Test conditional jump (JNZ) - should jump
MOV R3, #7
CMP R3, #0       ; R3 != 0, so zero flag not set
JNZ skip4
MOV R3, #35      ; This should be skipped
skip4:
PRINT R3         ; Should print 7

; Test conditional jump (JNZ) - should not jump
MOV R4, #0
CMP R4, #0       ; R4 == 0, so zero flag is set
JNZ skip5
MOV R4, #45      ; This should execute
skip5:
PRINT R4         ; Should print 45

HALT
