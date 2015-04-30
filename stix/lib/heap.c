/*
 * $Id$
 *
    Copyright (c) 2014-2015 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stix-prv.h"

stix_heap_t* stix_makeheap (stix_t* stix, stix_size_t size)
{
	stix_heap_t* heap;

	heap = (stix_heap_t*)STIX_MMGR_ALLOC(stix->mmgr, STIX_SIZEOF(*heap) + size);
	if (!heap)
	{
		stix->errnum = STIX_ENOMEM;
		return STIX_NULL;
	}

	STIX_MEMSET (heap, 0, STIX_SIZEOF(*heap) + size);

	heap->base = (stix_uint8_t*)(heap + 1);
	/* adjust the initial allocation pointer to a multiple of the oop size */
	heap->ptr = (stix_uint8_t*)STIX_ALIGN(((stix_uintptr_t)heap->base), STIX_SIZEOF(stix_oop_t));
	heap->limit = heap->base + size;

	STIX_ASSERT (heap->ptr >= heap->base);

	/* if size is too small, heap->ptr may go past heap->limit even at 
	 * this moment depending on the alignment of heap->base. subsequent
	 * calls to substix_allocheapmem() are bound to fail. Make sure to
	 * pass a heap size large enough */

	return heap;
}

void stix_killheap (stix_t* stix, stix_heap_t* heap)
{
	STIX_MMGR_FREE (stix->mmgr, heap);
}

void* stix_allocheapmem (stix_t* stix, stix_heap_t* heap, stix_size_t size)
{
	stix_uint8_t* ptr;

	/* check the heap size limit */
	if (heap->ptr >= heap->limit || heap->limit - heap->ptr < size)
	{
		stix->errnum = STIX_ENOMEM;
		return STIX_NULL;
	}

	/* allocation is as simple as moving the heap pointer */
	ptr = heap->ptr;
	heap->ptr += size;

	return ptr;
}
