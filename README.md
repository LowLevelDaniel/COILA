# COIL Assembler (coilasm)

This is a first-generation implementation of a COIL (Computer Oriented Intermediate Language) assembler based on the COIL language specification.

## Overview

The COIL Assembler (coilasm) is a tool for processing COIL assembly language and generating COIL Object Format (COF) binary files. It supports a subset of the COIL instruction set and targets the x86-64 architecture.

## Features

- COIL assembly parsing and processing
- COF binary format generation
- Support for x86-64 target architecture
- Basic variable management
- Basic error reporting and diagnostics

## Building from Source

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/username/coilasm.git
   cd coilasm
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   cmake --build .
   ```

4. Run tests (optional):
   ```bash
   ctest
   ```

5. Install (optional):
   ```bash
   cmake --install .
   ```

## Usage

```bash
coilasm [options] <input_file>
```

### Options

- `-o <output_file>`: Specify output file (default: input.cof)
- `-t <target>`: Specify target architecture (default: x86-64)
- `-v`: Enable verbose output
- `-h, --help`: Display help message

### Example

```bash
coilasm -o hello.cof examples/hello_world.coil
```

## COIL Assembly Syntax

COIL assembly follows a consistent syntax pattern:

```
[category] [operation] [destination], [source1], [source2], ... -> [output1], [output2], ...
```

### Examples

```
MATH ADD R0, R1, 42       ; R0 = R1 + 42
MEM MOV [R0], R1          ; Store R1 at address in R0
CF BRC EQ label           ; Branch to label if equal flag is set
VAR DECL $0 : int32 = 10  ; Declare int32 variable with initial value 10
```

### Directives

```
DIR SECT text READ EXEC           ; Define text section with read and execute permissions
DIR LABEL function_name           ; Define a label
DIR HINT function_name FUNC GLOBAL ; Define a function hint with global scope
DIR ABI abi_name { ... }          ; Define an ABI
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgements

This implementation is based on the COIL language specification, which provides a comprehensive guide to the COIL language and its binary formats.