#ifndef CORE_LIB_MEMORY_POOL_H
#define CORE_LIB_MEMORY_POOL_H

#include "Basic.h"
#include "IntSet.h"

namespace CoreLib
{
	namespace Basic
	{
		struct MemoryBlockFields
		{
			unsigned int Occupied : 1;
			unsigned int Order : 31;
		};
		struct FreeListNode
		{
			FreeListNode * PrevPtr = nullptr, *NextPtr = nullptr;
		};
		class MemoryPool
		{
		private:
			static const int MaxLevels = 32;
			int blockSize = 0, log2BlockSize = 0;
			int numLevels = 0;
			int bytesAllocated = 0;
			int bytesWasted = 0;
			unsigned char * buffer = nullptr;
			FreeListNode * freeList[MaxLevels];
			IntSet used;
			int AllocBlock(int level);
			void FreeBlock(unsigned char * ptr, int level);
		public:
			MemoryPool(unsigned char * buffer, int log2BlockSize, int numBlocks);
			MemoryPool() = default;
			void Init(unsigned char * buffer, int log2BlockSize, int numBlocks);
			unsigned char * Alloc(int size);
			void Free(unsigned char * ptr, int size);
		};
	}
}

#endif