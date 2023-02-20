#include "LuaScheduler.h"
#include "Lua.hpp"
#include "TextFile.h"
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

	//Create a weak table for script loading
	lua_checkstack(mMainState, 4);
	lua_newtable(mMainState);
	lua_newtable(mMainState);
	lua_pushstring(mMainState, "__mode");
	lua_pushstring(mMainState, "v");
	lua_settable(mMainState, -3);
	lua_setmetatable(mMainState, -2);
	mScriptTableRegistryIndex = luaL_ref(mMainState, LUA_REGISTRYINDEX);
}

void KEngineCore::LuaScheduler::Deinit() {
	mRunningThreads.Clear();
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
		mResumingThreads.PushBack(thread);
	}
	mAllThreads[thread->mThreadState] = thread;
}

void KEngineCore::LuaScheduler::LoadScript(lua_State * thread, char const * scriptPath) 
{
	lua_rawgeti(mMainState, LUA_REGISTRYINDEX, mScriptTableRegistryIndex);
	lua_pushstring(mMainState, scriptPath);
	lua_gettable(mMainState, -2);
	if (lua_type(mMainState, -1) != LUA_TFUNCTION) {
		lua_pop(mMainState, 1);
		int val = luaL_loadfile(mMainState, scriptPath);  
		char const * string;
		switch (val) {
		case LUA_ERRSYNTAX: ///syntax error during pre-compilation;			
			string = lua_tostring(mMainState, -1);
			fprintf(stderr, "Syntax Error: %s\n", string);
			lua_pop(mMainState, 1);
			assert(0);
			break;
		case LUA_ERRMEM: //memory allocation error.
			string = lua_tostring(mMainState, -1);
			fprintf(stderr, "Memory Error: %s\n", string);
			lua_pop(mMainState, 1);
			assert(0);
			break;
		case LUA_ERRFILE: //Couldn't load file
			string = lua_tostring(mMainState, -1);
			fprintf(stderr, "File Load Error: %s\n", string);
			lua_pop(mMainState, 1);
			assert(0);
			break;
		}
		lua_pushstring(mMainState, scriptPath);
		lua_pushvalue(mMainState, -2);
		lua_settable(mMainState, -4);
	}
	lua_xmove(mMainState, thread, 1);
	lua_pop(mMainState, 1);
}

void KEngineCore::LuaScheduler::PauseThread(ScheduledLuaThread * thread) {
	assert(mMainState);
	assert(thread->mScheduler == this);
	mPausingThreads.PushBack(thread);
}

void KEngineCore::LuaScheduler::ResumeThread(ScheduledLuaThread * thread) {
	assert(mMainState);
	assert(thread->mScheduler == this);
	mResumingThreads.PushBack(thread);
}

void KEngineCore::LuaScheduler::KillThread(ScheduledLuaThread * thread) {
	assert(thread != mCurrentRunningThread);//Look, it's simply not valid to kill threads while they are running.  
	assert(mMainState);
	assert(thread->mScheduler == this);
	lua_checkstack(mMainState, 2);
	lua_pushlightuserdata(mMainState, thread);
	lua_pushnil(mMainState);
	lua_settable(mMainState, LUA_REGISTRYINDEX);

	int threadRef = thread->GetRegistryIndex();
	thread->SetRegistryIndex(LUA_REFNIL);
	luaL_unref(mMainState, LUA_REGISTRYINDEX, threadRef);
	
	mRunningThreads.RemoveIfPresent(thread);
	mPausingThreads.RemoveIfPresent(thread);
	mResumingThreads.RemoveIfPresent(thread);

	mAllThreads.erase(thread->mThreadState);
}

void KEngineCore::LuaScheduler::Update() {
	std::vector<ScheduledLuaThread *> deadThreads;
	
	auto iterator = mPausingThreads.begin();


	for (ScheduledLuaThread * thread : mPausingThreads)
	{
		mRunningThreads.Remove(thread);
	}	
	mPausingThreads.Clear();
	
	for (ScheduledLuaThread * thread : mResumingThreads)
	{
		mRunningThreads.PushFront(thread);
	}
	mResumingThreads.Clear();
	
	for (ScheduledLuaThread * thread : mRunningThreads)
	{
		mCurrentRunningThread = thread;
		lua_State * threadState = thread->mThreadState;
		int nstack = 0;  
		int scriptResult = lua_resume(threadState, nullptr, thread->mReturnValues, &nstack);
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
	mCurrentRunningThread = nullptr;
	
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

	lua_State* thread = lua_newthread(luaState);
	if (lua_type(luaState, 1) == LUA_TSTRING) {
		char const * fileName = lua_tostring(luaState, 1); 
	    KEngineCore::TextFile file;
        file.LoadFromFile(fileName, ".lua");
		int val = luaL_loadbuffer(luaState, file.GetContents().c_str(), file.GetContents().length(), fileName); //TODO: Check return value
	} else {
		luaL_checktype(luaState, 1, LUA_TFUNCTION);
		lua_checkstack(luaState, 1);
		lua_pushvalue(luaState, 1);
	}
			
	lua_checkstack(thread, 1); 
	assert(lua_type(luaState, -1) == LUA_TFUNCTION);
	lua_xmove(luaState, thread, 1); //move the function from the parent thread to the child
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

void KEngineCore::ScheduledLuaThread::Init(LuaScheduler * scheduler, std::string_view fileName, bool run)
{
	lua_State * mainState = scheduler->GetMainState();
	lua_State * thread = lua_newthread(mainState);
    
    /*KEngineCore::TextFile file;
    file.LoadFromFile(fileName, ".lua");
    int val = luaL_loadbuffer(thread, file.GetContents().c_str(), file.GetContents().length(), fileName); //TODO: Check return value
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
	}*/

	std::string fileNameCopy(fileName);

	scheduler->LoadScript(thread, fileNameCopy.c_str());
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