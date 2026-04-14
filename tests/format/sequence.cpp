#include "cli/format.hpp"
#include "cli/vector.hpp"

#include <catch2/catch_all.hpp>
#include <string>

using Seq1 = cli::FixedCapacityVector<int, 10>;

using Seq2 = std::vector<int>;
template <class Seq> struct FmtSeqTestVector {
  Seq input;
  std::string str;
  std::string buffer;
};

#define TV1(...)                                                               \
  FmtSeqTestVector<TestType> {                                                 \
    TestType{__VA_ARGS__}, std::string("[" #__VA_ARGS__ "]"),                  \
        std::string(256, 0)                                                    \
  }

TEMPLATE_TEST_CASE("format::Sequence", "[format][sequence]", Seq1, Seq2) {

  FmtSeqTestVector<TestType> vectors[]{TV1(), TV1(1), TV1(1, 2),
                                       TV1(1, 2, 3, 4, 5, 6, 7, 8),
                                       TV1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)};

  for (auto &tv : vectors) {
    auto res = cli::format::DefaultFormat<TestType, char>{}(
        {tv.buffer.data(), tv.buffer.size()}, tv.input);
    CHECK(res);
    CHECK(res.size_written == tv.str.size());
    tv.buffer.resize(res.size_written);
    CHECK(tv.str == tv.buffer);
  }
}
