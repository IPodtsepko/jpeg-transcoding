#pragma once

#include <tuple>

class Image
{
public:
    Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const void * data);
    Image(const std::size_t width, const std::size_t height, const std::size_t components_count, const unsigned char * data);

    std::size_t get_width() const;
    std::size_t get_height() const;
    std::size_t get_components_count() const;

    std::tuple<std::size_t, std::size_t, std::size_t> get(const std::size_t row, const std::size_t column) const;
    std::tuple<std::size_t, std::size_t, std::size_t> get(const std::size_t position) const;

    std::size_t get_red(const std::size_t position) const;
    std::size_t get_green(const std::size_t position) const;
    std::size_t get_blue(const std::size_t position) const;

private:
    std::size_t get(const unsigned char * component, const std::size_t position) const;

private:
    const std::size_t m_width;
    const std::size_t m_height;
    const std::size_t m_components_count;
    const unsigned char * m_red_component;
    const unsigned char * m_green_component;
    const unsigned char * m_blue_component;
};
