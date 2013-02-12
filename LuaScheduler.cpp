#include "LuaScheduler.h"
#include "Lua/lua.hpp"
#include <assert.h>
#include <vector>


KEngineCore::LuaScheduler::LuaScheduler(void)
{
	mMainState = nullptr;
}


KEngineCore::LuaScheduler::~LuaScheduler(void)
{
	Deinit();
}

void KEngineCore::LuaScheduler::Init() {
	assert(!mMainState);
	mMainState = luaL_newstate();
	luaL_openlibs(mMainState);
	RegisterLibrary(mMainState);
}

void KEngineCore::LuaScheduler::Deinit() {
	mRunningThreads.clear();
	std::vector<ScheduledLuaThread *> deadThreads;
	for (auto iterator = mAllThreads.begin(); iterator != mAllThreads.end(); iterator++) {
		deadThreads.push_back(iterator->second);
	}
	for (auto it = deadThreads.begin(); it != deadThreads.end(); it++) {
		KillThread(*it);
	}
	mAllThreads.clear();
	if (mMainState != nullptr) {
		lua_close(mMainState);
		mMainState = nullptr;
	}
}

lua_State * KEngineCore::LuaScheduler::GetMainState() const  {
	return mMainState;
}

void KEngineCore::LuaScheduler::ScheduleThread(ScheduledLuaThread * thread, bool running) {
	assert(mMainState);
	//Push the thread into the lua registry with its pointer as key
	lua_checkstack(mMainState, 2);
	lua_pushlightuserdata(mMainState, thread); // push the pointer to be the key
	lua_pushthread(thread->mThreadState); // push the thread ... to itself
	lua_xmove(thread->mThreadState, mMainState, 1); // move the thread to the main state
	lua_settable(mMainState, LUA_REGISTRYINDEX); // registry[&thread] = thread->mThreadState;
	if (running) {
		mResumingThreads.push_back(thread);
	}
	mAllThreads[thread->mThreadState] = thread;
}


void KEngineCore::LuaScheduler::PauseThread(ScheduledLuaThread * thread) {
	assert(mMainState);
	assert(thread->mScheduler == this);
	mPausingThreads.push_back(thread);
}

void KEngineCore::LuaScheduler::ResumeThread(ScheduledLuaThread * thread) {
	assert(mMainState);
	assert(thread->mScheduler == this);
	mResumingThreads.push_back(thread);
}

void KEngineCore::LuaScheduler::KillThread(ScheduledLuaThread * thread) {
	assert(mMainState);
	assert(thread->mScheduler == this);
	lua_checkstack(mMainState, 2);
	lua_pushlightuserdata(mMainState, thread);
	lua_pushnil(mMainState);
	lua_settable(mMainState, LUA_REGISTRYINDEX);

	int threadRef = thread->GetRegistryIndex();
	thread->SetRegistryIndex(LUA_REFNIL);
	luaL_unref(mMainState, LUA_REGISTRYINDEX, threadRef);

	if (thread->mPosition != mRunningThreads.end()) {
		mRunningThreads.erase(thread->mPosition);
		thread->mPosition = mRunningThreads.end();
	}
	mAllThreads.erase(thread->mThreadState);
}

void KEngineCore::LuaScheduler::Update() {
	std::vector<ScheduledLuaThread *> deadThreads;

	for (auto it = mPausingThreads.begin(); it != mPausingThreads.end(); it++)
	{
		ScheduledLuaThread *thread = *it;
		mRunningThreads.erase(thread->mPosition);
		thread->mPosition = mRunningThreads.end();
	}	
	mPausingThreads.clear();

	for (auto it = mResumingThreads.begin(); it != mResumingThreads.end(); it++)
	{
		ScheduledLuaThread *thread = *it;
		mRunningThreads.push_front(*it);
		thread->mPosition = mRunningThreads.begin();
	}
	mResumingThreads.clear();

	for (auto it = mRunningThreads.begin(); it != mRunningThreads.end(); it++)
	{
		ScheduledLuaThread * thread = *it;
		lua_State * threadState = thread->mThreadState;

		int scriptResult = lua_resume(threadState, nullptr, thread->mReturnValues);
		thread->mReturnValues = 0;
		if (scriptResult != LUA_YIELD) {
			deadThreads.push_back(thread);
			if (scriptResult != 0) {
				char const * string = lua_tostring(threadState, -1);
				fprintf(stderr, "%s", string);
				lua_pop(threadState, 1);
				assert(0);
			}
		}
	}

	for (auto it = deadThreads.begin(); it != deadThreads.end(); it++) {
		KillThread(*it);
	}
}

KEngineCore::ScheduledLuaThread * KEngineCore::LuaScheduler::GetScheduledThread(lua_State * thread)
{
	return mAllThreads[thread];
}

static int create(lua_State * luaState) {
	KEngineCore::LuaScheduler * scheduler = (KEngineCore::LuaScheduler *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::ScheduledLuaThread * scheduledThread = new (lua_newuserdata(luaState, sizeof(KEngineCore::ScheduledLuaThread))) KEngineCore::ScheduledLuaThread;
	lua_checkstack(luaState, 1);
	luaL_getmetatable(luaState, "KEngineCore.ScheduledThread");
	lua_setmetatable(luaState, -2);
	
	char const * scriptPath = luaL_optstring(luaState, 1, nullptr);
	if (scriptPath != nullptr) {
		int val = luaL_loadfile(luaState, scriptPath); //TODO: Check return value
	} else {
		luaL_checktype(luaState, 1, LUA_TFUNCTION);
		lua_checkstack(luaState, 1);
		lua_pushvalue(luaState, 1);
	}
			
	lua_State * thread = lua_newthread(luaState);	
	lua_checkstack(luaState, 1);
	lua_xmove(luaState, scheduledThread->GetThreadState(), 1); //move the function from the parent thread to the child
	scheduledThread->Init(scheduler, thread);

	lua_pop(luaState, 1);  // Pop the raw thread, it has been copied to the registry.
	return 1;
}

static int resume(lua_State * luaState) {
	KEngineCore::LuaScheduler * scheduler = (KEngineCore::LuaScheduler *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::ScheduledLuaThread * scheduledThread = (KEngineCore::ScheduledLuaThread *)luaL_checkudata(luaState, 1, "KEngineCore.ScheduledThread");
	scheduler->ResumeThread(scheduledThread);
	return 0;
}

static int pause(lua_State * luaState) {
	KEngineCore::LuaScheduler * scheduler = (KEngineCore::LuaScheduler *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::ScheduledLuaThread * scheduledThread = (KEngineCore::ScheduledLuaThread *)luaL_checkudata(luaState, 1, "KEngineCore.ScheduledThread");
	scheduler->PauseThread(scheduledThread);
	return 0;
}

static int kill(lua_State * luaState) {
	KEngineCore::LuaScheduler * scheduler = (KEngineCore::LuaScheduler *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::ScheduledLuaThread * scheduledThread = (KEngineCore::ScheduledLuaThread *)luaL_checkudata(luaState, 1, "KEngineCore.ScheduledThread");
	scheduler->KillThread(scheduledThread);
	return 0;
}

static int deleteScheduledThread(lua_State * luaState) {
	KEngineCore::ScheduledLuaThread * scheduledThread = (KEngineCore::ScheduledLuaThread *)luaL_checkudata(luaState, 1, "KEngineCore.ScheduledThread");
	scheduledThread->~ScheduledLuaThread();
	return 0;
}

static const struct luaL_Reg schedulerLibrary [] = {
	{"create", create},
	{"resume", resume},
	{"pause", pause},
	{"kill", kill},
	{nullptr, nullptr}
};

static int luaopen_scheduler (lua_State * luaState) {
	lua_checkstack(luaState, 2);
	luaL_newlibtable(luaState, schedulerLibrary);
	lua_pushvalue(luaState, lua_upvalueindex(1));
	luaL_setfuncs(luaState, schedulerLibrary, 1);
	
	return 1;
};

void KEngineCore::LuaScheduler::RegisterLibrary(lua_State * luaState, char const * name) {
	PreloadLibrary(luaState, name, luaopen_scheduler);
	
	lua_checkstack(luaState, 3);
	luaL_newmetatable(luaState, "KEngineCore.ScheduledThread");
	lua_pushstring(luaState, "__gc");
	lua_pushcfunction(luaState, deleteScheduledThread);
	lua_settable(luaState, -3);
	lua_pop(luaState, 1);
	
	lua_checkstack(luaState, 1);
	luaL_newmetatable(luaState, "KEngineCore.Callbackything");
	lua_pop(luaState, 1);
}

namespace KEngineCore{
struct CallbackChunk {
	LuaScheduler * mScheduler;
	int mSelfRegistryIndex;
	int mFunctionRegistryIndex;
};
}

//This callback doesn't work (tries to run a thread instead of a function?)  and if it did the callback would be non-reentrant!  MUST FIX!
KEngineCore::ScheduledLuaCallback KEngineCore::LuaScheduler::CreateCallback(lua_State * luaState, int callbackIndex) 
{	
	assert(mMainState);
	luaL_checktype(luaState, callbackIndex, LUA_TFUNCTION);
	lua_pushvalue(luaState, callbackIndex);
	int functionRegistryIndex = luaL_ref(luaState, LUA_REGISTRYINDEX);

	CallbackChunk * callbackChunk = new (lua_newuserdata(luaState, sizeof(CallbackChunk))) CallbackChunk;
	luaL_getmetatable(luaState, "KEngineCore.Callbackything");
	lua_setmetatable(luaState, -2);

	callbackChunk->mScheduler = this;
	callbackChunk->mFunctionRegistryIndex = functionRegistryIndex;  //Register the function
	callbackChunk->mSelfRegistryIndex = luaL_ref(luaState, LUA_REGISTRYINDEX);  //Register callback chunk
	
	
		
	auto cb = [callbackChunk] () {
			lua_State * luaState = callbackChunk->mScheduler->GetMainState();
			KEngineCore::ScheduledLuaThread * scheduledThread = new (lua_newuserdata(luaState, sizeof(KEngineCore::ScheduledLuaThread))) KEngineCore::ScheduledLuaThread;
			luaL_getmetatable(luaState, "KEngineCore.ScheduledThread");
			lua_setmetatable(luaState, -2);

			lua_State * thread = lua_newthread(luaState);
			lua_rawgeti(luaState, LUA_REGISTRYINDEX, callbackChunk->mFunctionRegistryIndex); //get the function
			luaL_checktype(luaState, -1, LUA_TFUNCTION); //check for a function
			lua_xmove(luaState, thread, 1); //move the function from the parent thread to the child

			scheduledThread->Init(callbackChunk->mScheduler, thread);
			lua_pop(luaState, 1); //Pop the thread off the stack now that it's been wrapped
			scheduledThread->SetRegistryIndex(luaL_ref(luaState, LUA_REGISTRYINDEX));  //This pops the wrapped thread and copies it to the registry as well
			scheduledThread->Resume();
		};

	auto cancel = [callbackChunk] () {
			lua_State * luaState = callbackChunk->mScheduler->GetMainState();
			luaL_unref(luaState, LUA_REGISTRYINDEX, callbackChunk->mFunctionRegistryIndex); // drop the function reference
			luaL_unref(luaState, LUA_REGISTRYINDEX, callbackChunk->mSelfRegistryIndex); // drop the chunk reference
		};

	ScheduledLuaCallback retVal = {
		cb,
		cancel
	};
	return retVal;
}

KEngineCore::ScheduledLuaThread::ScheduledLuaThread()
{
	mScheduler = nullptr;
	mThreadState = nullptr;
	mRegistryIndex = LUA_REFNIL;
	mReturnValues = 0;
}

KEngineCore::ScheduledLuaThread::~ScheduledLuaThread()
{
	Deinit();
}
	
void KEngineCore::ScheduledLuaThread::Init(LuaScheduler * scheduler, lua_State * thread, bool run)
{
	assert(mScheduler == nullptr);
	mScheduler = scheduler;
	mThreadState = thread;
	scheduler->ScheduleThread(this, run);
}

void KEngineCore::ScheduledLuaThread::Init(LuaScheduler * scheduler, char const * scriptPath, bool run)
{
	lua_State * mainState = scheduler->GetMainState();
	lua_State * thread = lua_newthread(mainState);
	int val = luaL_loadfile(thread, scriptPath);  //TODO Check return value
	char const * string;
	switch (val) {
		case LUA_ERRSYNTAX: ///syntax error during pre-compilation;
			
				string = lua_tostring(thread, -1);
				fprintf(stderr, "Syntax Error: %s", string);
				lua_pop(thread, 1);
				assert(0);
		break;
		case LUA_ERRMEM: //memory allocation error.
						string = lua_tostring(thread, -1);
				fprintf(stderr, "Memory Error: %s", string);
				lua_pop(thread, 1);
				assert(0);
		break;
		case LUA_ERRFILE: //Couldn't load file
						string = lua_tostring(thread, -1);
				fprintf(stderr, "File Load Error: %s", string);
				lua_pop(thread, 1);
				assert(0);
		break;
	}
	Init(scheduler, thread, run);
}

void KEngineCore::ScheduledLuaThread::Deinit()
{
	if (mScheduler != nullptr) {
		mScheduler->KillThread(this);
		mScheduler = nullptr;
		mThreadState = nullptr;
	}
}

void KEngineCore::ScheduledLuaThread::Pause()
{
	assert(mScheduler != nullptr);
	mScheduler->PauseThread(this);
}

void KEngineCore::ScheduledLuaThread::Resume(int returnValues)
{
	assert(mScheduler != nullptr);
	mReturnValues = returnValues;
	mScheduler->ResumeThread(this);
}

void KEngineCore::ScheduledLuaThread::Kill()
{
	assert(mScheduler != nullptr);
	mScheduler->KillThread(this);
}

void KEngineCore::ScheduledLuaThread::SetRegistryIndex(int registryIndex)
{
	mRegistryIndex = registryIndex;
}

int KEngineCore::ScheduledLuaThread::GetRegistryIndex() const
{
	assert(mScheduler != nullptr);
	return mRegistryIndex;
}

lua_State * KEngineCore::ScheduledLuaThread::GetThreadState() const
{
	assert(mScheduler != nullptr);
	return mThreadState;
}

KEngineCore::LuaScheduler * KEngineCore::ScheduledLuaThread::GetLuaScheduler() const
{
	assert(mScheduler != nullptr);
	return mScheduler;
}