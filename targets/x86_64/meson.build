project_name = 'x86_64'

x86_64_sources = [
  'x86_64_target.c',
  'instruction_mapping.c',
  'optimizations.c',
  'abi.c',
]

x86_64_lib = static_library(
  'coil-target-' + project_name,
  x86_64_sources,
  include_directories : coil_inc,
  dependencies : [coil_dep],
  install : true,
  c_args : ['-DCOIL_TARGET_NAME="' + project_name + '"']
)

x86_64_dep = declare_dependency(
  link_with : x86_64_lib,
  include_directories : include_directories('.')
)

# Install target-specific headers if any
# install_headers('x86_64_specific.h', subdir : 'coil-assembler/targets/x86_64')

# Add the target to the list of available targets
available_targets += x86_64_lib