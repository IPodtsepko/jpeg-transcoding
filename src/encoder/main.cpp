#include <args.hxx>
#include <encoder/constants.hpp>
#include <encoder/encoder.hpp>
#include <fmt/core.h>
#include <iostream>
#include <utils/discrete_cosine_transform.hpp>
#include <utils/image.hpp>

int main(int argc, const char * argv[])
{
    args::ArgumentParser parser("JPEG Encoder");

    args::HelpFlag help(parser, "help", "Display this help menu", {"help"});

    args::ValueFlag<std::string> input_file_name(parser, "input_file_name", "The input file name", {'i', "input"}, args::Options::Required);
    args::ValueFlag<std::string> output_file_name(parser, "output_file_name", "The input file name", {'o', "output"}, args::Options::Required);

    args::ValueFlag<std::size_t> width(parser, "width", "The image width", {'w', "width"});
    args::ValueFlag<std::size_t> height(parser, "height", "The image height", {'h', "height"});
    args::ValueFlag<std::size_t> components_count(parser, "components_count", "The image colors count", {'c', "components_count"});

    args::ValueFlag<std::size_t> quality(parser, "quality", "Encoding quality", {'q', "quality"}, 90);

    try {
        parser.ParseCLI(argc, argv);

        if (!width || !height || !components_count) {
            const auto image = utils::Image::from_ppm(args::get(input_file_name));
            Encoder::encode(args::get(output_file_name), image, args::get(quality));
        }
        else {
            const auto image = utils::Image::from_file(args::get(width),
                                                       args::get(height),
                                                       args::get(components_count),
                                                       args::get(input_file_name));
            Encoder::encode(args::get(output_file_name), image, args::get(quality));
        }
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
    catch (const std::invalid_argument & e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (const std::runtime_error & e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 3;
    }

    return 0;
}
