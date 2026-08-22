#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// MSB First

class BitWriter {
  std::vector<uint8_t> completedBytes;
  uint8_t currByte = 0;
  int occupiedBits = 0;
  uint64_t total = 0;

public:
  void write_bits(uint64_t val, int n);
  void flush();
  const std::vector<uint8_t> &bytes() const;
  uint64_t bits_written() const;
};

class BitReader {
  std::span<const uint8_t> bytes;
  size_t byteIndex = 0;
  int bitsConsumed = 0;

public:
  explicit BitReader(std::span<const uint8_t> data);
  uint64_t read_bits(int n);
  void seek(uint64_t bit_pos);
};

void write_gamma(BitWriter &w, uint64_t x);
uint64_t read_gamma(BitReader &r);