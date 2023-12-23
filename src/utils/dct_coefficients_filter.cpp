#include "utils/dct_coefficients_filter.hpp"

#include <fmt/format.h>
#include <random>
#include <unordered_set>

namespace utils {

namespace {

inline static constexpr std::size_t MasksCount = 9;
using SetOfMasks = std::array<Mask, MasksCount>;

namespace {

std::size_t compositions(const std::size_t n, const std::size_t k)
{
    return static_cast<std::size_t>(std::tgamma(n + 1) / (std::tgamma(k + 1) * std::tgamma(n - k + 1)));
}

} // namespace

std::vector<Mask> generate_masks(const std::size_t power, std::size_t count, const unsigned int seed)
{
    std::size_t coefficients_to_save = 6;
    std::size_t coefficients_to_save_from_the_end = 21;

    std::size_t max_count;
    while ((max_count = compositions(64 - coefficients_to_save - coefficients_to_save_from_the_end, power)) <= 0) {
        if (coefficients_to_save_from_the_end > 0) {
            coefficients_to_save_from_the_end -= 1;
        }
        else {
            coefficients_to_save -= 1;
        }
    }

    std::vector<std::size_t> allowed_to_remove;
    for (std::size_t i = coefficients_to_save; i < 64 - coefficients_to_save_from_the_end; ++i) {
        allowed_to_remove.push_back(i);
    }

    std::unordered_set<Mask> result_set;

    count = std::min(count, max_count);

    std::seed_seq seed_sequence = {seed};
    std::mt19937 generator(seed_sequence);

    while (result_set.size() < count) {
        Mask mask = UINT64_MAX;

        std::vector<std::size_t> zero_bits_positions(allowed_to_remove.begin(), allowed_to_remove.end());
        std::shuffle(zero_bits_positions.begin(), zero_bits_positions.end(), generator);
        zero_bits_positions.resize(power);

        for (const auto position : zero_bits_positions) {
            mask[position] = false;
        }

        result_set.insert(mask);
    }

    return std::vector<Mask>(result_set.begin(), result_set.end());
}

} // namespace

DCTCoefficientsFilter::DCTCoefficientsFilter(const std::size_t power, const std::size_t masks_count, const unsigned int seed)
    : m_masks(generate_masks(power, masks_count, seed))
{
}

std::size_t DCTCoefficientsFilter::get_masks_count() const
{
    return m_masks.size();
}

Mask DCTCoefficientsFilter::get_mask()
{
    const auto mask = m_masks[m_index];
    m_index = (m_index + 1) % m_masks.size();
    return mask;
}

} // namespace utils
