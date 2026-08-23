#include <cstddef>
#include <string_view>
#include <vector>

size_t scan_tuple(std::string_view line, size_t pos,
                  std::vector<std::string_view> &out) {

  out.clear();
  pos++;
  size_t field_start = pos;
  bool in_string = false;

  while (pos < line.size()) {
    if (in_string) {
      switch (line[pos]) {
      case '\\':
        pos++;
        break;
      case '\'':
        in_string = false;
        break;
      }
      pos++;
      continue;
    } else {
      switch (line[pos]) {
      case '\'':
        in_string = true;
        break;
      case ',':
        out.push_back(line.substr(field_start, pos - field_start));
        field_start = pos + 1;
        break;
      case ')':
        out.push_back(line.substr(field_start, pos - field_start));
        return pos + 1;
      }
      pos++;
    }
  }
  return pos;
}