#pragma once

#include <map>
#include <list>
#include <vector>
#include <functional>
#include "LuaLibrary.h"

struct lua_State;

namespace KEngineCore {

class ScheduledLuaThread;

struct ScheduledLuaCallback {
	std::function<void ()> mCallback;
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

	ScheduledLuaCallback CreateCallback(lua_State * luaState, int index);

	ScheduledLuaThread * GetScheduledThread(lua_State * thread);

private:
	void ScheduleThread(ScheduledLuaThread * thread, bool running);
	lua_State *	mMainState;
	std::vector<ScheduledLuaThread *>			mResumingThreads;
	std::vector<ScheduledLuaThread *>			mPausingThreads;
	std::list<ScheduledLuaThread *>				mRunningThreads;
	std::map<lua_State *, ScheduledLuaThread *>	mAllThreads;

	friend class ScheduledLuaThread;
};

class ScheduledLuaThread
{
public:
	ScheduledLuaThread();
	~ScheduledLuaThread();
	
	void Init(LuaScheduler * scheduler, lua_State * thread, bool run = false);
	void Init(LuaScheduler * scheduler, char const * scriptPath, bool run = false);
	void Deinit();
	
	void Pause();
	void Resume(int returnValues = 0);
	void Kill();
	
	void SetRegistryIndex(int registryIndex);
	int GetRegistryIndex() const;
	LuaScheduler * GetLuaScheduler() const;

	lua_State * GetThreadState() const;
private:

	LuaScheduler *								mScheduler;
	lua_State *									mThreadState;
	std::list<ScheduledLuaThread *>::iterator	mPosition;
	int											mRegistryIndex;
	int											mReturnValues;

	friend class LuaScheduler;
};


}