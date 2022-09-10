#include "StringTable.h"
#include <assert.h>

KEngineCore::StringTable::StringTable()
{
}

KEngineCore::StringTable::~StringTable()
{
	Deinit();
}

void KEngineCore::StringTable::Init(size_t size, size_t numStrings, size_t* startIndices, size_t* endIndices, char* stringData)
{
	assert(mSize == 0);
	mSize = size;
	mNumStrings = numStrings;
	mStartIndices = startIndices;
	mEndIndices = endIndices;
	mStringData = stringData;
}

void KEngineCore::StringTable::Deinit()
{
	//Currently, StringTable does not own its memory, so just set pointers to null
	mSize = 0;
	mNumStrings = 0;
	mStartIndices = nullptr;
	mEndIndices = nullptr;
	mStringData = nullptr;
}

std::string_view KEngineCore::StringTable::GetString(size_t index)
{
	assert(index >= 0 && index < mNumStrings);
	size_t startIndex = mStartIndices[index];
	size_t endIndex = mEndIndices[index];
	size_t length = endIndex - startIndex;
	return std::string_view(mStringData + startIndex, length);
}

KEngineCore::StringTableBuilder::StringTableBuilder()
{
}

KEngineCore::StringTableBuilder::~StringTableBuilder()
{
	Deinit();
}

void KEngineCore::StringTableBuilder::Init(size_t startingSize, size_t startingIndices, size_t maxSize, size_t maxIndices)
{
	mStringData = new char[startingSize];
	mUnusedSize = startingSize;
	mUnusedStrings = startingIndices;
	mStartIndices = new size_t[startingIndices];
	mEndIndices = new size_t[startingIndices];
	mMaxSize = maxSize;
	mMaxNumStrings = maxIndices;
}

void KEngineCore::StringTableBuilder::Deinit()
{	
	mUnusedSize = 0;
	mMaxSize = 0;
	mMaxNumStrings = 0;
	delete[] mStringData;
	delete[] mStartIndices;
	delete[] mEndIndices;
	StringTable::Deinit();
}

size_t KEngineCore::StringTableBuilder::AddString(std::string_view string)
{
	//Check for substring matches
	for (size_t stringIndex = 0; stringIndex < mNumStrings; stringIndex++ )
	{
		auto other = GetString(stringIndex);
		if (other == string)
		{
			return stringIndex;
		}
		size_t position = other.find(string);
		if (position != -1)
		{
			if (mUnusedStrings <= 0)
			{
				size_t newSize = mNumStrings * 2;
				newSize = std::min(newSize, mMaxNumStrings);
				ResizeIndexData(newSize);
			}
			mUnusedStrings--;
			mStartIndices[mNumStrings] = position;
			mEndIndices[mNumStrings] = position + string.length();
			mUnusedStrings--;
			return mNumStrings++;
		}
	}
	if (mUnusedStrings <= 0)
	{
		size_t newSize = mNumStrings * 2;
		newSize = std::min(newSize, mMaxNumStrings);
		ResizeIndexData(newSize);
	}
	if (mUnusedSize < string.length())
	{
		size_t newSize = mSize * 2;
		while (newSize + mUnusedSize < string.length())
		{
			newSize *= 2;
		}
		newSize = std::min(newSize, mMaxSize);
		assert(newSize + mUnusedSize >= string.length());
		ResizeIndexData(newSize);
	}
	mStartIndices[mNumStrings] = mSize;
	mEndIndices[mNumStrings] = mSize + string.length();
	strncpy_s(mStringData + mStartIndices[mNumStrings], mUnusedSize, string.data(), string.length());
	mSize += string.length();
	mUnusedSize -= string.length();
	mUnusedStrings--;
	return mNumStrings++;
}

void KEngineCore::StringTableBuilder::ResizeStringData(size_t newSize)
{
	assert(newSize < mMaxSize);
	char* newData = new char[newSize];
	memcpy(newData, mStringData, mSize);
	delete[] mStringData;
	mStringData = newData;
	mUnusedSize = newSize - mSize;
}

void KEngineCore::StringTableBuilder::ResizeIndexData(size_t newSize)
{
	assert(newSize < mMaxNumStrings);
	size_t* newBeginnings = new size_t[newSize];
	size_t* newEndings = new size_t[newSize];
	memcpy(newBeginnings, mStartIndices, mNumStrings);
	memcpy(newEndings, mEndIndices, mNumStrings);
	delete[] mStartIndices;
	delete[] mEndIndices;
	mStartIndices = newBeginnings;
	mEndIndices = newEndings;
	mUnusedStrings = newSize - mNumStrings;
}


