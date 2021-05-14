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

template <typename _Tp, class HashFamily = MurMurHash3, class FingerprintFamily = RabinFingerprint>
class CuckooFilterLL {
    private:
        uint64_t size;
        const uint32_t threshold;
        const size_t n_buckets;
        uint64_t count;

        uint64_t** table;

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

template <typename _Tp, class HashFamily = MurMurHash3, class FingerprintFamily = RabinFingerprint>
class CuckooFilterHL {
    private:
        const uint64_t size;
        const uint32_t threshold;
        const size_t n_buckets;
        uint64_t count;

        uint64_t** table;

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

template <typename _Tp, class HF, class FF>
CuckooFilterLL<_Tp, HF, FF>::CuckooFilterLL(const uint64_t size, const uint32_t relocation_threshold, const double load_factor)
    : threshold(relocation_threshold), n_buckets(2),
    count(0), hasher(), fingerprint() {
    if (load_factor <= 0 or load_factor > 1) {
        throw std::invalid_argument("invalid load factor");
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
    n_buckets(cf_ll.n_buckets), count(cf_ll.count), hasher(), fingerprint() {
    
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
    count = cf_ll.count;
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
        throw std::overflow_error("relocation threshold reached");
    }

    if (not table[bucket_id][_hash]) {
        table[bucket_id][_hash] = fp;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = _hash ^ (hasher(fp) % size);

        if (not table[id][alt_hash]) {
            table[id][alt_hash] = fp;
        }

        else {
            id = std::rand() % n_buckets;
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
    count++;
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
        count--;
        return true;
    }

    else {
        _hash = _hash ^ (hasher(fp) % size);
        if (table[1][_hash] == fp) {
            table[1][_hash] = 0;
            count--;
            return true;
        }
    }

    return false;
}

template <typename _Tp, class HF, class FF>
constexpr double CuckooFilterLL<_Tp, HF, FF>::load_factor(void) const noexcept {
    return double(count) / size;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterLL<_Tp, HF, FF>::num_keys(void) const noexcept {
    return count;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterLL<_Tp, HF, FF>::size_in_bytes(void) const noexcept {
    return n_buckets * size * sizeof(uint64_t);
}

template <typename _Tp, class HF, class FF>
CuckooFilterHL<_Tp, HF, FF>::CuckooFilterHL(const uint64_t size, const uint32_t relocation_threshold, const size_t buckets)
    : size(size + 7), threshold(relocation_threshold),
    n_buckets(buckets), count(0), hasher(), fingerprint() {

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
    n_buckets(cf_hl.n_buckets), count(cf_hl.count), hasher(), fingerprint() {
    
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
    count = cf_hl.count;
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
        throw std::overflow_error("relocation threshold reached");
    }

    if (not table[bucket_id][_hash]) {
        table[bucket_id][_hash] = fp;
    }

    else {
        size_t id = (bucket_id + 1) % n_buckets;
        uint32_t alt_hash = (_hash ^ hasher(fp)) % size;

        if (not table[id][alt_hash]) {
            table[id][alt_hash] = fp;
        }

        else {
            id = std::rand() % n_buckets;
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
    count++;
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
        count--;
        return true;
    }

    return false;
}

template <typename _Tp, class HF, class FF>
constexpr double CuckooFilterHL<_Tp, HF, FF>::load_factor(void) const noexcept {
    return double(count) / size;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterHL<_Tp, HF, FF>::num_keys(void) const noexcept {
    return count;
}

template <typename _Tp, class HF, class FF>
constexpr uint64_t CuckooFilterHL<_Tp, HF, FF>::size_in_bytes(void) const noexcept {
    return n_buckets * size * sizeof(uint64_t);
}
