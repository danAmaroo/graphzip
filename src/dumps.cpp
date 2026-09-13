#include "dumps.hpp"
#include "sqlparse.hpp"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
std::string unquote(std::string_view field) {
  if (field.size() < 2) {
    return "";
  }
  field = field.substr(1, field.size() - 2);
  if (field.find('\\') == std::string_view::npos) {
    return std::string(field);
  }
  std::string ans;
  ans.reserve(field.size());
  for (size_t i = 0; i < field.size(); i++) {
    if (field[i] == '\\' && i + 1 < field.size()) {
      ans += (field[i + 1]);
      i++;
    } else {
      ans += (field[i]);
    }
  }
  return ans;
}
} // namespace

PageIndex parse_page(std::istream &in) {
  PageIndex idx;
  idx.page_to_dense.reserve(8'000'000);
  std::string line;

  while (std::getline(in, line)) {
    for_each_tuple(line, [&](const std::vector<std::string_view> &f) {
      static size_t seen = 0;
      if (++seen % 1'000'000 == 0) {
        std::cerr << seen << " tuples\n";
      }
      if (f.size() < 4)
        return;
      if (f[1] != "0")
        return;
      if (f[3] != "0")
        return;
      uint32_t page_id = 0;
      auto [ptr, ec] =
          std::from_chars(f[0].data(), f[0].data() + f[0].size(), page_id);
      if (ec != std::errc{})
        return;
      idx.page_to_dense[page_id] = static_cast<uint32_t>(idx.titles.size());
      idx.titles.push_back(unquote(f[2]));
    });
  }
  return idx;
}

void write_page_index(const PageIndex &idx, const std::string &dir) {
  std::ofstream titles_out(dir + "/titles.txt");
  if (!titles_out) {
    throw std::runtime_error("titles.txt couldn't be opened");
  }
  for (const std::string &t : idx.titles) {
    titles_out << t << '\n';
  }
  std::vector<uint32_t> page_ids(idx.titles.size());
  for (auto [page_id, dense] : idx.page_to_dense) {
    page_ids[dense] = page_id;
  }
  std::ofstream ids_out(dir + "/page_ids.bin", std::ios::binary);
  ids_out.write(reinterpret_cast<const char *>(page_ids.data()),
                page_ids.size() * sizeof(uint32_t));
}

std::unordered_map<std::string, uint32_t> load_titles(const std::string &dir) {
  std::unordered_map<std::string, uint32_t> dense_id_map;
  dense_id_map.reserve(8'000'000);
  std::ifstream in(dir + "/titles.txt");
  std::string line;
  uint32_t dense = 0;
  while (getline(in, line)) {
    dense_id_map[line] = dense++;
  }
  return dense_id_map;
}

std::vector<uint32_t>
parse_linktarget(std::istream &in,
                 const std::unordered_map<std::string, uint32_t> &titles) {
  std::vector<uint32_t> v;
  std::string line;
  while (std::getline(in, line)) {
    for_each_tuple(line, [&](const std::vector<std::string_view> &f) {
      if (f.size() < 3)
        return;
      if (f[1] != "0")
        return;
      uint64_t lt_id = 0;
      auto res = std::from_chars(f[0].data(), f[0].data() + f[0].size(), lt_id);
      if (res.ec != std::errc{})
        return;

      auto it = titles.find(unquote(f[2]));
      if (it == titles.end())
        return;
      if (lt_id >= v.size())
        v.resize(lt_id + 1, UINT32_MAX);
      v[lt_id] = it->second;
    });
  }
  return v;
}

void parse_pagelinks(std::istream &in,
                     const std::vector<uint32_t> &page_to_dense,
                     const std::vector<uint32_t> &lt_to_dense,
                     const std::string &out_path) {
  std::ofstream out(out_path, std::ios::binary);
  if (!out)
    throw std::runtime_error("could not open " + out_path);

  std::vector<uint32_t> buf;
  buf.reserve(2'000'000);

  auto flush = [&] {
    out.write(reinterpret_cast<const char *>(buf.data()),
              static_cast<std::streamsize>(buf.size() * sizeof(uint32_t)));
    buf.clear();
  };

  std::string line;
  while (std::getline(in, line)) {
    for_each_tuple(line, [&](const std::vector<std::string_view> &f) {
      static size_t seen = 0;
      if (++seen % 1'000'000 == 0)
        std::cerr << seen << " tuples\n";
      if (f.size() < 3)
        return;
      if (f[1] != "0")
        return;
      uint32_t from_page = 0;
      auto res1 =
          std::from_chars(f[0].data(), f[0].data() + f[0].size(), from_page);
      if (res1.ec != std::errc{})
        return;

      uint64_t target_id = 0;
      auto res2 =
          std::from_chars(f[2].data(), f[2].data() + f[2].size(), target_id);
      if (res2.ec != std::errc{})
        return;
      if (from_page >= page_to_dense.size())
        return;
      uint32_t from = page_to_dense[from_page];
      if (from == UINT32_MAX)
        return;
      if (target_id >= lt_to_dense.size())
        return;
      uint32_t to = lt_to_dense[target_id];
      if (to == UINT32_MAX)
        return;
      buf.push_back(from);
      buf.push_back(to);
      if (buf.size() >= 2'000'000)
        flush();
    });
  }
  flush();
}

std::vector<uint32_t> read_u32_file(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("could not open " + path);
  in.seekg(0, std::ios::end);
  const auto bytes = static_cast<size_t>(in.tellg());
  in.seekg(0, std::ios::beg);
  std::vector<uint32_t> v(bytes / sizeof(uint32_t));
  in.read(reinterpret_cast<char *>(v.data()),
          static_cast<std::streamsize>(bytes));
  return v;
}

void make_subset(const std::string &in_path, const std::string &out_path,
                 uint32_t limit) {
  std::ifstream in(in_path, std::ios::binary);
  if (!in)
    throw std::runtime_error("could not open " + in_path);
  std::ofstream out(out_path, std::ios::binary);
  if (!out)
    throw std::runtime_error("could not open " + out_path);

  std::vector<uint32_t> buf(2'000'000);
  std::vector<uint32_t> out_buf;
  out_buf.reserve(2'000'000);

  auto flush = [&] {
    out.write(reinterpret_cast<const char *>(out_buf.data()),
              static_cast<std::streamsize>(out_buf.size() * sizeof(uint32_t)));
    out_buf.clear();
  };

  while (true) {
    in.read(reinterpret_cast<char *>(buf.data()),
            buf.size() * sizeof(uint32_t));
    size_t got = in.gcount() / sizeof(uint32_t);
    if (got == 0)
      break;
    for (size_t i = 0; i + 1 < got; i += 2) {
      if (buf[i] < limit && buf[i + 1] < limit) {
        out_buf.push_back(buf[i]);
        out_buf.push_back(buf[i + 1]);
      }
    }
    if (out_buf.size() >= 2'000'000)
      flush();
  }
  flush();
}