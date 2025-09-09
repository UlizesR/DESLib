; Test file for memory operations
; This file tests LOAD and STORE instructions

; Store values in memory
MOV R0, #100
MOV R1, #200
STORE R0, [10]    ; Store 100 at memory address 10
STORE R1, [20]    ; Store 200 at memory address 20

; Load values from memory
LOAD R2, [10]     ; Load from address 10 into R2
LOAD R3, [20]     ; Load from address 20 into R3

; Print loaded values
PRINT R2          ; Should print 100
PRINT R3          ; Should print 200

; Test indirect addressing
MOV R4, #30       ; R4 = 30 (address)
MOV R5, #300      ; R5 = 300 (value)
STORE R5, [R4]    ; Store 300 at address in R4 (30)
LOAD R6, [R4]     ; Load from address in R4 into R6
PRINT R6          ; Should print 300

HALT
