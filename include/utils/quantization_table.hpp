#pragma once

#include <utils/bytes.hpp>
#include <utils/zigzag.hpp>

namespace utils {

class QuantizationTable
{
public:
    QuantizationTable(const int data[], const std::optional<std::size_t> & quality = std::nullopt)
    {
        for (int i = 0; i < 64; ++i) {
            m_data[ZIGZAG_ORDER[i]] = quantization_table_value(data[i], quality);
        }
    }

    const Bytes<64> & get() const
    {
        return m_data;
    }

    std::array<int, 64> forward(const std::array<float, 64> & block) const
    {
        std::array<int, 64> result;
        for (std::size_t j = 0; j < 64; ++j) {
            const auto k = ZIGZAG_ORDER[j];
            result[k] = round(block[j] / m_data[k]);
        }
        return result;
    }

    void inverse(std::array<int, 64> & block) const
    {
        for (std::size_t i = 0; i < 64; ++i) {
            block[REVERSED_ZIGZAG_ORDER[i]] *= m_data[ZIGZAG_ORDER[i]];
        }
    }

private:
    static unsigned char quantization_table_value(int base, const std::optional<std::size_t> & quality)
    {
        if (quality.has_value()) {
            base = (base * static_cast<int>(quality.value()) + 50) / 100;
        }
        return std::min(std::max(1, base), 255);
    }

    static int round(const float v)
    {
        return static_cast<int>(v < 0 ? std::ceil(v - 0.5f) : std::floor(v + 0.5f));
    }

    Bytes<64> m_data;
};

} // namespace utils
