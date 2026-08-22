#include "bitstream.hpp"
#include <algorithm>
#include <cstdint>

namespace {
uint64_t mask(int n) { return (n == 64) ? ~0ULL : ((1ULL << n) - 1); }
} // namespace

void BitWriter::write_bits(uint64_t val, int n) {
  total += n;
  while (n > 0) {
    int take = std::min(8 - occupiedBits, n);
    // extract top take of bits
    uint64_t chunk = (val >> (n - take)) & mask(take);
    // add these to currByte
    currByte |= chunk << ((8 - occupiedBits) - take);
    occupiedBits += take;
    n -= take;
    if (occupiedBits >= 8) {
      completedBytes.push_back(currByte);
      currByte = 0;
      occupiedBits = 0;
    }
  }
}

void BitWriter::flush() {
  if (occupiedBits > 0) {
    completedBytes.push_back(currByte);
    currByte = 0;
    occupiedBits = 0;
  }
}

const std::vector<uint8_t> &BitWriter::bytes() const { return completedBytes; }

uint64_t BitWriter::bits_written() const { return total; }

BitReader::BitReader(std::span<const uint8_t> data) : bytes(data) {}

uint64_t BitReader::read_bits(int n) {
  uint64_t result = 0;
  while (n > 0) {
    int take = std::min(8 - bitsConsumed, n);
    uint64_t chunk =
        (bytes[byteIndex] >> ((8 - bitsConsumed) - take)) & mask(take);
    result = (result << take) | chunk;
    bitsConsumed += take;
    n -= take;
    if (bitsConsumed == 8) {
      byteIndex++;
      bitsConsumed = 0;
    }
  }
  return result;
}

void BitReader::seek(uint64_t bit_pos) {
  byteIndex = bit_pos / 8;
  bitsConsumed = bit_pos % 8;
}