#include "Tweening.h"
#include "LuaScheduler.h"
#include <numbers>
#include <assert.h>

const char KEngineCore::Tween::MetaName[] = "KEngineCore.Tween";

void KEngineCore::Tween::Init(TweenSystem* tweenSystem)
{
	assert(mSystem == nullptr);
	mTime = 0.0;
	mSystem = tweenSystem;
}

void KEngineCore::Tween::Deinit()
{
	mSystem = nullptr;
}

bool KEngineCore::Tween::Update(double deltaTime)
{
	return SetTime(mTime + deltaTime);
}

bool KEngineCore::Tween::SetTime(double time)
{
	double newTime = std::min(GetDuration(), time);
	if (newTime != mTime)
	{
		mTime = newTime;
		return true;
	}
	else
	{
		return false;
	}
}

KEngineCore::TweenGroup::TweenGroup()
{
}

KEngineCore::TweenGroup::~TweenGroup()
{
	Deinit();
}

void KEngineCore::TweenGroup::Init(TweenSystem* tweenSystem)
{
	Tween::Init(tweenSystem);
}

void KEngineCore::TweenGroup::Deinit()
{
	mTweens.clear();
}

bool KEngineCore::TweenGroup::SetTime(double time)
{
	if (Tween::SetTime(time))
	{
		//Default behavior
		for (auto* tween : mTweens)
		{
			tween->SetTime(mTime);
		}
		return true;
	}
	else
	{
		return false;
	}
}

void KEngineCore::TweenGroup::AddTween(Tween* tween)
{
	mTweens.push_back(tween);
}

double KEngineCore::TweenGroup::GetDuration()
{
	//Default behavior
	double max = 0.0;
	for (auto* tween : mTweens)
	{
		max = std::max(tween->GetDuration(), max);
	}
	return max;
}

KEngineCore::TweenEase::TweenEase()
{
}

void KEngineCore::TweenEase::Init(TweenSystem* tweenSystem, EaseFunc func)
{
	TweenGroup::Init(tweenSystem);
	mEaseFunc = func;
}

bool KEngineCore::TweenEase::SetTime(double time)
{
	if (Tween::SetTime(time))
	{
		float duration = GetDuration();
		double modifiedT = duration != 0.0 ? mTime / GetDuration() : 0.0;
		switch (mEaseFunc)
		{
		case EaseIn:
			modifiedT = 1 - std::cos(modifiedT * std::numbers::pi / 2.0);
			break;
		case EaseOut:
			modifiedT = std::sin(modifiedT * std::numbers::pi / 2.0);
			break;
		case EaseInOut:
			modifiedT = -(std::cos(modifiedT * std::numbers::pi) - 1.0) / 2.0;
			break;
		}
		modifiedT *= duration;

		for (auto* tween : mTweens)
		{
			tween->SetTime(modifiedT);
		}
		return true;
	}
	else
	{
		return false;
	}
}

KEngineCore::TweenDuration::TweenDuration()
{
}

void KEngineCore::TweenDuration::Init(TweenSystem* tweenSystem, double duration)
{
	TweenGroup::Init(tweenSystem);
	mDuration = duration;
}

double KEngineCore::TweenDuration::GetDuration()
{
	return mDuration;
}

bool KEngineCore::TweenDuration::SetTime(double time)
{
	if (Tween::SetTime(time))
	{
		float ratio = mTime / mDuration;
		for (auto* tween : mTweens)
		{
			tween->SetTime(ratio * tween->GetDuration());
		}
		return true;
	}
	else
	{
		return false;
	}
}

KEngineCore::TweenSequence::TweenSequence()
{
}

KEngineCore::TweenSequence::~TweenSequence()
{
	Deinit();
}

void KEngineCore::TweenSequence::Init(TweenSystem* tweenSystem)
{
	TweenGroup::Init(tweenSystem);
}

double KEngineCore::TweenSequence::GetDuration()
{
	double duration = 0.0;
	for (auto* tween : mTweens)
	{
		duration += tween->GetDuration();
	}
	return duration;
}

bool KEngineCore::TweenSequence::SetTime(double time)
{
	for (auto* tween : mTweens)
	{
		double duration = tween->GetDuration();
		if (duration < time)
		{
			time -= duration;
			tween->SetTime(duration);
		}
		else
		{
			return tween->SetTime(time);
		}
	}
	return false;
}

void KEngineCore::TweenSystem::Init(KEngineCore::LuaScheduler* scheduler)
{
	assert(mScheduler == nullptr);
	mScheduler = scheduler;
	RegisterLibrary(scheduler->GetMainState());
}

void KEngineCore::TweenSystem::Deinit()
{
	//TODO:  Possibly cancel some things?
	mScheduler = nullptr;
}

void KEngineCore::TweenSystem::Run(Tween* tween, std::function<void()> callback)
{
	mRunningTweens.push_back(tween);
	if (callback)
	{
		mCallbacks[tween] = callback;
	}
}

void KEngineCore::TweenSystem::Update(double time)
{
	std::vector<Tween*> completedTweens;
	for (auto* tween : mRunningTweens)
	{
		if (!tween->Update(time))
		{
			completedTweens.push_back(tween);
		}
	}
	mCallbackQueue.Update();	
	for (auto* tween : completedTweens)
	{
		mRunningTweens.erase(std::remove(mRunningTweens.begin(), mRunningTweens.end(), tween), mRunningTweens.end());
		if (mCallbacks.find(tween) != mCallbacks.end())
		{
			mCallbacks[tween]();
			mCallbacks.erase(tween);
		}
	}
	completedTweens.clear();
}

void KEngineCore::TweenSystem::GatherTweens(lua_State* luaState, int startIndex, int endIndex, TweenSystem* tweenSystem, TweenGroup* tweenGroup)
{
	for (int i = startIndex; i <= endIndex; i++)
	{
		Tween* tween = (Tween*)luaL_checkudata(luaState, i, Tween::MetaName);
		tweenGroup->AddTween(tween);
	}
}

void KEngineCore::TweenSystem::RegisterLibrary(lua_State* luaState, char const* name)
{
	auto luaopen_tweening = [](lua_State* luaState) {

		auto createGroup = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenGroup* tweenGroup = new (lua_newuserdata(luaState, sizeof(TweenGroup))) TweenGroup; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			tweenGroup->Init(tweenSystem);
			GatherTweens(luaState, 1, argumentCount, tweenSystem, tweenGroup);
			return 1;
		};

		auto createDuration = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenDuration* tweenDuration = new (lua_newuserdata(luaState, sizeof(TweenDuration))) TweenDuration; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			double duration = luaL_checknumber(luaState, 1);
			tweenDuration->Init(tweenSystem, duration);
			GatherTweens(luaState, 2, argumentCount, tweenSystem, tweenDuration);
			return 1;
		};

		auto createSequence = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenSequence* tweenSequence = new (lua_newuserdata(luaState, sizeof(TweenSequence))) TweenSequence; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			tweenSequence->Init(tweenSystem);
			GatherTweens(luaState, 1, argumentCount, tweenSystem, tweenSequence);
			return 1;
		};

		auto createEaseIn = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenEase* tweenSequence = new (lua_newuserdata(luaState, sizeof(TweenEase))) TweenEase; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			tweenSequence->Init(tweenSystem, TweenEase::EaseIn);
			GatherTweens(luaState, 1, argumentCount, tweenSystem, tweenSequence);
			return 1;
		};

		auto createEaseOut = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenEase* tweenSequence = new (lua_newuserdata(luaState, sizeof(TweenEase))) TweenEase; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			tweenSequence->Init(tweenSystem, TweenEase::EaseOut);
			GatherTweens(luaState, 1, argumentCount, tweenSystem, tweenSequence);
			return 1;
		};

		auto createEaseInOut = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			LuaScheduler* scheduler = tweenSystem->mScheduler;
			int argumentCount = lua_gettop(luaState);
			TweenEase* tweenSequence = new (lua_newuserdata(luaState, sizeof(TweenEase))) TweenEase; // Do we actually want to GARBAGE COLLECT these?  Pool?
			luaL_getmetatable(luaState, "KEngineCore.Tween");
			lua_setmetatable(luaState, -2);
			tweenSequence->Init(tweenSystem, TweenEase::EaseInOut);
			GatherTweens(luaState, 1, argumentCount, tweenSystem, tweenSequence);
			return 1;
		};

		auto run = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			Tween* tween = (Tween*)luaL_checkudata(luaState, 1, Tween::MetaName); 
			tweenSystem->mRunningTweens.push_back(tween);
			return 0;
		};


		auto runAndWait = [](lua_State* luaState) {
			TweenSystem* tweenSystem = (TweenSystem*)lua_touserdata(luaState, lua_upvalueindex(1));
			Tween* tween = (Tween*)luaL_checkudata(luaState, 1, Tween::MetaName);
			LuaScheduler* scheduler = tweenSystem->mScheduler;

			KEngineCore::ScheduledLuaThread* scheduledThread = scheduler->GetScheduledThread(luaState);
			scheduledThread->Pause();

			tweenSystem->Run(tween, [scheduledThread]() {
				scheduledThread->Resume();
			});
			return lua_yield(luaState, 0);
		};

		const luaL_Reg tweeningLibrary[] = {
			{"createGroup", createGroup},
			{"createDuration", createDuration},
			{"createSequence", createSequence},
			{"createEaseIn", createEaseIn},
			{"createEaseOut", createEaseOut},
			{"createEaseInOut", createEaseInOut},
			{"run", run},
			{"runAndWait", runAndWait},
			{nullptr, nullptr}
		};

		CreateGCMetaTableForClass<Tween, Tween::MetaName>(luaState);

		luaL_newlibtable(luaState, tweeningLibrary);
		lua_pushvalue(luaState, lua_upvalueindex(1));
		luaL_setfuncs(luaState, tweeningLibrary, 1);
		return 1;
	};


	PreloadLibrary(luaState, name, luaopen_tweening);
}



void KEngineCore::TweenSystem::QueueCallback(void* key, std::function<void()> callback)
{	
	mCallbackQueue.ScheduleAppointment(key, callback);
}
