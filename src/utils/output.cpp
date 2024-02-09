#include <fstream>
#include <utils/output.hpp>

void Output::to_file(const std::string & file_name) const
{
    std::ofstream file{file_name, std::ios::binary};
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open result.jpg");
    }
    file.write(reinterpret_cast<const char *>(m_result.data()), m_result.size());
    file.close();
}

void Output::reset()
{
    m_bits_buffer = 0;
    m_bits_count = 0;
}

const std::vector<unsigned char> & Output::get() const
{
    return m_result;
}

void Output::write(const unsigned short bits[])
{
    m_bits_count += bits[1];
    m_bits_buffer |= bits[0] << (24 - m_bits_count);
    while (m_bits_count >= 8) {
        unsigned char byte_to_write = (m_bits_buffer >> 16) & 0xFF;
        auto & self = *this;
        self << byte_to_write;
        if (byte_to_write == 0xFF) {
            self << static_cast<char>(0x00);
        }
        m_bits_buffer <<= 8;
        m_bits_count -= 8;
    }
}

Output & Output::operator<<(const unsigned short bits[])
{
    write(bits);
    return *this;
}

Output & Output::operator<<(const unsigned char value)
{
    m_result.push_back(value);
    return *this;
}
