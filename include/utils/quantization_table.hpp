#pragma once

#include <utils/bytes.hpp>
#include <utils/zigzag.hpp>

namespace utils {

class QuantizationTable
{
public:
    QuantizationTable(const int data[], const std::size_t quality)
    {
        for (int i = 0; i < 64; ++i) {
            m_data[ZIGZAG_ORDER[i]] = quantization_table_value(data[i], quality);
        }
    }

    const Bytes<64> & get() const
    {
        return m_data;
    }

    std::array<int, 64> apply(std::array<float, 64> & block) const
    {
        std::array<int, 64> result;
        for (std::size_t y = 0, j = 0; y < 8; ++y) {
            for (std::size_t x = 0; x < 8; ++x, ++j) {
                const auto k = ZIGZAG_ORDER[j];
                result[k] = round(block[j] / m_data[k]);
            }
        }
        return result;
    }

private:
    static unsigned char quantization_table_value(const int base, const std::size_t quality)
    {
        return std::min(std::max(1, (base * static_cast<int>(quality) + 50) / 100), 255);
    }

    static int round(const float v)
    {
        return static_cast<int>(v < 0 ? std::ceil(v - 0.5f) : std::floor(v + 0.5f));
    }

    Bytes<64> m_data;
};

} // namespace utils
