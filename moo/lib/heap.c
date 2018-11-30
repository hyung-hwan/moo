/*
 * $Id$
 *
    Copyright (c) 2014-2018 Chung, Hyung-Hwan. All rights reserved.

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

moo_heap_t* moo_makeheap (moo_t* moo, moo_oow_t size)
{
	moo_heap_t* heap;
	moo_oow_t half_size;

	if (size < 65536) size = 65536; /* not really useful check as 65536 is too small */
	half_size = size / 2;

	heap = (moo_heap_t*)moo->vmprim.alloc_heap(moo, MOO_SIZEOF(*heap) + size);
	if (!heap) 
	{
		const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
		moo_seterrbfmt (moo, moo_geterrnum(moo), "unable to allocate heap - %js", oldmsg);
		return MOO_NULL;
	}

	MOO_MEMSET (heap, 0, MOO_SIZEOF(*heap) + size);
	heap->base = (moo_uint8_t*)(heap + 1);
	heap->size = size;

	heap->curspace.base= (moo_uint8_t*)(heap + 1);
	heap->curspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)heap->curspace.base), MOO_SIZEOF(moo_oop_t));
	heap->curspace.limit = heap->curspace.ptr + half_size;

	heap->newspace.base = (moo_uint8_t*)(heap + 1) + half_size;
	heap->newspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)heap->newspace.base), MOO_SIZEOF(moo_oop_t));
	heap->newspace.limit = heap->newspace.ptr + half_size;

	/* if size is too small, space->ptr may go past space->limit even at 
	 * this moment depending on the alignment of space->base. subsequent
	 * calls to moo_allocheapspace() are bound to fail. Make sure to
	 * pass a heap size large enough */

	return heap;
}

void moo_killheap (moo_t* moo, moo_heap_t* heap)
{
	moo->vmprim.free_heap (moo, heap);
}

void* moo_allocheapspace (moo_t* moo, moo_space_t* space, moo_oow_t size)
{
	moo_uint8_t* ptr;

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
