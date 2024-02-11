#pragma once

#include <iostream>
#include <map>

#include "elfio/elfio.hpp"

#include "command.h"
#include "compressed_section.h"
#include "config.h"
#include "dynbitset.h"
#include "encode_table.h"
#include "size_stat.h"

namespace utils
{

enum class comp_cmd_type
{
    DICT,
    MASK,
    NOT
};

ELFIO::elfio *compress_executable(size_stat &szstat, std::vector<std::string> &dict_infos, ELFIO::elfio *file, config cfg);
ELFIO::elfio *decompress_executable(ELFIO::elfio *file);

ELFIO::section *get_section_with_name(const ELFIO::elfio *file, const std::string &name);

template<size_t CMDLEN, size_t INDX_SIZE>
void dict_make_encode_table(const std::vector<command> &commands, const config &cfg, encode_table<CMDLEN, INDX_SIZE> &entab)
{
    std::map<command, unsigned int> data;

    for (auto command : commands) {
        if (data.find(command) != data.end())
            data[command]++;
        else
            data[command] = 1;
    }

    std::vector<std::pair<command, unsigned int>> most_freq_commands;
    for (auto p : data)
        most_freq_commands.push_back(p);
    std::sort(most_freq_commands.begin(), most_freq_commands.end(),
              [](std::pair<command, unsigned int> p1, std::pair<command, unsigned int> p2)
              { return p1.second > p2.second; });

    std::vector<command> entab_entries;
    size_t max_entab_size = (1 << INDX_SIZE);
    for (size_t i = 0; i < max_entab_size && i < most_freq_commands.size(); ++i)
    {
        entab_entries.push_back(most_freq_commands[i].first);
    }

    entab = encode_table<CMDLEN, INDX_SIZE>(entab_entries);
}


static bool find_single_missmatch(size_t &missmatch_pos, size_t poscnt, size_t mask_size, const command &entry, const command &cmd)
{
    size_t missmatch_cnt = 0;
    for (size_t j = 0; missmatch_cnt < 2 && j < poscnt; ++j)
    {
        bool missmacth_finded = false;
        for (size_t k = 0; !missmacth_finded && k < mask_size; ++k)
        {
            size_t cmdpos = j * mask_size + k;
            if (entry.getbit(cmdpos) != cmd.getbit(cmdpos))
            {
                missmacth_finded = true;
                missmatch_pos = j;
            }
        }

        if (missmacth_finded)
            missmatch_cnt++;
    }

    //std::cout << missmatch_cnt << " ";

    return missmatch_cnt == 1;
}

template<size_t CMDLEN, size_t MASK_SIZE>
void get_mask_by_pos(dynbitset &mask, const command &cmd, size_t missmatch_pos)
{
    size_t start_pos = missmatch_pos * MASK_SIZE;
    mask = cmd.getseq(start_pos, start_pos + MASK_SIZE);
}

template<size_t CMDLEN, size_t POS_SIZE, size_t MASK_SIZE, size_t INDX_SIZE>
bool find_mask(dynbitset &mask, size_t &pos, size_t &indx, const encode_table<CMDLEN, INDX_SIZE> &entab, const command &cmd)
{
    bool finded = false;
    const size_t poscnt = 0x1 << POS_SIZE;
    for (size_t i = 0; !finded && i < entab.get_entries_cnt(); ++i)
    {
        command entry = entab[i];
        size_t missmatch_pos;
        if (find_single_missmatch(missmatch_pos, poscnt, MASK_SIZE, entry, cmd))
        {
            get_mask_by_pos<CMDLEN, MASK_SIZE>(mask, cmd, missmatch_pos);
            pos = missmatch_pos;
            indx = i;
            finded = true;
        }
    }

    return finded;
}

template<size_t CMDLEN, size_t POS_SIZE, size_t MASK_SIZE, size_t INDX_SIZE>
#ifdef BENCH_COVERAGE
command compress_command_with_mask(const encode_table<CMDLEN, INDX_SIZE> &entab, const command &comm, comp_cmd_type &ctp)
#else
command compress_command_with_mask(const encode_table<CMDLEN, INDX_SIZE> &entab, const command &comm)
#endif
{
    std::vector<command>::iterator it;
    auto ccmd = command{};
    int indx = 0;
    if ((indx = entab.find(comm)) != -1)
    {
        ccmd.add(true);
        ccmd.add(true);
        ccmd.add(indx, INDX_SIZE);

#ifdef BENCH_COVERAGE
        ctp = comp_cmd_type::DICT;
#endif
    }
    else
    {
        dynbitset mask;
        size_t pos, indx;
        if (find_mask<CMDLEN, POS_SIZE, MASK_SIZE, INDX_SIZE>(mask, pos, indx, entab, comm))
        {
            ccmd.add(true);
            ccmd.add(false);
            ccmd.add(pos, POS_SIZE);
            ccmd.add(mask);
            ccmd.add(indx, INDX_SIZE);

#ifdef BENCH_COVERAGE
            ctp = comp_cmd_type::MASK;
#endif
        }
        else
        {
            ccmd.add(false);
            const char *command_data = comm.data();
            size_t cmdlen_bits = comm.get_data_sz() << 3;
            ccmd.add(command_data, cmdlen_bits);

#ifdef BENCH_COVERAGE
            ctp = comp_cmd_type::NOT;
#endif
        }
    }

    return ccmd;
}

template<size_t CMDLEN, size_t INDX_SIZE>
#ifdef BENCH_COVERAGE
command compress_command_with_dictionary(const encode_table<CMDLEN, INDX_SIZE> &entab, const command &comm, comp_cmd_type &ctp)
#else
command compress_command_with_dictionary(const encode_table<CMDLEN, INDX_SIZE> &entab, const command &comm)
#endif
{
    std::vector<command>::iterator it;
    command ccmd;
    int indx = 0;
    if ((indx = entab.find(comm)) != -1)
    {
        ccmd.add(true);
        ccmd.add(indx, INDX_SIZE);

#ifdef BENCH_COVERAGE
        ctp = comp_cmd_type::DICT;
#endif
    }
    else
    {
        ccmd.add(false);
        const char *command_data = comm.data();
        size_t cmdlen_bits = comm.get_data_sz_bits();
        ccmd.add(command_data, cmdlen_bits);

#ifdef BENCH_COVERAGE
        ctp = comp_cmd_type::NOT;
#endif
    }

    return ccmd;
}

template<size_t CMDLEN, size_t POS_SIZE, size_t MASK_SIZE, size_t INDX_SIZE>
command restore_block_mask(const compressed_section &csec, size_t &pos, const encode_table<CMDLEN, INDX_SIZE> &entab)
{
    command cmd{};
    bool cbit = csec.getbit(pos++);
    if (cbit == true)
    {
        bool mbit = csec.getbit(pos++);
        if (mbit == false)
        {
            dynbitset mask_pos_bitset = csec.getseq(pos, pos + POS_SIZE);
            pos += POS_SIZE;
            size_t mask_pos = mask_pos_bitset.to_size_t();

            dynbitset mask = csec.getseq(pos, pos + MASK_SIZE);
            pos += MASK_SIZE;

            dynbitset indx_bitset = csec.getseq(pos, pos + INDX_SIZE);
            pos += INDX_SIZE;
            size_t indx = indx_bitset.to_size_t();

            cmd = entab[indx];
            for (size_t j = 0; j < MASK_SIZE; ++j)
            {
                bool v = mask.getbit(j);
                cmd.setbit((mask_pos * MASK_SIZE) + j, v);
            }
        }
        else
        {
            dynbitset indx_bitset = csec.getseq(pos, pos + INDX_SIZE);
            pos += INDX_SIZE;
            size_t indx = indx_bitset.to_size_t();
            cmd = entab[indx];
        }
    }
    else
    {
        constexpr size_t cmdlen_bits = CMDLEN << 3;
        for (size_t j = 0; j < cmdlen_bits; ++j)
        {
            cmd.add(csec.getbit(pos++));
        }
    }

    return cmd;
}

template<size_t CMDLEN, size_t INDX_SIZE>
command restore_block_dict(const compressed_section &csec, size_t &pos, const encode_table<CMDLEN, INDX_SIZE> &entab)
{
    command cmd;
    bool cbit = csec.getbit(pos++);
    if (cbit == true)
    {
        dynbitset indx_bitset = csec.getseq(pos, pos + INDX_SIZE);
        pos += INDX_SIZE;
        size_t indx = indx_bitset.to_size_t();
        cmd = entab[indx];
    }
    else
    {
        constexpr size_t cmdlen_bits = CMDLEN << 3;
        for (size_t j = 0; j < cmdlen_bits; ++j)
        {
            cmd.add(csec.getbit(pos++));
        }
    }

    return cmd;
}

}
