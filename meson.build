project('cli', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('include')

build_tests = get_option('tests')
build_examples = get_option('examples')
# testing and building examples can only be done for o none embeded targets
build_tests.disable_auto_if(meson.is_cross_build())
build_examples.disable_auto_if(meson.is_cross_build())

deps = [
  dependency('gcem', required: true),
]

cli_dep = declare_dependency(include_directories:inc, dependencies: deps)
meson.override_dependency('cli',cli_dep)

if(not build_examples.disabled())
  terminal_dep = dependency('cpp-terminal', required: true)

  executable(
    'cli',
    'examples/cli.cpp',
    dependencies: [cli_dep, terminal_dep]
  )

  executable('led-example',
    'examples/led.cpp',
    dependencies: cli_dep)
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
