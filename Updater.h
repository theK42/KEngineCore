#pragma once
#pragma once

#include <vector>
#include <cassert>


namespace KEngineCore {

	template <class TClass>
	class Updater;

	///------------------------------------------------------------------------
	///------------------------------------------------------------------------

	template <class TClass>
	class Updating
	{
	public:
		Updating();
		~Updating();

		void Deinit();

		void Start();
		void Stop();

		///Gets the internal updatable object
		TClass & Get();

	protected:
		///Init is protected because most template instances will need to override it with more parameters.
		void Init(Updater<TClass> * updater);

		///Only the associated Updater should call Update
		void Update(double time);

		TClass				mUpdatable;
        Updater<TClass> *	mUpdater {nullptr};
        bool				mUpdating {false};

		friend class Updater<TClass>;
	};

	///------------------------------------------------------------------------

	template <class TClass>
	Updating<TClass>::Updating()
	{
		mUpdater = nullptr;
		mUpdating = false;
	}

	///------------------------------------------------------------------------

	template <class TClass>
	Updating<TClass>::~Updating()
	{
		Deinit();
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updating<TClass>::Init( Updater<TClass> * updater )
	{
		assert(updater != nullptr);
		assert(mUpdater == nullptr);  ///Don't allow double initialization
		mUpdater = updater;
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updating<TClass>::Deinit()
	{
		Stop();
		mUpdater = nullptr;
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updating<TClass>::Start()
	{
		assert(mUpdater != nullptr); ///Initialized
		if (!mUpdating)
		{
			mUpdater->AddToUpdateList(this);
			mUpdating = true;
		}
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updating<TClass>::Stop()
	{
		assert(mUpdater != nullptr); ///Initialized
		if (mUpdating)
		{
			mUpdater->RemoveFromUpdateList(this);
			mUpdating = false;
		}
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updating<TClass>::Update( double fTime )
	{
		assert(mUpdater != nullptr); ///Initialized
		assert(mUpdating);
		mUpdatable.Update(fTime);
	}

	///------------------------------------------------------------------------

	template <class TClass>
	TClass & Updating<TClass>::Get()
	{
		return mUpdatable;
	}

	///------------------------------------------------------------------------
	///------------------------------------------------------------------------

	template <class TClass>
	class Updater
	{
	public:
		Updater();
		~Updater();

		void Init();
		void Deinit();

		void Update(double fTime);

	protected:
		void AddToUpdateList(Updating<TClass> * updatable);
		void RemoveFromUpdateList(Updating<TClass> * updatable);

		std::vector<Updating<TClass> *>	mUpdateList;
		bool							mInitialized;

		friend class Updating<TClass>;
	};

	///------------------------------------------------------------------------

	template <class TClass>
	Updater<TClass>::Updater()
	{
		mInitialized = false;
	}

	///------------------------------------------------------------------------

	template <class TClass>
	Updater<TClass>::~Updater()
	{
		Deinit();
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updater<TClass>::Init()
	{
		if (!mInitialized)
		{
			mInitialized = true;
		}
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updater<TClass>::Deinit()
	{
		if (mInitialized)
		{
			assert(mUpdateList.empty()); /// The updater is a dependency for any updating objects, must ensure they are cleaned up first
			mInitialized = false;
		}
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updater<TClass>::Update(double fTime)
	{
		assert(mInitialized);
        for (auto * updateable: mUpdateList)
        {
            updateable->Update(fTime);
        }
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updater<TClass>::AddToUpdateList( Updating<TClass> * updatable )
	{
		assert(mInitialized);
		mUpdateList.push_back(updatable);
	}

	///------------------------------------------------------------------------

	template <class TClass>
	void Updater<TClass>::RemoveFromUpdateList( Updating<TClass> * updatable )
	{
		assert(mInitialized);
        mUpdateList.erase(remove(mUpdateList.begin(), mUpdateList.end(), updatable));
	}

	///------------------------------------------------------------------------
}
