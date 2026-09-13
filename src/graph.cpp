#include "graph.hpp"
#include "dumps.hpp"

#include <cstdint>
#include <string>
#include <vector>

std::span<const uint32_t> Graph::neighbours(uint32_t node) const {
  return {targets.data() + offsets[node], offsets[node + 1] - offsets[node]};
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

uint32_t bfs(const Graph &g, uint32_t from, uint32_t to) {
  std::vector<uint32_t> dist(g.node_count(), UINT32_MAX);
  std::vector<uint32_t> queue;
  size_t head = 0;
  dist[from] = 0;
  queue.push_back(from);
  if (from == to)
    return 0;
  while (head < queue.size()) {
    uint32_t node = queue[head++];
    for (uint32_t nb : g.neighbours(node)) {
      if (dist[nb] != UINT32_MAX)
        continue;
      dist[nb] = dist[node] + 1;
      if (nb == to)
        return dist[nb];
      queue.push_back(nb);
    }
  }
  return UINT32_MAX;
}