#include "decoder/decoder.hpp"
#include "decoder/decoding_exception.hpp"

#include <fstream>

// Third-party:
#include <args.hxx>

std::size_t size_of(std::ifstream & file)
{
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    return size;
}

int main(const int argc, const char * argv[])
{
    args::ArgumentParser parser("JPEG Decoder");

    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::ValueFlag<std::string> input_file_name_flag(parser, "input_file_name", "The input file name", {'i', "input"}, args::Options::Required);
    args::ValueFlag<std::string> output_file_name_flag(parser, "output_file_name", "The input file name", {'o', "output"}, args::Options::Required);
    args::ValueFlag<std::string> enhanced_file_name_flag(parser, "enhanced_file_name", "The enhanced file name", {'e', "enhanced"});

    args::Group mode_group(parser, "Modes:", args::Group::Validators::AtMostOne);
    args::Flag compress_and_decode_flag(
            mode_group, "compress-and-decode", "Filter DCT coefficients, decode image and write result bitstream to output file", {"compress-and-decode"});
    args::Flag encode_residuals_flag(
            mode_group, "encode_residuals", "Encode the difference between the AC coefficients of the original image and the predicted one", {"encode_residuals"});
    args::Flag decode_residuals_flag(mode_group, "decode-residuals", "Decompress transcoded image", {"decode_residuals"});

    args::ValueFlag<std::size_t> filter_power_flag(parser, "filter", "The power of the DCT coefficient filter", {'p', "power"}, 16);

    try {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    auto & input_file_name = args::get(input_file_name_flag);
    std::ifstream file(input_file_name, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open input file: " << input_file_name << '\n';
        return 1;
    }

    const auto size = size_of(file);

    BytesList buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        std::cerr << "Cannot read data from file: " << input_file_name << '\n';
        return 2;
    }
    file.close();

    Decoder decoder;
    if (compress_and_decode_flag) {
        decoder.toggle_mode(Decoder::Mode::ZERO_OUT_AND_DECODE).set_dct_filter(args::get(filter_power_flag));
    }
    else if (encode_residuals_flag) {
        decoder.toggle_mode(Decoder::Mode::ENCODE_RESIDUALS)
                .set_dct_filter(args::get(filter_power_flag))
                .set_enhanced_file(args::get(enhanced_file_name_flag));
    }
    else if (decode_residuals_flag) {
        decoder.toggle_mode(Decoder::Mode::DECODE_RESIDUALS)
                .set_dct_filter(args::get(filter_power_flag))
                .set_enhanced_file(args::get(enhanced_file_name_flag));
    }

    try {
        decoder.decode(buffer);
    }
    catch (const DecodingException & e) {
        std::cout << "Error occured while decoding file " << input_file_name << ": " << e.what() << std::endl;
        return 3;
    }

    auto & output_file_name = args::get(output_file_name_flag);

    if (encode_residuals_flag || decode_residuals_flag) {
        decoder.get_output().to_file(output_file_name);
    }
    else {
        std::ofstream output(output_file_name, std::ios::binary);
        if (!output.is_open()) {
            std::cout << "Error opening the output file:" << output_file_name << '\n';
            return 4;
        }

        output << "P" << (decoder.is_color_image() ? 6 : 5) << "\n"
               << decoder.get_width() << " " << decoder.get_height() << "\n255\n";
        output.write(reinterpret_cast<const char *>(decoder.get_image().data()), decoder.get_image_size());
        output.close();
    }

    return 0;
}
