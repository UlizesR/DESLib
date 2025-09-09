; Test file for arithmetic operations
; This file tests ADD, SUB, MUL, and DIV instructions

; Test addition: 15 + 25 = 40
MOV R0, #15
MOV R1, #25
ADD R2, R0, R1
PRINT R2

; Test subtraction: 50 - 30 = 20
MOV R3, #50
MOV R4, #30
SUB R5, R3, R4
PRINT R5

; Test multiplication: 6 * 7 = 42
MOV R6, #6
MOV R7, #7
MUL R0, R6, R7
PRINT R0

; Test division: 84 / 4 = 21
MOV R1, #84
MOV R2, #4
DIV R3, R1, R2
PRINT R3

; Test with negative results: 10 - 20 = -10
MOV R4, #10
MOV R5, #20
SUB R6, R4, R5
PRINT R6

; Test division with remainder: 17 / 3 = 5 (integer division)
MOV R7, #17
MOV R0, #3
DIV R1, R7, R0
PRINT R1

HALT