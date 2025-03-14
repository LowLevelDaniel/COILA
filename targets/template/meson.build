project_name = 'template'

# Source files for the template target
template_sources = [
  'target.c',
  'instruction_mapping.c',
  'optimizations.c',
  'abi.c',   # Added abi.c to be consistent with other targets
]

# Create the static library for the template target
template_lib = static_library(
  'coil-target-' + project_name,
  template_sources,
  include_directories : coil_inc,
  dependencies : [coil_dep],
  install : false,  # Not installed by default since this is a template
  c_args : [
    '-DCOIL_TARGET_NAME="' + project_name + '"',
    '-DCOIL_TARGET_TEMPLATE'  # Add a define to indicate this is the template
  ]
)

# Create a dependency object that can be used by other components
template_dep = declare_dependency(
  link_with : template_lib,
  include_directories : include_directories('.')
)

# Add conditional installation based on option
if get_option('install_template')
  install_headers(
    'template.h',
    subdir : 'coil-assembler/targets/template'
  )
endif

# This is a template and not included in the main build by default
# Uncomment the line below to include it in available targets
# available_targets += template_lib

# Print a helpful message
message('Template target configured - use this as a starting point for new targets')