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

    LuaScheduler *			mScheduler {nullptr};
	std::list<Timeout *>	mTimeouts;
    double					mCurrentTime {0.0};
    Timeout *				mProcessingTimeout {nullptr};
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
	Timer*							mTimer{ nullptr };
    double							mSetTime {0.0};
    int								mFiresAt {0};
    bool							mRepeats {false};
	std::list<Timeout *>::iterator	mPosition;
    std::function<void()>			mCallback {nullptr};
    std::function<void()>			mCancelCallback {nullptr};
	friend class Timer;
};

}
