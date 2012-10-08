#include "StringHash.h"
#include <assert.h>
#include <string>
#include "boost/crc.hpp"


KEngineCore::StringHash::StringHash(char const * inString, unsigned int inHash)
{
#ifndef NDEBUG
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString, strlen(inString));
	assert(crcCalculator.checksum() == inHash);
	string = inString;
#endif
	hash = inHash;
}

KEngineCore::StringHash::StringHash(char const * inString)
{	
#ifndef NDEBUG
	string = inString;
#endif
	boost::crc_32_type crcCalculator;
	crcCalculator.process_bytes(inString, strlen(inString));
	hash = crcCalculator.checksum();
}