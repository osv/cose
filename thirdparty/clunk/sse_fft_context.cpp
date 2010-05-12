#include <stdlib.h>
#ifdef _WINDOWS
#include <malloc.h>
#endif
#include <stdio.h>
#include <new>
#include "fft_context.h"
#include "sse_fft_context.h"

using namespace clunk;

void * aligned_allocator::allocate(size_t size, size_t alignment) {
	void * ptr;
#ifdef _WINDOWS
	ptr = _aligned_malloc(size, alignment);
	if (ptr == NULL)
#else
    if (posix_memalign(&ptr, alignment, size) != 0)
#endif
		throw std::bad_alloc();
	return ptr;
}

void aligned_allocator::deallocate(void *ptr) {
#ifdef _WINDOWS
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}
