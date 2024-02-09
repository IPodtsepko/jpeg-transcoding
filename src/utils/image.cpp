#include <utils/image.hpp>

Image::Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const void * data)
    : Image(width, height, components_count, static_cast<const unsigned char *>(data))
{
}

Image::Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const unsigned char * data)
    : m_width(width)
    , m_height(height)
    , m_components_count(components_count)
    , m_red_component(data)
    , m_green_component(data + (components_count > 1 ? 1 : 0))
    , m_blue_component(data + (components_count > 1 ? 2 : 0))
{
}

std::size_t Image::get_width() const
{
    return m_width;
}

std::size_t Image::get_height() const
{
    return m_height;
}

std::size_t Image::get_components_count() const
{
    return m_components_count;
}

std::tuple<std::size_t, std::size_t, std::size_t> Image::get(const std::size_t row, const std::size_t column) const
{
    // Дополнение блоков, если размеры изображения не кратны размеру блока
    const auto fixed_row = row >= m_height ? m_height - 1 : row;
    const auto fixed_column = column >= m_width ? m_width - 1 : column;

    const auto position = (fixed_row * m_width + fixed_column) * m_components_count;

    return get(position);
}

std::tuple<std::size_t, std::size_t, std::size_t> Image::get(const std::size_t position) const
{
    return {get_red(position),
            get_green(position),
            get_blue(position)};
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

std::size_t Image::get(const unsigned char * component, const std::size_t position) const
{
    return static_cast<std::size_t>(component[position]);
}
