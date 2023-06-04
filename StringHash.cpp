#include "StringHash.h"
#include <assert.h>
#include <string>
#include <cstring>

KEngineCore::StringHash::StringHash()
{
	hash = 0;
}


KEngineCore::StringHash::StringHash(StringHash const & other)
{
#ifndef DELETE_STRINGS
	string = other.string;
	tableIndex = other.tableIndex;
#endif
	hash = other.hash;
}


KEngineCore::StringHash::StringHash(std::string_view inString, unsigned int inHash)
{
#ifndef DELETE_STRINGS
	assert(crcdetail::compute(inString.data(), inString.length()) == inHash);
	string = inString;
#endif
	hash = inHash;
}

KEngineCore::StringHash::StringHash(std::string_view inString, unsigned int inHash, int inTableIndex)
{
#ifndef DELETE_STRINGS
	assert(crcdetail::compute(inString.data(), inString.length()) == inHash);
	string = inString;
	tableIndex = inTableIndex;
#endif
	hash = inHash;
}

KEngineCore::StringHash::StringHash(std::string_view inString)
{	
#ifndef DELETE_STRINGS
	string = inString;
#endif
	hash = crcdetail::compute(inString.data(), inString.length());
}

KEngineCore::StringHash::StringHash(const char* c_string)
{
	std::string_view inString(c_string);
#ifndef DELETE_STRINGS
	string = inString;
#endif
	hash = crcdetail::compute(inString.data(), inString.length());
}

