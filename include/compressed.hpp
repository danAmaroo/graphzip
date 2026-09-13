#pragma once

#include "graph.hpp"

#include <cstdint>
#include <vector>

class CompressedGraph {
  std::vector<uint8_t> bits;
  std::vector<uint64_t> node_offsets; // size n+1, in bits
  uint64_t edges = 0;

public:
  static CompressedGraph build(const Graph &g);

  void neighbours(uint32_t node, std::vector<uint32_t> &out) const;
  uint32_t node_count() const;
  uint64_t edge_count() const;
  uint64_t memory_bytes() const;
};
