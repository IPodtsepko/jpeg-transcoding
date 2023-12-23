#pragma once

#include "utils/dct_coefficients_filter.hpp"
#include "utils/output.hpp"

#include <array>
#include <fmt/core.h>

namespace utils {

struct Entry
{
    unsigned short m_code = 0;
    unsigned short m_length = 0;
};

class HuffmanCode
{
public:
    using HuffmanTable = std::array<Entry, 256>;

    constexpr HuffmanCode() = default;

    constexpr HuffmanCode(const HuffmanTable & dc_table,
                          const HuffmanTable & ac_table)
        : m_dc_table(dc_table)
        , m_ac_table(ac_table){};

    void encode_dc(int dc, int last_dc, Output & output) const;

    void encode_ac(const std::array<int, 64> & block, Output & output, const Mask & mask) const;

    template <class InputIt>
    void perform_run_level_encoding(const InputIt & begin, const InputIt & end, Output & output, const Mask & mask = utils::MaskAll) const
    {
        const auto placeholders = get_shortest_code_words_by_runs();
        std::size_t i = 1, run = 0;
        for (InputIt it = begin; it != end; ++i, ++it) {
            const int ac = *it;
            if (ac == 0) {
                ++run;
                continue;
            }

            // Run level encoding

            const Entry & sixteen_zeros_marker = m_ac_table[0xF0];
            const auto sixteen_zeros_markers_count = run >> 4;
            for (std::size_t i = 0; i < sixteen_zeros_markers_count; ++i) {
                output.write(sixteen_zeros_marker.m_code, sixteen_zeros_marker.m_length);
            }

            run &= 0xf;

            if (i < mask.size() && !mask[i]) {
                const auto & placeholder = placeholders[run];
                output.write(placeholder.m_code, placeholder.m_length);
            }
            else {
                const auto entry = to_entry(ac);
                const auto code_word = m_ac_table[(run << 4) | entry.m_length];
                if (code_word.m_length == 0) {
                    fmt::println(stderr, "Huffman code word has lenght 0");
                }
                output.write(code_word.m_code, code_word.m_length)
                        .write(entry.m_code, entry.m_length);
            }

            run = 0;
        }

        if (run > 0) {
            const Entry & end_of_block_marker = m_ac_table[0x00];
            output.write(end_of_block_marker.m_code, end_of_block_marker.m_length);
        }
    }

    int encode(const std::array<int, 64> & block, int last_dc, Output & output, const Mask & mask = MaskAll) const;

private:
    static Entry to_entry(int value);
    std::array<Entry, 16> get_shortest_code_words_by_runs() const;

private:
    HuffmanTable m_dc_table;
    HuffmanTable m_ac_table;
};

bool operator==(const Entry & lhs, const Entry & rhs);
bool operator!=(const Entry & lhs, const Entry & rhs);

} // namespace utils
