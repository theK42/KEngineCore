#pragma once

#include "Lua.hpp"
#include <functional>
#include <assert.h>

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


template <typename T>
class LuaWrapping
{
public:
	LuaWrapping() {mMetaTableName = nullptr;}
	~LuaWrapping() {Deinit();}
	void Init(char * metaTableName) { mMetaTableName = metaTableName; }
	void Deinit() { mMetaTableName = nullptr; }
	void WrapAndPush(lua_State * luaState, int index); 
	T * Unwrap(lua_State * luaState, int index);
	char const * GetMetaTableName() const { return mMetaTableName; }
private:
	char const * mMetaTableName;
};

template <typename T>
void LuaWrapping<T>::WrapAndPush(lua_State * luaState, int index) {
	lua_checkstack(luaState, 2);
	if (!lua_islightuserdata(luaState, index)) {
		luaL_argerror(luaState, index, "Light Userdata required.");
		assert(0);
		return;
	}
	T * pointer = (T *)lua_touserdata(luaState, index);
	T ** pointerWrapper = (T **)lua_newuserdata(luaState, sizeof(T *));
	(*pointerWrapper) = pointer;
	luaL_getmetatable(luaState, mMetaTableName);
	lua_setmetatable(luaState, -2);
	
}

template <typename T>
T * LuaWrapping<T>::Unwrap(lua_State * luaState, int index) {
	assert(mMetaTableName != nullptr);
	T * pointer;
	if (lua_islightuserdata(luaState, index)) {
		pointer = (T *)lua_touserdata(luaState, index);
	} else if (lua_isuserdata(luaState, index)) {
		pointer = *(T **)luaL_checkudata(luaState, index, mMetaTableName);
	} else {
		pointer = nullptr;
		luaL_argerror(luaState, 1, "%s required.");
	}
	return pointer;
}



}
