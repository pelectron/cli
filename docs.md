# CLI library documentation

`CLI` consists of the following components:

- [`cli::Engine`](#engine)
- [`cli::concepts::Config`](#config): configuration for the `cli::Engine`
- [`cli::concepts::Input`](#input-concept) and [`cli::Input`](#input): used to
  receive characters
- [`cli::concepts::Display`](#display-concept) and
  [`cli::AnsiDisplay`](#ansidisplay): used to display characters
- [`cli::concept::Command`](#commands): the commands, which are either
  [Parameters](#parameters) or [Functions](#functions)
- [`cli::string_constant`](#string-constant): a compile time string
- [Parsing](#parsing)
- [Formatting](#formatting)

## Engine

The Engine class is composed of a [`config`](#config), an
[`input`](#input-concept), a [`display`](#display-concept), and
[`commands`](#commands).

In your main application, you will create an Engine object, and populate it
with the parameters and functions, and calls the engine's `on_char()` and
`process()` methods.

`on_char` must either be called in your character reception ISR (see
[input concept](#input-concept) for details), or in the main thread before
process. It preprocesses the character input and puts the data into an internal
event buffer.

`process()` is the method takes the data from the event buffer and actually
processes the input.

### Engine Example

```cpp
#include <cli.hpp>

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

char get_char();
cli::Error send_char(char c);

using cli::operator""_sc;
using cli::operator""_arg;

static constinit cli::Engine the_cli{
  cli::default_config{}, // your configuration structure
  cli::AnsiDisplay{&send_char}, // your Display
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

## Config

The [`Engine`](#engine) uses a configuration. A configuration is a traits-like
structure that sets core aspects of the engine. There is a concept called
`cli::concepts::Config` that formalizes the configuration.

A configuration of type `C` must have the following static constexpr members
and typedefs to satisfy the `Config` concept:

- `char_type`: a typedef for the character type to use. Can be any of `char`,
  `signed char`, `unsigned char`, `char8_t`, `char16_t`, `char32_t`.
- `name`: convertible to `cli::View<const char_type>`. The name of the cli as a
  string.
- `description`: convertible to `cli::View<const char_type>`. The description
  of the cli.
- `access_separator`: of type `char_type`. This specifies what kind of
  character is used to delimit individual sub commands.
- `use_autocomplete`: of type `bool`. If true, the engine uses autocomplete.
- `use_cursor`: of type `bool`. If true, the engine recognizes cursor movement.
  If true, your [display](#display-concept) must support cursor movement.
- `use_history`: of type `bool`. Specifies if the engine implements a command
  history. If true, then the configuration must also specify a member
  `history_depth` of type `std::size_t`. `history_depth` specifies how many
  commands can be stored in the history.
- `max_line_length`: of type `std::size_t`. Specifies how long the maximum
  command input is. Commands longer than this length cannot be processed.

### Optional Entries

Optional entries of a configuration are static constexpr members that can be
left out because `CLI` uses a default value if they are not specified. Entries
with name `X` of a configuration type `C` can be retrieved with
`cli::config::X_v<C>` (for members) and `cli::config::X_t<C>` (for typedefs).

- `input_type`: a typedef satisfying the [input concept](#input-concept).
  Defaults to [`cli::Input`](#input). You must leave this entry out if you want
  to use the default.
- `input_delimiter`: of type `cli::Delimiter`. Specifies the character
  sequence for the enter key. The default is `cli::Delimiter::lf`.
- `input_size`: of type `std::size_t`. Specifies how many elements the
  internal Event buffer of `cli::Input` stores. The default is 32.
- `use_volatile_input_buffer`: of type `bool`. Specifies the use of a volatile
  buffer for the input. Must be set to true if the engine's `on_char` method is
  called in an ISR. Custom input types should respect this value. Defaults to
  false.
- `output_size`: of type `std::size_t`. Specifies how big the buffer for
  outputting characters is. The default is `max_line_length`.

### Config Example

```cpp
#include <cli.hpp>

struct my_config{
  using char_type = char;
  static constexpr View<const char> name = "cli";
  static constexpr View<const char> description = "a command line interface";
  static constexpr char_type access_separator = '.';
  static constexpr bool use_autocomplete = false;
  static constexpr bool use_cursor = false;
  static constexpr bool use_history = true;
  // history_depth can be left out if use_history is false
  static constexpr std::size_t history_depth = 16;
  static constexpr std::size_t max_line_length = 80;

  // the entries below can be left out if the default is acceptable.
  static constexpr cli::Delimiter input_delimiter = cli::Delimiter::lf;
  static constexpr std::size_t input_size = 32;
  static constexpr std::size_t output_size = 80;
  static constexpr bool use_volatile_input_buffer = false;
  // using input_type = my_input_type;
};

static_assert(cli::concepts::Config<my_config>);
```

## Input Concept

The input concept formalizes the interface that the [`Engine`](#engine) uses
for preprocessing character input.

An Input preprocesses the character input received with `on_char` into a
sequence of `cli::Event`. This sequence is then accessed in a FIFO order via
`pop_event` by the [engine](#engine). This way, `on_char` could be called in an
interrupt service routine because preprocessing the input is not a very
expensive operation.

```cpp
template<typename I, typename CharT>
concept cli::concepts::Input;
```

For a variable `input` of type `I`, a `character` value of type `CharT`,
and a non-const lvalue reference `event` of type `cli::Event<CharT>&`, the
following must be satisfied:

- `I` is constructible without any arguments.
- `input.on_char(character)`: is called by the engine when a character is
  received. Must return a `cli::Error`. `on_char` preprocesses the character
  input and stores a resulting `cli::Event` in an internal buffer.
- `input.pop_event(event)`: called by the engine to get the next available
  event. Must return a `bool`, which indicates that an event has been popped
  (`true`), or that no event was available (`false`).

`CLI` provides a default implementation called [`cli::Input`](#input).

### Input

The default implementation of the [Input Concept](#input-concept). If you
want to use a custom input, your [Config](#config) must specify an inner
typedef called `input_type`. This `input_type` must satisfy the [Input
Concept](#input-concept).

Note: f you want to call the engine's `on_char` method in an ISR, you must add
a static constexpr member called `use_volatile_input_buffer` of type `bool` to
your [`Config`](#config) and set it to true. If you want your own input type to
respect that setting, you can use `cli::config::use_volatile_input_buffer_v` to
query this configuration value.

```cpp
template<cli::concepts::Config Cfg>
class cli:Input{
public:
  using char_type = typename Cfg::char_type;
  using event_type = cli::Event<char_type>;

  constexpr cli::Error on_char(char_type c);
  constexpr bool pop_event(event_type& event);
  constexpr void reset();
}
```

## Display Concept

`CLI` uses the `cli::concepts::Display` concept to specify the interface `CLI`
uses to output characters to a screen.

There are two types of displays: ones that support cursor movement and ones that
do not. The concepts `cli::concepts::DisplayWithoutCursor` and
`cli::concepts::DisplayWithCursor` formalizes the interface.

```cpp
// a display D without cursor for a character type CharT
template<typename D, typename CharT>
concept cli::concepts::DisplayWithoutCursor;

// a display D with a cursor for a character type of CharT
template<typename D, typename CharT>
concept cli::concepts::DisplayWithCursor;

// either a display with cursor or without cursor for a character type CharT
template<typename D, typename CharT>
concept cli::concepts::Display;
```

A type `D` satisfies the `Display` concept if it either satisfies
`DisplayWithoutCursor` or `DisplayWithCursor`.

`CLI` provides a default implementation called [`cli::AnsiDisplay`](#ansidisplay).

### Display Without Cursor

For a variable `d` of type `D`, a variable `character` of type `CharT`, a
variable `string` of type `cli::View<const CharT>`, and a variable `n` of type
`std::size_t`, the following must hold for `D` to satisfy the
`DisplayWithoutCursor` concept. Note that all methods must return a
`cli::Error`.

- `d.write(character)`: writes a character.
- `d.write(string)`: writes a string.
- `d.backspace(n)`: deletes the last `n` characters.
- `d.clear_line()`: deletes all characters in the current line.
- `d.clear_screen()`: deletes all characters in the screen and moves the input
  back to the home position.
- `d.newline()`: writes a new line.

### Display With Cursor

Displays with a cursor support cursor movement. `DisplayWithCursor` is a
superset of `DisplayWithoutCursor`.

For a variable `d` of type `D`, a variable `character` of type `CharT`, a
variable `string` of type `cli::View<const CharT>`, and a variable `n` of type
`std::size_t`, the following must hold for `D` to satisfy the
`DisplayWithCursor` concept. Note that all methods must return a
`cli::Error`.

- `d.write(character)`: writes a character at the current cursor position. This
  will overwrite a character if it was already there.
- `d.write(string)`: writes a string at the current cursor position. This will
  overwrite any characters that were already displayed.
- `d.backspace(n)`: deletes the last `n` characters from the cursor position.
- `d.clear_line()`: deletes all characters in the current line.
- `d.clear_screen()`: deletes all characters in the screen and moves the input
  back to the home position.
- `d.newline()`: writes a new line.
- `d.delete_line_to_end()`: deletes all characters from the cursor position to
  the end.
- `d.delete_line_to_begin()`: deletes all characters from the cursor to the
  start of the line and moves the cursor to the start.
- `d.cursor_left(n)`: moves the cursor `n` positions to the left.
- `d.cursor_right(n)`: moves the cursor `n` positions to the right.

### AnsiDisplay

`cli::AnsiDisplay` is a class template that can be used to output characters to
a ANSI compliant display device, for example a terminal.

```cpp
template<cli::concepts::Output Out>
class AnsiDisplay;
```

`ÀnsiDisplay` uses an [output](#output) to actually write characters.

### Output

The output concept denotes a callable that can take a character of type
`CharT`, or a string of type `cli::View<const CharT>`, or both, as an input and
returns a `cli::Error`.

```cpp
// denotes an output for strings of type cli::View<const CharT>
template<class O, typename CharT>
concept cli::concepts::StringOutput;

// denotes an output that for characters of type CharT
template<class O, typename CharT>
concept cli::concepts::CharOutput;

// denotes an output for the character types char, signed char,
// unsigned char, char8_t, char16_t, and char32_t
template<typename O>
concept cli::concepts::Output;
```

#### Output Example

Here is an example of an [Output](#output) of char for an embedded UART.

```cpp
// satisfies cli::concepts::StringOutput<char>
cli::Error string_output(cli::View<const char> string){
  for(const char& ch: string){
    HAL_Status status = HAL_UART_Transmit(ch);
    if(status != HAL_STAUS_OK)
      return status_to_cli_err(status);
  }
  return cli::Error::none;
}

static_assert(cli::concepts::StringOutput<decltype(string_output), char>);

// satisfies cli::concepts::CharOutput<char>
cli::Error char_output(char c){
  HAL_Status status = HAL_UART_Transmit(c);
  return status_to_cli_err(status);
}

static_assert(cli::concepts::CharOutput<decltype(char_output), char>);

// satisfies cli::concepts::StringOutput<char> and
// cli::concepts::CharOutput<char> and
// cli::concepts::Output.
struct MyOutput{
  cli::Error operator()(cli::View<const char> string){
    for(const char& ch: string){
      HAL_Status status = HAL_UART_Transmit(ch);
      if(status != HAL_STAUS_OK)
        return status_to_cli_err(status);
    }
    return cli::Error::none;
  }

  cli::Error operator()(char c){
    HAL_Status status = HAL_UART_Transmit(c);
    return status_to_cli_err(status);
  }
};

static_assert(cli::concepts::StringOutput<MyOutput, char>);
static_assert(cli::concepts::CharOutput<MyOutput, char>);
static_assert(cli::concepts::Output<MyOutput>);

```

An [`cli::AnsiDisplay`](#ansidisplay) can then be constructed with the output.

```cpp
cli::AnsiDisplay display{&string_output};
cli::AnsiDisplay display{&char_output};
cli::AnsiDisplay display{MyOutput{}};
```

## Commands

### Parameters

Parameters are what they sound like. They are values the CLI makes available.

As an example: A parameter with the name "enable" with type bool can be set
with `enable = true`, and can be retrieved with `enable` (which will print
`true` or `false`)

To create a parameter, use the `cli::param` template overload set.

There are several different forms for creating parameters, described below.

#### Parameters Without Object Declarations

Paramters Without Object/Variable Declarations

Parameter commands without an object/variable declaration can be setup with the
following functions.

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
- set: a Setter for a T. It sets the value associated with the parameter. See
  also `cli::param::Getter` and `cli::param::GetterOf`.
- parse: a Parser for a T. It parses a T from a string. See also
  `cli::parse::Parser` and `cli::parse::ParserOf`.
- format: a Formatter for a T. It formats a T to a string. See also
  `cli::format::Formatter` and `cli::format::FormatterOf`.
- validate: a Validator for a T. It validates parsed values before they are
  set. See also `cli::validate::Validator` and `cli::validate::ValidatorOf`.
- subcommands: any amount of subcommands, either further parameters or
  functions.

There are a multitude of overloads so that certain parts can be left out, if
you wish to use the defaults provided by cli.

Note that:

- leaving out the setter creates a read-only parameter. A parser is then not
  necessary.
- leaving out the getter creates a write-only parameter. A formatter is then
  not necessary.

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

The following functions can be used to setup parameters with object/variable
declarations.

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
- set: a Setter for a T. It sets the value associated with the parameter. See
  also `cli::param::Getter` and `cli::param::GetterOf`.
- parse: a Parser for a T. It parses a T from a string. See also
  `cli::parse::Parser` and `cli::parse::ParserOf`.
- format: a Formatter for a T. It formats a T to a string. See also
  `cli::format::Formatter` and `cli::format::FormatterOf`.
- validate: a Validator for a T. It validates parsed values before they are
  set. See also `cli::validate::Validator` and `cli::validate::ValidatorOf`.
- subcommands: any amount of subcommands, either further parameters or
  functions.

There are a multitude of overloads so that certain parts can be left out, if
you wish to use the defaults provided by cli.

The parts that can be left out are:

- get: in that case, `cli::param::DefaultGet` is used.
- set: in that case, `cli::param::DefaultSe`t is used.
- validate: in that case, `cli::validate::DefaultValidate` is used.
- parse and format: in that case, `cli::parse::DefaultParse` and
  `cli::format::DefaultFormat` are used.

#### Parameters With const Object/Variable Declarations

Read-only parameters for const objects can be defined with the following
functions.

The base form is:

`cpp param(name, description, t, get, format)`

In total there are four overloads, where get, or format, or both, can be left
out. In that case, a default getter and/or default formatter are used.

#### Member Data Parameters

Member data commands are used to easily setup subcommands for parameters with
subobjects.

This is easiest explainable by example. Take this struct and its variabe
definition:

```cpp
struct Settings{
  int foo;
  char baz;
};

static Settings settings;
```

To make the settings and its members foo and baz available to cli, you can use
the following param call to easily setup this structure.

```cpp
param("settings"_sc,
      "core settings",
      settings,
      param("foo"_sc,
            "foo mode"_sc,
            &Settings::foo),
      param("baz"_sc,
            "baz setting"_sc,
            &Settings::baz));
```

Then `settings`, `settings.foo` and `settings.baz` can be used as parameter
commands like so:

```python
# setting the parameter
settings = {foo = 1, baz = k}
# retrieving the parameter
settings
# prints
{foo = 1, baz = 'k'}

# setting the parameter
settings.foo = 5
# retrieving the parameter
setting.foo
# prints
5

# setting the parameter
settings.baz = k
# retrieving the parameter
settings.baz
# prints
'k'
```

The full list of member data parameter functions is:

```cpp
param(name, description, ptr_to_member, parse, format, validate);
param(name, description, ptr_to_member, parse, format);
param(name, description, ptr_to_member, validate);
param(name, description, ptr_to_member);
```

In this case, parse, format, and validate are parsers, formatters and
validators for the type pointed to by ptr_to_member.

There are also overloads available for const member data.

```cpp
param(name, description, ptr_to_member, format);
param(name, description, ptr_to_member);
```

### Functions

Functions are commands that can be called. They take arguments and optionally
return a value. Functions can be called like so:

```bash
function(arg1, arg2)
function(name1 = arg1, name2 = arg2)
# parentheses are optional
function arg1, arg2
function name1 = arg1, name2 = arg2
```

#### Arguments

Arguments are the elements that describe c++ function arguments. These argument
specifications are then used by Functions to parse the input into the specified
values and validate them.

There are two kinds of function arguments:

- required: these parameters must be specified, else it is an error.
- optional: these parameters can be left out because they have a default value.

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
template <
class T, // the arguments type, must be explicitly specified
auto DefaultValue, // the default value, must be explicitly specified
SC Name, // must be specified
SC Description, // either specified or left out
parse::Parser Parse, // either specified or deduced validate::Validator
Validate // either specified or deduced
>
constexpr auto arg(
  Name name, // the name
  Description description, // the description
  Parse&& parse, // the parser
  Validate&& validate // the validator
);
```

The parser and validator can be left out.

##### Required Arguments

The base form for a required arguments is:

```cpp
template <
  class T, // the arguments type, must be explicitly specified
  SC Name, //must be specified SC Description, // either specified or left out
  parse::Parser Parse, // either specified or deduced
  validate::Validator Validate // either specified or deduced
>
constexpr auto arg(
  Name name, // the name
  Description description, // the description
  Parse&& parse, // the parser
  Validate&& validate // the validator
);
```

The parser and validator can be left out.

##### Deduced Arguments

Deduced arguments are arguments that have their type deduced. The default
parser and validator are always used for these type of arguments. The benefit
of deduced arguments is the shorter notation.

There are two functions for creating deduced arguments:

```cpp
arg(name, description)
arg(name)
```

and the literal operator `_arg`

```cpp
"name"_arg
```

## String Constant

`cli::string_constant` is a compile time string used by `CLI`.

```cpp
template<typename CharT, CharT...Chars>
struct string_constant;
```

The concept `cli::SC` checks if a type is a string constant.

To create string constants, use the literal operator `_sc`.

```cpp
#include <cli/string.hpp>

using cli::operator""_sc;

auto s = "hello"_sc;    // CharT is char
auto s8 = u8"hello"_sc; // CharT is char8_t
auto s16 = u"hello"_sc; // CharT is char16_t
auto s32 = U"hello"_sc; // CharT is char32_t
```

## Formatting

A Formatter takes a `cli::View<CharT>` as its first argument and a `T` as its
second argument, and returns a `cli::format::FormatResult`. The first argument
specifies the buffer into which the second argument should be formatted into.
The concepts `cli::format::FormatterOf` and `cli::format::Formatter` formalize
the interface. All formatting utilities can be found in the header
`cli/format.hpp`.

An example of formatters:

```cpp
auto format_char(cli::View<char> output, char c)
  -> cli::format::FormatResult{
  if(output.size() == 0)
    return cli::Error::buffer_overflow;
  output[0] = c;
  return 1;
}

auto format_quoted_char(cli::View<char> output, char c)
  -> cli::format::FormatResult {
  if(output.size() < 3)
    return cli::Error::buffer_overflow;
  output[0] = '\'';
  output[1] = c;
  output[2] = '\'';
  return 3;
}

auto format_bool(cli::View<char> output, bool b)
  -> cli::format::FormatResult {
  if(b){
    if(output.size() < 4)
      return cli::Error::buffer_overflow;
    output[0] = 't';
    output[1] = 'r';
    output[2] = 'u';
    output[3] = 'e';
    return 4;
  }else{
    if(output.size() < 5)
      return cli::Error::buffer_overflow;
    output[0] = 'f';
    output[1] = 'a';
    output[2] = 'l';
    output[3] = 's';
    output[4] = 'e';
    return 5;
  }
}
```

Default implementations of formatters can be found in the `cli::format`
namespace. There are default formatters for:

- bool
- characters
- enumerations
- integers
- fixpoint numbers
- sequences, i.e. arrays/lists/vectors
- strings
- aggregates, i.e. simple structs
- TODO: floating point numbers

To see how to enable formatting for your custom type, see [Parsing](#parsing).
Enumerations, sequences, strings, and aggregates/structs need to opt in the
same way as described there, i.e. by specializing the corresponding traits
structure.

### Custom Formatting

To enable your type to be formattable when it is a member of a struct, then
you must specialize `cli::format::Format` and implement its call
operator.

```cpp

class MyClass{
// ..
};

namespace cli::format{
  template<typename CharT>
  struct Format{
    FormatResult operator()(View<CharT> output, const MyClass& c) const;
  };
}
```

## Parsing

Parsing in CLI is based on two things:

- the class `cli::parse::ParseResult`: the result of a parsing operation.
- `cli::parse::Parser`: the parser concept.

A parser of `T` is a callable that parses a `T` from a string and returns a
`ParseResult`. It takes a `cli::View<const CharT>` as its first and only
argument and returns a `cli::parse::ParseResult<T, CharT>`.

All parsing utilities can be found in the header `cli/parse.hpp`.

Example:

```cpp
#include <cli/parse.hpp>

cli::parse::ParseResult<bool, char>
parse_bool(vli::View<const char> buf){
  if(buf.starts_with("true")){
    return {true, buf.substr(4)};
  }else if(buf.starts_with("false")){
    return {false, buf.substr(5)};
  }else{
    return cli::Error::invalid_character;
  }
}

static_assert(cli::parse::ParserOf<decltype(parse_bool), bool, char>);
```

CLI provides defaults for parsing for the following types:

- bool
- characters
- enumerations
- integers
- fixpoint numbers
- sequences, i.e. arrays/lists/vectors
- strings
- aggregates, i.e. simple structs

### Enum Parsing

enum parsing is available for enum classes by default. These must be an enum
class and not weak C style enumerations, because casting a weak enum outside
its value range is undefined behaviour. If you wish to use weak enums, you must
specialize `cli::traits::enum:traits`.

If your enum is signed and only has values in the range [-128, 127] or your
enum is unsigned and has values in the range of [0, 255], then you don't have
to write anything to get enum class parsing suport. If that is not the case, or
your enum is a weak enum, you will have to write your own enum traits. Do this
by defining `cli::traits::enum_traits` and adjusting the parameters.

```cpp
 #include "cli/traits.hpp"

 namespace your_namespace{
   enum class YourEnum : std::unit32_t{
       A = 5,
       ...
       ABCD = 300
   };
 }

 namespace cli::traits{
   template<>
   struct enum_traits<your_namespace::YourEnum>{
     // the minimum value
     static constexpr unit32_t min = 5;
     // the maximum value
     static constexpr uint32_t max = 300;
     // set to false if your enum is not a flag enum, else true
     static constexpr bool is_flag = false;
   };
 }
```

Enumerations don't include the enum class name when formatted and parsed by
CLI. In the exampe above, the string "A" would be parsed as `YourEnum::A`
and "ABCD" would be parsed as `YourEnum::ABCD`.

### Sequence Parsing

To tell CLI that your type is a sequence, you must specialize
`cli::traits::is_sequence` or `cli::traits::is_fixed_size_sequence`.
If your type then also conforms to the (fixed size) sequence interface, your
type will be parseable by CLI.

A fixed sequence is a list of fixed size, for example arrrays.
A non-fixed sequence is a list of variable size, for example
`cli::FixedCapacityVector`.

Sequences have the following format:

`[e1, e2, e3, ..., en]`

where `ex` are the elements of the sequence, delimited by commas.

See the concepts `cli::traits::Sequence` and `cli::traits::FixedSizeSequence` for
the requirements on the interface.

Example for specializing the traits structure:

```cpp
#include "cli/traits.hpp"
namespace A{
 class Vec{
 public:
   using value_type = T;
   Vec();
   Vec(const Vec&);
   iterator begin();
   iterator end();
   std::size_t size() const;
   std::size_t max_size() const;
   void push_back(const T&);
 };

 class Array{
 public:
   using value_type = T;
   Array();
   Array(const Array&);
   iterator begin();
   iterator end();
   std::size_t size() const;
   T& operator[](std::size_t i);
 };
}

namespace cli::traits{
 template<>
 struct is_sequence<A::Vec> : std::true_type{};

 template<>
 struct is_fixed_size_sequence<A::Array> : std::true_type{};
}

static_assert(cli::traits::Sequence<A::Vec>);
static_assert(cli::traits::FixedSizeSequence<A::Array>);
```

### String Parsing

To enable parsing for your custom string type, specialize
`cli::traits::is_string` and conform to the interface detailed by the concept
`cli::traits::String`.

Strings can either be unescaped, in which case they can't contain spaces, or
escaped with the \" character. Inner \" characters can be escaped by backslash.

Examples of valid strings:

```
hello
"hello world"
"hello \"world\""
hello"world
```

Example:

```cpp
 #include "cli/traits.hpp"
 class MyString{
   using value_type = char;
   MyString(const value_type* s, std::size_t n);
 };

 namespace cli::traits{
   template<>
   struct is_string<MyString> : std::true_type{};
 }
```

There is also parsing for string views, i.e. non owning strings, available.
Keep in mind however that escaped quotes will remain as they are and not
converted due to views not owning any memory. Additionally, the referenced
memory area may change contents when a new command is entered. For string
views, `cli::traits::is_string_view` must be specialized.

### Fixpoint Parsing

TBD

### Struct Parsing

Simple aggregates can be decomposed and parsed by CLI as long as each member
is parseable.

The format for aggregates is

`{name1 = v1, name2 = v2, v3, name4 = v4}`

namex are he member names, vx are the member values. Keyword elements
(namex = vx) and value elements (vx) can be mixed in any way if the value
elements appear in order.

Concrete Example:

```cpp
#include <cli/parse.hpp>

struct S{
  int i;
  char c;
  cli::View<const char> str;
};

cli::parse::Parse<S,char> parse{};

cli::parse::ParseResult p = parse("{1,k,\"hello world\"}")
assert(p);
assert(p.value == S{1, 'k', "hello world"});

p = parse("{c=k, 1, str = \"hello world\"}")
assert(p);
assert(p.value == S{1, 'k', "hello world"});

p = parse("{str = \"hello world\", 1, k}")
assert(p);
assert(p.value == S{1, 'k', "hello world"});
```

### Custom Parsing

Custom parsers just have to conform to the Parser concept.
To enable CLI to select your implementation by default, which is needed
if you want your types parseable when they are members of a struct, then
explicitly specialize `cli::parse::Parse` and implement it's
call operator.

```cpp
#include <cli/parse.hpp>

class MyClass{
// ...
};

namespace cli::parse{

template<typename CharT>
struct Parse<MyClass, CharT>{
  ParseResult<MyClass, CharT> operator()(cli::View<const CharT> buf) const;
};

}

```
