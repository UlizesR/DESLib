; Test bitwise operations
start:
; Test AND operation
MOV R0, 10         ; R0 = 10 (binary: 1010)
MOV R1, 12         ; R1 = 12 (binary: 1100)
AND R2, R0, R1     ; R2 = R0 & R1 = 8 (binary: 1000)
SYS R2, PRINT      ; Print R2 (should be 8)

; Test OR operation
OR R3, R0, R1      ; R3 = R0 | R1 = 14 (binary: 1110)
SYS R3, PRINT      ; Print R3 (should be 14)

; Test XOR operation
XOR R4, R0, R1     ; R4 = R0 ^ R1 = 6 (binary: 0110)
SYS R4, PRINT      ; Print R4 (should be 6)

; Test NOT operation
NOT R5, R0         ; R5 = ~R0 = 0xFFFFFFF5
SYS R5, PRINT      ; Print R5

; Test SHL operation
MOV R6, 2          ; R6 = 2
SHL R7, R0, R6     ; R7 = R0 << R6 = 10 << 2 = 40
SYS R7, PRINT      ; Print R7 (should be 40)

; Test SHR operation
SHR R8, R7, R6     ; R8 = R7 >> R6 = 40 >> 2 = 10
SYS R8, PRINT      ; Print R8 (should be 10)

HALT
