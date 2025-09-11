; Test program for CALL and RET instructions
; This program demonstrates subroutine calls and returns

PRINTS "=== Subroutine Test Program ==="
PRINTS "Testing CALL and RET instructions"
PRINTS ""

; Main program
MOV R0, #10
PRINTS "Before calling subroutine: R0 ="
PRINT R0

; Call subroutine to double the value
CALL double_value

PRINTS "After calling subroutine: R0 ="
PRINT R0

; Call another subroutine
CALL print_message

PRINTS "Program completed successfully!"
HALT

; Subroutine to double a value
double_value:
    MOV R1, #2
    MUL R0, R0, R1
    RET

; Subroutine to print a message
print_message:
    PRINTS "This message is from a subroutine!"
    RET
