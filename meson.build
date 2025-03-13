project('coil-assembler', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'werror=true',
    'c_std=c11',
  ]
)

# Project configuration
conf_data = configuration_data()
conf_data.set('VERSION_MAJOR', '0')
conf_data.set('VERSION_MINOR', '1')
conf_data.set('VERSION_PATCH', '0')

# Generate config header
configure_file(
  output : 'config.h',
  configuration : conf_data
)

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

# Include directories
inc_dirs = include_directories('include')

# Core source files
core_sources = [
  'src/core/binary_parser.c',
  'src/core/instruction_decoder.c',
  'src/core/target_config.c',
  'src/core/translator.c',
  'src/core/optimization.c',
  'src/core/native_generator.c',
]

# Target-specific sources
target_sources = [
  'src/targets/x86/x86_translator.c',
  'src/targets/x86/x86_optimizer.c',
  'src/targets/x86/x86_generator.c',
]

# Utility sources
util_sources = [
  'src/utils/error_handling.c',
  'src/utils/memory_management.c',
  'src/utils/logging.c',
]

# Main program
main_sources = [
  'src/main.c',
  'src/coil_assembler.c',
]

all_sources = core_sources + target_sources + util_sources + main_sources

# Compiler flags
c_args = [
  '-D_GNU_SOURCE',
  '-pedantic',
  '-fstack-protector-strong',
  '-fPIC',
  '-Wall',
  '-Wextra',
  '-Wformat=2',
  '-Wno-unused-parameter',
  '-Wshadow',
  '-Wwrite-strings',
  '-Wstrict-prototypes',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wmissing-include-dirs',
  '-Wjump-misses-init',
  '-Wlogical-op',
]

# Build the executable
coil_assembler = executable('coil-assembler',
  all_sources,
  include_directories : inc_dirs,
  c_args : c_args,
  dependencies : [m_dep],
  install : true,
)

# Build the library
coil_assembler_lib = library('coil_assembler',
  all_sources,
  include_directories : inc_dirs,
  c_args : c_args,
  dependencies : [m_dep],
  install : true,
)

# Install headers
install_headers('include/coil_assembler.h')

# Documentation
doxygen = find_program('doxygen', required : false)
if doxygen.found()
  doxyfile = configure_file(
    input : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : conf_data,
  )
  
  custom_target('doc',
    input : doxyfile,
    output : 'html',
    command : [doxygen, doxyfile],
    build_by_default : false,
  )
endif

# Unit tests
subdir('tests/unit')

# Integration tests
subdir('tests/integration')