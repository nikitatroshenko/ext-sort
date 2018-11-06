BLOCK_SIZE=256
MEMO_SIZE=512

CFLAGS=-DDEFAULT_MEMORY_SIZE=$(MEMO_SIZE) -DDEFAULT_BLOCK_SIZE=$(BLOCK_SIZE) -D_LOCAL_TEST

all: executables

test: executables
	./test-case.bash 8
	./test-case.bash 512
	./test-case.bash 0
	./test-case.bash 262144
	./test-case.bash 524288
	./test-case.bash 33333
	./test-case.bash 1280000

executables: test.out test_gen.out ext_sort.out exhaustive_test_gen.out

test_gen.out: test_gen.cpp
	g++ $(CFLAGS) -o test_gen.out test_gen.cpp

test.out: test.cpp
	g++ -o test.out test.cpp

exhaustive_test_gen.out: exhaustive_test_gen.cpp
	g++ -o exhaustive_test_gen.out exhaustive_test_gen.cpp

ext_sort.out: main.cpp
	g++ $(CFLAGS) -o ext_sort.out main.cpp

clean:
	rm -f *.out