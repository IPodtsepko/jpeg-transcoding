// NanoJPEG -- KeyJ's Tiny Baseline JPEG Decoder
// version 1.3.5 (2016-11-14)
// Copyright (c) 2009-2016 Martin J. Fiedler <martin.fiedler@gmx.net>
// published under the terms of the MIT license
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

///////////////////////////////////////////////////////////////////////////////
// DOCUMENTATION SECTION                                                     //
// read this if you want to know what this is all about                      //
///////////////////////////////////////////////////////////////////////////////

// INTRODUCTION
// ============
//
// This is a minimal decoder for baseline JPEG images. It accepts memory dumps
// of JPEG files as input and generates either 8-bit grayscale or packed 24-bit
// RGB images as output. It does not parse JFIF or Exif headers; all JPEG files
// are assumed to be either grayscale or YCbCr. CMYK or other color spaces are
// not supported. All YCbCr subsampling schemes with power-of-two ratios are
// supported, as are restart intervals. Progressive or lossless JPEG is not
// supported.
// Summed up, NanoJPEG should be able to decode all images from digital cameras
// and most common forms of other non-progressive JPEG images.
// The decoder is not optimized for speed, it's optimized for simplicity and
// small code. Image quality should be at a reasonable level. A bicubic chroma
// upsampling filter ensures that subsampled YCbCr images are rendered in
// decent quality. The decoder is not meant to deal with broken JPEG files in
// a graceful manner; if anything is wrong with the bitstream, decoding will
// simply fail.
// The code should work with every modern C compiler without problems and
// should not emit any warnings. It uses only (at least) 32-bit integer
// arithmetic and is supposed to be endianness independent and 64-bit clean.
// However, it is not thread-safe.

// COMPILE-TIME CONFIGURATION
// ==========================
//
// The following aspects of NanoJPEG can be controlled with preprocessor
// defines:
//
// _NJ_EXAMPLE_PROGRAM     = Compile a main() function with an example
//                           program.
// _NJ_INCLUDE_HEADER_ONLY = Don't compile anything, just act as a header
//                           file for NanoJPEG. Example:
//                               #define _NJ_INCLUDE_HEADER_ONLY
//                               #include "nanojpeg.c"
//                               int main(void) {
//                                   njInit();
//                                   // your code here
//                                   njDone();
//                               }
// NJ_USE_LIBC=1           = Use the malloc(), free(), memset() and memcpy()
//                           functions from the standard C library (default).
// NJ_USE_LIBC=0           = Don't use the standard C library. In this mode,
//                           external functions njAlloc(), njFreeMem(),
//                           njFillMem() and njCopyMem() need to be defined
//                           and implemented somewhere.
// NJ_USE_WIN32=0          = Normal mode (default).
// NJ_USE_WIN32=1          = If compiling with MSVC for Win32 and
//                           NJ_USE_LIBC=0, NanoJPEG will use its own
//                           implementations of the required C library
//                           functions (default if compiling with MSVC and
//                           NJ_USE_LIBC=0).
// NJ_CHROMA_FILTER=1      = Use the bicubic chroma upsampling filter
//                           (default).
// NJ_CHROMA_FILTER=0      = Use simple pixel repetition for chroma upsampling
//                           (bad quality, but faster and less code).

// API
// ===
//
// For API documentation, read the "header section" below.

// EXAMPLE
// =======
//
// A few pages below, you can find an example program that uses NanoJPEG to
// convert JPEG files into PGM or PPM. To compile it, use something like
//     gcc -O3 -D_NJ_EXAMPLE_PROGRAM -o nanojpeg nanojpeg.c
// You may also add -std=c99 -Wall -Wextra -pedantic -Werror, if you want :)
// The only thing you might need is -Wno-shift-negative-value, because this
// code relies on the target machine using two's complement arithmetic, but
// the C standard does not, even though *any* practically useful machine
// nowadays uses two's complement.

///////////////////////////////////////////////////////////////////////////////
// HEADER SECTION                                                            //
// copy and pase this into nanojpeg.h if you want                            //
///////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cstdio>
#include <decoder/dct_coefficients_filter.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utils/huffman.hpp>
#include <utils/output.hpp>
#include <vector>

#define _CRT_SECURE_NO_WARNINGS

#ifndef _NANOJPEG_H
#define _NANOJPEG_H

/**
 * @brief Results for JPEG decoding.
 */
enum class DecodingResult
{
    /// Decoding successful.
    OK = 0,

    /// Not a JPEG file.
    NO_JPEG,

    /// Unsupported format.
    UNSUPPORTED,

    /// Out of memory error.
    OUT_OF_MEMORY,

    /// Internal application error.
    INTERNAL_ERROR,

    /// Syntax error in JPEG file.
    SYNTAX_ERROR,

    /// Internal result, will never be reported.
    __FINISHED,
};

#endif //_NANOJPEG_H

///////////////////////////////////////////////////////////////////////////////
// CONFIGURATION SECTION                                                     //
// adjust the default settings for the NJ_ defines here                      //
///////////////////////////////////////////////////////////////////////////////

#ifndef NJ_USE_LIBC
#define NJ_USE_LIBC 1
#endif

#ifndef NJ_USE_WIN32
#ifdef _MSC_VER
#define NJ_USE_WIN32 (!NJ_USE_LIBC)
#else
#define NJ_USE_WIN32 0
#endif
#endif

#ifndef NJ_CHROMA_FILTER
#define NJ_CHROMA_FILTER 1
#endif

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION SECTION                                                    //
// you may stop reading here                                                 //
///////////////////////////////////////////////////////////////////////////////

#ifndef _NJ_INCLUDE_HEADER_ONLY

#ifdef _MSC_VER
#define NJ_INLINE static __inline
#define NJ_FORCE_INLINE static __forceinline
#else
#define NJ_INLINE static inline
#define NJ_FORCE_INLINE static inline
#endif

#if NJ_USE_LIBC
#include <stdlib.h>
#include <string.h>
#define njAllocMem malloc
#define njFreeMem free
#define njFillMem memset
#define njCopyMem memcpy
#elif NJ_USE_WIN32
#include <windows.h>
#define njAllocMem(size) ((void *)LocalAlloc(LMEM_FIXED, (SIZE_T)(size)))
#define njFreeMem(block) ((void)LocalFree((HLOCAL)block))
NJ_INLINE void njFillMem(void * block, unsigned char value, int count)
{
    __asm {
        mov edi, block
        mov al, value
        mov ecx, count
        rep stosb
    }
}
NJ_INLINE void njCopyMem(void * dest, const void * src, int count)
{
    __asm {
        mov edi, dest
        mov esi, src
        mov ecx, count
        rep movsb
    }
}
#else
extern void * njAllocMem(int size);
extern void njFreeMem(void * block);
extern void njFillMem(void * block, unsigned char byte, int size);
extern void njCopyMem(void * dest, const void * src, int size);
#endif

// clang-format off
static const int ZIGZAG_ORDER[64] = {
        0,  1,  8,  16, 9,  2,  3,  10,
        17, 24, 32, 25, 18, 11, 4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13, 6,  7,  14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
};
// clang-format on

struct Decoder
{

    Decoder()
    {
        initialize();
    }

    void set_dct_filter(const std::size_t dct_filter_power)
    {
        m_dct_filter_power = dct_filter_power;
        std::cout << "Use DCT filter: " << m_dct_filter_power << '\n';

    }

    struct HuffmanCodeEntry
    {
        unsigned char m_length = 0;
        unsigned char m_code = 0;
    };

    struct Component
    {
        int m_id;
        int m_ssx;
        int m_ssy;
        int m_width;
        int m_height;
        int m_stride;
        int m_quantization_table_id;
        int m_ac_huffman_table_id;
        int m_dc_huffman_table_id;
        int m_last_dc;
        std::vector<unsigned char> m_pixels;
    };

    DecodingResult m_error;
    const unsigned char * m_position;
    int m_size;
    int m_length;
    int m_width;
    int m_height;
    int mbwidth;
    int mbheight;
    int mbsizex;
    int mbsizey;
    int m_components_count;
    std::array<Component, 3> m_components;
    int qtused;
    int qtavail;
    unsigned char m_quantization_tables[4][64];
    HuffmanCodeEntry m_huffman_tables[4][65536];
    int m_buffer;
    int m_bits_in_buffer;
    int rstinterval;
    std::vector<unsigned char> rgb;

    std::size_t m_dct_filter_power = 0;
    unsigned short m_huffman_encoding_tables[4][256][2];
    bool m_is_scanning = false;
    Output m_jpeg_output;

    static unsigned char clip(const int x)
    {
        if (x < 0) {
            return 0;
        }
        if (x > 0xFF) {
            return 0xFF;
        }
        return static_cast<unsigned char>(x);
    }

#define W1 2841
#define W2 2676
#define W3 2408
#define W5 1609
#define W6 1108
#define W7 565

    static void inversed_discrete_cosine_transform_by_rows(int * blk)
    {
        int x0, x1, x2, x3, x4, x5, x6, x7, x8;
        if (!((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) | (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) {
            blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
            return;
        }
        x0 = (blk[0] << 11) + 128;
        x8 = W7 * (x4 + x5);
        x4 = x8 + (W1 - W7) * x4;
        x5 = x8 - (W1 + W7) * x5;
        x8 = W3 * (x6 + x7);
        x6 = x8 - (W3 - W5) * x6;
        x7 = x8 - (W3 + W5) * x7;
        x8 = x0 + x1;
        x0 -= x1;
        x1 = W6 * (x3 + x2);
        x2 = x1 - (W2 + W6) * x2;
        x3 = x1 + (W2 - W6) * x3;
        x1 = x4 + x6;
        x4 -= x6;
        x6 = x5 + x7;
        x5 -= x7;
        x7 = x8 + x3;
        x8 -= x3;
        x3 = x0 + x2;
        x0 -= x2;
        x2 = (181 * (x4 + x5) + 128) >> 8;
        x4 = (181 * (x4 - x5) + 128) >> 8;
        blk[0] = (x7 + x1) >> 8;
        blk[1] = (x3 + x2) >> 8;
        blk[2] = (x0 + x4) >> 8;
        blk[3] = (x8 + x6) >> 8;
        blk[4] = (x8 - x6) >> 8;
        blk[5] = (x0 - x4) >> 8;
        blk[6] = (x3 - x2) >> 8;
        blk[7] = (x7 - x1) >> 8;
    }

    static void inversed_discrete_cosine_transform_by_columns(const int * blk, unsigned char * out, int stride)
    {
        int x0, x1, x2, x3, x4, x5, x6, x7, x8;
        if (!((x1 = blk[8 * 4] << 8) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) | (x4 = blk[8 * 1]) | (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3]))) {
            x1 = clip(((blk[0] + 32) >> 6) + 128);
            for (x0 = 8; x0; --x0) {
                *out = (unsigned char)x1;
                out += stride;
            }
            return;
        }
        x0 = (blk[0] << 8) + 8192;
        x8 = W7 * (x4 + x5) + 4;
        x4 = (x8 + (W1 - W7) * x4) >> 3;
        x5 = (x8 - (W1 + W7) * x5) >> 3;
        x8 = W3 * (x6 + x7) + 4;
        x6 = (x8 - (W3 - W5) * x6) >> 3;
        x7 = (x8 - (W3 + W5) * x7) >> 3;
        x8 = x0 + x1;
        x0 -= x1;
        x1 = W6 * (x3 + x2) + 4;
        x2 = (x1 - (W2 + W6) * x2) >> 3;
        x3 = (x1 + (W2 - W6) * x3) >> 3;
        x1 = x4 + x6;
        x4 -= x6;
        x6 = x5 + x7;
        x5 -= x7;
        x7 = x8 + x3;
        x8 -= x3;
        x3 = x0 + x2;
        x0 -= x2;
        x2 = (181 * (x4 + x5) + 128) >> 8;
        x4 = (181 * (x4 - x5) + 128) >> 8;
        *out = clip(((x7 + x1) >> 14) + 128);
        out += stride;
        *out = clip(((x3 + x2) >> 14) + 128);
        out += stride;
        *out = clip(((x0 + x4) >> 14) + 128);
        out += stride;
        *out = clip(((x8 + x6) >> 14) + 128);
        out += stride;
        *out = clip(((x8 - x6) >> 14) + 128);
        out += stride;
        *out = clip(((x0 - x4) >> 14) + 128);
        out += stride;
        *out = clip(((x3 - x2) >> 14) + 128);
        out += stride;
        *out = clip(((x7 - x1) >> 14) + 128);
    }

#define handle_error(e) \
    do {                \
        m_error = e;    \
        return;         \
    } while (0)
#define check_error()                      \
    do {                                   \
        if (m_error != DecodingResult::OK) \
            return;                        \
    } while (0)

    unsigned char njUse(const std::size_t count = 1)
    {
        const auto * begin = m_position;
        m_position += count;
        m_size -= count;
        if (use_jpeg_output() && !m_is_scanning) {
            for (auto * byte = begin; byte != m_position != 0; ++byte) {
                m_jpeg_output << *byte;
            }
        }
        return *begin;
    }

    int read_bits(int bits)
    {
        unsigned char newbyte;
        if (!bits)
            return 0;
        while (m_bits_in_buffer < bits) {
            if (m_size <= 0) {
                m_buffer = (m_buffer << 8) | 0xFF;
                m_bits_in_buffer += 8;
                continue;
            }
            newbyte = njUse();
            m_bits_in_buffer += 8;
            m_buffer = (m_buffer << 8) | newbyte;
            if (newbyte == 0xFF) {
                if (m_size) {
                    const auto marker = njUse();
                    switch (marker) {
                    case 0x00:
                    case 0xFF:
                        break;
                    case 0xD9: m_size = 0; break;
                    default:
                        if ((marker & 0xF8) != 0xD0)
                            m_error = DecodingResult::SYNTAX_ERROR;
                        else {
                            m_buffer = (m_buffer << 8) | marker;
                            m_bits_in_buffer += 8;
                        }
                    }
                }
                else
                    m_error = DecodingResult::SYNTAX_ERROR;
            }
        }
        return (m_buffer >> (m_bits_in_buffer - bits)) & ((1 << bits) - 1);
    }

    void skip_bits(const int bits)
    {
        if (m_bits_in_buffer < bits) {
            read_bits(bits);
        }
        m_bits_in_buffer -= bits;
    }

    int get_bits(int bits)
    {
        const int res = read_bits(bits);
        skip_bits(bits);
        return res;
    }

    void byte_align(void)
    {
        m_bits_in_buffer &= 0xF8;
    }

    void skip(int count)
    {
        njUse(count);
        m_length -= count;
        if (m_size < 0)
            m_error = DecodingResult::SYNTAX_ERROR;
    }

    unsigned short njDecode16(const unsigned char * pos)
    {
        return (pos[0] << 8) | pos[1];
    }

    void decode_length(void)
    {
        if (m_size < 2)
            handle_error(DecodingResult::SYNTAX_ERROR);
        m_length = njDecode16(m_position);
        if (m_length > m_size)
            handle_error(DecodingResult::SYNTAX_ERROR);
        skip(2);
    }

    void skip_marker(void)
    {
        decode_length();
        skip(m_length);
    }

    void decode_start_of_scan(void)
    {
        int ssxmax = 0, ssymax = 0;
        decode_length();
        check_error();
        if (m_length < 9)
            handle_error(DecodingResult::SYNTAX_ERROR);
        if (m_position[0] != 8)
            handle_error(DecodingResult::UNSUPPORTED);
        m_height = njDecode16(m_position + 1);
        m_width = njDecode16(m_position + 3);
        if (!m_width || !m_height)
            handle_error(DecodingResult::SYNTAX_ERROR);
        m_components_count = m_position[5];
        skip(6);
        switch (m_components_count) {
        case 1:
        case 3:
            break;
        default:
            handle_error(DecodingResult::UNSUPPORTED);
        }
        if (m_length < (m_components_count * 3))
            handle_error(DecodingResult::SYNTAX_ERROR);
        for (auto & component : m_components) {
            component.m_id = m_position[0];
            if (!(component.m_ssx = m_position[1] >> 4))
                handle_error(DecodingResult::SYNTAX_ERROR);
            if (component.m_ssx & (component.m_ssx - 1))
                handle_error(DecodingResult::UNSUPPORTED); // non-power of two
            if (!(component.m_ssy = m_position[1] & 15))
                handle_error(DecodingResult::SYNTAX_ERROR);
            if (component.m_ssy & (component.m_ssy - 1))
                handle_error(DecodingResult::UNSUPPORTED); // non-power of two
            if ((component.m_quantization_table_id = m_position[2]) & 0xFC)
                handle_error(DecodingResult::SYNTAX_ERROR);
            skip(3);
            qtused |= 1 << component.m_quantization_table_id;
            if (component.m_ssx > ssxmax)
                ssxmax = component.m_ssx;
            if (component.m_ssy > ssymax)
                ssymax = component.m_ssy;
        }
        if (m_components_count == 1) {
            auto & c = m_components.front();
            c.m_ssx = c.m_ssy = ssxmax = ssymax = 1;
        }
        mbsizex = ssxmax << 3;
        mbsizey = ssymax << 3;
        mbwidth = (m_width + mbsizex - 1) / mbsizex;
        mbheight = (m_height + mbsizey - 1) / mbsizey;
        for (auto & c : m_components) {
            c.m_width = (m_width * c.m_ssx + ssxmax - 1) / ssxmax;
            c.m_height = (m_height * c.m_ssy + ssymax - 1) / ssymax;
            c.m_stride = mbwidth * c.m_ssx << 3;
            if (((c.m_width < 3) && (c.m_ssx != ssxmax)) || ((c.m_height < 3) && (c.m_ssy != ssymax)))
                handle_error(DecodingResult::UNSUPPORTED);
            c.m_pixels = std::vector<unsigned char>(c.m_stride * mbheight * c.m_ssy << 3);
        }
        if (m_components_count == 3) {
            rgb = std::vector<unsigned char>(m_width * m_height * m_components_count);
        }
        skip(m_length);
    }

    static void restore_huffman_codes(std::array<unsigned char, 17> spectrum,
                                      std::vector<std::pair<unsigned short, unsigned short>> & to)
    {
        const int initial_length = 0;
        const int initial_code = 0;
        restore_huffman_codes(spectrum, initial_length, initial_code, to);
    }
    static void restore_huffman_codes(
            std::array<unsigned char, 17> & spectrum,
            int length,
            int code,
            std::vector<std::pair<unsigned short, unsigned short>> & to)
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

    void decode_huffman_tables(void)
    {
        decode_length();
        check_error();
        while (m_length >= 17) {
            int i = m_position[0];
            if (i & 0xEC) {
                handle_error(DecodingResult::SYNTAX_ERROR);
            }
            if (i & 0x02) {
                handle_error(DecodingResult::UNSUPPORTED);
            }
            i = (i | (i >> 3)) & 3; // combined DC/AC + tableid value
            static std::array<unsigned char, 17> counts;
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

            auto * restored_table = m_huffman_encoding_tables[i];
            for (std::size_t k = 0; k < 256; ++k) {
                for (std::size_t l = 0; l < 2; ++l) {
                    restored_table[k][l] = 0;
                }
            }

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
                    handle_error(DecodingResult::SYNTAX_ERROR);
                }
                remain -= current_count << (16 - code_length);
                if (remain < 0) {
                    handle_error(DecodingResult::SYNTAX_ERROR);
                }
                for (int i = 0; i < current_count; ++i) {
                    unsigned char code = m_position[i];

                    auto & row = restored_table[code];
                    auto & entry = restored_codes[code_used++];
                    row[0] = entry.first;
                    row[1] = entry.second;

                    for (int j = 0; j < spread; ++j) {
                        huffman_table[entry_id] = HuffmanCodeEntry{
                                static_cast<unsigned char>(code_length), // m_bits
                                code                                     // m_code
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
            handle_error(DecodingResult::SYNTAX_ERROR);
        }
    }

    void decode_quantize_table(void)
    {
        int i;
        unsigned char * t;
        decode_length();
        check_error();
        while (m_length >= 65) {
            i = m_position[0];
            if (i & 0xFC)
                handle_error(DecodingResult::SYNTAX_ERROR);
            qtavail |= 1 << i;
            t = &m_quantization_tables[i][0];
            for (i = 0; i < 64; ++i)
                t[i] = m_position[i + 1];
            skip(65);
        }
        if (m_length)
            handle_error(DecodingResult::SYNTAX_ERROR);
    }

    void njDecodeDRI(void)
    {
        decode_length();
        check_error();
        if (m_length < 2)
            handle_error(DecodingResult::SYNTAX_ERROR);
        rstinterval = njDecode16(m_position);
        skip(m_length);
    }

    int decode_huffman(HuffmanCodeEntry huffman_table[], unsigned char * code)
    {
        int value = read_bits(16);
        int bits = huffman_table[value].m_length;
        if (bits == 0) {
            m_error = DecodingResult::SYNTAX_ERROR;
            return 0;
        }
        skip_bits(bits);
        value = huffman_table[value].m_code;
        if (code)
            *code = (unsigned char)value;
        bits = value & 15;
        if (!bits)
            return 0;
        value = get_bits(bits);
        if (value < (1 << (bits - 1)))
            value += ((-1) << bits) + 1;
        return value;
    }

    bool use_jpeg_output() const
    {
        static bool print = true;
        if (print) {
            print = false;
            std::cout << "Use JPEG output: " << (m_dct_filter_power > 0) << " (m_dct_filter_power=" << m_dct_filter_power << ")\n";
        }
        return m_dct_filter_power > 0;
    }

    void decode_block(Component & component, unsigned char * out)
    {
        std::array<int, 64> block;
        block.fill(0);

        // Decode DC
        auto * dc_huffman_table = m_huffman_tables[component.m_dc_huffman_table_id];
        const int dc_difference = decode_huffman(dc_huffman_table, nullptr);
        block[0] = component.m_last_dc + dc_difference;

        static std::array<int, 64> encoder_part_input;
        if (use_jpeg_output())
        {
            encoder_part_input.fill(0);
            encoder_part_input[0] = block[0];
        }

        // Decode AC
        auto * ac_huffman_table = m_huffman_tables[component.m_ac_huffman_table_id];
        for (int i = 0; i < 63;) {
            unsigned char code = 0;
            const auto value = decode_huffman(ac_huffman_table, &code);
            if (!code) {
                break; // EOB
            }
            if (!(code & 0x0F) && (code != 0xF0)) {
                handle_error(DecodingResult::SYNTAX_ERROR);
            }
            i += (code >> 4) + 1;
            if (i > 63) {
                handle_error(DecodingResult::SYNTAX_ERROR);
            }
            block[ZIGZAG_ORDER[i]] = value;
            if (use_jpeg_output()) {
                encoder_part_input[i] = value;
            }
        }

        if (use_jpeg_output()) {
            DCTCoefficientsFilter(m_dct_filter_power).apply(encoder_part_input.data());
            encode_by_huffman(
                    encoder_part_input.data(),
                    component.m_last_dc,
                    m_huffman_encoding_tables[component.m_dc_huffman_table_id],
                    m_huffman_encoding_tables[component.m_ac_huffman_table_id],
                    m_jpeg_output);
        }

        component.m_last_dc += dc_difference;

        // De-Quantization
        for (std::size_t i = 0; i < 64; ++i) {
            const auto j = ZIGZAG_ORDER[i];
            block[j] *= m_quantization_tables[component.m_quantization_table_id][i];
        }

        // Inverse DCT
        for (int i = 0; i < 64; i += 8) {
            inversed_discrete_cosine_transform_by_rows(&block[i]);
        }
        for (int i = 0; i < 8; ++i) {
            inversed_discrete_cosine_transform_by_columns(&block[i], &out[i], component.m_stride);
        }
    }

    void decode_scan(void)
    {
        int i, mbx, mby, sbx, sby;
        int rstcount = rstinterval, nextrst = 0;
        decode_length();
        check_error();
        if (m_length < (4 + 2 * m_components_count))
            handle_error(DecodingResult::SYNTAX_ERROR);
        if (m_position[0] != m_components_count)
            handle_error(DecodingResult::UNSUPPORTED);
        skip(1);
        for (auto & c : m_components) {
            if (m_position[0] != c.m_id)
                handle_error(DecodingResult::SYNTAX_ERROR);
            if (m_position[1] & 0xEE)
                handle_error(DecodingResult::SYNTAX_ERROR);
            c.m_dc_huffman_table_id = m_position[1] >> 4;
            c.m_ac_huffman_table_id = (m_position[1] & 1) | 2;
            skip(2);
        }
        if (m_position[0] || (m_position[1] != 63) || m_position[2]) {
            handle_error(DecodingResult::UNSUPPORTED);
        }
        skip(m_length);
        m_is_scanning = true;
        for (mbx = mby = 0;;) {
            for (auto & component : m_components) {
                for (sby = 0; sby < component.m_ssy; ++sby) {
                    for (sbx = 0; sbx < component.m_ssx; ++sbx) {
                        decode_block(component, &component.m_pixels[((mby * component.m_ssy + sby) * component.m_stride + mbx * component.m_ssx + sbx) << 3]);
                        check_error();
                    }
                }
            }
            if (++mbx >= mbwidth) {
                mbx = 0;
                if (++mby >= mbheight) {
                    break;
                }
            }
            if (rstinterval && !(--rstcount)) {
                byte_align();
                i = get_bits(16);
                if (((i & 0xFFF8) != 0xFFD0) || ((i & 7) != nextrst)) {
                    handle_error(DecodingResult::SYNTAX_ERROR);
                }
                nextrst = (nextrst + 1) & 7;
                rstcount = rstinterval;
                for (auto & component : m_components) {
                    component.m_last_dc = 0;
                }
            }
        }

        static const unsigned short fill_bits[] = {0x7F, 0x07};
        if (use_jpeg_output()) {
            m_jpeg_output << fill_bits << 0xFF << 0xD9;
        }

        m_error = DecodingResult::__FINISHED;
    }

#if NJ_CHROMA_FILTER

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

    void horizontal_upsample(Component & component)
    {
        const int xmax = component.m_width - 3;
        unsigned char *lin, *lout;
        std::vector<unsigned char> out((component.m_width * component.m_height) << 1);
        lin = component.m_pixels.data();
        lout = out.data();
        for (int y = component.m_height; y; --y) {
            lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
            lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
            lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
            for (int x = 0; x < xmax; ++x) {
                lout[(x << 1) + 3] = CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
                lout[(x << 1) + 4] = CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
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

    void vertical_upsample(Component & c)
    {
        const int w = c.m_width, s1 = c.m_stride, s2 = s1 + s1;
        unsigned char *cin, *cout;
        int x, y;
        std::vector<unsigned char> out((c.m_width * c.m_height) << 1);
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

#else

    NJ_INLINE void njUpsample(nj_component_t * c)
    {
        int x, y, xshift = 0, yshift = 0;
        unsigned char *out, *lin, *lout;
        while (c->width < width) {
            c->width <<= 1;
            ++xshift;
        }
        while (c->height < height) {
            c->height <<= 1;
            ++yshift;
        }
        out = (unsigned char *)njAllocMem(c->width * c->height);
        if (!out)
            njThrow(NJ_OUT_OF_MEM);
        lin = c->pixels;
        lout = out;
        for (y = 0; y < c->height; ++y) {
            lin = &c->pixels[(y >> yshift) * c->m_stride];
            for (x = 0; x < c->width; ++x)
                lout[x] = lin[x >> xshift];
            lout += c->width;
        }
        c->m_stride = c->width;
        njFreeMem((void *)c->pixels);
        c->pixels = out;
    }

#endif

    void njConvert(void)
    {
        for (auto & component : m_components) {
#if NJ_CHROMA_FILTER
            while ((component.m_width < m_width) || (component.m_height < m_height)) {
                if (component.m_width < m_width)
                    horizontal_upsample(component);
                check_error();
                if (component.m_height < m_height)
                    vertical_upsample(component);
                check_error();
            }
#else
            if ((c->width < nj.width) || (c->height < nj.height))
                njUpsample(c);
#endif
            if (component.m_width < m_width || component.m_height < m_height)
                handle_error(DecodingResult::INTERNAL_ERROR);
        }
        if (m_components_count == 3) {
            // convert to RGB
            auto * prgb = rgb.data();
            const auto * py = m_components[0].m_pixels.data();
            const auto * pcb = m_components[1].m_pixels.data();
            const auto * pcr = m_components[2].m_pixels.data();
            for (int y = m_height; y; --y) {
                for (int x = 0; x < m_width; ++x) {
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
        }
        else if (m_components[0].m_width != m_components[0].m_stride) {
            // grayscale -> only remove stride
            unsigned char * pin = &m_components[0].m_pixels[m_components[0].m_stride];
            unsigned char * pout = &m_components[0].m_pixels[m_components[0].m_width];
            int y;
            for (y = m_components[0].m_height - 1; y; --y) {
                njCopyMem(pout, pin, m_components[0].m_width);
                pin += m_components[0].m_stride;
                pout += m_components[0].m_width;
            }
            m_components[0].m_stride = m_components[0].m_width;
        }
    }

    void initialize(void)
    {
        njFillMem(this, 0, sizeof(Decoder));
    }

    void njDone(void)
    {
        initialize();
    }

    DecodingResult decode(const std::vector<unsigned char>& jpeg)
    {
        m_position = jpeg.data();
        this->m_size = jpeg.size() & 0x7FFFFFFF;
        if (jpeg.size() < 2)
            return DecodingResult::NO_JPEG;
        if ((m_position[0] ^ 0xFF) | (m_position[1] ^ 0xD8))
            return DecodingResult::NO_JPEG;
        skip(2);
        while (m_error == DecodingResult::OK) {
            if ((jpeg.size() < 2) || (m_position[0] != 0xFF))
                return DecodingResult::SYNTAX_ERROR;
            skip(2);
            switch (m_position[-1]) {
            case 0xC0: decode_start_of_scan(); break;  // Начало кадра: высота и ширина рисунков, количество каналов,
                                                       // идентификаторы и данные о прореживании каналов, идентификаторы таблиц квантований
            case 0xC4: decode_huffman_tables(); break; // Определение таблицы Хаффмана
            case 0xDB: decode_quantize_table(); break; // Определение таблицы квантования
            case 0xDD: njDecodeDRI(); break;           // Определение интервала перезапуска
            case 0xDA: decode_scan(); break;           // Начало сканирования: Количество каналов, идентификаторы таблиц Хаффмана
                                                       // для DC и AC коэффициентов
            case 0xFE: skip_marker(); break;           // Комментарий
            default:
                if ((m_position[-1] & 0xF0) == 0xE0)
                    skip_marker();
                else
                    return DecodingResult::UNSUPPORTED;
            }
        }
        if (m_error != DecodingResult::__FINISHED)
            return m_error;
        m_error = DecodingResult::OK;
        njConvert();
        return m_error;
    }

    int get_width(void) { return m_width; }
    int get_height(void) { return m_height; }
    int is_color_image(void) { return (m_components_count != 1); }
    const std::vector<unsigned char> & get_image(void) { return (m_components_count == 1) ? m_components.front().m_pixels : rgb; }
    int get_image_size(void) { return m_width * m_height * m_components_count; }
    const Output & get_jpeg_output(void) { return m_jpeg_output; }
};

#endif // _NJ_INCLUDE_HEADER_ONLY

class CommandLineArguments
{
public:
    CommandLineArguments(int argc, const char * argv[])
        :m_is_valid(validate(argc, argv))
        , m_input_file_name(argv[1])
        , m_output_file_name(argv[2])
        , m_dct_filter_power(argc > 3 ? to_size_t(argv[3]) : 0)
        , m_jpeg_output_file_name(argc > 4 ? argv[4] : "")
    {
        validate();
    }

    bool is_valid() const
    {
        return m_is_valid;
    }

    const std::string & get_input_file_name() const
    {
        return m_input_file_name;
    }
    const std::string & get_output_file_name() const
    {
        return m_output_file_name;
    }

    bool use_dct_filter() const
    {
        return m_dct_filter_power > 0;
    }

    std::size_t get_dct_filter_power() const
    {
        return m_dct_filter_power;
    }

    const std::string& get_jpeg_output_file_name() const
    {
        return m_jpeg_output_file_name;
    }

private:
    static std::size_t to_size_t(const char * arg)
    {
        return static_cast<std::size_t>(std::stoi(arg));
    }

    void validate()
    {
        if (m_dct_filter_power > 63) {
            m_is_valid = false;
            throw std::invalid_argument("DCT filter power cannot to be more then 63");
        }
    }

    static bool validate(const int argc, const char * argv[])
    {
        if (argc != 3 && argc != 5) {
            std::string app_name{argv[0]};
            throw std::invalid_argument("Usage: " + app_name + " <input_file> <output_file> [<dct_filter_power> <m_jpeg_output_file_name>]");
        }
        return true;
    }

private:
    bool m_is_valid = false;
    const std::string m_input_file_name;
    const std::string m_output_file_name;
    const std::size_t m_dct_filter_power;
    const std::string m_jpeg_output_file_name;
};

std::size_t size_of(std::ifstream& file) {
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    return size;
}

int main(const int argc, const char * argv[])
{
    CommandLineArguments args{argc, argv};

    std::ifstream file(args.get_input_file_name(), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error opening the input file.\n";
        return 1;
    }

    const auto size = size_of(file);

    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        std::cout << "Error reading the input file.\n";
        return 1;
    }
    file.close();

    Decoder decoder;
    if (args.use_dct_filter()) {
        decoder.set_dct_filter(args.get_dct_filter_power());
    }
    std::cout << "decoder.m_dct_filter_power=" << decoder.m_dct_filter_power << '\n';
    if (decoder.decode(buffer) != DecodingResult::OK) {
        std::cout << "Error decoding the input file.\n";
        return 1;
    }
    std::cout << "Successful!\n";

    std::ofstream output(args.get_output_file_name(), std::ios::binary);
    if (!output.is_open()) {
        std::cout << "Error opening the output file.\n";
        return 1;
    }

    output << "P" << (decoder.is_color_image() ? 6 : 5) << "\n"
           << decoder.get_width() << " " << decoder.get_height() << "\n255\n";
    output.write(reinterpret_cast<const char *>(decoder.get_image().data()), decoder.get_image_size());
    output.close();

    if (args.use_dct_filter()) {
        decoder.get_jpeg_output().to_file(args.get_jpeg_output_file_name());
    }

    decoder.njDone();

    return 0;
}
