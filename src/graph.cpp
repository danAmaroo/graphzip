#include "graph.hpp"
#include "dumps.hpp"

#include <cstdint>
#include <string>
#include <vector>

void Graph::neighbours(uint32_t node, std::vector<uint32_t> &out) const {
  out.assign(targets.begin() + offsets[node],
             targets.begin() + offsets[node + 1]);
}

uint32_t Graph::node_count() const { return offsets.size() - 1; };
uint64_t Graph::edge_count() const { return targets.size(); };
uint64_t Graph::memory_bytes() const {
  return sizeof(uint32_t) * (targets.size() + offsets.size());
};

Graph Graph::load(const std::string &path, uint32_t n) {
  auto flat = read_u32_file(path);
  Graph g;
  g.offsets.assign(n + 1, 0);
  for (size_t i = 0; i < flat.size(); i += 2) {
    g.offsets[flat[i] + 1]++;
  }
  for (uint32_t i = 0; i < n; ++i) {
    g.offsets[i + 1] += g.offsets[i];
  }
  g.targets.resize(flat.size() / 2);
  std::vector<uint32_t> cursor(g.offsets.begin(), g.offsets.end() - 1);
  for (size_t i = 0; i + 1 < flat.size(); i += 2) {
    g.targets[cursor[flat[i]]++] = flat[i + 1];
  }
  return g;
};
