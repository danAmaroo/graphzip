#include "compressed.hpp"
#include "dumps.hpp"
#include "graph.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

int main(int argc, char **argv) {
  std::ios::sync_with_stdio(false);

  const std::string_view cmd = argc > 1 ? argv[1] : "";

  if (cmd == "parse-page") {
    PageIndex idx = parse_page(std::cin);
    std::cerr << idx.titles.size() << " articles\n";
    write_page_index(idx, "data");
    return 0;
  }

  if (cmd == "parse-linktarget") {
    auto titles = load_titles("data");
    std::cerr << titles.size() << " titles loaded\n";

    auto lt = parse_linktarget(std::cin, titles);
    std::cerr << lt.size() << " lt entries\n";

    std::ofstream out("data/lt_to_dense.bin", std::ios::binary);
    if (!out) {
      std::cerr << "could not open data/lt_to_dense.bin\n";
      return 1;
    }
    out.write(reinterpret_cast<const char *>(lt.data()),
              lt.size() * sizeof(uint32_t));
    return 0;
  }
  if (cmd == "parse-pagelinks") {
    auto page_ids = read_u32_file("data/page_ids.bin");
    uint32_t max_id = *std::max_element(page_ids.begin(), page_ids.end());
    std::vector<uint32_t> page_to_dense(max_id + 1, UINT32_MAX);
    for (uint32_t dense = 0; dense < page_ids.size(); ++dense) {
      page_to_dense[page_ids[dense]] = dense;
    }
    auto lt_to_dense = read_u32_file("data/lt_to_dense.bin");
    parse_pagelinks(std::cin, page_to_dense, lt_to_dense, "data/edges_raw.bin");
    return 0;
  }
  if (cmd == "subset") {
    make_subset("data/edges_raw.bin", "data/edges_1m.bin", 1'000'000);
    return 0;
  }

  if (cmd == "sort-edges") {
    auto flat = read_u32_file("data/edges_1m.bin");
    std::vector<std::pair<uint32_t, uint32_t>> pairs;
    pairs.reserve(flat.size() / 2);

    for (size_t i = 0; i + 1 < flat.size(); i += 2) {
      pairs.emplace_back(flat[i], flat[i + 1]);
    }
    std::cerr << pairs.size() << " edges before dedup\n";

    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    std::cerr << pairs.size() << " edges after dedup\n";

    std::ofstream out("data/edges_1m_sorted.bin", std::ios::binary);
    out.write(reinterpret_cast<const char *>(pairs.data()),
              static_cast<std::streamsize>(pairs.size() * 8));
    static_assert(sizeof(std::pair<uint32_t, uint32_t>) == 8);

    return 0;
  }

  if (cmd == "load") {
    auto t0 = std::chrono::steady_clock::now();
    Graph g = Graph::load("data/edges_1m_sorted.bin", 1'000'000);
    auto t1 = std::chrono::steady_clock::now();
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cerr << g.node_count() << " nodes\n";
    std::cerr << g.edge_count() << " edges\n";
    std::cerr << g.memory_bytes() / 1024.0 / 1024.0 << " MB\n";
    std::cerr << g.memory_bytes() * 8.0 / g.edge_count() << " bits/edge\n";
    std::cerr << ms << " ms to load\n";

    return 0;
  }

  if (cmd == "bfs" && argc > 3) {
    Graph g = Graph::load("data/edges_1m_sorted.bin", 1'000'000);
    uint32_t from = std::stoul(argv[2]);
    uint32_t to = std::stoul(argv[3]);
    auto t0 = std::chrono::steady_clock::now();
    uint32_t d = bfs(g, from, to);
    auto t1 = std::chrono::steady_clock::now();
    auto us =
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    if (d == UINT32_MAX) {
      std::cerr << from << " -> " << to << ": unreachable";
    } else {
      std::cerr << from << " -> " << to << ": " << d << " hops";
    }
    std::cerr << " (" << us << " us)\n";
    return 0;
  }

  if (cmd == "bench") {
    Graph g = Graph::load("data/edges_1m_sorted.bin", 1'000'000);
    std::cerr << g.node_count() << " nodes, " << g.edge_count() << " edges\n";
    std::cerr << g.memory_bytes() * 8.0 / g.edge_count() << " bits/edge\n\n";

    std::mt19937 rng(20260913);
    std::uniform_int_distribution<uint32_t> pick(0, g.node_count() - 1);

    uint64_t total_us = 0;
    const int runs = 10;

    for (int i = 0; i < runs; ++i) {
      uint32_t from = pick(rng);
      uint32_t to = pick(rng);

      auto t0 = std::chrono::steady_clock::now();
      uint32_t d = bfs(g, from, to);
      auto t1 = std::chrono::steady_clock::now();

      auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0)
                    .count();
      total_us += us;

      std::cerr << from << " -> " << to << ": ";
      if (d == UINT32_MAX) {
        std::cerr << "unreachable";
      } else {
        std::cerr << d << " hops";
      }
      std::cerr << " (" << us << " us)\n";
    }

    std::cerr << "\naverage: " << total_us / runs << " us\n";
    return 0;
  }
  if (cmd == "compressed-bench") {
    Graph g = Graph::load("data/edges_1m_sorted.bin", 1'000'000);
    CompressedGraph cg = CompressedGraph::build(g);
    std::cerr << g.node_count() << " nodes, " << g.edge_count() << " edges\n";
    std::cerr << g.memory_bytes() * 8.0 / g.edge_count() << " bits/edge\n\n";

    std::mt19937 rng(20260913);
    std::uniform_int_distribution<uint32_t> pick(0, g.node_count() - 1);

    uint64_t total_us = 0;
    const int runs = 10;

    for (int i = 0; i < runs; ++i) {
      uint32_t from = pick(rng);
      uint32_t to = pick(rng);

      auto t0 = std::chrono::steady_clock::now();
      uint32_t d = bfs(cg, from, to);
      auto t1 = std::chrono::steady_clock::now();

      auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0)
                    .count();
      total_us += us;

      std::cerr << from << " -> " << to << ": ";
      if (d == UINT32_MAX) {
        std::cerr << "unreachable";
      } else {
        std::cerr << d << " hops";
      }
      std::cerr << " (" << us << " us)\n";
    }

    std::cerr << "\naverage: " << total_us / runs << " us\n";
    return 0;
  }
  if (cmd == "verify") {
    Graph g = Graph::load("data/edges_1m_sorted.bin", 1'000'000);
    std::cerr << "baseline loaded\n";

    CompressedGraph cg = CompressedGraph::build(g);
    std::cerr << "compressed built\n";

    std::vector<uint32_t> a, b;
    uint64_t checked = 0;

    for (uint32_t node = 0; node < g.node_count(); ++node) {
      g.neighbours(node, a);
      cg.neighbours(node, b);

      if (a != b) {
        std::cerr << "MISMATCH at node " << node << "\n";
        std::cerr << "  baseline degree " << a.size() << ", compressed degree "
                  << b.size() << "\n";
        for (size_t i = 0; i < std::min(a.size(), b.size()) && i < 10; ++i) {
          std::cerr << "  [" << i << "] " << a[i] << " vs " << b[i] << "\n";
        }
        return 1;
      }
      checked += a.size();
    }

    std::cerr << "OK: " << g.node_count() << " nodes, " << checked
              << " edges round-tripped exactly\n";
    std::cerr << cg.memory_bytes() / 1024.0 / 1024.0 << " MB\n";
    std::cerr << cg.memory_bytes() * 8.0 / cg.edge_count() << " bits/edge\n";
    return 0;
  }

  std::cerr << "usage: graphzip <parse-page|parse-linktarget>\n";
  return 1;
}