#include <./sqlparse.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <vector>

TEST_CASE("splits a simple tuple", "[sqlparse]") {
  std::vector<std::string_view> out;
  std::string_view line = "(10,0,'Anarchism',1)";

  size_t end = scan_tuple(line, 0, out);

  REQUIRE(out.size() == 4);
  REQUIRE(out[0] == "10");
  REQUIRE(out[1] == "0");
  REQUIRE(out[2] == "'Anarchism'");
  REQUIRE(out[3] == "1");
  REQUIRE(end == line.size());
}

TEST_CASE("commas inside strings are not separators", "[sqlparse]") {
  std::vector<std::string_view> out;
  std::string_view line = "(1,2,'a,b')";

  scan_tuple(line, 0, out);

  REQUIRE(out.size() == 3);
  REQUIRE(out[2] == "'a,b'");
}
