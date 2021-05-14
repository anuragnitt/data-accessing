#pragma once

#include <random>

template <typename _Tp, class HashFamily, class FingerprintFamily>
class CuckooFilterLL {
    private:
        uint64_t size;
        const uint32_t threshold;
        const size_t n_buckets;
        uint64_t key_count;

        uint64_t** table;

        std::random_device prng;

        const HashFamily hasher;
        const FingerprintFamily fingerprint;

        void insert_util(uint64_t, uint32_t, size_t, size_t);

    public:
        explicit CuckooFilterLL(const uint64_t, const uint32_t, const double = 0.25);

        CuckooFilterLL(const CuckooFilterLL&);

        CuckooFilterLL& operator=(const CuckooFilterLL&);

        ~CuckooFilterLL(void);

        void insert(const _Tp);

        bool lookup(const _Tp) const noexcept;

        bool remove(const _Tp) noexcept;

        constexpr double load_factor(void) const noexcept;

        constexpr uint64_t num_keys(void) const noexcept;
        
        constexpr uint64_t size_in_bytes(void) const noexcept;
};

template <typename _Tp, class HashFamily, class FingerprintFamily>
class CuckooFilterHL {
    private:
        const uint64_t size;
        const uint32_t threshold;
        const size_t n_buckets;
        uint64_t key_count;

        uint64_t** table;

        std::random_device prng;

        const HashFamily hasher;
        const FingerprintFamily fingerprint;

        void insert_util(uint64_t, uint32_t, size_t, size_t);

        bool lookup_util(uint64_t, uint32_t, size_t, size_t) const noexcept;

        bool remove_util(uint64_t, uint32_t, size_t, size_t) noexcept;

    public:
        explicit CuckooFilterHL(const uint64_t, const uint32_t, const size_t = 2);

        CuckooFilterHL(const CuckooFilterHL&);

        CuckooFilterHL& operator=(const CuckooFilterHL&);

        ~CuckooFilterHL(void);

        void insert(const _Tp);

        bool lookup(const _Tp) const noexcept;

        bool remove(const _Tp) noexcept;

        constexpr double load_factor(void) const noexcept;

        constexpr uint64_t num_keys(void) const noexcept;
        
        constexpr uint64_t size_in_bytes(void) const noexcept;
};

template <typename _Tp, class HF, class FF>
CuckooFilterLL<_Tp, HF, FF>::CuckooFilterLL(const uint64_t size, const uint32_t relocation_threshold, const double load_factor)
    : threshold(relocation_threshold), n_buckets(2),
    key_count(0), hasher(), fingerprint() {
    if (load_factor <= 0 or load_factor > 1) {
        std::string exc_msg = "invalid load factor: " + std::to_string(load_factor);
        throw std::invalid_argument(exc_msg.c_str());
    }
    this->size = ceil(size / load_factor);

    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[this->size];
    }
}

template <typename _Tp, class HF, class FF>
CuckooFilterLL<_Tp, HF, FF>::~CuckooFilterLL(void) {
    for (size_t i = 0; i < n_buckets; i++) {
        delete[] table[i];
    }
    delete[] table;
}

template <typename _Tp, class HF, class FF>
CuckooFilterLL<_Tp, HF, FF>::CuckooFilterLL(const CuckooFilterLL& cf_ll)
    : size(cf_ll.size), threshold(cf_ll.threshold),
    n_buckets(cf_ll.n_buckets), key_count(cf_ll.key_count), hasher(), fingerprint() {
    
    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[size];

        for (size_t j = 0; j < size; j++) {
            table[i][j] = cf_ll.table[i][j];
        }
    }
}

template <typename _Tp, class HF, class FF>
CuckooFilterLL<_Tp, HF, FF>& CuckooFilterLL<_Tp, HF, FF>::operator=(const CuckooFilterLL& cf_ll) {
    for (size_t i = 0; i < n_buckets; i++) {
        delete[] table[i];
    }
    delete[] table;

    size = cf_ll.size;
    threshold = cf_ll.threshold;
    n_buckets = cf_ll.n_buckets;
    key_count = cf_ll.key_count;
    hasher = cf_ll.hasher;
    fingerprint = cf_ll.fingerprint;

    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[size];

        for (size_t j = 0; j < size; j++) {
            table[i][j] = cf_ll.table[i][j];
        }
    }

    return *this;
}

template <typename _Tp, class HF, class FF>
void CuckooFilterLL<_Tp, HF, FF>::insert_util(uint64_t fp, uint32_t _hash, size_t bucket_id, size_t count) {
    if (count == threshold) {
        std::string exc_msg = "relocation threshold reached: " + std::to_string(threshold);
        throw std::overflow_error(exc_msg.c_str());
    }

    else if (_hash >= size) {
        std::string exc_msg = "hash: " + std::to_string(_hash);
        exc_msg += " is out of bucket range: " + std::to_string(size - 1);
        throw std::out_of_range(exc_msg.c_str());
    }

    else if (not table[bucket_id][_hash]) {
        table[bucket_id][_hash] = fp;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = _hash ^ (hasher(fp) % size);

        if (not table[id][alt_hash]) {
            table[id][alt_hash] = fp;
        }

        else {
            id = prng() % n_buckets;
            uint32_t target_hash = (id == 0) ? _hash : alt_hash;
            uint32_t relocate_fp = table[id][target_hash];

            table[id][target_hash] = fp;
            target_hash = target_hash ^ (hasher(relocate_fp) % size);
            insert_util(relocate_fp, target_hash, id, count + 1);
        }
    }
}

template <typename _Tp, class HF, class FF>
void CuckooFilterLL<_Tp, HF, FF>::insert(const _Tp key) {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;
    insert_util(fp, _hash, 0, 0);
    key_count++;
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterLL<_Tp, HF, FF>::lookup(const _Tp key) const noexcept {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;

    if (table[0][_hash] == fp) {
        return true;
    }
    
    else {
        _hash = _hash ^ (hasher(fp) % size);
        if (table[1][_hash] == fp) {
            return true;
        }
    }

    return false;
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterLL<_Tp, HF, FF>::remove(const _Tp key) noexcept {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;
            
    if (table[0][_hash] == fp) {
        table[0][_hash] = 0;
        key_count--;
        return true;
    }

    else {
        _hash = _hash ^ (hasher(fp) % size);
        if (table[1][_hash] == fp) {
            table[1][_hash] = 0;
            key_count--;
            return true;
        }
    }

    return false;
}

template <typename _Tp, class HF, class FF>
constexpr double CuckooFilterLL<_Tp, HF, FF>::load_factor(void) const noexcept {
    return double(key_count) / size;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterLL<_Tp, HF, FF>::num_keys(void) const noexcept {
    return key_count;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterLL<_Tp, HF, FF>::size_in_bytes(void) const noexcept {
    return n_buckets * size * sizeof(uint64_t);
}

template <typename _Tp, class HF, class FF>
CuckooFilterHL<_Tp, HF, FF>::CuckooFilterHL(const uint64_t size, const uint32_t relocation_threshold, const size_t buckets)
    : size(size + 7), threshold(relocation_threshold),
    n_buckets(buckets), key_count(0), hasher(), fingerprint() {

    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[this->size];
    }
}

template <typename _Tp, class HF, class FF>
CuckooFilterHL<_Tp, HF, FF>::~CuckooFilterHL(void) {
    for (size_t i = 0; i < n_buckets; i++) {
        delete[] table[i];
    }
    delete[] table;
}

template <typename _Tp, class HF, class FF>
CuckooFilterHL<_Tp, HF, FF>::CuckooFilterHL(const CuckooFilterHL& cf_hl)
    : size(cf_hl.size), threshold(cf_hl.threshold),
    n_buckets(cf_hl.n_buckets), key_count(cf_hl.key_count), hasher(), fingerprint() {
    
    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[size];

        for (size_t j = 0; j < size; j++) {
            table[i][j] = cf_hl.table[i][j];
        }
    }
}

template <typename _Tp, class HF, class FF>
CuckooFilterHL<_Tp, HF, FF>& CuckooFilterHL<_Tp, HF, FF>::operator=(const CuckooFilterHL& cf_hl) {
    for (size_t i = 0; i < n_buckets; i++) {
        delete[] table[i];
    }
    delete[] table;

    size = cf_hl.size;
    threshold = cf_hl.threshold;
    n_buckets = cf_hl.n_buckets;
    key_count = cf_hl.key_count;
    hasher = cf_hl.hasher;
    fingerprint = cf_hl.fingerprint;

    table = new uint64_t*[n_buckets];
    for (size_t i = 0; i < n_buckets; i++) {
        table[i] = new uint64_t[size];

        for (size_t j = 0; j < size; j++) {
            table[i][j] = cf_hl.table[i][j];
        }
    }

    return *this;
}

template <typename _Tp, class HF, class FF>
void CuckooFilterHL<_Tp, HF, FF>::insert_util(uint64_t fp, uint32_t _hash, size_t bucket_id, size_t count) {
    if (count == threshold) {
        std::string exc_msg = "relocation threshold reached: " + std::to_string(threshold);
        throw std::overflow_error(exc_msg.c_str());
    }

    else if (not table[bucket_id][_hash]) {
        table[bucket_id][_hash] = fp;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

        if (not table[id][alt_hash]) {
            table[id][alt_hash] = fp;
        }

        else {
            id = prng() % n_buckets;
            uint32_t target_hash = (id == 0) ? _hash : alt_hash;
            uint32_t relocate_fp = table[id][target_hash];

            table[id][target_hash] = fp;
            target_hash = (target_hash ^ hasher(relocate_fp)) % size;
            insert_util(relocate_fp, target_hash, id, count + 1);
        }
    }
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterHL<_Tp, HF, FF>::lookup_util(uint64_t fp, uint32_t _hash, size_t bucket_id, size_t count) const noexcept {
    if (count == threshold) {
        return false;
    }

    else if (table[bucket_id][_hash] == fp) {
        return true;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

        if (table[id][alt_hash] == fp) {
            return true;
        }

        else {
            id = (bucket_id + 1) % n_buckets;
            alt_hash = (alt_hash ^ hasher(fp)) % size;
            return lookup_util(fp, alt_hash, id, count + 1);
        }
    }
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterHL<_Tp, HF, FF>::remove_util(uint64_t fp, uint32_t _hash, size_t bucket_id, size_t count) noexcept {
    if (count == threshold) {
        return false;
    }

    else if (table[bucket_id][_hash] == fp) {
        table[bucket_id][_hash] = 0;
        return true;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

        if (table[id][alt_hash] == fp) {
            table[id][alt_hash] = 0;
            return true;
        }

        else {
            id = (bucket_id + 1) % n_buckets;
            alt_hash = (alt_hash ^ hasher(fp)) % size;
            return remove_util(fp, alt_hash, id, count + 1);
        }
    }
}

template <typename _Tp, class HF, class FF>
void CuckooFilterHL<_Tp, HF, FF>::insert(const _Tp key) {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;
    insert_util(fp, _hash, 0, 0);
    key_count++;
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterHL<_Tp, HF, FF>::lookup(const _Tp key) const noexcept {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;
    return lookup_util(fp, _hash, 0, 0);
}

template <typename _Tp, class HF, class FF>
bool CuckooFilterHL<_Tp, HF, FF>::remove(const _Tp key) noexcept {
    uint64_t fp = fingerprint(key);
    uint32_t _hash = hasher(key) % size;

    if (remove_util(fp, _hash, 0, 0)) {
        key_count--;
        return true;
    }

    return false;
}

template <typename _Tp, class HF, class FF>
constexpr double CuckooFilterHL<_Tp, HF, FF>::load_factor(void) const noexcept {
    return double(key_count) / size;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterHL<_Tp, HF, FF>::num_keys(void) const noexcept {
    return key_count;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterHL<_Tp, HF, FF>::size_in_bytes(void) const noexcept {
    return n_buckets * size * sizeof(uint64_t);
}
