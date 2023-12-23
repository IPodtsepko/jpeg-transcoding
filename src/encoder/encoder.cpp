#include "encoder/encoder.hpp"

#include "encoder/constants.hpp"
#include "encoder/implementation/encoder.hpp"
#include "utils/image.hpp"

#include <string>

bool Encoder::encode(const std::string & file_name, const utils::Image & image, int quality)
{
    Output output;
    implementation::Encoder encoder(quality, output);

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

    output << head0 << encoder.m_luminance_quantization_table.get() << 0x01 // 0_ Values length (1 byte), _1 table id
           << encoder.m_chrominance_quantization_table.get();

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
            static_cast<unsigned char>(encoder.m_subsample ? 0x22 : 0x11), // Subsampling
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
    output << head1 << constants::luminance::dc::SPECTRUM << constants::luminance::dc::VALUES
           << 0x10 // Class: 1_ (AC), table id: _0.
           << constants::luminance::ac::SPECTRUM << constants::luminance::ac::VALUES
           << 0x01 // Class: 0_ (DC), table id: _1.
           << constants::chrominance::dc::SPECTRUM << constants::chrominance::dc::VALUES
           << 0x11 // Class: 1_ (AC), table id: _1.
           << constants::chrominance::ac::SPECTRUM << constants::chrominance::ac::VALUES;

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
    output << head2;

    output.reset();
    encoder.encode(image);

    output.write(0b1111111, 7) // Do the bit alignment of the EOI marker
            << 0xFF << 0xD9;

    output.to_file(file_name);

    return true;
}
