#include "command.h"

#include <iostream>

namespace utils
{

void command::devide(command &ocmd1, command &ocmd2, size_t fbits) const
{
    devide_impl(ocmd1, ocmd2, fbits, *this);
}

void command::devide_half(command &ocmd1, command &ocmd2) const
{
    devide_impl(ocmd1, ocmd2, this->get_data_sz_bits() / 2, *this);
}

void command::devide_impl(command &ocmd1, command &ocmd2, size_t fbits, const command &icmd) const
{
    if (icmd.get_data_sz_bits() < fbits)
    {
        throw std::logic_error("Fbits must be less than cmd len");
    }

    const char *icmd_data = icmd.data();
    ocmd1 = command{};
    ocmd1.add(icmd_data, fbits);
    ocmd2 = command{};
    size_t rem = icmd.get_data_sz_bits() - fbits;
    ocmd2.add(icmd_data + (fbits >> 3), rem, fbits % 8);
}

bool operator<(const command &c1, const command &c2)
{
    return c1.to_size_t() < c2.to_size_t();
}

bool operator==(const command &c1, const command &c2)
{
    return c1.to_size_t() == c2.to_size_t();
}

bool operator!=(const command &c1, const command &c2)
{
    return !(c1 == c2);
}

std::ostream & operator<<(std::ostream &os, const command &cmd)
{
    const char *cmd_data = cmd.data();
    std::ios_base::fmtflags f(os.flags());
    os << "0x";
    for (size_t i = 0; i < cmd.get_data_sz(); ++i)
    {
        os << std::hex << (((int)*(char*)(cmd_data + i) & 0xf0) >> 4);
        os << std::hex << (((int)*(char*)(cmd_data + i)) & 0x0f);
    }
    os.flags(f);
    return os;
}

}
