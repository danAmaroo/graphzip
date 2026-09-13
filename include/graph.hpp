#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

class Graph {
  std::vector<uint32_t> targets;
  std::vector<uint32_t> offsets;

public:
  static Graph load(const std::string &, uint32_t);

  std::span<const uint32_t> neighbours(uint32_t node) const;
  uint32_t node_count() const;
  uint64_t edge_count() const;
  uint64_t memory_bytes() const;
};

Graph load_graph(const std::string &path, uint32_t n);
uint32_t bfs(const Graph &, uint32_t from, uint32_t to);