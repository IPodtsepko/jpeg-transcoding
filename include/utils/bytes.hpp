#pragma once

#include <array>

using Byte = unsigned char;

template <std::size_t BytesCount>
using Bytes = std::array<Byte, BytesCount>;

using BytesList = std::vector<Byte>;
