#include "Timer.h"
#include "LuaScheduler.h"
#include <cassert>

KEngineCore::Timer::Timer(void)
{
}

KEngineCore::Timer::~Timer(void)
{
	Deinit();
}

void KEngineCore::Timer::Init(LuaScheduler * scheduler)
{
	mScheduler = scheduler;
	RegisterLibrary(scheduler->GetMainState());
	mCurrentTime = 0;
	mProcessingTimeout = nullptr;
}

void KEngineCore::Timer::Deinit()
{
	mScheduler = nullptr;
	mTimeouts.clear();
}

void KEngineCore::Timer::Update(double time)
{
    constexpr int millisecondsPerSecond = 1000;
	mCurrentTime += time;
	int currentTime = (int)(mCurrentTime * millisecondsPerSecond);
	
	std::vector<Timeout *>	elapsedTimeouts;

	for (auto iterator = mTimeouts.begin(); iterator != mTimeouts.end(); iterator++) {
		mProcessingTimeout = *iterator;
		if (mProcessingTimeout->mFiresAt <= currentTime) {
			mProcessingTimeout->TimeElapsed();
			if (mProcessingTimeout->mRepeats) {
				mProcessingTimeout->mFiresAt += (int)(millisecondsPerSecond * mProcessingTimeout->mSetTime);
			} else {
				elapsedTimeouts.push_back(mProcessingTimeout);
			}
		}
	}
	mProcessingTimeout = nullptr;
	for (auto iterator = elapsedTimeouts.begin(); iterator != elapsedTimeouts.end(); iterator++) {
		Timeout * timeout = *iterator;
		mTimeouts.erase(timeout->mPosition);
		timeout->mPosition = mTimeouts.end();
	}
	elapsedTimeouts.clear();
}

void KEngineCore::Timer::AddTimeout(Timeout * timeout)
{
	mTimeouts.push_back(timeout);
	timeout->mPosition = std::next(mTimeouts.rbegin()).base();
	timeout->mFiresAt = (int)((timeout->mSetTime + mCurrentTime) * 1000);
}

void KEngineCore::Timer::ClearTimeout(Timeout * timeout)
{
	if (mProcessingTimeout == timeout) {
		timeout->mRepeats = false;
	} else {
		mTimeouts.erase(timeout->mPosition);		
		timeout->mPosition = mTimeouts.end();
	}
}

 KEngineCore::LuaScheduler *  KEngineCore::Timer::GetLuaScheduler() const {
	 return mScheduler;
 }

static int setTimeout(lua_State * luaState) {
	KEngineCore::Timer * timer = (KEngineCore::Timer *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::LuaScheduler * scheduler = timer->GetLuaScheduler();
	double requestedTime = luaL_checknumber(luaState, 1);

	luaL_checktype(luaState, 2, LUA_TFUNCTION);
	
	KEngineCore::Timeout * timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
	luaL_getmetatable(luaState, "KEngineCore.Timeout");
	lua_setmetatable(luaState, -2);

	KEngineCore::ScheduledLuaCallback<> callback = scheduler->CreateCallback<>(luaState, 2);
	timeout->Init(timer, requestedTime, false, callback.mCallback, callback.mCancelCallback);
	return 1;
}

static int setInterval(lua_State * luaState) {
	KEngineCore::Timer * timer = (KEngineCore::Timer *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::LuaScheduler * scheduler = timer->GetLuaScheduler();
	double requestedTime = luaL_checknumber(luaState, 1);
	
	luaL_checktype(luaState, 2, LUA_TFUNCTION);
	
	KEngineCore::Timeout * timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
	luaL_getmetatable(luaState, "KEngineCore.Timeout");
	lua_setmetatable(luaState, -2);
	
	KEngineCore::ScheduledLuaCallback<> callback = scheduler->CreateCallback<>(luaState, 2);
	timeout->Init(timer, requestedTime, true, callback.mCallback, callback.mCancelCallback);


	return 1;
}

static int clearTimeout(lua_State * luaState) {
	//KEngineCore::Timer * timer = (KEngineCore::Timer *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::Timeout * timeout = (KEngineCore::Timeout *)luaL_checkudata(luaState, 1, "KEngineCore.Timeout"); 
	timeout->Cancel();
	return 0;
}

static int waits(lua_State * luaState) {
	KEngineCore::Timer * timer = (KEngineCore::Timer *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::LuaScheduler * scheduler = timer->GetLuaScheduler();
	double requestedTime = luaL_checknumber(luaState, 1);
	
	KEngineCore::Timeout * timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
	luaL_getmetatable(luaState, "KEngineCore.Timeout");
	lua_setmetatable(luaState, -2);

	KEngineCore::ScheduledLuaThread * scheduledThread = scheduler->GetScheduledThread(luaState);
	scheduledThread->Pause();

	timeout->Init(timer, requestedTime, false, [scheduledThread] () {
		scheduledThread->Resume();
	});
	return lua_yield(luaState, 1);  //Pretend we're going to pass the timeout to resume so it doesn't get GC'd
}

static int deleteTimeout(lua_State * luaState) {
	KEngineCore::Timeout * timeout = (KEngineCore::Timeout *)luaL_checkudata(luaState, 1, "KEngineCore.Timeout"); 
	timeout->~Timeout();
	return 0;
}

static const struct luaL_Reg timerLibrary [] = {
	{"setTimeout", setTimeout},
	{"clearTimeout", clearTimeout},
	{"setInterval", setInterval}, 
	{"clearInterval", clearTimeout},
	{"waits", waits},
	{nullptr, nullptr}
};

static int luaopen_timer (lua_State * luaState) {
	lua_checkstack(luaState, 2);
	luaL_newlibtable(luaState, timerLibrary);
	lua_pushvalue(luaState, lua_upvalueindex(1));
	luaL_setfuncs(luaState, timerLibrary, 1);
	
	lua_checkstack(luaState, 3);
	luaL_newmetatable(luaState, "KEngineCore.Timeout");
	lua_pushstring(luaState, "__gc");
	lua_pushcfunction(luaState, deleteTimeout);
	lua_settable(luaState, -3);
	lua_pop(luaState, 1);
	
	return 1;
};


void KEngineCore::Timer::RegisterLibrary(lua_State * luaState, char const * name)
{	
	PreloadLibrary(luaState, name, luaopen_timer);
}


KEngineCore::Timeout::Timeout()
{
	mTimer = nullptr;
	mSetTime = 0;
	mFiresAt = 0;
	mRepeats = false;
}

KEngineCore::Timeout::~Timeout()
{
	Deinit();
}

void KEngineCore::Timeout::Init(Timer *timer, double time, bool repeats, std::function<void ()> callback, std::function<void ()> cancelCallback)
{
	assert(mTimer == nullptr);
	mTimer = timer;
	mSetTime = time;
	mRepeats = repeats;
	mCallback = callback;
	mCancelCallback = cancelCallback;
	timer->AddTimeout(this);
}

void KEngineCore::Timeout::Deinit()
{
	Cancel();
}

void KEngineCore::Timeout::TimeElapsed()
{
	assert(mCallback);
	mCallback();
}

void KEngineCore::Timeout::Cancel() {
	if (mTimer != nullptr && mPosition != mTimer->mTimeouts.end()) {
		mTimer->ClearTimeout(this);
		if (mCancelCallback) {
			mCancelCallback();
		}
	}
	mTimer = nullptr;
}
