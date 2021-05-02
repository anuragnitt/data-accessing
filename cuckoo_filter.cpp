#include <iostream>
#include <vector>
#include <string>
#include <random>
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

class RabinKarp {
    private:
        uint32_t rand_1;
        uint32_t rand_2;

        uint32_t rk_fingerprint(const void* key, const size_t len, const uint32_t seed_1, const uint32_t seed_2) {
            const uint8_t* data = (const uint8_t*)key;
            uint32_t p = 2*len + seed_1;
            uint32_t x = seed_2 % p;
            uint32_t fp = 0;
            
            for (size_t i=0; i<len; i++) {
                fp += data[i]*(uint32_t)pow(x, i);
                fp %= p;
            }
            return fp;
        }

    public:
        RabinKarp(void) {
            random_device random;
            rand_1 = random() & 0xffffffff;
            rand_2 = random() & 0xffffffff;
        }

        uint32_t operator()(const void* key, const size_t len) {
            return rk_fingerprint(key, len, rand_1, rand_2);
        }

        uint32_t operator()(const string str) {
            return rk_fingerprint(str.data(), str.length(), rand_1, rand_2);
        }

        template <typename uintX_t = uint32_t>
        uint32_t operator()(const uintX_t num) {
            const size_t len = sizeof(decltype(num));
            uint8_t* key = new uint8_t[len];

            for (size_t i=len; i>0; i--) {
                key[i-1] = num & 0xff;
                num >>= 8;
            }

            uint32_t fp = rk_fingerprint(key, len, rand_1, rand_2);
            delete[] key;
            return fp;
        }
};

template <typename keyType, class HashFamily, class FingerprintFamily>
class CuckooFilter {
    private:
        uint32_t size;
        size_t threshold;
        vector<vector<uint32_t>> table;
        HashFamily hasher;
        FingerprintFamily fingerprint;

        void insert_util(uint32_t fp, uint32_t _hash, size_t bucket_id, size_t count) {
            if (count >= threshold)
                throw overflow_error("relocation threshold reached");

            if (!table.at(bucket_id).at(_hash))
                table.at(bucket_id).at(_hash) = fp;
            else {
                size_t id = (bucket_id + 1) % table.size();
                uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

                if (!table.at(id).at(alt_hash))
                    table.at(id).at(alt_hash) = fp;
                else {
                    id = rand() % table.size();
                    uint32_t target_hash = (id == 0) ? _hash : alt_hash;
                    uint32_t relocate_fp = table.at(id).at(target_hash);

                    table.at(id).at(target_hash) = fp;
                    target_hash = (target_hash ^ hasher(relocate_fp)) % size;
                    insert_util(relocate_fp, target_hash, id, count + 1);
                }
            }
        }

        bool lookup_util(uint32_t fp, uint32_t _hash, size_t bucket_id, size_t count) {
            if (count >= threshold)
                return false;

            if (table.at(bucket_id).at(_hash) == fp)
                return true;
            else {
                size_t id = (bucket_id + 1) % table.size();
                uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

                if (table.at(id).at(alt_hash) == fp)
                    return true;
                else {
                    id = (bucket_id + 1) % table.size();
                    alt_hash = (alt_hash ^ hasher(fp)) % size;
                    return lookup_util(fp, alt_hash, id, count + 1);
                }
            }
        }

        bool remove_util(uint32_t fp, uint32_t _hash, size_t bucket_id, size_t count) {
            if (count >= threshold)
                return false;

            if (table.at(bucket_id).at(_hash) == fp) {
                table.at(bucket_id).at(_hash) = 0;
                return true;
            }
            else {
                size_t id = (bucket_id + 1) % table.size();
                uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

                if (table.at(id).at(alt_hash) == fp) {
                    table.at(id).at(alt_hash) = 0;
                    return true;
                }
                else {
                    id = (bucket_id + 1) % table.size();
                    alt_hash = (alt_hash ^ hasher(fp)) % size;
                    return remove_util(fp, alt_hash, id, count + 1);
                }
            }
        }

    public:
        explicit CuckooFilter(uint32_t size, size_t relocation_threshold):
            size(size), threshold(relocation_threshold), hasher(), fingerprint() {
            vector<uint32_t> array(size + 7, 0);
            table.push_back(array);
            table.push_back(array);
        }

        void populate(const string filename) {
            fstream file(filename.c_str(), ios_base::in);
            string line;
            while (getline(file, line)) {
                try {
                    insert(line);
                }
                catch (const overflow_error& exc) {
                    continue;
                }
            }
            file.close();
        }

        void insert(const keyType key) {
            uint32_t fp = fingerprint(key);
            uint32_t _hash = hasher(key) % size;
            insert_util(fp, _hash, 0, 0);
        }

        bool lookup(const keyType key) {
            uint32_t fp = fingerprint(key);
            uint32_t _hash = hasher(key) % size;
            return lookup_util(fp, _hash, 0, 0);
        }

        bool remove(const keyType key) {
            uint32_t fp = fingerprint(key);
            uint32_t _hash = hasher(key) % size;
            return remove_util(fp, _hash, 0, 0);
        }
};

uint64_t count_lines(const string& filename) {
    fstream file(filename.c_str(), ios_base::in);
    if (!file.good()) {
        file.close();
        throw runtime_error("failed to open file");
    }

    string line;
    uint64_t count = 0;
    while (getline(file, line))
        count++;
    file.close();
    return count;
}

int main(void) {
    string filename;
    cout << "enter dictionary path: ";
    cin >> filename;

    try {
        cout << "reading file ...\n";
        uint64_t num_keys = count_lines(filename);
        CuckooFilter<string, MurMurHash3, RabinKarp> cuckoo(num_keys, 500);

        cout << "populating the filter ...\n";
        cuckoo.populate(filename);
        cout << "done\n";

        string input;
        while (true) {
            cout << "\nlookup password: ";
            cin.sync();
            getline(cin, input);
            if (input == "exit")
                break;
            if (cuckoo.lookup(input))
                cout << "password is common\n";
            else
                cout << "password is unique\n";
            input.clear();
        }
        return 0;
    }
    catch (const runtime_error& exc) {
        cerr << exc.what() << endl;
        return 1;
    }
}
