#include "StringHash.h"
#include <map>

#pragma once
namespace KEngineCore
{
	class StringTable;


	typedef std::map<StringHash, int> IndexMap;
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

		int GetNumInts() const;
		int GetNumFloats() const;
		int GetNumBools() const;
		int GetNumStrings() const;
		int GetNumHashes() const;

		StringTable* GetStringTable() const;
		StringHash TabulateStringHash(StringHash hash);

		int GetKeyIndex(StringHash id) const;


		bool IsKey(StringHash id) const;  //IsKey reports whether a Hash is being used as a key by the root
		int GetNumKeys() const ;
		StringHash GetKey(int index) const;


		void WriteToStream(std::ostream& stream) const;
		void ReadFromStream(std::istream& stream);

	protected:
		IndexMap	mIntMap;
		IndexMap	mFloatMap;
		IndexMap	mBoolMap;
		IndexMap	mStringMap;
		IndexMap	mHashMap;
		
		StringTable*	mStringTable;
		bool			mOwnsStringTable;
	};

	class DataTree
	{
	public:
		DataTree();
		~DataTree();

		void Init(DataTreeHeader* header);
		void Deinit();

		bool HasInt(StringHash id) const;
		int GetInt(StringHash id) const;
		int GetInt(int index) const;
		int GetNumInts()const;

		bool HasFloat(StringHash id) const;
		float GetFloat(StringHash id) const;
		float GetFloat(int index) const;
		int GetNumFloats()const;

		bool HasBool(StringHash id) const;
		bool GetBool(StringHash id) const;
		bool GetBool(int index) const;
		int GetNumBools()const;

		bool HasString(StringHash id) const;
		std::string_view GetString(StringHash id) const;
		std::string_view GetString(int index) const;
		int GetNumStrings()const;

		bool HasHash(StringHash id) const;
		StringHash GetHash(StringHash id) const;
		StringHash GetHash(int index) const;
		int GetNumHashes()const;

		bool HasKey(StringHash id) const;

		int GetNumBranches() const;
		DataTree* GetBranch(int index) const;
		bool HasBranch(StringHash key, StringHash id) const;
		DataTree* GetBranch(StringHash key, StringHash id) const;

		const std::vector<DataTree*>& GetBranches() const;

		void WriteToStream(std::ostream& stream) const;
		void ReadFromStream(std::istream& stream, StringTable* strings = nullptr);

	protected:

		DataTreeHeader*				mHeader{ nullptr };
		bool						mOwnsHeader{ false };
		DataTreeHeader*				mBranchHeader{ nullptr };

		std::vector<int>			mInts;
		std::vector<float>			mFloats;
		std::vector<uint8_t>		mBitFields;
		std::vector<size_t>			mStringIndices;
		std::vector<StringHash>		mHashes;
		std::vector<DataTree*>		mBranches;

		IndexMap					mKeyMap;
		std::vector<IndexMap>		mBranchMaps;
	};

	class DataSapling : public DataTree
	{
	public:
		DataSapling();
		~DataSapling();

		void Init(DataSapling* root, DataTreeHeader* header, StringTable * strings);
		void Deinit();

		void SetInt(StringHash id, int value);
		void AddInt(int value);
		void SetFloat(StringHash id, float value);
		void AddFloat(float value);
		void SetBool(StringHash id, bool value);
		void AddBool(bool value);
		void SetString(StringHash id, std::string_view value);  //caution:  May leave cruft in the string table.
		void AddString(std::string_view value);
		void SetHash(StringHash id, StringHash value);
		void AddHash(StringHash value);

		DataTreeHeader* CreateBranchHeader();

		void AddKey(StringHash id);
		DataSapling* GrowBranch();
		void BranchReady(DataSapling * branch);

		void PruneBranch(int index);
		void PruneBranch(StringHash key, StringHash id);

		void RebuildStringTable(); //May only be called by the root and owner of the string table.

	protected:
		

		DataSapling *			mRoot;


		std::vector<DataSapling*>	mSaplingBranches;
	};

}
