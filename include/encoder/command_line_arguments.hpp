/**
 * @file command_line_arguments.hpp
 * @author IPodtsepko
 */
#pragma once

#include <string>

class CommandLineArguments
{
public:
    CommandLineArguments(int argc, const char * argv[]);

    const std::string & get_input_file_name() const;
    const std::string & get_output_file_name() const;

    std::size_t get_width() const;
    std::size_t get_height() const;
    std::size_t get_components_count() const;
    std::size_t get_quality() const;

private:
    static std::size_t to_size_t(const char * arg);

    static bool validate(const int argc, const char * argv[]);

private:
    const bool m_is_valid = false;
    const std::string m_input_file_name;
    const std::string m_output_file_name;
    const std::size_t m_width;
    const std::size_t m_height;
    const std::size_t m_components_count;
    const std::size_t m_quality;
};
