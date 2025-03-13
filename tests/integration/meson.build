# Integration tests for COIL assembler

integration_test_sources = [
  'test_full_pipeline.c',
  'test_x86_assembly.c',
]

test_fixtures_dir = meson.current_source_dir() + '/../fixtures'

foreach test_src : integration_test_sources
  test_name = test_src.split('.')[0]
  
  test_exe = executable(
    test_name,
    [test_src],
    include_directories : inc_dirs,
    dependencies : [m_dep],
    link_with : coil_assembler_lib,
    c_args : ['-DTEST_FIXTURES_DIR="' + test_fixtures_dir + '"'],
  )
  
  test(test_name, test_exe)
endforeach

# Test script to verify assembler behavior with real programs
python = find_program('python3', required : false)
if python.found()
  test('comprehensive_tests',
       python,
       args : ['run_comprehensive_tests.py', coil_assembler.full_path()],
       workdir : meson.current_source_dir())
endif