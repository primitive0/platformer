#pragma once

#include <limits>

template<typename N>
inline constexpr N NUM_MAX = std::numeric_limits<N>::max();

template<typename N>
inline constexpr N NUM_MIN = std::numeric_limits<N>::min();
