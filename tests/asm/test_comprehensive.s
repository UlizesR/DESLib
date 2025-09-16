; Comprehensive test for all Dez VM features
; This file tests all the new features we implemented

start:
    ; Basic arithmetic
    MOV R0, 10
    MOV R1, 5
    ADD R2, R0, R1      ; R2 = 15
    SUB R3, R0, R1      ; R3 = 5
    MUL R4, R0, R1      ; R4 = 50
    DIV R5, R0, R1      ; R5 = 2

    ; Memory operations
    STORE R2, 256       ; Store 15 at address 256
    LOAD R6, 256        ; R6 = 15

    ; Bitwise operations
    MOV R7, 10          ; R7 = 10 (1010 in binary)
    MOV R8, 12          ; R8 = 12 (1100 in binary)
    AND R9, R7, R8      ; R9 = 8 (1000 in binary)
    OR R10, R7, R8      ; R10 = 14 (1110 in binary)
    XOR R11, R7, R8     ; R11 = 6 (0110 in binary)
    NOT R12, R7         ; R12 = ~10
    SHL R13, R7, R8     ; R13 = 10 << 12
    SHR R14, R7, R8     ; R14 = 10 >> 12

    ; Stack operations
    PUSH R0             ; Push 10
    PUSH R1             ; Push 5
    POP R15             ; R15 = 5
    POP R0              ; R0 = 10

    ; Function call
    CALL my_function
    MOV R1, 20          ; This should execute after return

    ; Conditional jumps
    MOV R2, 10
    MOV R3, 5
    CMP R2, R3          ; 10 > 5
    JG greater_case
    MOV R4, 0           ; Should be skipped
    JMP end_conditional
greater_case:
    MOV R4, 1           ; Should execute
end_conditional:

    ; System calls
    SYS R0, PRINT       ; Print R0 (10)
    SYS R1, PRINT       ; Print R1 (20)

    HALT

my_function:
    ADD R0, R0, 5       ; R0 = 15
    SYS R0, PRINT       ; Print R0 (15)
    RET
