#pragma once
#include <functional>

namespace KEngineCore {
struct StringHash
{
	StringHash(char const * string, unsigned int hash);
	StringHash(char const * string);
	unsigned int hash;
#ifndef NDEBUG
	char const * string;
#endif
	inline operator unsigned int(void) const { return hash; }
};

}
#ifndef NDEBUG
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
