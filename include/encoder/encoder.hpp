#pragma once

#include <encoder/discrete_cosine_transform.hpp>
#include <fstream>
#include <utils/huffman.hpp>
#include <utils/image.hpp>
#include <utils/output.hpp>
#include <iostream>

// clang-format off
static const unsigned char ZIGZAG_ORDER[] = {
    0,  1,  5,  6,  14, 15, 27, 28,
    2,  4,  7,  13, 16, 26, 29, 42,
    3,  8,  12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

static const Bytes<16> std_dc_luminance_nrcodes{ 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static const Bytes<12> std_dc_luminance_values{ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const Bytes<16> std_ac_luminance_nrcodes{ 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static const Bytes<162> std_ac_luminance_values{
        0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
        0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
        0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
        0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
        0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
        0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
        0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};
static const Bytes<16> std_dc_chrominance_nrcodes{ 0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static const Bytes<12> std_dc_chrominance_values{ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const Bytes<16> std_ac_chrominance_nrcodes{ 0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
static const Bytes<162> std_ac_chrominance_values{
        0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
        0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
        0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
        0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
        0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
        0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};

// Huffman tables
static const unsigned short YDC_HT[256][2] = { {0,2},{2,3},{3,3},{4,3},{5,3},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9} };
static const unsigned short UVDC_HT[256][2] = { {0,2},{1,2},{2,2},{6,3},{14,4},{30,5},{62,6},{126,7},{254,8},{510,9},{1022,10},{2046,11} };
static const unsigned short YAC_HT[256][2] = {
        {10,4},{0,2},{1,2},{4,3},{11,4},{26,5},{120,7},{248,8},{1014,10},{65410,16},{65411,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {12,4},{27,5},{121,7},{502,9},{2038,11},{65412,16},{65413,16},{65414,16},{65415,16},{65416,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {28,5},{249,8},{1015,10},{4084,12},{65417,16},{65418,16},{65419,16},{65420,16},{65421,16},{65422,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {58,6},{503,9},{4085,12},{65423,16},{65424,16},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {59,6},{1016,10},{65430,16},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {122,7},{2039,11},{65438,16},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {123,7},{4086,12},{65446,16},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {250,8},{4087,12},{65454,16},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {504,9},{32704,15},{65462,16},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {505,9},{65470,16},{65471,16},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {506,9},{65479,16},{65480,16},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {1017,10},{65488,16},{65489,16},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {1018,10},{65497,16},{65498,16},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {2040,11},{65506,16},{65507,16},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {65515,16},{65516,16},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{0,0},{0,0},{0,0},{0,0},{0,0},
        {2041,11},{65525,16},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
static const unsigned short UVAC_HT[256][2] = {
        {0,2},{1,2},{4,3},{10,4},{24,5},{25,5},{56,6},{120,7},{500,9},{1014,10},{4084,12},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {11,4},{57,6},{246,8},{501,9},{2038,11},{4085,12},{65416,16},{65417,16},{65418,16},{65419,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {26,5},{247,8},{1015,10},{4086,12},{32706,15},{65420,16},{65421,16},{65422,16},{65423,16},{65424,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {27,5},{248,8},{1016,10},{4087,12},{65425,16},{65426,16},{65427,16},{65428,16},{65429,16},{65430,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {58,6},{502,9},{65431,16},{65432,16},{65433,16},{65434,16},{65435,16},{65436,16},{65437,16},{65438,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {59,6},{1017,10},{65439,16},{65440,16},{65441,16},{65442,16},{65443,16},{65444,16},{65445,16},{65446,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {121,7},{2039,11},{65447,16},{65448,16},{65449,16},{65450,16},{65451,16},{65452,16},{65453,16},{65454,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {122,7},{2040,11},{65455,16},{65456,16},{65457,16},{65458,16},{65459,16},{65460,16},{65461,16},{65462,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {249,8},{65463,16},{65464,16},{65465,16},{65466,16},{65467,16},{65468,16},{65469,16},{65470,16},{65471,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {503,9},{65472,16},{65473,16},{65474,16},{65475,16},{65476,16},{65477,16},{65478,16},{65479,16},{65480,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {504,9},{65481,16},{65482,16},{65483,16},{65484,16},{65485,16},{65486,16},{65487,16},{65488,16},{65489,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {505,9},{65490,16},{65491,16},{65492,16},{65493,16},{65494,16},{65495,16},{65496,16},{65497,16},{65498,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {506,9},{65499,16},{65500,16},{65501,16},{65502,16},{65503,16},{65504,16},{65505,16},{65506,16},{65507,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {2041,11},{65508,16},{65509,16},{65510,16},{65511,16},{65512,16},{65513,16},{65514,16},{65515,16},{65516,16},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
        {16352,14},{65517,16},{65518,16},{65519,16},{65520,16},{65521,16},{65522,16},{65523,16},{65524,16},{65525,16},{0,0},{0,0},{0,0},{0,0},{0,0},
        {1018,10},{32707,15},{65526,16},{65527,16},{65528,16},{65529,16},{65530,16},{65531,16},{65532,16},{65533,16},{65534,16},{0,0},{0,0},{0,0},{0,0},{0,0}
};
static const int YQT[] = { 16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99 };
static const int UVQT[] = { 17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99 };

// clang-format on

class Encoder
{
public:
    static int round(const float v)
    {
        return static_cast<int>(v < 0 ? std::ceil(v - 0.5f) : std::floor(v + 0.5f));
    }

    static void perform_quantization(const Bytes<64> & quantization_table, const std::size_t stride, const float dct_coefficients[64], int output[64])
    {
        for (std::size_t y = 0, j = 0; y < 8; ++y) {
            for (std::size_t x = 0; x < 8; ++x, ++j) {
                const auto i = y * stride + x;
                output[ZIGZAG_ORDER[j]] = round(dct_coefficients[i] / quantization_table[ZIGZAG_ORDER[j]]);
            }
        }
    }

    int process_block(float * CDU,
                      const std::size_t du_stride,
                      const Bytes<64> & quantization_table,
                      const int last_dc,
                      const unsigned short dc_huffman_table[256][2],
                      const unsigned short ac_huffman_table[256][2])
    {
        discrete_cosine_transform(CDU, du_stride);

        int DU[64];
        perform_quantization(quantization_table, du_stride, CDU, DU);

        return encode_by_huffman(DU, last_dc, dc_huffman_table, ac_huffman_table, m_output);
    }

    template <std::size_t N, std::size_t M = 1>
    struct YCbCrBlock
    {
        YCbCrBlock(const Image & image, const std::size_t x, const std::size_t y)
        {
            if constexpr (M == 1) {
                calculate_components(Y, Cb, Cr, image, x, y);
                return;
            }
            else {
                float Cb[Y_SIZE];
                float Cr[Y_SIZE];
                calculate_components(Y, Cb, Cr, image, x, y);

                // Цветовая субдискретизация
                for (std::size_t row = 0, position = 0; row < CHROMA_SIDE_SIZE; ++row) {
                    for (std::size_t column = 0; column < CHROMA_SIDE_SIZE; ++column, ++position) {
                        const auto j = (row * N + column) * M;
                        this->Cb[position] = subsample(Cb, j);
                        this->Cr[position] = subsample(Cr, j);
                    }
                }
            }
        }

        static float subsample(const float * component, const std::size_t position)
        {
            return average(component[position],
                           component[position + 1],
                           component[position + N],
                           component[position + N + 1]);
        }

        static float average(const float x1, const float x2, const float x3, const float x4)
        {
            return (x1 + x2 + x3 + x4) * 0.25f;
        }

        static void calculate_components(float * Y, float * Cb, float * Cr, const Image & image, const std::size_t x, const std::size_t y)
        {
            for (std::size_t row = y, pos = 0; row < y + N; ++row) {
                for (std::size_t column = x; column < x + N; ++column, ++pos) {
                    std::tie(Y[pos], Cb[pos], Cr[pos]) = to_y_cb_cr(image.get(row, column));
                }
            }
        }

        static std::tuple<float, float, float> to_y_cb_cr(const std::tuple<float, float, float> & rgb)
        {
            const auto [r, g, b] = rgb;
            return {
                    +0.29900f * r + 0.58700f * g + 0.11400f * b - 128,
                    -0.16874f * r - 0.33126f * g + 0.50000f * b,
                    +0.50000f * r - 0.41869f * g - 0.08131f * b};
        }

        inline static constexpr auto Y_SIZE = N * N;
        inline static constexpr auto CHROMA_SIDE_SIZE = N / M;
        inline static constexpr auto CHROMA_SIZE = CHROMA_SIDE_SIZE * CHROMA_SIDE_SIZE;

        float Y[Y_SIZE];
        float Cb[CHROMA_SIZE];
        float Cr[CHROMA_SIZE];
    };

    static unsigned char quantization_table_value(const int base, const int quality)
    {
        return std::min(std::max(1, (base * quality + 50) / 100), 255);
    }

    bool encode(const std::string & file_name, const Image & image, int quality)
    {
        const bool subsample = quality <= 90 ? true : false;
        quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

        Bytes<64> YTable, UVTable;
        for (int i = 0; i < 64; ++i) {
            YTable[ZIGZAG_ORDER[i]] = quantization_table_value(YQT[i], quality);
            UVTable[ZIGZAG_ORDER[i]] = quantization_table_value(UVQT[i], quality);
        }

        // Write Headers

        // clang-format off
        static const Bytes<25> head0{
                0xFF, 0xD8, // SOI (Start of Image) marker

                0xFF, 0xE0, // APP0	(Application Segment 0) marker
                0x00, 0x10, // Lenght (16)
                'J', 'F', 'I', 'F', // JFIF JPEG Image
                0, 1, 1, 0, 0, 1, 0, 1, 0, 0, // ?

                0xFF, 0xDB, // DQT (Define Quantization Table) marker
                0x00, 0x84, // Lenght (132)
                0x00  // 0_ Values length (1 byte), _0 table id
        };
        // clang-format on

        m_output << head0 << YTable << 0x01 // 0_ Values length (1 byte), _1 table id
                 << UVTable;

        // clang-format off
        static const Bytes<24> head1{
                0xFF, 0xC0, // SOF0 (Start of Frame 0) marker
                0x00, 0x11, // Lenght (17)
                0x08, // Precision
                static_cast<unsigned char>(image.get_height() >> 8), static_cast<unsigned char>(image.get_height() & 0xFF), // Image height
                static_cast<unsigned char>(image.get_width() >> 8), static_cast<unsigned char>(image.get_width() & 0xFF), // Image width
                0x03, // Channels count

                // Channel description
                0x01, // Channel id
                static_cast<unsigned char>(subsample ? 0x22 : 0x11), // Subsampling
                0x00, // Quantization table id

                // Channel description
                0x02, // Channel id
                0x11, // Subsampling
                0x01, // Quantization table id

                // Channel description
                0x03, // Channel id
                0x11, // Subsampling
                0x01, // Quantization table id

                0xFF, 0xC4, // DHT marker (Huffman tables)
                0x01, 0xA2, // Lenght (418)
                0x00 // Class: 0_ (DC), table id: _0.
        };
        // clang-format on
        m_output << head1
                 << std_dc_luminance_nrcodes
                 << std_dc_luminance_values
                 << 0x10 // Class: 1_ (AC), table id: _0.
                 << std_ac_luminance_nrcodes
                 << std_ac_luminance_values
                 << 0x01 // Class: 0_ (DC), table id: _1.
                 << std_dc_chrominance_nrcodes
                 << std_dc_chrominance_values
                 << 0x11 // Class: 1_ (AC), table id: _1.
                 << std_ac_chrominance_nrcodes
                 << std_ac_chrominance_values;

        // clang-format off
        static const Bytes<14> head2{
                0xFF, 0xDA, // SOS (Start of Scan) marker
                0x00, 0x0C, // Length (12)
                0x03, // Channels count (3)

                0x01, // Channel id
                0x00, // Huffman table for DC coefficients: 0_,
                      // Huffman table for AC coefficients: _0.

                0x02, // Channel id
                0x11, // Huffman table for DC coefficients: 0_,
                      // Huffman table for AC coefficients: _0.

                0x03, // Channel id
                0x11, // Huffman table for DC coefficients: 0_,
                      // Huffman table for AC coefficients: _0.

                0x00, // Start of spectral or predictor selection
                0x3F, // End of spectral selection
                0x00  // Successive approximation bit position
        };
        // clang-format on
        m_output << head2;

        int DCY = 0, DCU = 0, DCV = 0;
        m_output.reset();
        if (subsample) {
            static constexpr auto SIZE = 16;
            for (std::size_t y = 0; y < image.get_height(); y += SIZE) {
                for (std::size_t x = 0; x < image.get_width(); x += SIZE) {
                    YCbCrBlock<SIZE, 2> block{image, x, y};
                    // Дискретное косинусное преобразование для компоненты яркости
                    DCY = process_block(block.Y + 0, SIZE, YTable, DCY, YDC_HT, YAC_HT);
                    DCY = process_block(block.Y + 8, SIZE, YTable, DCY, YDC_HT, YAC_HT);
                    DCY = process_block(block.Y + 128, SIZE, YTable, DCY, YDC_HT, YAC_HT);
                    DCY = process_block(block.Y + 136, SIZE, YTable, DCY, YDC_HT, YAC_HT);
                    //  Дискретное косинусное преобразование цветовых компонент
                    DCU = process_block(block.Cb, 8, UVTable, DCU, UVDC_HT, UVAC_HT);
                    DCV = process_block(block.Cr, 8, UVTable, DCV, UVDC_HT, UVAC_HT);
                }
            }
        }
        else {
            static constexpr auto SIZE = 8;
            for (std::size_t y = 0; y < image.get_height(); y += SIZE) {
                for (std::size_t x = 0; x < image.get_width(); x += SIZE) {
                    YCbCrBlock<SIZE> block{image, x, y};
                    // Дискретное косинусное преобразование для компоненты яркости
                    DCY = process_block(block.Y, SIZE, YTable, DCY, YDC_HT, YAC_HT);
                    //  Дискретное косинусное преобразование цветовых компонент
                    DCU = process_block(block.Cb, SIZE, UVTable, DCU, UVDC_HT, UVAC_HT);
                    DCV = process_block(block.Cr, SIZE, UVTable, DCV, UVDC_HT, UVAC_HT);
                }
            }
        }

        // Do the bit alignment of the EOI marker
        static const unsigned short fill_bits[] = {0x7F, 0x07};
        m_output << fill_bits << 0xFF << 0xD9;

        m_file = std::ofstream{file_name, std::ios::binary};
        if (!m_file.is_open()) {
            throw std::runtime_error("Cannot open output file: " + file_name);
        }
        const auto & result = m_output.get();
        m_file.write(reinterpret_cast<const char *>(result.data()), result.size());
        m_file.close();
        return true;
    }

private:
    Output m_output;
    std::ofstream m_file;
};
