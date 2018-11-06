#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <algorithm>

#define DEFAULT_BLOCK_SIZE (1 << 20)

#define DEFAULT_INPUT_PATTERN ("input.bin")
#define DEFAULT_OUTPUT ("output.bin")

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int main(int argc, char const *argv[])
{
    FILE *input;
    uint64_t size;


    if (argc < 2) {
        fprintf(stderr, "Usage: ./test_gen.out <file_size>\n");
        return EXIT_FAILURE;
    }
    size = strtoull(argv[1], nullptr, 10);
    auto block = new uint64_t[size];
    for (size_t i = 0; i < size; i++) {
        block[i] = i + 1;
    }
    size_t i = 0;
    char name[4096];
    for (size_t j = 0; j < size; j++) {
        for (size_t k = 0; k < size; k++) {

        }
        input = fopen(DEFAULT_INPUT_PATTERN, "wb");
        fwrite(&size, sizeof size, 1, input);
        fwrite(block, sizeof *block, size, input);
        fclose(input);

        system("./ext_sort.out");
        if (system("./test.out")) {
            sprintf(name, "input.%ld.bin", i++);
            fprintf(stderr, "[" ANSI_COLOR_RED " WA " ANSI_COLOR_RESET "] saved %s\n", name);
            rename(DEFAULT_INPUT_PATTERN, name);
        } else {
            fprintf(stderr, "[" ANSI_COLOR_GREEN " OK " ANSI_COLOR_RESET "]\n");
        }
    };
    delete[] block;
    return EXIT_SUCCESS;
}
