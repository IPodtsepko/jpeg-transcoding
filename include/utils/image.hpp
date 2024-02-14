#pragma once

#include <tuple>
#include <utils/bytes.hpp>

namespace utils {

/**
 * @brief A class representing an image.
 */
class Image
{
public:
    /**
     * @brief Constructor for initializing the Image object.
     *
     * @param width The width of the image.
     * @param height The height of the image.
     * @param components_count The number of color components in the image (e.g.,
     * 3 for RGB).
     * @param data A pointer to the image data.
     */
    Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const void * data);

    /**
     * @brief Constructor for initializing the Image object.
     *
     * @param width The width of the image.
     * @param height The height of the image.
     * @param components_count The number of color components in the image (e.g.,
     * 3 for RGB).
     * @param data A pointer to the image data.
     */
    Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const Byte * data);

    /**
     * @brief Get the width of the image.
     *
     * @return The width of the image.
     */
    std::size_t get_width() const;

    /**
     * @brief Get the height of the image.
     *
     * @return The height of the image.
     */
    std::size_t get_height() const;

    /**
     * @brief Get the number of color components in the image.
     *
     * @return The number of color components.
     */
    std::size_t get_components_count() const;

    /**
     * @brief Get the RGB components at the specified row and column.
     *
     * @param row The row index.
     * @param column The column index.
     * @return A tuple representing the RGB components at the specified position.
     */
    std::tuple<std::size_t, std::size_t, std::size_t>
    get(const std::size_t row, const std::size_t column) const;

    /**
     * @brief Get the RGB components with the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return A tuple representing the RGB components at the specified position.
     */
    std::tuple<std::size_t, std::size_t, std::size_t>
    get(const std::size_t position) const;

    /**
     * @brief Get the red component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The red component value.
     */
    std::size_t get_red(const std::size_t position) const;

    /**
     * @brief Get the green component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The green component value.
     */
    std::size_t get_green(const std::size_t position) const;

    /**
     * @brief Get the blue component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The blue component value.
     */
    std::size_t get_blue(const std::size_t position) const;

private:
    std::size_t get(const unsigned char * component,
                    const std::size_t position) const;

private:
    const std::size_t m_width;
    const std::size_t m_height;
    const std::size_t m_components_count;
    const Byte * m_red_component;
    const Byte * m_green_component;
    const Byte * m_blue_component;
};

} // namespace utils
