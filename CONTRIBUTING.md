# Contributing to COIL Assembler Framework

Thank you for your interest in contributing to the COIL Assembler Framework! This document outlines the guidelines and workflow for contributing to the project.

## Code of Conduct

This project adheres to a code of conduct that expects all participants to be respectful and considerate of others. Please maintain a professional and inclusive environment for all contributors.

## Getting Started

Before you begin, make sure you have:

1. A C11-compatible compiler (GCC 9+ or Clang 10+)
2. Meson build system (0.56.0 or later)
3. Ninja build tool
4. (Optional) Doxygen for documentation generation

## Building the Project

```bash
# Clone the repository
git clone https://github.com/llt/coil-assembler.git
cd coil-assembler

# Configure the build
meson setup build

# Build the project
ninja -C build

# Run tests
ninja -C build test
```

## Contribution Workflow

1. **Fork the repository** and create a new branch from `main`
2. **Make your changes**, following the coding standards
3. **Add or update tests** to cover your changes
4. **Update documentation** if necessary
5. **Run tests** to ensure they pass
6. **Submit a pull request** with a clear description of the changes

## Coding Standards

The project uses a strict set of coding standards to maintain consistency and quality:

### C Code Style

- Follow the NASA and Google C programming style guides
- Use 2 space indentation (no tabs)
- Use Doxygen-style comments for all public API functions and structures
- Use `snake_case` for function and variable names
- Use `ALL_CAPS` for macros and constants
- Maximum line length of 80 characters (with exceptions for readability)

### Memory Management

- Explicitly check for allocation failures
- Always free allocated memory in error paths
- Document ownership transfer in function comments
- Use defensive programming practices

### Error Handling

- Functions should return error codes (0 for success, non-zero for failure)
- Document all possible error codes
- Provide meaningful error messages through the diagnostics system

### Documentation

- All public API functions must have complete Doxygen documentation
- Document parameter constraints and ownership transfer
- Provide examples for non-trivial API usage
- Update README.md when adding new features

## Adding Support for New Target Architectures

When adding support for a new target architecture:

1. Create a new directory under `targets/` using the template as a starting point
2. Implement all required target interfaces
3. Add comprehensive tests for the new target
4. Update documentation to include the new target

See `targets/template/README.md` for detailed guidelines on implementing a new target backend.

## Submitting Pull Requests

When submitting a pull request:

1. Provide a clear title and description
2. Link to any related issues
3. Include test cases that validate the changes
4. Ensure all tests pass
5. Address review comments in a timely manner

## Reporting Issues

When reporting issues:

1. Use the issue template
2. Provide a minimal reproduction case
3. Include environment details (OS, compiler version, etc.)
4. Describe expected vs. actual behavior

## License

By contributing to this project, you agree that your contributions will be licensed under the project's MIT license.

## Contact

If you have questions about contributing, please [open an issue](https://github.com/llt/coil-assembler/issues) on the repository.