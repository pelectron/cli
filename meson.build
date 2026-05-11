project(
  'cli',
  'cpp',
  version: '1.0',
  license: 'BSL-1.0',
  license_files: 'LICENSE_1_0.txt',
  meson_version: '>=1.1.0',
  default_options: ['cpp_std=c++20'],
)

inc = include_directories('include')

build_tests = get_option('tests')
build_examples = get_option('examples')
simulation = get_option('sim')
cli_term = get_option('cli-term')
embedded = get_option('embedded')

# testing, building examples, simulation, and cli-term can only be used
# for non-embeded targets
build_tests.disable_if(
  embedded,
  error_message: 'can\'t build tests for embedded targets',
)

build_examples.disable_if(
  embedded,
  error_message: 'can\'t build examples on embedded targets',
)

simulation.disable_if(
  embedded,
  error_message: 'can\'t have simulation with embedded targets',
)

cli_term.disable_if(
  embedded,
  error_message: 'cli-term can\'t be built for embedded targets',
)

# settings up the cli dependency
deps = []

if (not simulation.disabled())
  deps += dependency('cpp-terminal', required: true, default_options: ['warning_level=0'])
endif

cli_dep = declare_dependency(include_directories: inc, dependencies: deps)

meson.override_dependency('cli', cli_dep)

# headers to install
install_headers(
  'include/cli.hpp',
)

install_headers(
  'include/cli/command.hpp',
  'include/cli/command_tree.hpp',
  'include/cli/concepts.hpp',
  'include/cli/config.hpp',
  'include/cli/ctti.hpp',
  'include/cli/display.hpp',
  'include/cli/engine.hpp',
  'include/cli/enums.hpp',
  'include/cli/event.hpp',
  'include/cli/exec_result.hpp',
  'include/cli/format.hpp',
  'include/cli/function.hpp',
  'include/cli/help.hpp',
  'include/cli/history.hpp',
  'include/cli/input.hpp',
  'include/cli/line.hpp',
  'include/cli/param.hpp',
  'include/cli/parse.hpp',
  'include/cli/ring_buffer.hpp',
  'include/cli/string.hpp',
  'include/cli/traits.hpp',
  'include/cli/type_list.hpp',
  'include/cli/u64.hpp',
  'include/cli/util.hpp',
  'include/cli/validator.hpp',
  'include/cli/vector.hpp',
  subdir: 'cli',
)

if (not simulation.disabled())
  install_headers(
    'include/cli/sim.hpp', 
    subdir: 'cli',
  )
endif

if (not build_examples.disabled())
  if (not simulation.disabled())
    executable('cli', 'examples/cli.cpp', dependencies: cli_dep)
    executable('cli-singleline', 'examples/cli-singleline.cpp', dependencies: cli_dep)
  endif
endif

if get_option('led-example').enabled()
  executable('led-example', 'examples/led.cpp', 'examples/syscalls.cpp', dependencies: cli_dep)
  executable('empty', 'examples/empty.cpp', 'examples/syscalls.cpp', dependencies: cli_dep)
endif

if (not build_tests.disabled())

  # check for failing tests that need to fail
  must_fail_to_compile = files('tests/fails/param_t.cpp')

  cpp = meson.get_compiler('cpp')

  foreach file : must_fail_to_compile
    if (cpp.compiles(file, include_directories: inc))
      error(
        'CLI is able to compile '
        + file.full_path()
        + ', which should be a compile error.',
      )
    endif
  endforeach

  # unit test dependencies
  catch_dep = dependency(
    'catch2',
    default_options: [
      'tests=false', 
      'warning_level=0',
      'prefer_static=true',
    ],
    fallback: ['catch2', 'catch2_with_main_dep'],
    version: '>=3.4.0',
    required: true,
  )

  test_inc = include_directories('tests')

  # unit test warnings to disable
  if cpp.get_id() == 'clang' or cpp.get_id() == 'gcc'
    exclude_warnings = [
      '-Wno-abi-tag',
      '-Wno-effc++',
      '-Wno-ctor-dtor-privacy',
      '-Wno-padded',
      '-Wno-shadow',
      '-Wno-unknown-warning-option',
    ]
  else
    exclude_warnings = []
  endif

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
    'tests/ansidisplay.cpp',
    'tests/command.cpp',
    'tests/command_tree.cpp',
    'tests/ctti.cpp',
    'tests/display.cpp',
    'tests/event.cpp',
    'tests/exec_result.cpp',
    'tests/history.cpp',
    'tests/input.cpp',
    'tests/help.cpp',
    'tests/line.cpp',
    'tests/param.cpp',
    'tests/ring_buffer.cpp',
    'tests/string.cpp',
    'tests/util.cpp',
    'tests/validate.cpp',
    include_directories: test_inc,
    cpp_args: exclude_warnings,
    dependencies: [cli_dep, catch_dep],
    install_tag: 'tests',
  )

  test('unit test', ut)
endif

if (get_option('docs'))
  doxygen = find_program('doxygen', required: true)
  run_command(doxygen, 'Doxyfile', check: false)
endif

if (not cli_term.disabled())
  # cli-term dependencies
  deps = [
    dependency('cpp-terminal', required: true),
    dependency('asio', required: true),
  ]

  executable(
    'cli-term',
    'source/cli-term/cli-term.cpp',
    'source/cli-term/main.cpp',
    dependencies: deps,
    include_directories: inc,
    install: true,
    override_options:[
      'warning_level=3', 
      'buildtype=release'
    ],
    cpp_args: [
      '-DASIO_NO_DEPRECATED',
    ],
    install_tag: 'bin'
  )
endif
