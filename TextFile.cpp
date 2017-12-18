#include "TextFile.h"
#include <iostream>
#include <fstream>


void KEngineCore::TextFile::LoadFromFile(const std::string& filename, const std::string& extension)
{
	std::ifstream inFile;
	inFile.open(filename + extension, std::ifstream::in | std::ifstream::binary);
	if (!inFile) {
		std::cerr << "Unable to open file " + filename + extension;
		exit(1);   // call system to stop
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