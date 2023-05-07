#include "Timer.h"
#include "LuaScheduler.h"
#include "LuaContext.h"
#include <cassert>

const char KEngineCore::Timeout::MetaName[] = "KEngineCore.Timeout";

KEngineCore::Timer::Timer()
{
}

KEngineCore::Timer::~Timer()
{
	Deinit();
}

void KEngineCore::Timer::Init(LuaScheduler * scheduler)
{
	mScheduler = scheduler;
	mCurrentTime = 0;
	mProcessingTimeout = nullptr;
	mSpeed = 1.0;
	mRunning = true;
}

void KEngineCore::Timer::Deinit()
{
	for (auto timeout : mTimeouts)
	{
		timeout->Cancel(true);
	}
	mScheduler = nullptr;
	mTimeouts.clear();
	for (auto forwarder : mForwarders)
	{
		forwarder->Deinit(true);
	}
	mForwarders.clear();
	mRunning = false;
}

void KEngineCore::Timer::Update(double time)
{
	time *= mRunning ? mSpeed : 0.0f;
	if (time == 0.0)
	{
		return;
	}
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
		timeout->mTimer = nullptr;
	}
	elapsedTimeouts.clear();

	for (auto timein : mForwarders)
	{
		timein->Update(time);
	}
}

void KEngineCore::Timer::Pause()
{
	mRunning = false;
}

void KEngineCore::Timer::Resume()
{
	mRunning = true;
}

void KEngineCore::Timer::SetSpeed(double speed)
{
	mSpeed = speed;
}

double KEngineCore::Timer::GetSpeed() const
{
	return mSpeed;
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

void KEngineCore::Timer::AddTimeForwarder(TimeForwarder* timein)
{
	mForwarders.push_back(timein);
	timein->mPosition = std::next(mForwarders.rbegin()).base();
}

void KEngineCore::Timer::RemoveTimeForwarder(TimeForwarder* timein)
{
	mForwarders.erase(timein->mPosition);
	timein->mPosition = mForwarders.end();
}

 KEngineCore::LuaScheduler *  KEngineCore::Timer::GetLuaScheduler() const {
	 return mScheduler;
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

void KEngineCore::Timeout::Cancel(bool batched) {
	if (mTimer != nullptr && mPosition != mTimer->mTimeouts.end()) {
		if (!batched) {
			mTimer->ClearTimeout(this);
		}
		if (mCancelCallback) {
			mCancelCallback();
		}
	}
	mTimer = nullptr;
}

KEngineCore::TimeForwarder::TimeForwarder()
{
}

KEngineCore::TimeForwarder::~TimeForwarder()
{
	Deinit();
}

void KEngineCore::TimeForwarder::Init(Timer* timer, std::function<void(double)> callback)
{
	assert(mTimer == nullptr);
	mCallback = callback;
	mTimer = timer;
	timer->AddTimeForwarder(this);
}

void KEngineCore::TimeForwarder::Deinit(bool batched)
{
	if (mTimer && !batched)
	{
		mTimer->RemoveTimeForwarder(this);
	}
	mTimer = nullptr;
	mCallback = nullptr;
}

void KEngineCore::TimeForwarder::Update(double deltaTime)
{
	mCallback(deltaTime);
}

KEngineCore::TimeLibrary::TimeLibrary()
{
}

KEngineCore::TimeLibrary::~TimeLibrary()
{
	Deinit();
}

void KEngineCore::TimeLibrary::Init(lua_State* luaState)
{
	auto luaopen_time = [](lua_State* luaState) {
		auto setTimeout = [](lua_State* luaState) {
			KEngineCore::TimeLibrary* timeLib = (KEngineCore::TimeLibrary*)lua_touserdata(luaState, lua_upvalueindex(1));
			KEngineCore::Timer* timer = timeLib->GetContextualObject(luaState, 2);
			KEngineCore::LuaScheduler* scheduler = timer->GetLuaScheduler();
			double requestedTime = luaL_checknumber(luaState, 1);

			luaL_checktype(luaState, 2, LUA_TFUNCTION);

			KEngineCore::Timeout* timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
			luaL_getmetatable(luaState, "KEngineCore.Timeout");
			lua_setmetatable(luaState, -2);

			KEngineCore::ScheduledLuaCallback<> callback = scheduler->CreateCallback<>(luaState, 2);
			timeout->Init(timer, requestedTime, false, callback.mCallback, callback.mCancelCallback);
			return 1;
		};

		auto setInterval = [](lua_State* luaState) {
			KEngineCore::TimeLibrary* timeLib = (KEngineCore::TimeLibrary*)lua_touserdata(luaState, lua_upvalueindex(1));
			KEngineCore::Timer* timer = timeLib->GetContextualObject(luaState, 2);
			KEngineCore::LuaScheduler* scheduler = timer->GetLuaScheduler();
			double requestedTime = luaL_checknumber(luaState, 1);

			luaL_checktype(luaState, 2, LUA_TFUNCTION);

			KEngineCore::Timeout* timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
			luaL_getmetatable(luaState, "KEngineCore.Timeout");
			lua_setmetatable(luaState, -2);

			KEngineCore::ScheduledLuaCallback<> callback = scheduler->CreateCallback<>(luaState, 2);
			timeout->Init(timer, requestedTime, true, callback.mCallback, callback.mCancelCallback);


			return 1;
		};

		auto clearTimeout = [](lua_State* luaState) {
			KEngineCore::Timeout* timeout = (KEngineCore::Timeout*)luaL_checkudata(luaState, 1, "KEngineCore.Timeout");
			timeout->Cancel();
			return 0;
		};

		auto wait = [](lua_State* luaState) {
			KEngineCore::TimeLibrary* timeLib = (KEngineCore::TimeLibrary*)lua_touserdata(luaState, lua_upvalueindex(1));
			KEngineCore::Timer* timer = timeLib->GetContextualObject(luaState, 2);

			KEngineCore::LuaScheduler* scheduler = timer->GetLuaScheduler();
			double requestedTime = luaL_checknumber(luaState, 1);

			KEngineCore::Timeout* timeout = new (lua_newuserdata(luaState, sizeof(KEngineCore::Timeout))) KEngineCore::Timeout;
			luaL_getmetatable(luaState, KEngineCore::Timeout::MetaName);
			lua_setmetatable(luaState, -2);

			KEngineCore::ScheduledLuaThread* scheduledThread = scheduler->GetScheduledThread(luaState);
			scheduledThread->Pause();

			timeout->Init(timer, requestedTime, false, [scheduledThread]() {
				scheduledThread->ClearCleanupCallback();
				scheduledThread->Resume();
			});

			scheduledThread->SetCleanupCallback([timeout]() {
				timeout->Cancel();
			});

			return lua_yield(luaState, 1);  //Pretend we're going to pass the timeout to resume so it doesn't get GC'd
		};

		auto deleteTimeout = [](lua_State* luaState) {
			KEngineCore::Timeout* timeout = (KEngineCore::Timeout*)luaL_checkudata(luaState, 1, KEngineCore::Timeout::MetaName);
			timeout->~Timeout();
			return 0;
		};

		const luaL_Reg timeLibrary[] = {
			{"setTimeout", setTimeout},
			{"clearTimeout", clearTimeout},
			{"setInterval", setInterval},
			{"clearInterval", clearTimeout},
			{"wait", wait},
			{nullptr, nullptr}
		};

		lua_checkstack(luaState, 3);
		luaL_newmetatable(luaState, KEngineCore::Timeout::MetaName);
		lua_pushstring(luaState, "__gc");
		lua_pushcfunction(luaState, deleteTimeout);
		lua_settable(luaState, -3);
		lua_pop(luaState, 1);

		luaL_newlibtable(luaState, timeLibrary);
		lua_pushvalue(luaState, lua_upvalueindex(1));
		luaL_setfuncs(luaState, timeLibrary, 1);
		return 1;
	};

	LuaLibraryTwo<Timer>::Init(luaState, "time", luaopen_time);
}

void KEngineCore::TimeLibrary::Deinit()
{
	LuaLibraryTwo::Deinit();
}
