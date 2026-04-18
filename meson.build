project('cli', 'cpp', default_options: ['cpp_std=c++20'])

inc = include_directories('include')

native_deps = [
  dependency('gcem', native: true),
]

deps = [
  dependency('gcem'),
  dependency('cpp-terminal'),
]

catch_dep = dependency(
  'catch2',
  default_options: ['tests=false', 'buildtype=debug'],
  fallback: ['catch2', 'catch2_with_main_dep'],
  version: '>=3.4.0',
  required: true,
  native: true,
)

cli_dep = declare_dependency(include_directories:inc, dependencies: native_deps)
meson.override_dependency('cli',cli_dep)

executable(
  'cli-test',
  'tests/cli.cpp',
  include_directories: inc,
  native: true,
  dependencies: deps,
)

executable(
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
  include_directories: [inc, include_directories('tests')],
  dependencies: [deps, native_deps, catch_dep],
)

# executable('led-example',
#   'examples/led.cpp',
#   include_directories: inc,
#   dependencies: cli_dep)
