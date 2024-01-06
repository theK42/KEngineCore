#pragma once

#include "Lua.hpp"

#include <functional>
#include <assert.h>
#include <string>

#ifndef __cpp_lib_format //Clang doesn't support this, Android doesn't even have the header
#include "fmt/format.h"
#else
#include <format>
#endif

namespace KEngineCore {

template <typename T, const char* S>
void CreateGCMetaTableForClass(lua_State* luaState)
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

#ifdef NDEBUG
#define LuaAssert(luaState, expression, message, ...) ((void)0)
#else

    #ifdef __cpp_lib_format
        #define LuaAssert(luaState, expression, message, ...)								    \
            if (!expression)																	\
            {																					\
                std::string formattedMessage = std::format(message __VA_OPT__(,) __VA_ARGS__);  \
                luaL_error(luaState, "%s\n", formattedMessage.c_str());							\
            }
    #else
        #define LuaAssert(luaState, expression, message, ...)                                   \
            if (!expression)                                                                    \
            {                                                                                   \
                std::string formattedMessage = fmt::format(message __VA_OPT__(,) __VA_ARGS__);  \
                luaL_error(luaState, "%s\n", formattedMessage.c_str());                         \
            }
    #endif
#endif

template <class T>
class LuaLibraryTwo
{
public:
	LuaLibraryTwo() {}
	~LuaLibraryTwo() { Deinit(); }

	void Init(lua_State* luaState, char const* name, lua_CFunction libraryFunction);
	void Deinit();

	char const* GetName();

protected:
	T* GetContextualObject(lua_State * luaState, int contextIndex) const;
	char const* mName;
};

template<class T>
inline void KEngineCore::LuaLibraryTwo<T>::Init(lua_State* luaState, char const* name, lua_CFunction libraryFunction)
{
	mName = name;
	lua_checkstack(luaState, 4);

	lua_getglobal(luaState, "package");  //load global package table
	lua_getfield(luaState, -1, "preload");  //load package.preload table
	
	lua_pushlightuserdata(luaState, this); // This library object will be the first and only upvalue for the library function
	lua_pushcclosure(luaState, libraryFunction, 1); // Push the library function 
	lua_setfield(luaState, -2, name); // Set it into package.preload by name
	lua_pop(luaState, 2); // Clear out package and preload from the stack
}

template<class T>
inline void KEngineCore::LuaLibraryTwo<T>::Deinit()
{
	//Possibly try to remove the library from Lua?  Currently there's no situation where the libraries are deinited but the lua state is preserved.
}

template<class T>
inline char const* LuaLibraryTwo<T>::GetName()
{
	return mName;
}

template<class T>
inline T* KEngineCore::LuaLibraryTwo<T>::GetContextualObject(lua_State* luaState, int contextIndex) const
{	
	lua_checkstack(luaState, 1);
	int type = lua_type(luaState, contextIndex);

	LuaAssert(luaState, lua_istable(luaState, contextIndex), "Paramater {} must be a context table.", contextIndex);


	lua_getfield(luaState, contextIndex, mName); // grab the contextual object pointer (push it to the stack in place of the key)
	if (!lua_isuserdata(luaState, -1))
	{
		int type = lua_type(luaState, -1);
		const char* name = lua_typename(luaState, type);

		LuaAssert(luaState, lua_isuserdata(luaState, -1), "Context table doesn't contain a userdata named {}", mName);
	}

	LuaAssert(luaState, lua_isuserdata(luaState, -1), "Context table doesn't contain a userdata named {}", mName);
	assert(lua_isuserdata(luaState, -1));
	T* retVal = (T*)lua_touserdata(luaState, -1);
	lua_pop(luaState, 2);
	return retVal;

}

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
	int origIndex = index;
	if (lua_istable(luaState, index)) {
		lua_getfield(luaState, index, mMetaTableName);
		index = -1;
	}
	if (!lua_islightuserdata(luaState, index)) {
		luaL_argerror(luaState, origIndex, "Light Userdata (or context table containing one) required.");
		return;
	}
	T * pointer = (T *)lua_touserdata(luaState, index);
	if (index != origIndex)
	{
		lua_pop(luaState, 1);
	}
	T ** pointerWrapper = (T **)lua_newuserdata(luaState, sizeof(T *));
	(*pointerWrapper) = pointer;
	luaL_getmetatable(luaState, mMetaTableName);
	lua_setmetatable(luaState, -2);
	
}

template <typename T>
T * LuaWrapping<T>::Unwrap(lua_State * luaState, int index) const {
	assert(mMetaTableName != nullptr);
	int origIndex = index;
	T * pointer;
	if (lua_istable(luaState, index)) {
		lua_getfield(luaState, index, mMetaTableName);
		index = -1;	
	}
	if (lua_islightuserdata(luaState, index)) {
		pointer = (T *)lua_touserdata(luaState, index);
	} else if (lua_isuserdata(luaState, index)) {
		pointer = *(T **)luaL_checkudata(luaState, index, mMetaTableName);
	} 
	else {
		pointer = nullptr;
		luaL_argerror(luaState, origIndex, "%s required.");
	}
	if (index != origIndex)
	{
		lua_pop(luaState, 1);
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
