//
//  TextFile.h
//  KEngineCore
//
//  Created by Kelson Hootman on 11/26/17.
//

#pragma once
#include <string>

namespace KEngineCore
{
    class TextFile
    {
    public:
        void LoadFromFile(const std::string_view& filename, const std::string_view& extension);
        const std::string& GetContents() const;
    private:
        std::string mFileContents;
    };
}
