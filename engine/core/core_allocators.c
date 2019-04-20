#include "core_allocators.h"
#include "../util/util.h"

///////////////////////////////////////////////////
// SCRATCH ALLOCATOR
///////////////////////////////////////////////////

///////////////////////////////////////////////////
int16 Core_InitScratch(ScratchBuffer* o_buffer, uint32 i_scratchSizeInBytes, uint8 i_alignment)
{
	o_buffer->m_start = (uint8*)malloc3(i_scratchSizeInBytes + i_alignment);
	if (!o_buffer->m_start)
		return E_OUT_OF_MEMORY;

	{
		uint8* alignedPtr = Util_AlignPtr(o_buffer->m_start, i_alignment);
		o_buffer->m_alignment = alignedPtr - o_buffer->m_start;
		o_buffer->m_start = alignedPtr;

		o_buffer->m_end = ((uint8*)o_buffer->m_start) + i_scratchSizeInBytes;
		o_buffer->m_next = o_buffer->m_start;

		memset(o_buffer->m_start, 0xbabababa, i_scratchSizeInBytes);
	}
	return E_OK;
}

///////////////////////////////////////////////////
int16 Core_FreeScratch(ScratchBuffer* i_buffer)
{
	uint8* ptr = (uint8*)i_buffer->m_start - i_buffer->m_alignment;
	free3(ptr);
	return E_OK;
}

///////////////////////////////////////////////////
int16 Core_ResetScratch(ScratchBuffer* i_buffer)
{
	i_buffer->m_next = i_buffer->m_start;
	return E_OK;
}

///////////////////////////////////////////////////
uint32 Core_GetTotalScratch(ScratchBuffer* i_buffer)
{
	return (uint32)i_buffer->m_end - (uint32)i_buffer->m_start;
}

///////////////////////////////////////////////////
uint32 Core_GetFreeScratch(ScratchBuffer* i_buffer)
{
	return (uint32)i_buffer->m_end - (uint32)i_buffer->m_next;
}

///////////////////////////////////////////////////
uint32 Core_GetUsedScratch(ScratchBuffer* i_buffer)
{
	return (uint32)i_buffer->m_next - (uint32)i_buffer->m_start;
}

///////////////////////////////////////////////////
void* Core_AllocScratch(ScratchBuffer* i_buffer, uint32 i_bytes, uint8 i_alignment)
{
	i_buffer->m_next = Util_AlignPtr(i_buffer->m_next, i_alignment);
	{
		uint8 *mem = i_buffer->m_next;
		uint8* next = mem + i_bytes;

		if (next <= i_buffer->m_end)
		{
			i_buffer->m_next = next;
			return mem;
		}
	}
	VERIFY_ASSERT(FALSE, "Core_AllocScratch: Core scratch allocator is out of memory!");
	return NULL;
}

///////////////////////////////////////////////////
bool Core_IsFromScratch(ScratchBuffer* i_buffer, void* i_address)
{
	return ( ((uint32)i_address > (uint32)i_buffer->m_start) && ((uint32)i_address < (uint32)i_buffer->m_end) );
}

///////////////////////////////////////////////////
// STACK ALLOCATOR
///////////////////////////////////////////////////

typedef struct
{
	uint32 m_size;
}StackBufferHeader;

///////////////////////////////////////////////////
void TestStack(StackBuffer* i_buffer)
{
	uint32 k = 0;
	uint8* before = i_buffer->m_head;

	REPORT("Begin core allocator stack test");
	for (k = 0; k < 4; ++k)
	{
		uint32 i = 0;
		uint32 c = 32 + k * 64;

		uint32* ints = (uint32*)Core_PushStack(i_buffer, sizeof(uint32) * c, 16);

		for (i = 0; i < c; ++i)
		{
			*ints++ = i;
		}
	}

	for (k = 0; k < 4; ++k)
	{
		Core_PopStack(i_buffer);
	}
	VERIFY_ASSERT(i_buffer->m_head == before, "INVALID");
	REPORT("End core allocator stack test");
}

///////////////////////////////////////////////////
int16 Core_InitStack(StackBuffer* o_buffer, uint32 i_stackSizeInBytes, uint8 i_alignment)
{
	o_buffer->m_start = (uint8*)malloc3(i_stackSizeInBytes + i_alignment);
	if (!o_buffer->m_start)
		return E_OUT_OF_MEMORY;

	{
		uint8* alignedPtr = Util_AlignPtr(o_buffer->m_start, i_alignment);
		o_buffer->m_alignment = alignedPtr - o_buffer->m_start;
		o_buffer->m_start = alignedPtr;
		o_buffer->m_head = o_buffer->m_start;
		o_buffer->m_end = ((uint8*)o_buffer->m_start) + i_stackSizeInBytes;	

		memset(o_buffer->m_start, 0xbabababa, i_stackSizeInBytes);
	}

	return E_OK;
}

///////////////////////////////////////////////////
int16 Core_ResetStack(StackBuffer* i_buffer)
{
	i_buffer->m_head = i_buffer->m_start;
	return E_OK;
}

///////////////////////////////////////////////////
int16 Core_FreeStack(StackBuffer* i_buffer)
{
	uint8* ptr = (uint8*)i_buffer->m_start - i_buffer->m_alignment;
	free3(ptr);
	return E_OK;
}

///////////////////////////////////////////////////
uint32 Core_GetTotalStack(StackBuffer* i_buffer)
{
	return (uint32)i_buffer->m_end - (uint32)i_buffer->m_start;
}

///////////////////////////////////////////////////
uint32 Core_GetFreeStack(StackBuffer* i_buffer)
{
	return (uint32)i_buffer->m_end - (uint32)i_buffer->m_head;
}

///////////////////////////////////////////////////
uint32 Core_GetUsedStack(StackBuffer* i_buffer)
{
	return (uint32)i_buffer->m_head - (uint32)i_buffer->m_start;
}

///////////////////////////////////////////////////
void* Core_PushStack(StackBuffer* i_buffer, uint32 i_bytes, uint8 i_alignment)
{	
	uint32 headerSize = sizeof(StackBufferHeader);
	i_bytes = Util_AlignUp(i_bytes + headerSize, i_alignment);
		
	if (i_buffer->m_head + i_bytes <= i_buffer->m_end)
	{
		uint8* mem = i_buffer->m_head;
		uint8* next = mem + i_bytes;
		StackBufferHeader* header = (StackBufferHeader*)(next - headerSize);
		header->m_size = i_bytes;

		i_buffer->m_head = next;
		
		REPORT("Allocating %u bytes, head: %p, header: %p", i_bytes, next, header);
		return mem;
	}
	VERIFY_ASSERT(FALSE, "Core_PushStack: Core stack allocator is out of memory!");
	return NULL;
}

///////////////////////////////////////////////////
uint16 Core_PopStack(StackBuffer* i_buffer)
{
	StackBufferHeader* header = (StackBufferHeader*)(i_buffer->m_head - sizeof(StackBufferHeader));
	REPORT("Freeing %u bytes, head: %p, header: %p", header->m_size, i_buffer->m_head, header);
	VERIFY_ASSERT(header->m_size > 0, "Trying to free 0 bytes, this is probably a memory corruption");

	i_buffer->m_head = i_buffer->m_head - header->m_size;
	return 0u;
}

///////////////////////////////////////////////////
bool Core_IsFromStack(StackBuffer* i_buffer, void* i_address)
{
	return (((uint32)i_address > (uint32)i_buffer->m_start) && ((uint32)i_address < (uint32)i_buffer->m_end));
}