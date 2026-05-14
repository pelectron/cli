/**
 * @file cli/sim.hpp
 * @brief This header contains the functions to simulate a cli on a PC terminal.
 * See also @ref Simulation.
 *
 * @defgroup Simulation Simulation
 *
 * clis can also be simulated on a PC.
 *
 * See [here](docs.md#simulation) for more info.
 */

#ifndef CLI_SIM_HPP
#define CLI_SIM_HPP

#include "cli.hpp"
#include "cli/config.hpp"
#include "cli/enums.hpp"
#include "display.hpp"

namespace cli::sim {

  class Engine {
  public:
    template<concepts::Config Config, concepts::Command... Commands>
    Engine(Config config, Commands &&...commands)
      : engine_{
          new EngineImpl{Config{},
                         constant_v<std::numeric_limits<std::size_t>::max()>,
                         std::forward<Commands>(commands)...}
    } {
      (void)config;
    }

    template<concepts::Config Config,
             auto NumLines,
             concepts::Command... Commands>
    Engine(Config config,
           constant<NumLines> number_of_lines,
           Commands &&...commands)
      : engine_{
          new EngineImpl{Config{},
                         constant_v<NumLines>,
                         std::forward<Commands>(commands)...}
    } {
      (void)config;
      (void)number_of_lines;
    }

    ~Engine();

    cli::Error error() const;

    bool get_input_and_process();

    cli::Error on_char(char c);

    cli::Error on_control(cli::Control ctrl, std::uint8_t n = 1);

    void print();

    void reset();

  private:
    static void write_view(cli::View<const char> s);

    struct EngineInterface {
      virtual cli::Error on_char(char c) = 0;
      virtual cli::Error on_control(cli::Control ctrl, std::uint8_t n = 1) = 0;
      virtual cli::Error process() = 0;
      virtual void reset() = 0;
      virtual void print() = 0;
      virtual cli::Delimiter delimiter() const = 0;
      virtual bool has_multiline_display() const = 0;
      virtual ~EngineInterface();
    };

    template<concepts::Config Config,
             auto NumLines,
             concepts::Command... Commands>
    struct EngineImpl : EngineInterface {
      EngineImpl(Config, constant<NumLines>, Commands &&...commands)
        : engine{
            Config{},
            cli::AnsiDisplay{&Engine::write_view,
                   constant<std::size_t{NumLines}>{}},
            std::forward<Commands>(commands)...
      } {}

      ~EngineImpl() {}

      cli::Error on_char(char c) override { return engine.on_char(c); }

      cli::Error on_control(cli::Control ctrl, std::uint8_t n = 1) override {
        return engine.on_control(ctrl, n);
      }

      cli::Error process() override { return engine.process(); }

      void print() override { return engine.print(); }

      void reset() override { return engine.reset(); }

      cli::Delimiter delimiter() const override {
        return cli::config::input_delimiter_v<Config>;
      }

      virtual bool has_multiline_display() const { return NumLines > 0; }

      cli::Engine<Config,
                  cli::AnsiDisplay<decltype(&Engine::write_view),
                                   static_cast<std::size_t>(NumLines)>,
                  Commands...>
        engine;
    };

    template<concepts::Config Config,
             auto NumLines,
             concepts::Command... Commands>
    EngineImpl(Config, constant<NumLines>, Commands &&...)
      -> EngineImpl<std::remove_cvref_t<Config>,
                    NumLines,
                    std::remove_cvref_t<Commands>...>;

    EngineInterface *engine_;
    cli::Error error_{cli::Error::none};
  };

  /**
   * @brief The init function for the sim.
   *
   * @ingroup Simulation
   * @return true if init succeded, else false.
   */
  bool init();

} // namespace cli::sim

#endif
