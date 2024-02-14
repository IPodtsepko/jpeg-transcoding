#pragma once

#include <utils/bytes.hpp>
#include <utils/huffman_code.hpp>
#include <vector>

class Output
{
public:
    void to_file(const std::string & file_name) const;

    void reset();

    const std::vector<unsigned char> & get() const;

    void write(const unsigned short bits[]);
    void write(const utils::HuffmanCode::Entry & entry);

    Output & operator<<(const unsigned short bits[]);
    Output & operator<<(const utils::HuffmanCode::Entry & entry);

    template <std::size_t BytesCount>
    Output & operator<<(const Bytes<BytesCount> & bytes)
    {
        m_result.insert(m_result.end(), bytes.begin(), bytes.end());
        return *this;
    }

    Output & operator<<(const unsigned char value);

private:
    std::vector<unsigned char> m_result;
    int m_bits_buffer;
    int m_bits_count;
};
