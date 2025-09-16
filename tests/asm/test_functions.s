; Test CALL/RET function operations
start:
MOV R0, 10
SYS R0, PRINT      ; Print 10

CALL add_five      ; Call function

MOV R1, 20
SYS R1, PRINT      ; Print 20

HALT

; Function that adds 5 to R0
add_five:
MOV R2, 5
ADD R0, R0, R2     ; R0 = R0 + 5
SYS R0, PRINT      ; Print R0 (should be 15)
RET                ; Return to caller
