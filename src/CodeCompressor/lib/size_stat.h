#pragma once

#include <cstddef>

namespace utils
{

class size_stat
{
public:
    size_t initial_code_size { 0 };
    size_t final_code_size { 0 };
    size_t dict_32_bit_size { 0 };
    size_t dict_addr_bit_size { 0 };
};

}
