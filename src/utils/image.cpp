#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <utils/image.hpp>

namespace utils {

Image::Image(const std::size_t width, const std::size_t height, const std::size_t components_count, std::vector<char> && data)
    : m_width(width)
    , m_height(height)
    , m_components_count(components_count)
    , m_data(std::move(data))
    , m_red_component(get_bytes_ptr())
    , m_green_component(get_bytes_ptr() + (components_count > 1 ? 1 : 0))
    , m_blue_component(get_bytes_ptr() + (components_count > 1 ? 2 : 0))
{
}

Image::Image(Image && other)
{
    Image tmp{};
    swap(other, tmp);
    swap(*this, tmp);
}

Image & Image::operator=(Image && rhs)
{
    Image tmp{};
    swap(rhs, tmp);
    return swap(*this, tmp);
}

namespace {
bool is_ppm_file(const std::string & file_name)
{
    return (file_name.size() >= 4 && file_name.substr(file_name.size() - 4) == ".ppm");
}

std::size_t get_components_count_by_ppm_format(const std::string & format)
{
    if (format == "P5") {
        return 1;
    }
    if (format == "P6") {
        return 3;
    }
    throw std::runtime_error(fmt::format("Unsupported ppm format: '{}'", format));
}

std::ifstream open_file(const std::string & file_name)
{
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening input file: " + file_name);
    }
    return file;
}

std::vector<char> read_bytes(const std::size_t bytes_count,
                             std::ifstream & input)
{
    std::vector<char> buffer;
    buffer.resize(bytes_count);
    if (!input.read(buffer.data(), bytes_count)) {
        throw std::runtime_error("Failed to read input file properly.");
    }
    return buffer;
}

} // namespace

Image Image::from_file(std::size_t width, std::size_t height, std::size_t components_count, const std::string & file_name)
{
    const auto bytes_count = components_count * width * height;
    auto file = open_file(file_name);
    return {width, height, components_count, read_bytes(bytes_count, file)};
}

Image Image::from_ppm(const std::string & file_name)
{
    if (!is_ppm_file(file_name)) {
        throw std::runtime_error(fmt::format("Expected .ppm file: {}", file_name));
    }

    auto file = open_file(file_name);

    std::string format;
    std::size_t width;
    std::size_t height;
    std::size_t max_color_value;

    file >> format >> width >> height >> max_color_value;
    file.ignore(1, '\n');

    std::size_t components_count = get_components_count_by_ppm_format(format);
    std::size_t bytes_count = width * height * components_count;

    return {width, height, components_count, read_bytes(bytes_count, file)};
}

std::size_t Image::get_width() const { return m_width; }

std::size_t Image::get_height() const { return m_height; }

std::size_t Image::get_components_count() const { return m_components_count; }

Image::RGBPixel Image::get_rgb(const std::size_t row, const std::size_t column) const
{
    // Дополнение блоков, если размеры изображения не кратны размеру блока
    const auto fixed_row = row >= m_height ? m_height - 1 : row;
    const auto fixed_column = column >= m_width ? m_width - 1 : column;

    const auto position =
            (fixed_row * m_width + fixed_column) * m_components_count;

    return get(position);
}

Image::RGBPixel Image::get(const std::size_t position) const
{
    return {get_red(position), get_green(position), get_blue(position)};
}

namespace {

static Image::YUVPixel to_yuv(const Image::RGBPixel & rgb)
{
    const float r = rgb.m_red;
    const float g = rgb.m_green;
    const float b = rgb.m_blue;

    return {+0.29900f * r + 0.58700f * g + 0.11400f * b - 128,
            -0.16874f * r - 0.33126f * g + 0.50000f * b,
            +0.50000f * r - 0.41869f * g - 0.08131f * b};
}

} // namespace

Image::YUVPixel Image::get_yuv(const std::size_t row, const std::size_t column) const
{
    return to_yuv(get_rgb(row, column));
}

std::size_t Image::get_red(const std::size_t position) const
{
    return get(m_red_component, position);
}

std::size_t Image::get_green(const std::size_t position) const
{
    return get(m_green_component, position);
}

std::size_t Image::get_blue(const std::size_t position) const
{
    return get(m_blue_component, position);
}

std::size_t Image::get(const unsigned char * component,
                       const std::size_t position) const
{
    return static_cast<std::size_t>(component[position]);
}

const Byte * Image::get_bytes_ptr() const
{
    return static_cast<const unsigned char *>(static_cast<const void *>(m_data.data()));
}

Image & swap(Image & lhs, Image & rhs)
{
    std::swap(lhs.m_width, rhs.m_width);
    std::swap(lhs.m_height, rhs.m_height);
    std::swap(lhs.m_components_count, rhs.m_components_count);
    std::swap(lhs.m_data, rhs.m_data);
    std::swap(lhs.m_red_component, rhs.m_red_component);
    std::swap(lhs.m_green_component, rhs.m_green_component);
    std::swap(lhs.m_blue_component, rhs.m_blue_component);

    return lhs;
}

} // namespace utils
