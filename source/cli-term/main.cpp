#include "asio/io_context.hpp"
#include "cli-term/cli-term.hpp"
#include <exception>

int main(int argc, const char **argv) {
  cli::term::cli_term_init();

  if (argc == 1) {
    Term::cerr << "Error: no arguments given\n";
    cli::term::print_usage();
    exit(-1);
  }

  asio::io_context ctx;

  const cli::term::Settings settings = cli::term::get_settings_from_args(
    std::vector<std::string_view>{argv + 1, argv + argc});

  try {
    std::shared_ptr<cli::term::Connection> conn =
      cli::term::create_connection(ctx, settings);
    conn->start();
    return 0;
  } catch (const Term::Exception &re) {
    Term::cerr << "cpp-terminal error: " << re.what() << std::flush;
    return -1;
  } catch (const asio::system_error &e) {
    Term::cerr << std::format("IO Error: {}:{}", e.code().value(), e.what())
               << std::flush;
    return -1;
  } catch (const std::exception &e) {
    Term::cerr << "Unknown Error: " << e.what() << std::flush;
    return -1;
  }

  return 0;
}
