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


stix_t* stix_open (stix_mmgr_t* mmgr, stix_size_t xtnsize, stix_size_t heapsize, const stix_vmprim_t* vmprim, stix_errnum_t* errnum)
{
	stix_t* stix;

	/* if this assertion fails, correct the type definition in stix.h */
	STIX_ASSERT (STIX_SIZEOF(stix_oow_t) == STIX_SIZEOF(stix_oop_t));

	stix = STIX_MMGR_ALLOC (mmgr, STIX_SIZEOF(*stix) + xtnsize);
	if (stix)
	{
		if (stix_init(stix, mmgr, heapsize, vmprim) <= -1)
		{
			if (errnum) *errnum = stix->errnum;
			STIX_MMGR_FREE (mmgr, stix);
			stix = STIX_NULL;
		}
		else STIX_MEMSET (stix + 1, 0, xtnsize);
	}
	else if (errnum) *errnum = STIX_ENOMEM;

	return stix;
}

void stix_close (stix_t* stix)
{
	stix_fini (stix);
	STIX_MMGR_FREE (stix->mmgr, stix);
}

int stix_init (stix_t* stix, stix_mmgr_t* mmgr, stix_size_t heapsz, const stix_vmprim_t* vmprim)
{
	STIX_MEMSET (stix, 0, STIX_SIZEOF(*stix));
	stix->mmgr = mmgr;
	stix->vmprim = *vmprim;

	/*stix->permheap = stix_makeheap (stix, what is the best size???);
	if (!stix->curheap) goto oops; */
	stix->curheap = stix_makeheap (stix, heapsz);
	if (!stix->curheap) goto oops;
	stix->newheap = stix_makeheap (stix, heapsz);
	if (!stix->newheap) goto oops;

	if (stix_rbt_init (&stix->pmtable, mmgr, STIX_SIZEOF(stix_uch_t), 1) <= -1) goto oops;
	stix_rbt_setstyle (&stix->pmtable, stix_getrbtstyle(STIX_RBT_STYLE_INLINE_COPIERS));
	return 0;

oops:
	if (stix->newheap) stix_killheap (stix, stix->newheap);
	if (stix->curheap) stix_killheap (stix, stix->curheap);
	if (stix->permheap) stix_killheap (stix, stix->permheap);
	return -1;
}

void stix_fini (stix_t* stix)
{
	stix_cb_t* cb;

	for (cb = stix->cblist; cb; cb = cb->next)
	{
		if (cb->fini) cb->fini (stix);
	}

	stix_rbt_fini (&stix->pmtable);

	stix_killheap (stix, stix->newheap);
	stix_killheap (stix, stix->curheap);
	stix_killheap (stix, stix->permheap);

	/* deregister all callbacks */
	while (stix->cblist) stix_deregcb (stix, stix->cblist);
}

stix_mmgr_t* stix_getmmgr (stix_t* stix)
{
	return stix->mmgr;
}

void* stix_getxtn (stix_t* stix)
{
	return (void*)(stix + 1);
}


stix_errnum_t stix_geterrnum (stix_t* stix)
{
	return stix->errnum;
}

void stix_seterrnum (stix_t* stix, stix_errnum_t errnum)
{
	stix->errnum = errnum;
}

int stix_setoption (stix_t* stix, stix_option_t id, const void* value)
{
	switch (id)
	{
		case STIX_TRAIT:
			stix->option.trait = *(const int*)value;
			return 0;

		case STIX_SYMTAB_SIZE:
			stix->option.dfl_symtab_size = *(stix_oow_t*)value;
			return 0;

		case STIX_SYSDIC_SIZE:
			stix->option.dfl_sysdic_size = *(stix_oow_t*)value;
			return 0;

		case STIX_PROCSTK_SIZE:
			stix->option.dfl_procstk_size = *(stix_oow_t*)value;
			return 0;
	}

	stix->errnum = STIX_EINVAL;
	return -1;
}

int stix_getoption (stix_t* stix, stix_option_t id, void* value)
{
	switch  (id)
	{
		case STIX_TRAIT:
			*(int*)value = stix->option.trait;
			return 0;

		case STIX_SYMTAB_SIZE:
			*(stix_oow_t*)value = stix->option.dfl_symtab_size;
			return 0;

		case STIX_SYSDIC_SIZE:
			*(stix_oow_t*)value = stix->option.dfl_sysdic_size;
			return 0;

		case STIX_PROCSTK_SIZE:
			*(stix_oow_t*)value = stix->option.dfl_procstk_size;
			return 0;
	};

	stix->errnum = STIX_EINVAL;
	return -1;
}


stix_cb_t* stix_regcb (stix_t* stix, stix_cb_t* tmpl)
{
	stix_cb_t* actual;

	actual = stix_allocmem (stix, STIX_SIZEOF(*actual));
	if (!actual) return STIX_NULL;

	*actual = *tmpl;
	actual->next = stix->cblist;
	actual->prev = STIX_NULL;
	stix->cblist = actual;

	return actual;
}

void stix_deregcb (stix_t* stix, stix_cb_t* cb)
{
	if (cb == stix->cblist)
	{
		stix->cblist = stix->cblist->next;
		if (stix->cblist) stix->cblist->prev = STIX_NULL;
	}
	else
	{
		if (cb->next) cb->next->prev = cb->prev;
		if (cb->prev) cb->prev->next = cb->next;
	}

	stix_freemem (stix, cb);
}

stix_size_t stix_hashbytes (const stix_byte_t* ptr, stix_size_t len)
{
	stix_size_t h = 0;
	const stix_uint8_t* bp, * be;

	bp = ptr; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

stix_size_t stix_hashuchars (const stix_uch_t* ptr, stix_size_t len)
{
	return stix_hashbytes ((const stix_byte_t *)ptr, len * STIX_SIZEOF(*ptr));
}


void* stix_allocmem (stix_t* stix, stix_size_t size)
{
	void* ptr;

	ptr = STIX_MMGR_ALLOC (stix->mmgr, size);
	if (!ptr) stix->errnum = STIX_ENOMEM;
	return ptr;
}

void* stix_callocmem (stix_t* stix, stix_size_t size)
{
	void* ptr;

	ptr = STIX_MMGR_ALLOC (stix->mmgr, size);
	if (!ptr) stix->errnum = STIX_ENOMEM;
	else STIX_MEMSET (ptr, 0, size);
	return ptr;
}

void* stix_reallocmem (stix_t* stix, void* ptr, stix_size_t size)
{
	ptr = STIX_MMGR_REALLOC (stix->mmgr, ptr, size);
	if (!ptr) stix->errnum = STIX_ENOMEM;
	return ptr;
}

void stix_freemem (stix_t* stix, void* ptr)
{
	STIX_MMGR_FREE (stix->mmgr, ptr);
}
