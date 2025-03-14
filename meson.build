project('coil-assembler', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11',
    'werror=true',
    'buildtype=debugoptimized',
    'default_library=both'
  ],
  meson_version : '>= 0.56.0')

# Setup compilation flags
c_compiler = meson.get_compiler('c')
c_args = [
  '-pedantic',
  '-Wall',
  '-Wextra',
  '-Werror',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wshadow',
  '-Wformat=2',
  '-Wfloat-equal',
  '-Wcast-align',
  '-Wwrite-strings',
  '-Wmissing-declarations',
  '-fstack-protector-strong',
  '-D_FORTIFY_SOURCE=2'
]

# Add compiler-specific flags
if c_compiler.get_id() == 'gcc'
  c_args += [
    '-Wno-unused-parameter',
    '-Wlogical-op',
    '-Wduplicated-cond',
    '-Wduplicated-branches',
    '-Wnull-dereference'
  ]
elif c_compiler.get_id() == 'clang'
  c_args += [
    '-Wno-unused-parameter',
    '-Wdocumentation',
    '-Wno-documentation-unknown-command',
    '-Wno-missing-braces'
  ]
endif

add_project_arguments(c_args, language : 'c')

# Configuration data
conf_data = configuration_data()
conf_data.set('COIL_VERSION', meson.project_version())
conf_data.set('COIL_COMMIT', 'git-' + run_command('git', 'rev-parse', '--short', 'HEAD').stdout().strip())

# Configure version header
configure_file(
  input : 'include/coil-assembler/version.h.in',
  output : 'version.h',
  configuration : conf_data,
  install_dir : 'include/coil-assembler'
)

# Dependencies
thread_dep = dependency('threads')
m_dep = c_compiler.find_library('m', required : false)

# Include directories
coil_inc = include_directories('include')

# Core library
coil_sources = [
  # Core functionality
  'src/core/assembler.c',
  'src/core/binary_parser.c',
  'src/core/instruction_decoder.c',
  'src/core/target_registry.c',
  'src/core/config_loader.c',

  # API implementation
  'src/api/assembler_api.c',
  'src/api/target_api.c',
  'src/api/config_api.c',

  # Optimizer
  'src/optimizer/optimizer.c',
  'src/optimizer/pass_manager.c',

  # Code generation
  'src/codegen/code_generator.c',
  'src/codegen/register_allocator.c',
  'src/codegen/instruction_selector.c',

  # Utilities
  'src/utils/logging.c',
  'src/utils/memory.c',
  'src/utils/error_handling.c'
]

# Library target
coil_lib = library('coil-assembler', 
  coil_sources,
  include_directories : coil_inc,
  dependencies : [thread_dep, m_dep],
  install : true,
  c_args : ['-DBUILDING_COIL']
)

# Dependency for use by subprojects
coil_dep = declare_dependency(
  include_directories : coil_inc,
  link_with : coil_lib
)

# For usage by external projects
pkg = import('pkgconfig')
pkg.generate(
  name : 'coil-assembler',
  description : 'COIL assembler framework',
  version : meson.project_version(),
  libraries : coil_lib,
  subdirs : 'coil-assembler'
)

# Install headers
install_headers('include/coil/binary.h', 
                'include/coil/types.h',
                'include/coil/instructions.h',
                'include/coil/memory_model.h',
                subdir : 'coil')

install_headers('include/coil-assembler/assembler.h',
                'include/coil-assembler/target.h',
                'include/coil-assembler/config.h',
                'include/coil-assembler/diagnostics.h',
                subdir : 'coil-assembler')

# Target architectures
subdir('targets/x86_64')
subdir('targets/arm64')
subdir('targets/riscv')

# Tools
subdir('tools/coil-dis')
subdir('tools/coil-opt')
subdir('tools/target-config')

# Tests (conditionally)
if get_option('build_tests')
  subdir('tests')
endif

# Documentation (conditionally)
if get_option('build_docs')
  doxygen = find_program('doxygen', required : false)
  if doxygen.found()
    subdir('docs')
  else
    message('Doxygen not found, documentation will not be built')
  endif
endif

message('COIL Assembler Framework has been configured')