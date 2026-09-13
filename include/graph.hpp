#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

class Graph {
  std::vector<uint32_t> targets;
  std::vector<uint32_t> offsets;

public:
  static Graph load(const std::string &, uint32_t);

  void neighbours(uint32_t node, std::vector<uint32_t> &out) const;
  uint32_t node_count() const;
  uint64_t edge_count() const;
  uint64_t memory_bytes() const;
};

template <typename G> uint32_t bfs(const G &g, uint32_t from, uint32_t to) {
  if (from == to)
    return 0;

  std::vector<uint32_t> dist(g.node_count(), UINT32_MAX);
  std::vector<uint32_t> queue;
  std::vector<uint32_t> nbrs;
  size_t head = 0;

  dist[from] = 0;
  queue.push_back(from);

  while (head < queue.size()) {
    uint32_t node = queue[head++];
    g.neighbours(node, nbrs);
    for (uint32_t nb : nbrs) {
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

template <typename G>
uint32_t bfs_path(const G &g, uint32_t from, uint32_t to,
                  std::vector<uint32_t> &path) {
  path.clear();
  if (from == to) {
    path.push_back(from);
    return 0;
  }

  std::vector<uint32_t> dist(g.node_count(), UINT32_MAX);
  std::vector<uint32_t> parent(g.node_count(), UINT32_MAX);
  std::vector<uint32_t> queue;
  std::vector<uint32_t> nbrs;
  size_t head = 0;

  dist[from] = 0;
  queue.push_back(from);

  while (head < queue.size()) {
    uint32_t node = queue[head++];
    g.neighbours(node, nbrs);
    for (uint32_t nb : nbrs) {
      if (dist[nb] != UINT32_MAX)
        continue;
      dist[nb] = dist[node] + 1;
      parent[nb] = node;
      if (nb == to) {
        for (uint32_t v = to; v != from; v = parent[v])
          path.push_back(v);
        path.push_back(from);
        std::reverse(path.begin(), path.end());
        return dist[nb];
      }
      queue.push_back(nb);
    }
  }
  return UINT32_MAX;
}