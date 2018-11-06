#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <random>

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE (1 << 20)
#endif

#define DEFAULT_INPUT_PATTERN ("input.bin")
#define DEFAULT_OUTPUT ("output.bin")

int main(int argc, char const *argv[])
{
    FILE *input = fopen(DEFAULT_INPUT_PATTERN, "wb");
    uint64_t size;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> uid;


    if (argc < 2) {
        fprintf(stderr, "Usage: ./test_gen.out <file_size>\n");
        return EXIT_FAILURE;
    }

    size = strtoull(argv[1], nullptr, 10);
    fwrite(&size, sizeof size, 1, input);
    auto *block = new uint64_t[DEFAULT_BLOCK_SIZE];
    uid = std::uniform_int_distribution<uint64_t>(1, size);
    for (uint64_t i = 0; i < size;) {
        uint64_t written = (size - i < DEFAULT_BLOCK_SIZE) ? (size - i) : DEFAULT_BLOCK_SIZE;

        for (uint64_t j = 0; j < written; j++) {
            block[j] = written - j;
//            block[j] = uid(mt);
        }
        fwrite(block, sizeof *block, written, input);
        i += written;
    }

    fclose(input);
    delete[] block;
    return EXIT_SUCCESS;
}
