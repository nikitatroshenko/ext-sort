#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <queue>

#ifndef DEFAULT_MEMORY_SIZE
#define DEFAULT_MEMORY_SIZE (512)
#endif

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE (DEFAULT_MEMORY_SIZE >> 2)
#endif

#ifndef DEFAULT_BLOCKS_NUMBER
#define DEFAULT_BLOCKS_NUMBER (DEFAULT_MEMORY_SIZE / DEFAULT_BLOCK_SIZE)
#endif

#define DATA_START_OFFSET 8

#define DEFAULT_INPUT_PATTERN ("input.bin")
#define DEFAULT_OUTPUT ("output.bin")

#if _LOCAL_TEST
#define RUN_NAME_PATTERN "/tmp/run.%d.bin"
#else
#define RUN_NAME_PATTERN "run.%d.bin"
#endif

int cmp_ll(const void *l, const void *r)
{
    uint64_t left = *(uint64_t *) l;
    uint64_t right = *(uint64_t *) r;

    return static_cast<int>(left - right); // todo: overflow
}

#ifndef MAX_PATH
#define MAX_PATH 20
#endif

struct run_t {
    FILE *file;
    int id;

    explicit run_t(int id): file(nullptr), id(id) {}

    const char *get_name() {
        static char name[MAX_PATH] {};

        sprintf(name, RUN_NAME_PATTERN, id);
        return name;
    }
};

struct run_pool_t {
    static int id_counter;

    std::queue<run_t *> runs;

    run_pool_t() : runs() {}

    static run_pool_t *of_size(size_t size) {
        auto *pool = new run_pool_t();
        const uint64_t zero = 0;

        for (size_t i = 0; i < size; i++) {
            auto run = new run_t(id_counter++);
            const char *name = run->get_name();
            
            run->file = fopen(name, "wb+");
            setvbuf(run->file, nullptr, _IONBF, 0);
            fwrite(&zero, sizeof zero, 1, run->file);
            fclose(run->file);
            pool->runs.push(run);
        }
        return pool;
    }

    run_t *get() {
        auto run = runs.front();
        run->file = fopen(run->get_name(), "rb+");
        setvbuf(run->file, nullptr, _IONBF, 0);
        fseek(run->file, 0, SEEK_SET);
        runs.pop();
        return run;
    }

    void put(run_t *run) {
        fclose(run->file);
        runs.push(run);
    }

    void release(run_t *run) {
        fclose(run->file);
        // remove(run->name);

        delete run;
    }

    size_t size() const {
        return runs.size();
    }

    ~run_pool_t() {
        while (!runs.empty()) {
            auto *run = runs.front();
            fclose(run->file);
            runs.pop();
            delete run;
        }
    }
};

int run_pool_t::id_counter = 0;

struct merger_t {
    uint64_t *ram;
    size_t block_size;
    size_t ram_size;
    run_pool_t *outs;
    run_pool_t *ins;
    run_pool_t *runs;

    explicit merger_t(size_t ram_size) :
            block_size(ram_size / 4),
            ram_size(ram_size),
            outs(nullptr),
            ins(nullptr),
            runs(nullptr)
    {
        ram = new uint64_t[ram_size];
    }

    void split_into_runs(FILE *in) {
        uint64_t size;

        fread(&size, sizeof size, 1, in);
        uint64_t runs_cnt = (size != 0) ? 1 + ((size - 1) / ram_size) : 0; // ceiling
        runs = run_pool_t::of_size(runs_cnt + 1);

        for (uint64_t i = 0; i < runs_cnt; i++) {
            uint64_t read = ((i + 1) * ram_size <= size) ? ram_size : (size % ram_size);

            fread(ram, sizeof *ram, read, in);
            qsort(ram, read, sizeof *ram, cmp_ll);

            run_t *run = runs->get();
            fwrite(&read, sizeof read, 1, run->file);
            fwrite(ram, sizeof *ram, read, run->file);
            runs->put(run);
        }
    }

    void merge(FILE *left, FILE *right, FILE *result) {
        uint64_t *left_block = ram;
        uint64_t *right_block = ram + block_size;
        uint64_t *result_block = ram + 2 * block_size;
        uint64_t left_size;
        uint64_t right_size;
        uint64_t result_size;

        fread(&left_size, sizeof left_size, 1, left);
        fread(&right_size, sizeof right_size, 1, right);
        result_size = left_size + right_size;
        fwrite(&result_size, sizeof result_size, 1, result);

        uint64_t left_read = 0;
        uint64_t right_read = 0;
        uint64_t i = 0;
        uint64_t j = 0;
        uint64_t k = 0;
        uint64_t *lp = left_block;
        uint64_t *rp = right_block;
        uint64_t *resp = result_block;
        while ((left_read < left_size || i != 0) && (right_read < right_size || j != 0)) {
            if (i == 0) {
                lp = left_block;
                i = (left_size - left_read < block_size) ? (left_size - left_read) : block_size;
                fread(left_block, sizeof *left_block, i, left);
                left_read += i;
            }
            if (j == 0) {
                rp = right_block;
                j = (right_size - right_read < block_size) ? (right_size - right_read) : block_size;
                fread(right_block, sizeof *right_block, j, right);
                right_read += j;
            }
            while ((i != 0) && (j != 0)) {
                if (*lp <= *rp) {
                    i--;
                    *(resp++) = *(lp++);
                } else {
                    j--;
                    *(resp++) = *(rp++);
                }
                k++;
                if (k == block_size * 2) {
                    fwrite(result_block, sizeof *result_block, block_size * 2, result);
                    k = 0;
                    resp = result_block;
                    fflush(result);
                }
            }
        }
        if (k != 0) {
            fwrite(result_block, sizeof *result_block, k, result);
        }
        if (i != 0) {
            fwrite(lp, sizeof *lp, i, result);
        }
        if (j != 0) {
            fwrite(rp, sizeof *rp, j, result);
        }
        fflush(result);
        while (left_read < left_size) {
            i = (left_size - left_read < ram_size) ? (left_size - left_read) : ram_size;
            fread(ram, sizeof *ram, i, left);
            left_read += i;
            fwrite(ram, sizeof *ram, i, result);
            fflush(result);
        }
        while (right_read < right_size) {
            j = (right_size - right_read < ram_size) ? (right_size - right_read) : ram_size;
            fread(ram, sizeof *ram, j, right);
            right_read += j;
            fwrite(ram, sizeof *ram, j, result);
            fflush(result);
        }
    }

    void do_merge_sort(FILE *in, FILE *out) {
        split_into_runs(in);

        run_t *result = runs->get();
        run_t *left;
        run_t *right;

        if (runs->size() == 0) {
            fseek(in, 0, SEEK_SET);
            merge(in, result->file, out);
            return;
        }
        if (runs->size() == 1) {
            left = runs->get();
            merge(left->file, result->file, out); // result is a file with empty sequence;
            runs->release(left);
            runs->release(result);
            return;
        }
        while (runs->size() > 2) {
            left = runs->get();
            right = runs->get();

            merge(left->file, right->file, result->file);
            runs->put(result);
            runs->release(left);
            result = right;
            fseek(result->file, 0, SEEK_SET);
        }
        left = runs->get();
        right = runs->get();
        merge(left->file, right->file, out);
        runs->release(result);
        runs->release(left);
        runs->release(right);
    }

    ~merger_t() {
        delete[] ram;
        delete runs;
    }
};

int main(int argc, char const *argv[])
{
	FILE *in = fopen(DEFAULT_INPUT_PATTERN, "rb");
	FILE *out = fopen(DEFAULT_OUTPUT, "wb");

    merger_t merger = merger_t(DEFAULT_MEMORY_SIZE);

    merger.do_merge_sort(in, out);

	fclose(in);
	fclose(out);
    return 0;
}
