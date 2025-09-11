; Test program for nested subroutine calls
; Demonstrates CALL and RET with multiple levels of nesting

PRINTS "=== Nested Subroutine Test ==="
PRINTS "Testing nested CALL and RET instructions"
PRINTS ""

; Main program
MOV R0, #5
PRINTS "Initial value: R0 ="
PRINT R0

; Call first level subroutine
CALL level1

PRINTS "Final value: R0 ="
PRINT R0

PRINTS "Test completed successfully!"
HALT

; Level 1 subroutine
level1:
    PRINTS "  In level1, R0 ="
    PRINT R0
    
    ; Call level 2 subroutine
    CALL level2
    
    ; Multiply by 2
    MOV R1, #2
    MUL R0, R0, R1
    PRINTS "  After level2, R0 ="
    PRINT R0
    RET

; Level 2 subroutine
level2:
    PRINTS "    In level2, R0 ="
    PRINT R0
    
    ; Call level 3 subroutine
    CALL level3
    
    ; Add 10
    ADD R0, R0, #10
    PRINTS "    After level3, R0 ="
    PRINT R0
    RET

; Level 3 subroutine
level3:
    PRINTS "      In level3, R0 ="
    PRINT R0
    
    ; Add 5
    ADD R0, R0, #5
    PRINTS "      After adding 5, R0 ="
    PRINT R0
    RET
