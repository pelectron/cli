project('cli', 'cpp', meson_version: '>=1.1.0', default_options: ['cpp_std=c++20'])

inc = include_directories('include')

build_tests = get_option('tests')
build_examples = get_option('examples')
simulation = get_option('sim')

# testing, building examples, and simulation can only be done for  non-embeded targets
build_tests.disable_if(meson.is_cross_build(), error_message: 'cant build tests for embedded targets')
build_examples.disable_if(meson.is_cross_build(), error_message: 'cant build examples on embedded targets')
simulation.disable_if(meson.is_cross_build(), error_message: 'cant have simulation with embedded targets')

deps = [
  dependency('gcem', required: true),
]

if(not simulation.disabled())
  deps += dependency('cpp-terminal', required: true)
endif

cli_dep = declare_dependency(
  include_directories:inc, 
  dependencies: deps
)

meson.override_dependency('cli',cli_dep)

if(not build_examples.disabled())

  if(not simulation.disabled())
    executable(
      'cli',
      'examples/cli.cpp',
      dependencies: cli_dep
    )
  endif

  executable('led-example',
    'examples/led.cpp',
    dependencies: cli_dep
  )
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
    'tests/format/bool.cpp',
    'tests/format/enum.cpp',
    # 'tests/format/fixpoint.cpp',
    'tests/format/int.cpp',
    'tests/format/sequence.cpp',
    'tests/format/string.cpp',
    'tests/format/struct.cpp',
    'tests/parse/bool.cpp',
    'tests/parse/fixed_size_sequence.cpp',
    'tests/parse/enum.cpp',
    # 'tests/parse/fixpoint.cpp',
    'tests/parse/int.cpp',
    'tests/parse/sequence.cpp',
    'tests/parse/string.cpp',
    'tests/parse/struct.cpp',
    'tests/history.cpp',
    'tests/param.cpp',
    'tests/string.cpp',
    # 'tests/tracker.cpp',
    include_directories: test_inc,
    dependencies: [cli_dep, catch_dep],
  )

  test('unit test', ut)
endif

if(get_option('docs'))
  doxygen = find_program('doxygen', required: true)
  run_command(doxygen, 'Doxyfile')
endif
