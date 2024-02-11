CC := g++
CCFLAGS := -std=c++17 -Wall -Werror -g3 -ggdb -I lib #-DBENCH_COVERAGE

all : lib bench

lib: lib/libcompress.a

bench : bin/bench.exe

rv32_hello_world: tests/hello_world-rv32i.exe

rv64_hello_world: tests/hello_world-rv64i.exe

tests: lib tests/core_unit_tests.exe
	./tests/core_unit_tests.exe

tests/core_unit_tests.exe : tests/core_unit_tests.o lib/libcompress.a
	$(CC) $< -L./lib -lcompress -o $@

lib/libcompress.a: lib/command.o lib/compressed_section.o lib/config.o lib/dynbitset.o lib/size_stat.o lib/utils.o 
	ar crf $@ $^

bin/bench.exe : bin/bench.o lib/libcompress.a
	$(CC) $< -L./lib -lcompress -o $@

tests/%.o : tests/%.cpp
	$(CC) $(CCFLAGS) -c $< -o $@

lib/%.o : lib/%.cpp
	$(CC) $(CCFLAGS) -c $< -o $@

bin/%.o : bin/%.cpp
	$(CC) $(CCFLAGS) -c $< -o $@


# rv32i pipeline

tests/hello_world-rv32i.exe : tests/hello_world-rv32i.o
	riscv32-unknown-elf-g++ $^ -o $@

tests/hello_world-rv32i.o : tests/hello_world.cpp
	riscv32-unknown-elf-g++ -c $^ -o $@

# rv64i pipeline

tests/hello_world-rv64i.exe : tests/hello_world-rv64i.o
	riscv64-unknown-elf-g++ $^ -o $@

tests/hello_world-rv64i.o : tests/hello_world.cpp
	riscv64-unknown-elf-g++ -march=rv64g -c $^ -o $@

.PHONY : clean
clean : 
	rm -rf *.exe *.o bin/*.exe bin/*.o lib/*.o lib/*.a
