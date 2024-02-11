#include "config.h"

namespace utils
{

encode_type config::get_etype() const
{
    return _etype;
}

config config_builder::build() const
{
    config cfg;

    cfg._etype = _etype;

    return cfg;
}

void config_builder::set_etype(encode_type etype)
{
    _etype = etype;
}
}
