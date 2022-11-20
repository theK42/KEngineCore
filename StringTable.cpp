#include "StringTable.h"
#include "BinaryFile.h"
#include <iostream>
#include <assert.h>
#include <string.h>

KEngineCore::StringTable::StringTable()
{
}

KEngineCore::StringTable::~StringTable()
{
	Deinit();
}

void KEngineCore::StringTable::Init(int64_t startingSize, int64_t startingIndices, int64_t maxSize, int64_t maxIndices)
{
	assert(mStringData == nullptr);
	mSize = 0;
	mNumStrings = 0;
	mStringData = new char[startingSize];
	mUnusedSize = startingSize;
	mUnusedStrings = startingIndices;
	mStartIndices = new int64_t[startingIndices];
	mEndIndices = new int64_t[startingIndices];
	mMaxSize = maxSize;
	mMaxNumStrings = maxIndices;
}

void KEngineCore::StringTable::Deinit()
{
	if (mStringData != nullptr) {
		mUnusedSize = 0;
		mMaxSize = 0;
		mMaxNumStrings = 0;
		mSize = 0;
		mNumStrings = 0;
		delete[] mStringData;
		delete[] mStartIndices;
		delete[] mEndIndices;
		mStartIndices = nullptr;
		mEndIndices = nullptr;
		mStringData = nullptr;
	}
}

void KEngineCore::StringTable::Init(int64_t size, int64_t numStrings, int64_t* startIndices, int64_t* endIndices, char* stringData)
{
	assert(mSize == 0);
	mSize = size;
	mNumStrings = numStrings;
	mStartIndices = startIndices;
	mEndIndices = endIndices;
	mStringData = stringData;
}

std::string_view KEngineCore::StringTable::GetString(int64_t index)
{
	assert(index >= 0 && index < mNumStrings);
	int64_t startIndex = mStartIndices[index];
	int64_t endIndex = mEndIndices[index];
	int64_t length = endIndex - startIndex;
	return std::string_view(mStringData + startIndex, length);
}


void KEngineCore::StringTable::WriteToStream(std::ostream& stream) const
{
	stream.write((char*)&mSize, sizeof(mSize));
	stream.write((char*)&mNumStrings, sizeof(mNumStrings));
	stream.write((char*)mStartIndices, sizeof(int64_t) * mNumStrings);
	stream.write((char*)mEndIndices, sizeof(int64_t) * mNumStrings);
	stream.write((char*)mStringData, sizeof(char) * mSize);
}

void KEngineCore::StringTable::ReadFromStream(std::istream& stream)
{
	assert(mStringData == nullptr);
	stream.read((char*)&mSize, sizeof(mSize));
	stream.read((char*)&mNumStrings, sizeof(mNumStrings));
	mStartIndices = new int64_t[mNumStrings];
	stream.read((char*)mStartIndices, sizeof(int64_t) * mNumStrings);
	mEndIndices = new int64_t[mNumStrings];
	stream.read((char*)mEndIndices, sizeof(int64_t) * mNumStrings);
	mStringData = new char[mSize];
	stream.read((char*)mStringData, sizeof(char) * mSize);
    
}

int64_t KEngineCore::StringTable::AddString(std::string_view string)
{
	//Check for substring matches
	for (int64_t stringIndex = 0; stringIndex < mNumStrings; stringIndex++ )
	{
		auto other = GetString(stringIndex);
		if (other == string)
		{
			return stringIndex;
		}
		int64_t position = other.find(string);
		if (position != -1)
		{
			if (mUnusedStrings <= 0)
			{
				int64_t newSize = mNumStrings * 2;
				if (mMaxNumStrings > 0)
				{
					newSize = std::min(newSize, mMaxNumStrings);
				}
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
		int64_t newSize = mNumStrings * 2;
		if (mMaxNumStrings > 0)
		{
			newSize = std::min(newSize, mMaxNumStrings);
		}
		ResizeIndexData(newSize);
	}
	if (mUnusedSize < string.length())
	{
		assert(mSize > 0);
		int64_t newSize = mSize * 2;
		while (newSize - (mSize - mUnusedSize) < string.length())
		{
			newSize *= 2;
		}
		if (mMaxSize > 0) {
			newSize = std::min(newSize, mMaxSize);
		}
		assert(newSize + mUnusedSize >= string.length());
		ResizeStringData(newSize);
	}
	mStartIndices[mNumStrings] = mSize;
	mEndIndices[mNumStrings] = mSize + string.length();
	strncpy(mStringData + mStartIndices[mNumStrings], string.data(), std::min(string.length(), (size_t)mUnusedSize));
	mSize += string.length();
	mUnusedSize -= string.length();
	mUnusedStrings--;
	return mNumStrings++;
}

void KEngineCore::StringTable::ResizeStringData(int64_t newSize)
{
	assert(newSize < mMaxSize);
	char* newData = new char[newSize];
	memcpy(newData, mStringData, mSize);
	delete[] mStringData;
	mStringData = newData;
	mUnusedSize = newSize - mSize;
}

void KEngineCore::StringTable::ResizeIndexData(int64_t newSize)
{
	assert(newSize < mMaxNumStrings);
	int64_t* newBeginnings = new int64_t[newSize];
	int64_t* newEndings = new int64_t[newSize];
	memcpy(newBeginnings, mStartIndices, mNumStrings * sizeof(int64_t));
	memcpy(newEndings, mEndIndices, mNumStrings * sizeof(int64_t));
	delete[] mStartIndices;
	delete[] mEndIndices;
	mStartIndices = newBeginnings;
	mEndIndices = newEndings;
	mUnusedStrings = newSize - mNumStrings;
}


