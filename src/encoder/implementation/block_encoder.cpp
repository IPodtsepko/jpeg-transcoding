#include "encoder/implementation/block_encoder.hpp"

#include "utils/discrete_cosine_transform.hpp"

namespace implementation {

BlockEncoder::BlockEncoder(const utils::QuantizationTable & quantization_table, const utils::HuffmanCode & huffman, Output & output)
    : m_last_dc(0)
    , m_quantization_table(quantization_table)
    , m_huffman(huffman)
    , m_output(output)
{
}

void BlockEncoder::encode(std::array<float, 64> & block)
{
    utils::DiscreteCosineTransform::forward(block);

    const auto quantized = m_quantization_table.forward(block);

    m_last_dc = m_huffman.encode(quantized, m_last_dc, m_output);
}

} // namespace implementation
