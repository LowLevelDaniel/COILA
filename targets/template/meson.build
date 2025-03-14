project_name = 'template'

template_sources = [
  'target.c',
  'instruction_mapping.c',
  'optimizations.c',
]

template_lib = static_library(
  'coil-target-' + project_name,
  template_sources,
  include_directories : coil_inc,
  dependencies : [coil_dep],
  install : false,
  c_args : ['-DCOIL_TARGET_NAME="' + project_name + '"']
)

template_dep = declare_dependency(
  link_with : template_lib,
  include_directories : include_directories('.')
)

# Register the target with the main library
# This is just a template and won't be built by default