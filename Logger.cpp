//
//  Logger.cpp
//  KEngineCore
//
//  Created by Kelson Hootman on 8/21/23.
//

#include "Logger.h"
#include <iostream>
#include <fstream>
#include <stdio.h>

KEngineCore::Logger::Logger()
{
    
}
KEngineCore::Logger::~Logger()
{
    Deinit();
}

void KEngineCore::Logger::Init()
{
    
}

void KEngineCore::Logger::Deinit()
{
    mLogHandlers.clear();
    mErrorHandlers.clear();
}

void KEngineCore::Logger::AddLogHandler(std::function<void(std::string_view)> handler)
{
    mLogHandlers.push_back(handler);
}

void KEngineCore::Logger::AddErrorHandler(std::function<void(std::string_view)> handler)
{
    mErrorHandlers.push_back(handler);
}

void KEngineCore::Logger::LogInternal(std::string_view logString)
{
    for (auto logHandler : mLogHandlers)
    {
        logHandler(logString);
    }
}

void KEngineCore::Logger::LogErrorInternal(std::string_view logString)
{
    for (auto errorHandler : mErrorHandlers)
    {
        errorHandler(logString);
    }
    std::cerr << logString << std::endl;
}

void KEngineCore::Logger::ReplaceLuaPrint(lua_State* luaState)
{
    auto printToLogger = [](lua_State* luaState) {
        KEngineCore::Logger* logger = (KEngineCore::Logger*)lua_touserdata(luaState, lua_upvalueindex(1));
        int n = lua_gettop(luaState);  /* number of arguments */
        int i;
        std::string output;
        for (i = 1; i <= n; i++) {  /* for each argument */
          size_t l;
          const char *s = luaL_tolstring(luaState, i, &l);  /* convert it to string */
          if (i > 1)  /* not the first element? */
            output.append("\t");  /* add a tab before it */
          output.append(s, l);  /* print it */
          lua_pop(luaState, 1);  /* pop result */
        }
        logger->LogInternal(output);
        return 0;
    };
    
    lua_getglobal(luaState, "_G");
    lua_pushstring(luaState, "print");
    lua_pushlightuserdata(luaState, this); // set this logger to be the upvalue
    lua_pushcclosure(luaState, printToLogger, 1); // Push the library function with the upvalue, pops the upvalue
    lua_settable(luaState, -3); // sets _G["print"] = printToLogger, pops "print" and printToLogger
    lua_pop(luaState, 1); // pops _G
}
