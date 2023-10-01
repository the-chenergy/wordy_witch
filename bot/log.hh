#pragma once

#include <iomanip>
#include <iostream>

namespace wordy_witch {

template <typename T>
void trace(const char* l, const T& x) {
  if (l[0] != '"') std::cerr << l + (l[0] == ' ') << ": ";
  std::cerr << x << std::endl;
}
template <typename T, typename... A>
void trace(const char* l, const T& x, const A&... a) {
  if (l[0] == ' ') l++;
  size_t s = strchr(l, ',') - l;
  if (l[0] != '"') std::cerr.write(l, s) << ": ";
  std::cerr << x << ", ", trace(l + s + 1, a...);
}

static std::chrono::steady_clock::time_point start_time;
static const auto init_start_time = []() -> int {
  start_time = std::chrono::steady_clock::now();
  return 0;
}();

void print_timestamp() {
  auto current_time = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                                  start_time);
  std::cerr << "(" << std::setprecision(3) << std::fixed << ms.count() / 1000.0
            << "s) " << std::defaultfloat;
}

/*
  `WORDY_WITCH_TRACE(x, ...)` prints `(timestamp) x: {x}, ...` to standard error
*/
#define WORDY_WITCH_TRACE(...)     \
  (wordy_witch::print_timestamp(), \
   wordy_witch::trace(#__VA_ARGS__, __VA_ARGS__), 0)

}  // namespace wordy_witch
