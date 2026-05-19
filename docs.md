# CLI Library Documentation

`CLI` consists of the following main components:

- [cli::Engine](#engine): the interface of `CLI`
- [cli::concepts::Config](#config): configuration for the `cli::Engine`
- [cli::concepts::Input](#input) and [cli::Input](#input-class-template):
  used to receive characters
- [cli::concepts::Display](#display) and
  [cli::AnsiDisplay](#ansidisplay): used to display characters
- [cli::concepts::Command](#commands): the commands, which are either
  [Parameters](#parameters) or [Functions](#functions)
- [cli::string_constant](#string-constant): a compile time string
- [Parsing](#parsing)
- [Formatting](#formatting)
- [Validation](#validation)
- [Simulation](#simulation)

## Contents

- [Engine](#engine)
  - [Declaration](#engine-declaration)
  - [Typedefs](#engine-typedefs)
  - [Constructor](#engine-constructor)
  - [Methods](#engine-methods)
  - [Example](#engine-example)
- [Config](#config)
  - [Optional Entries](#optional-entries)
  - [Example](#config-example)
- [Input](#input)
  - [Input Class Template](#input-class-template)
  - [SimpleInput Class Template](#simpleinput-class-template)
  - [Example](#input-example)
- [Display](#display)
  - [Displays Without Cursor](#display-without-cursor)
    - [Example Of A Display Without Cursor](#example-of-a-display-with-cursor)
  - [Displays With Cursor](#display-with-cursor)
    - [Example Of A Display With Cursor](#example-of-a-display-with-cursor)
  - [AnsiDisplay](#ansidisplay)
    - [AnsiDisplay Constructor](#ansidisplay-constructor)
      - [AnsiDisplay(output)](#ansidisplayout-output)
      - [AnsiDisplay(args...)](#ansidisplayargsargs)
    - [Output](#output)
    - [Output Example](#output-example)
- [Commands](#commands)
  - [Parameters](#parameters)
    - [Virtual Parameters](#virtual-parameters)
    - [Parameters Without Object Declarations](#parameters-without-object-declarations)
    - [Parameters With Object/Variable Declarations](#parameters-with-objectvariable-declarations)
    - [Parameters With Const Object/Variable Declarations](#parameters-with-const-objectvariable-declarations)
    - [Member Data Parameters](#member-data-parameters)
    - [Recursive Parameters](#recursive-parameters)
    - [Getters](#getters)
    - [Setters](#setters)
  - [Functions](#functions)
    - [Free Functions And Callables](#free-functions-and-callables)
    - [Member Functions](#member-functions)
    - [Arguments](#arguments)
      - [Optional Arguments](#optional-arguments)
      - [Required Arguments](#required-arguments)
      - [Deduced Arguments](#deduced-arguments)
- [Help Command](#help-command)
- [String Constant](#string-constant)
- [Formatting](#formatting)
  - [Custom Formatting](#custom-formatting)
- [Parsing](#parsing)
  - [Custom Parsing](#custom-parsing)
- [Traits](#traits)
  - [enum_traits](#enum_traits)
- [Concepts](#concepts)
  - [Sequence](#sequence)
  - [FixedSizeSequence](#fixedsizesequence)
  - [String](#string)
  - [StringVIew](#stringview)
  - [Fixpoint](#fixpoint)
  - [Struct](#struct)
- [Validation](#validation)
- [Simulation](#simulation)
- [cli-term](#cli-term)
  - [Usage](#cli-term-usage)
  - [Mappings](#mappings)
  - [Compiling](#compiling-cli-term)

## Engine

The Engine class is composed of a [config](#config), an [input](#input), a
[display](#display), and [commands](#commands).

In your main application, you will create an Engine object, and populate it
with the parameters and functions, and calls the engine's `on_char()` and
`process()` methods.

`on_char` must either be called in your character reception ISR (see [input
concept](#input) for details), or in the main thread before process. It
preprocesses the character input and puts the data into an internal event
buffer.

`process()` is the method takes the data from the event buffer and actually
processes the input.

### Engine Declaration

```cpp
template<
  concepts::Config Configuration,
  concepts::Display Display,
  concepts::Command... Commands>
class Engine;
```

### Engine Typedefs

`Engine` has three inner typedefs:

- **config_type**: the [configuration](#config) of the engine.
- **char_type**: the character type of the [configuration](#config).
- **input_type**: the [input](#input) the engine uses.

### Engine Constructor

There is one constructor for `Engine`:

#### `Engine(config, display, commands...)`

To create an `Engine`, provide a [config](#config), a [display](#display), and
[commands](#commands).

- **config**: this is the [Engine configuration](#config)
- **display**: this is the [display](#display) the engine will use to output
  characters.
- **commands**: these are the [commands](#commands) of the engine. At least one
  command must be provided.

### Engine Methods

#### `cli::Error on_char(char_type c)`

This method must be called when a character is received. No heavy processing is
performed because the `Engine` simply forwards `c` to its [input](#input).

If the default [input](#input-class-template) is used and the [config](#config)
specifies the use of a volatile input buffer, then this method can be called in
an ISR.

The return value will be either `cli:Error::none` in case of success, or
`cli::Error::buffer_overflow` in case the [input](#input) cannot accept more
characters.

Example:

```cpp
cli::Engine engine{...};
engine.on_char('k');
```

#### `cli::Error on_control(cli::Control ctrl, std::uint8_t param)`

This method can be called, instead of `on_char` to notify the engine of a
control input. No heavy processing is performed because the `Engine` simply
forwards `c` to its [input](#input).

The return value will be either `cli:Error::none` in case of success, or
`cli::Error::buffer_overflow` in case the [input](#input) cannot accept more
characters.

Example:

```cpp
cli::Engine engine{...};
engine.on_control(cli::Control::cursor_left, 5);
```

#### `cli::Error process()`

Processes the available events. Returns early if an error occurred during
processing.

#### `void reset()`

Resets the engine. This will reset the input, the display, and any internal state.

#### `void print()`

Prints the command tree.

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
  cli::default_config{}, // the config
  cli::AnsiDisplay{&send_char}, // the display
  // the commands
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

The [Engine](#engine) uses a configuration. A configuration is a traits-like
structure that sets core aspects of the engine. There is a concept called
`cli::concepts::Config` that formalizes the configuration.

A configuration of type `C` must have the following static constexpr members
and typedefs to satisfy the `Config` concept:

- **name**: convertible to `cli::View<const char_type>`. The name of the cli as
  a string. Must be an [Id](#id).
- **description**: convertible to `cli::View<const char_type>`. The description
  of the cli.
- **max_line_length**: of type `std::size_t`. Specifies how long the maximum
  command input is. Commands longer than this length cannot be processed.

### Optional Entries

Optional entries of a configuration are static constexpr members/typedefs that
can be left out because `CLI` uses a default value if they are not specified.
Entries with name `X` of a configuration type `C` can be retrieved with
`cli::config::X_v<C>` (for members) and `cli::config::X_t<C>` (for typedefs).

Note that the size increases given are for a 32-bit Cortex-M processor,
compiled with GCC, optimized for size and NDEBUG defined.

- **char_type**: a **typedef** for the character type to use. Can be any of
  _char_, _signed char_, _unsigned char_, _char8_t_, _char16_t_, _char32_t_.
  Default is _char_.
- **access_separator**: of type `char_type`. This specifies what kind of
  character is used to delimit individual sub commands. Default is `.`.
- **use_autocomplete**: of type `bool`. If true, the engine uses autocomplete.
  Default is `false`.
- **use_cursor**: of type `bool`. If true, the engine recognizes cursor movement.
  If true, your [display](#display) must support cursor movement.
  Default is `false`.
- **use_history**: of type `bool`. Specifies if the engine implements a command
  history. Default is `false`.
- **history_depth**: of type `std::size_t`. Specifies how many commands can be
  stored in the command history. Default is 16.
- **input_type**: either a typedef or class template. If it is a typedef, it
  must satisfy the [input concept](#input). If it is a template, then the
  template must take one template parameter, which is the Config itself. In that
  case, Config::input_type\<Config\> must satisfy the [input concept](#input).
  Defaults to [cli::Input](#input). You must leave this entry out if you want to
  use the default.
- **input_delimiter**: of type `cli::Delimiter`. Specifies the character
  sequence for the enter key. The default is `cli::Delimiter::lf`.
- **input_size**: of type `std::size_t`. Specifies how many elements the
  internal Event buffer of `cli::Input` stores. The default is 16.
- **use_volatile_input_buffer**: of type `bool`. Specifies the use of a volatile
  buffer for the input. Must be set to true if the engine's `on_char` method is
  called in an ISR. Custom input types should respect this value. Defaults to
  false.
- **output_size**: of type `std::size_t`. Specifies how big the buffer for
  outputting characters is. The default is `max_line_length`.
- **use_help**: of type `bool`. If true, a [help command](#help-command) is
  available. Default is false. This will add 2Kb to the executable's size.
- **empty_help_prints_commands**: of type `bool`. If true, then the help
  command will print all commands when it is given no argument. Else it will
  print "no such command". Defaults to false. This will add roughly 200B to the
  executable's size.
- **use_detailed_error_messages**: of type `bool`. If true, then `CLI` will
  print a detailed description when an invalid command is entered. Else it will
  just print display "error". Defaults to false. Enabling this will add 1kB.

### Config Example

```cpp
#include <cli.hpp>

struct my_config{
  using char_type = char;
  static constexpr cli::View<const char> name = "cli";
  static constexpr cli::View<const char> description = "a command line interface";
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
  // for the input type:
  // as a typedef
  struct input_type {...};
  // or as a template:
  template<concepts::Config Cfg>
  struct input_type {...};

};

static_assert(cli::concepts::Config<my_config>);
```

## Input

The input concept formalizes the interface that the [Engine](#engine) uses
for preprocessing character input.

An Input preprocesses the character and control input received with `on_char` and
`on_control` into a sequence of `cli::Event`. This sequence is then accessed in
a FIFO order via `pop_event` by the [engine](#engine). This way, `on_char` and
`on_control` could be called in an interrupt service routine because
preprocessing the input is not a very expensive operation.

**Note**: f you want to call the engine's `on_char` and `on_control` methods in
an ISR, you must add a static constexpr member called
`use_volatile_input_buffer` of type `bool` to your [Config](#config) and set it
to true. If you want your own input type to respect that setting, you can use
`cli::config::use_volatile_input_buffer_v` to query this configuration value.

`CLI` provides a default implementation called [cli::Input](#input-class-template).

All input classes provided by `CLI` are defined in the header `cli/input.hpp`.

### Input Concept Definition

```cpp
template<typename I, typename CharT>
concept cli::concepts::Input =
  std::is_constructible_v<I> and
  requires(I input,
           CharT character,
           cli::Event<CharT> &event,
           cli::Control ctrl,
           std::uint8_t param)
  {
    /// on_char is called by the engine's on_char method. It processes the
    /// character, transforms it into a cli::Event<CharT>, and stores it in
    /// an internal buffer. Returns cli::Error::none on success.
    { input.on_char(character) } -> std::same_as<cli::Error>;

    /// on_control is called by the engine's on_control method. It adds a
    /// cli::Event<CharT>, constructed from the control and param, to its
    /// internal buffer. Returns cli::Error::none on success.
    { input.on_control(ctrl, param) } -> std::same_as<cli::Error>;

    /// pop_event is called by the engine to process an event.
    { input.pop_event(event) } -> std::convertible_to<bool>;

    /// reset resets the input to its initial state.
    { input.reset() } -> std::same_as<void>;
  };
```

### Input Class Template

The default implementation of the [Input Concept](#input).
It is useful when `CLI` is connected to an ANSI capable terminal, for example
when using [cli-term](#cli-term).

**Definition**:

```cpp
template<cli::concepts::Config Cfg>
class cli::Input{
public:
  using char_type = cli::config::char_type_t<char_type>;
  using event_type = cli::Event<char_type>;

  constexpr cli::Error on_char(char_type c);
  constexpr cli::Error on_control(Control ctrl, std::uint8_t param=1);
  constexpr bool pop_event(event_type& event);
  constexpr void reset();
}
```

`cli::Input` recognizes the following ANSI escape sequences:

In the following paragraph, _CSI_ stands for [Control Sequence
Introducer](https://en.wikipedia.org/wiki/ANSI_escape_code#Control_Sequence_Introducer_commands),
which is the character sequence `0x1B 0x5B`, also commonly written as
`ESC[`.

These special characters and escape sequences are recognized by `cli::Input`:

- **BEL** (0x07) -> Control::bell
- **backspace** (0x08, \\b) -> Control::backspace. This means deleting the
  character before the cursor (or the last character in case
  Cfg::use_cursor is false).
- **tab** (0x09, \\t) -> passed through as is if autocomplete is not
  enabled, else Control:autocomplete.
- **linefeed** (0x0A, \\n) -> Control::enter if Cfg::input_delimiter is lf, else
  passed through as is.
- **carriage return** (0x0D, \\r) -> Control::enter if Cfg::input_delimiter is
  cr, else passed through as is.
- **carriage return + linefeed** ([0x0A, 0x0B], \\r\\n) -> Control::enter
  if Cfg::input_delimiter is crlf.
- **delete** (0x7F) -> Control::delete_char. Deletes the character under
  the cursor. If cursor is not enabled, this has no effect.
- **CSI n A** -> Control::cursor_up. Cursor up movement. If history is
  enabled, this translates to going back in history. n is optional.
- **CSI n B** -> Control::cursor_down. Cursor down movement. If history is
  enabled, this translates to moving forward in history. n is optional.
- **CSI n C** -> Control::cursor_right. Cursor right movement. If cursor is
  enabled, this translates to moving the cursor to the right. If the cursor
  is at the end of the current input, nothing happens. n is optional.
- **CSI n D** -> Control::cursor_left. Cursor left movement. If cursor is
  enabled, this translates to moving the cursor to the left. If the cursor
  is at the start of the current input, nothing happens. n is optional.
- **CSI 0 K** -> Control::clear_line_to_end: clears the line from the
  cursor to the end. The cursor position will not change. If the cursor is
  not enabled, this has no effect.
- **CSI 1 K** -> Control::clear_line_to_begin. Clears the line from the cursor to
  the beginning. The cursor and rest of the line content moves to the beginning
  of the input. If the cursor is not enabled, this has no effect.
- **CSI 2 K** -> Control::clear_line. Clears the entire line. The cursor
  moves to the beginning of the line. If the cursor is not enabled, this
  has no effect.
- **CSI 2 J** -> Control::clear_screen. Clears the entire screen. The
  cursor moves to the top starting position. If the cursor is not enabled,
  this has no effect.

**NOTE**: Certain escape sequences effects differ from the ANSI standard because
CLI is intended to be used as a single line interface, not a fully featured
ANSI terminal. This affects the sequences `CSI n K` and `CSI 2 J`. However, if
your display/output is connected to a fully ANSI compliant device, then you can
use [cli::AnsiOutput](#ansidisplay), which sends the needed cursor move
sequences to be ANSI compliant.

### SimpleInput Class Template

The `SimpleInput` class template is an input that does not recognize ANSI
escape sequences. It is can be used when the character input does not come from
an ANSI capable source, for example when the system running `CLI` has a custom
keyboard/keypad.

**Definition**:

```cpp
template<cli::concepts::Config Cfg>
class cli::SimpleInput{
public:
  using char_type = cli::config::char_type_t<char_type>;
  using event_type = cli::Event<char_type>;

  constexpr cli::Error on_char(char_type c);
  constexpr cli::Error on_control(Control ctrl, std::uint8_t param=1);
  constexpr bool pop_event(event_type& event);
  constexpr void reset();
}
```

### Input Example

Below is an example definition for an [Input](#input) for the character type `char`.

```cpp
#include <cli/event.hpp>

class MyInput{
public:
  constexpr cli::Error on_char(char c);
  constexpr bool pop_event(cli::Event<char>& event);
  constexpr void reset();
};

static_assert(cli::concepts::Input<MyInput, char>);
```

## Display

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
[DisplayWithoutCursor](#display-without-cursor) or
[DisplayWithCursor](#display-with-cursor).

Displays can be further categorized in single-line and multi-line displays.
Single-line displays only have a single line to display content. Multi-line
displays have multiple lines.

For single-line displays, a newline will only be printed when a new command is
entered, or a command results in an output. Commands result in an output when
a function command that doesn't return void is executed or when a parameter is
retrieved.

For multi-line displays, a newline will always be printed when a command is executed.

By default, displays are single-line. To specify that your display is
multi-line, the display must have a static constexpr member called
`is_multiline` of type `bool` that is set to true.

For multi-line displays, a static constexpr member of type `std::size_t` called
`number_of_lines` should be added to it to specify how many lines the display
has. It is used in conjunction with the [help command](#help-command) and the
engine's [print method](#void-print).

`CLI` provides a default implementation called [cli::AnsiDisplay](#ansidisplay).

### Display Without Cursor

For a variable `d` of type `D`, a variable `character` of type `CharT`, a
variable `string` of type `cli::View<const CharT>`, and a variable `n` of type
`std::size_t`, the following must hold for `D` to satisfy the
`DisplayWithoutCursor` concept. Note that all methods must return void.

- `d.write(character)`: writes a character.
- `d.write(string)`: writes a string.
- `d.backspace(n)`: deletes the last `n` characters.
- `d.clear_line()`: deletes all characters in the current line.
- `d.clear_screen()`: deletes all characters in the screen and moves the input
  back to the home position.
- `d.newline()`: writes a new line.

#### Example Of A Display Without Cursor

Below is an example definition for a char display without cursor.

```cpp
class MySingleLineDisplay{
public:
  MySingleLineDisplay();
  void write(char character);
  void write(cli::View<const char> string);
  void backspace(std::size_t n);
  void clear_line();
  void clear_screen();
  void newline();
};

class MyMutlilineDisplay{
public:
  static constexpr bool is_multiline = true;
  MyMutlilineDisplay();
  void write(char character);
  void write(cli::View<const char> string);
  void backspace(std::size_t n);
  void clear_line();
  void clear_screen();
  void newline();
};
```

### Display With Cursor

Displays with a cursor support cursor movement. `DisplayWithCursor` is a
superset of `DisplayWithoutCursor`.

For a variable `d` of type `D`, a variable `character` of type `CharT`, a
variable `string` of type `cli::View<const CharT>`, and a variable `n` of type
`std::size_t`, the following must hold for `D` to satisfy the
`DisplayWithCursor` concept. Note that all methods must return void.

- `d.write(character)`: writes a character at the current cursor position. This
  will overwrite a character if it was already there.
- `d.write(string)`: writes a string at the current cursor position. This will
  overwrite any characters that were already displayed.
- `d.backspace(n)`: deletes the last `n` characters. This method is only called
  when the cursor is at the end of the line.
- `d.clear_line()`: deletes all characters in the current line.
- `d.clear_screen()`: deletes all characters in the screen and moves the input
  back to the home position.
- `d.newline()`: writes a new line.
- `d.cursor_left(n)`: moves the cursor `n` positions to the left.
- `d.cursor_right(n)`: moves the cursor `n` positions to the right.

#### Example Of A Display With Cursor

Below is an example definition for a char display with cursor.

```cpp
class MySingleLineDisplay{
public:
  MySingleLineDisplay();
  void write(char character);
  void write(cli::View<const char> string);
  void backspace(std::size_t n);
  void clear_line();
  void clear_screen();
  void newline();
  void cursor_left(std::size_t n);
  void cursor_right(std::size_t n);
};

class MyMutlilineDisplay{
public:
  static constexpr bool is_multiline = true;
  MyMutlilineDisplay();
  void write(char character);
  void write(cli::View<const char> string);
  void backspace(std::size_t n);
  void clear_line();
  void clear_screen();
  void newline();
  void cursor_left(std::size_t n);
  void cursor_right(std::size_t n);
};
```

### AnsiDisplay

`cli::AnsiDisplay` is a class template that can be used to output characters to
a ANSI compliant display device, for example a terminal.

```cpp
template<cli::concepts::Output Out, std::size_t NumLines = cli::unlimited_lines>
class AnsiDisplay;
```

`AnsiDisplay` uses an [Output](#output) to actually write characters.

#### AnsiDisplay Constructor

There are three constructors for `AnsiDisplay`:

##### AnsiDisplay(Out output)

Constructs an AnsiDisplay from an [output](#output).

Example:

```cpp
void my_output(char c);

cli::AnsiDisplay display{&my_output};
```

##### AnsiDisplay(Out output, constant\<NumLines\>)

Constructs an AnsiDisplay from an [output](#output) and the number of lines.

Example:

```cpp
cli::AnsiDisplay display{output, cli::constant<10>{}};
```

##### AnsiDisplay(Args...args)

Constructs an AnsiDisplay by forwarding `args` to its [output](#output).

Example:

```cpp
class MyOutput{
public:
  MyOutput(int handle, void* bla);
  void operator()(char c);
};

int handle = 5;
void* bla = ...;

cli::AnsiDisplay<MyOutput> display{handle, bla};
```

#### Output

The Output concept denotes a callable that can take a character of type
`CharT`, or a string of type `cli::View<const CharT>`, or both, as an input and
returns `void`.

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
void string_output(cli::View<const char> string){
  for(const char& ch: string){
    HAL_UART_Transmit(ch);
  }
}

static_assert(cli::concepts::StringOutput<decltype(string_output), char>);

// satisfies cli::concepts::CharOutput<char>
void char_output(char c){
  HAL_UART_Transmit(c);
}

static_assert(cli::concepts::CharOutput<decltype(char_output), char>);

// satisfies cli::concepts::StringOutput<char> and
// cli::concepts::CharOutput<char> and
// cli::concepts::Output.
struct MyOutput{
  void operator()(cli::View<const char> string){
    for(const char& ch: string){
      HAL_UART_Transmit(ch);
    }
  }

  void operator()(char c){
    HAL_UART_Transmit(c);
  }
};

static_assert(cli::concepts::StringOutput<MyOutput, char>);
static_assert(cli::concepts::CharOutput<MyOutput, char>);
static_assert(cli::concepts::Output<MyOutput>);

```

A [cli::AnsiDisplay](#ansidisplay) can then be constructed with the output.

```cpp
cli::AnsiDisplay display{&string_output};
cli::AnsiDisplay display{&char_output};
cli::AnsiDisplay display{MyOutput{}};
```

## Commands

Commands are the functionality to execute on the cli.

There are two types of commands:

- [Parameters](#parameters)
- [Functions](#functions)

Parameters are values that can be set and retrieved, functions are
functionality that can be executed.

### Parameters

Parameters are commands that represent a value.

They can be set with:

```bash
parameter = value
```

and read with:

```bash
parameter
```

To create a parameter, use the `cli::param` template overload set.

There are four parameter categories:

1. [Parameters without object/variable
   declarations](#parameters-without-object-declarations). These kinds of
   parameters don't store their value in a variable, as far as CLI is
   concerned. They give complete control regarding read and write access and is
   the most flexible. The drawback is that using these requires more boilerplate.
2. [Parameters with object/variable
   declarations](#parameters-with-objectvariable-declarations). These
   parameters store their value in a variable. A reference to this variable is
   passed to `cli::param`.
3. [Member data parameters](#member-data-parameters). These are subcommands of
   a parent parameter.
4. [Virtual parameters](#virtual-parameters). These can't be read or written
   to, but act as a grouping for sub commands.
5. [Recursive Parameters](#recursive-parameters). These parameters enable low
   boilerplate for nested structures.

The second and third category reduce the boilerplate required of the first
category and provide sensible defaults.

#### Virtual Parameters

Virtual parameters don't have a value associated with them and can only be used
to group subcommands.

To create a virtual parameters, use one of the following overloads:

```cpp
param(name, description, subcommands...);
param(name, subcommands...);
```

where

- name is a `cli::string_constant` that makes up the command name.
- description is a `cli::string_constant` that describes the command.
- subcommands are subcommands of the parameter, either
  [parameters](#parameters) or [functions](#functions). You must have at least
  one subcommand.

#### Parameters Without Object Declarations

Parameters Without Object/Variable Declarations

Parameter commands without an object/variable declaration can be setup with the
following functions.

The basic form is:

```cpp
param<T>(name, description, get, set, parse, format, validate, subcommands...);
```

The parts have the following functions:

- **T**: the parameter's type
- **name**: a `cli::string_constant` that makes up the command name. Must be an
  [Id](#id).
- **description**: a `cli::string_constant` that describes the command.
- **get**: a [Getter](#getters) for a T. It retrieves the value associated with
  the parameter. See also `cli::param::Setter` and `cli::param::SetterOf`.
- **set**: a [Setter](#setters) for a T. It sets the value associated with the
  parameter. See also `cli::param::Getter` and `cli::param::GetterOf`.
- **parse**: a [Parser](#parsing) for a T. It parses a T from a string. See
  also `cli::parse::Parser` and `cli::parse::ParserOf`.
- **format**: a [Formatter](#formatting) for a T. It formats a T to a string.
  See also `cli::format::Formatter` and `cli::format::FormatterOf`.
- **validate**: a [Validator](#validation) for a T. It validates parsed values
  before they are set. See also `cli::validate::Validator` and
  `cli::validate::ValidatorOf`.
- **subcommands**: any amount of subcommands, either further parameters or
  functions.

There are a multitude of overloads so that certain parts can be left out, if
you wish to use the defaults provided by cli.

**Note**:

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

- **name**: a `cli::string_constant` that makes up the command name. Must be an
  [Id](#id).
- **description**: a `cli::string_constant` that describes the command.
- **t**: the variable/object of type T that holds the value of the parameter.
- **get**: a [Getter](#getters) for a T. It retrieves the value associated with
  the parameter. See also `cli::param::Setter` and `cli::param::SetterOf`.
- **set**: a [Setter](#setters) for a T. It sets the value associated with the
  parameter. See also `cli::param::Getter` and `cli::param::GetterOf`.
- **parse**: a [Parser](#parsing) for a T. It parses a T from a string. See
  also `cli::parse::Parser` and `cli::parse::ParserOf`.
- **format**: a [Formatter](#formatting) for a T. It formats a T to a string.
  See also `cli::format::Formatter` and `cli::format::FormatterOf`.
- **validate**: a [Validator](#validation) for a T. It validates parsed values
  before they are set. See also `cli::validate::Validator` and
  `cli::validate::ValidatorOf`.
- **subcommands**: any amount of subcommands, either further parameters or
  functions.

There are a multitude of overloads so that certain parts can be left out, if
you wish to use the defaults provided by cli.

The parts that can be left out are:

- get: in that case, `cli::param::DefaultGet` is used.
- set: in that case, `cli::param::DefaultSet` is used.
- validate: in that case, `cli::validate::DefaultValidate` is used.
- parse and format: in that case, `cli::parse::DefaultParse` and
  `cli::format::DefaultFormat` are used.

#### Parameters With const Object/Variable Declarations

Read-only parameters for const objects can be defined with the following
functions.

The base form is:

```cpp
param(name, description, t, get, format)
```

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

#### Recursive Parameters

Recursive parameters are created by any of the following param overloads:

```cpp
 cli::param(name, description, t, set_callback, validate, cli::recursive)
 cli::param(name, description, t, validate, cli::recursive)
 cli::param(name, description, t, set_callback, cli::recursive)
 cli::param(name, description, t, cli::recursive)
 cli::param(name, t, set_callback, validate, cli::recursive)
 cli::param(name, t, validate, cli::recursive)
 cli::param(name, t, set_callback, cli::recursive)
 cli::param(name, t, cli::recursive)
```

where:

- **name** and **description** are `cli::string_constants`. **name** must be
  an [Id](#id).
- **t** is the object of the parameter of type `T`.
- **set_callback** is a callable the takes a `T` and returns `void`. Is is
  called when **t** or any of its subparameters are set.
- **validate**: is a validator for a `T`.

A call to these overloads will recursively build up subcommands of all the
members of **t**.

For example, given the structs and variable:

```cpp
struct SubSettings{
  int i = 0;
};

struct Settings{
  char c = 'x';
  SubSettings subsettings{};
};

void settings_callback(const Settings& s);

bool validate_settings(const Settings& s);

static constinit Settings settings;
```

and this call to `cli::param`

```cpp
cli::param("settings"_sc, settings, &settings_callback, &validate_settings, cli::recursive);
```

will generate the following command structure:

- settings \[Settings\]
  - c \[char\]
  - subsettings \[SubSettings\]
    - i \[int\]

#### Getters

Getters retrieve a parameter value. They are callables that take an lvalue
reference as its first and only argument and return a `cli::Error`. The return
value dictates if the value retrieval was successful. Getters are called by the
engine when a parameter retrieval command has been issued.

The concepts `cli::params::Getter` and `cli::params:GetterOf` check the interface.

```cpp
// denotes a getter. Requires that G is not templated.
template<typename G>
concept cli::params::Getter;

// denotes a getter of T. G can have a templated call operator.
template<typename G, typename T>
concept cli::param::GetterOf;
```

An example of a Getter for a parameter with type int:

```cpp

int* global_i=nullptr;

cli::Error get_int(int& i){
  if(global_i){
    i = *global_i;
    return cli::Error::none;
  }else{
    return cli::Error::cant_read_param;
  }
}

static_assert(cli::concept::Getter<decltype(get_int)>);
static_assert(cli::concept::GetterOf<decltype(get_int), int>);
```

#### Setters

Setters set a parameter value. They are callables that take an argument by
value or const reference and return a `cli::Error`. The return value dictates
if settings the value was successful. Setter are called by the engine when a
parameter set command has been issued.

The concepts `cli::params::Setter` and `cli::params::SetterOf` check the
interface.

An example of a Setter for a parameter with type int:

```cpp

int* global_i=nullptr;

cli::Error set_int(int i){
  if(global_i){
    *global_i = i;
    return cli::Error::none;
  }else{
    return cli::Error::cant_set_param;
  }
}

static_assert(cli::concept::Setter<decltype(set_int)>);
static_assert(cli::concept::SetterOf<decltype(set_int), int>);
```

### Functions

Functions are commands that execute an action. They take arguments and optionally
return a value. Functions can be called like so:

```bash
function(arg1, arg2, ..., argn)
function(name1 = arg1, name2 = arg2)
```

To create a function, you can use the `cli::func` template overload set.

#### Free Functions and Callables

A function is fully defined by:

- **name**: the function's name. Must be an [Id](#id).
- **description**: the function's description. Must be a `cli::string_constant`.
- **f**: the C++ callable that actually performs the action. This
  may be a free function, a functor/lambda, or a member function.
- **arguments**: Elements that describe the callable's arguments.
  [Arguments](#arguments) are created with the `cli::arg` overload set. All
  arguments of **f** must be specified.

The following overloads are available for free functions and
callables:

```cpp
// the base form
cli::func(name, description, f, arguments...);

// a function without a description
cli::func(name, f, arguments...);

// the functions name will be the type of f. I.e. if f's type is called
// "Functor", the function's name will be "Functor".
cli::func(f, description, arguments...);

// same as the previous overload without a description.
cli::func(f, arguments...);
```

Example:

```cpp
#include <cli.hpp>

using cli::operator""_sc;
using cli::operator""_arg;

int free1(int i, char c);
char free2();

struct func1{
  void operator()(int k);
};

struct func2{
  void operator()(int k, char c);
};

cli::func("free1"_sc, "free1 description"_sc, &free1, "i"_arg, "c"_arg);
cli::func("free2"_sc, &free2);
cli::func(func1{}, "func1 description"_sc, "k"_arg);
cli::func(func2{}, "k"_arg, "c"_arg);

```

These functions can then be called on the cli like so:

```python
free1(i = 5, c = k)
free2()
func1(k=10)
func1(10)
func2(k=10, c = d)
```

#### Member Functions

For member functions, the following overloads are available, where `t`
has the member function pointed to by `mem_fun_ptr`.

Warning: `t` is taken in by reference! `t` must live for the duration of the
engine's lifetime.

```cpp
cli::func(name, description, t, mem_fun_ptr, arguments...);
cli::func(name, t, mem_fun_ptr, arguments...);
```

Example:

```cpp
#include <cli.hpp>

using cli::operator""_sc;
using cli::operator""_arg;

struct T1{
  void m1();
};

static T1 t1;

cli::func("m1"_sc, t, &T1::m1);
```

There are additional overload for member functions available, but these require
a parent command that the corresponding member function can be executed upon.

```cpp
cli::func(name, description, mem_fun_ptr, args...);
cli::func(name, mem_fun_ptr, args...);
cli::func<mem_fun_ptr>(description, args...);
cli::func<mem_fun_ptr>(args...);
```

Example:

```cpp
struct S{
  int a;
  void apply();
  int foo();
};

static S s;

cii::param("s"_sc, // <- the parent command
           "s description"_sc,
           s,
           // the member function commands
           cli::func("apply"_sc, &S::apply),
           cli::func<&S::foo>())
```

Then `apply` and `foo` can be called like so:

```bash
s.apply()
s.foo()
```

#### Arguments

Arguments are the elements that describe c++ function arguments. These are then
used by Functions to parse the input into the specified values and validate
them.

There are two kinds of function arguments:

- _required_: these arguments must be specified when calling a function command,
  else it is an error.
- _optional_: these arguments can be left out because they have a default value.

Arguments are fully specified by their:

- **name**: the human readable name in form of a `cli::string_constant`. Must
  be an [Id](#id).
- **description**: a string that is used by the help functionality. A
  `cli::string_constant`.
- **T**: the value type of the argument
- **parser**: a [Parser](#parsing) to parse the arguments value.
- **validator**: a [Validator](#validation) used to validate the parsed value
- optionally, **Default**: a default value

`cli::arg` is the template overload set to use for creating arguments.

##### Optional Arguments

These overloads for optional arguments are available:

For this first overload set, `T` is explicitly specified.

```cpp
cli::arg<T, Default>(name, description, parser, validator);
cli::arg<T, Default>(name, description, parser);
cli::arg<T, Default>(name, description, validator);
cli::arg<T, Default>(name, description);
cli::arg<T, Default>(name);

// Example usage:
static constexpr int DefaultValue = 1.

cli::arg<double, DefaultValue>(
  "x"_sc,
  "the target x position"_sc,
  cli::parse::Parse<double, char>{},
  cli::validate::DefaultValidate<double, char>{});
```

For the following overloads, `T` is deduced from the parser (if available), or
the validator. This requires that **parser** and/or **validator** don't have a
templated call operator.

```cpp
cli::arg<Default>(name, description, parser, validator);
cli::arg<Default>(name, description, parser);
cli::arg<Default>(name, description, validator);

// Example usage:
cli::arg<1>("x"_sc,
            "the target x position"_sc,
            cli::parse::Parse<double, char>{},
            cli::validate::DefaultValidate<double, char>{});
```

For this overload set, `T` is deduced from Default, i.e. `T` is `decltype(Default)`

```cpp
cli::arg<Default>(name, description);
cli::arg<Default>(name);

// Example usage: T is deduced to int
cli::arg<100>("x"_sc, "the target x position"_sc);
```

##### Required Arguments

These overloads for required arguments are available:

```cpp
// the base form
cli::arg<T>(name, description, parser, validator);

// default validator is used
cli::arg<T>(name, description, parser);

// default parser is used
cli::arg<T>(name, description, validator);

// default parser and validator are used
cli::arg<T>(name, description);

// default parser and validator are used and no description will be available
// with the help command.
cli::arg<T>(name);

// Example usage:
cli::arg<double>("x"_sc,
                 "the target x position"_sc,
                 cli::parse::Parse<double, char>{},
                 cli::validate::DefaultValidate<double, char>{});
```

##### Deduced Arguments

Deduced arguments are required arguments that have their type, i.e `T`,
deduced. The default parser and validator are always used for these type of
arguments. The benefit of deduced arguments is the shorter notation.

There are two functions for creating deduced arguments:

```cpp
cli::arg(name, description)
cli::arg(name)

// Example usage:
cli::arg("x"_sc, "the target x position"_sc);
```

and the literal operator `_arg`

```cpp
cli::operator""_arg();

// Example usage:
"x"_arg // equivalent to cli::arg("x"_sc)
```

## Help Command

`CLI` provides a `help` function command, which prints the description of a
command.

To enable the help command, set `use_help` in the [config](#config) to `true`.

If the whole command tree should be printed when `help` is called without
arguments, set `empty_help_prints_commands` in the [config](#config) to `true`.

`help` takes two optional string arguments. The first argument is `cmd`, which
is the command name. The second argument is `arg`. `arg` should be empty for
parameter commands. For function commands, `arg` can set to an argument of the
function to get its description.

The signature of `help` is:

`(cmd: string?, arg: string?) -> void`

### Parameter Help

Calling `help` with the parameter name:

```python
help(param_name)
```

will return parameter's info in the form:

```python
[type]: description
```

### Function Help

Calling `help` for a function:

```python
help(function_name)
```

will return the function's info in the form:

```python
[(args...) -> return_type]: description
```

To get the description of a function's argument, use:

```python
help(function_name, arg_name)
```

which will print the argument's info in the form:

```python
[type]: description
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

### Id

The concept `cli::Id` denotes a `cli::string_constant` which does
not contain whitespace or any of these characters: `(){},='"`.

## Formatting

Formatting in `CLI` is performed with **Formatters**.

A **Formatter** takes a `cli::View<CharT>` as its first argument and a `T` as
its second argument, and returns a `cli::format::FormatResult`. The first
argument specifies the buffer into which the `T` should be formatted into. The
concepts `cli::format::FormatterOf` and `cli::format::Formatter` formalize the
interface. All formatting utilities can be found in the headers
`cli/basic_format.hpp` and `cli/format.hpp`.

An example of formatters:

```cpp
auto format_char(cli::View<char> output, char c)
  -> cli::format::FormatResult{
  if(output.size() == 0)
    return cli::Error::buffer_overflow;
  output[0] = c;
  return 1;
}

static_assert(cli::format::Formatter<decltype(format_char)>);
static_assert(cli::format::FormatterOf<decltype(format_char), char, char>);

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

static_assert(cli::format::Formatter<decltype(format_bool)>);
static_assert(cli::format::FormatterOf<decltype(format_bool), bool, char>);
```

Default implementations of formatters can be found in the `cli::format`
namespace, defined in the headers `cli/basic_format.hpp` and `cli/format.hpp`.
There are default formatters for:

- **bool**: formats as `true` and `false`. See also `cli::format::Bool`.
- **characters**: characters enclosed in **'**. For example `'c'`. See also
  `cli:format::Char`.
- **enumerations**: formats enumeration with their name. The namespace and enum
  type names are stripped. Example: `cli::Error::none` is formatted as `none`.
  Flag enums will have their bit name printed, separated by `|`.
  See also `cli::format::Enum`.
- **integers**: integers are formatted in decimal form. See also
  `cli::format::Int`.
- **fixpoint numbers**: to be documented.
- **sequences**: Sequences are formatted by formatting each element separated
  by `,`, and enclosed by square brackets (`[]`). Example: `[1, 2, 3]`. See also
  `cli::format::Sequence` and `cli::format::FixedSizeSequence`.
- **strings**: strings without spaces are put in as is. Strings with spaces are
  enclosed by double quotes (`"`). See also `cli::format::String` and
  `cli::format::StringView`.
- **aggregates**, i.e. simple structs. Their elements are formatted in the form
  `name = value`, enclosed by curly braces (`{}`). Example:
  `{name1=val1, name2 = val2}`. See also `cli::format::Struct`.
- TODO: floating point numbers

To enable formatting for custom types, first see if the type fulfills any of
these [concepts](#concepts):

- `Enum`
- `(FixedSize)Sequence`
- `String(View)`
- `Fixpoint`
- `Struct`

If so, you will just need to specialize the trait predicate as described in
[traits](#traits).

### FormatResult

`cli::format::FormatResult` is the type returned by formatters. It either
contains a `cli::Error`, indicating what went wrong (usually the buffer to
format into is too small), or the number of characters written into the buffer.

### Custom Formatting

To enable your type to be formattable when it is a member of a struct, then
you must specialize `cli::format::Format` and implement its call
operator.

```cpp
#include <cli/format.hpp>

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

To overrride default formatting behaviour, for example using hex format for
integers, you can explicitly specialize the corresponding formatter. But for
this to work, the character type of the buffer must be explicitly specified as
well. An example for integer formatters:

```cpp
#include <cli/format.hpp>

namespace cli::format{
  template<concepts::Integer T>
  struct Format<T,
                char /*<- your character type*/
                >: Int<T, char, Fmt::hex>{
  };
}
```

## Parsing

Parsing in CLI is based on two things:

- the class template `cli::parse::ParseResult`: the result of a parsing operation.
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
    return {cli::Error::invalid_character, buf};
  }
}

static_assert(cli::parse::ParserOf<decltype(parse_bool), bool, char>);
```

CLI provides defaults for parsing for the following types:

- **bool**: accepts `true`, `TRUE`, `1`, `false`, `FALSE`, and `0`.
- **characters**: accepts unquoted characters, quoted characters and hex
  numbers. See also `cli::parse::Char`.
- **enumerations**: enums are parsed by their name. For example a parser of
  `cli::Error` would parse the string `none` as `cli::Error::none`. See also
  `cli::parse::Enum`.
- **integers**: by default integers can be written in decimal, hex, and binary
  notation. See also `cli::parse::Int`.
- **fixpoint numbers**: TBD!
- **sequences**: sequences need their elements comma separated and have the
  whole things surrounded by square brackets. Example: `[1, 2 , 3]`. See also
  `cli::parse::Sequence` and `cli::parse::FixedSizeSequence`.
- **strings**: string can be unquoted, if they don't have any spaces. If they
  are quoted, then the inner quote can be escaped by `\`. See also
  `cli::parse::String` and `cli::parse::StringView`.
- **aggregates**: aggregates are key value pairs, separated by `,`, and
  enclosed by curly braces. Example: `{name1 = val1, ame2 = val2}`. The names are
  optional.

### ParseResult

`ParseResult<T, CharT>` is the class templated used as the parser return value.

There are four ways to construct a `ParseResult`. For the following section,
`e` is a value of type `cli::Error`, `val` is a value of type `T`, and `rest`
is a value of type `View<const CharT>`. `e` is the error that occurred during
parsing. `val` is the result of a successful parse. `rest`, in case of an
unsuccessful parse, can add context to the error location, but it is ok to
either return an empty rest string, just return the original buffer passed in,
or the just the view from the error location to the end. In case of success,
`rest` must contain the leftover string after the parsing operation.

```cpp
// construct a failed result
ParseResult<T, CharT>(e, rest);
ParseResult<T, CharT>(cli::parse::from_error, e, rest);

ParseResult<T, CharT>(val, rest);
ParseResult<T, CharT>(cli::parse::from_value, val, rest);

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

To overrride default parsing behaviour, for example using hex format for
integers, you can explicitly specialize the corresponding formatter. But for
this to work, the character type of the buffer must be explicitly specified as
well. An example for integer formatters:

```cpp
#include <cli/format.hpp>

namespace cli::parse{
  template<concepts::Integer T>
  struct Pare<T,
              char /*<- your character type*/
              >: Int<T, char, Fmt::hex>{
  };
}
```

## Traits

`CLI` uses traits to allow for an explicit opt-in approach to concepts. Traits
are defined in the header `cli/traits.hpp` in the namespace `cli::traits`. For
a concept `cli::concepts::Xxx` there is a traits predicate called
`cli::traits::is_xxx` (with the exception of chars, enums, and integers). The
concept is only true if the traits predicate is true. To enable the concept,
the traits predicate must be specialized.

There are the following trait predicates (i.e. they either inherit from
`std::true_type` or `std::false_type`, or have a static constexpr bool member
`value`):

- `is_char`: predicate for `cli::concepts::Char`. Should not be specialzed.
- `is_integer`: predicate for `cli::concepts::Integer`. Should not be specialized.
- `is_float`: predicate for `cli::concepts::Float`.
- `is_fixpoint`: predicate for `cli::concepts::Fixpoint`.
- `is_string`: predicate for `cli::concepts::String`.
- `is_string_view`: predicate for `cli::concepts::StringView`.
- `is_sequence`: predicate for `cli::concepts::Sequence`.
- `is_fixed_size_sequence`: predicate for `cli::concepts::FixedSizeSequence`.
- `is_struct`: predicate for `cli::concepts::Struct`. Inherits
  `std::is_aggregate`. Should only be specialized if the type can be
  deconstructed into a structured binding.
- `is_enum`: inherits `std::is_enum`. Should not be specialized.

There are three default specializations:

- `is_string_view<cli::View<CharT>>`: `cli::View` is a string view.
- `is_sequence<cli::FixedCapacityVector<T, Cap>>`: `cli::FixedCapacityVector`
  is a sequence.
- `is_fixed_size_sequence<std::array<T, Size>>`: `std::array` is a fixed size
  sequence.

Additionally, there are three "classic" traits structures, called
`integer_traits`, `float_traits`, and `enum_traits`. The first two must not be
specialized, but the last one must be specialized to avoid space overhead for
formatting and parsing enum classes, and to make weak enums formattable and
parseable. `CLI` tries to make a default guess, but that can have overhead, and
in case of weak enums results in undefined behaviour.

### enum_traits

If your enum class is signed and only has values in the range \[-128, 127\] or
your enum class is unsigned and has values in the range of \[0, 255\], then you
don't have to write anything to get enum class formatting and parsing support.
If that is not the case, or your enum is a weak enum, you will have to write
your own enum traits. Do this by specializing
[cli::traits::enum_traits](#enum_traits) and adjusting the parameters.

Specializing `enum_traits` for normal enum (classes).

```cpp
#include <cli/traits.hpp>
enum MyEnum{
  A = 1,
  B,
  C,
};

namespace cli::traits{
  template<>
  struct enum_traits<MyEnum>{
    // the smallest value of MyEnum
    static constexpr std::underlying_type_t<MyEnum> min = 1;
    // the biggest value of MyEnum
    static constexpr std::underlying_type_t<MyEnum> max = 3;
    // specifies that MyEnum is not a flag enum
    static constexpr bool is_flag = false;
  };
}
```

Specializing `enum_traits` for flag enumerations:

```cpp
#include <cli/traits.hpp>
enum MyFlags{
  A = 1 << 0,
  B = 1 << 1,
  C = 1 << 2,
};

namespace cli::traits{
  template<>
  struct enum_traits<MyFlags>{
    // the lowest bit of the flag
    static constexpr std::underlying_type_t<MyFlags> min = 0;
    // the highest bit of the flag
    static constexpr std::underlying_type_t<MyFlags> max = 2;
    // specifies that MyFlags is a flag enum
    static constexpr bool is_flag = true;
  };
}
```

## Concepts

There are the following concepts used by the formatting and parsing utilities
in `CLI`. They are defined in the header `cli/concepts.hpp` n the namespace
`cli::concepts`.

### Sequence

A `Sequence` represents a dynamically sized collection of values, for example
`cli::FixedCapacityVector`. To enable the concept for your custom sequence
type, you must override the traits predicate `cli::traits::is_sequence` and
satisfy the following properties:

- `T` must be constructible without arguments.
- `T` must be copy constructible.
- `T` must have an inner typedef called `value_type`.
- `T` must be for loop iterable, i.e. `for(const T::value_type& value:
seq){...}` must iterate over the sequence. This requires `begin` and `end`
  methods which return an iterator for `T`.
- have a `max_size()` method, which returns the maximum number of elements the
  sequence can store as a `std::size_t`.
- have a `push_back(const T&)/push_back(T&&)` method, which adds a new element at
  the end of the sequence.

Example:

```cpp
class MySeq{
  using value_type = int;
  class iterator;
  class const_iterator;

  MySeq();
  MySeq(const MySeq&);
  MySeq(MySeq&&);
  const_iterator begin() const;
  const_iterator end() const;
  std::size_t max_size() const;
  void push_back(const value_type& value);
  void push_back(value_type&& value);
};

#include <cli/traits.hpp>

namespace cli::traits{
  template<>
  struct is_sequence<MySeq> : std::true_type{};
}

#include <cli/concepts.hpp>

static_assert(cli::concepts::Sequence<MySeq>);
```

### FixedSizeSequence

A `FixedSizeSequence` represents a collection of values with a fixed number of
elements, for example `std::array` (c-style arrays are not supported). To
enable the concept for your custom sequence type `T`, you must specialize the
traits predicate `cli::traits::is_fixed_size_sequence` and satisfy the
following properties:

- `T` must be constructible without arguments.
- `T` must be copy constructible.
- `T` must have an inner typedef called `value_type`.
- `T` must be for loop iterable, i.e. `for(const T::value_type& value:
seq){...}` must iterate over the sequence. This requires `begin` and `end`
  methods which return an iterator for `T`.
- have a `size()` method which returns the number of elements in the sequence
  as a `std::size_t`.
- have `operator[](std::size_t i) -> value_type&` defined, i.e. mutable element
  access.

Example:

```cpp
class MyFixedSeq{
  using value_type = int;
  class iterator;
  class const_iterator;

  MyFixedSeq();
  MyFixedSeq(const MySeq&);
  MyFixedSeq(MySeq&&);
  const_iterator begin() const;
  const_iterator end() const;
  std::size_t size() const;
  value_type& operator[](std::size_t i);
};

#include <cli/traits.hpp>

namespace cli::traits{
  template<>
  struct is_fixed_size_sequence<MySeq> : std::true_type{};
}

#include <cli/concepts.hpp>

static_assert(cli::concepts::FixedSizeSequence<MySeq>);
```

### String

`String` denotes a sequence of characters. To enable the concept for your
custom sequence type `T`, you must override the traits predicate
`cli::traits::is_string` and satisfy the following properties:

- `T` must have an inner typedef called `value_type`.
- `T` must be constructible without arguments
- `T` must be constructible from a `const value_type*` and a `std::size_t`,
  i.e. pointer and size.
- `T` must be copy constructible
- `T` must be for loop iterable, i.e. `for const T::value_type&ch: str){...}`
  must iterate over the characters of `T`.
- `T` must have a `size()` method that returns a `std::size_t`, which is the
  number of characters stored in `T`.
- `T` must have a `push_back(value_type)` method that appends a character.
- `T` must have `operator[](std::size_t i) const -> const value_type&` defined,
  i.e. immutable element access.

Example:

```cpp
class MyString{
  using value_type = char;
  class const_iterator;
  MyString();
  MyString(const char* s, std::size_t n);
  const_iterator begin() const;
  const_iterator end() const;
  const value_type& operator[](std::size_t i) const;
  void push_back(char c);
};

#include <cli/traits.hpp>

namespace cli::traits{
  template<>
  struct is_string<MyString> : std::true_type{};
}

#include <cli/concepts.hpp>

static_assert(cli::concepts::String<MyString>);
```

### StringView

`StringView` denotes a non-owning [string](#string). To enable the concept for
your custom sequence type `T`, you must override the traits predicate
`cli::traits::is_string` and satisfy the following properties:

- `T` must have an inner typedef called `value_type`.
- `T` must be constructible without arguments
- `T` must be constructible from a `value_type*` and a `std::size_t`,
  i.e. pointer and size.
- `T` must be copy constructible
- `T` must be for loop iterable, i.e. `for const T::value_type&ch: str){...}`
  must iterate over the characters of `T`.
- `T` must have a `size()` method that returns a `std::size_t`, which is the
  number of characters stored in `T`.
- `T` must have `operator[](std::size_t i) const -> const value_type&` defined,
  i.e. immutable element access.

### Fixpoint

### Struct

A `Struct` denotes a `T` that can be used with structured bindings, and is not
a [Sequence](#sequence) or a [FixedSizeSequence](#fixedsizesequence). All
aggregates fulfill the `Struct` concept.

## Validation

`CLI` has the concepts `cli::validate::ValidatorOf` and `cli::validate::Validator`.
These concepts define the interface `CLI` uses to validate values parsed from a
string.

```cpp
template<class V>
concept cli::validate::Validator;

template<class V, typename T>
concept cli::validate::ValidatorOf;
```

A `Validator` of `T` is a callable that takes a `T` either by value or const
reference, and returns a `bool`. The return value indicates if the value is
valid (`true`) or invalid (`false`).

Example for validator of `int`:

```cpp
// foo must be in the range [0,100]
bool validate_foo(int foo){
  return foo >= 0 and foo <= 100;
}
```

## Simulation

`CLI` can also be run on the PC. All simulation functions are in the header
`cli/sim.hpp` and in the `cli::sim` namespace.

An example:

```cpp
#include <cli/sim.hpp>

struct Config{...};

cli::sim::Engine engine{
  Config{},
  commands...
};

int main(){
  if (not cli::sim::init())
    return -1;

  engine.print();

  while (engine.get_input_and_process()) {
  }

  return 0;
}
```

You can then compile and run the executable in a terminal.

There are the following key mappings available:

- `Ctrl+C`: quits the simulation.
- `Ctrl+J`: clears the screen.
- `Ctrl+L`: clears the entire line
- `Ctrl+K`: clears text from the cursor to the end of the line.
- `Ctrl+U`: clears text from the cursor to the beginning of the line.
- Arrow up/down: scrolls through the history, if history is enabled.
- Arrow left/right: moves the cursor, if the cursor is enabled.
- backspace: deletes the character before the cursor.
- delete: deletes the character under the cursor.
- Enter: executes the current line.

**Note:** [cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal) is
required to run simulations. For meson projects, the option `sim` must be
enabled.

## cli-term

`cli-term` is a command line executable to connect to an embedded system
running `CLI` over a serial port or an ethernet connection.

### cli-term Usage

```bash
cli-term args...
```

where args are:

- -a, --address: a serial port, or ip address in the form ip:port. Must be
  supplied. Example: "COM4" or "168.0.0.1:80".
- -s, --size: the character size used by your cli. One of [8, 16, 32].
  Default is 8.
- -sb, --stopbits: number of serial poprt stop bits. One of [1, 1.5, 2, one,
  one_point_five, two]. Default is none.
- -p, --parity: serial port parity. One of [none, odd, even]. Default is
  none.
- -d, --delimiter: the cli delimiter. One of [lf, cr, crlf]. Defualt is
  lf.
- -e, --endian: the endian used on the system running cli. Only applicable if
  size is not 8. One of [big, little]. Default is big.
- -b, --baudrate: the serial port baudrate. A positive number. Default is 115200.

Example command:

```bash
cli-term --address COM4 --parity odd --stopbits 1.5 --baudrate 115200
```

### Mappings

There are the following key mappings available:

- `Ctrl+C`: quits cli-term.
- `Ctrl+J`: clears the screen.
- `Ctrl+L`: clears the entire line
- `Ctrl+K`: clears text from the cursor to the end of the line.
- `Ctrl+U`: clears text from the cursor to the beginning of the line.
- Arrow up/down: scrolls through the history, if history is enabled.
- Arrow left/right: moves the cursor, if the cursor is enabled.
- backspace: deletes the character before the cursor.
- delete: deletes the character under the cursor.
- Enter: executes the current line.

### Compiling cli-term

Building cli-term requires these dependencies:

- [asio](https://think-async.com/Asio/)
- [cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal)

When you use meson, simply execute this command in the project root:

```bash
meson setup build -Dcli-term=enabled
meson compile -C build
```

If you don't use meson, you must have asio and cpp-terminal available and
also add CLIs include folder to your include path.

Example command to build it with g++:

```bash
g++ source/cli-term.cpp -Iinclude -o cli-term
```
