#include <fstream>
#include <iostream>
#include <utils/output.hpp>

void Output::to_file(const std::string & file_name) const
{
    std::ofstream file{file_name, std::ios::binary};
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output file " + file_name);
    }
    file.write(reinterpret_cast<const char *>(m_result.data()), m_result.size());
    file.close();
}

void Output::reset()
{
    if (m_bits_buffer != 0 || m_bits_count != 0) {
        std::wcout << "Reset output with non empty buffer\n";
    }
    m_bits_buffer = 0;
    m_bits_count = 0;
}

const std::vector<unsigned char> & Output::get() const { return m_result; }

Output & Output::write(unsigned short code, unsigned short lenght)
{
    m_bits_count += lenght;
    m_bits_buffer |= code << (24 - m_bits_count);
    while (m_bits_count >= 8) {
        Byte byte_to_write = (m_bits_buffer >> 16) & 0xFF;
        auto & self = *this;
        self << byte_to_write;
        if (byte_to_write == 0xFF) {
            self << static_cast<char>(0x00);
        }
        m_bits_buffer <<= 8;
        m_bits_count -= 8;
    }
    return *this;
}

Output & Output::operator<<(const unsigned char value)
{
    m_result.push_back(value);
    return *this;
}
