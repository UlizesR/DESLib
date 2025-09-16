# DEZ VM - A Modern Virtual Machine

DEZ VM is a lightweight 32-bit virtual machine designed for educational purposes and simple program execution. It features a complete assembler toolchain with modern language features including labels, comments, and advanced instruction encoding.

## Features

- **32-bit RISC-like instruction set** with 16 general-purpose registers
- **Complete assembler toolchain** with lexer, parser, and symbol table
- **Label and comment support** for readable assembly code
- **Advanced instruction encoding** supporting both register-to-register and register-to-immediate operations
- **System calls** for I/O operations including string printing
- **Memory management** with code, data, and stack segments
- **Binary file format** for compiled programs
- **Comprehensive test suite** with CTest integration

## Architecture

### Memory Layout
- **Total Memory**: 16KB (4096 words)
- **Code Segment**: 0x0000 - 0x0FFF (4KB, read-only)
- **Data Segment**: 0x1000 - 0x1FFF (4KB, writable)
- **Stack Segment**: 0x2000 - 0x3FFF (8KB, writable)

### Registers
- **R0 - R15**: 16 general-purpose 32-bit registers
- **PC**: Program Counter
- **SP**: Stack Pointer
- **Flags**: Status flags (zero, carry, overflow)

## Building

```bash
# Create build directory
mkdir build && cd build

# Build the project
cmake ..
make

# All executables will be created in the bin/ directory
ls bin/
# asm dez_vm disasm_tool test_assembly test_core test_memory test_performance test_strings

# Run all tests
ctest --output-on-failure
```

## Usage

### 1. Writing Assembly Code

Create a `.s` file with your program:

```assembly
; examples/hello.s
start: ; Entry point label
MOV R0, 42 ; Load value 42 into R0
SYS R0, PRINT ; Print the value
HALT ; Stop execution
```

### 2. Assembling

Use the assembler to compile your assembly code:

```bash
# From the build directory
./bin/asm ../examples/hello.s hello.bin
```

### 3. Running

Execute the compiled binary with the VM:

```bash
# From the build directory
./bin/dez_vm hello.bin
```

### 4. Testing

Run the comprehensive test suite:

```bash
# Run all tests
ctest --output-on-failure

# Or run specific tests
./bin/test_assembly
./bin/test_core
```

## Instruction Set

### Data Movement
- `MOV R1, 42` - Load immediate value 42 into R1
- `MOV R1, R2` - Copy R2 to R1

### Arithmetic
- `ADD R1, R2, R3` - R1 = R2 + R3 (register-to-register)
- `ADD R1, R2, 5` - R1 = R2 + 5 (register-to-immediate)
- `SUB R1, R2, R3` - R1 = R2 - R3 (register-to-register)
- `SUB R1, R2, 3` - R1 = R2 - 3 (register-to-immediate)
- `MUL R1, R2, R3` - R1 = R2 * R3
- `DIV R1, R2, R3` - R1 = R2 / R3

### Memory Operations
- `STORE R1, 256` - Store R1 at memory address 256

### Control Flow
- `JMP label` - Jump to label
- `JZ label` - Jump if zero flag set
- `JNZ label` - Jump if zero flag not set

### System Calls
- `SYS R1, PRINT` - Print value in R1
- `SYS R1, PRINT_CHAR` - Print character in R1

### Other
- `CMP R1, R2` - Compare R1 and R2 (sets flags)
- `CMP R1, 5` - Compare R1 with immediate value 5 (sets flags)
- `HALT` - Halt execution
- `NOP` - No operation

### Labels and Comments
- `label:` - Define a label
- `; This is a comment` - Full-line comment
- `MOV R0, 42 ; Inline comment` - Inline comment

## Examples

### Hello World
```assembly
; Simple hello world program
start: ; Entry point
MOV R0, 42 ; Load value 42
SYS R0, PRINT ; Print the value
HALT ; Stop execution
```

### Arithmetic Operations
```assembly
; Basic arithmetic with immediate values
start:
MOV R0, 10 ; Load first operand
MOV R1, 20 ; Load second operand
ADD R2, R0, R1 ; Add: R2 = R0 + R1 = 30
SUB R3, R0, R1 ; Subtract: R3 = R0 - R1 = -10
HALT
```

### Loops and Labels
```assembly
; Loop to sum numbers 1 to 5
start:
MOV R0, 5 ; Initialize counter
MOV R1, 0 ; Initialize sum

loop: ; Loop label
ADD R1, R1, R0 ; Add counter to sum
SUB R0, R0, 1  ; Decrement counter
CMP R0, 0      ; Check if counter is zero
JNZ loop       ; Continue loop if not zero

done: ; End of loop
HALT ; Stop execution
```

### Conditional Logic
```assembly
; Conditional jump example
start:
MOV R0, 5 ; Set R0 to 5
MOV R1, 5 ; Set R1 to 5 (equal to R0)
CMP R0, R1 ; Compare R0 and R1
JZ equal ; Jump if equal
MOV R2, 1 ; This should be skipped
JMP end ; Jump to end
equal: ; Jump target for equal case
MOV R2, 1 ; Set R2 to 1
end: ; End label
HALT ; Stop execution
```

### Memory Operations
```assembly
; Store and retrieve values from memory
start:
MOV R0, 123 ; Load value 123
STORE R0, 256 ; Store R0 at address 256
MOV R1, 456 ; Load value 456
STORE R1, 257 ; Store R1 at address 257
HALT ; Stop execution
```

## Tools

### Assembler (`asm`)
Compiles assembly source files to binary format:
```bash
# From the build directory
./bin/asm ../examples/hello.s hello.bin
```

### Disassembler (`disasm_tool`)
Disassembles binary files back to assembly:
```bash
# From the build directory
./bin/disasm_tool hello.bin
```

### VM (`dez_vm`)
Executes compiled binary files:
```bash
# From the build directory
./bin/dez_vm hello.bin
```

### Test Suite
Comprehensive test suite with multiple test categories:
```bash
# Run all tests
ctest --output-on-failure

# Individual test executables
./bin/test_assembly  # Assembly integration tests
./bin/test_core      # Core VM functionality
./bin/test_memory    # Memory operations
./bin/test_performance # Performance benchmarks
./bin/test_strings   # String handling
```

## File Format

The binary format consists of:
1. **Header**: 4-byte instruction count
2. **Instructions**: 32-bit encoded instructions with advanced encoding:
   - **Register-to-register**: `(opcode << 24) | (reg1 << 20) | (reg2 << 16) | (reg3 << 12)`
   - **Register-to-immediate**: `(opcode << 24) | (reg1 << 20) | (reg2 << 16) | (1 << 11) | immediate`
   - **Bit 11 flag**: Distinguishes between register and immediate modes
3. **String Data**: Raw string data (null-terminated, word-aligned)

## Advanced Features

### Two-Pass Assembly
The assembler uses a two-pass approach for proper label resolution:
1. **First pass**: Collect all label definitions and their addresses
2. **Second pass**: Parse instructions and resolve label references

### Instruction Encoding
Advanced encoding scheme supports both register-to-register and register-to-immediate operations:
- **Register R0 handling**: Correctly distinguishes between register R0 and immediate value 0
- **Immediate values**: Supports values up to 2047 (11-bit range)
- **Flag-based encoding**: Uses bit 11 as a mode flag for unambiguous instruction interpretation

## Development

### Project Structure
```
src/
├── core/                    # VM core implementation
│   ├── dez_vm.c            # Main VM logic with PC management
│   ├── dez_memory.c        # Memory management
│   ├── dez_instruction_table.c # Instruction execution with advanced encoding
│   ├── dez_disasm.c        # Disassembler core
│   └── dez_disasm.h        # Disassembler header
├── assembler/              # Assembler toolchain
│   ├── dez_lexer.c         # Tokenizer with comment support
│   ├── dez_parser.c        # Two-pass parser with label resolution
│   ├── dez_symbol_table.c  # Symbol management for labels/constants
│   ├── dez_assembler.c     # Main assembler interface
│   └── asm.c              # Assembler command-line tool
├── main.c                  # VM command-line interface
└── tools/
    └── disasm_tool.c       # Disassembler command-line tool

tests/
├── asm/                    # Assembly test files
│   ├── test_basic.s        # Basic arithmetic operations
│   ├── test_memory.s       # Memory operations
│   ├── test_jumps.s        # Jump instructions
│   ├── test_conditional.s  # Conditional jumps
│   ├── test_system.s       # System calls
│   ├── test_loop.s         # Loop constructs
│   ├── test_labels.s       # Label functionality
│   ├── test_comments.s     # Comment support
│   └── test_mixed.s        # Mixed labels and comments
├── test_assembly.c         # Assembly integration tests
├── test_core.c            # Core VM tests
├── test_memory.c          # Memory operation tests
├── test_performance.c     # Performance tests
└── test_strings.c         # String handling tests

include/
└── dez_vm_types.h         # VM type definitions and opcodes
```

### Adding New Instructions

1. Add opcode to `dez_instruction_type_t` in `include/dez_vm_types.h`
2. Add instruction metadata to `instruction_table` in `src/core/dez_instruction_table.c`
3. Implement execution logic in the appropriate handler function
4. Update the parser to recognize the new instruction syntax
5. Consider instruction encoding for register vs immediate modes

### Adding New System Calls

1. Add syscall number to `dez_syscall_t` in `include/dez_vm_types.h`
2. Add handling in `execute_sys()` function in `src/core/dez_instruction_table.c`
3. Update the parser to recognize the new syscall name

### Testing New Features

1. Add assembly test files in `tests/asm/`
2. Create corresponding C tests in `tests/test_assembly.c`
3. Run the full test suite with `ctest --output-on-failure`
4. Ensure all existing tests continue to pass

## Recent Improvements

### Version 2.0 Features
- **Label Support**: Full label definition and resolution with two-pass assembly
- **Comment Support**: Both full-line (`;`) and inline comments
- **Advanced Instruction Encoding**: Fixed register R0 vs immediate 0 ambiguity
- **Comprehensive Test Suite**: Complete test coverage with CTest integration
- **Improved Error Handling**: Better error messages and validation
- **PC Management**: Fixed program counter handling for all instruction types
- **Loop Support**: Proper conditional and unconditional jump handling

### Technical Improvements
- **Two-Pass Assembly**: Proper forward reference resolution
- **Bit 11 Flag Encoding**: Unambiguous register vs immediate mode distinction
- **Register R0 Fix**: Correct handling of register 0 in arithmetic operations
- **PC Increment Logic**: Centralized program counter management
- **Memory Safety**: Improved bounds checking and error handling

## Limitations

- Maximum program size: 1024 instructions (4KB code segment)
- Maximum immediate value: 2047 (11-bit range due to encoding scheme)
- No floating-point arithmetic
- No dynamic memory allocation
- Limited error handling for some edge cases

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Acknowledgments

This VM was designed as an educational project to demonstrate virtual machine concepts and assembly language implementation.
