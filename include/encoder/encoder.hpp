/**
 * @file encoder.hpp
 * @author IPodtsepko
 */
#pragma once

#include <string>

namespace utils {

class Image;

}

class Encoder
{
public:
    static bool encode(const std::string & file_name, const utils::Image & image, int quality);
};
