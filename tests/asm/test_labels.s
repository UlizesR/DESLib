; Test file for label functionality
start: ; Main entry point
MOV R0, 5 ; Initialize counter (reduced for testing)
MOV R1, 0  ; Initialize sum

loop: ; Loop label
ADD R1, R1, R0 ; Add counter to sum
SUB R0, R0, 1  ; Decrement counter
CMP R0, 0      ; Check if counter is zero
JNZ loop       ; Continue loop if not zero

done: ; End of loop
HALT ; Stop execution
