#ifndef CLI_CURSOR_HPP
#define CLI_CURSOR_HPP

#include "cli/command.hpp"
#include "cli/concepts.hpp"
#include "cli/enums.hpp"
#include "cli/string.hpp"

#include <cstddef>
#include <cstdint>

namespace cli {

  namespace dtl {
    template<typename CharT, typename Line, concepts::Display<CharT> Display>
    constexpr Error print_error(Line &line, Display &display, Error e) {
      Error err = display.newline();
      if (err != Error::none)
        return err;

      err = display.write(ctti::enum_name<Error, CharT>(e));
      if (err != Error::none)
        return err;

      line.reset();
      return e;
    }
  } // namespace dtl

  /**
   * @brief The Line class is used to handle autocomplete, cursor movement,
   * storing received characters, and displaying the characters.
   *
   * @tparam Cfg
   */
  template<typename Cfg, concepts::Display<typename Cfg::char_type> Display>
  class Line;

  template<typename Cfg,
           concepts::DisplayWithoutCursor<typename Cfg::char_type> Display>
    requires((not Cfg::use_cursor) and (not Cfg::use_autocomplete))
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    CharT data_[Cfg::max_line_length]{};
    std::size_t size_{};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display)
      : root_(root), display_(display) {}

    constexpr Error on_char(CharT c) {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0) {
        const Error e = display_.newline();
        if (e != Error::none)
          return e;
      }

      data_[size_++] = c;
      return display_.write(c);
    }

    constexpr Error on_backspace(std::size_t n = 1) {
      if (n >= size_) {
        size_ = 0;
        return display_.clear_line();
      } else {
        size_ -= n;
        return display_.backspace(n);
      }
    }

    constexpr Error on_autocomplete() { return Error::none; }

    constexpr Error set_data(View<const CharT> s) {
      if (s.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      Error e = display_.clear_line();
      if (e != Error::none or s.size() == 0)
        return e;

      size_ = 0;
      for (const auto &ch : s) {
        data_[size_++] = ch;
      }

      return display_.write(view());
    }

    constexpr Error clear() {
      size_ = 0;
      return display_.clear_line();
    }

    constexpr Error on_delete_char() { return Error::none; }

    constexpr Error on_cursor_left(uint32_t n) { return Error::none; }

    constexpr Error on_cursor_right(uint32_t n) { return Error::none; }

    constexpr Error on_clear_line_to_end() { return Error::none; }

    constexpr Error on_clear_line_to_begin() { return clear(); }

    constexpr Error on_clear_screen() {
      size_ = 0;
      return display_.clear_screen();
    }

    constexpr View<const CharT> view() const { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) {
      // parse the input
      SplitResult res = split_line(view(), &root_, Cfg::access_separator);
      if (res.command == nullptr)
        return dtl::print_error<CharT>(*this, display_, Error::invalid_cmd);

      // execute the command
      bool should_print_newline = false;
      Error e = res.command->execute(res.args, out, should_print_newline);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      if (should_print_newline) {
        e = display_.newline();
        if (e != Error::none or size_ == 0)
          return e;
      }
      // print the result
      e = display_.write(out);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      // reset line data
      size_ = 0;
      return Error::none;
    }

    constexpr void reset() { size_ = 0; }
  };

  template<typename Cfg,
           concepts::DisplayWithoutCursor<typename Cfg::char_type> Display>
    requires((not Cfg::use_cursor) and Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    CharT data_[Cfg::max_line_length]{};
    std::size_t size_{0};
    std::size_t start_of_args_{View<const CharT>::npos};
    std::size_t last_access_separator_ = View<const CharT>::npos;
    const CommandNode<CharT> *command_{};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display)
      : command_(&root), root_(root), display_(display) {}

    constexpr Error on_char(CharT c) {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0) {
        const Error e = display_.newline();
        if (e != Error::none)
          return e;
      }

      if (start_of_args_ < size_) {
        data_[size_++] = c;
        return display_.write(c);
      }

      switch (c) {
        case Cfg::access_separator:
          if (command_->subcommand == nullptr)
            return Error::none;

          last_access_separator_ = size_;
          data_[size_++] = c;
          return display_.write(c);
        case ' ':
          [[fallthrough]];
        case '=':
          [[fallthrough]];
        case '(':
          start_of_args_ = size_;
          data_[size_++] = c;
          return display_.write(c);
        default: {
          data_[size_++] = c;
          if (size_ == 1) {
            for (const auto &cmd : root_) {
              if (cmd.name[0] == c) {
                command_ = &cmd;
                return display_.write(c);
              }
            }
            --size_;
            return Error::none;
          }

          if (last_access_separator_ == size_ - 2) {
            for (const auto &cmd : *command_) {
              if (cmd.name[0] == c) {
                command_ = &cmd;
                return display_.write(c);
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
          return display_.write(c);
        }
      }
    }

    constexpr void reset() {
      size_ = 0;
      command_ = &root_;
      start_of_args_ = View<const CharT>::npos;
      last_access_separator_ = View<const CharT>::npos;
    }

    constexpr Error clear() {
      reset();
      return display_.clear_line();
    }

    constexpr Error on_backspace(std::size_t n = 1) {
      if (n >= size_) {
        return clear();
      }
      size_ -= n;
      if (size_ > start_of_args_) {
        return display_.backspace(n);
      }

      for (std::size_t i = start_of_args_ - 1; i >= size_; --i) {
        if (data_[i] == Cfg::access_separator) {
          command_ = command_->parent;
        }
      }
      start_of_args_ = View<const CharT>::npos;
      last_access_separator_ = view().find_last_of(Cfg::access_separator);
      return display_.backspace(n);
    }

    constexpr Error on_autocomplete() {
      if (start_of_args_ < size_)
        return Error::none;

      const auto line = view();
      if (size_ == 0 or last_access_separator_ == size_ - 1) {
        if (command_->subcommand == nullptr)
          return Error::none;
        command_ = command_->subcommand;
        const View autocomplete_string = command_->name;
        for (const auto &ch : autocomplete_string) {
          data_[size_++] = ch;
        }
        return display_.write(autocomplete_string);
      }

      const View autocomplete_string =
        command_->name.substr(size_ - last_access_separator_ - 1);

      if (autocomplete_string.size() == 0) {
        return on_char(Cfg::access_separator);
      }

      for (const CharT &ch : autocomplete_string) {
        data_[size_++] = ch;
      }

      return display_.write(autocomplete_string);
    }

    constexpr Error set_data(View<const CharT> s) {
      if (s.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      Error e = clear();
      if (e != Error::none or s.size() == 0) {
        return e;
      }

      const std::size_t arg_start = s.find_first_of(
        View<const CharT>{string_constant<CharT, ' ', '(', '='>{}});

      View<const CharT> cmd_name;
      if (arg_start == View<const CharT>::npos) {
        cmd_name = s;
      } else {
        cmd_name = s.substr(0, arg_start);
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
        const View s = cmd_name.substr(0, end);
        bool found = false;
        for (const CommandNode<CharT> &child : *parent) {
          if (child.name == s) {
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

      start_of_args_ = arg_start;
      last_access_separator_ =
        s.substr(0, arg_start).find_last_of(Cfg::access_separator);
      size_ = 0;
      for (const auto &ch : s) {
        data_[size_++] = ch;
      }

      return display_.write(s);
    }

    constexpr Error on_delete_char() { return Error::none; }

    constexpr Error on_cursor_left(uint32_t n) { return Error::none; }

    constexpr Error on_cursor_right(uint32_t n) { return Error::none; }

    constexpr Error on_clear_line_to_end(uint32_t n) { return Error::none; }

    constexpr Error on_clear_line_to_begin() { return clear(); }

    constexpr Error on_clear_screen() {
      size_ = 0;
      command_ = &root_;
      start_of_args_ = View<const CharT>::npos;
      last_access_separator_ = View<const CharT>::npos;
      return display_.clear_screen();
    }

    constexpr View<const CharT> view() const { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) {
      // execute the command
      bool should_print_newline = false;
      Error e = command_->execute(
        view().substr(start_of_args_), out, should_print_newline);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      if (should_print_newline) {
        e = display_.newline();
        if (e != Error::none or size_ == 0)
          return e;
      }

      // print the result
      e = display_.write(out);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      // reset line data
      size_ = 0;
      start_of_args_ = View<const CharT>::npos;
      last_access_separator_ = View<const CharT>::npos;
      command_ = &root_;
      return Error::none;
    }
  };

  template<typename Cfg,
           concepts::DisplayWithCursor<typename Cfg::char_type> Display>
    requires(Cfg::use_cursor and not Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    CharT data_[Cfg::max_line_length]{};
    std::size_t size_{};
    std::size_t cursor_{};
    const CommandNode<CharT> &root_;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display)
      : root_(root), display_(display) {}

    constexpr Error on_char(CharT c) {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0) {
        const Error e = display_.newline();
        if (e != Error::none)
          return e;
      }

      if (cursor_ == size_) {
        data_[size_++] = c;
        ++cursor_;
        return display_.write(c);
      }

      for (std::size_t i = size_ - 1; i < size_ and i >= cursor_; --i) {
        data_[i + 1] = data_[i];
      }

      data_[cursor_++] = c;
      ++size_;

      Error e = display_.clear_line_to_end();
      if (e != Error::none)
        return e;

      e = display_.write({data_ + cursor_ - 1, data_ + size_});
      if (e != Error::none)
        return e;

      return display_.cursor_left(size_ - cursor_);
    }

    constexpr Error on_backspace(std::size_t n = 1) {
      if (cursor_ == 0)
        return Error::none;
      else if (cursor_ == size_) {
        if (n >= size_) {
          size_ = 0;
          cursor_ = 0;
          return display_.clear_line();
        } else {
          size_ -= n;
          cursor_ -= n;
          return display_.backspace(n);
        }
      } else if (n >= cursor_) {
        for (std::size_t i = cursor_; i < size_; ++i) {
          data_[i - cursor_] = data_[i];
        }
        size_ -= cursor_;
        cursor_ = 0;

        Error e = display_.clear_line();
        if (e != Error::none)
          return e;

        e = display_.write({data_, size_});
        if (e != Error::none)
          return e;
        return display_.cursor_left(size_);
      } else {
        for (std::size_t i = cursor_ - n; i < size_ - n; ++i) {
          data_[i] = data_[i + n];
        }
        cursor_ -= n;
        size_ -= n;

        Error e = display_.cursor_left(n);
        if (e != Error::none)
          return e;

        e = display_.clear_line_to_end();
        if (e != Error::none)
          return e;

        e = display_.write({data_ + cursor_, data_ + size_});
        if (e != Error::none)
          return e;

        return display_.cursor_left(size_ - cursor_);
      }
    }

    constexpr View<const CharT> on_autocomplete() { return {}; }

    constexpr Error set_data(View<const CharT> s) {
      if (s.size() > Cfg::max_line_length)
        return Error::buffer_overflow;

      Error e = clear();
      if (e != Error::none or s.size() == 0)
        return e;

      for (const auto &ch : s) {
        data_[size_++] = ch;
      }
      cursor_ = size_;
      return display_.write(view());
    }

    constexpr void reset() {
      size_ = 0;
      cursor_ = 0;
    }

    constexpr Error clear() {
      reset();
      return display_.clear_line();
    }

    constexpr Error on_delete_char() {
      if (cursor_ == size_)
        return Error::none;

      for (std::size_t i = cursor_; (i + 1) < size_; ++i) {
        data_[i] = data_[i + 1];
      }

      --size_;
      Error e = display_.clear_line_to_end();
      if (e != Error::none)
        return e;

      e = display_.write({data_ + cursor_, size_ - cursor_});
      if (e != Error::none)
        return e;

      return display_.cursor_left(size_ - cursor_);
    }

    constexpr Error on_cursor_left(uint32_t n) {
      if (cursor_ == 0)
        return Error::none;
      if (n >= cursor_)
        n = cursor_;
      cursor_ -= n;
      return display_.cursor_left(n);
    }

    constexpr Error on_cursor_right(uint32_t n) {
      if (cursor_ == size_)
        return Error::none;
      if (n + cursor_ > size_)
        n = size_ - cursor_;
      cursor_ += n;
      return display_.cursor_left(n);
    }

    constexpr Error on_clear_line_to_end() {
      return display_.clear_line_to_end();
    }

    constexpr Error on_clear_line_to_begin() {
      return display_.clear_line_to_begin();
    }

    constexpr Error on_clear_screen() {
      size_ = 0;
      cursor_ = 0;
      return display_.clear_screen();
    }

    constexpr View<const CharT> view() const { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) {
      // parse the input
      SplitResult res = split_line(view(), &root_, Cfg::access_separator);
      if (res.command == nullptr)
        return dtl::print_error<CharT>(*this, display_, Error::invalid_cmd);

      // execute the command
      bool should_print_newline = false;
      Error e = res.command->execute(res.args, out, should_print_newline);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      if (should_print_newline) {
        e = display_.newline();
        if (e != Error::none or size_ == 0)
          return e;
      }

      // print the result
      e = display_.write(out);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      // reset line data
      size_ = 0;
      cursor_ = 0;
      return Error::none;
    }
  };

  template<typename Cfg,
           concepts::DisplayWithCursor<typename Cfg::char_type> Display>
    requires(Cfg::use_cursor and Cfg::use_autocomplete)
  class Line<Cfg, Display> {
    using CharT = typename Cfg::char_type;
    CharT data_[Cfg::max_line_length]{};
    std::size_t size_{};
    std::size_t cursor_{};
    const CommandNode<CharT> &root;
    Display &display_;

  public:
    constexpr Line(const CommandNode<CharT> &root, Display &display)
      : root(root), display_(display) {}

    constexpr Error on_char(CharT c) {
      if (size_ == Cfg::max_line_length)
        return Error::buffer_overflow;

      if (size_ == 0 and c == Cfg::access_separator) {
        return Error::none;
      }

      if (size_ == 0) {
        const Error e = display_.newline();
        if (e != Error::none)
          return e;
      }

      return write({&c, 1});
    }

    constexpr Error on_backspace(std::size_t n = 1) {
      if (cursor_ == 0)
        return Error::none;
      else if (cursor_ == size_) {
        if (n >= size_) {
          size_ = 0;
          cursor_ = 0;
          return display_.clear_line();
        } else {
          size_ -= n;
          cursor_ -= n;
          return display_.backspace(n);
        }
      } else if (n >= cursor_) {
        for (std::size_t i = cursor_; i < size_; ++i) {
          data_[i - cursor_] = data_[i];
        }
        size_ -= cursor_;
        cursor_ = 0;

        Error e = display_.clear_line();
        if (e != Error::none)
          return e;

        e = display_.write({data_, size_});
        if (e != Error::none)
          return e;
        return display_.cursor_left(size_);
      } else {
        for (std::size_t i = cursor_ - n; i < size_ - n; ++i) {
          data_[i] = data_[i + n];
        }
        cursor_ -= n;
        size_ -= n;

        Error e = display_.cursor_left(n);
        if (e != Error::none)
          return e;

        e = display_.clear_line_to_end();
        if (e != Error::none)
          return e;

        e = display_.write({data_ + cursor_, data_ + size_});
        if (e != Error::none)
          return e;

        return display_.cursor_left(size_ - cursor_);
      }
    }

    constexpr Error write(View<const CharT> s) {
      if (s.size() == 0)
        return Error::none;

      if (s.size() + size_ > Cfg::max_line_length)
        return Error::buffer_overflow;

      if (cursor_ == size_ or size_ == 0) {
        for (const CharT &ch : s)
          data_[size_++] = ch;
        cursor_ = size_;
        return display_.write(s);
      }

      // copy data to the back
      for (std::size_t i = size_ - 1; i >= cursor_ and i < size_; --i) {
        data_[i + s.size()] = data_[i];
      }

      // copy s into data_
      for (std::size_t i = 0; i < s.size(); ++i) {
        data_[cursor_ + i] = s[i];
      }

      // update size and cursor
      const std::size_t old_cursor = cursor_;
      size_ += s.size();
      cursor_ += s.size();

      // write new data
      Error e = display_.write({data_ + old_cursor, data_ + size_});
      if (e != Error::none)
        return e;

      // adjust cursor
      return display_.cursor_left(size_ - cursor_);
    }

    constexpr View<const CharT> get_cursor_name() {
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

    constexpr View<const CharT> get_full_cursor_name() {
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

    constexpr Error on_autocomplete() {
      if (not root.subcommand)
        return Error::none;

      if (size_ == 0)
        return write(root.subcommand->name);

      const std::size_t start_of_args = view().find_first_of(
        View<const CharT>{string_constant<CharT, ' ', '=', '('>{}});

      // cant autocomplete if cursor is in the argument portion
      if (start_of_args < cursor_)
        return Error::none;

      const View cmd_name = view().substr(0, start_of_args);

      if (cmd_name.size() == 0)
        return write(root.subcommand->name);

      // the following cases:
      // - cursor is at the end of cmd_name -> normal autocomplete
      //   - if the full name matches and sibling also starts with name -> take
      //     sibling
      //   - else take cmd
      // - the char before cursor is access separator
      //   - if there is a name following cursor, and it matches a command, then
      //     simply move cursor right
      //   - else normal autocomplete
      // - char on cursor is access separator ->
      //   if there is a sibling that also starts with the same name, then
      //   autocomplete the sibling, else move cursor to the right by one
      // - cursor is in the middle of a name ->
      //   - if the full name matches, just move cursor to the right
      //   - if the full name doesnt match, check the siblings and if they match
      //     take the sibling
      //   - if the full name doesnt match cmd or siblings, then autocomplete
      //     with cmd

      const bool char_before_cursor_is_access_separator =
        cursor_ > 0 and data_[cursor_ - 1] == Cfg::access_separator;

      const bool cursor_is_on_access_separator =
        cursor_ < size_ and data_[cursor_] == Cfg::access_separator;

      if (cursor_ == cmd_name.size() and
          not char_before_cursor_is_access_separator) {
        const std::size_t last_access_separator =
          cmd_name.find_last_of(Cfg::access_separator);
        const View name = cmd_name.substr(0, last_access_separator);
        const View cmdlet =
          cmd_name.substr(last_access_separator == View<const CharT>::npos
                            ? last_access_separator
                            : last_access_separator + 1);

        if (cmdlet.size() == 0) {
          for (const CommandNode<CharT> &cmd : root) {
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
          get_command(name, &root, Cfg::access_separator);

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
      } else if (char_before_cursor_is_access_separator) {
        // there is a name under the cursor
        View cursor_name = get_full_cursor_name();
        View name = cmd_name.substr(0, cursor_ - 1);
        const CommandNode<CharT> *cmd =
          get_command(name, &root, Cfg::access_separator);

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
      } else if (cursor_is_on_access_separator) {
        const CommandNode<CharT> *parent = &root;
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
      } else {
        const CommandNode<CharT> *parent = &root;
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

        assert(begin <= cursor_ and end >= cursor_);

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
          if (cmd.name.starts_with(cursor_let)) {
            // only the part up to the cursor matches -> just write the rest of
            // the name
            return write(cmd.name.substr(cursor_let.size()));
          }
        }
        return Error::none;
      }
      return Error::none;
    }
    constexpr Error set_data(View<const CharT> s) {
      Error e = display_.clear_line();
      if (e != Error::none or s.size() == 0)
        return e;

      size_ = 0;
      for (const auto &ch : s) {
        data_[size_++] = ch;
      }
      cursor_ = size_;
      return display_.write(view());
    }

    constexpr void reset() {
      size_ = 0;
      cursor_ = 0;
    }

    constexpr Error clear() {
      size_ = 0;
      cursor_ = 0;
      return display_.clear_line();
    }

    constexpr Error on_delete_char() {
      if (cursor_ == size_)
        return Error::none;

      for (std::size_t i = cursor_; (i + 1) < size_; ++i) {
        data_[i] = data_[i + 1];
      }

      --size_;
      Error e = display_.clear_line_to_end();
      if (e != Error::none)
        return e;

      e = display_.write({data_ + cursor_, size_ - cursor_});
      if (e != Error::none)
        return e;

      return display_.cursor_left(size_ - cursor_);
    }

    constexpr Error on_cursor_left(uint32_t n) {
      if (cursor_ == 0)
        return Error::none;
      if (n >= cursor_)
        n = cursor_;
      cursor_ -= n;
      return display_.cursor_left(n);
    }

    constexpr Error on_cursor_right(uint32_t n) {
      if (cursor_ == size_)
        return Error::none;
      if (n + cursor_ > size_)
        n = size_ - cursor_;
      cursor_ += n;
      return display_.cursor_right(n);
    }

    constexpr Error on_clear_line_to_end() {
      return display_.clear_line_to_end();
    }

    constexpr Error on_clear_line_to_begin() {
      return display_.clear_line_to_begin();
    }

    constexpr Error on_clear_screen() {
      size_ = 0;
      cursor_ = 0;
      return display_.clear_screen();
    }

    constexpr View<const CharT> view() const { return {data_, size_}; }

    constexpr Error execute(View<CharT> &out) {
      // parse the input
      SplitResult res = split_line(view(), &root, Cfg::access_separator);
      if (res.command == nullptr)
        return dtl::print_error<CharT>(*this, display_, Error::invalid_cmd);

      // execute the command
      bool should_print_newline = false;
      Error e = res.command->execute(res.args, out, should_print_newline);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      if (should_print_newline) {
        Error e = display_.newline();
        if (e != Error::none or size_ == 0)
          return e;
      }

      // print the result
      e = display_.write(out);
      if (e != Error::none)
        return dtl::print_error<CharT>(*this, display_, e);

      // reset line data
      reset();
      return Error::none;
    }
  };

} // namespace cli

#endif
