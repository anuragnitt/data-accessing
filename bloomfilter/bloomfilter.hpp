#pragma once

#include <random>
#include <string>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;

class MurMurHash3 {
    private:
        uint32_t murmurhash3(const void*, const size_t, const uint32_t) const noexcept;

    public:
        MurMurHash3(void);

        ~MurMurHash3(void) = default;

        MurMurHash3(const MurMurHash3&) = default;

        MurMurHash3& operator=(const MurMurHash3&);

        uint32_t operator()(const void*, const size_t, const uint32_t = 0) const noexcept;

        uint32_t operator()(const std::string, const uint32_t = 0) const noexcept;

        template <typename _Tp = uint64_t>
        uint32_t operator()(_Tp, const uint32_t = 0) const noexcept;
};

class RabinFingerprint {
    private:
        uint32_t base;
        uint64_t modulus;

        uint64_t mod_exp(uint32_t, size_t, uint64_t) const noexcept;

        uint64_t fingerprint(const void*, const size_t) const noexcept;

    public:
        RabinFingerprint(void);

        ~RabinFingerprint(void) = default;

        RabinFingerprint(const RabinFingerprint&) = default;

        RabinFingerprint& operator=(const RabinFingerprint&);

        uint64_t operator()(const void*, const size_t) const noexcept;

        uint64_t operator()(const std::string) const noexcept;

        template <typename _Tp = uint64_t>
        uint64_t operator()(const _Tp) const noexcept;
};

template <typename _Tp, class HashFamily = MurMurHash3>
class BloomFilter {
    private:
        const uint64_t n_keys;
        const uint64_t size;
        uint64_t hash_count;
        uint64_t count;

        bool* bits;
        
        HashFamily hasher;

    public:
        explicit BloomFilter(const uint64_t, const uint64_t);

        ~BloomFilter(void);

        BloomFilter(const BloomFilter&);

        BloomFilter& operator=(const BloomFilter&);

        void insert(const _Tp) noexcept;

        bool lookup(const _Tp) const noexcept;

        constexpr double fp_prob(void) const noexcept;

        constexpr double occupancy_ratio(void) const noexcept;

        constexpr uint64_t num_keys(void) const noexcept;

        constexpr uint64_t size_in_bytes(void) const noexcept;
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

uint32_t MurMurHash3::operator()(const std::string str, const uint32_t seed) const noexcept {
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

RabinFingerprint::RabinFingerprint(void)
    : base(0x101), modulus(0xe8d4a51027) {}

RabinFingerprint& RabinFingerprint::operator=(const RabinFingerprint& rf) {
    base = rf.base;
    modulus = rf.modulus;
    return *this;
}

uint64_t RabinFingerprint::mod_exp(uint32_t base, size_t exp, uint64_t modulus) const noexcept {
    uint64_t res = 1;
    base = base % modulus;

    if (not base) {
        return 0;
    }

    while (exp > 0) {
        if (exp bitand 1) {
            res = (res * base) % modulus;
        }

        exp >>= 1;
        base = (base * base) % modulus;
    }

    return res;
}

uint64_t RabinFingerprint::fingerprint(const void* key, const size_t len) const noexcept {
    const uint8_t* data = (const uint8_t*)key;
    uint64_t fp = 0;

    for (size_t i = 0; i < len; i++) {
        fp += (data[i] * mod_exp(base, i, modulus)) % modulus;
        fp = fp % modulus;
    }

    return fp;
}

uint64_t RabinFingerprint::operator()(const void* key, const size_t len) const noexcept {
    return fingerprint(key, len);
}

uint64_t RabinFingerprint::operator()(const std::string str) const noexcept {
    return fingerprint(str.data(), str.length());
}

template <typename _Tp>
uint64_t RabinFingerprint::operator()(const _Tp num) const noexcept {
    const size_t len = sizeof(decltype(num));
    uint8_t* key = new uint8_t[len];

    for (size_t i = len; i > 0; i--) {
        key[i - 1] = num bitand 0xff;
        num >>= 8;
    }

    uint32_t fp = fingerprint(key, len);
    delete[] key;
    return fp;
}

template <typename _Tp, class HF>
BloomFilter<_Tp, HF>::BloomFilter(const uint64_t n_keys, const uint64_t size)
    : n_keys(n_keys), size(size), count(0), hasher() {
    hash_count = ceil((size * log(2)) / n_keys);
            
    bits = new bool[size];
    for (size_t i = 0; i < size; i++) {
        bits[i] = false;
    }
}

template <typename _Tp, class HF>
BloomFilter<_Tp, HF>::~BloomFilter(void) {
    delete[] bits;
}

template <typename _Tp, class HF>
BloomFilter<_Tp, HF>::BloomFilter(const BloomFilter& bloom)
    : n_keys(bloom.n_keys), size(bloom.size),
    hash_count(bloom.hash_count), count(bloom.count), hasher() {
    
    bits = new bool[size];
    for (size_t i = 0; i < size; i++) {
        bits[i] = bloom.bits[i];
    }
}

template <typename _Tp, class HF>
BloomFilter<_Tp, HF>& BloomFilter<_Tp, HF>::operator=(const BloomFilter& bloom) {
    delete[] bits;

    n_keys = bloom.n_keys;
    size = bloom.size;
    hash_count = bloom.hash_count;
    count = bloom.count;
    hasher = bloom.hasher;

    bits = new bool[size];
    for (size_t i = 0; i < size; i++) {
        bits[i] = bloom.bits[i];
    }

    return *this;
}

template <typename _Tp, class HF>
void BloomFilter<_Tp, HF>::insert(const _Tp key) noexcept {
    uint32_t _hash;
    for (size_t i = 0; i < hash_count; i++) {
        _hash = hasher(key, i) % size;
        bits[_hash] = true;
    }

    count++;
}

template <typename _Tp, class HF>
bool BloomFilter<_Tp, HF>::lookup(const _Tp key) const noexcept {
    uint32_t _hash;
    for (size_t i = 0; i < hash_count; i++) {
        _hash = hasher(key, i) % size;

        if (not bits[_hash]) {
            return false;
        }
    }

    return true;
}

template <typename _Tp, class HF>
constexpr double BloomFilter<_Tp, HF>::fp_prob(void) const noexcept {
    double temp = (size * log(2)) / count;
    return pow(2, -temp);
}

template <typename _Tp, class HF>
constexpr double BloomFilter<_Tp, HF>::occupancy_ratio(void) const noexcept {
    return (1.0 * count) / n_keys;
}

template <typename _Tp, class HF>
constexpr uint64_t BloomFilter<_Tp, HF>::num_keys(void) const noexcept {
    return count;
}

template <typename _Tp, class HF>
constexpr uint64_t BloomFilter<_Tp, HF>::size_in_bytes(void) const noexcept {
    return size * sizeof(bool);
}
