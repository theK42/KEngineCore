#pragma once

#include "Lua.hpp"
#include <functional>
#include <assert.h>
#include <string>

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

	template <typename T, const char* S>
	static void CreateGCMetaTableForClass(lua_State* luaState);

	template <typename T, const char* S>
	static void CreateEmptyMetaTableForClass(lua_State* luaState);

};


template <typename T>
class LuaWrapping
{
public:
	LuaWrapping() {mMetaTableName = nullptr;}
	~LuaWrapping() {Deinit();}
	void Init(const char * metaTableName) { mMetaTableName = metaTableName; }
	void Deinit() { mMetaTableName = nullptr; }
	void WrapAndPush(lua_State * luaState, int index) const ; 
	T * Unwrap(lua_State * luaState, int index) const;
	char const * GetMetaTableName() const { return mMetaTableName; }
private:
	char const * mMetaTableName;
};

template <typename T>
void LuaWrapping<T>::WrapAndPush(lua_State * luaState, int index) const {
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
T * LuaWrapping<T>::Unwrap(lua_State * luaState, int index) const {
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


template <typename T, const char* S>
void LuaLibrary::CreateGCMetaTableForClass(lua_State* luaState)
{
	auto destroy = [](lua_State* luaState) {
		T* binding = (T*)luaL_checkudata(luaState, 1, S);
		binding->~T();
		return 0;
	};

	lua_checkstack(luaState, 3);
	luaL_newmetatable(luaState, S);
	lua_pushstring(luaState, "__gc");
	lua_pushcfunction(luaState, destroy);
	lua_settable(luaState, -3);
	lua_pop(luaState, 1);
}

template <typename T, const char* S>
void LuaLibrary::CreateEmptyMetaTableForClass(lua_State* luaState)
{
	lua_checkstack(luaState, 1);
	luaL_newmetatable(luaState, S);
	lua_pop(luaState, 1);
}

}
