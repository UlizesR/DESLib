; Debug program to check memory size
PRINTS "=== Memory Debug ==="

; Try to access memory at different addresses
MOV R0, #1000
STORE R0, [1000]
PRINTS "Stored 1000 at address 1000 - SUCCESS"

MOV R0, #2000
STORE R0, [2000]
PRINTS "Stored 2000 at address 2000 - SUCCESS"

MOV R0, #3000
STORE R0, [3000]
PRINTS "Stored 3000 at address 3000 - SUCCESS"

MOV R0, #4000
STORE R0, [4000]
PRINTS "Stored 4000 at address 4000 - SUCCESS"

MOV R0, #5000
STORE R0, [5000]
PRINTS "Stored 5000 at address 5000 - SUCCESS"

PRINTS "Memory test completed!"
HALT
