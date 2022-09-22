#pragma once

#include <vector>
#include <list>
#include <assert.h>


namespace KEngineCore
{

	template <typename T>
	class Pool
	{
	public:
		Pool();
		~Pool();

		void Init(int startingSize = 1, int maxSize = -1);
		void Deinit();

		T* GetItem();
		void ReleaseItem(T* item);
		void Clear();

	private:
		void Resize(int newSize);

		int mPoolSize{ 0 };
		int mMaxPoolSize{ -1 };
		std::vector<T*> mPools;
		std::list<T*> mFreeList;

	};


	template <typename T>
	Pool<T>::Pool()
	{
	}

	template <typename T>
	Pool<T>::~Pool()
	{
		Deinit();
	}

	template <typename T>
	void Pool<T>::Init(int startingSize, int maxSize)
	{
		assert(mPoolSize == 0);
		mMaxPoolSize = maxSize;
		Resize(startingSize);
	}

	template <typename T>
	void Pool<T>::Deinit()
	{
		assert(mFreeList.size() == mPoolSize);
		Clear();
	}

	template <typename T>
	void Pool<T>::Clear()
	{
		mFreeList.clear();
		for (auto pool : mPools)
		{
			delete[] pool;
		}
		mPools.clear();
		mPoolSize = 0;
	}

	template <typename T>
	T* Pool<T>::GetItem()
	{
		if (mFreeList.empty())
		{
			int newSize = mPoolSize * 2;
			if (mMaxPoolSize > 0)
			{
				assert(mMaxPoolSize > mPoolSize);
				mPoolSize = std::min(mPoolSize, mMaxPoolSize);
			}
			Resize(newSize);
		}

		T* item = mFreeList.front();
		mFreeList.pop_front();
		return item;
	}

	template <typename T>
	void Pool<T>::ReleaseItem(T* item)
	{
		mFreeList.push_back(item);
	}

	template <typename T>
	void Pool<T>::Resize(int newSize)
	{
		assert(newSize > 0 && (mMaxPoolSize < 0 || newSize <= mMaxPoolSize));
		int toAlloc = newSize - mPoolSize;
		T* newPool = new T[toAlloc];
		mPools.push_back(newPool);
		for (int i = 0; i < toAlloc; i++) {
			mFreeList.push_back(&newPool[i]);
		}
		mPoolSize = newSize;
	}

}