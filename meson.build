project(
  'cli', 
  'cpp', 
  license: 'BSL-1.0', 
  license_files: 'LICENSE_1_0.txt', 
  meson_version: '>=1.1.0', 
  default_options: ['cpp_std=c++20']
)

inc = include_directories('include')

build_tests = get_option('tests')
build_examples = get_option('examples')
simulation = get_option('sim')
cli_term = get_option('cli-term')

# testing, building examples, and simulation can only be done for  non-embeded targets
build_tests.disable_if(meson.is_cross_build(), error_message: 'can\'t build tests for embedded targets')
build_examples.disable_if(meson.is_cross_build(), error_message: 'can\'t build examples on embedded targets')
simulation.disable_if(meson.is_cross_build(), error_message: 'can\'t have simulation with embedded targets')
cli_term.disable_if(meson.is_cross_build(), error_message: 'cli-term can\'t be built for embedded targets')

deps = []

if(not simulation.disabled())
  deps += dependency('cpp-terminal', required: true)
endif

cli_dep = declare_dependency(
  include_directories:inc, 
  dependencies: deps
)

meson.override_dependency('cli', cli_dep)

must_fail_to_compile = files(
  'tests/fails/param_t.cpp'
)

cpp = meson.get_compiler('cpp')

foreach file: must_fail_to_compile 
  if(cpp.compiles(file, include_directories: inc))
    error('CLI is able to compile '+ file.full_path()+ ', which should be a compile error.')
  endif
endforeach

if(not build_examples.disabled())

  if(not simulation.disabled())
    executable(
      'cli',
      'examples/cli.cpp',
      dependencies: cli_dep
    )
  endif

  # executable('led-example',
  #   'examples/led.cpp',
  #   dependencies: cli_dep
  # )

endif

if(not build_tests.disabled())
  catch_dep = dependency(
    'catch2',
    default_options: ['tests=false'],
    fallback: ['catch2', 'catch2_with_main_dep'],
    version: '>=3.4.0',
    required: true,
  )

  test_inc = include_directories('tests')

  ut = executable(
    'unit-test',
    # formatting tests
    'tests/format/bool.cpp',
    'tests/format/enum.cpp',
    'tests/format/fixed_size_sequence.cpp',
    # 'tests/format/fixpoint.cpp',
    'tests/format/int.cpp',
    'tests/format/sequence.cpp',
    'tests/format/string.cpp',
    'tests/format/struct.cpp',
    # parsing tests
    'tests/parse/bool.cpp',
    'tests/parse/char.cpp',
    'tests/parse/fixed_size_sequence.cpp',
    'tests/parse/enum.cpp',
    # 'tests/parse/fixpoint.cpp',
    'tests/parse/int.cpp',
    'tests/parse/sequence.cpp',
    'tests/parse/string.cpp',
    'tests/parse/struct.cpp',
    'tests/parse/trim_and_skip.cpp',
    # combined parse and formatting tests
    'tests/parse_format/bool.cpp',
    'tests/parse_format/enum.cpp',
    'tests/parse_format/fixed_size_sequence.cpp',
    'tests/parse_format/int.cpp',
    'tests/parse_format/sequence.cpp',
    'tests/parse_format/struct.cpp',
    'tests/command.cpp',
    'tests/ctti.cpp',
    'tests/event.cpp',
    'tests/exec_result.cpp',
    'tests/history.cpp',
    'tests/line.cpp',
    'tests/param.cpp',
    'tests/ring_buffer.cpp',
    'tests/string.cpp',
    'tests/util.cpp',
    'tests/validate.cpp',
    include_directories: test_inc,
    dependencies: [cli_dep, catch_dep],
  )

  test('unit test', ut)
endif

if(get_option('docs'))
  doxygen = find_program('doxygen', required: true)
  run_command(doxygen, 'Doxyfile', check: false)
endif

if(not cli_term.disabled())
  deps = [
    dependency('cpp-terminal', required: true),
    dependency('asio', required: true)
  ]

  executable(
    'cli-term',
    'source/cli-term.cpp',
    dependencies: deps,
    include_directories: inc,
    cpp_args:['-DASIO_NO_DEPRECATED']
  )

  executable(
    'cli-term-mock',
    'source/cli-term-mock.cpp',
    dependencies: deps,
    include_directories: inc,
    cpp_args:['-DASIO_NO_DEPRECATED']
  )
endif
