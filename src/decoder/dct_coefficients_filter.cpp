#include <decoder/dct_coefficients_filter.hpp>

DCTCoefficientsFilter::DCTCoefficientsFilter(const std::size_t power)
    : m_power(power)
{
}

void DCTCoefficientsFilter::apply(int block[64]) const
{
    const auto & mask = FILTER_RANDOM_MASKS[m_power];
    for (std::size_t i = 0; i < 64; ++i) {
        if (!mask[i]) {
            block[i] = 0;
        }
    }
}
