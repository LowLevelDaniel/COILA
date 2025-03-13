# Unit tests for COIL assembler

unity_dep = dependency('unity', required : false)

if not unity_dep.found()
  message('Unity test framework not found, using fallback')
  unity_src = files('../../vendor/unity/unity.c')
  unity_inc = include_directories('../../vendor/unity')
else
  unity_src = []
  unity_inc = []
endif

test_inc = [
  inc_dirs,
  unity_inc,
  include_directories('../../src'),
]

# Helper for adding tests
test_sources = [
  'test_binary_parser.c',
  'test_instruction_decoder.c',
  'test_target_config.c',
  'test_translator.c',
  'test_x86_translator.c',
]

foreach test_src : test_sources
  test_name = test_src.split('.')[0]
  
  test_exe = executable(
    test_name,
    [test_src, unity_src],
    include_directories : test_inc,
    dependencies : [m_dep],
    link_with : coil_assembler_lib,
  )
  
  test(test_name, test_exe)
endforeach