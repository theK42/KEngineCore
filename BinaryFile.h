//
//  BinaryFile.h
//  KEngineCore
//
//  Created by Kelson Hootman on 7/14/22.
//

#pragma once
#include <string>
#include <vector>
#include <assert.h>

namespace KEngineCore
{
    class BinaryFile
    {
    public:
        void LoadFromFile(const std::string& filename, const std::string& extension);
        const void * GetContents() const;
        template<class DataType>
        const DataType* GetContents() const;

        size_t GetSize() const;
    private:
        std::vector<char> mFileContents;
    };

    template<class DataType>
    const DataType* KEngineCore::BinaryFile::GetContents() const
    {
        assert(GetSize() >= sizeof(DataType));
        return reinterpret_cast<const DataType*>(&mFileContents[0]);
    }
}
