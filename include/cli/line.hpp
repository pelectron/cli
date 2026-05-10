#ifndef CLI_CURSOR_HPP
#define CLI_CURSOR_HPP

#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/config.hpp"
#include "cli/display.hpp"
#include "cli/enums.hpp"
#include "cli/exec_result.hpp"
#include "cli/format.hpp"
#include "cli/string.hpp"
#include "cli/util.hpp"

#include <cstddef>
#include <limits>

namespace cli {

  namespace dtl {
    template<bool MutliLineDisplay>
    struct CommandEntered {
      bool value{false};
      constexpr operator bool() noexcept { return value; }
      constexpr CommandEntered &operator=(bool v) noexcept {
        value = v;
        return *this;
      }
    };

    template<>
    struct CommandEntered<true> {
      constexpr CommandEntered(bool) noexcept {}
      constexpr operator bool() noexcept { return false; }
      constexpr CommandEntered &operator=(bool) noexcept { return *this; }
    };

    template<class Index, class Display>
    constexpr Error
    cursor_left(Index &cursor_, Display &display_, std::size_t n) noexcept {
      if (cursor_ == 0)
        return Error::none;
      if (n >= cursor_)
        n = cursor_;
      cursor_ = static_cast<Index>(cursor_ - n);
      display_.cursor_left(n);
      return Error::none;
    }

    template<class Index, class Display>
    constexpr Error cursor_right(auto &cursor_,
                                 Index &size_,
                                 Display &display_,
                                 std::size_t n) noexcept {
      if (cursor_ == size_)
        return Error::none;
      if (n + cursor_ > size_)
        n = size_ - cursor_;
      cursor_ = static_cast<Index>(cursor_ + n);
      display_.cursor_right(n);
      return Error::none;
    }

    template<std::size_t MaxLineLength, class CharT, class Index, class Display>
    constexpr Error insert_write(CharT *data_,
                                 Index &cursor_,
                                 Index &size_,
                                 Display &display_,
                                 View<const CharT> s) noexcept {
      if (s.size() == 0)
        return Error::none;

      if (s.size() + size_ > MaxLineLength)
        return Error::buffer_overflow;

      if (cursor_ == size_ or size_ == 0) {
        for (const CharT &ch : s)
          data_[size_++] = ch;
        cursor_ = size_;
        display_.write(s);
        return Error::none;
      }

      // copy data to the back
      for (auto i = size_ - 1; i >= cursor_ and i < size_; --i) {
        data_[i + s.size()] = data_[i];
      }

      // copy s into data_
      for (std::size_t i = 0; i < s.size(); ++i) {
        data_[cursor_ + i] = s[i];
      }

      // update size and cursor
      const std::size_t old_cursor = cursor_;
      size_ += static_cast<Index>(s.size());
      cursor_ += static_cast<Index>(s.size());

      // write new data
      display_.write({data_ + old_cursor, data_ + size_});
      // adjust cursor
      display_.cursor_left(size_ - cursor_);
      return Error::none;
    }

    template<class Cfg, class CharT, class Index, class Display>
    constexpr Error on_char(CharT *data_,
                            Index &cursor_,
                            Index &size_,
                            Display &display_,
                            auto &command_entered_,
                            CharT c) noexcept {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0 and command_entered_) {
        command_entered_ = false;
        display_.newline();
      }

      return insert_write<Cfg::max_line_length>(
        data_, cursor_, size_, display_, {&c, 1});
    }

    template<class CharT, class Index, class Display>
    constexpr Error backspace(CharT *data_,
                              Index &cursor_,
                              Index &size_,
                              Display &display_,
                              std::size_t n) noexcept {
      if (cursor_ == 0)
        return Error::none;
      else if (cursor_ == size_) {
        if (n >= size_) {
          size_ = 0;
          cursor_ = 0;
          display_.clear_line();
          return Error::none;
        } else {
          size_ = static_cast<Index>(size_ - n);
          cursor_ = static_cast<Index>(cursor_ - n);
          display_.backspace(n);
          return Error::none;
        }
      } else if (n >= cursor_) {
        for (Index i = cursor_; i < size_; ++i) {
          data_[i - cursor_] = data_[i];
        }
        size_ -= cursor_;
        cursor_ = 0;

        display_.clear_line();

        display_.write({data_, size_});
        display_.cursor_left(size_);
        return Error::none;
      } else {
        n = dtl::min(cursor_, n);
        for (Index i = static_cast<Index>(cursor_ - n); i < size_ - n; ++i) {
          data_[i] = data_[i + n];
        }
        cursor_ = static_cast<Index>(cursor_ - n);
        size_ = static_cast<Index>(size_ - n);

        display_.clear_line();
        display_.write({data_, size_});
        display_.cursor_left(size_ - cursor_);
        return Error::none;
      }
    }

    template<class CharT, class Display>
    constexpr Error delete_char(CharT *data_,
                                auto &cursor_,
                                auto &size_,
                                Display &display_) noexcept {
      if (cursor_ == size_)
        return Error::none;

      // move cursor to the end
      display_.cursor_right(size_ - cursor_);
      // delete characters
      display_.backspace(size_ - cursor_);
      if (cursor_ == size_ - 1) {
        --size_;
        return Error::none;
      }

      // write the new characters
      display_.write({data_ + cursor_ + 1, data_ + size_});

      // move cursor to old position
      display_.cursor_left(size_ - cursor_ - 1);

      // copy data
      for (auto i = cursor_; (i + 1) < size_; ++i) {
        data_[i] = data_[i + 1];
      }
      --size_;
      return Error::none;
    }

    template<class Display>
    constexpr Error
    clear_line_to_end(auto &cursor_, auto &size_, Display &display_) noexcept {
      if (cursor_ == size_) {
        return Error::none;
      }

      if (cursor_ == 0) {
        size_ = 0;
        display_.clear_line();
        return Error::none;
      }

      display_.cursor_right(size_ - cursor_);
      display_.backspace(size_ - cursor_);
      size_ = cursor_;
      return Error::none;
    }

    template<class CharT, class Display>
    constexpr Error clear_line_to_begin(CharT *data_,
                                        auto &cursor_,
                                        auto &size_,
                                        Display &display_) noexcept {
      if (cursor_ == 0)
        return Error::none;
      if (cursor_ == size_) {
        cursor_ = 0;
        size_ = 0;
        display_.clear_line();
        return Error::none;
      }

      display_.clear_line();
      for (std::size_t i = cursor_; i < size_; ++i)
        data_[i - cursor_] = data_[i];

      size_ = size_ - cursor_;
      cursor_ = 0;

      display_.write({data_, size_});
      display_.cursor_left(size_);
      return Error::none;
    }

    template<typename Cfg,
             typename Line,
             typename CharT,
             typename Index,
             typename Display>
    Error print_result(const ExecResult<CharT> &exec_result,
                       [[maybe_unused]] Line &line,
                       const CharT *data_,
                       Index size_,
                       Display &display_) noexcept {
      if constexpr (cli::is_multiline_display_v<Display>) {
        display_.newline();
      }

      const auto print_success = [&exec_result, &display_]() {
        View<const CharT> result = exec_result.result();
        if (result.size() != 0) {
          if constexpr (not cli::is_multiline_display_v<Display>) {
            display_.newline();
          }
          // print the result
          display_.write(result);

          if constexpr (cli::is_multiline_display_v<Display>) {
            display_.newline();
          }
        }
        return Error::none;
      };

      if constexpr (config::use_detailed_error_messages_v<Cfg>) {
        constexpr View<const CharT> expected_str =
          string_constant<CharT, 'e', 'x', 'p', 'e', 'c', 't', 'e', 'd', ' '>{};

        switch (exec_result.type()) {
          case ExecResult<CharT>::success:
            return print_success();
          case ExecResult<CharT>::parse_error: {
            if constexpr (not cli::is_multiline_display_v<Display>) {
              display_.newline();
            }
            display_.write(string_constant<CharT,
                                           'p',
                                           'a',
                                           'r',
                                           's',
                                           'e',
                                           ' ',
                                           'e',
                                           'r',
                                           'r',
                                           'o',
                                           'r',
                                           ':',
                                           ' '>{});

            switch (exec_result.error()) {
              case Error::expected_assignment:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('=');
                display_.write('\'');
                break;
              case Error::expected_delimiter:
                display_.write(expected_str);
                display_.write('\'');
                display_.write(',');
                display_.write('\'');
                break;
              case Error::expected_endquote:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('"');
                display_.write('\'');
                break;
              case Error::expected_lparen:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('(');
                display_.write('\'');
                break;
              case Error::expected_rparen:
                display_.write(expected_str);
                display_.write('\'');
                display_.write(')');
                display_.write('\'');
                break;
              case Error::expected_lbrace:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('{');
                display_.write('\'');
                break;
              case Error::expected_rbrace:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('}');
                display_.write('\'');
                break;
              case Error::expected_lbracket:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('[');
                display_.write('\'');
                break;
              case Error::expected_rbracket:
                display_.write(expected_str);
                display_.write('\'');
                display_.write(']');
                display_.write('\'');
                break;
              case Error::expected_another_field:
                display_.write(expected_str);
                display_.write(string_constant<CharT,
                                               'a',
                                               'n',
                                               'o',
                                               't',
                                               'h',
                                               'e',
                                               'r',
                                               ' ',
                                               'f',
                                               'i',
                                               'e',
                                               'l',
                                               'd'>{});
                break;
              case Error::expected_another_arg:
                display_.write(expected_str);
                display_.write(string_constant<CharT,
                                               'a',
                                               'n',
                                               'o',
                                               't',
                                               'h',
                                               'e',
                                               'r',
                                               ' ',
                                               'a',
                                               'r',
                                               'g'>{});
                break;
              case Error::expected_field:
                display_.write(expected_str);
                display_.write(
                  string_constant<CharT, 'f', 'i', 'e', 'l', 'd'>{});
                break;
              case Error::expected_args:
                display_.write(expected_str);
                display_.write(string_constant<CharT, 'a', 'r', 'g', 's'>{});
                break;
              case Error::unexpected_characters_after_closing_paren:
                display_.write(string_constant<CharT,
                                               'c',
                                               'h',
                                               'a',
                                               'r',
                                               'a',
                                               'c',
                                               't',
                                               'e',
                                               'r',
                                               's',
                                               ' ',
                                               'a',
                                               'f',
                                               't',
                                               'e',
                                               'r',
                                               ' ',
                                               '\'',
                                               ')',
                                               '\''>{});
                break;
              default:
                display_.write(
                  ctti::enum_name<Error, CharT>(exec_result.error()));
            }

            display_.write(string_constant<CharT, ' ', 'a', 't', ' '>{});

            const CharT *err_loc = exec_result.error_location();
            std::size_t error_location =
              (err_loc == nullptr ? size_ : err_loc - data_) + 1;

            CharT buffer[20]{};
            format::Int<std::size_t, CharT> format;
            format::FormatResult fmt_res = format({buffer, 20}, error_location);
            CLI_ASSERT(fmt_res);
            display_.write({buffer, fmt_res.size_written});
          } break;
          case ExecResult<CharT>::format_error: {
            if constexpr (not cli::is_multiline_display_v<Display>) {
              display_.newline();
            }
            display_.write(string_constant<CharT,
                                           'f',
                                           'o',
                                           'r',
                                           'm',
                                           'a',
                                           't',
                                           ' ',
                                           'e',
                                           'r',
                                           'r',
                                           'o',
                                           'r',
                                           ':',
                                           ' '>{});

            display_.write(ctti::enum_name<Error, CharT>(exec_result.error()));
          } break;
          case ExecResult<CharT>::validation_error: {
            if constexpr (not cli::is_multiline_display_v<Display>) {
              display_.newline();
            }
            display_.write(string_constant<CharT, 'a', 'r', 'g', ' '>{});
            CharT buffer[20]{};
            format::Int<std::size_t, CharT> format;
            format::FormatResult fmt_res =
              format({buffer, 20}, exec_result.index());
            CLI_ASSERT(fmt_res);
            display_.write({buffer, fmt_res.size_written});
            display_.write(string_constant<CharT,
                                           ' ',
                                           'i',
                                           's',
                                           ' ',
                                           'i',
                                           'n',
                                           'v',
                                           'a',
                                           'l',
                                           'i',
                                           'd'>{});
          } break;
          case ExecResult<CharT>::set_error: {
            if constexpr (not cli::is_multiline_display_v<Display>) {
              display_.newline();
            }
            display_.write(string_constant<CharT,
                                           'c',
                                           'a',
                                           'n',
                                           '\'',
                                           't',
                                           ' ',
                                           's',
                                           'e',
                                           't',
                                           ' ',
                                           'p',
                                           'a',
                                           'r',
                                           'a',
                                           'm',
                                           ':',
                                           ' '>{});
            switch (exec_result.error()) {
              case Error::expected_assignment:
                display_.write(expected_str);
                display_.write('\'');
                display_.write('=');
                display_.write('\'');
                break;
              default:
                display_.write(
                  ctti::enum_name<Error, CharT>(exec_result.error()));
            }
          } break;
          case ExecResult<CharT>::get_error: {
            if constexpr (not cli::is_multiline_display_v<Display>) {
              display_.newline();
            }
            display_.write(string_constant<CharT,
                                           'c',
                                           'a',
                                           'n',
                                           '\'',
                                           't',
                                           ' ',
                                           'g',
                                           'e',
                                           't',
                                           ' ',
                                           'p',
                                           'a',
                                           'r',
                                           'a',
                                           'm',
                                           ':',
                                           ' '>{});
            display_.write(ctti::enum_name<Error, CharT>(exec_result.error()));
          } break;
        }

      } else {
        switch (exec_result.type()) {
          case ExecResult<CharT>::success:
            return print_success();
          default:
            display_.write(string_constant<CharT, 'e', 'r', 'r', 'o', 'r'>{});
        }
      }

      if (cli::is_multiline_display_v<Display>) {
        display_.newline();
      }
      return Error::none;
    }

    template<class Cfg, typename Line, class CharT, class Display>
    Error execute(Line &line,
                  CharT *data_,
                  auto &size_,
                  auto &root_,
                  Display &display_,
                  auto &command_entered_,
                  const View<CharT> &out) noexcept {
      command_entered_ = true;
      // parse the input
      SplitResult res =
        split_line({data_, size_}, &root_, Cfg::access_separator);

      if (res.command == nullptr) {
        display_.newline();

        display_.write(ctti::enum_name<Error, CharT>(Error::invalid_cmd));

        if constexpr (cli::is_multiline_display_v<Display>) {
          display_.newline();
        }

        size_ = 0;
        return Error::invalid_cmd;
      }

      // execute the command
      ExecResult<CharT> exec_result = res.command->execute(res.args, out);

      Error e = print_result<Cfg>(exec_result, line, data_, size_, display_);
      // reset line data
      size_ = 0;
      return e;
    }
  } // namespace dtl

  /**
   * @brief The Line class is used to handle autocomplete, cursor movement,
   * storing received characters, and displaying the characters.
   *
   * There are four specializations for each combination of use_cursor and
   * use_autocomplete.
   * @tparam Cfg
   */
  template<typename Cfg, concepts::Display<typename Cfg::char_type> Display>
  class Line;

  template<typename Cfg,
           concepts::DisplayWithoutCursor<typename Cfg::char_type> Display>
    requires((not Cfg::use_cursor) and (not Cfg::use_autocomplete))
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    using Index = smallest_type_for_value_t<Cfg::max_line_length>;
    using CmdEntered = dtl::CommandEntered<is_multiline_display_v<Display>>;

    CharT data_[Cfg::max_line_length]{};
    Index size_{};
    CLI_NO_UNIQUE_ADDRESS
    CmdEntered command_entered_{false};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display) noexcept
      : root_(root), display_(display) {}

    constexpr Error on_char(CharT c) noexcept {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0 and command_entered_) {
        command_entered_ = false;
        display_.newline();
      }

      data_[size_++] = c;
      display_.write(c);
      return Error::none;
    }

    constexpr Error on_backspace(std::size_t n = 1) noexcept {
      if (n >= size_) {
        size_ = 0;
        display_.clear_line();
      } else {
        size_ = static_cast<Index>(size_ - n);
        display_.backspace(n);
      }
      return Error::none;
    }

    constexpr Error on_autocomplete() noexcept { return Error::none; }

    constexpr Error set_data(View<const CharT> s) noexcept {
      if (s.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      display_.clear_line();

      size_ = 0;
      if (s.size() == 0)
        return Error::none;

      for (const auto &ch : s) {
        data_[size_++] = ch;
      }

      display_.write(view());
      return Error::none;
    }

    constexpr Error clear() noexcept {
      size_ = 0;
      display_.clear_line();
      return Error::none;
    }

    constexpr Error on_delete_char() noexcept { return Error::none; }

    constexpr Error on_cursor_left(std::size_t) noexcept { return Error::none; }

    constexpr Error on_cursor_right(std::size_t) noexcept {
      return Error::none;
    }

    constexpr Error on_clear_line_to_end() noexcept { return Error::none; }

    constexpr Error on_clear_line_to_begin() noexcept { return clear(); }

    constexpr Error on_clear_screen() noexcept {
      size_ = 0;
      display_.clear_screen();
      return Error::none;
    }

    constexpr View<const CharT> view() const noexcept { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) noexcept {
      return dtl::execute<Cfg>(
        *this, data_, size_, root_, display_, command_entered_, out);
    }

    constexpr void reset() noexcept {
      command_entered_ = false;
      size_ = 0;
    }
  };

  template<typename Cfg,
           concepts::DisplayWithoutCursor<typename Cfg::char_type> Display>
    requires((not Cfg::use_cursor) and Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    using Index = smallest_type_for_value_t<Cfg::max_line_length>;
    using CmdEntered = dtl::CommandEntered<is_multiline_display_v<Display>>;
    static constexpr Index max_index = std::numeric_limits<Index>::max();

    CharT data_[Cfg::max_line_length]{};
    Index size_{0};
    Index start_of_args_{max_index};
    Index last_access_separator_ = max_index;
    CLI_NO_UNIQUE_ADDRESS
    CmdEntered command_entered_{true};
    const CommandNode<CharT> *command_{};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display) noexcept
      : command_(&root), root_(root), display_(display) {}

    constexpr Error on_char(CharT c) noexcept {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0 and command_entered_) {
        command_entered_ = false;
        display_.newline();
      }

      if (start_of_args_ < size_) {
        data_[size_++] = c;
        display_.write(c);
        return Error::none;
      }

      switch (c) {
        case Cfg::access_separator:
          if (command_->subcommand == nullptr)
            return Error::none;

          last_access_separator_ = size_;
          data_[size_++] = c;
          display_.write(c);
          return Error::none;
        case ' ':
          [[fallthrough]];
        case '=':
          [[fallthrough]];
        case '(':
          [[fallthrough]];
        case ')':
          start_of_args_ = size_;
          data_[size_++] = c;
          display_.write(c);
          return Error::none;
        default: {
          data_[size_++] = c;
          if (size_ == 1) {
            for (const auto &cmd : root_) {
              if (cmd.name[0] == c) {
                command_ = &cmd;
                display_.write(c);
                return Error::none;
              }
            }
            --size_;
            return Error::none;
          }

          if (last_access_separator_ == size_ - 2) {
            for (const auto &cmd : *command_) {
              if (cmd.name[0] == c) {
                command_ = &cmd;
                display_.write(c);
                return Error::none;
              }
            }
            --size_;
            return Error::none;
          }

          const View<const CharT> name{data_ + (last_access_separator_ > size_
                                                  ? 0
                                                  : last_access_separator_),
                                       size_};
          auto *cmd = command_;
          while (cmd and not cmd->name.starts_with(name)) {
            cmd = cmd->next;
          }
          if (cmd == nullptr) {
            --size_;
            return Error::none;
          }
          command_ = cmd;
          display_.write(c);
          return Error::none;
        }
      }
    }

    constexpr void reset() noexcept {
      command_entered_ = false;
      size_ = 0;
      command_ = &root_;
      start_of_args_ = max_index;
      last_access_separator_ = max_index;
    }

    constexpr Error clear() noexcept {
      reset();
      display_.clear_line();
      return Error::none;
    }

    constexpr Error on_backspace(std::size_t n = 1) noexcept {
      if (n >= size_) {
        return clear();
      }
      size_ = static_cast<Index>(size_ - n);
      if (size_ > start_of_args_) {
        display_.backspace(n);
        return Error::none;
      }

      for (Index i = start_of_args_ - 1; i >= size_; --i) {
        if (data_[i] == Cfg::access_separator) {
          command_ = command_->parent;
        }
      }

      start_of_args_ = max_index;
      last_access_separator_ = view().find_last_of(Cfg::access_separator);
      display_.backspace(n);
      return Error::none;
    }

    constexpr Error on_autocomplete() noexcept {
      if (start_of_args_ < size_)
        return Error::none;

      if (size_ == 0 or last_access_separator_ == size_ - 1) {
        if (command_->subcommand == nullptr)
          return Error::none;
        command_ = command_->subcommand;
        const View autocomplete_string = command_->name;
        for (const auto &ch : autocomplete_string) {
          data_[size_++] = ch;
        }
        display_.write(autocomplete_string);
        return Error::none;
      }

      const View autocomplete_string =
        command_->name.substr(size_ - last_access_separator_ - 1);

      if (autocomplete_string.size() == 0) {
        return on_char(Cfg::access_separator);
      }

      for (const CharT &ch : autocomplete_string) {
        data_[size_++] = ch;
      }

      display_.write(autocomplete_string);
      return Error::none;
    }

    constexpr Error set_data(View<const CharT> string) noexcept {
      if (string.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      Error e = clear();
      if (e != Error::none or string.size() == 0) {
        return e;
      }

      const std::size_t arg_start = string.find_first_of(
        View<const CharT>{string_constant<CharT, ' ', '(', '='>{}});

      View<const CharT> cmd_name;
      if (arg_start == View<const CharT>::npos) {
        cmd_name = string;
      } else {
        cmd_name = string.substr(0, arg_start);
      }

      bool last_char_is_access_separator = false;
      if (arg_start == View<const CharT>::npos and
          cmd_name[cmd_name.size() - 1] == Cfg::access_separator) {
        cmd_name = cmd_name.substr(0, cmd_name.size() - 1);
        last_char_is_access_separator = true;
      }

      const CommandNode<CharT> *parent = &root_;
      std::size_t end = cmd_name.find_first_of(Cfg::access_separator);
      while (end != View<const CharT>::npos) {
        const View child_name = cmd_name.substr(0, end);
        bool found = false;
        for (const CommandNode<CharT> &child : *parent) {
          if (child.name == child_name) {
            parent = &child;
            cmd_name = cmd_name.substr(end + 1);
            end = cmd_name.find_last_of(Cfg::access_separator);
            found = true;
            break;
          }
        }
        if (not found)
          return Error::invalid_cmd;
      }

      bool found = false;
      for (const CommandNode<CharT> &child : *parent) {
        if (child.name.starts_with(cmd_name)) {
          if (last_char_is_access_separator and child.subcommand == nullptr)
            return Error::invalid_cmd;
          command_ = &child;
          found = true;
          break;
        }
      }
      if (not found)
        return Error::invalid_cmd;

      start_of_args_ = static_cast<Index>(arg_start);
      last_access_separator_ = static_cast<Index>(
        string.substr(0, arg_start).find_last_of(Cfg::access_separator));
      size_ = 0;
      for (const auto &ch : string) {
        data_[size_++] = ch;
      }

      display_.write(string);
      return Error::none;
    }

    constexpr Error on_delete_char() noexcept { return Error::none; }

    constexpr Error on_cursor_left(std::size_t) noexcept { return Error::none; }

    constexpr Error on_cursor_right(std::size_t) noexcept {
      return Error::none;
    }

    constexpr Error on_clear_line_to_end() noexcept { return Error::none; }

    constexpr Error on_clear_line_to_begin() noexcept { return clear(); }

    constexpr Error on_clear_screen() noexcept {
      size_ = 0;
      command_ = &root_;
      start_of_args_ = max_index;
      last_access_separator_ = max_index;
      display_.clear_screen();
      return Error::none;
    }

    constexpr View<const CharT> view() const noexcept { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) noexcept {
      command_entered_ = true;
      // execute the command
      ExecResult<CharT> exec_result =
        command_->execute(view().substr(start_of_args_), out);

      Error e =
        dtl::print_result<Cfg>(exec_result, *this, data_, size_, display_);

      // reset line data
      size_ = 0;
      start_of_args_ = max_index;
      last_access_separator_ = max_index;
      command_ = &root_;
      return e;
    }
  };

  template<typename Cfg,
           concepts::DisplayWithCursor<typename Cfg::char_type> Display>
    requires(Cfg::use_cursor and not Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    using Index = smallest_type_for_value_t<Cfg::max_line_length>;
    using CmdEntered = dtl::CommandEntered<is_multiline_display_v<Display>>;

    CharT data_[Cfg::max_line_length]{};
    Index size_{};
    Index cursor_{};
    CLI_NO_UNIQUE_ADDRESS
    CmdEntered command_entered_{false};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display) noexcept
      : root_(root), display_(display) {}

    constexpr Error on_char(CharT c) noexcept {
      return dtl::on_char<Cfg>(
        data_, cursor_, size_, display_, command_entered_, c);
    }

    constexpr Error on_backspace(std::size_t n = 1) noexcept {
      return dtl::backspace(data_, cursor_, size_, display_, n);
    }

    constexpr Error on_autocomplete() noexcept { return Error::none; }

    constexpr Error set_data(View<const CharT> s) noexcept {
      if (s.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      Error e = clear();
      if (e != Error::none or s.size() == 0)
        return e;

      for (const auto &ch : s) {
        data_[size_++] = ch;
      }
      cursor_ = size_;
      display_.write(view());
      return Error::none;
    }

    constexpr void reset() noexcept {
      command_entered_ = false;
      size_ = 0;
      cursor_ = 0;
    }

    constexpr Error clear() noexcept {
      reset();
      display_.clear_line();
      return Error::none;
    }

    constexpr Error on_delete_char() noexcept {
      return dtl::delete_char(data_, cursor_, size_, display_);
    }

    constexpr Error on_cursor_left(std::size_t n) noexcept {
      return dtl::cursor_left(cursor_, display_, n);
    }

    constexpr Error on_cursor_right(std::size_t n) noexcept {
      return dtl::cursor_right(cursor_, size_, display_, n);
    }

    constexpr Error on_clear_line_to_end() noexcept {
      return dtl::clear_line_to_end(cursor_, size_, display_);
    }

    constexpr Error on_clear_line_to_begin() noexcept {
      return dtl::clear_line_to_begin(data_, cursor_, size_, display_);
    }

    constexpr Error on_clear_screen() noexcept {
      size_ = 0;
      cursor_ = 0;
      display_.clear_screen();
      return Error::none;
    }

    constexpr View<const CharT> view() const noexcept { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) noexcept {
      Error e = dtl::execute<Cfg>(
        *this, data_, size_, root_, display_, command_entered_, out);
      cursor_ = 0;
      return e;
    }
  };

  template<typename Cfg,
           concepts::DisplayWithCursor<typename Cfg::char_type> Display>
    requires(Cfg::use_cursor and Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    using Index = smallest_type_for_value_t<Cfg::max_line_length>;
    using CmdEntered = dtl::CommandEntered<is_multiline_display_v<Display>>;
    CharT data_[Cfg::max_line_length]{};
    Index size_{};
    Index cursor_{};
    CLI_NO_UNIQUE_ADDRESS
    CmdEntered command_entered_{false};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display) noexcept
      : root_(root), display_(display) {}

    constexpr Error on_char(CharT c) noexcept {
      return dtl::on_char<Cfg>(
        data_, cursor_, size_, display_, command_entered_, c);
    }

    constexpr Error on_backspace(std::size_t n = 1) noexcept {
      return dtl::backspace(data_, cursor_, size_, display_, n);
    }

    constexpr Error on_autocomplete() noexcept {
      CLI_ASSERT(root_.subcommand);

      if (size_ == 0)
        return write(root_.subcommand->name);

      const std::size_t start_of_args = view().find_first_of(
        View<const CharT>{string_constant<CharT, ' ', '=', '('>{}});

      // cant autocomplete if cursor is in the argument portion
      if (start_of_args < cursor_)
        return Error::none;

      const View cmd_name = view().substr(0, start_of_args);

      if (cmd_name.size() == 0)
        return write(root_.subcommand->name);

      const bool char_before_cursor_is_access_separator =
        cursor_ > 0 and data_[cursor_ - 1] == Cfg::access_separator;

      const bool cursor_is_on_access_separator =
        cursor_ < size_ and data_[cursor_] == Cfg::access_separator;

      if (cursor_ == cmd_name.size() and
          not char_before_cursor_is_access_separator) {
        return autocomplete_at_end(cmd_name);
      } else if (char_before_cursor_is_access_separator) {
        return autocomplete_after_access_separator(cmd_name);
      } else if (cursor_is_on_access_separator) {
        return autocomplete_on_access_separator(cmd_name);
      } else {
        return autocomplete_in_middle(cmd_name);
      }
    }

    constexpr Error set_data(View<const CharT> string) noexcept {
      if (string.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      display_.clear_line();

      size_ = 0;
      for (const auto &ch : string) {
        data_[size_++] = ch;
      }
      cursor_ = size_;

      display_.write(view());
      return Error::none;
    }

    constexpr void reset() noexcept {
      command_entered_ = false;
      size_ = 0;
      cursor_ = 0;
    }

    constexpr Error clear() noexcept {
      command_entered_ = false;
      size_ = 0;
      cursor_ = 0;
      display_.clear_line();
      return Error::none;
    }

    constexpr Error on_delete_char() noexcept {
      return dtl::delete_char(data_, cursor_, size_, display_);
    }

    constexpr Error on_cursor_left(std::size_t n) noexcept {
      return dtl::cursor_left(cursor_, display_, n);
    }

    constexpr Error on_cursor_right(std::size_t n) noexcept {
      return dtl::cursor_right(cursor_, size_, display_, n);
    }

    constexpr Error on_clear_line_to_end() noexcept {
      return dtl::clear_line_to_end(cursor_, size_, display_);
    }

    constexpr Error on_clear_line_to_begin() noexcept {
      return dtl::clear_line_to_begin(data_, cursor_, size_, display_);
    }

    constexpr Error on_clear_screen() noexcept {
      command_entered_ = false;
      size_ = 0;
      cursor_ = 0;
      display_.clear_screen();
      return Error::none;
    }

    constexpr View<const CharT> view() const noexcept { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) noexcept {
      Error e = dtl::execute<Cfg>(
        *this, data_, size_, root_, display_, command_entered_, out);
      cursor_ = 0;
      return e;
    }

  private:
    constexpr View<const CharT> get_cursor_name() noexcept {
      // find left border
      std::size_t left = 0;
      for (std::size_t i = cursor_; i < size_; --i) {
        if (data_[i] == Cfg::access_separator) {
          left = i + 1;
          break;
        }
      }
      return {data_ + left, data_ + cursor_};
    }

    constexpr View<const CharT> get_full_cursor_name() noexcept {
      auto is_term_char = [](CharT c) {
        return c == Cfg::access_separator or c == ' ' or c == '(' or c == '=';
      };

      if (cursor_ == size_ or is_term_char(data_[cursor_]))
        return {};

      // find left border
      std::size_t left = cursor_;
      for (std::size_t i = cursor_; i < size_; --i) {
        if (data_[i] == Cfg::access_separator) {
          left = i + 1;
          break;
        }
      }

      std::size_t right = size_;
      for (std::size_t i = cursor_; i < size_; ++i) {
        if (is_term_char(data_[i])) {
          right = i;
          break;
        }
      }
      return {data_ + left, data_ + right};
    }
    constexpr Error write(View<const CharT> string) noexcept {
      return dtl::insert_write<Cfg::max_line_length>(
        data_, cursor_, size_, display_, string);
    }

    constexpr Error autocomplete_at_end(View<const CharT> cmd_name) noexcept {
      const std::size_t last_access_separator =
        cmd_name.find_last_of(Cfg::access_separator);
      const View name = cmd_name.substr(0, last_access_separator);
      const View cmdlet =
        cmd_name.substr(last_access_separator == View<const CharT>::npos
                          ? last_access_separator
                          : last_access_separator + 1);

      if (cmdlet.size() == 0) {
        for (const CommandNode<CharT> &cmd : root_) {
          if (cmd.name == name) {
            if (cmd.subcommand == nullptr)
              return Error::none;
            const CharT c = Cfg::access_separator;
            return write({&c, 1});
          }
          if (cmd.name.starts_with(name)) {
            return write(cmd.name.substr(name.size()));
          }
        }
        return Error::none;
      }

      const CommandNode<CharT> *cmd =
        get_command(name, &root_, Cfg::access_separator);

      if (cmd == nullptr)
        return Error::none;

      if (cmd->subcommand == nullptr)
        return Error::none;

      const CommandNode<CharT> *node = nullptr;
      for (const CommandNode<CharT> &child : *cmd) {
        if (child.name.starts_with(cmdlet)) {
          node = &child;
          break;
        }
      }

      if (node == nullptr)
        return Error::none;

      if (node->name == cmdlet) {
        if (node->next and node->next->name.starts_with(
                             cmdlet)) // take the sibling if it matches
          return write(node->next->name.substr(cmdlet.size()));
        if (node->subcommand) {
          const CharT c{Cfg::access_separator};
          return write({&c, 1});
        }
        return Error::none;
      }
      return write(node->name.substr(cmdlet.size()));
    }

    constexpr Error
    autocomplete_after_access_separator(View<const CharT> cmd_name) noexcept {
      // there is a name under the cursor
      View cursor_name = get_full_cursor_name();
      View name = cmd_name.substr(0, cursor_ - 1);
      const CommandNode<CharT> *cmd =
        get_command(name, &root_, Cfg::access_separator);

      if (cmd == nullptr)
        return Error::none;

      if (cursor_name.size() == 0) {
        if (cmd->subcommand)
          return write(cmd->subcommand->name);
        return Error::none;
      }

      const CommandNode<CharT> *node = nullptr;
      for (const CommandNode<CharT> &child : *cmd) {
        if (child.name.starts_with(cursor_name)) {
          node = &child;
          break;
        }
      }

      if (node == nullptr) // didnt find a match
        return Error::none;

      if (node->name == cursor_name) {
        return on_cursor_right(cursor_name.size());
      }

      return write(node->name.substr(cursor_name.size()));
    }

    constexpr Error
    autocomplete_on_access_separator(View<const CharT> cmd_name) noexcept {
      const CommandNode<CharT> *parent = &root_;
      std::size_t begin = 0;
      std::size_t end = cmd_name.find_first_of(Cfg::access_separator);
      while (end < cursor_) {
        View cmdlet = cmd_name.substr(begin, end);
        bool found = false;
        for (const CommandNode<CharT> &cmd : *parent) {
          if (cmd.name == cmdlet) {
            found = true;
            parent = &cmd;
            break;
          }
        }
        if (not found)
          return Error::none;

        std::size_t next_end =
          cmd_name.find_first_of(Cfg::access_separator, begin);

        if (next_end >= cursor_) {
          break;
        }

        begin = end + 1;
        end = next_end;
      }

      const View cmdlet = cmd_name.substr(begin, cursor_ - begin);

      const CommandNode<CharT> *node = nullptr;
      for (const auto &child : *parent) {
        if (child.name.starts_with(cmdlet)) {
          node = &child;
          break;
        }
      }

      if (node == nullptr)
        return Error::none;

      bool node_name_fully_matches = node->name == cmdlet;

      if (node_name_fully_matches and node->next and
          node->next->name.starts_with(cmdlet))
        return write(node->next->name.substr(cmdlet.size()));

      if (node_name_fully_matches)
        return on_cursor_right(1);

      return write(node->name.substr(cmdlet.size()));
    }

    constexpr Error
    autocomplete_in_middle(View<const CharT> cmd_name) noexcept {
      const CommandNode<CharT> *parent = &root_;
      std::size_t begin = 0;
      std::size_t end = cmd_name.find_first_of(Cfg::access_separator);
      while (end < cursor_) {
        View cmdlet = cmd_name.substr(begin, end);
        bool found = false;
        for (const CommandNode<CharT> &cmd : *parent) {
          if (cmd.name == cmdlet) {
            found = true;
            parent = &cmd;
            break;
          }
        }
        if (not found)
          return Error::none;
        begin = end + 1;
        end = cmd_name.find_first_of(Cfg::access_separator, begin);
      }

      CLI_ASSERT(begin <= cursor_ and end >= cursor_);

      View cmdlet = cmd_name.substr(begin, end - begin);
      View cursor_let = cmd_name.substr(begin, cursor_);
      for (const CommandNode<CharT> &cmd : *parent) {
        if (cmd.name.starts_with(cmdlet)) {
          // the command name matches the cmdlet -> move cursor right and
          // print the remaining characters
          if (Error e = on_cursor_right(end - cursor_); e != Error::none)
            return e;

          return write(cmd.name.substr(cmdlet.size()));
        }
      }

      for (const CommandNode<CharT> &cmd : *parent) {
        if (cmd.name.starts_with(cursor_let)) {
          // only the part up to the cursor matches -> just write the rest of
          // the name
          return write(cmd.name.substr(cursor_let.size()));
        }
      }
      return Error::none;
    }
  };

} // namespace cli

#endif
