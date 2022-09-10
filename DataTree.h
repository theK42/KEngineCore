#include "StringHash.h"
#include <map>

#pragma once
namespace KEngineCore
{
	class StringTable;
	class StringTableBuilder;

	class DataTreeHeader
	{
	public:
		DataTreeHeader();
		~DataTreeHeader();
		void Init(StringTable* stringTable);
		void Deinit();

		void AddInt(StringHash id);
		void AddFloat(StringHash id);
		void AddBool(StringHash id);
		void AddString(StringHash id);
		void AddHash(StringHash id);

		bool HasInt(StringHash id) const;
		bool HasFloat(StringHash id) const;
		bool HasBool(StringHash id) const;
		bool HasString(StringHash id) const;
		bool HasHash(StringHash id) const;

		int GetIntIndex(StringHash id) const;
		int GetFloatIndex(StringHash id) const;
		int GetBoolIndex(StringHash id) const;
		int GetStringIndex(StringHash id) const;
		int GetHashIndex(StringHash id) const;
		int GetKeyIndex(StringHash id) const;


		bool IsKey(StringHash id) const;  //IsKey reports whether a Hash is being used as a key by the root
		int GetNumKeys() const ;
		StringHash GetKey(int index) const;
	protected:
		typedef std::map<StringHash, int> IndexMap;
		IndexMap	mIntMap;
		IndexMap	mFloatMap;
		IndexMap	mBoolMap;
		IndexMap	mStringMap;
		IndexMap	mHashMap;
		IndexMap	mKeyMap;
#ifndef DELETE_STRINGS
		StringTable* mStringTable;
#endif

	};

	class DataTree
	{
	public:
		DataTree();
		~DataTree();

		bool HasInt(StringHash id) const;
		int GetInt(StringHash id) const;

		bool HasFloat(StringHash id) const;
		float GetFloat(StringHash id) const;

		bool HasBool(StringHash id) const;
		bool GetBool(StringHash id) const;

		bool HasString(StringHash id) const;
		std::string_view GetString(StringHash id) const;

		bool HasHash(StringHash id) const;
		StringHash GetHash(StringHash id) const;

		bool HasKey(StringHash id) const;

		int GetNumBranches() const;
		DataTree* GetBranch(int index) const;
		bool HasBranch(StringHash key, StringHash id) const;
		DataTree* GetBranch(StringHash key, StringHash id) const;

		const std::vector<DataTree*>& GetBranches() const;

	protected:

		DataTreeHeader*				mHeader{ nullptr };
		bool						mOwnsHeader{ false };
		DataTreeHeader*				mBranchHeader{ nullptr };

		StringTableBuilder*			mStringTable{ nullptr };
		bool						mOwnsStringTable{ false };

		std::vector<int>			mInts;
		std::vector<float>			mFloats;
		std::vector<uint8_t>		mBitFields;
		std::vector<size_t>			mStringIndices;
		std::vector<StringHash>		mHashes;
		std::vector<DataTree*>		mBranches;

		typedef std::map<StringHash, int> IndexMap;
		IndexMap					mKeyMap;
		std::vector<IndexMap>		mBranchMaps;
	};

	class DataSapling : public DataTree
	{
	public:
		DataSapling();
		~DataSapling();

		void Init(DataSapling* root, DataTreeHeader* header, StringTableBuilder* stringTable);
		void Deinit();

		void SetInt(StringHash id, int value);
		void SetFloat(StringHash id, float value);
		void SetBool(StringHash id, bool value);
		void SetString(StringHash id, std::string_view value);  //caution:  May leave cruft in the string table.
		void SetHash(StringHash id, StringHash value); 

		template<typename T>
		void Set(StringHash id, T value);

		DataTreeHeader* CreateBranchHeader();

		void AddKey(StringHash id);

		//DataSapling* GetBranch(int index) const;
		//DataSapling* GetBranch(StringHash key, StringHash id) const;


		template<typename T, typename ... Targs>
		DataSapling* GrowBranch(std::pair<KEngineCore::StringHash, T>, Targs ... args); //Recursive variadic template
		DataSapling* GrowBranch();
		void BranchReady(DataSapling * branch);

		void PruneBranch(int index);
		void PruneBranch(StringHash key, StringHash id);

		void RebuildStringTable(); //May only be called by the root and owner of the string table.

	protected:
		template<typename T, typename ... Targs>
		void GrowBranchInternal(DataSapling* branch, std::pair<KEngineCore::StringHash, T> v1, Targs ... args);
		void GrowBranchInternal(DataSapling* branch) {};//Recursive base case, do nothing;
		StringHash TabulateStringHash(StringHash hash);
		

		DataSapling *			mRoot;


		std::vector<DataSapling*>	mSaplingBranches;
	};


	template<typename T>
	void DataSapling::Set(StringHash id, T value)
	{
		static_assert(false, "Unsupported type Set on DataTree");
	}

	template<>
	inline void DataSapling::Set<StringHash>(StringHash id, StringHash value)
	{
		SetHash(id, value);
	}

	template<typename T, typename ... Targs>
	DataSapling * DataSapling::GrowBranch(std::pair<KEngineCore::StringHash, T>v1, Targs ... args)
	{
		DataSapling* branch = new DataSapling();
		branch->Init(this, mBranchHeader, mStringTable);
		branch->Set(v1.first, v1.second);
		GrowBranchInternal(branch, args...);
		int branchIndex = (int)mBranches.size();
		mBranches.push_back(branch);
		mSaplingBranches.push_back(branch);
		for (auto keyPair : mKeyMap)
		{
			auto keyVal = branch->GetHash(keyPair.first);
			mBranchMaps[keyPair.second][keyVal] = branchIndex;
		}
		return branch;
	}

	template<typename T, typename ... Targs>
	void DataSapling::GrowBranchInternal(DataSapling* branch, std::pair<KEngineCore::StringHash, T>v1, Targs ... args)
	{
		branch->Set(v1.first, v1.second);
		GrowBranchInternal(branch, args...);
	}
	
#define LEAF(key, value) (std::pair(key, value))

}
