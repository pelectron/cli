#ifndef CLI_EXEC_RESULT_HPP
#define CLI_EXEC_RESULT_HPP

#include "cli/enums.hpp"
#include "cli/string.hpp"
#include "cli/util.hpp"

#include <cstddef>

namespace cli {

  template<typename CharT>
  class ExecResult {
  public:
    enum Type {
      success,
      parse_error,
      format_error,
      validation_error,
      set_error,
      get_error,
    };

    constexpr ExecResult(const ExecResult &o)
      : type_{o.type_}, error_{o.error_}, result_{} {
      switch (type_) {
        case success:
          result_ = o.result_;
          break;
        case parse_error:
          error_location_ = o.error_location_;
          break;
        case validation_error:
          index_ = o.index_;
        default:
          break;
      }
    }

    constexpr ExecResult(ExecResult &&o)
      : type_{o.type_}, error_{o.error_}, result_{} {
      switch (type_) {
        case success:
          result_ = o.result_;
          break;
        case parse_error:
          error_location_ = o.error_location_;
          break;
        case validation_error:
          index_ = o.index_;
          break;
        case format_error:
          [[fallthrough]];
        case set_error:
          [[fallthrough]];
        case get_error:
          [[fallthrough]];
        default:
          break;
      }
    }

    static constexpr ExecResult make_success() { return {}; }

    static constexpr ExecResult make_success(View<const CharT> result) {
      ExecResult r;
      r.result_ = result;
      return r;
    }

    static constexpr ExecResult make_parse_error(Error error,
                                                 const CharT *location) {
      ExecResult r;
      r.type_ = parse_error;
      r.error_ = error;
      r.error_location_ = location;
      return r;
    }

    static constexpr ExecResult make_set_error(Error error) {
      ExecResult r;
      r.type_ = set_error;
      r.error_ = error;
      return r;
    }

    static constexpr ExecResult make_get_error(Error error) {
      ExecResult r;
      r.type_ = get_error;
      r.error_ = error;
      return r;
    }

    static constexpr ExecResult make_format_error(Error error) {
      ExecResult r;
      r.type_ = format_error;
      r.error_ = error;
      return r;
    }

    static constexpr ExecResult make_validation_error(std::size_t index) {
      ExecResult r;
      r.type_ = validation_error;
      r.error_ = Error::invalid_value;
      r.index_ = index;
      return r;
    }

    constexpr operator bool() const { return type_ == success; }

    constexpr Type type() const { return type_; }

    constexpr View<const CharT> result() const {
      CLI_ASSERT(type_ == success);
      return result_;
    }

    constexpr const CharT *error_location() const {
      CLI_ASSERT(type_ == parse_error);
      return error_location_;
    }

    constexpr Error error() const { return error_; }

    constexpr std::size_t index() const {
      CLI_ASSERT(type_ == validation_error);
      return index_;
    }

  private:
    constexpr ExecResult()
      : result_{} {}
    Type type_{success};
    // the error that occured, if any
    Error error_{Error::none};
    union {
      // the resulting string to write if success
      View<const CharT> result_;
      // contains the error location if parse_error
      const CharT *error_location_;
      // the index of the argument that failed to validate
      std::size_t index_;
    };
  };
} // namespace cli

#endif
