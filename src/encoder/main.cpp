#include <encoder/command_line_arguments.hpp>
#include <encoder/encoder.hpp>
#include <fstream>
#include <iostream>
#include <utils/image.hpp>
#include <vector>

std::vector<char> read_bytes_from_file(const std::size_t bytes_count, const std::string & file_name)
{
    std::vector<char> buffer(bytes_count);

    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening input file: " + file_name);
    }

    if (!file.read(buffer.data(), bytes_count)) {
        throw std::runtime_error("Failed to read input file properly.");
    }

    file.close();

    return buffer;
}

int main(int argc, const char * argv[])
{
    try {
        CommandLineArguments args(argc, argv);

        const auto components_count = args.get_components_count();
        const auto width = args.get_width();
        const auto height = args.get_height();

        const auto bytes_count = components_count * width * height;
        const auto image_data = read_bytes_from_file(bytes_count, args.get_input_file_name());

        Image image{width, height, components_count, image_data.data()};
        Encoder().encode(args.get_output_file_name(), image, args.get_quality());
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
