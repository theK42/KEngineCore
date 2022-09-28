#include "StringHash.h"
#include "boost/crc.hpp"
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
#endif
	hash = other.hash;
}


KEngineCore::StringHash::StringHash(std::string_view inString, unsigned int inHash)
{
#ifndef DELETE_STRINGS
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString.data(), inString.length());
	assert(crcCalculator.checksum() == inHash);
	string = inString;
#endif
	hash = inHash;
}

KEngineCore::StringHash::StringHash(std::string_view inString, unsigned int inHash, int inTableIndex)
{
#ifndef DELETE_STRINGS
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString.data(), inString.length());
	assert(crcCalculator.checksum() == inHash);
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
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString.data(), inString.length());
	hash = crcCalculator.checksum();
}

KEngineCore::StringHash::StringHash(const char* c_string)
{
	std::string_view inString(c_string);
#ifndef DELETE_STRINGS
	string = inString;
#endif
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString.data(), inString.length());
	hash = crcCalculator.checksum();
}

