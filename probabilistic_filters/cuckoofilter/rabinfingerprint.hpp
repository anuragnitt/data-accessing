#pragma once

#include <string>

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

        uint64_t operator()(char*) const noexcept;

        uint64_t operator()(const char*) const noexcept;

        uint64_t operator()(const std::string&) const noexcept;

        template <typename _Tp = uint64_t>
        uint64_t operator()(const _Tp) const noexcept;
};

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

uint64_t RabinFingerprint::operator()(char* str) const noexcept {
    return operator()((const char*) str);
}

uint64_t RabinFingerprint::operator()(const char* str) const noexcept {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return fingerprint(str, len);
}

uint64_t RabinFingerprint::operator()(const std::string& str) const noexcept {
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
