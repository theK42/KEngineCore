#include "LuaContext.h"
#include "LuaScheduler.h"
#include <assert.h>

const char KEngineCore::LuaContext::MetaName[] = "KEngineCore.LuaContext";

KEngineCore::LuaContext::LuaContext()
{
}

KEngineCore::LuaContext::~LuaContext()
{
	Deinit();
}

void KEngineCore::LuaContext::Init(LuaScheduler* scheduler, LuaContext* parent)
{
	assert(mScheduler == nullptr);
	mScheduler = scheduler;
	lua_State* luaState = scheduler->GetMainState();
	lua_checkstack(luaState, parent ? 4 : 2);
	lua_pushlightuserdata(luaState, this); // key
	lua_newtable(luaState); // value
	if (parent != nullptr)
	{
		lua_newtable(luaState); //metatable
		parent->PushToLua(luaState); // parent table
		lua_setfield(luaState, -2, "__index"); // set up forwarding to the parent
		lua_setmetatable(luaState, -2); // set as metatable
	}
	lua_settable(luaState, LUA_REGISTRYINDEX); // registry[&context] = table;
	
	AddContextualObject(KEngineCore::LuaContext::MetaName, this);
}

void KEngineCore::LuaContext::Deinit()
{
	if (mScheduler != nullptr)
	{
		lua_State* luaState = mScheduler->GetMainState();
		lua_checkstack(luaState, 2);
		lua_pushlightuserdata(luaState, this);
		lua_pushnil(luaState);
		lua_settable(luaState, LUA_REGISTRYINDEX);
		for (auto thread : mRunningThreads)
		{
			thread->Deinit();
		}
		mRunningThreads.clear();
		mScheduler = nullptr;
	}
}

void KEngineCore::LuaContext::RunScript(const std::string_view& scriptPath, ScheduledLuaThread * thread)
{
	thread->Init(mScheduler, scriptPath, 1, true);
	lua_State* luaState = thread->GetThreadState();// mScheduler->GetMainState();
	lua_checkstack(luaState, 1);
	PushToLua(luaState);
	mRunningThreads.push_back(thread);
}

void KEngineCore::LuaContext::AddContextualObject(const char* name, void* thingy)
{
	lua_State* luaState = mScheduler->GetMainState();
	PushToLua(luaState);
	lua_pushstring(luaState, name);
	lua_pushlightuserdata(luaState, thingy);
	lua_settable(luaState, -3);
	lua_pop(luaState, 1);
}

void KEngineCore::LuaContext::PushToLua(lua_State* luaState)
{
	assert(mScheduler != nullptr);
	lua_checkstack(luaState, 1);
	lua_pushlightuserdata(luaState, this);
	lua_gettable(luaState, LUA_REGISTRYINDEX);
}

KEngineCore::LuaScheduler* KEngineCore::LuaContext::GetLuaScheduler() const
{
	return mScheduler;
}

KEngineCore::LuaContext* KEngineCore::LuaContext::GetFromState(lua_State* luaState, int stackIndex)
{
	lua_checkstack(luaState, 1);
	LuaAssert(luaState, lua_istable(luaState, -1), "Requires context table at parameter {}", stackIndex);
	lua_getfield(luaState, stackIndex, MetaName);
	LuaAssert(luaState, lua_isuserdata(luaState, -1), "Failed to get context from context table");
	LuaContext* context =  static_cast<LuaContext*>(lua_touserdata(luaState, -1));
	lua_pop(luaState, 1);
	return context;
}

KEngineCore::ContextLibrary::ContextLibrary()
{
}

KEngineCore::ContextLibrary::~ContextLibrary()
{
	Deinit();
}

void KEngineCore::ContextLibrary::Init(lua_State* luaState)
{
	auto luaopen_context = [](lua_State* luaState) {

		auto createContext = [](lua_State* luaState) {
			KEngineCore::ContextLibrary* contextLib = (KEngineCore::ContextLibrary*)lua_touserdata(luaState, lua_upvalueindex(1));
			KEngineCore::LuaContext* parentContext = contextLib->GetContextualObject(luaState, 1);
			KEngineCore::LuaScheduler* scheduler = parentContext->GetLuaScheduler();

			KEngineCore::LuaContext* newContext = new (lua_newuserdata(luaState, sizeof(KEngineCore::LuaContext))) KEngineCore::LuaContext;
			luaL_getmetatable(luaState, KEngineCore::LuaContext::MetaName);
			lua_setmetatable(luaState, -2);

			newContext->Init(scheduler, parentContext);
			return 1;
		};

		auto deleteContext = [](lua_State* luaState) {
			KEngineCore::LuaContext* context = (KEngineCore::LuaContext*)luaL_checkudata(luaState, 1, KEngineCore::LuaContext::MetaName);
			context->~LuaContext();
			return 0;
		};


		const luaL_Reg contextLibrary[] = {
			{"createContext", createContext},
			{nullptr, nullptr}
		};

		auto runScript = [](lua_State* luaState) {
			//TODO IMPLEMENT
			return 0;
		};

		auto stop = [](lua_State* luaState) {
			//TODO IMPLEMENT
			return 0;
		};

		const luaL_Reg contextMethods[] = {
			{"runScript", runScript},
			{"stop", stop},
			{nullptr, nullptr}
		};


		lua_checkstack(luaState, 5);
		luaL_newmetatable(luaState, KEngineCore::LuaContext::MetaName);
		luaL_newlibtable(luaState, contextMethods);
		lua_pushvalue(luaState, lua_upvalueindex(1));
		luaL_setfuncs(luaState, contextMethods, 1);
		lua_setfield(luaState, -2, "__index");
		lua_pushstring(luaState, "__gc");
		lua_pushcfunction(luaState, deleteContext);
		lua_pop(luaState, 1);


		luaL_newlibtable(luaState, contextLibrary);
		lua_pushvalue(luaState, lua_upvalueindex(1));
		luaL_setfuncs(luaState, contextLibrary, 1);
		return 1;
	};

	LuaLibraryTwo<LuaContext>::Init(luaState, "time", luaopen_context);
}

void KEngineCore::ContextLibrary::Deinit()
{
	LuaLibraryTwo::Deinit();
}
