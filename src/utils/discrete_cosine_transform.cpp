#include <utils/discrete_cosine_transform.hpp>

namespace {

/**
 * @brief AAN DCT algorithm scaling constants.
 *
 * @details AAN DCT algorithm scaling constants are defined as follows:
 * - aan_scale_factors[0] equals 1;
 * - aan_scale_factors[k] is calculated as cos(k * PI / 16) * sqrt(2) for k
 * = 1..7.
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
 * @brief Implements the forward discrete cosine transform (DCT).
 */
void forward_transform(float & d0, float & d1, float & d2, float & d3, float & d4, float & d5, float & d6, float & d7)
{
    const auto x0 = d0 + d7;
    const auto x7 = d0 - d7;
    const auto x1 = d1 + d6;
    const auto x6 = d1 - d6;
    const auto x2 = d2 + d5;
    const auto x5 = d2 - d5;
    const auto x3 = d3 + d4;
    const auto x4 = d3 - d4;

    // Even part
    auto x10 = x0 + x3; // phase 2
    const auto tmp13 = x0 - x3;
    auto x11 = x1 + x2;
    auto x12 = x1 - x2;

    d0 = x10 + x11; // phase 3
    d4 = x10 - x11;

    const auto z1 = (x12 + tmp13) * 0.707106781f; // c4
    d2 = tmp13 + z1;                              // phase 5
    d6 = tmp13 - z1;

    // Odd part
    x10 = x4 + x5; // phase 2
    x11 = x5 + x6;
    x12 = x6 + x7;

    // The rotator is modified from fig 4-8 to avoid extra negations.
    const auto z5 = (x10 - x12) * 0.382683433f; // c6
    const auto z2 = x10 * 0.541196100f + z5;    // c2-c6
    const auto z4 = x12 * 1.306562965f + z5;    // c2+c6
    const auto z3 = x11 * 0.707106781f;         // c4

    const auto z11 = x7 + z3; // phase 5
    const auto z13 = x7 - z3;

    d5 = z13 + z2; // phase 6
    d3 = z13 - z2;
    d1 = z11 + z4;
    d7 = z11 - z4;
}

// Inverse transform constants
inline static constexpr int W1 = 2841;
inline static constexpr int W2 = 2676;
inline static constexpr int W3 = 2408;
inline static constexpr int W5 = 1609;
inline static constexpr int W6 = 1108;
inline static constexpr int W7 = 565;

static void inverse_rows_transform(int * block)
{
    int x1 = block[4] << 11;
    int x2 = block[6];
    int x3 = block[2];
    int x4 = block[1];
    int x5 = block[7];
    int x6 = block[5];
    int x7 = block[3];

    if ((x1 | x2 | x3 | x4 | x5 | x6 | x7) == 0) {
        block[0] = block[1] = block[2] = block[3] = block[4] = block[5] = block[6] = block[7] = block[0]
                << 3;
        return;
    }

    int x0 = (block[0] << 11) + 128;
    int x8 = W7 * (x4 + x5);
    x4 = x8 + (W1 - W7) * x4;
    x5 = x8 - (W1 + W7) * x5;
    x8 = W3 * (x6 + x7);
    x6 = x8 - (W3 - W5) * x6;
    x7 = x8 - (W3 + W5) * x7;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2);
    x2 = x1 - (W2 + W6) * x2;
    x3 = x1 + (W2 - W6) * x3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;

    block[0] = (x7 + x1) >> 8;
    block[1] = (x3 + x2) >> 8;
    block[2] = (x0 + x4) >> 8;
    block[3] = (x8 + x6) >> 8;
    block[4] = (x8 - x6) >> 8;
    block[5] = (x0 - x4) >> 8;
    block[6] = (x3 - x2) >> 8;
    block[7] = (x7 - x1) >> 8;
}

static unsigned char clip(const int x)
{
    if (x < 0) {
        return 0;
    }
    if (x > 0xFF) {
        return 0xFF;
    }
    return static_cast<unsigned char>(x);
}

static void inverse_column_transform(const int * blk, int stride, unsigned char * out)
{
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;
    if (!((x1 = blk[8 * 4] << 8) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) | (x4 = blk[8 * 1]) |
          (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3]))) {
        x1 = clip(((blk[0] + 32) >> 6) + 128);
        for (x0 = 8; x0; --x0) {
            *out = (unsigned char)x1;
            out += stride;
        }
        return;
    }
    x0 = (blk[0] << 8) + 8192;
    x8 = W7 * (x4 + x5) + 4;
    x4 = (x8 + (W1 - W7) * x4) >> 3;
    x5 = (x8 - (W1 + W7) * x5) >> 3;
    x8 = W3 * (x6 + x7) + 4;
    x6 = (x8 - (W3 - W5) * x6) >> 3;
    x7 = (x8 - (W3 + W5) * x7) >> 3;
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6 * (x3 + x2) + 4;
    x2 = (x1 - (W2 + W6) * x2) >> 3;
    x3 = (x1 + (W2 - W6) * x3) >> 3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181 * (x4 + x5) + 128) >> 8;
    x4 = (181 * (x4 - x5) + 128) >> 8;
    *out = clip(((x7 + x1) >> 14) + 128);
    out += stride;
    *out = clip(((x3 + x2) >> 14) + 128);
    out += stride;
    *out = clip(((x0 + x4) >> 14) + 128);
    out += stride;
    *out = clip(((x8 + x6) >> 14) + 128);
    out += stride;
    *out = clip(((x8 - x6) >> 14) + 128);
    out += stride;
    *out = clip(((x0 - x4) >> 14) + 128);
    out += stride;
    *out = clip(((x3 - x2) >> 14) + 128);
    out += stride;
    *out = clip(((x7 - x1) >> 14) + 128);
}

} // namespace

namespace utils {

void DiscreteCosineTransform::forward(std::array<float, 64> & block)
{
    // Application of discrete cosine transformation by rows.
    for (std::size_t i = 0; i < 64; i += 8) {
        forward_transform(block[i + 0], block[i + 1], block[i + 2], block[i + 3], block[i + 4], block[i + 5], block[i + 6], block[i + 7]);
    }

    // Application of discrete cosine transformation by columns
    for (std::size_t i = 0; i < 8; ++i) {
        forward_transform(block[i + 0], block[i + 8], block[i + 16], block[i + 24], block[i + 32], block[i + 40], block[i + 48], block[i + 56]);
    }

    // Descale the DCT coefficients
    for (std::size_t y = 0, j = 0; y < 8; ++y) {
        for (std::size_t x = 0; x < 8; ++x, ++j) {
            block[j] /= aan_scale_factors[y] * aan_scale_factors[x];
        }
    }
}

void DiscreteCosineTransform::inverse(std::array<int, 64> & block, int stride, unsigned char * out)
{
    for (int i = 0; i < 64; i += 8) {
        inverse_rows_transform(&block[i]);
    }
    for (int i = 0; i < 8; ++i) {
        inverse_column_transform(&block[i], stride, &out[i]);
    }
}

} // namespace utils
