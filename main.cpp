#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <queue>
#include <algorithm>

#ifndef DEFAULT_MEMORY_SIZE
#define DEFAULT_MEMORY_SIZE (4096)
#endif

#ifndef DEFAULT_MERGE_RANK
#define DEFAULT_MERGE_RANK 8
#endif

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE (DEFAULT_MEMORY_SIZE >> 2)
#endif

#ifndef DEFAULT_BLOCKS_NUMBER
#define DEFAULT_BLOCKS_NUMBER (DEFAULT_MEMORY_SIZE / DEFAULT_BLOCK_SIZE)
#endif

#define DATA_START_OFFSET 8

#ifndef DEFAULT_MERGE_RANK
#define DEFAULT_MERGE_RANK 2
#endif

#define DEFAULT_INPUT_PATTERN ("input.bin")
#define DEFAULT_OUTPUT ("output.bin")

#if _LOCAL_TEST
#define RUN_NAME_PATTERN "/tmp/run.%d.bin"
#else
#define RUN_NAME_PATTERN "run.%d.bin"
#endif

int cmp_ll(const void *l, const void *r) {
    uint64_t left = *(uint64_t *) l;
    uint64_t right = *(uint64_t *) r;

    if (left < right) {
        return -1;
    } else if (left > right) {
        return 1;
    } else {
        return 0;
    }
}

#ifndef MAX_PATH
#define MAX_PATH 20
#endif

struct run_t {
    FILE *file;
    int id;

    explicit run_t(int id) : file(nullptr), id(id) {}

    const char *get_name() {
        static char name[MAX_PATH]{};

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
            pool->put(run);
        }
        return pool;
    }

    run_t *get() {
        return get(nullptr, 0, 0);
    }

    run_t *get(void *buf, size_t element_size, size_t elements_cnt) {
        auto run = runs.front();
        run->file = fopen(run->get_name(), "rb+");
        size_t buf_size = element_size * elements_cnt;
        setvbuf(run->file, (char *) buf, buf_size ? _IOFBF : _IONBF, buf_size);
        runs.pop();
        return run;
    }

    void put(run_t *run) {
        fclose(run->file);
        runs.push(run);
    }

    void release(run_t *run) {
        fclose(run->file);
        delete run;
    }

    size_t size() const {
        return runs.size();
    }

    ~run_pool_t() {
        while (!runs.empty()) {
            auto *run = runs.front();
            runs.pop();
            delete run;
        }
    }
};

int run_pool_t::id_counter = 0;

// Brand new bicycle
struct block_t {
    FILE *external_storage;
    uint64_t *internal_storage;
    uint64_t external_size;
    uint64_t internal_capacity;
    uint64_t internal_size;
    uint64_t external_storage_offset;
    uint64_t *internal_storage_ptr;

    block_t(FILE *external_storage, uint64_t *internal_storage, uint64_t external_size, uint64_t internal_capacity,
            uint64_t internal_size, uint64_t external_storage_offset, uint64_t *internal_storage_ptr)
            : external_storage(external_storage), internal_storage(internal_storage), external_size(external_size),
              internal_capacity(internal_capacity), internal_size(internal_size),
              external_storage_offset(external_storage_offset), internal_storage_ptr(internal_storage_ptr) {}


    static block_t *create_input(FILE *external_storage, uint64_t *internal_storage, uint64_t internal_capacity) {
        auto *result = new block_t(external_storage, internal_storage, 0, internal_capacity, 0, 0, internal_storage);
        fread(&result->external_size, sizeof result->external_size, 1, external_storage);
        return result;
    }

    static block_t *create_output(FILE *external_storage,
            uint64_t external_size,
            uint64_t *internal_storage,
            uint64_t internal_capacity) {
        auto *result = new block_t(external_storage, internal_storage, external_size, internal_capacity, 0, 0, internal_storage);
        fwrite(&result->external_size, sizeof result->external_size, 1, external_storage);
        return result;
    }

    bool empty() const {
        return !internal_size;
    }

    bool full() const {
        return internal_size == internal_capacity;
    }

    bool has_external_data() const {
        return external_storage_offset < external_size;
    }

    void flush() {
        fwrite(internal_storage, sizeof *internal_storage, internal_size, external_storage);
        internal_size = 0;
        internal_storage_ptr = internal_storage;
//        fflush(external_storage);
    }

    void move_to(block_t *dest) {
        fwrite(internal_storage_ptr, sizeof *internal_storage_ptr, internal_size, dest->external_storage);
        internal_size = 0;
        internal_storage_ptr = internal_storage + internal_capacity;
    }

    uint64_t next() {
        assert(!empty());
        internal_size--;
        return *(internal_storage_ptr++);
    }

    void push(uint64_t v) {
        assert(!full());
        internal_size++;
        *(internal_storage_ptr++) = v;
    }

    void read_next_block() {
        internal_storage_ptr = internal_storage;
        internal_size = (external_size - external_storage_offset < internal_capacity)
                ? (external_size - external_storage_offset)
                : internal_capacity;
        fread(internal_storage_ptr, sizeof *internal_storage, internal_size, external_storage);
        external_storage_offset += internal_size;
    }

    /**
     * Changes internal buffer size and location. Former buffer contains are lost
     * @param internal_storage  new buffer location
     * @param internal_capacity new buffer size
     */
    void resize_buffer(uint64_t *internal_storage, uint64_t internal_capacity) {
        this->internal_storage = internal_storage;
        this->internal_storage_ptr = internal_storage;
        this->internal_capacity = internal_capacity;
        this->internal_size = 0;
    }
};

struct merger_t {
    uint64_t *ram;
    size_t ram_size;
    run_pool_t *runs;

    explicit merger_t(size_t ram_size) :
            ram_size(ram_size),
            runs(nullptr) {
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

    void merge(FILE *files[], size_t rank, FILE *result) {
        struct input { FILE *file; uint64_t val; uint64_t size; bool read;} *inputs = new input[rank];
        uint64_t ressiz = 0;

        for (size_t i = 0; i < rank; i++) {
            auto &inp = inputs[i];
            inp = {files[i], 0, 0, false};
            fread(&inp.size, sizeof inp.size, 1, inp.file);
            ressiz += inp.size;
        }
        fwrite(&ressiz, sizeof ressiz, 1, result);

        while (true) {
            input *min_elem = nullptr;
            for (size_t i = 0; i < rank; i++) {
                auto &inp = inputs[i];
                if (!inp.read && !inp.size) {
                    continue;
                }
                if (!inp.read) {
                    fread(&inp.val, sizeof inp.val, 1, inp.file);
                    inp.read = true;
                    inp.size--;
                }
                if ((min_elem == nullptr) || (inp.val < min_elem->val)) {
                    min_elem = &inp;
                }
            }
            if (min_elem == nullptr) {
                break;
            }
            fwrite(&min_elem->val, sizeof min_elem->val, 1, result);
            min_elem->read = false;
        }
    }

    void do_merge_sort(FILE *in, FILE *out, size_t rank = DEFAULT_MERGE_RANK) {
        split_into_runs(in);
        size_t block_size = ram_size / 2 / (rank);
        size_t result_block_size = ram_size / 2;
        auto *result_block = ram + rank * block_size;

        run_t *result = runs->get(result_block, sizeof *ram, result_block_size);

        if (runs->size() == 0) {
            // should never happen since N > 1
            fseek(in, 0, SEEK_SET);
            merge(&in, 1, out);
            return;
        }
        FILE **files = new FILE *[rank];
        auto **used_runs = new run_t*[rank];
        while (runs->size() > 1) {
            size_t files_cnt = 0;
            uint64_t *block_start = ram;

            for (; (files_cnt < rank) && (runs->size() != 0); files_cnt++, block_start += block_size) {
                used_runs[files_cnt] = runs->get(block_start, sizeof *block_start, block_size);
                files[files_cnt] = used_runs[files_cnt]->file;
            }

            merge(files, files_cnt, result->file);
            runs->put(result);
            for (size_t i = 1; i < files_cnt; i++) {
                runs->release(used_runs[i]);
            }
            result = used_runs[0];
            freopen(result->get_name(), "rb+", result->file);
            setvbuf(result->file, (char *) block_start, _IOFBF, result_block_size * sizeof *block_start);
        }
        used_runs[0] = runs->get(ram, sizeof *ram, ram_size);
        merge(&used_runs[0]->file, 1, out);
        runs->release(used_runs[0]);
        runs->release(result);

        delete[] used_runs;
        delete[] files;
    }

    ~merger_t() {
        delete[] ram;
        delete runs;
    }
};

int main(int argc, char const *argv[]) {
    FILE *in = fopen(DEFAULT_INPUT_PATTERN, "rb");
    FILE *out = fopen(DEFAULT_OUTPUT, "wb");

    merger_t merger = merger_t(DEFAULT_MEMORY_SIZE);

    merger.do_merge_sort(in, out);

    fclose(in);
    fclose(out);
    return 0;
}