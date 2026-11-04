#pragma once
#include <cstdint>
#include <string>

namespace SE
{

// values for string hashing FNV-1a for 64 bit values
// hashing suggested by chatgpt
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
constexpr uint64_t FNVPrime = 0x00000100000001b3;
constexpr uint64_t FNVOffset = 0xcbf29ce484222325;

inline uint64_t FNV_1A(const std::string& string)
{
    uint64_t hash = FNVOffset;
    for (const auto& c : string)
    {
        hash ^= c;
        hash *= FNVPrime;
    }
    return hash;
}

constexpr inline uint64_t FNV_1A(const std::string_view& string)
{
    uint64_t hash = FNVOffset;
    for (const auto& c : string)
    {
        hash ^= c;
        hash *= FNVPrime;
    }
    return hash;
}

constexpr inline uint64_t FNV_1A(const char* str)
{
    uint64_t hash = FNVOffset;
    while (*str)
    {
        hash ^= *str;
        hash *= FNVPrime;
        str++;
    }
    return hash;
}

class Hash
{
public:
    Hash() = default;
    constexpr Hash(const char* str) : m_hash(FNV_1A(str)) {};

    bool operator==(const Hash& other) const { return m_hash == other.m_hash; }
    bool operator!=(const Hash& other) const { return m_hash != other.m_hash; }

    explicit operator uint64_t() const { return m_hash; }

    uint64_t Value() const { return m_hash; }

private:
    uint64_t m_hash = 0;
};

class HasString
{
public:
    explicit HasString() = default;
    explicit HasString(const std::string& string) : m_hash(string.c_str()), m_string(string) {};

    // conversion to uint64 for simplicity
    explicit operator uint64_t() const { return m_hash.Value(); }

    uint64_t GetHash() const { return m_hash.Value(); }

    bool operator==(const HasString& other) const { return m_hash == other.m_hash; }
    bool operator!=(const HasString& other) const { return m_hash != other.m_hash; }

    bool IsValid() const { return m_hash != 0; }

    std::string GetName() const { return m_string; }

private:
    Hash m_hash;
    std::string m_string;
};

inline void SumHash(uint64_t& in, uint64_t value) { in ^= value; }
inline void SumHash(uint64_t& in, uint32_t value) { in ^= static_cast<uint64_t>(value); }
inline void SumHash(uint64_t& in, const std::string& value) { in ^= FNV_1A(value); }
inline void SumHash(uint32_t& in, uint32_t value) { in ^= value; }

}