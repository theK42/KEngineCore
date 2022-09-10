#pragma once
#include <functional>

#ifdef NDEBUG   
    #define DELETE_STRINGS
#endif

namespace KEngineCore {
struct StringHash
{
	StringHash();
	StringHash(StringHash const & other);
	StringHash(char const * string, unsigned int hash);
	StringHash(char const * string);
    unsigned int hash {0};
#ifndef DELETE_STRINGS
    char const * string {nullptr};
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
