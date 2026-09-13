#include "compressed.hpp"
#include "bitstream.hpp"
#include <cstdint>

uint32_t CompressedGraph::node_count() const {
  return static_cast<uint32_t>(node_offsets.size() - 1);
}

uint64_t CompressedGraph::edge_count() const { return edges; }

uint64_t CompressedGraph::memory_bytes() const {
  return bits.size() + node_offsets.size() * sizeof(uint64_t);
}

void CompressedGraph::neighbours(uint32_t node,
                                 std::vector<uint32_t> &out) const {
  BitReader r(bits);
  r.seek(node_offsets[node]);

  uint64_t degree = read_gamma(r);
  out.clear();
  out.reserve(degree);

  if (degree > 0) {
    uint32_t prev = static_cast<uint32_t>(read_gamma(r));
    out.push_back(prev);
    for (uint64_t i = 1; i < degree; ++i) {
      prev += static_cast<uint32_t>(read_gamma(r)) + 1;
      out.push_back(prev);
    }
  }
}

CompressedGraph CompressedGraph::build(const Graph &g) {
  CompressedGraph cg;
  uint32_t n = g.node_count();
  cg.node_offsets.resize(n + 1);
  cg.edges = g.edge_count();
  BitWriter w;
  std::vector<uint32_t> nbrs;
  for (uint32_t node = 0; node < n; node++) {
    cg.node_offsets[node] = w.bits_written();
    g.neighbours(node, nbrs);
    write_gamma(w, nbrs.size());
    if (!nbrs.empty()) {
      write_gamma(w, nbrs[0]);
      for (size_t i = 1; i < nbrs.size(); ++i) {
        write_gamma(w, nbrs[i] - nbrs[i - 1] - 1);
      }
    }
  }
  cg.node_offsets[n] = w.bits_written();
  w.flush();
  cg.bits = w.bytes();
  return cg;
}