#include <iostream>
#include <cassert>

#include "elfio/elfio.hpp"

#include "../lib/utils.h"
#include "../lib/encode_table.h"

using namespace utils;

namespace utils 
{

ELFIO::section *get_section_with_name(const ELFIO::elfio *file, const std::string &name);
std::vector<command> rv32i_get_commands(const ELFIO::section *sec_text);

}

void default_bench()
{
    std::cout << "Bench started" << std::endl;

    std::string ofilename = "result.out";
    std::vector<std::string> filenames = {
        "./rv32i_programms/src/bellman_ford/a.out",
        "./rv32i_programms/src/blur_image/a.out",
        "./rv32i_programms/src/dijkastra/a.out",
        "./rv32i_programms/src/fft/a.out",
        "./rv32i_programms/src/mersenne_twister/a.out",
        "./rv32i_programms/src/negative_image/a.out",
        "./rv32i_programms/src/qsort/a.out",
        "./rv32i_programms/src/rgb_to_gray/a.out",
        "./rv32i_programms/src/sha256/a.out",
    };

    std::vector<encode_type> encode_types = {
        encode_type::DICT,
        encode_type::MASK_SINGLE,
        encode_type::MASK_DUO,
        encode_type::MASK_DUO_QUAD,
        encode_type::MASK_QUAD,
        encode_type::MASK_OPERANDS_OPCODE
    };

    std::cout << "\t\t\t" << "DICT" << "\t" << "MASKS" << "\t" << "MASKD" << "\t" << "MASKDQ" << "\t" << "MASKQ" << "\t" << "MASKOO" << std::endl;

    for (const auto & ifilename : filenames) {

        std::cout << ifilename << "\t";

        for (const auto &entype : encode_types)
        {
            ELFIO::elfio reader;
            if (!reader.load(ifilename))
            {
                std::cout << "Can't find or process ELF file " << ifilename << std::endl;
                assert(false);
            }

            config_builder cfg_builder;
            cfg_builder.set_etype(entype);

            utils::size_stat sz_stat;
            std::vector<std::string> dict_infos;
            config cfg = cfg_builder.build();
            compress_executable(sz_stat, dict_infos, &reader, cfg);

            std::cout << sz_stat.final_code_size + sz_stat.dict_32_bit_size << "(" << sz_stat.initial_code_size << ")" << "\t" << std::flush;

            reader.save( ofilename );
        }

        std::cout << std::endl;
    }

    std::cout << "Bench finished" << std::endl;
}

void nullate_bit7(ELFIO::elfio *f)
{
    ELFIO::section *s = get_section_with_name(f, ".text");
    const char *sd = s->get_data();
    size_t ssz = s->get_size();

    char *data_cpy = new char[ssz];
    memcpy(data_cpy, sd, ssz);
    for (size_t i = 0; i < ssz; i += 4)
    {
        //data_cpy[i] = data_cpy[i] & 0x7f;
        data_cpy[i + 2] = data_cpy[i + 2] & 0xff;
    }

    s->set_data(data_cpy, ssz);
    delete [] data_cpy;
}

void bit7_nullable_bench()
{
    std::cout << "Bench started" << std::endl;

    std::string ofilename = "result.out";
    std::string ifilename = "./rv32i_programms/src/bellman_ford/a.out";

    utils::size_stat sz_stat;
    std::vector<std::string> dict_infos;

    ELFIO::elfio reader;
    if (!reader.load(ifilename))
    {
        std::cout << "Can't find or process ELF file " << ifilename << std::endl;
        assert(false);
    }
    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_DUO);
    config cfg = cfg_builder.build();
    compress_executable(sz_stat, dict_infos, &reader, cfg);

    /*
    std::cout << dict_infos[0] << std::endl;
    std::cout << "===" << std::endl;
    std::cout << dict_infos[1] << std::endl;
    std::cout << "===" << std::endl;
    */

    std::cout << "MASK_DUO: " << sz_stat.final_code_size + sz_stat.dict_32_bit_size << std::endl;

    if (!reader.load(ifilename))
    {
        std::cout << "Can't find or process ELF file " << ifilename << std::endl;
        assert(false);
    }

    nullate_bit7(&reader);
    compress_executable(sz_stat, dict_infos, &reader, cfg);

    /*
    std::cout << dict_infos[0] << std::endl;
    std::cout << "===" << std::endl;
    std::cout << dict_infos[1] << std::endl;
    */

    std::cout << "MASK_DUO*: " << sz_stat.final_code_size + sz_stat.dict_32_bit_size << std::endl;

    if (!reader.load(ifilename))
    {
        std::cout << "Can't find or process ELF file " << ifilename << std::endl;
        assert(false);
    }
    cfg_builder.set_etype(encode_type::MASK_OPERANDS_OPCODE);
    cfg = cfg_builder.build();
    compress_executable(sz_stat, dict_infos, &reader, cfg);
    std::cout << "MASKOO: " << sz_stat.final_code_size + sz_stat.dict_32_bit_size << std::endl;

    if (!reader.load(ifilename))
    {
        std::cout << "Can't find or process ELF file " << ifilename << std::endl;
        assert(false);
    }
    nullate_bit7(&reader);
    compress_executable(sz_stat, dict_infos, &reader, cfg);
    std::cout << "MASKOO*:" << sz_stat.final_code_size + sz_stat.dict_32_bit_size << std::endl;

    std::cout << "Bench finished" << std::endl;
}

void custom_bisect_bench()
{
    std::cout << "Bench started" << std::endl;

    std::string ofilename = "result.out";
    std::string ifilename = "./rv32i_programms/src/bellman_ford/a.out";

    ELFIO::elfio reader;
    if (!reader.load(ifilename))
    {
        std::cout << "Can't find or process ELF file " << ifilename << std::endl;
        assert(false);
    }

    config_builder cfg_builder;
    cfg_builder.set_etype(encode_type::MASK_DUO);

    utils::size_stat sz_stat;
    std::vector<std::string> dict_infos;
    config cfg = cfg_builder.build();
    compress_executable(sz_stat, dict_infos, &reader, cfg);

    std::cout << sz_stat.final_code_size + sz_stat.dict_32_bit_size << "(" << sz_stat.initial_code_size << ")" << "\t" << std::flush;

    std::cout << "Bench finished" << std::endl;
}

int main(int argc, char *argv[])
{
    default_bench();

    //bit7_nullable_bench();

    //custom_bisect_bench();

    return 0;
}
