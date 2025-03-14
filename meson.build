project('coilasm', 'c',
  version : '0.1.0',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ]
)

# Include directories
inc_dirs = include_directories('include', 'src')

# Core sources
core_sources = [
  'src/core/binary_parser.c',
  'src/core/instruction_decoder.c',
  'src/core/config.c',
  'src/core/optimizer.c',
  'src/core/native_generator.c',
  'src/core/ir/module.c',
  'src/core/ir/function.c',
  'src/core/ir/instruction.c',
  'src/core/ir/types.c',
]

# Utility sources
utils_sources = [
  'src/utils/sstream.c',
  'src/utils/coilbuf.c',
  'src/utils/logging.c',
  'src/utils/error.c',
  'src/utils/memory.c',
]

# Target-specific sources
x86_sources = [
  'src/targets/cpu/x86/x86_common.c',
  'src/targets/cpu/x86/translator.c',
  'src/targets/cpu/x86/optimizer.c',
  'src/targets/cpu/x86/generator.c',
]

# Optional runtime sources
runtime_sources = [
  'src/runtime/bare_metal.c',
  'src/runtime/os_support.c',
]

# Main executable
executable('coilasm',
  ['src/main.c'] + core_sources + utils_sources + x86_sources + runtime_sources,
  include_directories : inc_dirs,
  install : true,
)

# Build options
option('enable_tests', type : 'boolean', value : true, description : 'Build tests')
option('enable_docs', type : 'boolean', value : true, description : 'Build documentation')

# Conditionally build tests
if get_option('enable_tests')
  subdir('tests')
endif

# Conditionally build documentation
if get_option('enable_docs')
  subdir('docs')
endif