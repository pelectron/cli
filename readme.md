# CLI

`CLI` is a C++20 header-only library to build hierarchical command line
interfaces for embedded systems. It can be used to set and retrieve parameters
and call functions on the embedded device.

In addition, `CLI` also provides the command line executable `cli-term` to
connect to systems running `CLI` over serial port or ethernet.

## Library Features

- C++20
- header only
- easy to use
- completely constexpr
- type safe
- macro free
- customizable
- fully documented with doxygen and handwritten [markdown docs](docs.md)
- opt in functionality:
  - autocomplete
  - cursor movement
  - help command
  - detailed error reporting
- smallish code size, depending on configuration
  - example: autocomplete, cursor movement, help, and detailed errors are
    disabled -> base library code size is about 2kB on an arm Cortex-M3
  - enabling everything adds about 7.5kB on an arm Cortex-M3
  - each command adds 0.5kB - 1.5kB, depending on how many different types are
    used.

## Contents

- [Library Features](#library-features)
- [Short Example](#a-short-example)
- [How To Build](#how-to-build)
  - [Dependencies](#dependencies)
  - [Meson](#meson)
    - [As a dependency](#as-a-dependency)
    - [Building cli-term](#building-cli-term)
    - [Installing cli-term](#installing-cli-term)
    - [Installing The Headers](#installing-the-headers)
  - [Other Build Systems](#other-build-systems)
    - [As A Library](#as-a-library)
    - [Building](#building)
  - [Examples](#examples)
- [Simulation](#simulation)
- [Documentation](#documentation)
- [Design Rationale](#design-rationale)
- [Suported Compilers](#supported-compilers)
- [Contributions](#contributions)

## A Short Example

A short example to visualize what CLI is trying to do:

Imagine your embedded application has some parameters and some functionality
that you want to trigger remotely.

```cpp
struct Settings{
  int foo;
  char bar;
};

// the settings
static Settings settings;

enum class Mode{
  normal,
  moderate,
  extreme
};

// the functionality
int gooify(Mode mode, int n);
```

You want to modify the settings, and trigger `gooify`.
With CLI, the setup is easy, and you will be able to to this, in for example a
serial terminal, like so:

```{bash}
settings = {foo = 5, bar = k} # sets settings.foo to 5 and settings.bar to 'k'
settings.foo = 42 # set settings.foo to 42
settings.bar = 'x' # set settings.bar to 'x'
settings # returns {foo = 42, bar = 'x'}
settings.foo # returns 42
settings.bar # returns 'x'
gooify(normal, 15) # calls gooify(Mode::normal, 15)
gooify(n = 10, mode = extreme) # calls gooify(Mode::extreme, 10)
```

In your main application, you will create an Engine object, and populate it
with the parameters and functions, and call the engine's `on_char()` and
`process()` methods.

`on_char` must either be called in your character reception ISR, or in the main
thread before process. It preprocesses the character input and puts the data into
an internal buffer. For simplicity, this example assumes a blocking function
that retrieves one character from your communication peripheral called `get_char()`.

`process()` is the method takes the data from the input buffer and actually
processes the input.

To output data, a function called `send_char()` will be assumed to exist.

```cpp
#include <cli.hpp>

char get_char();
cli::Error send_char(char c);

using cli::operator""_sc;
using cli::operator""_arg;

static constinit cli::Engine the_cli{
  cli::default_config{}, // your configuration structure
  cli::AnsiDisplay{&send_char}, // your cli::Display
  // the parameter commands
  cli::param("settings"_sc,
             "settings description"_sc,
             settings,
             cli::param("foo"_sc,
                        "foo description"_sc,
                        &Settings::foo),
             cli::param("bar"_sc,
                        "bar description"_sc,
                        &Settings::bar)),
  // the function commands
  cli::func("gooify"_sc,
            "gooify description"_sc,
            &gooify,
            "mode"_arg,
            "n"_arg
            )
};

int main(){
  while(1){
    char c = get_char();
    the_cli.on_char(c);
    the_cli.process();
  }
}
```

For more involved examples, see `examples/cli.cpp` and `examples/led.cpp`.

## How to build

Using `CLI` as a library requires no build because it is a header only library.
For meson, refer to [this section](#meson). For other build systems, see
[here](#other-build-systems).

### Dependencies

When using the simulation components, you will need
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal).

When building the `cli-term` executable, you must also have
[asio](https://think-async.com/Asio/) installed.

For testing, [catch2](https://github.com/catchorg/Catch2) is used.

With meson, these dependencies will downloaded if the are not installed.

### Meson

#### As a dependency

If you use meson, add CLI as a subproject or wrap and use it like so:

```{python}
# with wrap
cli_dep = dependency(
  'cli',
  default_options: [
    'tests=disabled',
    'examples=disabled',
    'sim=disabled',
    'cli-term=disabled',
  ]
)

# as subproject
cli_dep = subproject(
  'cli',
  default_options: [
    'tests=disabled',
    'examples=disabled',
    'sim=disabled',
    'cli-term=disabled',
  ]
)
```

#### Building cli-term

To build `cli-term`, the `cli-term` option must be enabled. Simply execute the
following command in the project root directory:

```bash
meson setup build -Dcli-term=enabled -Dauto_features=disabled 
meson compile -C build
```

This will generate `cli-term` in the `build` directory.

If you are on windows, you may need to add `--vsenv` to the meson setup command

```bash
meson setup build -Dcli-term=enabled -Dauto_features=disabled --vsenv
meson compile -C build
```

#### Installing cli-term

To install `cli-term`, either use meson's install command or copy `cli-term` to
the desired location.

```bash
meson setup build -Dcli-term=enabled -Dauto_features=disabled
meson install -C build --tags bin --skip-subprojects
```

**NOTE**: To customize the installation directory, specify the `--prefix`
option in meson's setup command. On Linux, the prefix defaults to `usr/local/`.
On Windows the prefix defaults to `C:\`.

Windows example (administrator command prompt is needed):

```bash
meson setup build -Dcli-term=enabled -Dauto_features=disabled --vsenv --prefix="C:/Program Files/cli/"
meson install -C build --tags bin --skip-subprojects
```

#### Installing The Headers 

Either copy the `include` directory to the desired location or use meson's install command.

```bash
meson setup build -Dauto_features=disabled
meson install -C build --tags devel --skip-subprojects
```

On windows, you may need to add the `--vsenv` option and adjust the prefix.

```bash
meson setup build -Dauto_features=disabled --vsenv
meson install -C build --tags devel --skip-subprojects
```

#### Installing All In One 

```bash
meson setup build -Dauto_features=disabled -Dcli-term=enabled
meson install -C build --skip-subprojects
```

### Other Build Systems

#### As A Library

Just add CLI's `include` folder to your build
system's include directories and you are set.

To use the simulation component, you must have
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal) available, add
it's include path and link against it.

#### Building

To build `cli-term`, you must have [asio](https://think-async.com/Asio/) and
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal) available. 

Compile `source/cli-term/cli-term.cpp` and `source/cli-term/main.cpp` into `cli-term`.

Example:

```bash
gcc source/cli-term/cli-term.cpp source/cli-term/main.cpp -Iinclude -Lcpp-terminal
```

### Examples

If you wish to test out the library, you can build the examples. In the project
root directory, invoke:

```{bash}
meson setup build -Dexamples=enabled
meson compile -C build
```

This will create the `cli` and `cli-singleline` executable in the `build`
directory.

## Simulation

To simulate your own command line interfaces on the PC set the `sim` project
option to enabled.

Then you can include `cli/sim.hpp` and use your cli on the PC in a terminal:

```cpp
// my_cli.cpp
#include "cli/sim.hpp"

struct Config{...};

int main(){
  if (not cli::sim::init())
    return -1;

  cli::Cli my_cli = cli::sim::create_cli(Config{}, commands...);

  my_cli.print();

  while (cli::sim::get_input_and_process(my_cli)) {
  }


  return 0;
}
```

and build it with:

```{python}
executable(
  'my-cli',
  'my_cli.cpp',
  dependencies: cli_dep
)
```

If you don't use meson but want the simulation capability, you will need
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal).

## Documentation

This project is documented using doxygen. Either use the provided `Doxyfile`
and manually invoke doxygen or build the documentation with meson by setting
the `docs` project option to true.

To get a good overview of the main components and a condensed reference, use
the [accompanying markdown documentation (docs.md)](./docs.md).

```bash
meson setup build -Ddocs=true
meson compile -C build
```

## Design Rationale

Why C++20 and not some earlier standard?
  - concepts 
  - consteval
  - constinit

## Supported Compilers 

`CLI` is tested  with `clang`, `gcc`, `arm-none-eabi-gcc` and `msvc`. If you
want to use `CLI` with another compiler, `cli/ctti.hpp` most likely needs to be
extended.

## Contributions

Contributions are welcome and appreciated, especially for adding additional
compiler support. Just make a pull request. 
