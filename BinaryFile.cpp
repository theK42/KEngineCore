#include "BinaryFile.h"
#include <iostream>
#include <fstream>

void KEngineCore::BinaryFile::LoadFromFile(const std::string& filename, const std::string& extension)
{
    std::ifstream file(filename + extension, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    mFileContents.resize(fileSize);
    file.seekg(0);
    file.read(mFileContents.data(), fileSize);

    file.close();
}

const void* KEngineCore::BinaryFile::GetContents() const
{
    return (void *)&mFileContents[0];
}

size_t KEngineCore::BinaryFile::GetSize() const
{
    return mFileContents.size();
}

