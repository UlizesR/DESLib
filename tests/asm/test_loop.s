start:
MOV R0, 0
MOV R1, 0
loop:
ADD R1, R1, 2
ADD R0, R0, 1
CMP R0, 3
JZ end
JMP loop
end:
HALT
