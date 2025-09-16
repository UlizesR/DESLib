; Basic arithmetic operations test
start: ; Entry point
MOV R0, 42 ; Load first operand
MOV R1, 10 ; Load second operand
ADD R2, R0, R1 ; Add: R2 = R0 + R1 = 52
SUB R3, R0, R1 ; Subtract: R3 = R0 - R1 = 32
MUL R4, R0, R1 ; Multiply: R4 = R0 * R1 = 420
DIV R5, R0, R1 ; Divide: R5 = R0 / R1 = 4
HALT ; Stop execution
