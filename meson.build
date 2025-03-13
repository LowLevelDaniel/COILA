project('coil-assembler', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11',
    'buildtype=release'
  ]
)

# Configuration
conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('VERSION_MAJOR', meson.project_version().split('.')[0])
conf_data.set('VERSION_MINOR', meson.project_version().split('.')[1])
conf_data.set('VERSION_PATCH', meson.project_version().split('.')[2])

# Sources
coil_sources = [
  'src/coil_assembler.c',
  'src/coil_binary_parser.c',
  'src/coil_virtual_map.c',
  'src/coil_utils.c',
  'src/cpu/x86_map.c',
]

# Include directories
inc = include_directories('./include/')

# Library
coil_lib = shared_library('coil',
  coil_sources,
  include_directories : inc,
  install : true,
  c_args : ['-D_DEFAULT_SOURCE', '-D_POSIX_C_SOURCE=200809L']
)

# Executable
coil_exe = executable('coil-assembler',
  'src/main.c',
  include_directories : inc,
  link_with : coil_lib,
  install : true
)

# Generate documentation (requires Doxygen)
doxygen = find_program('doxygen', required : false)
if doxygen.found()
  doxyfile = configure_file(
    input : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : conf_data
  )
  
  custom_target('doc',
    input : doxyfile,
    output : 'html',
    command : [doxygen, doxyfile],
    build_by_default : false
  )
endif
