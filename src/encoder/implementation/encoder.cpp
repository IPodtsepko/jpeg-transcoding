#include "encoder/implementation/encoder.hpp"

#include "encoder/constants.hpp"
#include "encoder/implementation/y_cb_cr_block.hpp"
#include "utils/image.hpp"

namespace implementation {

Encoder::Encoder(const std::size_t quality, Output & output)
    : m_subsample(quality <= 90)
    , m_quality(quality < 50 ? 5000 / quality : 200 - quality * 2)
    , m_luminance_quantization_table(constants::luminance::QUANTIZATION_TABLE, m_quality)
    , m_chrominance_quantization_table(constants::chrominance::QUANTIZATION_TABLE, m_quality)
    , m_luminance_encoder(m_luminance_quantization_table, constants::luminance::HUFFMAN_CODE, output)
    , m_chrominance_blue_encoder(m_chrominance_quantization_table, constants::chrominance::HUFFMAN_CODE, output)
    , m_chrominance_red_encoder(m_chrominance_quantization_table, constants::chrominance::HUFFMAN_CODE, output)
{
}

void Encoder::encode(const utils::Image & image)
{
    if (m_subsample) {
        encode<2>(image);
    }
    else {
        encode<1>(image);
    }
}

template <std::size_t Scaling>
void Encoder::encode(const utils::Image & image)
{
    static constexpr std::size_t Stride = 8 * Scaling;
    for (std::size_t x = 0; x < image.get_height(); x += Stride) {
        for (std::size_t y = 0; y < image.get_width(); y += Stride) {
            implementation::YCbCrBlock<Scaling> block{image, x, y};
            for (auto & y : block.Ys()) {
                m_luminance_encoder.encode(y);
            }
            m_chrominance_blue_encoder.encode(block.Cb());
            m_chrominance_red_encoder.encode(block.Cr());
        }
    }
}

} // namespace implementation
