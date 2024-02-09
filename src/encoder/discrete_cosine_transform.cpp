#include <encoder/discrete_cosine_transform.hpp>

namespace {

/**
 * @brief AAN DCT algorithm scaling constants.
 *
 * @details AAN DCT algorithm scaling constants are defined as follows:
 * - aan_scale_factors[0] equals 1;
 * - aan_scale_factors[k] is calculated as cos(k * PI / 16) * sqrt(2) for k = 1..7.
 */
static const float aan_scale_factors[] = {
        1.000000000f * 2.828427125f,
        1.387039845f * 2.828427125f,
        1.306562965f * 2.828427125f,
        1.175875602f * 2.828427125f,
        1.000000000f * 2.828427125f,
        0.785694958f * 2.828427125f,
        0.541196100f * 2.828427125f,
        0.275899379f * 2.828427125f};

/**
 * @brief Implements the Discrete Cosine Transform (DCT).
 */
void discrete_cosine_transform_impl(float & d0, float & d1, float & d2, float & d3, float & d4, float & d5, float & d6, float & d7)
{
    float tmp0 = d0 + d7;
    float tmp7 = d0 - d7;
    float tmp1 = d1 + d6;
    float tmp6 = d1 - d6;
    float tmp2 = d2 + d5;
    float tmp5 = d2 - d5;
    float tmp3 = d3 + d4;
    float tmp4 = d3 - d4;

    // Even part
    float tmp10 = tmp0 + tmp3; // phase 2
    float tmp13 = tmp0 - tmp3;
    float tmp11 = tmp1 + tmp2;
    float tmp12 = tmp1 - tmp2;

    d0 = tmp10 + tmp11; // phase 3
    d4 = tmp10 - tmp11;

    float z1 = (tmp12 + tmp13) * 0.707106781f; // c4
    d2 = tmp13 + z1;                           // phase 5
    d6 = tmp13 - z1;

    // Odd part
    tmp10 = tmp4 + tmp5; // phase 2
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    // The rotator is modified from fig 4-8 to avoid extra negations.
    float z5 = (tmp10 - tmp12) * 0.382683433f; // c6
    float z2 = tmp10 * 0.541196100f + z5;      // c2-c6
    float z4 = tmp12 * 1.306562965f + z5;      // c2+c6
    float z3 = tmp11 * 0.707106781f;           // c4

    float z11 = tmp7 + z3; // phase 5
    float z13 = tmp7 - z3;

    d5 = z13 + z2; // phase 6
    d3 = z13 - z2;
    d1 = z11 + z4;
    d7 = z11 - z4;
}

} // namespace

void discrete_cosine_transform(float * block, const std::size_t stride)
{
    // Application of discrete cosine transformation by rows.
    for (std::size_t i = 0; i < stride * 8; i += stride) {
        discrete_cosine_transform_impl(
                block[i],
                block[i + 1],
                block[i + 2],
                block[i + 3],
                block[i + 4],
                block[i + 5],
                block[i + 6],
                block[i + 7]);
    }

    // Application of discrete cosine transformation by columns
    for (std::size_t i = 0; i < 8; ++i) {
        discrete_cosine_transform_impl(
                block[i],
                block[i + stride],
                block[i + stride * 2],
                block[i + stride * 3],
                block[i + stride * 4],
                block[i + stride * 5],
                block[i + stride * 6],
                block[i + stride * 7]);
    }

    // Descale the DCT coefficients
    for (std::size_t y = 0, j = 0; y < 8; ++y) {
        for (std::size_t x = 0; x < 8; ++x, ++j) {
            block[y * stride + x] /= aan_scale_factors[y] * aan_scale_factors[x];
        }
    }
}
