PRINTS "=== Hello World Program ==="
PRINTS "Welcome to the Dez Virtual Machine!"
PRINTS ""
PRINTS "This program demonstrates:"
PRINTS "- String printing with PRINTS instruction"
PRINTS "- Numeric printing with PRINT instruction"
PRINTS "- Mixed output capabilities"
PRINTS ""

MOV R0, #42
PRINT R0

PRINTS "The answer to everything is above!"
PRINTS ""

MOV R1, #100
MOV R2, #200
ADD R3, R1, R2
PRINTS "100 + 200 ="
PRINT R3

PRINTS ""
PRINTS "Program completed successfully!"
PRINTS "Thank you for using Dez VM!"

HALT
