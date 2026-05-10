/**
 * @example examples/led.cpp
 * This is an example for a CLI that controls an RGB LED.
 *
 * What the CLI should be able to do:
 *   - turn the LED on and off
 *   - set the color of the LED
 *   - set a flashing interval
 *   - enable and disable the flashing
 */

#include "cli.hpp"
#include "cli/enums.hpp"

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
void UartCallback() noexcept;

/// retrieves the received char from the UART peripheral
char UartGetChar() noexcept;

/// transmits a char over the UART peripheral
void UartTransmit(char c) noexcept;
/// @}

/**
 * The LED driver functions that need to be implemented for the example to work
 * @{
 */
void LedEnable(bool enable) noexcept { (void)enable; }
bool LedIsEnabled() noexcept { return false; }
void LedSetColor(RGB color) noexcept { (void)color; }
// an interval of 0 means no flashing
void LedSetFlashingInterval(Milliseconds interval) noexcept { (void)interval; }
/// @}

void write_char(char c) noexcept;

static RGB color{};
static Milliseconds interval;
static bool flashing_enabled;

cli::Error set_enable(bool enable) noexcept {
  LedEnable(enable);
  return cli::Error::none;
}

cli::Error get_enable(bool &enable) noexcept {
  enable = LedIsEnabled();
  return cli::Error::none;
}

auto set_color = [](const RGB &c) noexcept -> cli::Error {
  color = c;
  LedSetColor(c);
  return cli::Error::none;
};

cli::Error set_interval(Milliseconds i) noexcept {
  interval = i;
  LedSetFlashingInterval(interval);
  return cli::Error::none;
};

cli::Error get_interval(Milliseconds &i) noexcept {
  i = interval;
  return cli::Error::none;
};

bool validate_interval(Milliseconds i) noexcept { return i < 10000; }

cli::Error set_flashing(bool enabled) noexcept {
  flashing_enabled = enabled;
  if (enabled) {
    LedSetFlashingInterval(interval);
  } else {
    LedSetFlashingInterval(0);
  }
  return cli::Error::none;
}

cli::Error foo(int i = 0) noexcept { return cli::Error::none; }

struct config : cli::default_config {
  static constexpr bool use_autocomplete = false;
  static constexpr bool use_history = true;
  static constexpr bool use_cursor = false;
  static constexpr bool use_help = false;
  static constexpr bool empty_help_prints_commands = false;
  static constexpr bool use_detailed_error_messages = false;
};

using cli::operator""_sc;

// clang-format off
static constinit cli::Engine control{
  config{}, 
  cli::AnsiDisplay(&write_char),
  /*cli::param("color"_sc, 
             "the led color"_sc, 
             color, 
             set_color),*/
  cli::param<bool>("enable"_sc, 
                   "turn the led on or off"_sc,
                   &get_enable,
                   &set_enable)/*,
  cli::param("flashing"_sc, 
             "enable or disable LED flashing"_sc,
             flashing_enabled,
             &set_flashing),
  cli::func("foo"_sc,"foos the LED"_sc,&foo,cli::arg<1>("i"_sc))
  cli::param("interval"_sc,
             "the flashing interval"_sc,
             interval,
             &get_interval,
             &set_interval,
             &validate_interval)*/
};
// clang-format on

void UartCallback() noexcept {
  char c = UartGetChar();
  control.on_char(c);
}

char UartGetChar() noexcept { return 0; }

void UartTransmit(char c) noexcept { (void)c; }

void write_char(char c) noexcept {
  // transmit c over uart
  UartTransmit(c);
}

int main() {
  while (1) {
    control.process();
  }
  return 0;
}
