#pragma once

#include <utils/bytes.hpp>
#include <vector>

class Output
{
public:
    void to_file(const std::string & file_name) const;

    void reset();

    const std::vector<unsigned char> & get() const;

    Output & write(unsigned short code, unsigned short lenght);

    template <std::size_t BytesCount>
    Output & operator<<(const Bytes<BytesCount> & bytes)
    {
        m_result.insert(m_result.end(), bytes.begin(), bytes.end());
        return *this;
    }

    Output & operator<<(const unsigned char value);

private:
    std::vector<unsigned char> m_result{};
    int m_bits_buffer = 0;
    int m_bits_count = 0;
};
