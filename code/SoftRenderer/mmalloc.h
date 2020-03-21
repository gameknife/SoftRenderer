#pragma once

/////////////////////////////
// mm_malloc for mac
#ifndef OS_WIN32
//#define _mm_malloc(a,b) malloc(a)
//#define _mm_free(a) free(a)
#include <emmintrin.h>
#else
#include <malloc.h>
#endif
//


extern std::map<const void*, void*> m_align_pt_mapper;

inline void* _mm_malloc_16byte(size_t sz, size_t align)
{
	if (sz <= 0) {
		return NULL;
	}
	unsigned char* pSystemPointer = (unsigned char*)_mm_malloc(sz + 15, 16);
	if (NULL == pSystemPointer) {
		return NULL;
	}
	size_t offset = 16 - (((uint64)pSystemPointer) % 16);

	m_align_pt_mapper[pSystemPointer + offset] = pSystemPointer;

	return pSystemPointer + offset;
}

inline void* _mm_malloc_32byte(size_t sz, size_t align)
{
	if (sz <= 0) {
		return NULL;
	}
	unsigned char* pSystemPointer = (unsigned char*)_mm_malloc(sz + 31, 32);
	if (NULL == pSystemPointer) {
		return NULL;
	}
	size_t offset = 32 - (((uint64)pSystemPointer) % 32);

	m_align_pt_mapper[pSystemPointer + offset] = pSystemPointer;

	return pSystemPointer + offset;
}

inline void _mm_free_16byte(void* p)
{
	_mm_free(m_align_pt_mapper[p]);
}

#define _mm_malloc_custom _mm_malloc_32byte
#define _mm_free_custom _mm_free_16byte

