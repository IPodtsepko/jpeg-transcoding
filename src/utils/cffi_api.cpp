#include <utils/dct_coefficients_filter.hpp>
#include <utils/discrete_cosine_transform.hpp>

extern "C" {
void forward_discrete_cosine_transform(float * block)
{
    auto * cpp_type_block = reinterpret_cast<std::array<float, 64> *>(block);
    utils::DiscreteCosineTransform::forward(*cpp_type_block);
}

unsigned char * inverse_discrete_cosine_transform(int * block)
{
    auto * cpp_type_block = reinterpret_cast<std::array<int, 64> *>(block);
    auto * result = new unsigned char[64];
    utils::DiscreteCosineTransform::inverse(*cpp_type_block, 8, result);
    return result;
}

size_t get_dct_filter_masks_count(const size_t power)
{
    return utils::DCTCoefficientsFilter(power).get_masks_count();
}

size_t * get_dct_filter_masks(const size_t power)
{
    utils::DCTCoefficientsFilter filter(power);
    const auto n = filter.get_masks_count();

    auto * masks = new size_t[n];

    for (std::size_t i = 0; i < n; ++i) {
        masks[i] = filter.get_mask().to_ulong();
    }

    return masks;
}

void free_buffer(unsigned char * block)
{
    delete[] block;
}

void free_masks(size_t * masks)
{
    delete[] masks;
}

} // extern "C"
