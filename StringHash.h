#pragma once
#include <functional>
#include <string_view>

#ifdef NDEBUG   
    #define DELETE_STRINGS
#endif

namespace KEngineCore {
struct StringHash
{
	StringHash();
	StringHash(StringHash const & other);
    StringHash(std::string_view string, unsigned int hash);
    StringHash(std::string_view string, unsigned int hash, int tableIndex);
    StringHash(std::string_view string);
    StringHash(const char* c_string);
    unsigned int hash{ 0 };
#ifndef DELETE_STRINGS
    std::string_view    string;
    int                 tableIndex{ -1 };
#endif
	constexpr inline operator unsigned int(void) const { return hash; }
};

}
#ifndef DELETE_STRINGS
#define HASH(string, hash) (KEngineCore::StringHash(string, hash))
#else
#define HASH(string, hash) (KEngineCore::StringHash(nullptr, hash))
#endif

#define HASH_CONST(string, hash) hash

namespace std
{
    template <>
    struct hash<KEngineCore::StringHash>
    {
        size_t operator()(const KEngineCore::StringHash& s) const
        {
            return s.hash;
        }
    };
}
