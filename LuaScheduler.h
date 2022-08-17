#pragma once

#include <map>
#include <list>
#include <vector>
#include <functional>
#include <assert.h>
#include "List.h"
#include "LuaLibrary.h"

struct lua_State;

namespace KEngineCore {

class ScheduledLuaThread;


template <class ...args>
struct ScheduledLuaCallback {
    std::function<void (args...)> mCallback;
    std::function<void ()> mCancelCallback;
};

class LuaScheduler : protected LuaLibrary
{
public:
	LuaScheduler(void);
	virtual ~LuaScheduler(void);

	void Init();
	void Deinit();

	void Update();

	lua_State * GetMainState() const;
	
	void PauseThread(ScheduledLuaThread * thread);
	void ResumeThread(ScheduledLuaThread * thread);
	void KillThread(ScheduledLuaThread * thread);
	virtual void RegisterLibrary(lua_State * luaState, char const * name = "scheduler") override;

	ScheduledLuaCallback<> CreateCallback(lua_State * luaState, int index);

	template<class...args>
	ScheduledLuaCallback<args...> CreateCallback(lua_State* luaState, int index);

	ScheduledLuaThread * GetScheduledThread(lua_State * thread);

	void LoadScript(lua_State * thread, char const * scriptPath);
private:
	void ScheduleThread(ScheduledLuaThread * thread, bool running);
    lua_State *	mMainState {nullptr};
	List<ScheduledLuaThread, 0, 3>				mRunningThreads;
	List<ScheduledLuaThread, 1, 3>				mPausingThreads;
	List<ScheduledLuaThread, 2, 3>				mResumingThreads;
	std::map<lua_State *, ScheduledLuaThread *>	mAllThreads;
	int											mScriptTableRegistryIndex;
	friend class ScheduledLuaThread;
};

class ScheduledLuaThread : public ListElement<ScheduledLuaThread, 3>
{
public:
	ScheduledLuaThread();
	~ScheduledLuaThread();
	
	void Init(LuaScheduler * scheduler, lua_State * thread, bool run = false);
	void Init(LuaScheduler * scheduler, char const * scriptPath, bool run = false);
	template<typename T, typename ... Targs>
	int AddParameters(T t, Targs ... args); //Recursive variadic template
	int AddParameters() { return 0; }; //No-op base case for recursive definition of variadic template
	template<typename T>
	void AddParameter(T t);
	void Deinit();
	
	void Pause();
	void Resume(int returnValues = 0);
	void Kill();
	
	void SetRegistryIndex(int registryIndex);
	int GetRegistryIndex() const;
	LuaScheduler * GetLuaScheduler() const;

	lua_State * GetThreadState() const;
private:


	LuaScheduler *								mScheduler {nullptr};
	lua_State *									mThreadState {nullptr};
	int											mRegistryIndex {-1};
	int											mReturnValues {-1};

	friend class LuaScheduler;
};


template<typename T>
void pushToLua(lua_State* state, T t)
{
	assert(false);
}

template<>
inline void pushToLua<int>(lua_State* state, int t)
{
	lua_pushinteger(state, t);
}

template<typename T1, typename ...T>
int ScheduledLuaThread::AddParameters(T1 parameter, T ... parameters)
{
	AddParameter(parameter);
	return 1 + AddParameters(parameters...);
}

template<typename T>
void ScheduledLuaThread::AddParameter(T parameter)
{
	pushToLua(mThreadState, parameter);
}


struct CallbackChunk {
	LuaScheduler* mScheduler;
	int mSelfRegistryIndex;
	int mFunctionRegistryIndex;
};


//This callback doesn't work (tries to run a thread instead of a function?)  and if it did the callback would be non-reentrant!  MUST FIX! [Kelson, circa 2013]
//Years later, trying to understand this comment...Obviously this does something useful, it's the core of KEngine!  What did I mean? [Kelson, circa 2022]
template<typename...T>
ScheduledLuaCallback<T...> LuaScheduler::CreateCallback(lua_State* luaState, int callbackIndex)
{
	assert(mMainState);
	luaL_checktype(luaState, callbackIndex, LUA_TFUNCTION);
	lua_pushvalue(luaState, callbackIndex);
	int functionRegistryIndex = luaL_ref(luaState, LUA_REGISTRYINDEX);

	CallbackChunk* callbackChunk = new (lua_newuserdata(luaState, sizeof(CallbackChunk))) CallbackChunk;
	luaL_getmetatable(luaState, "KEngineCore.Callbackything");
	lua_setmetatable(luaState, -2);

	callbackChunk->mScheduler = this;
	callbackChunk->mFunctionRegistryIndex = functionRegistryIndex;  //Register the function
	callbackChunk->mSelfRegistryIndex = luaL_ref(luaState, LUA_REGISTRYINDEX);  //Register callback chunk (this keeps the CallbackChunk alive until cancel unregisters it)

	auto cb = [callbackChunk](T... args) {
		lua_State* luaState = callbackChunk->mScheduler->GetMainState();
		KEngineCore::ScheduledLuaThread* scheduledThread = new (lua_newuserdata(luaState, sizeof(KEngineCore::ScheduledLuaThread))) KEngineCore::ScheduledLuaThread;
		luaL_getmetatable(luaState, "KEngineCore.ScheduledThread");
		lua_setmetatable(luaState, -2);

		lua_State* thread = lua_newthread(luaState);
		lua_rawgeti(luaState, LUA_REGISTRYINDEX, callbackChunk->mFunctionRegistryIndex); //get the function
		luaL_checktype(luaState, -1, LUA_TFUNCTION); //check for a function
		lua_xmove(luaState, thread, 1); //move the function from the parent thread to the child
				
		scheduledThread->Init(callbackChunk->mScheduler, thread);
		lua_pop(luaState, 1); //Pop the thread off the stack now that it's been wrapped
		scheduledThread->SetRegistryIndex(luaL_ref(luaState, LUA_REGISTRYINDEX));  //This pops the wrapped thread and copies it to the registry as well
		int parameterCount = scheduledThread->AddParameters(args...);
		scheduledThread->Resume(parameterCount);
	};

	auto cancel = [callbackChunk]() {
		lua_State* luaState = callbackChunk->mScheduler->GetMainState();
		luaL_unref(luaState, LUA_REGISTRYINDEX, callbackChunk->mFunctionRegistryIndex); // drop the function reference
		luaL_unref(luaState, LUA_REGISTRYINDEX, callbackChunk->mSelfRegistryIndex); // drop the chunk reference
	};

	ScheduledLuaCallback<T...> retVal = {
		cb,
		cancel
	};
	return retVal;
}



}
