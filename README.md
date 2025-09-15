# Dez VM - A Simple Virtual Machine

Dez VM is a lightweight 32-bit virtual machine designed for educational purposes and simple program execution. It features a complete assembler toolchain and supports string printing capabilities.

## Features

- **32-bit RISC-like instruction set** with 16 general-purpose registers
- **Complete assembler toolchain** with lexer, parser, and symbol table
- **String literal support** with `LOAD R1, "Hello"` syntax
- **System calls** for I/O operations including string printing
- **Memory management** with code, data, and stack segments
- **Binary file format** for compiled programs

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
# asm dez_vm disasm_tool performance_test test_memory test_vm
```

## Usage

### 1. Writing Assembly Code

Create a `.asm` file with your program:

```assembly
# examples/hello.asm
LOAD R1, "Hello, World!"
SYS R1, PRINT_STR
LOAD R2, 0
SYS R2, EXIT
```

### 2. Assembling

Use the assembler to compile your assembly code:

```bash
# From the build directory
./bin/asm ../examples/hello_world.asm hello.bin
```

### 3. Running

Execute the compiled binary with the VM:

```bash
# From the build directory
./bin/dez_vm hello.bin
```

## Instruction Set

### Data Movement
- `LOAD R1, #42` - Load immediate value 42 into R1
- `LOAD R1, "Hello"` - Load address of string "Hello" into R1
- `MOV R1, R2` - Copy R2 to R1

### Arithmetic
- `ADD R1, R2, R3` - R1 = R2 + R3
- `SUB R1, R2, R3` - R1 = R2 - R3
- `MUL R1, R2, R3` - R1 = R2 * R3
- `DIV R1, R2, R3` - R1 = R2 / R3

### Memory Operations
- `STORE R1, [R2]` - Store R1 at address in R2
- `PUSH R1` - Push R1 onto stack
- `POP R1` - Pop from stack into R1

### Control Flow
- `JMP label` - Jump to label
- `JZ label` - Jump if zero flag set
- `JNZ label` - Jump if zero flag not set
- `CALL label` - Call subroutine
- `RET` - Return from subroutine

### System Calls
- `SYS R1, PRINT` - Print value in R1
- `SYS R1, PRINT_STR` - Print string at address in R1
- `SYS R1, PRINT_CHAR` - Print character in R1
- `SYS R1, EXIT` - Exit with code in R1

### Other
- `CMP R1, R2` - Compare R1 and R2 (sets flags)
- `HALT` - Halt execution

## Examples

### Hello World
```assembly
LOAD R1, "Hello, World!"
SYS R1, PRINT_STR
LOAD R2, 0
SYS R2, EXIT
```

### Arithmetic
```assembly
LOAD R1, #10
LOAD R2, #20
ADD R3, R1, R2
SYS R3, PRINT
HALT
```

### String Operations
```assembly
LOAD R1, "Hello"
SYS R1, PRINT_STR
LOAD R2, "World"
SYS R2, PRINT_STR
HALT
```

### Loops and Conditions
```assembly
start:
    LOAD R1, #10
    LOAD R2, #0
loop:
    ADD R2, R2, #1
    CMP R2, R1
    JNZ loop
    SYS R2, PRINT
    HALT
```

## Tools

### Assembler (`asm`)
Compiles assembly source files to binary format:
```bash
# From the build directory
./bin/asm ../examples/hello_world.asm hello.bin
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

## File Format

The binary format consists of:
1. **Header**: 4-byte instruction count
2. **Instructions**: 32-bit encoded instructions
3. **String Data**: Raw string data (null-terminated, word-aligned)

## Development

### Project Structure
```
src/
├── core/           # VM core implementation
│   ├── dez_vm.c    # Main VM logic
│   ├── dez_memory.c # Memory management
│   └── dez_instruction_table.c # Instruction execution
├── assembler/      # Assembler toolchain
│   ├── dez_lexer.c # Tokenizer
│   ├── dez_parser.c # Parser
│   ├── dez_symbol_table.c # Symbol management
│   └── dez_assembler.c # Main assembler
└── tools/          # Utility tools
    └── disasm_tool.c # Disassembler
```

### Adding New Instructions

1. Add opcode to `dez_instruction_type_t` in `include/dez_vm_types.h`
2. Add instruction metadata to `instruction_table` in `src/core/dez_instruction_table.c`
3. Implement execution logic in the appropriate handler function
4. Update the parser to recognize the new instruction syntax

### Adding New System Calls

1. Add syscall number to `dez_syscall_t` in `include/dez_vm_types.h`
2. Add handling in `execute_sys()` function in `src/core/dez_instruction_table.c`
3. Update the parser to recognize the new syscall name

## Limitations

- Maximum program size: 1024 instructions (4KB code segment)
- Maximum string length: 255 characters
- No floating-point arithmetic
- No dynamic memory allocation
- Limited error handling

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Acknowledgments

This VM was designed as an educational project to demonstrate virtual machine concepts and assembly language implementation.
