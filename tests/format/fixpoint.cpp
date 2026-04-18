
#include "common.hpp"

#include "cli/format.hpp"
#include "fixpoint.hpp"

#include <catch2/catch_all.hpp>

struct FmtFixPointTv {
  unsigned long long bit_pattern;
  std::string output;
};

template<class FP>
void test(const FmtFixPointTv &tv) {
  std::string buffer(256, 0);
  auto res = cli::format::DefaultFormat<FP>{}(
    {buffer.data(), buffer.size()},
    FP(typename FP::raw_value_type(tv.bit_pattern)));
  REQUIRE(res);
  buffer.resize(res.size_written);
  CHECK(tv.output == buffer);
}
template<class FP, auto Precision, bool PrintTrailingZeros>
void test(const FmtFixPointTv &tv) {
  std::string buffer(256, 0);
  auto res = cli::format::FixPoint<FP, Precision, PrintTrailingZeros>{}(
    {buffer.data(), buffer.size()},
    FP(typename FP::raw_value_type(tv.bit_pattern)));
  REQUIRE(res);
  buffer.resize(res.size_written);
  CHECK(tv.output == buffer);
}

TEST_CASE("format::FixPoint fixed<1,1>", "[format][fixpoint]") {
  SECTION("signed") {
    FmtFixPointTv vectors[]{
      {0,    "0.0" },
      {0b11, "-0.5"},
      {0b01, "0.5" },
      {0b10, "-1.0"}
    };
    for (const auto &tv : vectors)
      test<psm::signed_fixed<1, 1>>(tv);
  }

  SECTION("unsigned") {
    FmtFixPointTv vectors[]{
      {0,    "0.0"},
      {0b11, "1.5"},
      {0b01, "0.5"},
      {0b10, "1.0"}
    };
    for (const auto &tv : vectors)
      test<psm::unsigned_fixed<1, 1>>(tv);
  }
}

TEST_CASE("format::FixPoint fixed<16,8>", "[format][fixpoint]") {
  SECTION("signed") {
    FmtFixPointTv vectors[]{
      {0,         "0.0"           },
      {0x7FFFFFu, "32767.99609375"},
      {0xFFFFFFu, "-0.00390625"   },
      {0x000001u, "0.00390625"    },
      {0x800000u, "-32768.0"      },
      {0xFFFF00u, "-1.0"          }
    };
    for (const auto &tv : vectors)
      test<psm::signed_fixed<16, 8>>(tv);
  }

  SECTION("unsigned") {
    FmtFixPointTv vectors[]{
      {0,         "0.0"           },
      {0xFFFFFFu, "65535.99609375"},
      {0x800000u, "32768.0"       },
      {0x000001u, "0.00390625"    },
      {0x100u,    "1.0"           }
    };
    for (const auto &tv : vectors)
      test<psm::unsigned_fixed<16, 8>>(tv);
  }
}

TEST_CASE("format::FixPoint fixed<16,16>", "[format][fixpoint]") {
  SECTION("signed") {
    FmtFixPointTv vectors[]{
      {0,           "0.0"              },
      {0x7fffffffu, "32767.99998474121"},
      {0x7ffffffeu, "32767.99996948242"},
      {0x80000000u, "-32768.0"         },
      {0xFFFF0000u, "-1.0"             },
      {0xFFFFFFFFu, "-0.00001525879"   }
    };
    for (const auto &tv : vectors)
      test<psm::signed_fixed<16, 16>>(tv);
  }

  SECTION("unsigned") {
    FmtFixPointTv vectors[]{
      {0,           "0.0"              },
      {0xFFFFFFFFu, "65535.99998474121"},
      {0x7FFFFFFFu, "32767.99998474121"},
      {0x10000u,    "1.0"              },
      {0x80000000u, "32768.0"          }
    };
    for (const auto &tv : vectors)
      test<psm::unsigned_fixed<16, 16>>(tv);
  }
}

TEST_CASE("format::FixPoint fixed<3,21>", "[format][fixpoint]") {
  SECTION("signed") {
    FmtFixPointTv vectors[]{
      {0,         "0.0"          },
      {0x7FFFFFu, "3.99999952316"},
      {0x800000u, "-4.0"         },
      {0xe00000u, "-1.0"         },
      {0x100u,    "0.00012207031"}
    };
    for (const auto &tv : vectors)
      test<psm::signed_fixed<3, 21>>(tv);
  }

  SECTION("unsigned") {
    FmtFixPointTv vectors[]{
      {0,         "0.0"          },
      {0xffffffu, "7.99999952316"},
      {0x7FFFFFu, "3.99999952316"},
      {0x200000u, "1.0"          },
      {0x100u,    "0.00012207031"}
    };
    for (const auto &tv : vectors)
      test<psm::unsigned_fixed<3, 21>>(tv);
  }
}

TEST_CASE("format::FixPoint fixed<1,1> Precision=4", "[format][fixpoint]") {
  SECTION("signed") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,    "0.0" },
        {0b11, "-0.5"},
        {0b01, "0.5" },
        {0b10, "-1.0"}
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<1, 1>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,    "0.0000" },
        {0b11, "-0.5000"},
        {0b01, "0.5000" },
        {0b10, "-1.0000"}
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<1, 1>, 4, true>(tv);
    }
  }

  SECTION("unsigned") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,    "0.0"},
        {0b11, "1.5"},
        {0b01, "0.5"},
        {0b10, "1.0"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<1, 1>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,    "0.0000"},
        {0b11, "1.5000"},
        {0b01, "0.5000"},
        {0b10, "1.0000"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<1, 1>, 4, true>(tv);
    }
  }
}

TEST_CASE("format::FixPoint fixed<16,8> Precision=4", "[format][fixpoint]") {
  SECTION("signed") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0"       },
        {0x7FFFFFu, "32767.9961"},
        {0xFFFFFFu, "-0.0039"   },
        {0x000001u, "0.0039"    },
        {0x800000u, "-32768.0"  },
        {0xFFFF00u, "-1.0"      }
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<16, 8>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0000"     },
        {0x7FFFFFu, "32767.9961" },
        {0xFFFFFFu, "-0.0039"    },
        {0x000001u, "0.0039"     },
        {0x800000u, "-32768.0000"},
        {0xFFFF00u, "-1.0000"    }
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<16, 8>, 4, true>(tv);
    }
  }

  SECTION("unsigned") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0"       },
        {0xFFFFFFu, "65535.9961"},
        {0x800000u, "32768.0"   },
        {0x000001u, "0.0039"    },
        {0x100u,    "1.0"       }
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<16, 8>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0000"    },
        {0xFFFFFFu, "65535.9961"},
        {0x800000u, "32768.0000"},
        {0x000001u, "0.0039"    },
        {0x100u,    "1.0000"    }
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<16, 8>, 4, true>(tv);
    }
  }
}

TEST_CASE("format::FixPoint fixed<16,16> Precision=4", "[format][fixpoint]") {
  SECTION("signed") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,           "0.0"     },
        {0x7fffffffu, "32768.0" },
        {0x7ffffffeu, "32768.0" },
        {0x80000000u, "-32768.0"},
        {0xFFFF0000u, "-1.0"    },
        {0xFFFFFFFFu, "0.0"     }
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<16, 16>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,           "0.0000"     },
        {0x7fffffffu, "32768.0000" },
        {0x7ffffffeu, "32768.0000" },
        {0x80000000u, "-32768.0000"},
        {0xFFFF0000u, "-1.0000"    },
        {0xFFFFFFFFu, "0.0000"     }
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<16, 16>, 4, true>(tv);
    }
  }

  SECTION("unsigned") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,           "0.0"    },
        {0xFFFFFFFFu, "65536.0"},
        {0x7FFFFFFFu, "32768.0"},
        {0x10000u,    "1.0"    },
        {0x80000000u, "32768.0"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<16, 16>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,           "0.0000"    },
        {0xFFFFFFFFu, "65536.0000"},
        {0x7FFFFFFFu, "32768.0000"},
        {0x10000u,    "1.0000"    },
        {0x80000000u, "32768.0000"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<16, 16>, 4, true>(tv);
    }
  }
}

TEST_CASE("format::FixPoint fixed<3,21> Precision=4", "[format][fixpoint]") {
  SECTION("signed") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0"   },
        {0x7FFFFFu, "4.0"   },
        {0x800000u, "-4.0"  },
        {0xe00000u, "-1.0"  },
        {0x100u,    "0.0001"}
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<3, 21>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0000" },
        {0x7FFFFFu, "4.0000" },
        {0x800000u, "-4.0000"},
        {0xe00000u, "-1.0000"},
        {0x100u,    "0.0001" }
      };
      for (const auto &tv : vectors)
        test<psm::signed_fixed<3, 21>, 4, true>(tv);
    }
  }

  SECTION("unsigned") {
    SECTION("no trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0"   },
        {0xffffffu, "8.0"   },
        {0x7FFFFFu, "4.0"   },
        {0x200000u, "1.0"   },
        {0x100u,    "0.0001"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<3, 21>, 4, false>(tv);
    }
    SECTION("trailing zeros") {
      FmtFixPointTv vectors[]{
        {0,         "0.0000"},
        {0xffffffu, "8.0000"},
        {0x7FFFFFu, "4.0000"},
        {0x200000u, "1.0000"},
        {0x100u,    "0.0001"}
      };
      for (const auto &tv : vectors)
        test<psm::unsigned_fixed<3, 21>, 4, true>(tv);
    }
  }
}
