#include "DataTree.h"
#include "StringTable.h"
#include <assert.h>

KEngineCore::DataTreeHeader::DataTreeHeader()
{
}

KEngineCore::DataTreeHeader::~DataTreeHeader()
{
}

void KEngineCore::DataTreeHeader::Init(StringTable* stringtable)
{
#ifndef DELETE_STRINGS
	mStringTable = stringtable;
#endif
}

void KEngineCore::DataTreeHeader::Deinit()
{
	mIntMap.clear();
	mFloatMap.clear();
	mBoolMap.clear();
	mStringMap.clear();
	mHashMap.clear();
	mKeyMap.clear();
}

void KEngineCore::DataTreeHeader::AddInt(StringHash id)
{

	mIntMap[id] = (int)mIntMap.size();
}

void KEngineCore::DataTreeHeader::AddFloat(StringHash id)
{
	mIntMap[id] = (int)mFloatMap.size();
}

void KEngineCore::DataTreeHeader::AddBool(StringHash id)
{
	mIntMap[id] = (int)mBoolMap.size();
}

void KEngineCore::DataTreeHeader::AddString(StringHash id)
{
	mIntMap[id] = (int)mStringMap.size();
}

void KEngineCore::DataTreeHeader::AddHash(StringHash id)
{
	mHashMap[id] = (int)mHashMap.size();
}

bool KEngineCore::DataTreeHeader::HasInt(StringHash id) const
{
	return mIntMap.find(id) != mIntMap.end();
}

bool KEngineCore::DataTreeHeader::HasFloat(StringHash id) const
{
	return mFloatMap.find(id) != mFloatMap.end();
}

bool KEngineCore::DataTreeHeader::HasBool(StringHash id) const
{
	return mBoolMap.find(id) != mBoolMap.end();
}

bool KEngineCore::DataTreeHeader::HasString(StringHash id) const
{
	return mStringMap.find(id) != mStringMap.end();
}

bool KEngineCore::DataTreeHeader::HasHash(StringHash id) const
{
	return mHashMap.find(id) != mHashMap.end();
}

int KEngineCore::DataTreeHeader::GetIntIndex(StringHash id) const
{
	return mIntMap.find(id)->second;
}

int KEngineCore::DataTreeHeader::GetFloatIndex(StringHash id) const
{
	return mFloatMap.find(id)->second;
}

int KEngineCore::DataTreeHeader::GetBoolIndex(StringHash id) const
{
	return mBoolMap.find(id)->second;
}

int KEngineCore::DataTreeHeader::GetStringIndex(StringHash id) const
{
	return mStringMap.find(id)->second;
}

int KEngineCore::DataTreeHeader::GetHashIndex(StringHash id) const
{
	return mHashMap.find(id)->second;
}




KEngineCore::DataSapling::DataSapling()
{

}

KEngineCore::DataSapling::~DataSapling()
{
	Deinit();
}

void KEngineCore::DataSapling::Init(DataSapling* root, DataTreeHeader* header, StringTableBuilder* stringTable)
{
	mRoot = root;

	if (stringTable == nullptr)
	{
		mStringTable = new StringTableBuilder();
		mStringTable->Init();
		mOwnsStringTable = true;
	}
	else
	{
		mStringTable = stringTable;
		mOwnsStringTable = false;
	}

	if (header == nullptr)
	{
		mHeader = new DataTreeHeader();
		mHeader->Init(mStringTable);
		mOwnsHeader = true;
	}
	else
	{
		mHeader = header;
		mOwnsHeader = false;
	}

}

void KEngineCore::DataSapling::Deinit()
{
	for (auto * branch : mBranches)
	{
		delete branch;
	}
	mBranches.clear();
	mInts.clear();
	mFloats.clear();
	mBitFields.clear();
	mStringIndices.clear();
	mHashes.clear();
	mKeyMap.clear();
	mBranchMaps.clear(); 
	
	if (mOwnsHeader)
	{
		delete mHeader;
		mHeader = nullptr;
		mOwnsHeader = false;
	}

	if (mOwnsStringTable)
	{
		delete mStringTable;
		mStringTable = nullptr;
		mOwnsStringTable = false;
	}
}

void KEngineCore::DataSapling::SetInt(StringHash id, int value)
{
	id = TabulateStringHash(id);
	if (mOwnsHeader && !HasInt(id))
	{
		mHeader->AddInt(id);
		mInts.push_back(value);
		return;
	}
	else
	{
		assert(HasInt(id));
		int index = mHeader->GetIntIndex(id);
		mInts[index] = value;
	}
}

void KEngineCore::DataSapling::SetFloat(StringHash id, float value)
{
	id = TabulateStringHash(id);
	if (mOwnsHeader && !HasFloat(id))
	{
		mHeader->AddFloat(id);
		mFloats.push_back(value);
		return;
	}
	else
	{
		assert(HasFloat(id));
		int index = mHeader->GetFloatIndex(id);
		mFloats[index] = value;
	}
}

void KEngineCore::DataSapling::SetBool(StringHash id, bool value)
{
	id = TabulateStringHash(id);
	if (mOwnsHeader && !HasBool(id))
	{
		mHeader->AddBool(id);
	}
	assert(HasBool(id));

	int index = mHeader->GetBoolIndex(id);
	int interBitFieldIndex = index / 8;
	int intraBitFieldIndex = index % 8;
	if (mBitFields.size() <= interBitFieldIndex)
	{
		mBitFields.push_back(0);
	}
	mBitFields[interBitFieldIndex] = value ? (mBitFields[interBitFieldIndex] | (1 << intraBitFieldIndex)) : (mBitFields[interBitFieldIndex] & ~(1 << intraBitFieldIndex));
}

void KEngineCore::DataSapling::SetString(StringHash id, std::string_view value)
{
	id = TabulateStringHash(id);
	size_t stringIndex = mStringTable->AddString(value);
	if (mOwnsHeader && !HasString(id))
	{
		mHeader->AddString(id);
		mStringIndices.push_back(stringIndex);
	}
	else
	{
		assert(HasString(id));
		int index = mHeader->GetStringIndex(id);
		mStringIndices[index] = index;
	}
}


void KEngineCore::DataSapling::SetHash(StringHash id, StringHash value)
{
	id = TabulateStringHash(id);
	value = TabulateStringHash(value);
	if (mOwnsHeader && !HasHash(id))
	{
		mHeader->AddHash(id);
		mHashes.push_back(value);
		return;
	}
	else
	{
		assert(HasHash(id));
		int index = mHeader->GetHashIndex(id);
		mHashes[index] = value;
	}
}

KEngineCore::DataTreeHeader* KEngineCore::DataSapling::CreateBranchHeader()
{
	assert(mBranchHeader == nullptr);
	mBranchHeader = new DataTreeHeader();
	mBranchHeader->Init(mStringTable);
	return mBranchHeader;
}

void KEngineCore::DataSapling::AddKey(StringHash id)
{
	assert(mKeyMap.find(id) == mKeyMap.end());
	mKeyMap[id] = (int)mBranchMaps.size();
	mBranchMaps.emplace_back();
}

KEngineCore::DataSapling* KEngineCore::DataSapling::GrowBranch()
{
	auto* branch = new DataSapling();
	branch->Init(this, mBranchHeader, mStringTable);
	mBranches.push_back(branch);
	return branch;
}

void KEngineCore::DataSapling::BranchReady(DataSapling* branch)
{
	assert(mBranches.back() == branch); // This is shitty but right now the branch being declared ready must be the last one.
	for (auto keyPair : mKeyMap)
	{
		if (branch->HasHash(keyPair.first))
		{
			mBranchMaps[keyPair.second][branch->GetHash(keyPair.first)] = mBranches.size() - 1;
		}
	}
}

KEngineCore::DataTree::DataTree()
{
}

KEngineCore::DataTree::~DataTree()
{
	for (auto* branch : mBranches)
	{
		delete branch;
	}

	if (mBranchHeader)
	{
		delete mBranchHeader;
	}

	if (mOwnsHeader)
	{
		delete mHeader;
	}

	if (mOwnsStringTable)
	{
		delete mStringTable;
	}
}

bool KEngineCore::DataTree::HasInt(StringHash id) const
{
	return mHeader->HasInt(id);
}

int KEngineCore::DataTree::GetInt(StringHash id) const
{
	assert(HasInt(id));
	return mInts[mHeader->GetIntIndex(id)];
}

bool KEngineCore::DataTree::HasFloat(StringHash id) const
{
	return mHeader->HasFloat(id);
}

float KEngineCore::DataTree::GetFloat(StringHash id) const
{
	assert(HasFloat(id));
	return mFloats[mHeader->GetFloatIndex(id)];
}

bool KEngineCore::DataTree::HasBool(StringHash id) const
{
	return mHeader->HasBool(id);
}

bool KEngineCore::DataTree::GetBool(StringHash id) const
{
	assert(HasBool(id));
	int index = mHeader->GetBoolIndex(id);
	int interBitFieldIndex = index / 8;
	int intraBitFieldIndex = index % 8;
	return mBitFields[interBitFieldIndex] & 1 << intraBitFieldIndex;
}


bool KEngineCore::DataTree::HasString(StringHash id) const
{
	return mHeader->HasString(id);
}

std::string_view KEngineCore::DataTree::GetString(StringHash id) const
{
	assert(HasString(id));
	size_t tableIndex =  mStringIndices[mHeader->GetStringIndex(id)];
	return mStringTable->GetString(tableIndex);
}

bool KEngineCore::DataTree::HasHash(StringHash id) const
{
	return mHeader->HasHash(id);
}

KEngineCore::StringHash KEngineCore::DataTree::GetHash(StringHash id) const
{
	assert(HasHash(id));
	return mHashes[mHeader->GetHashIndex(id)];
}

const std::vector<KEngineCore::DataTree*>& KEngineCore::DataTree::GetBranches() const
{
	return mBranches;
}

int KEngineCore::DataTree::GetNumBranches() const
{
	return mBranches.size();
}

KEngineCore::DataTree* KEngineCore::DataTree::GetBranch(int index) const
{
	return mBranches[index];
}

bool KEngineCore::DataTree::HasBranch(StringHash key, StringHash id) const
{
	auto keyIt = mKeyMap.find(key);
	if (keyIt != mKeyMap.end())
	{
		int keyIdx = keyIt->second;
		auto branchIt = mBranchMaps[keyIdx].find(id);
		return branchIt != mBranchMaps[keyIdx].end();
	}
	return false;
}

KEngineCore::DataTree* KEngineCore::DataTree::GetBranch(StringHash key, StringHash id) const
{
	auto keyIt = mKeyMap.find(key);
	int branchMapIdx = keyIt->second;
	const IndexMap& branchMap = mBranchMaps[branchMapIdx];
	auto branchIt = branchMap.find(id);
	int branchIdx = branchIt->second;
	return mBranches[branchIdx];
}


KEngineCore::StringHash KEngineCore::DataSapling::TabulateStringHash(StringHash hash)
{
#ifndef DELETE_STRINGS
	size_t stringIndex = mStringTable->AddString(hash.string);
	return StringHash(mStringTable->GetString(stringIndex), hash);
#else
	return hash;
#endif
}
