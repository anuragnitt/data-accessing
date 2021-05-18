#pragma once

#include <string>

class MurMurHash3 {
    private:
        uint32_t murmurhash3(const void*, const size_t, const uint32_t) const noexcept;

    public:
        MurMurHash3(void);

        ~MurMurHash3(void) = default;

        MurMurHash3(const MurMurHash3&) = default;

        MurMurHash3& operator=(const MurMurHash3&);

        uint32_t operator()(const void*, const size_t, const uint32_t = 0) const noexcept;

        uint32_t operator()(char*, const uint32_t = 0) const noexcept;

        uint32_t operator()(const char*, const uint32_t = 0) const noexcept;

        uint32_t operator()(const std::string&, const uint32_t = 0) const noexcept;

        template <typename _Tp = uint64_t>
        uint32_t operator()(_Tp, const uint32_t = 0) const noexcept;
};

MurMurHash3::MurMurHash3(void) {}

MurMurHash3& MurMurHash3::operator=(const MurMurHash3& mmh3) {
    return *this;
}

uint32_t MurMurHash3::murmurhash3(const void* key, const size_t len, const uint32_t seed) const noexcept {
    const uint8_t* data = (const uint8_t*)key;
    const size_t n_blocks = len / 4;

    uint32_t h = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0xcc9e2d51;
    const uint32_t* blocks = (const uint32_t*)(data + n_blocks*4);

    for (int i = -n_blocks; i; i++) {
        uint32_t k = blocks[i];

        k *= c1;
        k = (k << 15) bitor (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) bitor (h >> (32 - 13));
        h = h * 5 + 0xe6546b64;
    }

    const uint8_t* tail = (const uint8_t*)(data + n_blocks*4);
    uint32_t k = 0;

    switch(len bitand 3) {
        case 3:
            k ^= tail[2] << 16;
        case 2:
            k ^= tail[1] << 8;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 15) bitor (k >> (32 - 15));
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

uint32_t MurMurHash3::operator()(const void* key, const size_t len, const uint32_t seed) const noexcept {
    return murmurhash3(key, len, seed);
}

uint32_t MurMurHash3::operator()(char* str, const uint32_t seed) const noexcept {
    return operator()((const char*) str, seed);
}

uint32_t MurMurHash3::operator()(const char* str, const uint32_t seed) const noexcept {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return murmurhash3(str, len, seed);
}

uint32_t MurMurHash3::operator()(const std::string& str, const uint32_t seed) const noexcept {
    return murmurhash3(str.data(), str.length(), seed);
}

template <typename _Tp>
uint32_t MurMurHash3::operator()(_Tp num, const uint32_t seed) const noexcept {
    const size_t len = sizeof(decltype(num));
    uint8_t* key = new uint8_t[len];

    for (size_t i = len; i > 0; i--) {
        key[i - 1] = num bitand 0xff;
        num >>= 8;
    }

    uint32_t hash = murmurhash3(key, len, seed);
    delete[] key;
    return hash;
}
