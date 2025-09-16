; Test file with labels and comments mixed
start: ; Entry point with comment
MOV R0, 5    ; Initialize R0
MOV R1, 10   ; Initialize R1

; Check if R0 equals R1
CMP R0, R1   ; Compare values
JZ equal     ; Jump if equal
JMP not_equal ; Jump if not equal

equal: ; Labels are equal
MOV R2, 1    ; Set result to 1
JMP end      ; Jump to end

not_equal: ; Labels are not equal
MOV R2, 0    ; Set result to 0

end: ; End of program
HALT ; Stop execution
