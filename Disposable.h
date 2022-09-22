#pragma once
#include <set>
#include "Pool.h"
namespace KEngineCore {

	class Disposable
	{
	public:
		Disposable() {}
		virtual ~Disposable() {};

		void Init() {}
		virtual void Deinit() = 0;
	};

	class DisposableGroup : public Disposable
	{
	public:
		DisposableGroup();
		virtual ~DisposableGroup();

		void Init();
		virtual void Deinit() override;
		void AddDisposable(Disposable* member);
	private:
		std::set<Disposable*> mMembers;

	};

	template<typename T>
	class RecyclingPool {
	public:
		RecyclingPool() {}
		~RecyclingPool() { Deinit(); }

		void Init(int startingSize = 1, int maxSize = -1);
		void Deinit();

		T* GetItem(DisposableGroup * recyclingGroup);
	private:
		struct Recyclable : public Disposable {
			virtual void Deinit() override;
			T* mWrappedObject;
			RecyclingPool<T>* mPool;
		};

		Pool<Recyclable>	mRecyclables;
		Pool<T>				mInternalPool;
		friend struct Recyclable;
	};

	template <typename T>
	void RecyclingPool<T>::Init(int startingSize, int maxSize)
	{
		mRecyclables.Init(startingSize, maxSize);
		mInternalPool.Init(startingSize, maxSize);
	}

	template <typename T>
	void RecyclingPool<T>::Deinit()
	{
		mRecyclables.Deinit();
		mInternalPool.Deinit();
	}


	template <typename T>
	T*  RecyclingPool<T>::GetItem(DisposableGroup* recyclingGroup)
	{
		Recyclable * recyclable = mRecyclables.GetItem();
		T* wrappedObject = mInternalPool.GetItem();
		recyclable->mWrappedObject = wrappedObject;
		recyclable->mPool = this;
		recyclingGroup->AddDisposable(recyclable);
		return wrappedObject;
	}

	template<typename T>
	void RecyclingPool<T>::Recyclable::Deinit() {
		mWrappedObject->Deinit();
		mPool->mInternalPool.ReleaseItem(mWrappedObject);
		mPool->mRecyclables.ReleaseItem(this);
	}
};