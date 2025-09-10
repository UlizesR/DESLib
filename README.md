# Dez Virtual Machine

A simple virtual machine and assembly language implementation in C.

## Project Structure

```
Dez/
├── src/                    # Core VM source files
│   ├── assembly_vm.c      # Main VM implementation
│   ├── vm_errors.c        # Error handling
│   ├── vm_hash_table.c    # Hash table implementation
│   └── vm_validation.c    # Input validation
├── include/               # Header files
│   ├── assembly_vm.h      # Main VM header
│   ├── vm_errors.h        # Error definitions
│   ├── vm_hash_table.h    # Hash table interface
│   └── vm_validation.h    # Validation functions
├── tools/                 # Development tools
│   ├── asm.c             # Assembler
│   ├── disasm.c          # Disassembler
│   └── objdump.c         # Objdump-style disassembler
├── examples/              # Example programs
│   └── hello_world.asm   # Hello world example
├── tests/                 # Test files
│   ├── asm/              # Test assembly files
│   └── test_*.c          # Test implementations
├── bin/                   # Generated binary files
│   └── *.dez             # Compiled binary programs
├── build/                 # Build output directory
└── CMakeLists.txt        # Build configuration
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

### Running Assembly Programs
```bash
./build/dez examples/hello_world.asm
```

### Assembling to Binary
```bash
./build/dez_asm examples/hello_world.asm bin/hello_world.dez
```

### Disassembling Binary Files
```bash
# Basic disassembly
./build/dez_disasm bin/hello_world.dez

# Objdump-style with memory addresses and execution flow
./build/dez_objdump bin/hello_world.dez
```

## Binary Format

The simple binary format (`.dez` files) uses:
- **Header**: Magic number (0xDEADBEEF), version, program size
- **Instructions**: 16 bytes each (opcode, operands, types)
- **String data**: Follows instructions that use strings

## Objdump Tool Features

The `dez_objdump` tool provides detailed analysis similar to GNU objdump:

- **Memory Layout**: Shows text, data, stack, and register memory regions
- **Execution Flow**: Displays program entry point and instruction sequence
- **Symbol Table**: Lists all symbols and their memory addresses
- **Disassembly**: Shows raw bytes with memory addresses and decoded instructions
- **Data Section**: Displays string data in hex and ASCII format

## Assembly Language

Dez supports:
- **Arithmetic**: ADD, SUB, MUL, DIV
- **Memory**: LOAD, STORE, PUSH, POP
- **Control flow**: JMP, JZ, JNZ
- **I/O**: PRINT, PRINTS, INPUT
- **System**: HALT, NOP

## Testing

```bash
cd build
make run_tests
```
