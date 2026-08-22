#include <./bitstream.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <random>
#include <vector>

namespace {
uint64_t mask(int n) { return (n == 64) ? ~0ULL : ((1ULL << n) - 1); }
} // namespace

TEST_CASE("single bits pack MSB-first", "[bitstream]") {
  BitWriter w;
  const int bits[] = {1, 0, 1, 1, 0, 0, 1, 0};
  for (int b : bits) {
    w.write_bits(b, 1);
  }
  w.flush();

  REQUIRE(w.bytes().size() == 1);
  REQUIRE(w.bytes()[0] == 0b10110010);
}

TEST_CASE("round-trips 10000 random values", "[bitstream]") {
  std::mt19937_64 rng(20260820);
  std::uniform_int_distribution<int> width_dist(1, 64);

  std::vector<std::pair<uint64_t, int>> items;
  for (int i = 0; i < 10000; ++i) {
    const int width = width_dist(rng);
    items.emplace_back(rng() & mask(width), width);
  }

  BitWriter w;
  for (const auto &[value, width] : items) {
    w.write_bits(value, width);
  }
  w.flush();

  BitReader r(w.bytes());
  for (size_t i = 0; i < items.size(); ++i) {
    const auto &[value, width] = items[i];
    INFO("index = " << i << ", width = " << width << ", expected = " << value);
    REQUIRE(r.read_bits(width) == value);
  }
}

TEST_CASE("gamma round-trips 0 to 1000", "[gamma]") {
  BitWriter w;
  for (uint64_t x = 0; x <= 1000; ++x) {
    write_gamma(w, x);
  }
  w.flush();

  BitReader r(w.bytes());
  for (uint64_t x = 0; x <= 1000; ++x) {
    INFO("x = " << x);
    REQUIRE(read_gamma(r) == x);
  }
}

TEST_CASE("gamma encodes 12 in 7 bits", "[gamma]") {
  BitWriter w;
  write_gamma(w, 12);
  REQUIRE(w.bits_written() == 7);
}