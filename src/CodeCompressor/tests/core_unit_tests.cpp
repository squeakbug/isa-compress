#include <iostream>
#include <cassert>

#include "elfio/elfio.hpp"

#include "../lib/utils.h"
#include "../lib/encode_table.h"

using namespace utils;

namespace utils 
{

extern const size_t RV32I_CMDLEN = 4;

ELFIO::section *get_section_with_name(const ELFIO::elfio *file, const std::string &name);

template<size_t CMDLEN>
std::vector<command> get_commands(const ELFIO::section *sec_text);

}

/* Subroutines */
bool compare_by_text_section(const ELFIO::elfio *f1, const ELFIO::elfio *f2)
{
    const ELFIO::section *s1 = get_section_with_name(f1, ".text");
    const ELFIO::section *s2 = get_section_with_name(f2, ".text");

    const char *d1 = s1->get_data();
    const char *d2 = s2->get_data();
    size_t sz1 = s1->get_size();
    size_t sz2 = s2->get_size();

    if (sz1 != sz2) {
        return false;
    }

    for (size_t sz = 0; sz < sz1 && sz < sz2; ++sz) {
        if (d1[sz] != d2[sz]) {
            std::cout << "Miss in " << sz << " byte" << std::endl;
            return false;
        }
    }

    return true;
}

#define FAILED "\033[1;31mFAILED\033[0m"
#define PASSED "\033[1;32mPASSED\033[0m"

#define ARRLEN(arr) \
    sizeof((arr)) / sizeof((arr[0]))

#define DUMMY_ASSERT(expr) \
    if (!(expr)) {  \
        std::cout << __func__ << ": " << FAILED << std::endl; \
        return 0; \
    }

#define DUMMY_TEST_PASS() \
    std::cout << __func__ << ": " << PASSED << std::endl; \
    return 1;

/* Функциональные тесты */
bool test_mask_duo_compress_executable()
{
    ELFIO::elfio reader;

    const std::string test_filename = "./tests/hello_world-rv32i.o";
    DUMMY_ASSERT(reader.load(test_filename))

    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_DUO);

    utils::size_stat sz_stat;
    std::vector<std::string> dict_infos;
    config cfg = cfg_builder.build();
    compress_executable(sz_stat, dict_infos, &reader, cfg);

    DUMMY_ASSERT(reader.save( "./tests/mask_duo_hello_world-rv32i.o" ))

    DUMMY_TEST_PASS()
}

bool test_mask_duo_decompress_executable()
{
    ELFIO::elfio reader;

    const std::string test_filename = "./tests/mask_duo_hello_world-rv32i.o";
    DUMMY_ASSERT(reader.load(test_filename))

    decompress_executable(&reader);

    DUMMY_ASSERT(reader.save( "./tests/mask_duo_hello_world-rv32i-d.o" ))

    DUMMY_TEST_PASS()
}

bool test_dict_compress_decompress_executable()
{
    ELFIO::elfio reader;
    ELFIO::elfio reader2;
    const std::string ifilename = "./tests/hello_world-rv32i.o";
    const std::string ofilename = "./tests/result.exe";
    const std::string cofilename = "./tests/hello_world-rv32i-d.o";

    DUMMY_ASSERT(reader.load(ifilename))
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::DICT);

    try
    {
        utils::size_stat sz_stat;
        std::vector<std::string> dict_infos;
        config cfg = cfg_builder.build();
        compress_executable(sz_stat, dict_infos, &reader, cfg);

        DUMMY_ASSERT(reader.save( ofilename ))
        DUMMY_ASSERT(reader2.load(ofilename))

        decompress_executable(&reader2);
        DUMMY_ASSERT(reader2.save( cofilename ))
    } 
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    DUMMY_ASSERT(reader.load(ifilename))
    DUMMY_ASSERT(compare_by_text_section(&reader, &reader2))

    DUMMY_TEST_PASS()
}

bool test_mask_single_compress_decompress_executable()
{
    ELFIO::elfio reader;
    ELFIO::elfio reader2;
    const std::string ifilename = "./tests/hello_world-rv32i.o";
    const std::string ofilename = "./tests/result.exe";
    const std::string cofilename = "./tests/hello_world-rv32i-d.o";

    DUMMY_ASSERT(reader.load(ifilename))
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_SINGLE);

    try
    {
        utils::size_stat sz_stat;
        std::vector<std::string> dict_infos;
        config cfg = cfg_builder.build();
        compress_executable(sz_stat, dict_infos, &reader, cfg);

        DUMMY_ASSERT(reader.save( ofilename ))
        DUMMY_ASSERT(reader2.load(ofilename))

        decompress_executable(&reader2);
        DUMMY_ASSERT(reader2.save( cofilename ))
    } 
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    DUMMY_ASSERT(reader.load(ifilename))
    DUMMY_ASSERT(compare_by_text_section(&reader, &reader2))

    DUMMY_TEST_PASS()
}

bool test_mask_duo_compress_decompress_executable()
{
    ELFIO::elfio reader;
    ELFIO::elfio reader2;
    const std::string ifilename = "./tests/hello_world-rv32i.o";
    const std::string ofilename = "./tests/result.exe";
    const std::string cofilename = "./tests/hello_world-rv32i-d.o";

    DUMMY_ASSERT(reader.load(ifilename))
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_DUO);

    try
    {
        utils::size_stat sz_stat;
        std::vector<std::string> dict_infos;
        config cfg = cfg_builder.build();
        compress_executable(sz_stat, dict_infos, &reader, cfg);

        DUMMY_ASSERT(reader.save( ofilename ))
        DUMMY_ASSERT(reader2.load(ofilename))

        decompress_executable(&reader2);
        DUMMY_ASSERT(reader2.save( cofilename ))
    } 
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    DUMMY_ASSERT(reader.load(ifilename))
    DUMMY_ASSERT(compare_by_text_section(&reader, &reader2))

    DUMMY_TEST_PASS()
}

bool test_mask_quad_compress_decompress_executable()
{
    ELFIO::elfio reader;
    ELFIO::elfio reader2;
    const std::string ifilename = "./tests/hello_world-rv32i.o";
    const std::string ofilename = "./tests/result.exe";
    const std::string cofilename = "./tests/hello_world-rv32i-d.o";

    DUMMY_ASSERT(reader.load(ifilename))
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_QUAD);

    try
    {
        utils::size_stat sz_stat;
        std::vector<std::string> dict_infos;
        config cfg = cfg_builder.build();
        compress_executable(sz_stat, dict_infos, &reader, cfg);

        DUMMY_ASSERT(reader.save( ofilename ))
        DUMMY_ASSERT(reader2.load(ofilename))

        decompress_executable(&reader2);
        DUMMY_ASSERT(reader2.save( cofilename ))
    } 
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    DUMMY_ASSERT(reader.load(ifilename))
    DUMMY_ASSERT(compare_by_text_section(&reader, &reader2))

    DUMMY_TEST_PASS()
}

bool test_mask_oper_compress_decompress_executable()
{
    ELFIO::elfio reader;
    ELFIO::elfio reader2;
    const std::string ifilename = "./tests/hello_world-rv32i.o";
    const std::string ofilename = "./tests/result.exe";
    const std::string cofilename = "./tests/hello_world-rv32i-d.o";

    DUMMY_ASSERT(reader.load(ifilename))
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_OPERANDS_OPCODE);

    try
    {
        utils::size_stat sz_stat;
        std::vector<std::string> dict_infos;
        config cfg = cfg_builder.build();
        compress_executable(sz_stat, dict_infos, &reader, cfg);

        DUMMY_ASSERT(reader.save( ofilename ))
        DUMMY_ASSERT(reader2.load(ofilename))

        decompress_executable(&reader2);
        DUMMY_ASSERT(reader2.save( cofilename ))
    } 
    catch (std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    DUMMY_ASSERT(reader.load(ifilename))
    DUMMY_ASSERT(compare_by_text_section(&reader, &reader2))

    DUMMY_TEST_PASS()
}

/* Модульные тесты */
/* find_mask */
bool test_find_mask_full_finded_in_dictionary()
{
    utils::command cmd;
    cmd.add(0xffff, 16);
    std::vector<utils::command> entab_commands = { cmd };
    utils::encode_table<2, 1> entab(entab_commands);

    dynbitset mask;
    size_t pos, indx;
    DUMMY_ASSERT(!(find_mask<2, 2, 4, 1>(mask, pos, indx, entab, cmd)))

    DUMMY_TEST_PASS()
}

bool test_find_mask_find_single_1_diff()
{
    utils::command dict_cmd;
    dict_cmd.add(0xffff, 16);
    std::vector<utils::command> entab_commands = { dict_cmd };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xafff, 16);
    dynbitset mask;
    size_t pos, indx;
    DUMMY_ASSERT((find_mask<2, 2, 4, 1>(mask, pos, indx, entab, cmd)))

    dynbitset target_mask;
    target_mask.add(0xa, 4);
    DUMMY_ASSERT(mask == target_mask)
    DUMMY_ASSERT(pos == 3)
    DUMMY_ASSERT(indx == 0)

    DUMMY_TEST_PASS()
}

bool test_find_mask_find_single_2_diff()
{
    utils::command dict_cmd;
    dict_cmd.add(0xffff, 16);
    std::vector<utils::command> entab_commands = { dict_cmd };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xffaf, 16);
    dynbitset mask;
    size_t pos, indx;
    DUMMY_ASSERT((find_mask<2, 2, 4, 1>(mask, pos, indx, entab, cmd)))

    dynbitset target_mask;
    target_mask.add(0xa, 4);
    DUMMY_ASSERT(mask == target_mask)
    DUMMY_ASSERT(pos == 1)
    DUMMY_ASSERT(indx == 0)

    DUMMY_TEST_PASS()
}

bool test_find_mask_find_many_diff()
{
    utils::command dict_cmd;
    dict_cmd.add(0xffff, 16);
    std::vector<utils::command> entab_commands = { dict_cmd };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xafaf, 16);
    dynbitset mask;
    size_t pos, indx;
    DUMMY_ASSERT((!find_mask<2, 2, 4, 1>(mask, pos, indx, entab, cmd)))

    DUMMY_TEST_PASS()
}

/* make_encode_table */
bool test_dict_make_encode_table_default()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd1, cmd1, cmd2, cmd3, cmd3, cmd4 };

    config cfg;
    encode_table<2, 1> entab;
    dict_make_encode_table(entab_commands, cfg, entab);

    auto entries = entab.get_entries();
    DUMMY_ASSERT(entries.size() == 2);
    DUMMY_ASSERT(entries[0] == cmd1 || entries[0] == cmd3);
    DUMMY_ASSERT(entries[1] == cmd1 || entries[1] == cmd3);

    DUMMY_TEST_PASS()
}

/* compress */
bool test_compress_command_with_mask_not_compressed()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xaaaa, 16);
    command ccmd = compress_command_with_mask<2, 2, 4, 1>(entab, cmd);
    utils::command target_cmd;
    target_cmd.add(0x15554, 17);
    DUMMY_ASSERT(ccmd == target_cmd)

    DUMMY_TEST_PASS()
}

bool test_compress_command_with_mask_dict_compressed()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xfffd, 16);
    command ccmd = compress_command_with_mask<2, 2, 4, 1>(entab, cmd);
    utils::command target_cmd;
    target_cmd.add(0b111, 3);
    DUMMY_ASSERT(ccmd == target_cmd)

    DUMMY_TEST_PASS()
}

bool test_compress_command_with_mask_mask_compressed()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xffff, 16);
    command ccmd = compress_command_with_mask<2, 2, 4, 1>(entab, cmd);
    utils::command target_cmd;
    target_cmd.add(0b11110001, 8);      // 0 - признак сжатия, 0 - признак маски, 00 - позиция, 1111 - маска
    DUMMY_ASSERT(ccmd == target_cmd)

    DUMMY_TEST_PASS()
}

bool compress_command_with_dictionary_not_compressed()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    utils::command cmd;
    cmd.add(0xffff, 16);
    command ccmd = compress_command_with_dictionary(entab, cmd);
    utils::command target_cmd;
    target_cmd.add(0x1fffe, 17);
    DUMMY_ASSERT(ccmd == target_cmd)

    DUMMY_TEST_PASS()
}

bool compress_command_with_dictionary_compressed()
{
    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    command ccmd = compress_command_with_dictionary(entab, cmd1);
    utils::command target_cmd;
    target_cmd.add(0b11, 2);
    DUMMY_ASSERT(ccmd == target_cmd)

    DUMMY_TEST_PASS()
}

/* decompress */
bool test_restore_block_mask_not_compressed()
{
    utils::compressed_section csec;
    csec.add(0x15554, 17);

    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    size_t pos = 0;
    command cmd = utils::restore_block_mask<2, 2, 4, 1>(csec, pos, entab);

    command target_cmd;
    target_cmd.add(0xaaaa, 16);

    DUMMY_ASSERT(cmd == target_cmd);

    DUMMY_TEST_PASS()
}

bool test_restore_block_mask_dict_compressed()
{
    utils::compressed_section csec;
    csec.add(0b111, 3);

    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    size_t pos = 0;
    command cmd = utils::restore_block_mask<2, 2, 4, 1>(csec, pos, entab);

    command target_cmd;
    target_cmd.add(0xfffd, 16);

    DUMMY_ASSERT(cmd == target_cmd);

    DUMMY_TEST_PASS()
}

bool test_restore_block_mask_mask_compressed()
{
    utils::compressed_section csec;
    csec.add(0b11110001, 8);      // 0 - признак сжатия, 0 - признак маски, 00 - позиция, 1111 - маска

    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    size_t pos = 0;
    command cmd = utils::restore_block_mask<2, 2, 4, 1>(csec, pos, entab);

    command target_cmd;
    target_cmd.add(0xffff, 16);

    DUMMY_ASSERT(cmd == target_cmd);

    DUMMY_TEST_PASS()   
}

bool test_restore_block_dict_not_compressed()
{
    utils::compressed_section csec;
    csec.add(0x1fffe, 17);

    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    size_t pos = 0;
    command cmd = utils::restore_block_dict<2, 1>(csec, pos, entab);

    command target_cmd;
    target_cmd.add(0xffff, 16);

    DUMMY_ASSERT(cmd == target_cmd);

    DUMMY_TEST_PASS()
}

bool test_restore_block_dict_compressed()
{
    utils::compressed_section csec;
    csec.add(0b11, 2);

    utils::command cmd1, cmd2, cmd3, cmd4;
    cmd1.add(0xfffd, 16);
    cmd2.add(0xfffc, 16);
    cmd3.add(0xfffb, 16);
    cmd4.add(0xfffa, 16);

    std::vector<utils::command> entab_commands = { cmd1, cmd3 };
    utils::encode_table<2, 1> entab(entab_commands);

    size_t pos = 0;
    command cmd = utils::restore_block_dict<2, 1>(csec, pos, entab);

    DUMMY_ASSERT(cmd == cmd1);

    DUMMY_TEST_PASS()
}

/* commands */
bool test_rv32i_get_commands()
{
    ELFIO::elfio reader;

    const std::string test_filename = "./tests/hello_world-rv32i.o";
    DUMMY_ASSERT(reader.load(test_filename))

    const ELFIO::section *sec_text = utils::get_section_with_name(&reader, ".text");
    std::vector<utils::command> cmds = utils::get_commands<utils::RV32I_CMDLEN>(sec_text);

    DUMMY_TEST_PASS()
}

bool test_command_operator_eq()
{
    command c1, c2;
    c1.add("abc", 3 * 8);
    c2.add("abc", 3 * 8);
    DUMMY_ASSERT(c1 == c2)

    command c3, c4;
    c3.add("abd", 3 * 8);
    c4.add("abc", 3 * 8);
    DUMMY_ASSERT(c3 != c4)

    DUMMY_TEST_PASS()
}

bool test_command_devide_default()
{
    command cmd1, cmd2;
    cmd1.add(0xaffa, 16);
    cmd2.add(0xfa, 8);

    command target_cmd11, target_cmd12, target_cmd21, target_cmd22;
    target_cmd11.add(0xffa, 12);
    target_cmd12.add(0xa, 4);
    target_cmd21.add(0x2, 2);
    target_cmd22.add(0x3e, 6);

    command cmd11, cmd12;
    cmd1.devide(cmd11, cmd12, 12);
    DUMMY_ASSERT(cmd11 == target_cmd11)
    DUMMY_ASSERT(cmd12 == target_cmd12)

    command cmd21, cmd22;
    cmd2.devide(cmd21, cmd22, 2);
    DUMMY_ASSERT(cmd21 == target_cmd21)
    DUMMY_ASSERT(cmd22 == target_cmd22)

    DUMMY_TEST_PASS()
}

bool test_command_devide_0_full()
{
    command cmd1, cmd2;
    cmd1.add(0xaffa, 16);
    cmd2.add(0xfa, 8);

    command cmd11, cmd12;
    cmd1.devide(cmd11, cmd12, 16);
    DUMMY_ASSERT(cmd11 == cmd1)
    DUMMY_ASSERT(cmd12.get_data_sz_bits() == 0)
    DUMMY_ASSERT(cmd12.get_data_sz() == 0)

    command cmd21, cmd22;
    cmd2.devide(cmd21, cmd22, 8);
    DUMMY_ASSERT(cmd21 == cmd2)
    DUMMY_ASSERT(cmd22.get_data_sz_bits() == 0)
    DUMMY_ASSERT(cmd22.get_data_sz() == 0)

    DUMMY_TEST_PASS()
}

bool test_command_devide_full_0()
{
    command cmd1, cmd2;
    cmd1.add(0xaffa, 16);
    cmd2.add(0xfa, 8);

    command cmd11, cmd12;
    cmd1.devide(cmd11, cmd12, 0);
    DUMMY_ASSERT(cmd11.get_data_sz_bits() == 0)
    DUMMY_ASSERT(cmd11.get_data_sz() == 0)
    DUMMY_ASSERT(cmd12 == cmd1)

    command cmd21, cmd22;
    cmd2.devide(cmd21, cmd22, 0);
    DUMMY_ASSERT(cmd21.get_data_sz_bits() == 0)
    DUMMY_ASSERT(cmd21.get_data_sz() == 0)
    DUMMY_ASSERT(cmd22 == cmd2)

    DUMMY_TEST_PASS()
}

bool test_command_devide_half_default()
{
    command cmd1, cmd2;
    cmd1.add(0xaffa, 16);
    cmd2.add(0xfa, 8);

    command target_cmd11, target_cmd12, target_cmd21, target_cmd22;
    target_cmd11.add(0xfa, 8);
    target_cmd12.add(0xaf, 8);
    target_cmd21.add(0xa, 4);
    target_cmd22.add(0xf, 4);

    command cmd11, cmd12;
    cmd1.devide_half(cmd11, cmd12);
    DUMMY_ASSERT(cmd11 == target_cmd11)
    DUMMY_ASSERT(cmd12 == target_cmd12)

    command cmd21, cmd22;
    cmd2.devide_half(cmd21, cmd22);
    DUMMY_ASSERT(cmd21 == target_cmd21)
    DUMMY_ASSERT(cmd22 == target_cmd22)

    DUMMY_TEST_PASS()
}

/* dynbitset */
bool test_dynbitset_add_bool_default()
{
    dynbitset s;
    s.add(0);

    DUMMY_ASSERT(s.get_data_sz_bits() == 1)
    DUMMY_ASSERT(s.get_data_sz() == 1)
    DUMMY_ASSERT(s.data()[0] == 0)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_add_size_t_default()
{
    dynbitset s;
    s.add(0xa, 4);

    DUMMY_ASSERT(s.get_data_sz_bits() == 4)
    DUMMY_ASSERT(s.get_data_sz() == 1)
    DUMMY_ASSERT(s.data()[0] == 10)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_add_byte_vector_default()
{
    dynbitset s;
    std::vector<char> bytes = { (char)0x5, (char)0xf1 };
    s.add(bytes, 12);

    DUMMY_ASSERT(s.get_data_sz_bits() == 12)
    DUMMY_ASSERT(s.get_data_sz() == 2)
    DUMMY_ASSERT(s.to_size_t() == 261)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_add_byte_array_default()
{
    dynbitset s;
    std::vector<char> bytes = { (char)0xf5, (char)0xf1 };
    s.add(bytes.data(), 12, 0);

    DUMMY_ASSERT(s.get_data_sz_bits() == 12)
    DUMMY_ASSERT(s.get_data_sz() == 2)
    DUMMY_ASSERT(s.to_size_t() == 501)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_add_byte_array_with_offset_default()
{
    dynbitset s;
    std::vector<char> bytes = { (char)0xf5, (char)0xf1 };
    s.add(bytes.data(), 12, 4);     // Пропуск первых четырех бит

    DUMMY_ASSERT(s.get_data_sz_bits() == 12)
    DUMMY_ASSERT(s.get_data_sz() == 2)
    DUMMY_ASSERT(s.to_size_t() == 3871)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_to_size_t_default()
{
    for (size_t i = 0; i < 1024; ++i) {
        dynbitset s;
        s.add(i, 16);

        DUMMY_ASSERT(s.get_data_sz_bits() == 16)
        DUMMY_ASSERT(s.get_data_sz() == 2)
        DUMMY_ASSERT(s.to_size_t() == i)
    }

    DUMMY_TEST_PASS()
}

bool test_dynbitset_lt_operator_default()
{
    dynbitset s1, s2, s3;
    s1.add(0xa, 4);
    s2.add(0xf, 4);
    s3.add(0xffff, 16);

    DUMMY_ASSERT(s1 < s2)
    DUMMY_ASSERT(!(s2 < s1))
    DUMMY_ASSERT(s1 < s3)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_eq_operator_default()
{
    dynbitset s1, s2, s3, s4;
    s1.add(0xa, 4);
    s2.add(0xa, 4);
    s3.add(0xf, 4);
    s4.add(0xffff, 16);

    DUMMY_ASSERT(s1 == s2)
    DUMMY_ASSERT(!(s1 == s3))
    DUMMY_ASSERT(!(s1 == s4))
    DUMMY_ASSERT(!(s1 != s2))
    DUMMY_ASSERT(s1 != s3)
    DUMMY_ASSERT(s1 != s4)

    DUMMY_TEST_PASS()
}

bool test_dynbitset_getseq_default()
{
    dynbitset s1, s2;
    s1.add(0xaffa, 16);
    s2.add(0xfaaf, 16);

    dynbitset target_s1, target_s2;
    target_s1.add(0xff, 8);
    target_s2.add(0xaa, 8);

    DUMMY_ASSERT(target_s1 == s1.getseq(4, 12))
    DUMMY_ASSERT(target_s2 == s2.getseq(4, 12))

    DUMMY_TEST_PASS()
}


bool (*unit_tests[])(void) = {
    test_rv32i_get_commands,
    
    test_command_devide_default,
    test_command_devide_0_full,
    test_command_devide_full_0,
    test_command_devide_half_default,
    test_command_operator_eq,

    test_find_mask_full_finded_in_dictionary,
    test_find_mask_find_single_1_diff,
    test_find_mask_find_single_2_diff,
    test_find_mask_find_many_diff,

    test_restore_block_mask_not_compressed,
    test_restore_block_mask_dict_compressed,
    test_restore_block_mask_mask_compressed,
    test_restore_block_dict_not_compressed,
    test_restore_block_dict_compressed,

    test_compress_command_with_mask_not_compressed,
    test_compress_command_with_mask_dict_compressed,
    test_compress_command_with_mask_mask_compressed,
    compress_command_with_dictionary_not_compressed,
    compress_command_with_dictionary_compressed,

    test_restore_block_dict_compressed,

    test_dynbitset_add_bool_default,
    test_dynbitset_add_size_t_default,
    test_dynbitset_add_byte_vector_default,
    test_dynbitset_add_byte_array_default,
    test_dynbitset_add_byte_array_with_offset_default,
    test_dynbitset_to_size_t_default,
    test_dynbitset_lt_operator_default,
    test_dynbitset_eq_operator_default,
    test_dynbitset_getseq_default
};

bool (*integrational_tests[])(void) = {
    test_mask_duo_compress_executable,
    test_mask_duo_decompress_executable,
    test_dict_compress_decompress_executable,
    test_mask_single_compress_decompress_executable,
    test_mask_duo_compress_decompress_executable,
    test_mask_quad_compress_decompress_executable,
    test_mask_oper_compress_decompress_executable
};

int main(int argc, char *argv[])
{
    std::cout << "Unit Tests starts" << std::endl;

    size_t passed_cnt = 0;
    for (size_t i = 0; i < ARRLEN(unit_tests); ++i)
    {
        passed_cnt += unit_tests[i]();
    }

    if (passed_cnt == ARRLEN(unit_tests))
        std::cout << "unit_tests [" << ARRLEN(unit_tests) << "] " << PASSED << std::endl;
    else
        std::cout << "unit_tests " << FAILED << std::endl;

    passed_cnt = 0;
    for (size_t i = 0; i < ARRLEN(integrational_tests); ++i)
    {
        passed_cnt += integrational_tests[i]();
    }

    if (passed_cnt == ARRLEN(integrational_tests))
        std::cout << "integrational_tests [" << ARRLEN(integrational_tests) << "] " << PASSED << std::endl;
    else
        std::cout << "integrational_tests " << FAILED << std::endl;

    return 0;
}
