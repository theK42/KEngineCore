#pragma once
#include "LuaLibrary.h"
#include "Timer.h"
#include "Psychopomp.h"
#include <vector>

namespace KEngineCore
{	
	class TweenSystem;
	
	class Tween
	{
	public:
		Tween() {}
		virtual ~Tween() { Deinit(); }
		void Init(TweenSystem* tweenSystem);
		virtual void Deinit();
		bool Update(double deltaTime);
		virtual double GetDuration() { return 1.0; };
		virtual bool SetTime(double time);
		static const char MetaName[];
	protected:
		double mTime{ 0.0 };
		TweenSystem* mSystem{ nullptr };
	};
	
	template<typename T>
	class TweenTo : public Tween
	{
	public:
		TweenTo() {}
		virtual ~TweenTo() override { Deinit(); }
		void Init(TweenSystem* tweenSystem, T* target, T end, void* callbackKey = nullptr, std::function<void()> changeCallback = nullptr);
		virtual bool SetTime(double time) override;
		void Deinit();
	protected:
		T* mTarget;
		T mStart;
		T mEnd;
		void* mCallbackKey;
		std::function<void()> mChangeCallback;
	};
	
	class TweenGroup : public Tween
	{
	public:
		TweenGroup();
		virtual ~TweenGroup() override;
		void Init(TweenSystem* tweenSystem);
		void Deinit();
	
	
		void AddTween(Tween* tween);
		virtual double GetDuration() override;
		virtual bool SetTime(double time) override;
	protected:
		std::vector<Tween*> mTweens;
	};
	
	class TweenEase : public TweenGroup
	{
	public:
		TweenEase();
		virtual ~TweenEase() override {}
		enum EaseFunc
		{
			EaseIn,
			EaseOut,
			EaseInOut
		};
		void Init(TweenSystem* tweenSystem, EaseFunc func);
		virtual bool SetTime(double time) override;
	protected:
		EaseFunc mEaseFunc;
		//In:	1 - Math.cos((x * Math.PI) / 2);
		//Out: Math.sin((x * Math.PI) / 2);
		//InOut:  -(Math.cos(Math.PI * x) - 1) / 2;
	};
	
	class TweenDuration : public TweenGroup
	{
	public:
		TweenDuration();
		virtual ~TweenDuration() override {};
	
		void Init(TweenSystem* tweenSystem, double duration);
		virtual double GetDuration() override;
		virtual bool SetTime(double time) override;
	protected:
		double mDuration;
	};
	
	class TweenSequence : public TweenGroup
	{
	public:
		TweenSequence();
		virtual ~TweenSequence();
		void Init(TweenSystem* tweenSystem);
		virtual double GetDuration() override;
		virtual bool SetTime(double time) override;
	protected:
	};
	
	
	class TweenSystem : private LuaLibrary
	{
	public:
		void Init(LuaScheduler* scheduler);
		void Deinit();
		virtual void RegisterLibrary(lua_State* luaState, char const* name = "tweening") override;
		
		void Run(Tween* tween, std::function<void()> callback = nullptr);
	
		void QueueCallback(void* key, std::function<void()> callback);
	
		void Update(double deltaTime);
	
	private:
		static void GatherTweens(lua_State* luaState, int startIndex, int endIndex, TweenSystem* tweenSystem, TweenGroup* tweenGroup);
	
		LuaScheduler*	mScheduler{ nullptr };
		Psychopomp		mCallbackQueue;
	
		std::vector<Tween*> mRunningTweens;
		std::map<Tween*, std::function<void()>> mCallbacks;
	};
	
	template<typename T>
	inline void TweenTo<T>::Init(TweenSystem* tweenSystem, T* target, T end, void* callbackKey, std::function<void()> changeCallback)
	{
		Tween::Init(tweenSystem);
		mTarget = target;
		mStart = *target;
		mEnd = end;
		mCallbackKey = callbackKey;
		mChangeCallback = changeCallback;
	}
	template<typename t>
	inline void TweenTo<t>::Deinit()
	{
		mTarget = nullptr;
		mCallbackKey = nullptr;
		mChangeCallback = nullptr;
	}
	template<typename T>
	inline bool TweenTo<T>::SetTime(double time)
	{
		if (Tween::SetTime(time))
		{
			*mTarget = mStart + (mTime * (mEnd - mStart));
			if (mChangeCallback && mCallbackKey)
			{
				mSystem->QueueCallback(mCallbackKey, mChangeCallback);
			}
			return true;
		} 
		else
		{
			return false;
		}
	}
}