#pragma once

#include "utils/huffman_code.hpp"
#include "utils/image.hpp"
#include "utils/quantization_table.hpp"

#include <map>
#include <string>

/**
 * @brief Implementation of JPEG-transcoder.
 */
struct Decoder
{
    /**
     * @brief Transcoder operation modes.
     */
    enum class Mode
    {
        /** Decoding JPEG images. */
        DEFAULT,

        /** Zero out DCT coefficients and decode JPEG. */
        ZERO_OUT_AND_DECODE,

        /** Encode the residuals of the DCT coefficients in their places. */
        ENCODE_RESIDUALS,

        /** Decode the residuals of the DCT coefficients in their places. */
        DECODE_RESIDUALS,
    };

    Decoder() = default;

    Decoder & set_dct_filter(const std::size_t dct_filter_power);

    Decoder & toggle_mode(const Mode & mode);

    Decoder & set_enhanced_file(const std::string & enhanced_file_name);

    struct HuffmanCodeEntry
    {
        unsigned char m_length = 0;
        unsigned char m_decoded_value = 0;
    };

    struct Sampling
    {
        std::size_t m_y = 1;
        std::size_t m_x = 1;

        Sampling & set_greater(const Sampling & other);
    };

    struct Shape
    {
        std::size_t m_width;
        std::size_t m_height;
    };

    struct Component
    {
        std::size_t m_id = 0;
        Sampling m_sampling{};
        std::size_t m_width = 0;
        std::size_t m_height = 0;
        std::size_t m_stride = 0;
        std::size_t m_quantization_table_id = 0;
        std::size_t m_ac_huffman_table_id = 0;
        std::size_t m_dc_huffman_table_id = 0;

        int m_last_dc = 0;
        utils::HuffmanCode m_huffman_code;
        BytesList m_pixels{};

        Component & set_id(const std::size_t id);

        Component & set_sampling(const std::size_t sampling);

        Component & set_sampling(const std::size_t x, const std::size_t y);

        Component & set_quantization_table(const std::size_t table_id);

        void verify() const;

        std::size_t get_x_sampling() const;

        std::size_t get_y_sampling() const;
    };

    Mode m_mode = Mode::DEFAULT;
    bool m_decoding_finished = false;
    const unsigned char * m_position = nullptr;
    std::size_t m_size = 0;
    std::size_t m_length = 0;
    std::size_t m_width = 0;
    std::size_t m_height = 0;
    Sampling m_sampling{};
    std::vector<Component> m_components{};
    std::map<std::size_t, utils::QuantizationTable> m_quantization_tables;
    HuffmanCodeEntry m_huffman_tables[4][65536];
    std::size_t m_buffer = 0;
    std::size_t m_bits_in_buffer = 0;
    int m_rst_interval = 0;
    BytesList m_rgb{};
    std::vector<std::vector<int>> m_dct_coefficients_distribution{64};

    std::size_t m_dct_filter_power = 0;
    std::array<utils::HuffmanCode::HuffmanTable, 4> m_huffman_encoding_tables;
    bool m_is_scanning = false;
    std::optional<utils::Image> m_enhanced_file;
    Output m_output{};
    std::map<int, std::size_t> m_corrections_statistic;
    std::size_t m_new_zeros_count = 0;
    std::size_t m_corrupted_zeros_count = 0;
    std::vector<int> m_residuals;

    static unsigned char clip(const int x);

    // Mode checks
    bool IsDefaultMode() const;
    bool IsZeroOutAndDecodeMode() const;
    bool IsEncodeResidualsMode() const;
    bool IsDecodeResidualsMode() const;
    bool IsResidualsProcessing() const;

    unsigned char get_bytes(const std::size_t count = 1);

    int read_bits(const std::size_t bits);

    void skip_bits(const std::size_t bits);

    int get_bits(int bits);

    void byte_align();

    void skip(const std::size_t count);

    unsigned short decode_16(const unsigned char * pos);

    void decode_length();

    void skip_marker();

    static bool is_point_of_two(const std::size_t x);

    void decode_start_of_frame();

    static void restore_huffman_codes(Bytes<17> spectrum,
                                      std::vector<std::pair<unsigned short, unsigned short>> & to);
    static void restore_huffman_codes(Bytes<17> & spectrum, int length, int code, std::vector<std::pair<unsigned short, unsigned short>> & to);

    void decode_huffman_tables();

    void decode_quantize_tables();

    void decode_dri(void);

    struct HuffmanDecodingResult
    {
        int m_run = 0;
        int m_level = 0;
        int m_coefficient = 0;
    };

    HuffmanDecodingResult decode_huffman(HuffmanCodeEntry huffman_table[], const std::size_t index = 0, const utils::Mask & mask = utils::MaskAll);

    void decode_block(Component & component, unsigned char * output, utils::DCTCoefficientsFilter & filter, const std::optional<std::array<int, 64>> optional_enhanced_block);

    static std::size_t get_blocks_count(const std::size_t size, std::size_t sampling);

    std::optional<std::array<int, 64>> get_enhanced_coefficients(const Component & component, const std::size_t x, const std::size_t y);

    void decode_start_of_scan(void);

    void horizontal_upsample(Component & component);

    void vertical_upsample(Component & c);

    void convert();

    void reset();

    void decode(const BytesList & jpeg);

    std::size_t get_width() const;

    std::size_t get_height() const;

    bool is_color_image() const;

    const BytesList & get_image() const;

    std::size_t get_image_size() const;

    const Output & get_output() const;
};
