#include "dumps.hpp"

#include <algorithm>
#include <fstream>
#include <ios>
#include <iostream>
#include <string_view>
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

  std::cerr << "usage: graphzip <parse-page|parse-linktarget>\n";
  return 1;
}