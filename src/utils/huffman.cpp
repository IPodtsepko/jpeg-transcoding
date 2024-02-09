#include <utils/huffman.hpp>

static void calculate_bits(int value, unsigned short bits[2])
{
    int absolute = value < 0 ? -value : value;

    auto & lenght = bits[1];

    lenght = 1;
    while (absolute >>= 1) {
        ++lenght;
    }

    if (value < 0) {
        --value;
    }

    auto mask = (1 << lenght) - 1;
    bits[0] = value & mask;
}

int encode_by_huffman(const int * DU, const int last_dc, const unsigned short dc_huffman_table[256][2], const unsigned short ac_huffman_table[256][2], Output & output)
{
    // Encode DC
    const auto actual_dc = DU[0];
    int delta_dc = actual_dc - last_dc;
    if (delta_dc == 0) {
        output << dc_huffman_table[0];
    }
    else {
        unsigned short bits[2];
        calculate_bits(delta_dc, bits);
        output << dc_huffman_table[bits[1]] << bits;
    }

    // Encode ACs

    // Поиск последнего не равного нулю коэффициента
    std::size_t trailling_zeros_position = 63;
    while (trailling_zeros_position > 0 && DU[trailling_zeros_position] == 0) {
        --trailling_zeros_position;
    }
    ++trailling_zeros_position;

    const unsigned short EOB[2] = {ac_huffman_table[0x00][0], ac_huffman_table[0x00][1]};
    if (trailling_zeros_position == 1) {
        // Все коэффициенты равны нулю
        output << EOB;
        return actual_dc;
    }

    const unsigned short M16zeroes[2] = {ac_huffman_table[0xF0][0], ac_huffman_table[0xF0][1]};
    for (std::size_t i = 1; i < trailling_zeros_position; ++i) {
        // Рассчитываем длину блока из нулей
        auto start_position = i;
        while (DU[i] == 0 && i < trailling_zeros_position) {
            ++i;
        }
        auto number_of_zeros = i - start_position;

        if (number_of_zeros >= 16) {
            int lng = number_of_zeros >> 4;
            for (int nrmarker = 1; nrmarker <= lng; ++nrmarker) {
                output << M16zeroes;
            }
            number_of_zeros &= 15;
        }

        unsigned short bits[2];
        calculate_bits(DU[i], bits);
        output << ac_huffman_table[(number_of_zeros << 4) | bits[1]];
        output << bits;
    }
    if (trailling_zeros_position != 64) {
        output << EOB;
    }
    return actual_dc;
}
