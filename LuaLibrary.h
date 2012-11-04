#pragma once

#include "Lua/lua.hpp"

namespace KEngineCore {
class LuaLibrary
{
public:
	LuaLibrary(void);
	virtual ~LuaLibrary(void);

	virtual void RegisterLibrary(lua_State * luaState, char const * name) = 0;
	void PreloadLibrary(lua_State * luaState, char const * name, lua_CFunction libraryFunction);
	void PreloadLibraryIntoTable(lua_State * luaState, char const * name, lua_CFunction libraryFunction, int tableIndex);
};


}
