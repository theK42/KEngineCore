#include "LuaLibrary.h"


KEngineCore::LuaLibrary::LuaLibrary(void)
{
}


KEngineCore::LuaLibrary::~LuaLibrary(void)
{
}

void KEngineCore::LuaLibrary::PreloadLibrary(lua_State * luaState, char const * name, lua_CFunction libraryFunction) {
	lua_getglobal(luaState, "package");
	lua_getfield(luaState, -1, "preload");
	lua_pushlightuserdata(luaState, this);
	lua_pushcclosure(luaState, libraryFunction, 1);
	lua_setfield(luaState, -2, name);
	lua_pop(luaState, 2);
}