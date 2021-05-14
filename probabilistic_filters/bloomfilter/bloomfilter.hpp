#pragma once

#include <cmath>

template <typename _Tp, class HashFamily>
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
    return ((double) count) / n_keys;
}

template <typename _Tp, class HF>
constexpr uint64_t BloomFilter<_Tp, HF>::num_keys(void) const noexcept {
    return count;
}

template <typename _Tp, class HF>
constexpr uint64_t BloomFilter<_Tp, HF>::size_in_bytes(void) const noexcept {
    return size * sizeof(bool);
}
