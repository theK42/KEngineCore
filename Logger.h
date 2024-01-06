//
//  Logger.h
//  KEngineCore
//
//  Created by Kelson Hootman on 8/21/23.
//

#pragma once

// C++ 20 does not support using the format library in the way I need for this, and several platforms don't have appropriate support for this feature of C++20 anyway, so we'll just switch to using the fmt library for the time being.  C++23 may make it possible to switch to std::format entirely.
#include "fmt/format.h"
#include "Lua.hpp"
#include <vector>
#include <string_view>

namespace KEngineCore {
    class Logger
    {
    public:
        Logger();
        ~Logger();
        
        void Init();
        void Deinit();
        
        void AddLogHandler(std::function<void(std::string_view)> handler);
        void AddErrorHandler(std::function<void(std::string_view)> handler);
        
        void ReplaceLuaPrint(lua_State* luaState);
        
        template<typename... Args>
        void Log(fmt::format_string<Args...> format, Args&&... args) {
            LogInternal(fmt::format(format, std::forward<Args>(args)...));
        }
        
        template<typename... Args>
        void LogError(fmt::format_string<Args...> format, Args&&... args) {
            LogErrorInternal(fmt::format(format, std::forward<Args>(args)...));
        }

    protected:
        
        void LogInternal(std::string_view logString);
        void LogErrorInternal(std::string_view logString);
        
        std::vector<std::function<void(std::string_view)>>  mLogHandlers;
        std::vector<std::function<void(std::string_view)>>  mErrorHandlers;
    };
}
