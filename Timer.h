#pragma once

#include "LuaLibrary.h"
#include <functional>
#include <list>
#include <vector>

namespace KEngineCore {

class LuaScheduler;
class Timeout; 
class TimeForwarder;


class Timer
{
public:
	Timer();
	~Timer();

	void Init(LuaScheduler * scheduler);
	void Deinit();

	void Update(double time);

	void Pause();
	void Resume();

	void SetSpeed(double speed);
	double GetSpeed() const;

	LuaScheduler * GetLuaScheduler() const;

private:
	void AddTimeout(Timeout * timeout);
	void ClearTimeout(Timeout * timeout);

	void AddTimeForwarder(TimeForwarder* tc);
	void RemoveTimeForwarder(TimeForwarder* tc);

    LuaScheduler *					mScheduler {nullptr};
	std::list<Timeout *>			mTimeouts;
	std::list<TimeForwarder*>		mForwarders;

    double					mCurrentTime {0.0};
    Timeout *				mProcessingTimeout {nullptr};
	bool					mRunning {false};
	double					mSpeed {1.0};
	friend class Timeout;
	friend class TimeForwarder;
};

class TimeLibrary : public LuaLibraryTwo<Timer>
{
public:
	TimeLibrary();
	~TimeLibrary();
	void Init(lua_State* luaState);
	void Deinit();
};

class Timeout 
{
public:
	Timeout();
	~Timeout();

	void Init(Timer * timer, double time, bool repeats, std::function<void ()> callback, std::function<void()> cancelCallback = nullptr); //templatize later for memory management  //What did I mean by this comment?
	void Deinit();

	void TimeElapsed();
	void Cancel(bool batched = false);

	static const char MetaName[];
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

class TimeForwarder
{
public:
	TimeForwarder();
	~TimeForwarder();

	void Init(Timer* timer, std::function<void(double)> callback);
	void Deinit(bool batched = false);
	void Update(double deltaTime);
private:
	Timer*								mTimer{ nullptr };
	std::list<TimeForwarder*>::iterator	mPosition;
	std::function<void(double)>			mCallback{ nullptr };
	friend class Timer;

};

}