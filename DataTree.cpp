#include "DataTree.h"
#include "StringTable.h"
#include <assert.h>
#include <iostream>

KEngineCore::DataTreeHeader::DataTreeHeader()
{
}

KEngineCore::DataTreeHeader::~DataTreeHeader()
{
}

void KEngineCore::DataTreeHeader::Init(StringTable* stringTable)
{
	if (stringTable != nullptr)
	{
		mStringTable = stringTable;
		mOwnsStringTable = false;
	}
	else
	{
		mStringTable = new StringTable();
		mStringTable->Init();
		mOwnsStringTable = true;
	}
}

void KEngineCore::DataTreeHeader::Deinit()
{
	mIntMap.clear();
	mFloatMap.clear();
	mBoolMap.clear();
	mStringMap.clear();
	mHashMap.clear();

	if (mOwnsStringTable)
	{
		delete mStringTable;
		mStringTable = nullptr;
		mOwnsStringTable = false;
	}
}

void KEngineCore::DataTreeHeader::AddInt(StringHash id)
{
	id = TabulateStringHash(id);
	mIntMap[id] = (int)mIntMap.size();
}

void KEngineCore::DataTreeHeader::AddFloat(StringHash id)
{
	id = TabulateStringHash(id);
	mIntMap[id] = (int)mFloatMap.size();
}

void KEngineCore::DataTreeHeader::AddBool(StringHash id)
{
	id = TabulateStringHash(id);
	mIntMap[id] = (int)mBoolMap.size();
}

void KEngineCore::DataTreeHeader::AddString(StringHash id)
{
	id = TabulateStringHash(id);
	mStringMap[id] = (int)mStringMap.size();
}

void KEngineCore::DataTreeHeader::AddHash(StringHash id)
{
	id = TabulateStringHash(id);
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

int KEngineCore::DataTreeHeader::GetNumInts() const
{
	return mIntMap.size();
}

int KEngineCore::DataTreeHeader::GetNumFloats() const
{
	return mFloatMap.size();
}

int KEngineCore::DataTreeHeader::GetNumBools() const
{
	return mBoolMap.size();
}

int KEngineCore::DataTreeHeader::GetNumStrings() const
{
	return mStringMap.size();
}

int KEngineCore::DataTreeHeader::GetNumHashes() const
{
	return mHashMap.size();
}

KEngineCore::StringTable* KEngineCore::DataTreeHeader::GetStringTable() const
{
	return mStringTable;
}


void WriteHash(KEngineCore::StringHash hash, std::ostream& stream)
{
	stream.write((char*)&hash.hash, sizeof(hash.hash));
#ifndef DELETE_STRINGS
	stream.write((char*)&hash.tableIndex, sizeof(hash.tableIndex));
#else
	int none = -1;
	stream.write((char*)&none, sizeof(int));
#endif
}


KEngineCore::StringHash ReadHash(std::istream& stream, KEngineCore::StringTable* strings)
{
	KEngineCore::StringHash hash;
	stream.read((char*)&hash.hash, sizeof(hash.hash));
#ifndef DELETE_STRINGS
	stream.read((char*)&hash.tableIndex, sizeof(hash.tableIndex));
	if (hash.tableIndex >= 0)
	{
		hash.string = strings->GetString(hash.tableIndex);
	}
#else
	int discard = -1;
	stream.read((char*)& discard, sizeof(int));
#endif
	return hash;
}

void WriteIndexMap(const KEngineCore::IndexMap& map, std::ostream& stream)
{
	size_t tempSize;
	tempSize = map.size();
	stream.write((char*)&tempSize, sizeof(tempSize));
	for (auto pair : map)
	{
		WriteHash(pair.first, stream);
		stream.write((char*)&pair.second, sizeof(pair.second));
	}
}

void ReadIndexMap(KEngineCore::IndexMap& map, std::istream& stream, KEngineCore::StringTable* strings)
{
	size_t tempSize;
	stream.read((char*)&tempSize, sizeof(tempSize));
	for (size_t i = 0; i < tempSize; i++)
	{
		KEngineCore::StringHash key = ReadHash(stream, strings);
		int value = 0;
		stream.read((char*)value, sizeof(value));
		map.emplace(key, value);
	}
}


void KEngineCore::DataTreeHeader::WriteToStream(std::ostream& stream) const
{
	stream.write((char*)&mOwnsStringTable, sizeof(mOwnsStringTable));
	if (mOwnsStringTable)
	{
		mStringTable->WriteToStream(stream);
	}
	WriteIndexMap(mIntMap, stream);
	WriteIndexMap(mFloatMap, stream);
	WriteIndexMap(mBoolMap, stream);
	WriteIndexMap(mStringMap, stream);
	WriteIndexMap(mHashMap, stream);
}

void KEngineCore::DataTreeHeader::ReadFromStream(std::istream& stream)
{
	ReadIndexMap(mIntMap, stream, mStringTable);
	ReadIndexMap(mFloatMap, stream, mStringTable);
	ReadIndexMap(mBoolMap, stream, mStringTable);
	ReadIndexMap(mStringMap, stream, mStringTable);
	ReadIndexMap(mHashMap, stream, mStringTable);
}


KEngineCore::DataSapling::DataSapling()
{

}

KEngineCore::DataSapling::~DataSapling()
{
	Deinit();
}

void KEngineCore::DataSapling::Init(DataSapling* root, DataTreeHeader* header, StringTable* stringTable)
{
	mRoot = root;

	if (header == nullptr)
	{
		mHeader = new DataTreeHeader();
		mHeader->Init(stringTable);
		mOwnsHeader = true;
	}
	else
	{
		mHeader = header;
		mOwnsHeader = false;
	}

	mInts.resize(mHeader->GetNumInts());
	mFloats.resize(mHeader->GetNumFloats());
	mBitFields.resize(mHeader->GetNumBools() / sizeof(mBitFields[0]));
	mStringIndices.resize(mHeader->GetNumStrings());
	mHashes.resize(mHeader->GetNumHashes());
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


}

void KEngineCore::DataSapling::SetInt(StringHash id, int value)
{
	id = mHeader->TabulateStringHash(id);
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
	id = mHeader->TabulateStringHash(id);
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
	id = mHeader->TabulateStringHash(id);
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
	id = mHeader->TabulateStringHash(id);
	size_t stringIndex = mHeader->GetStringTable()->AddString(value);
	if (mOwnsHeader && !HasString(id))
	{
		mHeader->AddString(id);
		mStringIndices.push_back(stringIndex);
	}
	else
	{
		assert(HasString(id));
		int index = mHeader->GetStringIndex(id);
		mStringIndices[index] = stringIndex;
	}
}


void KEngineCore::DataSapling::SetHash(StringHash id, StringHash value)
{
	id = mHeader->TabulateStringHash(id);
	value = mHeader->TabulateStringHash(value);
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
	mBranchHeader->Init(mHeader->GetStringTable());
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
	branch->Init(this, mBranchHeader, mHeader->GetStringTable());
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
			mBranchMaps[keyPair.second][branch->GetHash(keyPair.first)] = (int)mBranches.size() - 1;
		}
	}
}

KEngineCore::DataTree::DataTree()
{
}

KEngineCore::DataTree::~DataTree()
{
	Deinit();
}

void KEngineCore::DataTree::Init(DataTreeHeader* header)
{
	assert(mHeader == nullptr);
	mHeader = header;
	mOwnsHeader = false;
	mInts.resize(mHeader->GetNumInts());
	mFloats.resize(mHeader->GetNumFloats());
	mBitFields.resize(mHeader->GetNumBools() / sizeof(mBitFields[0]));
	mStringIndices.resize(mHeader->GetNumStrings());
	mHashes.resize(mHeader->GetNumHashes());
}

void KEngineCore::DataTree::Deinit()
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
	return mHeader->GetStringTable()->GetString(tableIndex);
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

void KEngineCore::DataTree::WriteToStream(std::ostream& stream) const
{
	stream.write((char*)&mOwnsHeader, sizeof(mOwnsHeader));
	if (mOwnsHeader)
	{
		mHeader->WriteToStream(stream);
	}
	if (mBranchHeader != nullptr)
	{
		bool truth = true;
		stream.write((char*)&truth, sizeof(truth));
		mBranchHeader->WriteToStream(stream);
	}
	else
	{
		bool falsehood = false;
		stream.write((char*)&falsehood, sizeof(falsehood));
	}

	size_t size;
	
	size = mInts.size();
	stream.write((char*)&size, sizeof(size));
	stream.write((char*)mInts.data(), size * sizeof(mInts[0]));

	size = mFloats.size();
	stream.write((char*)&size, sizeof(size));
	stream.write((char*)mFloats.data(), size * sizeof(mFloats[0]));

	size = mBitFields.size();
	stream.write((char*)&size, sizeof(size));
	stream.write((char*)mBitFields.data(), size * sizeof(mBitFields[0]));

	size = mStringIndices.size();
	stream.write((char*)&size, sizeof(size));
	stream.write((char*)mStringIndices.data(), size * sizeof(mStringIndices[0]));

	size = mHashes.size();
	stream.write((char*)&size, sizeof(size));
	for (auto& hash : mHashes)
	{
		WriteHash(hash, stream);
	}
	
	WriteIndexMap(mKeyMap, stream);

	size = mBranchMaps.size();
	stream.write((char*)&size, sizeof(size));
	for (auto& map : mBranchMaps)
	{
		WriteIndexMap(map, stream);
	}

	size = mBranches.size();
	stream.write((char*)&size, sizeof(size));
	for (auto branch : mBranches)
	{
		branch->WriteToStream(stream);
	}
}

void KEngineCore::DataTree::ReadFromStream(std::istream& stream, StringTable * strings)
{
	stream.read((char*)&mOwnsHeader, sizeof(mOwnsHeader));
	if (mOwnsHeader)
	{
		mHeader = new DataTreeHeader();
		if (strings != nullptr)
		{
			mHeader->Init(strings);
		}
		mHeader->ReadFromStream(stream);
	}
	bool hasBranchHeader;
	stream.read((char*)&hasBranchHeader, sizeof(hasBranchHeader));
	if (hasBranchHeader)
	{
		mBranchHeader = new DataTreeHeader();
		mBranchHeader->Init(mHeader->GetStringTable());
		mBranchHeader->ReadFromStream(stream);
	}

	size_t size = 0;

	stream.read((char*)size, sizeof(size));
	mInts.resize(size);
	stream.read((char*)mInts.data(), size * sizeof(mInts[0]));

	stream.read((char*)size, sizeof(size));
	mFloats.resize(size);
	stream.read((char*)mFloats.data(), size * sizeof(mFloats[0]));

	stream.read((char*)size, sizeof(size));
	mBitFields.resize(size);
	stream.read((char*)mBitFields.data(), size * sizeof(mBitFields[0]));

	stream.read((char*)size, sizeof(size));
	mStringIndices.resize(size);
	stream.read((char*)mStringIndices.data(), size * sizeof(mStringIndices[0]));

	stream.read((char*)size, sizeof(size));
	mHashes.resize(size);
	for (auto& hash : mHashes)
	{
		hash = ReadHash(stream, mHeader->GetStringTable());
	}

	ReadIndexMap(mKeyMap, stream, mHeader->GetStringTable());

	stream.read((char*)size, sizeof(size));
	mBranchMaps.resize(size);
	for (auto& map : mBranchMaps)
	{
		ReadIndexMap(map, stream, mHeader->GetStringTable());
	}

	stream.read((char*)size, sizeof(size));
	mBranches.resize(size);
	for (auto& branch : mBranches)
	{
		branch = new DataTree();
		if (mBranchHeader != nullptr)
		{
			branch->Init(mBranchHeader);  //Okay this...isn't quite right.
		}
		branch->ReadFromStream(stream, mHeader->GetStringTable());
	}
}

int KEngineCore::DataTree::GetNumBranches() const
{
	return (int)mBranches.size();
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


KEngineCore::StringHash KEngineCore::DataTreeHeader::TabulateStringHash(StringHash hash)
{
#ifndef DELETE_STRINGS
	size_t stringIndex = mStringTable->AddString(hash.string);
	return StringHash(mStringTable->GetString(stringIndex), hash); 
#else
	return hash;
#endif
}
