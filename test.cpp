#include <cstdlib>
#include <cstdio>
#include <cstdint>

#define DEFAULT_BLOCK_SIZE (1 << 18)

#define DEFAULT_INPUT_PATTERN ("input.bin")
#define DEFAULT_OUTPUT ("output.bin")

int main(int argc, char const *argv[])
{
    FILE *output = fopen(DEFAULT_OUTPUT, "rb");
    FILE *input = fopen(DEFAULT_INPUT_PATTERN, "rb");
    auto *block = new uint64_t[DEFAULT_BLOCK_SIZE];
    uint64_t size;
    uint64_t expected_size;
    bool sorted = true;

    fread(&size, sizeof size, 1, output);
    fread(&expected_size, sizeof expected_size, 1, input);
    if (size != expected_size) {
        return EXIT_FAILURE;
    }
    size_t i;
    for (i = 0; i < size;) {
        size_t read = (size - i < DEFAULT_BLOCK_SIZE) ? (size - i) : DEFAULT_BLOCK_SIZE;
        if (!fread(block, sizeof *block, read, output)) {
            sorted = false;
            break;
        }
        i += read;
        for (size_t j = 0; j < read - 1; j++) {
            if (block[j] > block[j + 1]) {
                sorted = false;
                break;
            }
        }
        if (!sorted) {
            break;
        }
    }
    if (i != size) {
        return EXIT_FAILURE;
    }
    
    fclose(output);
    delete[] block;
    return sorted ? EXIT_SUCCESS : EXIT_FAILURE;
}
