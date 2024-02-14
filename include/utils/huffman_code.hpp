#pragma once

#include <array>

class Output;

namespace utils {

class HuffmanCode
{
public:
    struct Entry
    {
        unsigned short m_code = 0;
        unsigned short m_length = 0;
    };

    using HuffmanTable = std::array<Entry, 256>;

    constexpr HuffmanCode(const HuffmanTable & dc_table,
                          const HuffmanTable & ac_table)
        : m_dc_table(dc_table)
        , m_ac_table(ac_table){};

    int encode(const int * block, int last_dc, Output & output) const;

private:
    // int m_last_dc = 0;
    HuffmanTable m_dc_table;
    HuffmanTable m_ac_table;
};

} // namespace utils
