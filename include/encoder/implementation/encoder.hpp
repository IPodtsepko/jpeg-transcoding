/**
 * @file encoder.hpp
 * @author IPodtsepko
 */
#pragma once

#include "encoder/implementation/block_encoder.hpp"
#include "utils/output.hpp"
#include "utils/quantization_table.hpp"

namespace utils {

class Image;

}

namespace implementation {

class Encoder
{
public:
    Encoder(const std::size_t quality, Output & output);

    void encode(const utils::Image & image);

private:
    template <std::size_t Scaling>
    void encode(const utils::Image & image);

public:
    const bool m_subsample;
    const std::size_t m_quality;

    const utils::QuantizationTable m_luminance_quantization_table;
    const utils::QuantizationTable m_chrominance_quantization_table;

    implementation::BlockEncoder m_luminance_encoder;
    implementation::BlockEncoder m_chrominance_blue_encoder;
    implementation::BlockEncoder m_chrominance_red_encoder;
};

} // namespace implementation
