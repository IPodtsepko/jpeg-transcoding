/**
 * @file y_cr_cb_block.hpp
 * @author IPodtsepko
 */
#pragma once

#include "utils/image.hpp"

#include <array>
#include <tuple>

namespace implementation {

template <std::size_t Scaling = 1>
class YCbCrBlock
{
    inline static constexpr auto BlockSide = 8;
    inline static constexpr auto FrameSide = BlockSide * Scaling;
    inline static constexpr auto BlocksCount = Scaling * Scaling;
    inline static constexpr float AverageModifier = 1. / Scaling / Scaling;

private:
    using Block = std::array<float, 64>;
    using Color = std::tuple<float, float, float>;

public:
    YCbCrBlock(const utils::Image & image, const std::size_t x, const std::size_t y)
    {
        m_chrominance_blue.fill(0.);
        m_chrominance_red.fill(0.);

        for (std::size_t i = 0; i < FrameSide; ++i) {
            for (std::size_t j = 0; j < FrameSide; ++j) {
                const auto rgb = image.get(x + i, y + j);
                const auto [luminance, chrominance_blue, chrominance_red] = to_y_cb_cr(rgb);

                m_luminance[block_id(i, j)][luminance_pixel_id(i, j)] = luminance;

                const std::size_t position = chrominance_pixel_id(i, j);
                m_chrominance_blue[position] += chrominance_blue * AverageModifier;
                m_chrominance_red[position] += chrominance_red * AverageModifier;
            }
        }
    }

    std::array<Block, BlocksCount> & Ys()
    {
        return m_luminance;
    }

    Block & Cb()
    {
        return m_chrominance_blue;
    }

    Block & Cr()
    {
        return m_chrominance_red;
    }

private:
    static std::size_t block_id(const std::size_t i, const std::size_t j)
    {
        return (i / BlockSide) * Scaling + j / BlockSide;
    }

    static std::size_t luminance_pixel_id(const std::size_t i, const std::size_t j)
    {
        return (i % BlockSide) * BlockSide + j % BlockSide;
    }

    static std::size_t chrominance_pixel_id(const std::size_t i, const std::size_t j)
    {
        return (i / Scaling) * BlockSide + j / Scaling;
    }

    static Color to_y_cb_cr(const Color & rgb)
    {
        const auto [r, g, b] = rgb;
        return {+0.29900f * r + 0.58700f * g + 0.11400f * b - 128,
                -0.16874f * r - 0.33126f * g + 0.50000f * b,
                +0.50000f * r - 0.41869f * g - 0.08131f * b};
    }

    std::array<Block, BlocksCount> m_luminance;
    Block m_chrominance_blue;
    Block m_chrominance_red;
};

} // namespace implementation
