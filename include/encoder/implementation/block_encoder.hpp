/**
 * @file block_encoder.hpp
 * @author IPodtsepko
 */
#pragma once

#include "utils/huffman_code.hpp"
#include "utils/output.hpp"
#include "utils/quantization_table.hpp"

namespace implementation {

class BlockEncoder
{
public:
    BlockEncoder(const utils::QuantizationTable & quantization_table,
                 const utils::HuffmanCode & huffman,
                 Output & output);

    void encode(std::array<float, 64> & block);

private:
    int m_last_dc;
    const utils::QuantizationTable & m_quantization_table;
    const utils::HuffmanCode & m_huffman;
    Output & m_output;
};

} // namespace implementation
