#include <utils/huffman_code.hpp>
#include <utils/output.hpp>

namespace {

static utils::HuffmanCode::Entry to_entry(int value)
{
    auto absolute = value < 0 ? -value : value;

    utils::HuffmanCode::Entry result;
    result.m_length = 1;
    while (absolute >>= 1) {
        ++result.m_length;
    }

    if (value < 0) {
        --value;
    }

    const auto mask = (1 << result.m_length) - 1;
    result.m_code = value & mask;

    return result;
}

} // namespace

namespace utils {

int HuffmanCode::encode(const int * block, const int last_dc, Output & output) const
{
    // Encode DC
    const auto actual_dc = block[0];
    int delta_dc = actual_dc - last_dc;

    if (delta_dc == 0) {
        output << m_dc_table[0x00];
    }
    else {
        const auto entry = to_entry(delta_dc);
        output << m_dc_table[entry.m_length] << entry;
    }

    // Encode ACs
    std::size_t trailling_zeros_position = 63;
    while (trailling_zeros_position > 0 && block[trailling_zeros_position] == 0) {
        --trailling_zeros_position;
    }
    ++trailling_zeros_position;

    const Entry & end_of_block_marker = m_ac_table[0x00];
    if (trailling_zeros_position == 1) {
        // All amplitude coefficients (ACs) are zero.
        output << end_of_block_marker;
        return actual_dc;
    }

    for (std::size_t i = 1; i < trailling_zeros_position; ++i) {
        auto start_position = i;
        while (block[i] == 0 && i < trailling_zeros_position) {
            ++i;
        }
        auto number_of_zeros = i - start_position;

        if (number_of_zeros >= 16) {
            const auto sixteen_zeros_blocks_count = number_of_zeros >> 4;
            for (std::size_t j = 1; j <= sixteen_zeros_blocks_count; ++j) {
                const Entry & sixteen_zeros_marker = m_ac_table[0xF0];
                output << sixteen_zeros_marker;
            }
            number_of_zeros &= 15;
        }

        const auto entry = to_entry(block[i]);
        output << m_ac_table[(number_of_zeros << 4) | entry.m_length] << entry;
    }

    if (trailling_zeros_position != 64) {
        output << end_of_block_marker;
    }

    return actual_dc;
}

} // namespace utils
