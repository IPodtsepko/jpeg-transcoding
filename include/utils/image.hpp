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
    struct RGBPixel
    {
        std::size_t m_red;
        std::size_t m_green;
        std::size_t m_blue;
    };

    struct YUVPixel
    {
        float m_luminance;
        float m_chrominance_blue;
        float m_chrominance_red;
    };

    Image(const Image &) = delete;
    Image(Image && other);

    Image & operator=(Image && lhs);

    /**
     * @brief Constructor for initializing the Image object.
     *
     * @param width The width of the image.
     * @param height The height of the image.
     * @param components_count The number of color components in the image (e.g., 3 for RGB).
     * @param data Image data.
     */
    Image(std::size_t width, std::size_t height, std::size_t components_count, std::vector<char> && data);

    /**
     * @brief Reads image from file and returns as object.
     *
     * @param width The width of the image.
     * @param height The height of the image.
     * @param components_count The number of color components in the image (e.g., 3 for RGB).
     * @param file_name Name of file with image.
     * @return Image readed from file.
     */
    static Image from_file(std::size_t width, std::size_t height, std::size_t components_count, const std::string & file_name);

    /**
     * @brief Reads image from .ppm file and returns as object.
     *
     * @param file_name Name of file with image.
     * @return Image readed from file.
     */
    static Image from_ppm(const std::string & file_name);

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
    RGBPixel get_rgb(std::size_t row, std::size_t column) const;

    /**
     * @brief Get the YUV components at the specified row and column.
     *
     * @param row The row index.
     * @param column The column index.
     * @return A tuple representing the YUV components at the specified position.
     */
    YUVPixel get_yuv(std::size_t row, std::size_t column) const;

    /**
     * @brief Get the RGB components with the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return A tuple representing the RGB components at the specified position.
     */
    RGBPixel get(std::size_t position) const;

    /**
     * @brief Get the red component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The red component value.
     */
    std::size_t get_red(std::size_t position) const;

    /**
     * @brief Get the green component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The green component value.
     */
    std::size_t get_green(std::size_t position) const;

    /**
     * @brief Get the blue component at the specified index in a linearized
     * representation.
     *
     * @param position The index in a linearized representation.
     * @return The blue component value.
     */
    std::size_t get_blue(std::size_t position) const;

private:
    Image() = default;

    std::size_t get(const unsigned char * component,
                    const std::size_t position) const;

    const Byte * get_bytes_ptr() const;

    friend Image & swap(Image & lhs, Image & rhs);

private:
    std::size_t m_width = 0;
    std::size_t m_height = 0;
    std::size_t m_components_count = 0;
    std::vector<char> m_data{};
    const Byte * m_red_component = nullptr;
    const Byte * m_green_component = nullptr;
    const Byte * m_blue_component = nullptr;
};

} // namespace utils
