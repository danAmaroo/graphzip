#pragma once

#include <functional>
#include <string_view>
#include <vector>

size_t scan_tuple(std::string_view line, size_t pos,
                  std::vector<std::string_view> &out);

void for_each_tuple(
    std::string_view line,
    const std::function<void(const std::vector<std::string_view> &)> &fn);