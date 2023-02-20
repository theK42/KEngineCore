#pragma once
#include "boost/noncopyable.hpp"

namespace KEngineCore
{

	///------------------------------------------------------------------------
	///------------------------------------------------------------------------

	template< typename T, int N >
	class ListElement : public boost::noncopyable
	{
	public:

		ListElement() {
			for (int i = 0; i < N; i++)
			{
				mNext[i] = mPrev[i] = nullptr;
			}
		}

		template< int M >
		ListElement* GetNext() const;

		template< int M >
		ListElement* GetPrev() const;

		template< int M >
		void	SetNext(ListElement* next);

		template< int M >
		void	SetPrev(ListElement* prev);

	private:
		ListElement(const ListElement&) {}

		ListElement* mNext[N];
		ListElement* mPrev[N];
	};

	///------------------------------------------------------------------------

	template< typename T, int N > template< int M >
	inline ListElement<T, N>* ListElement< T, N >::GetNext() const
	{
		return mNext[M];
	}

	///------------------------------------------------------------------------

	template< typename T, int N > template< int M >
	inline ListElement<T, N>* ListElement< T, N >::GetPrev() const
	{
		return mPrev[M];
	}

	///------------------------------------------------------------------------

	template< typename T, int N > template< int M >
	inline void ListElement< T, N >::SetNext(ListElement<T, N>* next)
	{
		mNext[M] = next;
	}

	///------------------------------------------------------------------------

	template< typename T, int N > template< int M >
	inline void ListElement< T, N >::SetPrev(ListElement<T, N>* prev)
	{
		mPrev[M] = prev;
	}

	///------------------------------------------------------------------------
	///------------------------------------------------------------------------

	template< typename T, int M, int N>
	class List : public boost::noncopyable
	{
	public:
		List();

		void		Clear();

		bool		IsEmpty() const;

		T* GetFront() const;
		T* GetBack() const;

		void		InsertBefore(T* item, ListElement<T, N>* pAnchor);
		void		InsertAfter(T* item, ListElement<T, N>* pAnchor);
		void		Remove(T* item);
		void		RemoveIfPresent(T* item);

		void		PushFront(T* item);
		void		PushBack(T* item);

		T* PopFront();
		T* PopBack();

		//For range based for
		class iterator {
		public:
			iterator(ListElement<T, N>* element) { mElement = element; }
			T* operator*();
			iterator& operator++();
			bool operator!=(iterator const& other);
		private:
			ListElement<T, N>* mElement;
		};

		iterator		begin();
		iterator	 	end();
	private:
		ListElement<T, N>	mHead;
	};

	template <typename T, int M, int N>
	typename List<T, M, N>::iterator List<T, M, N>::begin() {
		return iterator(mHead.template GetNext<M>());
	}

	template <typename T, int M, int N>
	typename List<T, M, N>::iterator List<T, M, N>::end() {
		return iterator(&mHead);
	}

	template <typename T, int M, int N>
	bool List<T, M, N>::iterator::operator!=(iterator const& other) {
		return mElement != other.mElement;
	}


	template <typename T, int M, int N>
	typename List<T, M, N>::iterator& List<T, M, N>::iterator::operator++() {
		mElement = mElement->template GetNext<M>();
		return *this;
	}

	template <typename T, int M, int N>
	T* List<T, M, N>::iterator::operator*() {
		return static_cast<T*>(mElement);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::Clear()
	{
		auto prev = mHead.template GetPrev<M>();
		auto next = mHead.template GetNext<M>();

		prev->template SetNext<M>(next);
		next->template SetPrev<M>(prev);

		mHead.template SetNext<M>(&mHead);
		mHead.template SetPrev<M>(&mHead);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline List< T, M, N >::List()
	{
		mHead.template SetNext<M>(&mHead);
		mHead.template SetPrev<M>(&mHead);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline bool List< T, M, N >::IsEmpty() const
	{
		return mHead.template GetNext<M>() == &mHead;
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline T* List< T, M, N >::GetFront() const
	{
		return mHead.template GetNext<M>();
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline T* List< T, M, N >::GetBack() const
	{
		return mHead.template GetPrev<M>();
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::InsertBefore(T* item, ListElement<T, N>* pAnchor)
	{
		ListElement<T, N>* prev = pAnchor->template GetPrev<M>();
		ListElement<T, N>* next = pAnchor;

		prev->template SetNext<M>(item);
		next->template SetPrev<M>(item);
		item->template SetPrev<M>(prev);
		item->template SetNext<M>(next);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::InsertAfter(T* item, ListElement<T, N>* pAnchor)
	{
		ListElement<T, N>* prev = pAnchor;
		ListElement<T, N>* next = pAnchor->template GetNext<M>();

		prev->template SetNext<M>(item);
		next->template SetPrev<M>(item);
		item->template SetPrev<M>(prev);
		item->template SetNext<M>(next);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::Remove(T* item)
	{
		ListElement<T, N>* prev = item->template GetPrev<M>();
		ListElement<T, N>* next = item->template GetNext<M>();

		prev->template SetNext<M>(next);
		next->template SetPrev<M>(prev);
		item->template SetNext<M>(nullptr);
		item->template SetPrev<M>(nullptr);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::RemoveIfPresent(T* item)
	{
		ListElement<T, N>* prev = item->template GetPrev<M>();
		ListElement<T, N>* next = item->template GetNext<M>();
		if (prev != nullptr && prev->template GetNext<M>() == item && next != nullptr && next->template GetPrev<M>() == item)
		{
			prev->template SetNext<M>(next);
			next->template SetPrev<M>(prev);
			item->template SetNext<M>(nullptr);
			item->template SetPrev<M>(nullptr);
		}
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::PushFront(T* item)
	{
		InsertAfter(item, &mHead);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline void List< T, M, N >::PushBack(T* item)
	{
		InsertBefore(item, &mHead);
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline T* List< T, M, N >::PopFront()
	{
		T* pFront = GetFront();
		Remove(pFront);
		return pFront;
	}

	///------------------------------------------------------------------------

	template< typename T, int M, int N >
	inline T* List< T, M, N >::PopBack()
	{
		T* pBack = GetBack();
		Remove(pBack);
		return pBack;
	}

	///------------------------------------------------------------------------
	///------------------------------------------------------------------------

} /// namespace KEngine