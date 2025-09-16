; Test LOAD instruction
start:
MOV R0, 42        ; Load immediate value 42 into R0
STORE R0, 256     ; Store R0 to memory address 256
MOV R1, 0         ; Clear R1
LOAD R1, 256      ; Load from memory address 256 into R1
SYS R1, PRINT     ; Print R1 (should be 42)
HALT
