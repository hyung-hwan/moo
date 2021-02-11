/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include "moo-prv.h"

#define MIN_HEAP_SIZE 65536 * 5 /* TODO: adjust this value? */
#define PERM_SPACE_SIZE 65536 * 2 /* TODO: adjust perm space size depending on what's allocated in the permspace */

static void* xma_alloc (moo_mmgr_t* mmgr, moo_oow_t size)
{
	return moo_xma_alloc(mmgr->ctx, size);
}

static void* xma_realloc (moo_mmgr_t* mmgr, void* ptr, moo_oow_t size)
{
	return moo_xma_realloc(mmgr->ctx, ptr, size);
}

static void xma_free (moo_mmgr_t* mmgr, void* ptr)
{
	return moo_xma_free (mmgr->ctx, ptr);
}

moo_heap_t* moo_makeheap (moo_t* moo, moo_oow_t size)
{
	moo_heap_t* heap;
	moo_oow_t alloc_size;

	if (size < MIN_HEAP_SIZE && moo->gc_type != MOO_GC_TYPE_MARK_SWEEP) size = MIN_HEAP_SIZE;

	if (size <= MOO_SIZEOF(*heap)) /* 0 or smaller than the heap header */
	{
		/* make a zero-sized heap using the default memory manager.
		 * this zero-sized heap contains only the heap header */
		size = 0;
		alloc_size = MOO_SIZEOF(*heap);
		heap = (moo_heap_t*)moo_allocmem(moo, alloc_size);
	}
	else
	{
		/* if a non-zero heap size is given, create the heap with
		 * the dedicated heap allocator which is allowed to create
		 * a bigger heap than requested  */
		alloc_size = size;
		heap = (moo_heap_t*)moo->vmprim.alloc_heap(moo, &alloc_size);
	}
	if (MOO_UNLIKELY(!heap)) 
	{
		const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
		moo_seterrbfmt (moo, moo_geterrnum(moo), "unable to allocate a heap - %js", oldmsg);
		return MOO_NULL;
	}

	/* the vmprim.alloc_heap() function is allowed to create a bigger heap than the requested size.
	 * if the created heap is bigger than requested, the heap will be utilized in full. */
	MOO_ASSERT (moo, alloc_size >= MOO_SIZEOF(*heap));
	MOO_MEMSET (heap, 0, alloc_size);

	alloc_size -= MOO_SIZEOF(*heap); /* exclude the header size */
	heap->base = (moo_uint8_t*)(heap + 1);
	heap->size = alloc_size;

	if (moo->gc_type == MOO_GC_TYPE_MARK_SWEEP)
	{
		if (size == 0)
		{
			/* use the existing memory allocator */
			MOO_ASSERT (moo, alloc_size == 0);
			heap->xmmgr = *moo_getmmgr(moo);
		}
		else
		{
			/* create a new memory allocator over the allocated heap */
			heap->xma = moo_xma_open(moo_getmmgr(moo), 0, heap->base, heap->size);
			if (MOO_UNLIKELY(!heap->xma))
			{
				moo->vmprim.free_heap (moo, heap);
				moo_seterrbfmt (moo, MOO_ESYSMEM, "unable to allocate a memory manager over a heap");
				return MOO_NULL;
			}

			heap->xmmgr.alloc = xma_alloc;
			heap->xmmgr.realloc = xma_realloc;
			heap->xmmgr.free = xma_free;
			heap->xmmgr.ctx = heap->xma;
		}
	}
	else
	{
		moo_oow_t space_size;

		MOO_ASSERT (moo, moo->gc_type == MOO_GC_TYPE_SEMISPACE);

		space_size = (alloc_size - PERM_SPACE_SIZE) / 2;

		/* TODO: consider placing permspace in a separate memory chunk in case we have to grow
		 *       other spaces. we may be able to realloc() the entire heap region without affecting the separately
		 *       allocated chunk. */
		heap->permspace.base = (moo_uint8_t*)(heap + 1);
		heap->permspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)heap->permspace.base), MOO_SIZEOF(moo_oop_t));
		heap->permspace.limit = heap->permspace.base + PERM_SPACE_SIZE;

		heap->curspace.base = heap->permspace.limit;
		heap->curspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)heap->curspace.base), MOO_SIZEOF(moo_oop_t));
		heap->curspace.limit = heap->curspace.base + space_size;

		heap->newspace.base = heap->curspace.limit;
		heap->newspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)heap->newspace.base), MOO_SIZEOF(moo_oop_t));
		heap->newspace.limit = heap->newspace.base + space_size;

		/* if size is too small, space.ptr may go past space.limit even at 
		 * this moment depending on the alignment of space.base. subsequent
		 * calls to moo_allocheapspace() are bound to fail. Make sure to
		 * pass a heap size large enough */
	}

	return heap;
}

void moo_killheap (moo_t* moo, moo_heap_t* heap)
{
	if (heap->size == 0)
	{
		moo_freemem (moo, heap);
	}
	else
	{
		if (heap->xma) moo_xma_close (heap->xma);
		moo->vmprim.free_heap (moo, heap);
	}
}

void* moo_allocheapspace (moo_t* moo, moo_space_t* space, moo_oow_t size)
{
	moo_uint8_t* ptr;

	MOO_ASSERT (moo, moo->gc_type == MOO_GC_TYPE_SEMISPACE);

	/* check the space size limit */
	if (space->ptr >= space->limit || space->limit - space->ptr < size)
	{
		MOO_DEBUG5 (moo, "Cannot allocate %zd bytes from space - ptr %p limit %p size %zd free %zd\n",
			size, space->ptr, space->limit, (moo_oow_t)(space->limit - space->base), (moo_oow_t)(space->limit - space->ptr));
		moo_seterrnum (moo, MOO_EOOMEM);
		return MOO_NULL;
	}

	/* allocation is as simple as moving the space pointer */
	ptr = space->ptr;
	space->ptr += size;

	return ptr;
}

void* moo_callocheapmem (moo_t* moo, moo_heap_t* heap, moo_oow_t size)
{
	void* ptr;

	MOO_ASSERT (moo, moo->gc_type == MOO_GC_TYPE_MARK_SWEEP);
	ptr = MOO_MMGR_ALLOC(&heap->xmmgr, size);
	if (MOO_UNLIKELY(!ptr)) 
	{
		MOO_DEBUG2 (moo, "Cannot callocate %zd bytes from heap - ptr %p\n", size, heap);
		moo_seterrnum (moo, MOO_EOOMEM);
	}
	else
	{
		MOO_MEMSET (ptr, 0, size);
	}
	return ptr;
}

void* moo_callocheapmem_noerr (moo_t* moo, moo_heap_t* heap, moo_oow_t size)
{
	void* ptr;
	MOO_ASSERT (moo, moo->gc_type == MOO_GC_TYPE_MARK_SWEEP);
	ptr = MOO_MMGR_ALLOC(&heap->xmmgr, size);
	if (MOO_LIKELY(ptr)) MOO_MEMSET (ptr, 0, size);
	return ptr;
}

void moo_freeheapmem (moo_t* moo, moo_heap_t* heap, void* ptr)
{
	MOO_ASSERT (moo, moo->gc_type == MOO_GC_TYPE_MARK_SWEEP);
	MOO_MMGR_FREE (&heap->xmmgr, ptr);
}
