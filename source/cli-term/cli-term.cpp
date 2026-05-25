#include "cli-term/cli-term.hpp"
#include "asio/io_context.hpp"
#include "cli/ctti.hpp"
#include "cli/parse.hpp"

#include <cpp-terminal/color.hpp>
#include <cpp-terminal/event.hpp>
#include <cpp-terminal/exception.hpp>
#include <cpp-terminal/input.hpp>
#include <cpp-terminal/iostream.hpp>
#include <cpp-terminal/terminal.hpp>
#include <cpp-terminal/tty.hpp>
#include <system_error>

namespace cli::term {

  void print_usage() {
    // clang-format off
  Term::cerr <<  "Usage:\n"
                "cli-term [args...]\n"
                "with args:\n"
                "-a, --address: a serial port, or ip address in the form ip:port. Must be supplied.\n"
                "-s, --size: the character size used by your cli. One of [8, 16, 32]. Default is 8.\n"
                "-sb, --stopbits: number of serial poprt stop bits. One of [1, 1.5, 2, one, one_point_five, two]. Default is none.\n"
                "-p, --parity: serial port parity. One of [none, odd, even]. Default is none.\n"
                "-d, --delimiter: the cli delimiter. One of [lf, cr, crlf]. Defualt is lf.\n"
                "-e, --endian: the endian used on the system running cli. Only applicable if size is not 8. One of [big, little]. Default is big.\n"
                "-b, --baudrate: the serial port baudrate. A positive number. Default is 115200.\n";
    // clang-format on
  }

  void print_settings(const Settings &settings) {
    Term::cout << "Settings:\n";
    Term::cout << "  address: " << settings.address
               << "\n  size: " << settings.char_size << "\n  delimiter: "
               << std::string_view{cli::ctti::enum_name(settings.delim).data()}
               << "\n endian: "
               << std::string_view{cli::ctti::enum_name(settings.endian).data()}
               << std::endl;

    if (settings.address.find(":") != std::string::npos) {
      return;
    }

    Term::cout << "  baudrate: " << settings.baudrate << '\n';
    Term::cout << "  stop bits: ";
    switch (settings.stopbits) {
      case asio::serial_port::stop_bits::one:
        Term::cout << "1\n";
        break;
      case asio::serial_port::stop_bits::onepointfive:
        Term::cout << "1.5\n";
        break;
      case asio::serial_port::stop_bits::two:
        Term::cout << "2\n";
        break;
    }

    Term::cout << "  parity: ";
    switch (settings.parity) {
      case asio::serial_port::parity::none:
        Term::cout << "none\n";
        break;
      case asio::serial_port::parity::odd:
        Term::cout << "odd\n";
        break;
      case asio::serial_port::parity::even:
        Term::cout << "even\n";
        break;
    }
    Term::cout << std::flush;
  }

  Settings get_settings_from_args(const std::vector<std::string_view> &args) {
    Settings settings{};
    bool has_address = false;
    for (std::size_t i = 0; i < (args.size() - 1); i += 2) {
      const auto &arg = args[i + 1];
      if (args[i] == "-a" or args[i] == "--address") {
        settings.address = args[i + 1];
        has_address = true;
      } else if (args[i] == "-s" or args[i] == "--size") {
        if (arg == "8") {
          settings.char_size = 8;
        } else if (arg == "16") {
          settings.char_size = 16;
        } else if (arg == "32") {
          settings.char_size = 32;
        } else {
          Term::cerr << "Invalid argument value for size, expected "
                        "one of [8, 16, 32], got '"
                     << arg << "'\n";
          print_usage();
          exit(-1);
        }
      } else if (args[i] == "-sb" or args[i] == "--stopbits") {
        if (arg == "1" or arg == "one") {
          settings.stopbits = asio::serial_port::stop_bits::one;
        } else if (arg == "1.5" or arg == "one_point_five") {
          settings.stopbits = asio::serial_port::stop_bits::onepointfive;
        } else if (arg == "2" or arg == "two") {
          settings.stopbits = asio::serial_port::stop_bits::two;
        } else {
          Term::cerr << "Invalid argument value for stopbits, expected one of "
                        "[1, 1.5, 2, "
                        "one, one_point_five,two], got '"
                     << arg << "'\n";
          print_usage();
          exit(-1);
        }
      } else if (args[i] == "-p" or args[i] == "--parity") {
        if (arg == "none") {
          settings.parity = asio::serial_port::parity::none;
        } else if (arg == "odd") {
          settings.parity = asio::serial_port::parity::odd;
        } else if (arg == "even") {
          settings.parity = asio::serial_port::parity::even;
        } else {
          Term::cerr << "Invalid argument value for parity, expected "
                        "one of [none, odd, even], got '"
                     << arg << "'\n";
          print_usage();
          exit(-1);
        }
      } else if (args[i] == "-d" or args[i] == "--delimiter") {
        if (arg == "lf") {
          settings.delim = cli::Delimiter::lf;
        } else if (arg == "cr") {
          settings.delim = cli::Delimiter::cr;
        } else if (arg == "crlf") {
          settings.delim = cli::Delimiter::crlf;
        } else {
          Term::cerr << "Invalid argument value for delimiter, expected "
                        "one of [none, odd, even], got '"
                     << arg << "'\n",
            print_usage();
          exit(-1);
        }
      } else if (args[i] == "-e" or args[i] == "--endian") {
        if (arg == "big") {
          settings.endian = std::endian::big;
        } else if (arg == "little") {
          settings.endian = std::endian::little;
        } else {
          Term::cerr << "Invalid argument value for endian, expected "
                        "one of [little, big], got '"
                     << arg << "'\n";
          print_usage();
          exit(-1);
        }
      } else if (args[i] == "-b" or args[i] == "--baudrate") {
        cli::parse::Parse<uint32_t, char> parse;
        auto res = parse({arg.data(), arg.size()});
        if ((not res) or res.rest.size() != 0) {
          Term::cerr << "Invalid argument value for baudrate, expected "
                        "a positive number, got '"
                     << arg << "'\n";
          print_usage();
          exit(-1);
        }
        settings.baudrate = res.value;
      } else {
        Term::cerr << "unknown argument '" << args[i] << "'\n";
        print_usage();
        exit(-1);
      }
    }
    if (not has_address) {
      Term::cerr << "no address supplied\n";
      print_usage();
      exit(-1);
    }
    return settings;
  }

  std::string to_utf8(char32_t cp) {
    char buf[16]{};
    std::mbstate_t state{};
    size_t len = std::c32rtomb(buf, cp, &state);
    return (len != (size_t)-1) ? std::string(buf, len) : "";
  }

  std::string to_utf8(char16_t cp) {
    char buf[16]{};
    std::mbstate_t state{};
    size_t len = std::c16rtomb(buf, cp, &state);
    return (len != (size_t)-1) ? std::string(buf, len) : "";
  }

  std::string mappings() {
    // clang-format off
  return  "\nMappings:\n"
          "  Ctrl+L clears the current line\n"
          "  Ctrl+J clears the screen\n"
          "  Ctrl+K clears line from cursor to the end\n"
          "  Ctrl+U clears line from cursor to the start\n"
          "  Ctrl+H prints the mappings\n";
    // clang-format on
  }

  void Connection::on_key(Term::Key key) {
    wr_size = 0;
    switch (key.value) {
      case Term::Key::Ctrl_C:
        close();
        exit(0);
        break;
      case Term::Key::Ctrl_J:
        put_char('\x1b');
        put_char('[');
        put_char('2');
        put_char('J');
        break;
      case Term::Key::Ctrl_K:
        put_char('\x1b');
        put_char('[');
        put_char('0');
        put_char('K');
        break;
      case Term::Key::Ctrl_L:
        put_char('\x1b');
        put_char('[');
        put_char('2');
        put_char('K');
        break;
      case Term::Key::Ctrl_U:
        put_char('\x1b');
        put_char('[');
        put_char('1');
        put_char('K');
        break;
      case Term::Key::Ctrl_H:
        Term::cout << mappings() << std::flush;
        return;
      case Term::Key::Enter:
        switch (settings.delim) {
          case cli::Delimiter::lf:
            put_char('\n');
            break;
          case cli::Delimiter::cr:
            put_char('\r');
            break;
          case cli::Delimiter::crlf:
            put_char('\r');
            put_char('\n');
            break;
        }
        break;
      case Term::Key::ArrowDown:
        put_char('\x1b');
        put_char('[');
        put_char('B');
        break;
      case Term::Key::ArrowUp:
        put_char('\x1b');
        put_char('[');
        put_char('A');
        break;
      case Term::Key::ArrowLeft:
        put_char('\x1b');
        put_char('[');
        put_char('D');
        break;
      case Term::Key::ArrowRight:
        put_char('\x1b');
        put_char('[');
        put_char('C');
        break;
      default:
        put_char(key.value);
    }

    do_write(asio::const_buffer{write_buf, wr_size},
             [self = this->shared_from_this()](std::error_code ec,
                                               std::size_t n) mutable {
               (void)n;
               self->on_error(ec);
             });
  }

  void Connection::put_char(char32_t c) {
    if (settings.char_size == 8) {
      write_buf[wr_size++] = static_cast<char8_t>(c);
    } else if (settings.char_size == 16) {
      if (settings.endian == std::endian::big) {
        write_buf[wr_size++] = static_cast<char8_t>(c >> 8);
        write_buf[wr_size++] = static_cast<char8_t>(c);
      } else {
        write_buf[wr_size++] = static_cast<char8_t>(c);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 8);
      }
    } else if (settings.char_size == 32) {
      if (settings.endian == std::endian::big) {
        write_buf[wr_size++] = static_cast<char8_t>(c >> 24);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 16);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 8);
        write_buf[wr_size++] = static_cast<char8_t>(c);
      } else {
        write_buf[wr_size++] = static_cast<char8_t>(c);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 8);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 16);
        write_buf[wr_size++] = static_cast<char8_t>(c >> 24);
      }
    } else {
      assert(false);
    }
  }

  void Connection::on_error(const std::error_code &ec) {
    if (not ec)
      return;

    if (ec == asio::error::connection_aborted or
        ec == asio::error::operation_aborted)
      return;

    Term::cout << Term::color_fg(Term::Color::Name::BrightRed)
               << "Error during communication. " << ec.value() << ": "
               << ec.message() << '\n'
               << Term::color_fg(Term::Color::Name::Default) << std::flush;
    close();
    return;
  }

  void Connection::close() {
    do_close();
    if (not ctx.stopped())
      ctx.stop();
  }

  void Connection::on_receive(const std::error_code &ec, std::size_t n) {
    if (ec) {
      return on_error(ec);
    }

    if (settings.char_size == 8) {
      for (std::size_t i = 0; i < n; ++i) {
        if (read_buf[i] == '\r')
          continue;
        Term::cout << static_cast<char>(read_buf[i]);
      }
      Term::cout << std::flush;
      start_receive();
    } else if (settings.char_size == 16) {
      read += n;
      if (read < 2) {
        start_receive();
      } else {
        for (std::size_t i = 0; i < (read - 1); i += 2) {
          char16_t ch;
          if (settings.endian == std::endian::big) {
            ch = static_cast<char16_t>(read_buf[i]) << 8 |
                 static_cast<char16_t>(read_buf[i + 1]);
          } else {
            ch = static_cast<char16_t>(read_buf[i + 1]) << 8 |
                 static_cast<char16_t>(read_buf[i]);
          }
          if (ch == '\r')
            continue;
          Term::cout << to_utf8(ch);
        }
        Term::cout << std::flush;

        if (read % 2 == 1) {
          read_buf[0] = read_buf[read - 1];
          read = 1;
        } else {
          read = 0;
        }
        start_receive();
      }
      Term::cout << std::flush;
    } else if (settings.char_size == 32) {
      read += n;
      if (read < 4) {
        start_receive();
      } else {
        for (std::size_t i = 0; i < (read - 3); i += 4) {
          char32_t ch;
          if (settings.endian == std::endian::big) {
            ch = static_cast<char32_t>(read_buf[i + 0]) << 24 |
                 static_cast<char32_t>(read_buf[i + 1]) << 16 |
                 static_cast<char32_t>(read_buf[i + 2]) << 8 |
                 static_cast<char32_t>(read_buf[i + 3]);
          } else {
            ch = static_cast<char32_t>(read_buf[i + 3]) << 24 |
                 static_cast<char32_t>(read_buf[i + 2]) << 16 |
                 static_cast<char32_t>(read_buf[i + 1]) << 8 |
                 static_cast<char32_t>(read_buf[i + 0]);
          }
          if (ch == '\r')
            continue;
          Term::cout << to_utf8(ch);
        }
        Term::cout << std::flush;

        if (read % 4 == 0) {
          read = 0;
        } else {
          const std::size_t max_i = read % 4;
          for (std::size_t i = 0; i < max_i; ++i) {
            read_buf[i] = read_buf[read - max_i + i];
          }
          read = read % 4;
        }
        start_receive();
      }
    } else {
      // invalid char_size
      assert(false);
    }
  }

  void Connection::start_receive() {
    do_start_receive(
      asio::buffer(read_buf.data() + read, read_buf.size() - read),
      [self = this->shared_from_this()](const std::error_code &ec,
                                        std::size_t n) mutable {
        self->on_receive(ec, n);
      });
  }

  Connection::Connection(asio::io_context &ctx, const Settings &settings)
    : settings(settings), read_buf(256, 0), ctx(ctx) {}

  Connection::~Connection() { close(); }

  void Connection::start() {
    start_receive();
    ctx_runner =
      std::jthread{[self = this->shared_from_this()] { self->ctx.run(); }};
    while (1) {
      try {
        Term::Event event = Term::read_event();
        if (event.type() == Term::Event::Type::Key)
          on_key(Term::Key(event));
      } catch (const Term::Exception &re) {
        Term::cerr << "cpp-terminal error: " << re.what() << std::flush;
        close();
      } catch (...) {
        Term::cerr << "Unknown error." << std::flush;
        close();
      }
    }
  }

  template<class Stream>
  class Connectionmpl : public Connection {
    Stream stream;

    void do_start_receive(asio::mutable_buffer buf,
                          std::function<void(const std::error_code &,
                                             std::size_t)> callback) override {
      stream.async_read_some(buf, std::move(callback));
    }

    void do_write(asio::const_buffer buf,
                  std::function<void(const std::error_code &, std::size_t)>
                    callback) override {
      asio::async_write(stream, buf, std::move(callback));
    }

    void do_close() override {
      if (stream.is_open())
        stream.close();
    }

  public:
    Connectionmpl(asio::io_context &ctx, const Settings &settings)
      : Connection(ctx, settings), stream(ctx) {
      if constexpr (std::is_same_v<Stream, asio::serial_port>) {
        try {
          stream.open(settings.address);
        } catch (...) {
          Term::cerr << "Couldn't open serial port '" << settings.address
                     << "'\n";
          exit(-1);
        }
        try {
          stream.set_option(asio::serial_port::baud_rate(settings.baudrate));
          stream.set_option(asio::serial_port::character_size(8));
          stream.set_option(asio::serial_port::parity(settings.parity));
          stream.set_option(asio::serial_port::stop_bits(settings.stopbits));
        } catch (const asio::system_error &e) {
          Term::cerr << "couldn't set serial port options. " << e.code().value()
                     << ": " << e.code().message() << "\n";
          exit(-1);
        }
      } else {
        static_assert(std::is_same_v<Stream, asio::ip::tcp::socket>);

        std::size_t split = settings.address.find(':');
        std::string_view addr = {settings.address.data(),
                                 settings.address.data() + split};
        std::string_view port = {settings.address.data() + split + 1,
                                 settings.address.data() +
                                   settings.address.size()};

        cli::parse::Parse<uint16_t, char> parse;
        auto port_res = parse({port.data(), port.size()});

        if ((not port_res) or port_res.rest.size() != 0) {
          Term::cerr << "Invalid port '" << port
                     << "': Expected 'ip-address:port', where port is an "
                        "unsigned 16 bit integer.\n";
          exit(-1);
        }

        asio::ip::tcp::endpoint ep{};
        try {
          ep = asio::ip::tcp::endpoint(asio::ip::make_address(addr),
                                       port_res.value);
        } catch (...) {
          Term::cerr << "Invalid ip address '" << addr
                     << "'. Expected an address "
                        "in the form of 'ip-address:port'\n";
          exit(-1);
        }

        std::error_code ec;
        stream.connect(ep, ec);
        if (ec) {
          Term::cerr << "Couldn't open socket " << addr << ":" << port_res.value
                     << ". " << ec.value() << ": " << ec.message();
          exit(-1);
        }
      }
    }
  };

  std::shared_ptr<Connection> create_connection(asio::io_context &ctx,
                                                const Settings &settings) {
    if (settings.address.find(':') == std::string::npos) {
      return std::make_shared<Connectionmpl<asio::serial_port>>(ctx, settings);
    } else {
      return std::make_shared<Connectionmpl<asio::ip::tcp::socket>>(ctx,
                                                                    settings);
    }
  }

  void cli_term_init() {
    Term::terminal_title("cli-term");

    // initialize cpp-terminal
    Term::terminal.setOptions(Term::Option::NoClearScreen,
                              Term::Option::NoSignalKeys,
                              Term::Option::Cursor,
                              Term::Option::Raw);

    if (!Term::is_stdin_a_tty()) {
      Term::cerr << "The terminal is not attached to a TTY and "
                    "therefore can't catch user input. Exiting..."
                 << std::flush;
      exit(-1);
    }
  }
} // namespace cli::term
