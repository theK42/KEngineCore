#pragma once

#include "Lua/lua.hpp"
#include <functional>

namespace KEngineCore {
class LuaLibrary
{
public:
	LuaLibrary(void);
	virtual ~LuaLibrary(void);

	virtual void RegisterLibrary(lua_State * luaState, char const * name) = 0;
	void PreloadLibrary(lua_State * luaState, char const * name, lua_CFunction libraryFunction);
	void PreloadLibraryIntoTable(lua_State * luaState, char const * name, lua_CFunction libraryFunction, int tableIndex);

	static void CreateLocalEnvironment(lua_State * scriptState, std::function<void (lua_State *)> registerLocalLibraries);
};


}
