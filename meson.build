project('coil-assembler', 'c',
  version : '1.0.0',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'optimization=2',
    'debug=true',
  ]
)

cc = meson.get_compiler('c')
conf = configuration_data()

# Check for required headers
conf.set('HAVE_STDINT_H', cc.has_header('stdint.h'))
conf.set('HAVE_STDBOOL_H', cc.has_header('stdbool.h'))
conf.set('HAVE_STRING_H', cc.has_header('string.h'))

# Check for required functions
conf.set('HAVE_MEMMOVE', cc.has_function('memmove'))
conf.set('HAVE_MEMSET', cc.has_function('memset'))

# Set include directory
front_incs = include_directories('include/')
src_core_incs = include_directories('src/core')
src_utils_incs = include_directories('src/utils')
src_target_x86 = include_directories('src/targets/x86')
inc_dir = [ front_incs, src_core_incs, src_utils_incs, src_target_x86, ]

# Define source files
sources = [
  'src/coil_assembler.c',
  'src/main.c',
  'src/core/binary_parser.c',
  'src/core/instruction_decoder.c',
  'src/core/native_generator.c',
  'src/core/optimization.c',
  'src/core/target_config.c',
  'src/core/translator.c',
  'src/utils/error_handling.c',
  'src/utils/logging.c',
  'src/utils/memory_management.c',
]

# Add architecture-specific sources based on platform
x86_sources = [
  'src/targets/x86/x86_generator.c',
  'src/targets/x86/x86_optimizer.c',
  'src/targets/x86/x86_translator.c',
]

if host_machine.cpu_family() in ['x86', 'x86_64']
  sources += x86_sources
  conf.set('HAVE_X86_SUPPORT', 1)
endif

# ARM sources could be added similarly
# arm_sources = [...]
# if host_machine.cpu_family() in ['arm', 'aarch64']
#   sources += arm_sources
#   conf.set('HAVE_ARM_SUPPORT', 1)
# endif

# Generate config.h
configure_file(output : 'config.h', configuration : conf)

# Add compile arguments
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

# Set up flags
c_args = []
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  c_args += [
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wno-unused-parameter',
    '-Wshadow',
    '-Wformat=2',
    '-fno-common',
  ]
endif

# Define executable
executable('coil-assembler',
  sources,
  include_directories : inc_dir,
  c_args : c_args,
  install : true,
)

# Create static library
libtarget = static_library('coil',
  sources,
  include_directories : inc_dir,
  c_args : c_args,
  install : true,
)

# Declare dependency for internal use
coil_dep = declare_dependency(
  include_directories : inc_dir,
  link_with : libtarget,
)

# Install headers
install_headers(
  'include/coil_assembler.h',
  subdir : 'coil',
)

# PKG-Config file
pkg = import('pkgconfig')
pkg.generate(
  name : 'coil-assembler',
  description : 'COIL Assembler',
  version : '1.0.0',
  libraries : libtarget,
)

# Add test directory
subdir('tests')