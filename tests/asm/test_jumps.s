; Test unconditional jump
start: ; Entry point
MOV R0, 1 ; Set R0 to 1
JMP skip ; Jump to skip label
MOV R1, 2 ; This should be skipped
skip: ; Jump target
MOV R2, 3 ; Set R2 to 3
HALT ; Stop execution
