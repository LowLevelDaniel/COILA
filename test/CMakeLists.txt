# Test configuration
set(TEST_SOURCES
    test_main.cpp
    test_lexer.cpp
    test_parser.cpp
    test_instruction.cpp
#    test_binary.cpp
)

# Add include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

# Add the test executable
add_executable(coil_tests ${TEST_SOURCES})

# Link against the main library
target_link_libraries(coil_tests)

# Register tests
add_test(NAME LexerTests COMMAND coil_tests lexer)
add_test(NAME ParserTests COMMAND coil_tests parser)
add_test(NAME InstructionTests COMMAND coil_tests instruction)
# add_test(NAME BinaryTests COMMAND coil_tests binary)