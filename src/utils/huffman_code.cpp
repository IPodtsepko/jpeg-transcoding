#include "utils/huffman_code.hpp"

#include "encoder/constants.hpp"
#include "fmt/core.h"

#include <cstdio>

namespace utils {

void HuffmanCode::encode_dc(int dc, int last_dc, Output & output) const
{
    int delta_dc = dc - last_dc;

    if (delta_dc == 0) {
        output.write(m_dc_table[0x00].m_code, m_dc_table[0x00].m_length);
    }
    else {
        const auto entry = to_entry(delta_dc);
        const auto & code_word = m_dc_table[entry.m_length];
        output.write(code_word.m_code, code_word.m_length)
                .write(entry.m_code, entry.m_length);
    }
}

Entry HuffmanCode::to_entry(int value)
{
    auto absolute = value < 0 ? -value : value;

    Entry result;
    result.m_length = 1;
    while ((absolute >>= 1) > 0) {
        ++result.m_length;
    }

    if (value < 0) {
        --value;
    }

    const auto mask = (1 << result.m_length) - 1;
    result.m_code = value & mask;

    return result;
}

std::array<Entry, 16> HuffmanCode::get_shortest_code_words_by_runs() const
{
    std::array<Entry, 16> result;
    for (std::size_t i = 1; i < m_ac_table.size(); ++i) {
        const auto & entry = m_ac_table[i];
        if (entry == constants::Nil || i == 0xF0) {
            continue;
        }
        const auto run = i >> 4;

        auto & argmin = result[run];
        if (argmin == constants::Nil || argmin.m_length > entry.m_length) {
            argmin = entry;
        }
    }
    return result;
}

void HuffmanCode::encode_ac(const std::array<int, 64> & block, Output & output, const Mask & mask) const
{
    perform_run_level_encoding(std::next(block.begin()), block.end(), output, mask);
}

int HuffmanCode::encode(const std::array<int, 64> & block, const int last_dc, Output & output, const Mask & mask) const
{
    const auto dc = block[0];
    encode_dc(dc, last_dc, output);
    encode_ac(block, output, mask);
    return dc;
}

bool operator==(const Entry & lhs, const Entry & rhs)
{
    return lhs.m_code == rhs.m_code && lhs.m_length == rhs.m_length;
}

bool operator!=(const Entry & lhs, const Entry & rhs)
{
    return !(lhs == rhs);
}

} // namespace utils
