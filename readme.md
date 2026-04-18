# CLI

CLI is a C++20 header only library to build hierarchical ANSI command line
interfaces for embedded systems. It can be used to set and retrieve
parameters and call functions on the embedded device.

## Components

CLI has the following components.

### Config

The Config is a type traits like structure to configure CLI. This tells CLI what
the character type is, how large the buffers it uses should be, and which
features to use.

An example is given here:

```{cpp}
struct my_cli_config {
  // the character type to be used
  using char_type = char8_t;
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

## How to use

## Simple Example
