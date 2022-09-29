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

void KEngineCore::StringTable::Init(size_t startingSize, size_t startingIndices, size_t maxSize, size_t maxIndices)
{
	assert(mStringData == nullptr);
	mSize = 0;
	mNumStrings = 0;
	mStringData = new char[startingSize];
	mUnusedSize = startingSize;
	mUnusedStrings = startingIndices;
	mStartIndices = new size_t[startingIndices];
	mEndIndices = new size_t[startingIndices];
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

void KEngineCore::StringTable::Init(size_t size, size_t numStrings, size_t* startIndices, size_t* endIndices, char* stringData)
{
	assert(mSize == 0);
	mSize = size;
	mNumStrings = numStrings;
	mStartIndices = startIndices;
	mEndIndices = endIndices;
	mStringData = stringData;
}

std::string_view KEngineCore::StringTable::GetString(size_t index)
{
	assert(index >= 0 && index < mNumStrings);
	size_t startIndex = mStartIndices[index];
	size_t endIndex = mEndIndices[index];
	size_t length = endIndex - startIndex;
	return std::string_view(mStringData + startIndex, length);
}


void KEngineCore::StringTable::WriteToStream(std::ostream& stream) const
{
	stream.write((char*)&mSize, sizeof(mSize));
	stream.write((char*)&mNumStrings, sizeof(mNumStrings));
	stream.write((char*)mStartIndices, sizeof(size_t) * mNumStrings);
	stream.write((char*)mEndIndices, sizeof(size_t) * mNumStrings);
	stream.write((char*)mStringData, sizeof(char) * mSize);
}

void KEngineCore::StringTable::ReadFromStream(std::istream& stream)
{
	assert(mStringData == nullptr);
	stream.read((char*)&mSize, sizeof(mSize));
	stream.read((char*)&mNumStrings, sizeof(mNumStrings));
	mStartIndices = new size_t[mNumStrings];
	stream.read((char*)mStartIndices, sizeof(size_t) * mNumStrings);
	mEndIndices = new size_t[mNumStrings];
	stream.read((char*)mEndIndices, sizeof(size_t) * mNumStrings);
	mStringData = new char[mSize];
	stream.read((char*)mStringData, sizeof(char) * mSize);
}

size_t KEngineCore::StringTable::AddString(std::string_view string)
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
		assert(mSize > 0);
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
	strncpy(mStringData + mStartIndices[mNumStrings], string.data(), std::min(string.length(), mUnusedSize));
	mSize += string.length();
	mUnusedSize -= string.length();
	mUnusedStrings--;
	return mNumStrings++;
}

void KEngineCore::StringTable::ResizeStringData(size_t newSize)
{
	assert(newSize < mMaxSize);
	char* newData = new char[newSize];
	memcpy(newData, mStringData, mSize);
	delete[] mStringData;
	mStringData = newData;
	mUnusedSize = newSize - mSize;
}

void KEngineCore::StringTable::ResizeIndexData(size_t newSize)
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


