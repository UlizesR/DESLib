; Test program to demonstrate 8KB memory capacity
; This program uses memory addresses that were impossible with 1KB

PRINTS "=== 8KB Memory Test ==="
PRINTS "Testing memory access at high addresses"
PRINTS ""

; Test memory access at various addresses
MOV R0, #0x1234        ; Test value
STORE R0, [2000]       ; Store at address 2000 (was impossible with 1KB)
PRINTS "Stored 0x1234 at address 2000"

; Test memory access at high address (within valid range)
MOV R1, #0x5678        ; Another test value
STORE R1, [7000]       ; Store at address 7000 (valid for 8KB memory)
PRINTS "Stored 0x5678 at address 7000"

; Verify the values
LOAD R2, [2000]
PRINTS "Read back from 2000:"
PRINT R2

LOAD R3, [7000]
PRINTS "Read back from 7000:"
PRINT R3

; Test stack operations with more space
PRINTS ""
PRINTS "Testing stack with 8KB memory:"
MOV R0, #100
MOV R1, #200
MOV R2, #300
MOV R3, #400
MOV R4, #500

; Push multiple values
PUSH R0
PUSH R1
PUSH R2
PUSH R3
PUSH R4

PRINTS "Pushed 5 values onto stack"

; Pop and print them
POP R5
PRINT R5
POP R6
PRINT R6
POP R7
PRINT R7
POP R0
PRINT R0
POP R1
PRINT R1

PRINTS ""
PRINTS "8KB memory test completed successfully!"
PRINTS "Stack pointer is now at:"
PRINTS "This demonstrates the increased memory capacity!"

HALT
