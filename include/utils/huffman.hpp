#pragma once

#include <utils/output.hpp>

int encode_by_huffman(const int * DU, const int last_dc, const unsigned short dc_huffman_table[256][2], const unsigned short ac_huffman_table[256][2], Output & output);
