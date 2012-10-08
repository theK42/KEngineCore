#pragma once

#include "LuaLibrary.h"
#include <functional>
#include <list>
#include <vector>

namespace KEngineCore {

class LuaScheduler;
class Timeout;

class Timer : protected LuaLibrary
{
public:
	Timer(void);
	~Timer(void);

	void Init(LuaScheduler * scheduler);
	void Deinit();
	
	void RegisterLibrary(lua_State * luaState, char const * name = "timer") override;

	void Update(double time);
	LuaScheduler * GetLuaScheduler() const;

private:
	void AddTimeout(Timeout * timeout);
	void ClearTimeout(Timeout * handler);

	LuaScheduler *			mScheduler;
	std::list<Timeout *>	mTimeouts;
	double					mCurrentTime;
	Timeout *				mProcessingTimeout;
	friend class Timeout;
};

class Timeout 
{
public:
	Timeout();
	~Timeout();

	void Init(Timer * timer, double time, bool repeats, std::function<void ()> callback, std::function<void()> cancelCallback = nullptr); //templatize later for memory management
	void Deinit();

	void TimeElapsed();
	void Cancel();

private:
	Timer *							mTimer;
	double							mSetTime;
	int								mFiresAt;
	bool							mRepeats;
	std::list<Timeout *>::iterator	mPosition;
	std::function<void()>			mCallback;
	std::function<void()>			mCancelCallback;
	friend class Timer;
};

}