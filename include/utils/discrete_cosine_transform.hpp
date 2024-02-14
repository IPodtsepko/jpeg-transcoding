#pragma once

#include <array>

namespace utils {

/**
 * @brief A class that provides an interface for performing forward and inverse
 * discrete cosine transformations.
 */
class DiscreteCosineTransform
{
public:
    /**
     * @brief Apply a forward discrete cosine transform.
     *
     * @param block
     * @param stride
     */
    static void forward(std::array<float, 64> & block);

    /**
     * @brief Apply inverse discrete cosine transform.
     *
     * @param block
     * @param stride
     * @param out
     */
    static void inverse(std::array<int, 64> & block, int stride, unsigned char * out);
};

} // namespace utils
