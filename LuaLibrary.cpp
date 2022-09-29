#include "LuaLibrary.h"
#include <functional>

KEngineCore::LuaLibrary::LuaLibrary(void)
{
}


KEngineCore::LuaLibrary::~LuaLibrary(void)
{
}

void KEngineCore::LuaLibrary::PreloadLibrary(lua_State * luaState, char const * name, lua_CFunction libraryFunction) {
	lua_getglobal(luaState, "package");
	lua_getfield(luaState, -1, "preload");
	PreloadLibraryIntoTable(luaState, name, libraryFunction, -1);
	lua_pop(luaState, 2);
}

void KEngineCore::LuaLibrary::PreloadLibraryIntoTable(lua_State * luaState, char const * name, lua_CFunction libraryFunction, int tableIndex) {
	lua_checkstack(luaState, 3);
	lua_pushvalue(luaState, tableIndex);
	lua_pushlightuserdata(luaState, this);
	lua_pushcclosure(luaState, libraryFunction, 1);
	lua_setfield(luaState, -2, name);
	lua_pop(luaState, 1);
}


int localRequire(lua_State * luaState) {
	luaL_checktype(luaState, 1, LUA_TSTRING);
	lua_checkstack(luaState, 2);
	lua_pushvalue(luaState, 1);
	lua_gettable(luaState, lua_upvalueindex(1)); //Upvalue 1 is the local "loaded" table
	if (lua_type(luaState, -1) != LUA_TNIL) {  //If there's something there, return it
		return 1;
	}
	lua_pop(luaState, 1);//otherwise pop
	lua_checkstack(luaState, 2);
	lua_pushvalue(luaState, 1);
	lua_gettable(luaState, lua_upvalueindex(2)); //Upvalue 2 is the local "preload" table
	if (lua_type(luaState, -1) == LUA_TFUNCTION) {  //If there's something there, call it and return what it returns
		lua_checkstack(luaState, 1);
		lua_pushvalue(luaState, 1); //Push the name of the module again
		lua_call(luaState, 1, 1); //Call the module loader with one return value
		lua_checkstack(luaState, 2);
		lua_pushvalue(luaState, 1); //Push the name of the module again
		lua_pushvalue(luaState, -2); //Push the loader's return value
		lua_settable(luaState, lua_upvalueindex(1)); //Set the return value of the loader into the local "loaded" table
		return 1;
	}
	lua_pop(luaState, 1); //otherwise pop
	lua_checkstack(luaState, 2);
	lua_pushvalue(luaState, lua_upvalueindex(3)); //Upvalue 3 is the original "require" function
	lua_pushvalue(luaState, 1); //Push the module name
	lua_call(luaState, 1, 1);
	return 1;
}

void KEngineCore::LuaLibrary::CreateLocalEnvironment(lua_State * scriptState, std::function<void (lua_State *)> registerLocalLibraries) {
	lua_checkstack(scriptState, 4);
	lua_getupvalue(scriptState, -1, 1); //Get the current environment (NASTY)
	lua_newtable(scriptState); //New table to be the custom environment
	lua_newtable(scriptState); //New table to be metatable of custom environment
	lua_pushvalue(scriptState, -3); //Get the old environment
	lua_setfield(scriptState, -2, "__index"); //Set up the replacement's metatable to forward to the old table
	lua_setmetatable(scriptState, -2); //Set the replacement's metatable
	lua_replace(scriptState, -2); //replace the old table with the replacement
	
	//Create a custom require function that checks local libraries first
	lua_checkstack(scriptState, 2);
	lua_newtable(scriptState); //new table to serve as loaded
	lua_newtable(scriptState); //new table to serve as preload
	
	//Register local library
	registerLocalLibraries(scriptState);
	
	lua_checkstack(scriptState, 4);
	lua_getglobal(scriptState, "require"); //original require for fallback
	lua_pushcclosure(scriptState, localRequire, 3); //push the new require function
	lua_pushstring(scriptState, "require"); //push the name require
	lua_pushvalue(scriptState, -2); //re-push the require function
	lua_remove(scriptState, -3); //remove the original copy of the new require function
	lua_rawset(scriptState, -3); //add it to the environment

	//Set the new environment as the first upvalue of the script
	lua_setupvalue(scriptState, -2, 1);
}


