project('cpp-terminal', 'cpp', version: '1.0.0')

headers = [
  'args.hpp',
  'buffer.hpp',
  'color.hpp',
  'cursor.hpp',
  'event.hpp',
  'exception.hpp',
  'focus.hpp',
  'input.hpp',
  'iostream_initializer.hpp',
  'iostream.hpp',
  'key.hpp',
  'mouse.hpp',
  'options.hpp',
  'position.hpp',
  'prompt.hpp',
  'screen.hpp',
  'stream.hpp',
  'style.hpp',
  'size.hpp',
  'terminal_impl.hpp',
  'terminal_initializer.hpp',
  'terminal.hpp',
  'terminfo.hpp',
  'tty.hpp',
  'version.hpp',
  'window.hpp',
]
conf_data = configuration_data()
version = meson.project_version().split('.')
conf_data.set('cpp-terminal_VERSION_MAJOR', version[0])
conf_data.set('cpp-terminal_VERSION_MINOR', version[1])
conf_data.set('cpp-terminal_VERSION_PATCH', version[2])
conf_data.set('cpp-terminal_HOMEPAGE_URL', 'https://github.com/jupyter-xeus/cpp-terminal')

f = configure_file(
  input: 'cpp-terminal/private/version.cpp.in',
  output: 'version.cpp',
  format: 'cmake@',
  configuration: conf_data,
)
src = [
  'cpp-terminal/terminal_impl.cpp',
  'cpp-terminal/terminal_initializer.cpp',
  'cpp-terminal/iostream_initializer.cpp',
  'cpp-terminal/mouse.cpp',
  'cpp-terminal/args.cpp',
  'cpp-terminal/focus.cpp',
  'cpp-terminal/buffer.cpp',
  'cpp-terminal/iostream.cpp',
  'cpp-terminal/stream.cpp',
  'cpp-terminal/prompt.cpp',
  'cpp-terminal/window.cpp',
  'cpp-terminal/terminal.cpp',
  'cpp-terminal/color.cpp',
  'cpp-terminal/key.cpp',
  'cpp-terminal/event.cpp',
  'cpp-terminal/screen.cpp',
  'cpp-terminal/options.cpp',
  'cpp-terminal/cursor.cpp',
  'cpp-terminal/style.cpp',
  'cpp-terminal/private/return_code.cpp',
  'cpp-terminal/private/file_initializer.cpp',
  'cpp-terminal/private/exception.cpp',
  'cpp-terminal/private/unicode.cpp',
  'cpp-terminal/private/conversion.cpp',
  'cpp-terminal/private/args.cpp',
  'cpp-terminal/private/terminal_impl.cpp',
  'cpp-terminal/private/tty.cpp',
  'cpp-terminal/private/terminfo.cpp',
  'cpp-terminal/private/input.cpp',
  'cpp-terminal/private/screen.cpp',
  'cpp-terminal/private/cursor.cpp',
  'cpp-terminal/private/file.cpp',
  'cpp-terminal/private/env.cpp',
  'cpp-terminal/private/blocking_queue.cpp',
  'cpp-terminal/private/sigwinch.cpp',
  'cpp-terminal/private/signals.cpp',
  f,
]

inc = include_directories('.')
cpp_terminal_lib = static_library(
  'cpp-terminal',
  sources: src,
  include_directories: inc,
  implicit_include_directories: true,
)
cpp_terminal_dep = declare_dependency(link_with: cpp_terminal_lib, include_directories: inc)
meson.override_dependency('cpp-terminal', cpp_terminal_dep)
