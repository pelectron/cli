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
 * The LED driver functions that need to be implemented for the example to work
 * @{
 */
void LedEnable(bool enable) { (void)enable; }
bool LedIsEnabled() { return false; }
void LedSetColor(RGB color) { (void)color; }
// an interval of 0 means no flashing
void LedSetFlashingInterval(Milliseconds interval) { (void)interval; }
/// @}

cli::Error write_char(char c);

static_assert(cli::CharStream<decltype(write_char), char>);

static RGB color{};
static Milliseconds interval;
static bool flashing_enabled;

cli::Error set_enable(bool enable) {
  LedEnable(enable);
  return cli::Error::none;
}

cli::Error get_enable(bool &enable) {
  enable = LedIsEnabled();
  return cli::Error::none;
}

auto set_color = [](const RGB &c) -> cli::Error {
  color = c;
  LedSetColor(c);
  return cli::Error::none;
};

cli::Error set_interval(Milliseconds i) {
  interval = i;
  LedSetFlashingInterval(interval);
  return cli::Error::none;
};

cli::Error get_interval(Milliseconds &i) {
  i = interval;
  return cli::Error::none;
};

bool validate_interval(Milliseconds i) { return i < 10000; }

cli::Error set_flashing(bool enabled) {
  flashing_enabled = enabled;
  if (enabled) {
    LedSetFlashingInterval(interval);
  } else {
    LedSetFlashingInterval(0);
  }
  return cli::Error::none;
}

using config = cli::default_config;
using cli::operator""_sc;

// clang-format off
static cli::Cli control{
  config{}, 
  cli::AnsiOutput(config{}, &write_char),
  cli::param("color"_sc, 
             "the led color"_sc, 
             color, 
             set_color),
  cli::param<bool>("enable"_sc, 
                   "turn the led on or off"_sc,
                   &get_enable,
                   &set_enable),
  cli::param("flashing"_sc, 
             "enable or disable LED flashing"_sc,
             flashing_enabled,
             &set_flashing),
  cli::param("interval"_sc,
             "the flashing interval"_sc,
             interval,
             &get_interval,
             &set_interval,
             &validate_interval)
};
// clang-format on

void UartCallback() {
  char c = UartGetChar();
  control.on_char(c);
}

char UartGetChar() { return 0; }

void UartTransmit(char c) { (void)c; }

cli::Error write_char(char c) {
  // transmit c over uart
  UartTransmit(c);
  return cli::Error::none;
}

int main() {
  while (1) {
    control.process();
  }
  return 0;
}
