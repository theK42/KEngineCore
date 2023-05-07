#include "TextFile.h"
#include <iostream>
#include <fstream>
#include <assert.h>


void KEngineCore::TextFile::LoadFromFile(const std::string_view& filename, const std::string_view& extension)
{
	std::ifstream inFile;
	std::string fullFilename(filename);
	fullFilename = fullFilename.append(extension);
	inFile.open(fullFilename, std::ifstream::in | std::ifstream::binary);
	if (!inFile) {
		std::cerr << "Unable to open file " << filename << extension;
		assert(false);    
		exit(1); // call system to stop
	}

	inFile.seekg(0, std::ios::end);
	mFileContents.resize(inFile.tellg()); //Gets file size.  Tellg
	inFile.seekg(0, std::ios::beg);
	inFile.read(&mFileContents[0], mFileContents.size());
	inFile.close();
}

const std::string& KEngineCore::TextFile::GetContents() const 
{
	return mFileContents;
	
}