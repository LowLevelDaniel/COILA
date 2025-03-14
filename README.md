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

## Build Instructions

### Prerequisites

- C11 compatible compiler (GCC 9+ or Clang 10+)
- Meson build system (0.56.0 or later)
- Ninja build tool
- (Optional) Doxygen for documentation generation

### Building with Meson

1. Clone the repository:
```bash
git clone https://github.com/llt/coil-assembler.git
cd coil-assembler
```

2. Configure the build:
```bash
meson setup build
```

3. Build the project:
```bash
ninja -C build
```

4. Run the tests:
```bash
ninja -C build test
```

5. Install the library:
```bash
sudo ninja -C build install
```

## Architecture Overview

The COIL Assembler Framework is built around several key components that work together to transform COIL binary code into optimized native code:

1. **Binary Parser**: Reads and validates COIL binary format
2. **Instruction Decoder**: Translates binary instructions into an internal representation
3. **Target Configuration**: Defines capabilities and characteristics of target hardware
4. **Target Translator**: Maps COIL instructions to native instructions
5. **Optimization Engine**: Applies various optimization passes
6. **Native Code Generator**: Produces the final native binary

Each component is designed with clear interfaces to allow for customization and extension.

## Public API

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