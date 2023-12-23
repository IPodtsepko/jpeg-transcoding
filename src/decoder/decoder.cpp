/**
 * @file decoder.cpp
 * @brief Implementation of JPEG Transcoder.
 * @author Igor Podtsepko (i.podtsepko@niuitmo.ru)
 * @date 2024
 */

#include "decoder/decoder.hpp"

#include "decoder/decoding_exception.hpp"
#include "utils/discrete_cosine_transform.hpp"

Decoder & Decoder::set_dct_filter(const std::size_t dct_filter_power)
{
    m_dct_filter_power = dct_filter_power;
    return *this;
}

Decoder & Decoder::toggle_mode(const Mode & mode)
{
    m_mode = mode;
    return *this;
}

Decoder & Decoder::set_enhanced_file(const std::string & enhanced_file_name)
{
    m_enhanced_file = utils::Image::from_ppm(enhanced_file_name);
    return *this;
}

unsigned char Decoder::clip(const int x)
{
    if (x < 0) {
        return 0;
    }
    if (x > 0xFF) {
        return 0xFF;
    }
    return static_cast<unsigned char>(x);
}

bool Decoder::IsDefaultMode() const
{
    return m_mode == Mode::DEFAULT;
}

bool Decoder::IsZeroOutAndDecodeMode() const
{
    return m_mode == Mode::ZERO_OUT_AND_DECODE;
}

bool Decoder::IsEncodeResidualsMode() const
{
    return m_mode == Mode::ENCODE_RESIDUALS;
}

bool Decoder::IsDecodeResidualsMode() const
{
    return m_mode == Mode::DECODE_RESIDUALS;
}

bool Decoder::IsResidualsProcessing() const
{
    return IsEncodeResidualsMode() || IsDecodeResidualsMode();
}

unsigned char Decoder::get_bytes(const std::size_t count)
{
    if (m_size < count) {
        throw DecodingException("The bitstream is expected to continue", DecodingException::Reason::SYNTAX_ERROR);
    }
    const auto * begin = m_position;
    m_position += count;
    m_size -= count;
    if (IsResidualsProcessing() && !m_is_scanning) {
        for (auto * byte = begin; byte != m_position != 0; ++byte) {
            m_output << *byte;
        }
    }
    return *begin;
}

int Decoder::read_bits(const std::size_t bits)
{
    if (bits == 0) {
        return 0;
    }

    while (m_bits_in_buffer < bits) {
        if (m_size == 0) {
            m_buffer = (m_buffer << 8) | 0xFF;
            m_bits_in_buffer += 8;
            continue;
        }
        const auto byte = get_bytes();
        m_bits_in_buffer += 8;
        m_buffer = (m_buffer << 8) | byte;
        if (byte == 0xFF) {
            const auto marker = get_bytes();
            switch (marker) {
            case 0x00:
            case 0xFF:
                break;
            case 0xD9:
                m_size = 0;
                break;
            default:
                if ((marker & 0xF8) != 0xD0)
                    throw DecodingException("Invalid marker", DecodingException::Reason::SYNTAX_ERROR);
                else {
                    m_buffer = (m_buffer << 8) | marker;
                    m_bits_in_buffer += 8;
                }
            }
        }
    }

    const auto offset = m_bits_in_buffer - bits;
    const auto mask = (1 << bits) - 1;

    return (m_buffer >> offset) & mask;
}

void Decoder::skip_bits(const std::size_t bits)
{
    if (m_bits_in_buffer < bits) {
        read_bits(bits);
    }
    m_bits_in_buffer -= bits;
}

int Decoder::get_bits(int bits)
{
    const int res = read_bits(bits);
    skip_bits(bits);
    return res;
}

void Decoder::byte_align()
{
    m_bits_in_buffer &= 0xF8;
}

void Decoder::skip(const std::size_t count)
{
    get_bytes(count);
    m_length -= count;
}

unsigned short Decoder::decode_16(const unsigned char * pos)
{
    return (pos[0] << 8) | pos[1];
}

void Decoder::decode_length()
{
    if (m_size < 2) {
        throw DecodingException("Cannot decode lenght", DecodingException::Reason::SYNTAX_ERROR);
    }
    m_length = decode_16(m_position);
    if (m_length > m_size) {
        throw DecodingException("Lenght is too long", DecodingException::Reason::SYNTAX_ERROR);
    }
    skip(2);
}

void Decoder::skip_marker()
{
    decode_length();
    skip(m_length);
}

bool Decoder::is_point_of_two(const std::size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

void Decoder::decode_start_of_frame()
{
    decode_length();
    if (m_length < 9) {
        throw DecodingException("Lenght of SOS is too small", DecodingException::Reason::SYNTAX_ERROR);
    }
    if (m_position[0] != 8) {
        throw DecodingException("Unsupported format", DecodingException::Reason::UNSUPPORTED);
    }
    m_height = decode_16(m_position + 1);
    m_width = decode_16(m_position + 3);
    if (m_width == 0 || m_height == 0) {
        throw DecodingException("Zero image size", DecodingException::Reason::UNSUPPORTED);
    }
    const auto components_count = m_position[5];
    skip(6);

    if (components_count != 1 && components_count != 3) {
        throw DecodingException(fmt::format("Invalid components count: {}", components_count),
                                DecodingException::Reason::SYNTAX_ERROR);
    }

    if (m_length < components_count * 3) {
        throw DecodingException("Incomplete image channels description", DecodingException::Reason::SYNTAX_ERROR);
    }
    m_components.resize(components_count);
    for (auto & component : m_components) {
        component.set_id(m_position[0])
                .set_sampling(m_position[1])
                .set_quantization_table(m_position[2])
                .verify();
        skip(3);
        m_sampling.set_greater(component.m_sampling);
    }

    const Shape blocks_shape{get_blocks_count(m_width, m_sampling.m_y),
                             get_blocks_count(m_height, m_sampling.m_x)};
    for (auto & c : m_components) {
        c.m_width = (m_width * c.m_sampling.m_y + m_sampling.m_y - 1) / m_sampling.m_y;
        c.m_height = (m_height * c.m_sampling.m_x + m_sampling.m_x - 1) / m_sampling.m_x;
        c.m_stride = blocks_shape.m_width * c.m_sampling.m_y << 3;
        if (((c.m_width < 3) && (c.m_sampling.m_y != m_sampling.m_y)) ||
            ((c.m_height < 3) && (c.m_sampling.m_x != m_sampling.m_x)))
            throw DecodingException("Unsupported image format", DecodingException::Reason::UNSUPPORTED);
        c.m_pixels = BytesList(c.m_stride * blocks_shape.m_height * c.m_sampling.m_x << 3);
    }
    if (components_count == 3) {
        m_rgb = BytesList(m_width * m_height * components_count);
    }

    skip(m_length);
}

void Decoder::restore_huffman_codes(Bytes<17> spectrum, std::vector<std::pair<unsigned short, unsigned short>> & to)
{
    const int initial_length = 0;
    const int initial_code = 0;
    restore_huffman_codes(spectrum, initial_length, initial_code, to);
}

void Decoder::restore_huffman_codes(Bytes<17> & spectrum, int length, int code, std::vector<std::pair<unsigned short, unsigned short>> & to)
{
    if (length > 16) {
        return;
    }

    if (spectrum[length] > 0) {
        std::pair<unsigned short, unsigned short> entry;
        entry.first = static_cast<unsigned short>(code);
        entry.second = static_cast<unsigned short>(length);
        to.emplace_back(std::move(entry));
        --spectrum[length];
        return;
    }

    length += 1;
    code <<= 1;
    restore_huffman_codes(spectrum, length, code, to);
    restore_huffman_codes(spectrum, length, code | 1, to);
}

void Decoder::decode_huffman_tables()
{
    decode_length();
    while (m_length >= 17) {
        int i = m_position[0];
        if (i & 0xEC) {
            throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
        }
        if (i & 0x02) {
            throw DecodingException("Unsupported image format", DecodingException::Reason::UNSUPPORTED);
        }
        i = (i | (i >> 3)) & 3; // combined DC/AC + tableid value
        static Bytes<17> counts;
        int total_codes_count = 0;
        for (int code_length = 1; code_length <= 16; ++code_length) {
            const auto count = m_position[code_length];
            counts[code_length] = count;
            total_codes_count += count;
        }
        skip(17);

        std::vector<std::pair<unsigned short, unsigned short>> restored_codes;
        restored_codes.reserve(total_codes_count);
        restore_huffman_codes(counts, restored_codes);

        std::size_t code_used = 0;

        auto & restored_table = m_huffman_encoding_tables[i];

        auto * huffman_table = m_huffman_tables[i];
        int entry_id = 0;
        int remain = 65536;
        int spread = 65536;
        for (int code_length = 1; code_length <= 16; ++code_length) {
            spread >>= 1;
            const auto current_count = counts[code_length];
            if (!current_count) {
                continue;
            }
            if (m_length < current_count) {
                throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
            }
            remain -= current_count << (16 - code_length);
            if (remain < 0) {
                throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
            }
            for (int i = 0; i < current_count; ++i) {
                const auto value = m_position[i];

                auto & entry = restored_codes[code_used++];
                restored_table[value].m_code = entry.first;
                restored_table[value].m_length = entry.second;

                for (int j = 0; j < spread; ++j) {
                    huffman_table[entry_id] = HuffmanCodeEntry{
                            static_cast<unsigned char>(code_length), // m_bits
                            value                                    // m_decoded_value
                    };
                    ++entry_id;
                }
            }

            skip(current_count);
        }
        while (remain--) {
            huffman_table[entry_id].m_length = 0;
            ++entry_id;
        }
    }
    if (m_length > 0) {
        throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
    }
}

void Decoder::decode_quantize_tables()
{
    decode_length();
    while (m_length >= 65) {
        const std::size_t id = m_position[0];
        skip(1);
        if (id > 4) {
            throw DecodingException(fmt::format("Invalid quantization table id: {}", id),
                                    DecodingException::Reason::SYNTAX_ERROR);
        }

        static constexpr std::size_t n = 64;
        int data[64];
        unsigned char uc_data[64];
        std::copy(m_position, m_position + n, uc_data);
        for (std::size_t i = 0; i < 64; ++i) {
            data[i] = static_cast<int>(uc_data[i]);
        }
        m_quantization_tables.emplace(id, data);
        skip(n);
    }
    if (m_length != 0) {
        throw DecodingException("Declared block length of the quantization tables is too long",
                                DecodingException::Reason::SYNTAX_ERROR);
    }
}

void Decoder::decode_dri()
{
    decode_length();
    if (m_length < 2) {
        throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
    }
    m_rst_interval = decode_16(m_position);
    skip(m_length);
}

Decoder::HuffmanDecodingResult Decoder::decode_huffman(HuffmanCodeEntry huffman_table[], const std::size_t index, const utils::Mask & mask)
{
    HuffmanDecodingResult result;

    while (true) {
        // Decode run and length
        const auto encoded = read_bits(16);

        const auto entry = huffman_table[encoded];
        if (entry.m_length == 0) {
            throw DecodingException("A codeword in the Huffman code cannot have a length of 0",
                                    DecodingException::Reason::SYNTAX_ERROR);
        }
        skip_bits(entry.m_length); // skip decoded code word

        const auto decoded_value = entry.m_decoded_value;

        result.m_run += decoded_value >> 4;
        result.m_level = decoded_value & 0b1111;

        if (result.m_level == 0) {
            if (result.m_run == 0) {
                return result; // End of block marker
            }
            ++result.m_run; // Sixteen zeros marker found

            continue;
        }

        if (mask[index + result.m_run]) {
            result.m_coefficient = get_bits(result.m_level);
            if (result.m_coefficient < (1 << (result.m_level - 1))) {
                result.m_coefficient += ((-1) << result.m_level) + 1;
            }
        }

        return result;
    }
}

void Decoder::decode_block(Component & component, unsigned char * output, utils::DCTCoefficientsFilter & filter, const std::optional<std::array<int, 64>> optional_enhanced_block)
{
    const auto mask = !IsDefaultMode() && component.m_id == 1 ? filter.get_mask() : utils::MaskAll;

    std::array<int, 64> block;
    block.fill(0);

    // Decode DC
    auto * dc_huffman_table = m_huffman_tables[component.m_dc_huffman_table_id];
    const auto dc = decode_huffman(dc_huffman_table);

    block[0] = component.m_last_dc + dc.m_coefficient;
    if (component.m_id == 1) {
        m_dct_coefficients_distribution[0].push_back(block[0]);
    }

    // Decode AC
    auto * ac_huffman_table = m_huffman_tables[component.m_ac_huffman_table_id];
    for (std::size_t i = 1; i < 64; ++i) {
        auto ac = decode_huffman(ac_huffman_table, i, utils::MaskAll);

        if (ac.m_level == 0 && ac.m_run == 0) {
            break; // End of block
        }

        i += ac.m_run;

        if (i > 63) {
            throw DecodingException(fmt::format("Run goes beyond the boundaries of the block: {}", i),
                                    DecodingException::Reason::SYNTAX_ERROR);
        }

        if (IsResidualsProcessing()) {
            block[i] = ac.m_coefficient;
        }
        else {
            block[utils::REVERSED_ZIGZAG_ORDER[i]] = ac.m_coefficient;
        }

        if (component.m_id == 1) {
            m_dct_coefficients_distribution[i].push_back(ac.m_coefficient);
        }
    }

    if (IsResidualsProcessing()) {
        if (optional_enhanced_block.has_value()) {
            if (component.m_id != 1) {
                throw DecodingException("Enhanced block provided for Cr/Cb component",
                                        DecodingException::Reason::INTERNAL_ERROR);
            }
            const auto & enhanced_block = optional_enhanced_block.value();
            for (std::size_t i = 1; i < 64; ++i) {
                if (mask[i]) {
                    continue;
                }
                if (IsEncodeResidualsMode()) {
                    block[i] -= enhanced_block[i];
                }
                else {
                    block[i] += enhanced_block[i];
                }
            }
        }
        component.m_huffman_code.encode(block, component.m_last_dc, m_output);
    }
    else {
        if (IsZeroOutAndDecodeMode()) {
            for (std::size_t i = 0; i < block.size(); ++i) {
                if (!mask[i]) {
                    block[i] = 0;
                }
            }
        }
        m_quantization_tables.at(component.m_quantization_table_id).inverse(block);
        utils::DiscreteCosineTransform::inverse(block, component.m_stride, output);
    }

    component.m_last_dc += dc.m_coefficient;
}

std::size_t Decoder::get_blocks_count(const std::size_t size, std::size_t sampling)
{
    const auto block_size = sampling << 3;
    return (size + block_size - 1) / block_size;
}

std::optional<std::array<int, 64>> Decoder::get_enhanced_coefficients(const Component & component, const std::size_t x, const std::size_t y)
{
    if (!IsResidualsProcessing() || component.m_id != 1) {
        return std::nullopt;
    }
    std::array<float, 64> image_fragment;
    for (std::size_t pixel_x = 0, k = 0; pixel_x < 8; ++pixel_x) {
        for (std::size_t pixel_y = 0; pixel_y < 8; ++pixel_y, ++k) {
            image_fragment[k] = m_enhanced_file->get_yuv(x + pixel_x, y + pixel_y).m_luminance;
        }
    }
    utils::DiscreteCosineTransform::forward(image_fragment);
    return m_quantization_tables.at(component.m_quantization_table_id).forward(image_fragment);
}

void Decoder::decode_start_of_scan()
{
    int rst_count = m_rst_interval, next_rst = 0;
    decode_length();
    if (m_length < (4 + 2 * m_components.size()))
        throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
    if (m_position[0] != m_components.size())
        throw DecodingException("Unsupported image format", DecodingException::Reason::UNSUPPORTED);
    skip(1);
    for (auto & c : m_components) {
        if (m_position[0] != c.m_id)
            throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
        if (m_position[1] & 0xEE)
            throw DecodingException("Syntax error", DecodingException::Reason::SYNTAX_ERROR);
        c.m_dc_huffman_table_id = m_position[1] >> 4;
        c.m_ac_huffman_table_id = (m_position[1] & 1) | 2;

        c.m_huffman_code = utils::HuffmanCode(m_huffman_encoding_tables[c.m_dc_huffman_table_id],
                                              m_huffman_encoding_tables[c.m_ac_huffman_table_id]);

        skip(2);
    }
    if (m_position[0] || (m_position[1] != 63) || m_position[2]) {
        throw DecodingException("Unsupported image format", DecodingException::Reason::UNSUPPORTED);
    }
    skip(m_length);
    m_is_scanning = true;
    m_output.reset();

    const auto x_blocks_count = get_blocks_count(m_height, m_sampling.m_x);
    const auto y_blocks_count = get_blocks_count(m_width, m_sampling.m_y);

    utils::DCTCoefficientsFilter filter(m_dct_filter_power);
    for (std::size_t global_block_x = 0; global_block_x < x_blocks_count; ++global_block_x) {
        for (std::size_t global_block_y = 0; global_block_y < y_blocks_count; ++global_block_y) {
            for (auto & component : m_components) {
                for (std::size_t block_x = 0; block_x < component.m_sampling.m_x; ++block_x) {
                    for (std::size_t block_y = 0; block_y < component.m_sampling.m_y; ++block_y) {
                        const auto x = (global_block_x * component.m_sampling.m_x + block_x) * 8;
                        const auto y = (global_block_y * component.m_sampling.m_y + block_y) * 8;

                        auto * out = &component.m_pixels[x * component.m_stride + y];

                        decode_block(component, out, filter, get_enhanced_coefficients(component, x, y));
                    }
                }
            }
            if (m_rst_interval > 0 && --rst_count == 0) {
                byte_align();
                const auto i = get_bits(16);
                if (((i & 0xFFF8) != 0xFFD0) || ((i & 7) != next_rst)) {
                    throw DecodingException("Invalid RST", DecodingException::Reason::SYNTAX_ERROR);
                }
                next_rst = (next_rst + 1) & 7;
                rst_count = m_rst_interval;
                for (auto & component : m_components) {
                    component.m_last_dc = 0;
                }
            }
        }
    }

    if (IsResidualsProcessing()) {
        m_output.write(0b1111111, 7) // Do the bit alignment of the EOI marker
                << 0xFF << 0xD9;
    }

    m_decoding_finished = true;
}

#define CF4A (-9)
#define CF4B (111)
#define CF4C (29)
#define CF4D (-3)
#define CF3A (28)
#define CF3B (109)
#define CF3C (-9)
#define CF3X (104)
#define CF3Y (27)
#define CF3Z (-3)
#define CF2A (139)
#define CF2B (-11)
#define CF(x) clip(((x) + 64) >> 7)

void Decoder::horizontal_upsample(Component & component)
{
    const int xmax = component.m_width - 3;
    unsigned char *lin, *lout;
    BytesList out((component.m_width * component.m_height) << 1);
    lin = component.m_pixels.data();
    lout = out.data();
    for (int y = component.m_height; y; --y) {
        lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
        lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
        lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
        for (int x = 0; x < xmax; ++x) {
            lout[(x << 1) + 3] =
                    CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
            lout[(x << 1) + 4] =
                    CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
        }
        lin += component.m_stride;
        lout += component.m_width << 1;
        lout[-3] = CF(CF3A * lin[-1] + CF3B * lin[-2] + CF3C * lin[-3]);
        lout[-2] = CF(CF3X * lin[-1] + CF3Y * lin[-2] + CF3Z * lin[-3]);
        lout[-1] = CF(CF2A * lin[-1] + CF2B * lin[-2]);
    }
    component.m_width <<= 1;
    component.m_stride = component.m_width;
    component.m_pixels = out;
}

void Decoder::vertical_upsample(Component & c)
{
    const int w = c.m_width, s1 = c.m_stride, s2 = s1 + s1;
    unsigned char *cin, *cout;
    int x, y;
    BytesList out((c.m_width * c.m_height) << 1);
    for (x = 0; x < w; ++x) {
        cin = &c.m_pixels[x];
        cout = &out[x];
        *cout = CF(CF2A * cin[0] + CF2B * cin[s1]);
        cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[s1] + CF3Z * cin[s2]);
        cout += w;
        *cout = CF(CF3A * cin[0] + CF3B * cin[s1] + CF3C * cin[s2]);
        cout += w;
        cin += s1;
        for (y = c.m_height - 3; y; --y) {
            *cout = CF(CF4A * cin[-s1] + CF4B * cin[0] + CF4C * cin[s1] + CF4D * cin[s2]);
            cout += w;
            *cout = CF(CF4D * cin[-s1] + CF4C * cin[0] + CF4B * cin[s1] + CF4A * cin[s2]);
            cout += w;
            cin += s1;
        }
        cin += s1;
        *cout = CF(CF3A * cin[0] + CF3B * cin[-s1] + CF3C * cin[-s2]);
        cout += w;
        *cout = CF(CF3X * cin[0] + CF3Y * cin[-s1] + CF3Z * cin[-s2]);
        cout += w;
        *cout = CF(CF2A * cin[0] + CF2B * cin[-s1]);
    }
    c.m_height <<= 1;
    c.m_stride = c.m_width;
    c.m_pixels = out;
}

void Decoder::convert()
{
    for (auto & component : m_components) {
        while ((component.m_width < m_width) || (component.m_height < m_height)) {
            if (component.m_width < m_width)
                horizontal_upsample(component);
            if (component.m_height < m_height)
                vertical_upsample(component);
        }
        if (component.m_width < m_width || component.m_height < m_height) {
            throw DecodingException("Internal application error occured", DecodingException::Reason::INTERNAL_ERROR);
        }
    }

    if (m_components.size() == 3) {
        // convert to RGB
        auto * prgb = m_rgb.data();
        const auto * py = m_components[0].m_pixels.data();
        const auto * pcb = m_components[1].m_pixels.data();
        const auto * pcr = m_components[2].m_pixels.data();
        for (int y = m_height; y; --y) {
            for (std::size_t x = 0; x < m_width; ++x) {
                const auto y = py[x] << 8;
                const auto cb = pcb[x] - 128;
                const auto cr = pcr[x] - 128;
                *prgb++ = clip((y + 359 * cr + 128) >> 8);
                *prgb++ = clip((y - 88 * cb - 183 * cr + 128) >> 8);
                *prgb++ = clip((y + 454 * cb + 128) >> 8);
            }
            py += m_components[0].m_stride;
            pcb += m_components[1].m_stride;
            pcr += m_components[2].m_stride;
        }
        return;
    }

    auto & component = m_components[0];
    if (component.m_width == component.m_stride) {
        return;
    }

    // grayscale -> only remove stride
    auto & pixels = component.m_pixels;

    unsigned char * pin = &pixels[component.m_stride];
    unsigned char * pout = &pixels[component.m_width];

    for (std::size_t y = component.m_height - 1; y != 0; --y) {
        std::memcpy(pout, pin, component.m_width);
        pin += component.m_stride;
        pout += component.m_width;
    }

    component.m_stride = component.m_width;
}

void Decoder::reset()
{
    Decoder decoder{};
    std::swap(*this, decoder);
}

void Decoder::decode(const BytesList & jpeg)
{
    m_position = jpeg.data();
    m_size = jpeg.size();

    if (jpeg.size() < 2 || m_position[0] != 0xFF || m_position[1] != 0xD8) {
        throw DecodingException("SOI (Start of Image) marker not found", DecodingException::Reason::NO_JPEG);
    }
    skip(2); // Skip SOI marker

    while (!m_decoding_finished) {
        if (m_size < 2 || m_position[0] != 0xFF) {
            throw DecodingException("Marker not found", DecodingException::Reason::SYNTAX_ERROR);
        }
        skip(2); // Skip marker

        switch (m_position[-1]) {
        case 0xC0:
            decode_start_of_frame();
            break;
        case 0xC4:
            decode_huffman_tables();
            break;
        case 0xDB:
            decode_quantize_tables();
            break;
        case 0xDD:
            decode_dri();
            break;
        case 0xDA:
            decode_start_of_scan();
            break;
        case 0xFE:
            skip_marker();
            break;
        default:
            if ((m_position[-1] & 0xF0) == 0xE0) {
                skip_marker();
            }
            else {
                throw DecodingException("Invalid marker", DecodingException::Reason::SYNTAX_ERROR);
            }
        }
    }
    convert();
}

std::size_t Decoder::get_width() const
{
    return m_width;
}

std::size_t Decoder::get_height() const
{
    return m_height;
}

bool Decoder::is_color_image() const
{
    return m_components.size() != 1;
}

const BytesList & Decoder::get_image() const
{
    return (m_components.size() == 1) ? m_components.front().m_pixels : m_rgb;
}

std::size_t Decoder::get_image_size() const
{
    return m_width * m_height * m_components.size();
}

const Output & Decoder::get_output() const
{
    return m_output;
}

Decoder::Sampling & Decoder::Sampling::set_greater(const Sampling & other)
{
    m_y = std::max(m_y, other.m_y);
    m_x = std::max(m_x, other.m_x);
    return *this;
}

Decoder::Component & Decoder::Component::set_id(const std::size_t id)
{
    m_id = id;
    return *this;
}

Decoder::Component & Decoder::Component::set_sampling(const std::size_t sampling)
{
    return set_sampling(sampling >> 4, sampling & 15);
}

Decoder::Component & Decoder::Component::set_sampling(const std::size_t x, const std::size_t y)
{
    m_sampling.m_y = x;
    m_sampling.m_x = y;
    return *this;
}

Decoder::Component & Decoder::Component::set_quantization_table(const std::size_t table_id)
{
    m_quantization_table_id = table_id;
    return *this;
}

void Decoder::Component::verify() const
{
    if (!is_point_of_two(m_sampling.m_y)) {
        throw DecodingException(
                fmt::format("Unsupported horizontal sampling ({} component): {}", m_id, m_sampling.m_y),
                DecodingException::Reason::UNSUPPORTED);
    }
    if (!is_point_of_two(m_sampling.m_x)) {
        throw DecodingException(
                fmt::format("Unsupported vertical sampling ({} component): {}", m_id, m_sampling.m_x),
                DecodingException::Reason::UNSUPPORTED);
    }
    if (m_quantization_table_id & 0xFC) {
        throw DecodingException(fmt::format("Invalid quantization table id: {}", m_quantization_table_id),
                                DecodingException::Reason::SYNTAX_ERROR);
    }
}

std::size_t Decoder::Component::get_x_sampling() const
{
    return m_sampling.m_y;
}

std::size_t Decoder::Component::get_y_sampling() const
{
    return m_sampling.m_x;
}
