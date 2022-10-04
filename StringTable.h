#pragma once
#include <string_view>

namespace KEngineCore {

	class StringTable
	{
	public:
		StringTable();
		~StringTable();
		void Init(int64_t size, int64_t numStrings, int64_t* startIndices, int64_t* endIndices, char* stringData);
		void Init(int64_t startingSize = 256, int64_t startingIndices = 16, int64_t maxSize = SIZE_MAX, int64_t maxIndices = SIZE_MAX);
		void Deinit();

		int64_t AddString(std::string_view string);
		std::string_view GetString(int64_t index);


		void WriteToStream(std::ostream& stream) const;
		void ReadFromStream(std::istream& stream);


	protected:		
		
		void ResizeStringData(int64_t newSize);
		void ResizeIndexData(int64_t newSize);

		int64_t	mUnusedSize{ 0 };
		int64_t	mMaxSize{ 0 };
		int64_t	mUnusedStrings{ 0 };
		int64_t	mMaxNumStrings{ 0 };
		int64_t	mSize{ 0 };
		int64_t	mNumStrings{ 0 };
		int64_t*	mStartIndices{ nullptr };
		int64_t*	mEndIndices{ nullptr };
		char*	mStringData{ nullptr };
	};
}
