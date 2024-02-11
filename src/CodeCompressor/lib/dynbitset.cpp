#include "dynbitset.h"
#include <iostream>

namespace utils
{

dynbitset::dynbitset()
    : _bits(0) { }

void dynbitset::add(bool value)
{
    add_impl(value);
}

void dynbitset::add(size_t value, size_t bitcnt)
{
    for (size_t i = 0; i < bitcnt; ++i)
    {
        bool v = value & 0x1;
        add_impl(v);
        value >>= 1;
    }
}

void dynbitset::add(std::vector<char> bytes, size_t bitcnt)
{
    const char *data = bytes.data();
    for (size_t i = 0; i < bitcnt; ++i)
    {
        bool v = (data[i >> 3] >> (i & 0x7)) & 0x1;
        add_impl(v);
    }
}

void dynbitset::add(const char * data, size_t bitcnt, size_t bitoffset)
{
    if (bitoffset > 7)
    {
        throw std::logic_error("bitoffset must be less then 8");
    }

    size_t lastbit = bitcnt + bitoffset;
    for (size_t i = bitoffset; i < lastbit; ++i)
    {
        bool v = (data[i >> 3] >> (i & 0x7)) & 0x1;
        add_impl(v);
    }
}

void dynbitset::add(const dynbitset &other)
{
    for (size_t i = 0; i < other._bits; ++i)
    {
        add_impl(other.getbit(i));
    }
}

size_t dynbitset::get_data_sz() const
{
    return _data.size();
}

size_t dynbitset::get_data_sz_bits() const
{
    return _bits;
}

const char * dynbitset::data() const
{
    return _data.data();
}

size_t dynbitset::to_size_t() const
{
    size_t retval = 0;

    for (size_t j = 0; j < _bits; ++j)
    {
        retval <<= 1;
        retval |= get_impl(_bits - j - 1);
    }

    return retval;
}

void dynbitset::setbit(size_t pos, bool v)
{
    if (v != 0)
        v = 1;

    size_t bytepos = pos >> 3;
    size_t bitpos = pos & 0x7;

    if (v == 0)
        _data[bytepos] &= (0xff ^ (0x1 << bitpos));
    else
        _data[bytepos] |= 0x1 << bitpos;
}

bool dynbitset::getbit(size_t pos) const
{
    return get_impl(pos);
}

dynbitset dynbitset::getseq(size_t start, size_t end) const
{
    dynbitset new_bitset;

    for (size_t i = start; i < end; ++i)
    {
        bool v = get_impl(i);
        new_bitset.add(v);
    }

    return new_bitset;
}

void dynbitset::add_impl(bool value)
{
    if (value != 0)
        value = 1;
    if (_data.size() * 8 < _bits + 1)
        _data.push_back(0);
    _data[_bits >> 3] |= (value << (_bits & 0x7));
    _bits++;
}

bool dynbitset::get_impl(size_t pos) const
{
    size_t bytepos = pos >> 3;
    size_t bitpos = pos & 0x7;
    return (_data[bytepos] >> bitpos) & 0x1;
}

bool operator<(const dynbitset &s1, const dynbitset &s2)
{
    return s1.to_size_t() < s2.to_size_t();
}

bool operator==(const dynbitset &s1, const dynbitset &s2)
{
    return s1.to_size_t() == s2.to_size_t();
}

bool operator!=(const dynbitset &s1, const dynbitset &s2)
{
    return !(s1 == s2);
}

std::ostream & operator<<(std::ostream &os, const dynbitset &dset)
{
    const char *dset_data = dset.data();
    std::ios_base::fmtflags f(os.flags());
    for (size_t i = 0; i < dset.get_data_sz(); ++i)
    {
        os << "0x" << std::hex << (((int)*(char*)(dset_data + i) & 0xf0) >> 4);
        os << std::hex << (((int)*(char*)(dset_data + i)) & 0x0f);
        os << " ";
    }
    os.flags(f);
    return os;
}

}
