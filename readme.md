<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [About](#about)
- [How to build](#how-to-build)
  - [Meson](#meson)
  - [Other Build Systems](#other-build-systems)
  - [Examples](#examples)
- [Simulation](#simulation)
- [Components](#components)
  - [Config](#config)
  - [Output](#output)
  - [Parameter Commands](#parameter-commands)
    - [Parameters Without Object Declarations](#parameters-without-object-declarations)
    - [Parameters With Object/Variable Declarations](#parameters-with-objectvariable-declarations)
    - [Parameters With const Object/Variable Declarations](#parameters-with-const-objectvariable-declarations)
    - [Member Data Parameters](#member-data-parameters)
  - [Function Commands](#function-commands)
    - [Arguments](#arguments)
      - [Optional Arguments](#optional-arguments)
      - [Required Arguments](#required-arguments)
      - [Deduced Arguments](#deduced-arguments)
- [TODOs](#todos)
- [How to use](#how-to-use)
- [Simple Example](#simple-example)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## About

CLI is a C++20 header only library to build hierarchical ANSI command line
interfaces for embedded systems. It can be used to set and retrieve
parameters and call functions on the embedded device.

A short example to visualize what CLI is trying to do:

Imagine your embedded application has some parameters and some functionality
that you want to trigger remotely.

```cpp
// the settings
struct Settings{
  int foo;
  char bar;
};

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
settings.bar # returns 'k'
gooify(normal, 15) # calls gooify(Mode::normal, 15)
gooify(n = 10, mode = extreme) # calls gooify(Mode::extreme, 10)
```

In your main application, you will create a Cli object, and populate it with the
parameters and functions, and calls the Cli object's `on_char()` and `process()`
methods.

`on_char` must either be called in your character reception ISR, or in the main
thread before process. It preprocesses the character input and puts the data into
an internal buffer. For simplicity, this example assumes a blocking function
that retrieves one character from your communication peripheral called `get_char()`.

`process()` is the method takes the data from the input buffer and actually
processes the input.

To output data, a function called `send_char()` will be assumed to exist.

```cpp
char get_char();
cli::Error send_char(char c);

static cli::Cli the_cli{
  cli::default_config{}, // your configuration structure
  cli::AnsiOutput{cli::default_config{}, &send_char}, // your cli::Output
  cli::param("settings"_sc,
             "settings description"_sc,
             settings,
             cli::param("foo"_sc,
                        "foo description"_sc,
                        &Settings::foo),
             cli::param("bar"_sc,
                        "bar description"_sc,
                        &Settings::bar)),
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

That's all there is to it.

## How to build

CLI is a header only library, so there is nothing to build.

### Meson

If you use meson, add CLI as a subproject and use it like so:

```{python}
cli_dep = dependency(
  'cli',
  default_options: [
    'tests=disabled',
    'examples=disabled',
    'sim=disabled'
  ]
)
```

### Other Build Systems

If you use another build system, you will also need
[gcem](https://github.com/kthohr/gcem). Just add CLI's and gcem's
`include` folder to your build system's include directories and you are set.

### Examples

If you wish to test out the library, you can build the examples. In the project
root directory, invoke:

```{bash}
meson setup build -Dtests=disabled
meson compile -C build
```

This will create the `cli` executable in the `build` directory.

## Simulation

To simulate your own command line interfaces on the PC set the `sim` project
option to auto or enabled.

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

If you dont use meson but want the simulation capability, you will need
[cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal).

## Components

CLI has the following components.

### Config

The `cli::Config` is a type traits like structure to configure CLI.
This tells CLI what the character type is, how large the buffers it uses should
be, and which features to use.

An example is given here:

```cpp
struct my_cli_config {
  // the character type to be used
  using char_type = char;
  // the character used to separate subcommands
  static constexpr char_type access_separator = '.';
  // the delimiter for commands, can be line feed('\n'), carriage return('\r'),
  // or carriage return followed by linefeed ("\r\n")
  static constexpr auto delimiter = cli::Delimiter::lf;
  // if true, all commands must start with the command separator
  static constexpr bool commands_start_with_separators = false;
  // the transmit buffer size. This has to be big enough to fit the largest
  // string output, i.e. the largest answer.
  static constexpr std::size_t tx_size = 128;
  // the receive buffer size. This can be kept fairly small because CLI
  // processes the input character by character.
  static constexpr std::size_t rx_size = 32;
  // if true, the autocomplete feature is enabled
  static constexpr bool use_autocomplete = true;

```

### Output

CLI defines the `cli::Output` concept to use as a terminal/display device.

```cpp

template<class S, typename Char>
concept Output = requires(S &stream,
                          Char c,
                          cli::View<const Char> s,
                          cli::Control ctrl) {
  // writes a raw character c
  { stream.write(c) } -> std::same_as<Error>;
  // writes the raw string s
  { stream.write(s) } -> std::same_as<Error>;
  // executes a control
  { stream.control(ctrl) } -> std::same_as<Error>;
};
```

To enable easy use with existing character stream, for example an embedded UART
connected to an ANSI capable terminal, CLI provides the class `cli::AnsiOutput`.

`cli::AnsiOutput` requires a `cli::Config` and a `cli::BasicOutputStream` to operate.

A `BasicOutputStream` is just a callable that takes a character or a `cli::View`
and returns a `cli::Error`.

Example:

```cpp
cli::Error write_char(char c);
cli::Error write_string(cli::View<const char> string);
```

Now the `AnsiOutput` can be created:

```cpp
struct config{...};

static_assert(cli::Config<config>);

cli::AnsiOutput output1{config{}, &write_char};
cli::AnsiOutput output2{config{}, &write_char};
```

### Parameter Commands

Parameters are what they sound like. They are values the CLI makes available.

As an example:
A parameter with the name "enable" with type bool can be set with
`enable = true`, and can be retrieved with `enable` (which will print
`true` or `false`)

To create a parameter, use the `cli::param` template overload set.

There are several different forms for creating parameters, described below.

#### Parameters Without Object Declarations

Paramters Without Object/Variable Declarations

Parameter commands without an object/variable declaration can be setup
with the following functions.

The basic form is:

```cpp
param<T>(name, description, get, set, parse, format, validate, subcommands...);
```

The parts have the following functions:

- T: the parameter's type
- name: a `cli::string_constant` that makes up the command name.
- description: a `cli::string_constant` that describes the command.
- get: a Getter for a T. It retrieves the value associated with the parameter.
  See also `cli::param::Setter` and `cli::param::SetterOf`.
- set: a Setter for a T. It sets the value associated with the parameter.
  See also `cli::param::Getter` and `cli::param::GetterOf`.
- parse: a Parser for a T. It parses a T from a string. See also
  `cli::parse::Parser` and `cli::parse::ParserOf`.
- format: a Formatter for a T. It formats a T to a string. See also
  `cli::format::Formatter` and `cli::format::FormatterOf`.
- validate: a Validator for a T. It validates parsed values before they are set.
  See also `cli::validate::Validator` and `cli::validate::ValidatorOf`.
- subcommands: any amount of subcommands, either further parameters or functions.

There are a multitude of overloads so that certain parts can be left out,
if you wish to use the defaults provided by cli.

Note that:

- leaving out the setter creates a read-only parameter. A parser is then not
  necessary.
- leaving out the getter creates a write-only parameter. A formatter is then not
  necessary.

The available overloads are:

```cpp
// the basic/full form
param<T>(name, description, get, set, parse, format, validate);

// a parameter with default validator
param<T>(name, description, get, set, parse, format);

// a parameter with default parser and formatter
param<T>(name, description, get, set, validate);

// a write-only parameter with custom parser and validator
param<T>(name, description, set, parse, validate);

// default parser, formatter and validator are used
param<T>(name, description, get, set);

// a write-only parameter with custom parser
param<T>(name, description, set, parse);

// a read-only parameter with custom formatter
param<T>(name, description, get, format);

// a write-only parameter with custom validator
param<T>(name, description, set, validate);

// a read-only parameter
param<T>(name, description, get);

// a write-only parameter
param<T>(name, description, set);
```

#### Parameters With Object/Variable Declarations

The following functions can be used to setup parameters with
object/variable declarations.

The basic form is:

```cpp
param(name, description, t, get, set, parse, format, validate, subcommands...);
```

The parts have the following functions:

- name: a `cli::string_constant` that makes up the command name.
- description: a `cli::string_constant` that describes the command.
- t: the variable/object of type T that holds the value of the parameter.
- get: a Getter for a T. It retrieves the value associated with the parameter.
  See also `cli::param::Setter` and `cli::param::SetterOf`.
- set: a Setter for a T. It sets the value associated with the parameter.
  See also `cli::param::Getter` and `cli::param::GetterOf`.
- parse: a Parser for a T. It parses a T from a string. See also
  `cli::parse::Parser` and `cli::parse::ParserOf`.
- format: a Formatter for a T. It formats a T to a string. See also
  `cli::format::Formatter` and `cli::format::FormatterOf`.
- validate: a Validator for a T. It validates parsed values before they are set.
  See also `cli::validate::Validator` and `cli::validate::ValidatorOf`.
- subcommands: any amount of subcommands, either further parameters or functions.

There are a multitude of overloads so that certain parts can be left out,
if you wish to use the defaults provided by cli.

The parts that can be left out are:

- get: in that case, cli::param::DefaultGet is used.
- set: in that case, cli::param::DefaultSet is used.
- validate: in that case, cli::validate::DefaultValidate is used.
- parse and format: in that case, cli::parse::DefaultParse and
  cli::format::DefaultFormat are used.

#### Parameters With const Object/Variable Declarations

Read-only parameters for const objects can be defined with the
following functions.

The base form is:

```cpp
param(name, description, t, get, format)
```

In total there are four overloads, where get, or format, or both, can be left
out. In that case, a default getter and/or default formatter are used.

#### Member Data Parameters

Member data commands are used to easily setup subcommands for parameters
with subobjects.

This is easiest explainable by example.
Take this struct and its variabe definition:

```cpp
 struct Settings{
   int foo;
   char baz;
 };

 static Settings settings;
```

To make the settings and its members foo and baz available to cli, you can
use the following param calls to easily setup this structure.

```cpp
 param("settings"_sc, "core settings", settings,
         param("foo"_sc, "foo mode"_sc, &Settings::foo),
         param("baz"_sc, "baz setting"_sc, &Settings::baz));
```

Then `settings`, `settings.foo` and `settings.baz` can be used as
parameter commands like so:

```{python}
# setting the parameter
settings = {foo = 1, baz = k}
# retrieving the parameter
settings
{foo = 1, baz = 'k'}

# setting the parameter
settings.foo = 5
# retrieving the parameter
setting.foo
5

# setting the parameter
settings.baz = k
# retrieving the parameter
settings.baz
'k'
```

The full list of member data parameter functions is:

```cpp
 param(name, description, ptr_to_member, parse, format, validate);
 param(name, description, ptr_to_member, parse, format);
 param(name, description, ptr_to_member, validate);
 param(name, description, ptr_to_member);
```

In this case, parse, format, and validate are parsers, formatters and validators
for the type pointed to by ptr_to_member.

There are also overloads available for const member data.

```cpp
 param(name, description, ptr_to_member, format);
 param(name, description, ptr_to_member);
```

### Function Commands

Functions are commands that can be called. They take arguments and optionally return a value.

#### Arguments

Arguments are the elements that describe c++ function arguments.
These argument specifications are then used by Functions to parse the
input into the specified values and validate them.

There are two kinds of function arguments:

- required: these parameters must be specified, else it is an error.
- optional: these parameters can be left out because they have a default
  value.

Arguments are fully specified by their:

- name: the human readable name
- description: a string that is used by the help functionality
- type: the value type of the argument
- parser: used to parse a value of the arguments type
- validator: used to validate the parsed value
- optionally, a default value

`cli::arg` is the templated overload set to use for creating arguments. There
are two main templates forms, one for required and one for optional arguments.

##### Optional Arguments

The base form for an optional arguments is:

```cpp
template <class T, // the arguments type, must be explicitly specified
          auto DefaultValue, // the default value, must be explicitly specified
          SC Name, // must be specified
          SC Description, // either specified or left out
          parse::Parser Parse, // either specified or deduced
          validate::Validator Validate // either specified or deduced
          >
constexpr auto arg( Name name, // the name
                    Description description, // the description
                    Parse &&parse, // the parser
                    Validate &&validate // the validator
                    );
```

The parser and validator can be left out.

##### Required Arguments

The base form for a required arguments is:

```cpp
template <class T, // the arguments type, must be explicitly specified
          SC Name, //must be specified
          SC Description, // either specified or left out
          parse::Parser Parse, // either specified or deduced
          validate::Validator Validate // either specified or deduced
          >
constexpr auto arg( Name name, // the name
                    Description description, // the description
                    Parse &&parse, // the parser
                    Validate &&validate // the validator
                    );
```

The parser and validator can be left out.

##### Deduced Arguments

Deduced arguments are arguments that have their type deduced. The default
parser and validator are always used for these type of arguments. The
benefit of deduced arguments is the shorter notation.

There are two functions for creating deduced arguments:

```cpp
arg(name, description)
arg(name)
```

and the literal operator `_arg`

```cpp
"name"_arg
```

## TODOs

- full doxygen documentation
- fixed size sequence parsing and formatting
- output config
- more testing
- make help command more detailed
  - print also subcommands
  - print function arguments and their description

## How to use

## Simple Example
