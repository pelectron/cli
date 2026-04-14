/**
 * This is an example for a CLI that controls an RGB LED.
 *
 * What the CLI should be able to do:
 *   - turn the LED on and off
 *   - set the color of the LED
 *   - set a flashing interval
 *   - enable and disable the flashing
 */

#include "cli/cli.hpp"

#include <cstdint>

/// the type that contains the led color information
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// the type that represents the flashing interval in milliseconds
using Milliseconds = uint32_t;

/**
 * The UART driver functions that need to be implemented/changed for the example
 * comunication to be functional
 * @{
 */

/// called when a char is received by the UART peripheral
void UartCallback();

/// retrieves the received char from the UART peripheral
char UartGetChar();

/// transmits a char over the UART peripheral
void UartTransmit(char c);
/// @}

/**
 * The LED driver functions that need to be implemented fot the example to work
 * @{
 */

void LedEnable(bool enable) { (void)enable; }
void LedSetColor(RGB color) { (void)color; }
void LedSetInterval(Milliseconds interval) { (void)interval; }
void LedFlashing(bool enable) { (void)enable; }
/// @}

cli::Error write(char c);

static_assert(cli::CharStream<decltype(write), char>);

static RGB color{};
static Milliseconds interval;

auto get_color = [](RGB &c) -> cli::Error {
  c = color;
  return cli::Error::none;
};

auto set_color = [](const RGB &c) -> cli::Error {
  color = c;
  LedSetColor(c);
  return cli::Error::none;
};

cli::Error set_interval(Milliseconds i) {
  interval = i;
  LedSetInterval(interval);
  return cli::Error::none;
};

cli::Error get_interval(Milliseconds &i) {
  i = interval;
  return cli::Error::none;
};

using config = cli::default_config;
using cli::operator""_sc;
// clang-format off
static cli::Cli control{
    config{}, 
    cli::AnsiOutput(config{}, &write),
    cli::param("color"_sc, 
               "the led color"_sc, 
               color, 
               get_color,
               set_color),
    cli::func("enable"_sc,
              "turn the led on or off"_sc,
              &LedEnable,
                cli::arg("enable"_sc)),
    cli::func("flashing"_sc, 
              "enable or disable LED flashing"_sc,
              &LedFlashing,
              cli::arg("enable"_sc)),
    cli::param("interval"_sc,
               "the flashing interval"_sc,
               interval,
               &get_interval,
               &set_interval)
};
// clang-format on

int main() {}

void UartCallback() {
  char c = UartGetChar();
  control.on_char(c);
}

char UartGetChar() { return 0; }

void UartTransmit(char c) { (void)c; }

cli::Error write(char c) {
  // transmit c over uart
  UartTransmit(c);
  return cli::Error::none;
}
