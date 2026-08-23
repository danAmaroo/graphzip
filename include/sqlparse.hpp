#pragma once

#include <string_view>
#include <vector>

size_t scan_tuple(std::string_view line, size_t pos,
                  std::vector<std::string_view> &out);
