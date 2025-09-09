; Test file for stack operations
; This file tests PUSH and POP instructions

; Push some values onto the stack
MOV R0, #100
MOV R1, #200
MOV R2, #300

PUSH R0          ; Push 100
PUSH R1          ; Push 200
PUSH R2          ; Push 300

; Clear registers to test that values are on stack
MOV R0, #0
MOV R1, #0
MOV R2, #0

; Pop values back from stack (should be in reverse order)
POP R3           ; Should get 300
POP R4           ; Should get 200
POP R5           ; Should get 100

; Print popped values
PRINT R3         ; Should print 300
PRINT R4         ; Should print 200
PRINT R5         ; Should print 100

; Test stack with arithmetic
MOV R6, #10
MOV R7, #20
ADD R0, R6, R7   ; R0 = 30
PUSH R0          ; Push 30
MOV R0, #0       ; Clear R0
POP R1           ; Pop 30 into R1
PRINT R1         ; Should print 30

HALT
