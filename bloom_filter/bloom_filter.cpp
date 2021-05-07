#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <chrono>
#include <fstream>
using namespace std;

class MurMurHash3 {
    private:
        uint32_t murmurhash3(const void* key, const size_t len, const uint32_t seed) {
            const uint8_t* data = (const uint8_t*)key;
            const size_t n_blocks = len / 4;

            uint32_t h = seed;

            const uint32_t c1 = 0xcc9e2d51;
            const uint32_t c2 = 0xcc9e2d51;
            const uint32_t* blocks = (const uint32_t*)(data + n_blocks*4);

            for (int i=-n_blocks; i; i++) {
                uint32_t k = blocks[i];

                k *= c1;
                k = (k << 15) | (k >> (32 - 15));
                k *= c2;

                h ^= k;
                h = (h << 13) | (h >> (32 - 13));
                h = h*5 + 0xe6546b64;
            }

            const uint8_t* tail = (const uint8_t*)(data + n_blocks*4);
            uint32_t k = 0;

            switch(len & 3) {
                case 3:
                    k ^= tail[2] << 16;
                case 2:
                    k ^= tail[1] << 8;
                case 1:
                    k ^= tail[0];
                    k *= c1;
                    k = (k << 15) | (k >> (32 - 15));
                    k *= c2;
                    h ^= k;
            };

            h ^= len;
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;
            return h;
        }

    public:
        MurMurHash3(void) {}

        uint32_t operator()(const void* key, const size_t len, const uint32_t seed = 0) {
            return murmurhash3(key, len, seed);
        }

        uint32_t operator()(const string str, const uint32_t seed = 0) {
            return murmurhash3(str.data(), str.length(), seed);
        }

        template <typename uintX_t = uint32_t>
        uint32_t operator()(uintX_t num, const uint32_t seed = 0) {
            const size_t len = sizeof(decltype(num));
            uint8_t* key = new uint8_t[len];

            for (size_t i=len; i>0; i--) {
                key[i-1] = num & 0xff;
                num >>= 8;
            }

            uint32_t hash = murmurhash3(key, len, seed);
            delete[] key;
            return hash;
        }
};

template <typename keyType, class HashFamily>
class BloomFilter {
    private:
        const uint64_t num_keys;
        const uint64_t size;
        uint64_t hash_count;
        size_t count;
        vector<bool> bits;
        HashFamily hasher;

    public:
        explicit BloomFilter(const uint64_t num_keys, const uint64_t size):
            num_keys(num_keys), size(size), count(0), hasher() {
            hash_count = ceil((size * log(2)) / num_keys);
            bits.assign(size, false);
        }

        void populate(const string filename) {
            fstream file(filename.c_str(), ios_base::in);
            string line;
            while (getline(file, line))
                insert(line);
            file.close();
        }

        void insert(const keyType key) {
            uint32_t _hash;
            for (size_t i=0; i<hash_count; i++) {
                _hash = hasher(key, i) % size;
                bits.at(_hash) = true;
            }
            count++;
        }

        bool lookup(const keyType key) {
            uint32_t _hash;
            for (size_t i=0; i<hash_count; i++) {
                _hash = hasher(key, i) % size;
                if (!bits.at(_hash))
                    return false;
            }
            return true;
        }

        double fp_prob(void) {
            return 1 / pow(2, hash_count);
        }

        double occupancy(void) {
            return (1.0 * count) / num_keys;
        }

        friend void benchmark(BloomFilter<string, MurMurHash3>&, const string&);
};

uint64_t count_lines(const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (!file.good()) {
        file.close();
        throw fstream::failure("failed to open file");
    }

    string line;
    uint64_t count = 0;
    while (getline(file, line))
        count++;
    file.close();
    return count;
}

uint64_t size_by_fp_prob(const uint64_t num_keys, double fp_prob) {
    if (fp_prob <= 0 or fp_prob > 1)
        throw invalid_argument("invalid probability");
    return ceil(-(num_keys / log(2)) * (log2(fp_prob)));
}

void benchmark(BloomFilter<string, MurMurHash3>& bloom, const string& filename) {
    using namespace std::chrono;

    std::cout << "\npopulating the filter ... ";
    auto start = high_resolution_clock::now();
    bloom.populate(filename);
    auto stop = high_resolution_clock::now();
    std::cout << "done\n";

    double time = duration_cast<nanoseconds>(stop - start).count();
    time /= bloom.num_keys;

    std::cout << "\nnumber of keys: " << bloom.num_keys;
    std::cout << "\nsize in bytes: " << bloom.size;
    std::cout << "\naverage size per key in bytes: " << (double)bloom.size / bloom.num_keys;
    std::cout << "\nspace occupancy: " << 100 * bloom.occupancy() << " %";
    std::cout << "\naverage key insertion time: " << time << " nanoseconds\n";
    std::cout << "\nfalse positive probabilty: " << 100 * bloom.fp_prob() << " %";

    string input;
    while (true) {
        std::cout << "\nlookup password: ";
        std::cin.sync();
        getline(std::cin, input);
        if (input == "exit")
            break;

        start = high_resolution_clock::now();
        bool result = bloom.lookup(input);
        stop = high_resolution_clock::now();
        time = duration_cast<nanoseconds>(stop - start).count();

        if (result)
            std::cout << "password is common";
        else
            std::cout << "password is unique";
        std::cout << " (operation time: " << time << " nanoseconds)\n";
        input.clear();
    }
}

int main(void) {
    string filename;
    cout << "enter dictionary path: ";
    cin >> filename;

    try {
        cout << "reading file ...\n";
        uint64_t num_keys = count_lines(filename);
        uint64_t size = size_by_fp_prob(num_keys, 0.05);

        BloomFilter<string, MurMurHash3> bloom(num_keys, size);
        benchmark(bloom, filename);
        return 0;
    }
    catch (const exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}
