#pragma once

#include <vector>
#include <cstddef>
#include <iostream>

namespace utils
{

class dynbitset
{
public:
    dynbitset();

    void add(bool value);
    void add(size_t value, size_t bitcnt);
    void add(std::vector<char> bytes, size_t bitcnt);
    void add(const char *data, size_t bitcnt, size_t bitoffset = 0);
    void add(const dynbitset &other);

    size_t get_data_sz() const;
    size_t get_data_sz_bits() const;

    const char *data() const;

    size_t to_size_t() const;

    void setbit(size_t pos, bool v);
    bool getbit(size_t pos) const;
    dynbitset getseq(size_t start, size_t end) const;

    friend std::ostream &operator<<(std::ostream &os, const dynbitset &dset);

private:
    void add_impl(bool value);
    bool get_impl(size_t pos) const;

private:
    std::vector<char> _data;
    std::size_t _bits;
};

bool operator<(const dynbitset &s1, const dynbitset &s2);

bool operator==(const dynbitset &s1, const dynbitset &s2);

bool operator!=(const dynbitset &s1, const dynbitset &s2);

std::ostream &operator<<(std::ostream &os, const dynbitset &dset);
}
