#pragma once

#include <cstddef>

enum class encode_type
{
    DICT,
    MASK_SINGLE,
    MASK_DUO,
    MASK_QUAD,
    MASK_OPERANDS_OPCODE,
    MASK_DUO_QUAD,
};

namespace utils
{

class config_builder;

class config
{
public:
    encode_type get_etype() const;

    friend class config_builder;

private:
    encode_type _etype;
};

class config_builder
{
public:
    config build() const;

    void set_etype(encode_type etype);

private:
    encode_type _etype;
};

}
