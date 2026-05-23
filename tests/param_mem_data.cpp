#include "cli/ctti.hpp"
#include "cli/param.hpp"
#include "cli/util.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using cli::operator""_sc;
using cli::params::param;

struct Struct {
  int i = 42;
};

static constinit Struct s{};

static_assert(cli::ctti::dtl::num_members<Struct>() == 1);

static constinit auto p =
  param("s"_sc, "s desc"_sc, s, param("i"_sc, &Struct::i));

static constinit auto const_p =
  param("s"_sc, "s desc"_sc, cli::as_const(s), param("i"_sc, &Struct::i));

static constinit auto tp = param("s"_sc, "s desc"_sc, s, param<&Struct::i>());

static constinit auto const_tp =
  param("s"_sc, "s desc"_sc, cli::as_const(s), param<&Struct::i>());

TEST_CASE("param(mem_data)", "[param]") {

  char buffer[10]{};

  using cli::get;
  auto exec_result = p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "{ i = 42 }");

  exec_result = p.execute("={i=21}", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(s.i == 21);

  auto &pi = get<0>(p);
  exec_result = pi.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  exec_result = pi.execute("=42", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(s.i == 42);

  exec_result = const_p.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "{ i = 42 }");

  auto &const_pi = get<0>(const_p);
  exec_result = const_pi.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  exec_result = const_p.execute(" = { i = 21 }", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  exec_result = const_pi.execute(" = 21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);
}

TEST_CASE("param<mem_data>()", "[param]") {

  char buffer[10]{};

  using cli::get;
  auto exec_result = tp.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "{ i = 42 }");

  exec_result = tp.execute("={i=21}", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(s.i == 21);

  auto &pi = get<0>(tp);
  exec_result = pi.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "21");

  exec_result = pi.execute("=42", {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(s.i == 42);

  exec_result = const_tp.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "{ i = 42 }");

  auto &const_pi = get<0>(const_tp);
  exec_result = const_pi.execute({}, {buffer, 10});
  REQUIRE(exec_result);
  REQUIRE(exec_result.result() == "42");

  exec_result = const_tp.execute(" = { i = 21 }", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);

  exec_result = const_pi.execute(" = 21", {buffer, 10});
  REQUIRE_FALSE(exec_result);
  REQUIRE(exec_result.type() == cli::ExecResult<char>::set_error);
  REQUIRE(exec_result.error() == cli::Error::cant_set_param);
}
