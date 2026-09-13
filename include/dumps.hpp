#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

struct PageIndex {
  std::unordered_map<uint32_t, uint32_t> page_to_dense;
  std::vector<std::string> titles;
};

PageIndex parse_page(std::istream &in);
void write_page_index(const PageIndex &idx, const std::string &dir);
std::unordered_map<std::string, uint32_t> load_titles(const std::string &dir);
std::vector<uint32_t>
parse_linktarget(std::istream &in,
                 const std::unordered_map<std::string, uint32_t> &titles);
std::vector<uint32_t> read_u32_file(const std::string &path);
void parse_pagelinks(std::istream &in,
                     const std::vector<uint32_t> &page_to_dense,
                     const std::vector<uint32_t> &lt_to_dense,
                     const std::string &out_path);
void make_subset(const std::string &in_path, const std::string &out_path,
                 uint32_t limit);