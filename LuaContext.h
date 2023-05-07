#pragma once
#include "LuaLibrary.h"
#include "Lua.hpp"
#include <vector>
#include <string>


namespace KEngineCore {

	class LuaScheduler;
	class ScheduledLuaThread;

	class LuaContext
	{
	public:
		LuaContext();
		~LuaContext();

		void Init(LuaScheduler *scheduler, LuaContext * parent);
		void Deinit();

		void RunScript(const std::string_view&, ScheduledLuaThread * thread);
		void AddContextualObject(const char* name, void* thingy);
		void PushToLua(lua_State* luaState);
		LuaScheduler* GetLuaScheduler() const;

		static LuaContext* GetFromState(lua_State* luaState, int stackIndex); 

		static const char MetaName[];

	private:
		LuaScheduler*						mScheduler{ nullptr };
		std::vector<ScheduledLuaThread* >	mRunningThreads;
	};

	class ContextLibrary : protected LuaLibraryTwo<LuaContext> {
	public:
		ContextLibrary();
		~ContextLibrary();
		void Init(lua_State* luaState);
		void Deinit();
	};
}
