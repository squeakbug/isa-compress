#include <bitset>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <iterator>
#include <exception>
#include <sstream>

#include "elfio/elfio.hpp"

#include "utils.h"
#include "config.h"
#include "command.h"
#include "size_stat.h"
#include "dynbitset.h"
#include "encode_table.h"
#include "compressed_section.h"

#include <iostream>

namespace utils
{

const size_t RV64I_CMDLEN = 8;

const size_t RV32I_CMDLEN = 4;
const size_t RV32I_CMDLEN_O = 3;
const size_t RV32I_CMDLEN_H = 2;
const size_t RV32I_CMDLEN_Q = 1;

const size_t DICT_INDX_SIZE = 14; // 14

const size_t MASK_SINGLE_POS_SIZE = 3;
const size_t MASK_SINGLE_MASK_SIZE = 4;
const size_t MASK_SINGLE_INDX_SIZE = 13; // 13

const size_t MASK_DUO_POS_SIZE = 2;
const size_t MASK_DUO_MASK_SIZE = 4;
const size_t MASK_DUO_INDX_SIZE = 6; // 6

const size_t MASK_QUAD_POS_SIZE = 2;
const size_t MASK_QUAD_MASK_SIZE = 2;
const size_t MASK_QUAD_INDX_SIZE = 3; // 2

const size_t MASK_OPERS_POS_SIZE = 3;
const size_t MASK_OPERS_MASK_SIZE = 3;
const size_t MASK_OPERS_INDX_SIZE = 10; // 10

/*
bellman_ford            123997  108918  104473  112014  123186  105580
dijkastra               124233  109175  104696  112255  123503  105756
qsort                   113327  100890  94843   99152   109556  97324

fft                     95230   87658   78302   83598   91735   79110
blur_image              83617   79582   68094   73022   80076   69056
negative_image          79998   76945   64870   69555   75010   65881
rgb_to_gray             82775   78919   67560   72184   77791   68259
sha256                  72505   70478   56529   60220   64904   57509

mersenne_twister        4455    4109    4194    4227    4514    4294
*/

ELFIO::section *get_section_with_name(const ELFIO::elfio *file, const std::string &name)
{
    bool finded = false;
    ELFIO::section *code_section = nullptr;
    for (int i = 0; !finded && i < file->sections.size(); ++i)
    {
        if (file->sections[i]->get_name() == name)
        {
            code_section = file->sections[i];
            finded = true;
        }
    }
    return code_section;
}


// doesn't work properly
std::vector<command> riscv64ci_get_commands(const ELFIO::section *sec_text, config cfg)
{
    const char *data = sec_text->get_data();
    ELFIO::Elf_Xword data_len = sec_text->get_size();
    std::vector<command> commands;

    for (ELFIO::Elf_Xword i = data_len - 1; i >= 0;) {
        std::vector<char> fetch_cmd_data;
        if ((data[i] ^ 0x3) != 0) 
        { // 16 bit instruction
            for (int j = 1; j >= 0; --j) {
                fetch_cmd_data.push_back(data[i - j]);
            }
            i -= 2;
        }
        else if ((data[i] ^ 0x1c) != 0)
        { // 32 bit instruction
            for (int j = 3; j >= 0; --j)
                fetch_cmd_data.push_back(data[i - j]);
            i -= 4;
        }
        else if ((data[i] ^ 0x1f) == 0)
        { // 48 bit instruction
            for (int j = 5; j >= 0; --j)
                fetch_cmd_data.push_back(data[i - j]);
            i -= 6;
        }
        else if ((data[i] ^ 0x3f) == 0)
        { // 64 bit instruction
            for (int j = 7; j >= 0; --j)
                fetch_cmd_data.push_back(data[i - j]);
            i -= 8;
        }
        else
        {
            throw std::logic_error("Not yet supported");
        }
        std::reverse(fetch_cmd_data.begin(), fetch_cmd_data.end());
        command fetch_cmd;
        fetch_cmd.add(fetch_cmd_data, fetch_cmd_data.size() << 3);
        commands.push_back(fetch_cmd);
    }
    return commands;
}

template<size_t CMDLEN>
std::vector<utils::command> get_commands(const ELFIO::section *sec_text)
{
    const char *data = sec_text->get_data();
    ELFIO::Elf_Xword data_len = sec_text->get_size();

    if (data_len % CMDLEN != 0) {
        throw std::runtime_error("Section size must be divided by cmdlen");
    }
    //data_len -= data_len % CMDLEN;

    std::vector<utils::command> commands;
    const size_t cmdlen_bits = CMDLEN << 3;

    for (ELFIO::Elf_Xword i = 0; i < data_len; i += CMDLEN) {
        command comm;
        comm.add(data + i, cmdlen_bits);
        commands.push_back(comm);
    }
    return commands;
}

compressed_section get_compressed_section(const ELFIO::section *sec_text, encode_type &etype)
{
    const char *data = sec_text->get_data();
    size_t code_sections_size = sec_text->get_size();
    char *data_cpy = new char[code_sections_size];
    size_t data_size = code_sections_size - sizeof(char);
    char etype_data;
    char metadata;

    memcpy(&metadata, data, sizeof(char));
    memcpy(data_cpy, data + sizeof(char), data_size);
    compressed_section csec;
    csec.add(data_cpy, (data_size << 3) - ((metadata >> 5) & 0x7));
    delete[] data_cpy;

    etype_data = metadata & 0x1f;
    etype = (encode_type)etype_data;
    return csec;
}


template<size_t INDX_SIZE>
void mask_single_make_encode_table(const std::vector<command> &commands, const config &cfg, encode_table<RV32I_CMDLEN, INDX_SIZE> &entab)
{
    dict_make_encode_table(commands, cfg, entab);
}

template<size_t P1_SIZE, size_t P2_SIZE, size_t INDX1_SIZE, size_t INDX2_SIZE>
void mask_duo_make_encode_table(const std::vector<command> &commands, const config &cfg, encode_table<P1_SIZE, INDX1_SIZE> &entab1, encode_table<P2_SIZE, INDX2_SIZE> &entab2)
{
    std::vector<command> cmds1, cmds2;

    for (const auto & cmd : commands)
    {
        command cmd1, cmd2;
        
        cmd.devide(cmd1, cmd2, P1_SIZE << 3);

        cmds1.push_back(cmd1);
        cmds2.push_back(cmd2);
    }

    dict_make_encode_table(cmds1, cfg, entab1);
    dict_make_encode_table(cmds2, cfg, entab2);
}

template<size_t INDX_SIZE>
void mask_quad_make_encode_table(const std::vector<command> &commands, const config &cfg, std::array<encode_table<RV32I_CMDLEN_Q, INDX_SIZE>, 4> &entabs)
{
    std::vector<command> cmds11, cmds12, cmds21, cmds22;

    for (const auto & cmd : commands)
    {
        command cmd1, cmd2, cmd11, cmd12, cmd21, cmd22;

        cmd.devide_half(cmd1, cmd2);
        cmd1.devide_half(cmd11, cmd12);
        cmd2.devide_half(cmd21, cmd22);

        cmds11.push_back(cmd11);
        cmds12.push_back(cmd12);
        cmds21.push_back(cmd21);
        cmds22.push_back(cmd22);
    }

    dict_make_encode_table(cmds11, cfg, entabs[0]);
    dict_make_encode_table(cmds12, cfg, entabs[1]);
    dict_make_encode_table(cmds21, cfg, entabs[2]);
    dict_make_encode_table(cmds22, cfg, entabs[3]);
}

template<size_t INDX_SIZE_H, size_t INDX_SIZE_Q>
void mask_duo_quad_make_encode_table(const std::vector<command> &commands, const config &cfg, encode_table<RV32I_CMDLEN_H, INDX_SIZE_H> &entab1, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab21, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab22)
{
    std::vector<command> cmds1, cmds21, cmds22;

    for (const auto & cmd : commands)
    {
        command cmd1, cmd2, cmd21, cmd22;

        cmd.devide_half(cmd1, cmd2);
        cmd2.devide_half(cmd21, cmd22);

        cmds1.push_back(cmd1);
        cmds21.push_back(cmd21);
        cmds22.push_back(cmd22);
    }

    dict_make_encode_table(cmds1, cfg, entab1);
    dict_make_encode_table(cmds21, cfg, entab21);
    dict_make_encode_table(cmds22, cfg, entab22);
}

template<size_t INDX_SIZE_O, size_t INDX_SIZE_Q>
void mask_operands_opcode_make_encode_table(const std::vector<command> &commands, const config &cfg, encode_table<RV32I_CMDLEN_O, INDX_SIZE_O> &entab_operands, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab_opcode)
{
    std::vector<command> cmds_operands, cmds_opcode;

    for (const auto & cmd : commands)
    {
        command cmd_operands, cmd_opcode;

        cmd.devide(cmd_opcode, cmd_operands, RV32I_CMDLEN_Q << 3);

        cmds_operands.push_back(cmd_operands);
        cmds_opcode.push_back(cmd_opcode);
    }

    dict_make_encode_table(cmds_operands, cfg, entab_operands);
    dict_make_encode_table(cmds_opcode, cfg, entab_opcode);
}


#ifdef BENCH_COVERAGE
void update_counters(const comp_cmd_type &tp, int &dict_cnt, int &mask_cnt, int &notc_cnt)
{
    if (tp == comp_cmd_type::DICT) {
        dict_cnt++;
    } else if (tp == comp_cmd_type::MASK) {
        mask_cnt++;
    } else {
        notc_cnt++;
    }
}

void update_counters(const comp_cmd_type &tp, int &dict_cnt, int &notc_cnt)
{
    if (tp == comp_cmd_type::DICT) {
        dict_cnt++;
    } else {
        notc_cnt++;
    }
}
#endif

template<size_t INDX_SIZE>
compressed_section encode_code_section_dictionary(std::vector<command> commands, encode_table<RV32I_CMDLEN, INDX_SIZE> entab)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict_cnt = 0;
    int notc_cnt = 0;
#endif

    for (auto comm : commands)
    {
#ifdef BENCH_COVERAGE
        comp_cmd_type tp;
        csec.add(compress_command_with_dictionary(entab, comm, tp));

        update_counters(tp, dict_cnt, notc_cnt);
#else
        csec.add(compress_command_with_dictionary(entab, comm));
#endif
    }

    return csec;
}

template<size_t INDX_SIZE>
compressed_section encode_code_section_mask_single(std::vector<command> commands, encode_table<RV32I_CMDLEN, INDX_SIZE> entab)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict_cnt = 0;
    int mask_cnt = 0;
    int notc_cnt = 0;
#endif

    for (auto comm : commands)
    {
#ifdef BENCH_COVERAGE
        comp_cmd_type tp;
        csec.add(compress_command_with_mask<RV32I_CMDLEN, MASK_SINGLE_POS_SIZE, MASK_SINGLE_MASK_SIZE, MASK_SINGLE_INDX_SIZE>(entab, comm, tp));

        update_counters(tp, dict_cnt, mask_cnt, notc_cnt);
#else
        csec.add(compress_command_with_mask<RV32I_CMDLEN, MASK_SINGLE_POS_SIZE, MASK_SINGLE_MASK_SIZE, MASK_SINGLE_INDX_SIZE>(entab, comm));
#endif
    }

    return csec;
}

template<size_t INDX_SIZE>
compressed_section encode_code_section_mask_duo(std::vector<command> commands, encode_table<RV32I_CMDLEN_H, INDX_SIZE> entab1, encode_table<RV32I_CMDLEN_H, INDX_SIZE> entab2)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict1_cnt = 0, dict2_cnt = 0;
    int mask1_cnt = 0, mask2_cnt = 0;
    int notc1_cnt = 0, notc2_cnt = 0;
#endif

    for (auto comm : commands)
    {
        command ccmd1, ccmd2;

        comm.devide_half(ccmd1, ccmd2);

#ifdef BENCH_COVERAGE
        comp_cmd_type tp;

        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab1, ccmd1, tp));
        update_counters(tp, dict1_cnt, mask1_cnt, notc1_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab2, ccmd2, tp));
        update_counters(tp, dict2_cnt, mask2_cnt, notc2_cnt);
#else
        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab1, ccmd1));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab2, ccmd2));
#endif
    }

    return csec;
}

template<size_t INDX_SIZE>
compressed_section encode_code_section_mask_quad(std::vector<command> commands, std::array<encode_table<RV32I_CMDLEN_Q, INDX_SIZE>, 4> entabs)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict1_cnt = 0, dict2_cnt = 0, dict3_cnt = 0, dict4_cnt = 0;
    int mask1_cnt = 0, mask2_cnt = 0, mask3_cnt = 0, mask4_cnt = 0;
    int notc1_cnt = 0, notc2_cnt = 0, notc3_cnt = 0, notc4_cnt = 0;
#endif

    for (auto comm : commands)
    {
        command ccmd1, ccmd2, ccmd11, ccmd12, ccmd21, ccmd22;

        comm.devide_half(ccmd1, ccmd2);
        ccmd1.devide_half(ccmd11, ccmd12);
        ccmd2.devide_half(ccmd21, ccmd22);

#ifdef BENCH_COVERAGE
        comp_cmd_type tp;

        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[0], ccmd11, tp));
        update_counters(tp, dict1_cnt, mask1_cnt, notc1_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[1], ccmd12, tp));
        update_counters(tp, dict2_cnt, mask2_cnt, notc2_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[2], ccmd21, tp));
        update_counters(tp, dict3_cnt, mask3_cnt, notc3_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[3], ccmd22, tp));
        update_counters(tp, dict4_cnt, mask4_cnt, notc4_cnt);
#else
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[0], ccmd11));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[1], ccmd12));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[2], ccmd21));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entabs[3], ccmd22));
#endif
    }

    return csec;
}

template<size_t INDX_SIZE_H, size_t INDX_SIZE_Q>
compressed_section encode_code_section_mask_duo_quad(std::vector<command> commands, encode_table<RV32I_CMDLEN_H, INDX_SIZE_H> entab1, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> entab2, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> entab3)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict1_cnt = 0, dict2_cnt = 0, dict3_cnt = 0;
    int mask1_cnt = 0, mask2_cnt = 0, mask3_cnt = 0;
    int notc1_cnt = 0, notc2_cnt = 0, notc3_cnt = 0;
#endif

    for (auto comm : commands)
    {
        command ccmd1, ccmd2, ccmd21, ccmd22;

        comm.devide_half(ccmd1, ccmd2);
        ccmd2.devide_half(ccmd21, ccmd22);

#ifdef BENCH_COVERAGE
        comp_cmd_type tp;

        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab1, ccmd1, tp));
        update_counters(tp, dict1_cnt, mask1_cnt, notc1_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab2, ccmd21, tp));
        update_counters(tp, dict2_cnt, mask2_cnt, notc2_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab3, ccmd22, tp));
        update_counters(tp, dict3_cnt, mask3_cnt, notc3_cnt);
#else
        csec.add(compress_command_with_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(entab1, ccmd1));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab2, ccmd21));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab3, ccmd22));
#endif
    }

    return csec;
}

template<size_t INDX_SIZE_O, size_t INDX_SIZE_Q>
compressed_section encode_code_section_operands_opcode(std::vector<command> commands, encode_table<RV32I_CMDLEN_O, INDX_SIZE_O> entab_operands, encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> entab_opcode)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict1_cnt = 0, dict2_cnt = 0;
    int mask1_cnt = 0, mask2_cnt = 0;
    int notc1_cnt = 0, notc2_cnt = 0;
#endif

    for (auto comm : commands)
    {
        command cmd_operands, cmd_opcode;
        comm.devide(cmd_opcode, cmd_operands, RV32I_CMDLEN_Q << 3);
#ifdef BENCH_COVERAGE
        comp_cmd_type tp;
        csec.add(compress_command_with_mask<RV32I_CMDLEN_O, MASK_OPERS_POS_SIZE, MASK_OPERS_MASK_SIZE, MASK_OPERS_INDX_SIZE>(entab_operands, cmd_operands, tp));
        update_counters(tp, dict1_cnt, mask1_cnt, notc1_cnt);
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab_opcode, cmd_opcode, tp));
        update_counters(tp, dict2_cnt, mask2_cnt, notc2_cnt);
#else
        csec.add(compress_command_with_mask<RV32I_CMDLEN_O, MASK_OPERS_POS_SIZE, MASK_OPERS_MASK_SIZE, MASK_OPERS_INDX_SIZE>(entab_operands, cmd_operands));
        csec.add(compress_command_with_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(entab_opcode, cmd_opcode));
#endif
    }

    return csec;
}

template<size_t P1SIZE, size_t P2SIZE, size_t POS1_SIZE, size_t POS2_SIZE, size_t MASK1_SIZE, size_t MASK2_SIZE, size_t INDX1_SIZE, size_t INDX2_SIZE>
compressed_section encode_code_section_mask_duo_p(std::vector<command> commands, encode_table<P1SIZE, INDX1_SIZE> entab1, encode_table<P2SIZE, INDX2_SIZE> entab2)
{
    compressed_section csec;

#ifdef BENCH_COVERAGE
    int dict1_cnt = 0, dict2_cnt = 0;
    int mask1_cnt = 0, mask2_cnt = 0;
    int notc1_cnt = 0, notc2_cnt = 0;
#endif

    for (auto comm : commands)
    {
        command ccmd1, ccmd2;

        comm.devide(ccmd1, ccmd2, P1SIZE << 3);

#ifdef BENCH_COVERAGE
        comp_cmd_type tp;

        csec.add(compress_command_with_mask<P1SIZE, POS1_SIZE, MASK1_SIZE, INDX1_SIZE>(entab1, ccmd1, tp));
        update_counters(tp, dict1_cnt, mask1_cnt, notc1_cnt);

        csec.add(compress_command_with_mask<P2SIZE, POS2_SIZE, MASK2_SIZE, INDX2_SIZE>(entab2, ccmd2, tp));
        update_counters(tp, dict2_cnt, mask2_cnt, notc2_cnt);
#else
        csec.add(compress_command_with_mask<P1SIZE, POS1_SIZE, MASK1_SIZE, INDX1_SIZE>(entab1, ccmd1));
        csec.add(compress_command_with_mask<P2SIZE, POS2_SIZE, MASK2_SIZE, INDX2_SIZE>(entab2, ccmd2));
#endif
    }

    return csec;
}


template<size_t INDX_SIZE>
std::vector<command> rv32i_dict_restore_section_commands(const compressed_section &csec, const encode_table<RV32I_CMDLEN, INDX_SIZE> &entab)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();
    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd = restore_block_dict<RV32I_CMDLEN, DICT_INDX_SIZE>(csec, pos, entab);
        retval.push_back(cmd);

        // Сделать проверку, если невозможно восстановить команду по оставшемуся количеству бит,
        // то отбросить их (чтобы невелировать добавление лишних команд)
    }

    return retval;
}

template<size_t INDX_SIZE>
std::vector<command> rv32i_mask_single_restore_section_commands(const compressed_section &csec, const encode_table<RV32I_CMDLEN, INDX_SIZE> &entab)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();
    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd = restore_block_mask<RV32I_CMDLEN, MASK_SINGLE_POS_SIZE, MASK_SINGLE_MASK_SIZE, MASK_SINGLE_INDX_SIZE>(csec, pos, entab);

        retval.push_back(cmd);
    }

    return retval;
}

template<size_t INDX_SIZE>
std::vector<command> rv32i_mask_duo_restore_section_commands(const compressed_section &csec, const encode_table<RV32I_CMDLEN_H, INDX_SIZE> &entab1, const encode_table<RV32I_CMDLEN_H, INDX_SIZE> &entab2)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();

    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd1 = restore_block_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(csec, pos, entab1);
        command cmd2 = restore_block_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(csec, pos, entab2);

        cmd1.add(cmd2);
        
        retval.push_back(cmd1);
    }

    return retval;
}

template<size_t INDX_SIZE>
std::vector<command> rv32i_mask_quad_restore_section_commands(const compressed_section &csec, const std::array<encode_table<RV32I_CMDLEN_Q, INDX_SIZE>, 4> &entabs)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();
    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd11 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entabs[0]);
        command cmd12 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entabs[1]);
        command cmd21 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entabs[2]);
        command cmd22 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entabs[3]);
        cmd11.add(cmd12);
        cmd11.add(cmd21);
        cmd11.add(cmd22);
        retval.push_back(cmd11);
    }

    return retval;
}

template<size_t INDX_SIZE_H, size_t INDX_SIZE_Q>
std::vector<command> rv32i_mask_duo_quad_restore_section_commands(const compressed_section &csec, const encode_table<RV32I_CMDLEN_H, INDX_SIZE_H> &entab1, const encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab21, const encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab22)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();
    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd1 = restore_block_mask<RV32I_CMDLEN_H, MASK_DUO_POS_SIZE, MASK_DUO_MASK_SIZE, MASK_DUO_INDX_SIZE>(csec, pos, entab1);
        command cmd21 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entab21);
        command cmd22 = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entab22);
        cmd1.add(cmd21);
        cmd1.add(cmd22);
        retval.push_back(cmd1);
    }

    return retval;
}

template<size_t INDX_SIZE_O, size_t INDX_SIZE_Q>
std::vector<command> rv32i_operands_opcode_restore_section_commands(const compressed_section &csec, const encode_table<RV32I_CMDLEN_O, INDX_SIZE_O> &entab_operands, const encode_table<RV32I_CMDLEN_Q, INDX_SIZE_Q> &entab_opcode)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();
    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd_operands = restore_block_mask<RV32I_CMDLEN_O, MASK_OPERS_POS_SIZE, MASK_OPERS_MASK_SIZE, MASK_OPERS_INDX_SIZE>(csec, pos, entab_operands);
        command cmd_opcode = restore_block_mask<RV32I_CMDLEN_Q, MASK_QUAD_POS_SIZE, MASK_QUAD_MASK_SIZE, MASK_QUAD_INDX_SIZE>(csec, pos, entab_opcode);

        cmd_opcode.add(cmd_operands);
        retval.push_back(cmd_opcode);
    }

    return retval;
}

template<size_t P1SIZE, size_t P2SIZE, size_t POS1_SIZE, size_t POS2_SIZE, size_t MASK1_SIZE, size_t MASK2_SIZE, size_t INDX1_SIZE, size_t INDX2_SIZE>
std::vector<command> rv64i_mask_duo_restore_section_commands(const compressed_section &csec, const encode_table<P1SIZE, INDX1_SIZE> &entab1, const encode_table<P2SIZE, INDX2_SIZE> &entab2)
{
    std::vector<command> retval;

    size_t csec_end = csec.get_data_sz_bits();

    for (size_t pos = 0; pos < csec_end;)
    {
        command cmd1 = restore_block_mask<P1SIZE, POS1_SIZE, MASK1_SIZE, INDX1_SIZE>(csec, pos, entab1);
        command cmd2 = restore_block_mask<P2SIZE, POS2_SIZE, MASK2_SIZE, INDX2_SIZE>(csec, pos, entab2);

        cmd1.add(cmd2);
        
        retval.push_back(cmd1);
    }

    return retval;
}


ELFIO::section* modify_code_section(ELFIO::section *sec_text, compressed_section csec, encode_type etype)
{
    size_t data_size = csec.get_data_sz();
    char last_free_bits = (8 - (csec.get_data_sz_bits() % 8)) % 8;

    size_t metadata_size = sizeof(char);
    char etype_data = (char)etype & 0x1f;
    char metadata = (last_free_bits << 5) | etype_data;

    char *data = new char[data_size + metadata_size];
    memcpy(data, &metadata, sizeof(char));
    memcpy(data + sizeof(char), csec.data(), data_size * sizeof(char));
    sec_text->set_data(data, data_size + metadata_size);
    sec_text->set_size(data_size + metadata_size);
    delete[] data;
    return sec_text;
}

ELFIO::section* restore_code_section(ELFIO::section *sec_text, const std::vector<command> &dcmds)
{
    size_t data_size = 0;
    for (const auto &cmd : dcmds)
        data_size += cmd.get_data_sz();

    size_t init_size = sec_text->get_size();

    char *data = new char[data_size];
    size_t pos = 0;
    for (const auto &cmd : dcmds)
    {
        const char *cmd_data = cmd.data();
        memcpy(data + pos, cmd_data, cmd.get_data_sz());
        pos += cmd.get_data_sz();
    }
    sec_text->set_data(data, init_size);
    sec_text->append_data(data + init_size, data_size - init_size);

    delete[] data;
    return sec_text;
}

ELFIO::elfio* build_exec_file(ELFIO::elfio *file, ELFIO::section* code_section)
{
    return file;
}


template<size_t CMDLEN, size_t INDX_SIZE>
std::vector<char> form_inst_dict_data(const encode_table<CMDLEN, INDX_SIZE> &entab)
{
    std::vector<char> data;
    std::vector<command> entries = entab.get_entries();
    for (const auto & cmd : entries)
    {
        const char *cmd_data = cmd.data();
        for (size_t i = 0; i < CMDLEN; ++i)
            data.push_back(cmd_data[i]);
    }
    return data;
}

template<size_t CMDLEN, size_t INDX_SIZE>
ELFIO::elfio* write_instr_dictionary(ELFIO::elfio *file, const encode_table<CMDLEN, INDX_SIZE> &entab, const std::string &section_name)
{
    ELFIO::section *dict_sec = file->sections.add(section_name);
    dict_sec->set_type( ELFIO::SHT_PROGBITS );
    dict_sec->set_addr_align( 0x1 );
    auto text_32 = form_inst_dict_data(entab);
    dict_sec->append_data(text_32.data(), text_32.size());
    return file;
}

template<size_t CMDLEN, size_t INDX_SIZE>
void read_instr_dictionary(const ELFIO::elfio *file, encode_table<CMDLEN, INDX_SIZE> &entab, const std::string &section_name)
{
    ELFIO::section *dict_section = get_section_with_name(file, section_name);
    if (dict_section == nullptr)
    {
        throw std::runtime_error("No section with name: " + section_name);
    }

    std::vector<command> entries;
    const size_t cmdlen_bits = CMDLEN << 3;
    const char *section_data = dict_section->get_data();
    size_t dict_section_sz = dict_section->get_size();
    for (size_t i = 0; i < dict_section_sz; i += CMDLEN)
    {
        command cmd;
        cmd.add(section_data + i, cmdlen_bits);
        entries.push_back(cmd);
    }

    entab = encode_table<CMDLEN, INDX_SIZE>(entries);
}

ELFIO::elfio* write_addr_dictionary(ELFIO::elfio *file, std::vector<command> commands)
{
    ELFIO::section* text_sec = file->sections.add( ".dict.addr" );
    text_sec->set_type( ELFIO::SHT_PROGBITS );

    return file;
}


template<size_t CMDLEN, size_t INDX_SIZE>
std::string entab_to_string(const encode_table<CMDLEN, INDX_SIZE> &entab)
{
    auto entries = entab.get_entries();
    std::stringstream dict_stream;
    int indx = 1;
    for (const auto &e : entries)
    {
        std::string tmp = std::to_string(indx);
        tmp += ") ";
        dict_stream << tmp;
        dict_stream << e;
        dict_stream << "\n";
        indx++;
    }
    return dict_stream.str();
}

void rv32i_dict_compress_section(ELFIO::elfio *&file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, const std::vector<command> &section_commands, const config &cfg)
{
    encode_table<RV32I_CMDLEN, DICT_INDX_SIZE> entab;
    dict_make_encode_table(section_commands, cfg, entab);

    compressed_section encoded_data = encode_code_section_dictionary(section_commands, entab);
    
    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab));
    dict_infos = dicts;

    szstat.dict_32_bit_size = entab.get_entries_cnt() * RV32I_CMDLEN;
    szstat.final_code_size = encoded_data.get_data_sz() + 1;

    section = modify_code_section(section, encoded_data, encode_type::DICT);
    file = write_instr_dictionary(file, entab, ".dict");
}

void rv32i_mask_single_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, const std::vector<command> &section_commands, const config &cfg)
{
    encode_table<RV32I_CMDLEN, MASK_SINGLE_INDX_SIZE> entab;
    mask_single_make_encode_table(section_commands, cfg, entab);

    compressed_section encoded_data = encode_code_section_mask_single(section_commands, entab);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab));
    dict_infos = dicts;

    szstat.dict_32_bit_size = entab.get_entries_cnt() * RV32I_CMDLEN;
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_SINGLE);
    file = write_instr_dictionary(file, entab, ".dict");
}

void rv32i_mask_duo_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, std::vector<command> section_commands, config cfg)
{
    encode_table<RV32I_CMDLEN_H, MASK_DUO_INDX_SIZE> entab1, entab2;
    mask_duo_make_encode_table(section_commands, cfg, entab1, entab2);

    compressed_section encoded_data = encode_code_section_mask_duo(section_commands, entab1, entab2);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab1));
    dicts.push_back(entab_to_string(entab2));
    dict_infos = dicts;

    szstat.dict_32_bit_size = (entab1.get_entries_cnt() + entab2.get_entries_cnt()) * RV32I_CMDLEN_H;
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_DUO);
    file = write_instr_dictionary(file, entab1, ".dict.1");
    file = write_instr_dictionary(file, entab2, ".dict.2");
}

void rv32i_mask_quad_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, std::vector<command> section_commands, config cfg)
{
    std::array<encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE>, 4> entabs;
    mask_quad_make_encode_table(section_commands, cfg, entabs);

    compressed_section encoded_data = encode_code_section_mask_quad(section_commands, entabs);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entabs[0]));
    dicts.push_back(entab_to_string(entabs[1]));
    dicts.push_back(entab_to_string(entabs[2]));
    dicts.push_back(entab_to_string(entabs[3]));
    dict_infos = dicts;

    szstat.dict_32_bit_size = (entabs[0].get_entries_cnt() + entabs[1].get_entries_cnt() 
        + entabs[2].get_entries_cnt() + entabs[3].get_entries_cnt()) * RV32I_CMDLEN_Q;
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_QUAD);
    file = write_instr_dictionary(file, entabs[0], ".dict.11");
    file = write_instr_dictionary(file, entabs[1], ".dict.12");
    file = write_instr_dictionary(file, entabs[2], ".dict.21");
    file = write_instr_dictionary(file, entabs[3], ".dict.22");
}

void rv32i_mask_duo_quad_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, const std::vector<command> &section_commands, const config &cfg)
{
    encode_table<RV32I_CMDLEN_H, MASK_DUO_INDX_SIZE> entab1;
    encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE> entab21, entab22;
    mask_duo_quad_make_encode_table(section_commands, cfg, entab1, entab21, entab22);

    compressed_section encoded_data = encode_code_section_mask_duo_quad(section_commands, entab1, entab21, entab22);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab1));
    dicts.push_back(entab_to_string(entab21));
    dicts.push_back(entab_to_string(entab22));
    dict_infos = dicts;

    szstat.dict_32_bit_size = entab1.get_entries_cnt() * RV32I_CMDLEN_H + (entab21.get_entries_cnt() + entab22.get_entries_cnt()) * RV32I_CMDLEN_Q;
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_DUO_QUAD);
    file = write_instr_dictionary(file, entab1, ".dict.1");
    file = write_instr_dictionary(file, entab21, ".dict.21");
    file = write_instr_dictionary(file, entab22, ".dict.22");
}

void rv32i_mask_operands_opcode_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, const std::vector<command> &section_commands, const config &cfg)
{
    encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE> entab_opcode;
    encode_table<RV32I_CMDLEN_O, MASK_OPERS_INDX_SIZE> entab_operands;
    mask_operands_opcode_make_encode_table(section_commands, cfg, entab_operands, entab_opcode);

    compressed_section encoded_data = encode_code_section_operands_opcode(section_commands, entab_operands, entab_opcode);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab_operands));
    dicts.push_back(entab_to_string(entab_opcode));
    dict_infos = dicts;

    szstat.dict_32_bit_size = (entab_opcode.get_entries_cnt() * RV32I_CMDLEN_Q + entab_operands.get_entries_cnt() * RV32I_CMDLEN_O);
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_OPERANDS_OPCODE);
    file = write_instr_dictionary(file, entab_operands, ".dict.operands");
    file = write_instr_dictionary(file, entab_opcode, ".dict.opcode");
}

template<size_t P1SIZE, size_t P2SIZE, size_t POS1_SIZE, size_t POS2_SIZE, size_t MASK1_SIZE, size_t MASK2_SIZE, size_t INDX1_SIZE, size_t INDX2_SIZE>
void rv64i_mask_duo_compress_section(ELFIO::elfio *file, ELFIO::section *&section, size_stat &szstat, std::vector<std::string> &dict_infos, std::vector<command> section_commands, config cfg)
{
    encode_table<P1SIZE, INDX1_SIZE> entab1;
    encode_table<P2SIZE, INDX2_SIZE> entab2;
    mask_duo_make_encode_table(section_commands, cfg, entab1, entab2);

    compressed_section encoded_data = encode_code_section_mask_duo_p<P1SIZE, P2SIZE, POS1_SIZE, POS2_SIZE, MASK1_SIZE, MASK2_SIZE, INDX1_SIZE, INDX2_SIZE>(section_commands, entab1, entab2);

    std::vector<std::string> dicts;
    dicts.push_back(entab_to_string(entab1));
    dicts.push_back(entab_to_string(entab2));
    dict_infos = dicts;

    szstat.dict_32_bit_size = (entab1.get_entries_cnt() * P1SIZE + entab2.get_entries_cnt() * P2SIZE);
    szstat.final_code_size = encoded_data.get_data_sz();

    section = modify_code_section(section, encoded_data, encode_type::MASK_DUO);
    file = write_instr_dictionary(file, entab1, ".dict.1");
    file = write_instr_dictionary(file, entab2, ".dict.2");
}


void rv32i_dict_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    encode_table<RV32I_CMDLEN, DICT_INDX_SIZE> entab;
    read_instr_dictionary<RV32I_CMDLEN>(file, entab, ".dict");

    std::vector<command> section_commands = rv32i_dict_restore_section_commands(csec, entab);

    section = restore_code_section(section, section_commands);
}

void rv32i_mask_single_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    encode_table<RV32I_CMDLEN, MASK_SINGLE_INDX_SIZE> entab;
    read_instr_dictionary<RV32I_CMDLEN>(file, entab, ".dict");

    std::vector<command> section_commands = rv32i_mask_single_restore_section_commands(csec, entab);

    section = restore_code_section(section, section_commands);
}

void rv32i_mask_duo_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    encode_table<RV32I_CMDLEN_H, MASK_DUO_INDX_SIZE> entab1, entab2;
    read_instr_dictionary<RV32I_CMDLEN_H>(file, entab1, ".dict.1");
    read_instr_dictionary<RV32I_CMDLEN_H>(file, entab2, ".dict.2");

    std::vector<command> section_commands = rv32i_mask_duo_restore_section_commands(csec, entab1, entab2);

    section = restore_code_section(section, section_commands);
}

void rv32i_mask_quad_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    std::array<encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE>, 4> entabs;
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entabs[0], ".dict.11");
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entabs[1], ".dict.12");
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entabs[2], ".dict.21");
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entabs[3], ".dict.22");

    std::vector<command> section_commands = rv32i_mask_quad_restore_section_commands(csec, entabs);

    section = restore_code_section(section, section_commands);
}

void rv32i_mask_duo_quad_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    encode_table<RV32I_CMDLEN_H, MASK_DUO_INDX_SIZE> entab1;
    encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE> entab21, entab22;
    read_instr_dictionary<RV32I_CMDLEN_H>(file, entab1, ".dict.1");
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entab21, ".dict.21");
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entab22, ".dict.22");

    std::vector<command> section_commands = rv32i_mask_duo_quad_restore_section_commands(csec, entab1, entab21, entab22);

    section = restore_code_section(section, section_commands);
}

void rv32i_mask_operands_opcode_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, const compressed_section &csec)
{
    encode_table<RV32I_CMDLEN_Q, MASK_QUAD_INDX_SIZE> entab_opcode;
    encode_table<RV32I_CMDLEN_O, MASK_OPERS_INDX_SIZE> entab_operands;
    read_instr_dictionary<RV32I_CMDLEN_Q>(file, entab_opcode, ".dict.opcode");
    read_instr_dictionary<RV32I_CMDLEN_O>(file, entab_operands, ".dict.operands");

    std::vector<command> section_commands = rv32i_operands_opcode_restore_section_commands(csec, entab_operands, entab_opcode);

    section = restore_code_section(section, section_commands);
}

template<size_t P1SIZE, size_t P2SIZE, size_t INDX1_SIZE, size_t INDX2_SIZE>
void rv64i_mask_duo_decompress_section(const ELFIO::elfio *file, ELFIO::section *&section, compressed_section csec)
{
    encode_table<P1SIZE, INDX1_SIZE> entab1;
    encode_table<P2SIZE, INDX2_SIZE> entab2;
    read_instr_dictionary<P1SIZE>(file, entab1, ".dict.1");
    read_instr_dictionary<P2SIZE>(file, entab2, ".dict.2");

    constexpr size_t p1_size = 1, p2_size = 7;
    constexpr size_t pos1_size = 2, pos2_size = 8;
    constexpr size_t mask1_size = 2, mask2_size = 8;
    constexpr size_t indx1_size = 2, indx2_size = 8;

    std::vector<command> section_commands = rv64i_mask_duo_restore_section_commands<p1_size, p2_size, pos1_size, pos2_size, mask1_size, mask2_size, indx1_size, indx2_size>(csec, entab1, entab2);

    section = restore_code_section(section, section_commands);
}


ELFIO::elfio* rv64i_compress_executable(size_stat &szstat, std::vector<std::string> &dict_infos, ELFIO::elfio *file, config cfg)
{
    szstat = size_stat { };
    ELFIO::section * code_section = get_section_with_name(file, ".text");
    if (code_section == nullptr)
        throw std::runtime_error("No code section in file");

    szstat.initial_code_size += code_section->get_size();
    std::vector<command> section_commands = get_commands<RV64I_CMDLEN>(code_section);

    encode_type etype = cfg.get_etype();

    /* 8/56 -> 129476
    constexpr size_t p1_size = 1, p2_size = 7;
    constexpr size_t pos1_size = 2, pos2_size = 3;
    constexpr size_t mask1_size = 2, mask2_size = 7;
    constexpr size_t indx1_size = 2, indx2_size = 11;
    */

    /* 16/48 -> 117633
    constexpr size_t p1_size = 2, p2_size = 6;
    constexpr size_t pos1_size = 2, pos2_size = 3;
    constexpr size_t mask1_size = 4, mask2_size = 6;
    constexpr size_t indx1_size = 6, indx2_size = 11;
    */

    // 32/32 -> 99094
    constexpr size_t p1_size = 4, p2_size = 4;
    constexpr size_t pos1_size = 3, pos2_size = 3;
    constexpr size_t mask1_size = 4, mask2_size = 4;
    constexpr size_t indx1_size = 10, indx2_size = 10;  // 2048 * 4
    //

    /* 48/16 -> 116570
    constexpr size_t p1_size = 6, p2_size = 2;
    constexpr size_t pos1_size = 3, pos2_size = 2;
    constexpr size_t mask1_size = 6, mask2_size = 4;
    constexpr size_t indx1_size = 11, indx2_size = 6;
    */

    /* 56/8 -> 121577
    constexpr size_t p1_size = 7, p2_size = 1;
    constexpr size_t pos1_size = 3, pos2_size = 2;
    constexpr size_t mask1_size = 7, mask2_size = 2;
    constexpr size_t indx1_size = 11, indx2_size = 2;  // 4096 * 7
    */

    switch (etype)
    {
        case encode_type::MASK_DUO:
            rv64i_mask_duo_compress_section<p1_size, p2_size, pos1_size, pos2_size, mask1_size, mask2_size, indx1_size, indx2_size>(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        default:
            throw std::runtime_error("Not yet supported encoding type");
    }

    return build_exec_file(file, code_section);
}

ELFIO::elfio* rv64i_decompress_executable(ELFIO::elfio *file)
{
    ELFIO::section * code_section = get_section_with_name(file, ".text");
    if (code_section == nullptr)
        throw std::runtime_error("No code section in file");

    encode_type etype;
    compressed_section csec = get_compressed_section(code_section, etype);

    switch (etype)
    {
        case encode_type::MASK_DUO:
            rv64i_mask_duo_decompress_section<1, 7, 2, 8>(file, code_section, csec);
            break;
        default:
            throw std::runtime_error("Not yet supported encoding type");
    }

    return file;
}

ELFIO::elfio* rv32i_compress_executable(size_stat &szstat, std::vector<std::string> &dict_infos, ELFIO::elfio *file, config cfg)
{
    szstat = size_stat { };
    ELFIO::section * code_section = get_section_with_name(file, ".text");
    if (code_section == nullptr)
        throw std::runtime_error("No code section in file");

    szstat.initial_code_size += code_section->get_size();
    std::vector<command> section_commands = get_commands<RV32I_CMDLEN>(code_section);

    encode_type etype = cfg.get_etype();
    switch (etype)
    {
        case encode_type::DICT:
            rv32i_dict_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        case encode_type::MASK_DUO:
            rv32i_mask_duo_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        case encode_type::MASK_QUAD:
            rv32i_mask_quad_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        case encode_type::MASK_DUO_QUAD:
            rv32i_mask_duo_quad_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        case encode_type::MASK_SINGLE:
            rv32i_mask_single_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        case encode_type::MASK_OPERANDS_OPCODE:
            rv32i_mask_operands_opcode_compress_section(file, code_section, szstat, dict_infos, section_commands, cfg);
            break;
        default:
            throw std::runtime_error("Not yet supported encoding type");
    }

    return build_exec_file(file, code_section);
}

ELFIO::elfio* rv32i_decompress_executable(ELFIO::elfio *file)
{
    ELFIO::section * code_section = get_section_with_name(file, ".text");
    if (code_section == nullptr)
        throw std::runtime_error("No code section in file");

    encode_type etype;
    compressed_section csec = get_compressed_section(code_section, etype);

    switch (etype)
    {
        case encode_type::DICT:
            rv32i_dict_decompress_section(file, code_section, csec);
            break;
        case encode_type::MASK_DUO:
            rv32i_mask_duo_decompress_section(file, code_section, csec);
            break;
        case encode_type::MASK_QUAD:
            rv32i_mask_quad_decompress_section(file, code_section, csec);
            break;
        case encode_type::MASK_DUO_QUAD:
            rv32i_mask_duo_quad_decompress_section(file, code_section, csec);
            break;
        case encode_type::MASK_SINGLE:
            rv32i_mask_single_decompress_section(file, code_section, csec);
            break;
        case encode_type::MASK_OPERANDS_OPCODE:
            rv32i_mask_operands_opcode_decompress_section(file, code_section, csec);
            break;
        default:
            throw std::runtime_error("Not yet supported encoding type");
    }

    return file;
}

ELFIO::elfio* compress_executable(size_stat &szstat, std::vector<std::string> &dict_infos, ELFIO::elfio *file, config cfg)
{
    ELFIO::Elf_Half machine = file->get_machine();
    switch (machine)
    {
        case ELFIO::EM_RISCV:
            return rv32i_compress_executable(szstat, dict_infos, file, cfg);
        default:
            throw std::runtime_error("Not supported machine type");
    }
}

ELFIO::elfio* decompress_executable(ELFIO::elfio *file)
{
    ELFIO::Elf_Half machine = file->get_machine();
    switch (machine)
    {
        case ELFIO::EM_RISCV:
            return rv32i_decompress_executable(file);
        default:
            throw std::runtime_error("Not supported machine type");
    }
}

}