#pragma once
#include <string_view>

namespace KEngineCore {

	class StringTable
	{
	public:
		StringTable();
		~StringTable();
		void Init(size_t size, size_t numStrings, size_t* startIndices, size_t* endIndices, char* stringData);
		void Init(size_t startingSize = 256, size_t startingIndices = 16, size_t maxSize = SIZE_MAX, size_t maxIndices = SIZE_MAX);
		void Deinit();

		size_t AddString(std::string_view string);
		std::string_view GetString(size_t index);


		void WriteToStream(std::ostream& stream) const;
		void ReadFromStream(std::istream& stream);


	protected:		
		
		void ResizeStringData(size_t newSize);
		void ResizeIndexData(size_t newSize);

		size_t	mUnusedSize{ 0 };
		size_t	mMaxSize{ 0 };
		size_t	mUnusedStrings{ 0 };
		size_t	mMaxNumStrings{ 0 };
		size_t	mSize{ 0 };
		size_t	mNumStrings{ 0 };
		size_t*	mStartIndices{ nullptr };
		size_t*	mEndIndices{ nullptr };
		char*	mStringData{ nullptr };
	};
}