#ifndef CLI_TERM_HPP
#define CLI_TERM_HPP

#ifdef _MSC_VER
#define _WIN32_WINNT 0x0601
#endif

#include "cli/enums.hpp"

#include <asio.hpp>
#include <bit>
#include <cpp-terminal/key.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cuchar>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

namespace cli::term {

  struct Settings {
    uint32_t baudrate = 115200;
    uint32_t char_size = 8;
    asio::serial_port::stop_bits::type stopbits =
      asio::serial_port::stop_bits::one;
    asio::serial_port::parity::type parity = asio::serial_port::parity::none;
    cli::Delimiter delim = cli::Delimiter::lf;
    std::endian endian = std::endian::big;
    std::string address{};
  };

  void print_usage();

  void print_settings(const Settings &settings);

  Settings get_settings_from_args(const std::vector<std::string_view> &args);

  std::string to_utf8(char32_t cp);

  std::string to_utf8(char16_t cp);

  std::string mappings();

  class Connection : public std::enable_shared_from_this<Connection> {
    uint8_t write_buf[16]{};
    const Settings settings;
    std::jthread ctx_runner;
    std::vector<uint8_t> read_buf{};
    asio::io_context &ctx;
    std::size_t read = 0;
    std::size_t wr_size = 0;

    void on_key(Term::Key key);

    void put_char(char32_t c);

    virtual void do_start_receive(
      asio::mutable_buffer buf,
      std::function<void(const std::error_code &, std::size_t)> callback) = 0;

    virtual void do_write(
      asio::const_buffer buf,
      std::function<void(const std::error_code &, std::size_t)> callback) = 0;

    virtual void do_close() = 0;

    void on_error(const std::error_code &ec);
    void close();
    void on_receive(const std::error_code &ec, std::size_t n);
    void start_receive();

  public:
    Connection(asio::io_context &ctx, const Settings &settings);

    virtual ~Connection();

    void start();
  };

  std::shared_ptr<cli::term::Connection>
  create_connection(asio::io_context &ctx, const Settings &settings);

  void cli_term_init();

} // namespace cli::term

#endif
