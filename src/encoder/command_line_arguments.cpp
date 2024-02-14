#include <encoder/command_line_arguments.hpp>

CommandLineArguments::CommandLineArguments(int argc, const char * argv[])
    : m_is_valid(validate(argc, argv))
    , m_input_file_name(argv[1])
    , m_output_file_name(argv[2])
    , m_width(to_size_t(argv[3]))
    , m_height(to_size_t(argv[4]))
    , m_components_count(to_size_t(argv[5]))
    , m_quality(to_size_t(argv[6]))
{
    if (m_components_count != 1 && m_components_count != 3 &&
        m_components_count != 4) {
        throw std::invalid_argument("Unsupported components count");
    }

    if (m_quality < 1 || m_quality > 100) {
        throw std::invalid_argument(
                "The quality value should be from 1 to 100 inclusive");
    }
}

const std::string & CommandLineArguments::get_input_file_name() const
{
    return m_input_file_name;
}

const std::string & CommandLineArguments::get_output_file_name() const
{
    return m_output_file_name;
}

std::size_t CommandLineArguments::get_width() const { return m_width; }

std::size_t CommandLineArguments::get_height() const { return m_height; }

std::size_t CommandLineArguments::get_components_count() const
{
    return m_components_count;
}

std::size_t CommandLineArguments::get_quality() const { return m_quality; }

std::size_t CommandLineArguments::to_size_t(const char * arg)
{
    return static_cast<std::size_t>(std::stoi(arg));
}

bool CommandLineArguments::validate(const int argc, const char * argv[])
{
    if (argc < 7) {
        std::string app_name{argv[0]};
        throw std::invalid_argument(
                "There are too few command line arguments\nUsage: " + app_name +
                " <input_file> <output_file> <width> <height> <components> <quality>");
    }
    return true;
}
