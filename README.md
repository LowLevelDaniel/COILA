# COIL Assembler
COIL Assembler built by the LLT (Low Level Team)

The COIL Assembler is a critical component in the COIL (Compiler Optimization Intermediate Language) ecosystem, translating architecture-independent COIL binary code into optimized native code for specific target architectures.

## Overview

COIL serves as a universal intermediate representation for compilers targeting diverse processing units. The COIL assembler provides the bridge between this intermediate format and actual hardware, allowing compiler developers to focus on language features without worrying about target-specific details.

Key features of the COIL Assembler:

- Translates COIL instructions to native instructions
- Leverages target-specific optimizations
- Emulates features not directly supported by hardware
- Generates efficient native binaries
- Provides runtime support when needed

## Architecture

The COIL Assembler consists of these primary components:

- **Binary Parser**: Parses and validates COIL binary format
- **Instruction Decoder**: Decodes COIL instructions into internal representation
- **Target Configuration**: Defines target architecture capabilities
- **Target-Specific Translator**: Maps COIL instructions to native code
- **Optimization Engine**: Applies target-specific optimizations
- **Native Code Generator**: Produces native binary output

## Building

The project uses the Meson build system:

```bash
# Setup build directory
meson setup build

# Compile
meson compile -C build

# Run tests
meson test -C build

# Install
meson install -C build
```

## Usage

```bash
coil-assembler [options] <input-file> -o <output-file>

Options:
  -h, --help                 Show help message
  -o, --output <file>        Specify output file
  -t, --target <target>      Specify target architecture (default: x86_64)
  -d, --device <device>      Specify device class (cpu, gpu, npu, tpu, dsp, fpga)
  -O<level>                  Set optimization level (0-3)
  -v, --verbose              Increase verbosity
  -q, --quiet                Decrease verbosity
```

Example:
```bash
coil-assembler -t x86_64 -O2 input.coil -o output.o
```

## Target Support

Currently supported targets:

- x86_64 (primary focus)

Planned future targets:
- ARM
- RISC-V
- PowerPC

## Device Classes

The assembler supports different device classes through a virtual mapping system:

- CPU (Central Processing Unit)
- GPU (Graphics Processing Unit)
- NPU (Neural Processing Unit)
- TPU (Tensor Processing Unit)
- DSP (Digital Signal Processor)
- FPGA (Field-Programmable Gate Array)

## API

The COIL Assembler provides a C API for integration into other tools:

```c
// Initialize assembler
coil_assembler_handle_t assembler = coil_assembler_init("x86_64", COIL_DEVICE_CPU);

// Load COIL module
coil_module_handle_t module = coil_module_load("input.coil");

// Assemble module to native code
coil_assemble(assembler, module, "output.o");

// Clean up
coil_module_free(module);
coil_assembler_cleanup(assembler);
```

## Documentation

Generate API documentation using Doxygen:

```bash
meson compile -C build doc
```

The generated documentation will be available in `build/html/`.

## License

This project is licensed under the MIT License - see the LICENSE file for details.