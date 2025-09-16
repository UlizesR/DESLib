; Test conditional jump
start: ; Entry point
MOV R0, 5 ; Set R0 to 5
MOV R1, 5 ; Set R1 to 5 (equal to R0)
CMP R0, R1 ; Compare R0 and R1
JZ equal ; Jump if equal
MOV R2, 1 ; This should be skipped
MOV R3, 10 ; This should be skipped
JMP end ; Jump to end
equal: ; Jump target for equal case
MOV R3, 10 ; Set R3 to 10
end: ; End label
HALT ; Stop execution
