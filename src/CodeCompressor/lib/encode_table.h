#pragma once

#include <vector>
#include <cstddef>
#include <iostream>

#include "command.h"

namespace utils
{

/*
static int ilog2(unsigned int v)
{
    int retval = 0;
    while (v >>= 1) ++retval;
    return retval;
}
*/

template<size_t CMDLEN, size_t INDX_SIZE>
class encode_table
{
public:
    encode_table()
    {

    }

    explicit encode_table(std::vector<command> entries)
    {
        size_t entries_cnt = entries.size();
        if (entries_cnt > (1 << INDX_SIZE))
        {
            throw std::runtime_error("Entries cnt must be less than INDX_SIZE");
        }

        const size_t required_cmdlen = CMDLEN << 3;
        for (const auto &cmd : entries)
        {
            if (cmd.get_data_sz_bits() != required_cmdlen)
                throw std::runtime_error("Command have bad length");
        }

        _entries = std::move(entries);
        std::sort(_entries.begin(), _entries.end());
    }

    size_t get_entries_cnt() const
    {
        return _entries.size();
    }

    const std::vector<command> &get_entries() const
    {
        return _entries;
    }

    const command& operator[](size_t pos) const
    {
        return _entries[pos];
    }

    command &operator[](size_t pos)
    {
        return _entries[pos];
    }

    int find(const command &cmd) const
    {
        auto it = std::lower_bound(_entries.begin(), _entries.end(), cmd);
        if (it == _entries.end() || *it != cmd)
        {
            return -1;
        }
        else
        {
            return std::distance(_entries.begin(), it);
        }
    }

private:
    std::vector<command> _entries;
};

}
