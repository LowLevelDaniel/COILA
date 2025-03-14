# COIL Assembler Framework

A minimal, extensible framework for creating COIL assemblers targeting diverse processing units.

## Overview

The COIL Assembler Framework provides a foundation for building optimized native code generators for the Compiler Optimization Intermediate Language (COIL). This framework allows developers to create assemblers that translate architecture-independent COIL binary code into highly efficient native code for specific target architectures.

Key features:
- Clean separation between public compiler interfaces and internal implementation
- Pluggable optimization pipeline
- Extensible target architecture support
- Zero dependencies in core components
- Comprehensive testing framework

## Directory Structure

```
coil-assembler/
│
├── README.md                  # Project overview and documentation
├── LICENSE                    # Project license
├── CONTRIBUTING.md            # Guidelines for contributors
├── CMakeLists.txt             # Main build configuration
├── meson.build                # Alternative build system support
│
├── include/                   # Public API headers
│   ├── coil/                  # Core COIL definitions
│   │   ├── binary.h           # COIL binary format definitions
│   │   ├── types.h            # COIL type system
│   │   ├── instructions.h     # COIL instruction set
│   │   └── memory_model.h     # COIL memory model
│   │
│   └── coil-assembler/        # Public assembler API
│       ├── assembler.h        # Main assembler interface
│       ├── target.h           # Target architecture definitions
│       ├── config.h           # Configuration interface
│       ├── diagnostics.h      # Error and warning reporting
│       └── version.h          # Version information
│
├── src/                       # Implementation source code
│   ├── core/                  # Core functionality
│   │   ├── assembler.c        # Assembler implementation
│   │   ├── binary_parser.c    # COIL binary parser
│   │   ├── instruction_decoder.c # Instruction decoder
│   │   ├── target_registry.c  # Target backend registry
│   │   └── config_loader.c    # Configuration loader
│   │
│   ├── api/                   # Public API implementation
│   │   ├── assembler_api.c    # Public API implementation
│   │   ├── target_api.c       # Target API implementation
│   │   └── config_api.c       # Configuration API implementation
│   │
│   ├── optimizer/             # Optimization framework
│   │   ├── optimizer.c        # Core optimizer
│   │   ├── pass_manager.c     # Optimization pass manager
│   │   ├── analysis/          # Analysis passes
│   │   └── transforms/        # Transformation passes
│   │
│   ├── codegen/               # Code generation framework
│   │   ├── code_generator.c   # Core code generator
│   │   ├── register_allocator.c # Register allocator
│   │   └── instruction_selector.c # Instruction selector
│   │
│   └── utils/                 # Utility functions
│       ├── logging.c          # Logging utilities
│       ├── memory.c           # Memory management
│       └── error_handling.c   # Error handling
│
├── targets/                   # Target architecture implementations
│   ├── x86_64/                # x86_64 architecture backend
│   │   ├── CMakeLists.txt     # Target-specific build config
│   │   ├── x86_64_target.c    # Target implementation
│   │   ├── instruction_mapping.c # COIL to x86_64 instruction mapping
│   │   ├── optimizations.c    # x86_64-specific optimizations
│   │   └── abi.c              # ABI implementation
│   │
│   ├── arm64/                 # ARM64 architecture backend
│   │   ├── ...                # Similar structure to x86_64
│   │
│   ├── riscv/                 # RISC-V architecture backend
│   │   ├── ...                # Similar structure to x86_64
│   │
│   └── template/              # Template for new target implementations
│       ├── CMakeLists.txt     # Template build config
│       ├── target.c           # Template target implementation
│       ├── instruction_mapping.c # Template instruction mapping
│       ├── optimizations.c    # Template optimizations
│       └── README.md          # Instructions for implementing a new target
│
├── tools/                     # Utility tools
│   ├── coil-dis/              # COIL disassembler
│   ├── coil-opt/              # COIL optimizer
│   └── target-config/         # Target configuration tools
│
├── examples/                  # Example code
│   ├── simple_assembler/      # Simple assembler example
│   ├── custom_target/         # Custom target example
│   └── optimization_pipeline/ # Custom optimization pipeline
│
├── tests/                     # Test suite
│   ├── unit/                  # Unit tests
│   ├── integration/           # Integration tests
│   ├── targets/               # Target-specific tests
│   └── test_data/             # Test input files
│
└── docs/                      # Documentation
    ├── api/                   # API documentation
    ├── design/                # Design documents
    ├── targets/               # Target-specific documentation
    └── tutorials/             # Tutorials and guides
```

## Architecture Overview

### Core Components

The COIL Assembler Framework is built around several key components that work together to transform COIL binary code into optimized native code:

1. **Binary Parser**: Reads and validates COIL binary format
2. **Instruction Decoder**: Translates binary instructions into an internal representation
3. **Target Configuration**: Defines capabilities and characteristics of target hardware
4. **Target Translator**: Maps COIL instructions to native instructions
5. **Optimization Engine**: Applies various optimization passes
6. **Native Code Generator**: Produces the final native binary

Each component is designed with clear interfaces to allow for customization and extension.

```
+------------------+     +------------------+     +------------------+
| COIL Binary      | --> | Binary Parser    | --> | Instruction      |
| Input            |     |                  |     | Decoder          |
+------------------+     +------------------+     +------------------+
                                                          |
                                                          v
+------------------+     +------------------+     +------------------+
| Native Binary    | <-- | Native Code      | <-- | Target-Specific  |
| Output           |     | Generator        |     | Translator       |
+------------------+     +------------------+     +------------------+
                              ^
                              |
+------------------+     +------------------+
| Target           | --> | Optimization     |
| Configuration    |     | Engine           |
+------------------+     +------------------+
```

### Public API

The public API is designed to be simple and stable, providing the essential functionality needed by compiler frontends and other tools that generate COIL code.

Key interfaces:
- `coil_assembler_create()`: Create an assembler instance
- `coil_assembler_set_target()`: Configure the target architecture
- `coil_assembler_process_module()`: Process a COIL module
- `coil_assembler_write_output()`: Generate native binary output

Example usage:
```c
coil_assembler_t* assembler = coil_assembler_create();
coil_assembler_set_target(assembler, "x86_64");
coil_assembler_set_optimization_level(assembler, COIL_OPT_LEVEL_3);
coil_module_t* module = coil_module_load_from_file("input.coil");
coil_assembler_process_module(assembler, module);
coil_assembler_write_output(assembler, "output.o");
coil_assembler_destroy(assembler);
```

### Internal API

The internal API provides a richer set of interfaces for target implementers and optimization pass developers. These interfaces are more flexible but may change between minor versions.

Key components:
- Instruction representation
- Register allocation
- Code generation primitives
- Optimization pass framework

### Optimization Pipeline

The framework uses a pluggable optimization pipeline that allows custom optimization passes to be inserted at various stages:

1. **Module-level optimizations**: Operating on the entire module
2. **Function-level optimizations**: Operating on individual functions
3. **Basic block optimizations**: Operating on basic blocks
4. **Peephole optimizations**: Operating on instruction sequences

Each optimization pass implements a standard interface:
```c
typedef struct {
    const char* name;
    bool (*initialize)(optimizer_context_t* ctx);
    bool (*run)(optimizer_context_t* ctx, void* data);
    void (*finalize)(optimizer_context_t* ctx);
} optimization_pass_t;
```

### Target Architecture Support

The framework is designed to be easily extended with new target architectures. Each target implementation provides:

1. **Target descriptor**: Defines basic properties and capabilities
2. **Instruction mapping**: Maps COIL instructions to native instructions
3. **Register allocator**: Manages hardware registers
4. **ABI implementation**: Handles calling conventions
5. **Target-specific optimizations**: Optimizations unique to the target

## Extending with New Targets

To add support for a new processing unit architecture:

1. Create a new directory under `targets/` (use the template as a starting point)
2. Implement the required target interfaces:
   - `target_initialize()`: Set up the target backend
   - `target_map_instruction()`: Map COIL instructions to native
   - `target_generate_function()`: Generate native code for functions
   - `target_finalize()`: Clean up and finalize the output

3. Register your target with the framework:
   ```c
   #include "coil-assembler/target_internal.h"
   
   void register_my_target(void) {
       target_descriptor_t desc = {
           .name = "my_architecture",
           .description = "My Custom Architecture",
           .word_size = 64,
           .endianness = ENDIAN_LITTLE,
           .initialize = my_target_initialize,
           .map_instruction = my_target_map_instruction,
           .generate_function = my_target_generate_function,
           .finalize = my_target_finalize
       };
       
       register_target(&desc);
   }
   ```

4. Provide a target configuration file describing your architecture's capabilities

See `targets/template/README.md` for detailed instructions and best practices.

## Configuration System

The COIL Assembler uses a flexible configuration system to define target architecture capabilities and optimization parameters. Configurations can be provided as:

1. Text files (`.coilcfg`)
2. Programmatic configuration through the API
3. Auto-detected configuration for the current platform

Example configuration for a custom target:
```
target {
  "my_custom_target" {
    architecture = "custom_arch"
    vendor = "custom_vendor"
    features = ["feature1", "feature2", "feature3"]
    
    resources {
      registers = 32
      vector_width = 256
      min_alignment = 8
    }
    
    optimization {
      unroll_factor = 4
      vector_threshold = 8
      use_specialized_instructions = true
    }
  }
}
```

## Build System

The COIL Assembler framework uses CMake as its primary build system, with support for Meson as an alternative. To build:

```bash
mkdir build
cd build
cmake ..
make
```

For development, enable testing:
```bash
cmake -DBUILD_TESTING=ON ..
make
ctest
```

## Contributing

Contributions are welcome! See `CONTRIBUTING.md` for guidelines. Key areas for contribution:

1. **New target architectures**: Implementing support for additional CPUs, GPUs, etc.
2. **Optimization passes**: Creating new optimization strategies
3. **Testing**: Expanding test coverage
4. **Documentation**: Improving guides and API documentation
5. **Bug fixes**: Addressing issues in existing code

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Design Principles

1. **Separation of concerns**: Clear boundaries between components
2. **Extensibility**: Easy to add new targets and optimizations
3. **Minimal dependencies**: Core functionality has zero external dependencies
4. **Progressive optimization**: Start simple, add complexity as needed
5. **Clear interfaces**: Well-defined APIs between components
6. **Testability**: Every component is designed to be testable in isolation

## Performance Considerations

The framework enables multiple levels of optimization:

1. **Generic optimizations**: Applied to all targets
2. **Architecture-specific optimizations**: Tailored to hardware features
3. **Specialized patterns**: Optimized code patterns for common operations

Performance-critical paths use zero-copy approaches where possible, and memory allocation is carefully managed to minimize overhead.