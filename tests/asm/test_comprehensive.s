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

    ; Arithmetic with immediate values
    ADD R2, R0, 5       ; R2 = 10 + 5 = 15
    SUB R3, R0, 3       ; R3 = 10 - 3 = 7
    MUL R4, R0, 4       ; R4 = 10 * 4 = 40
    DIV R5, R0, 2       ; R5 = 10 / 2 = 5

    ; Number formats (decimal, hex, binary)
    MOV R0, 42          ; Decimal
    MOV R1, 0x2A        ; Hex 42
    MOV R2, 0b101010    ; Binary 42
    ADD R3, R0, R1      ; 42 + 42 = 84
    ADD R4, R0, R2      ; 42 + 42 = 84

    ; Edge cases
    MOV R5, 0           ; Zero
    MOV R6, 0x0         ; Hex zero
    MOV R7, 0b0         ; Binary zero
    DIV R8, R0, 1       ; Division by 1
    MUL R9, R0, 0       ; Multiplication by 0

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

    ; Unconditional jumps
    MOV R2, 1
    JMP skip_unconditional
    MOV R3, 2           ; This should be skipped
skip_unconditional:
    MOV R4, 3           ; This should execute

    ; Conditional jumps
    MOV R5, 5
    MOV R6, 5
    CMP R5, R6          ; 5 == 5
    JZ equal_case
    MOV R7, 0           ; Should be skipped
    JMP end_conditional
equal_case:
    MOV R7, 1           ; Should execute
end_conditional:

    ; Loop functionality
    MOV R8, 0           ; Counter
    MOV R9, 0           ; Sum
loop_start:
    ADD R9, R9, 2       ; Add 2 to sum
    ADD R8, R8, 1       ; Increment counter
    CMP R8, 3           ; Check if counter == 3
    JZ loop_end
    JMP loop_start
loop_end:

    ; System calls
    SYS R0, PRINT       ; Print R0 (10)
    SYS R1, PRINT       ; Print R1 (20)
    MOV R10, 65
    SYS R10, PRINT_CHAR ; Print character 'A'

    HALT

my_function:
    ADD R0, R0, 5       ; R0 = 15
    SYS R0, PRINT       ; Print R0 (15)
    RET
