#pragma once

#include <bitset>

namespace utils {

using Mask = std::bitset<64>;
inline static constexpr Mask MaskAll = 0xffffffffffffffff;

class DCTCoefficientsFilter
{
public:
    DCTCoefficientsFilter(const std::size_t power, std::size_t masks_count = 9, unsigned int seed = 42);

    std::size_t get_masks_count() const;

    Mask get_mask();

private:
    std::size_t m_index = 0;
    const std::vector<Mask> m_masks;
};

} // namespace utils
