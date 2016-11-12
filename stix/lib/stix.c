/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

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


stix_t* stix_open (stix_mmgr_t* mmgr, stix_oow_t xtnsize, stix_oow_t heapsize, const stix_vmprim_t* vmprim, stix_errnum_t* errnum)
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
	else if (errnum) *errnum = STIX_ESYSMEM;

	return stix;
}

void stix_close (stix_t* stix)
{
	stix_fini (stix);
	STIX_MMGR_FREE (stix->mmgr, stix);
}

static void fill_bigint_tables (stix_t* stix)
{
	int radix, safe_ndigits;
	stix_oow_t ub, w;
	stix_oow_t multiplier;

	for (radix = 2; radix <= 36; radix++)
	{
		w = 0;
		ub = (stix_oow_t)STIX_TYPE_MAX(stix_liw_t) / radix - (radix - 1);
		multiplier = 1;
		safe_ndigits = 0;

		while (w <= ub)
		{
			w = w * radix + (radix - 1);
			multiplier *= radix;
			safe_ndigits++;
		}

		/* safe_ndigits contains the number of digits that never
		 * cause overflow when computed normally with a native type. */
		stix->bigint[radix].safe_ndigits = safe_ndigits;
		stix->bigint[radix].multiplier = multiplier;
	}
}

int stix_init (stix_t* stix, stix_mmgr_t* mmgr, stix_oow_t heapsz, const stix_vmprim_t* vmprim)
{
	STIX_MEMSET (stix, 0, STIX_SIZEOF(*stix));
	stix->mmgr = mmgr;
	stix->vmprim = *vmprim;

	stix->option.log_mask = ~0u;
	stix->option.dfl_symtab_size = STIX_DFL_SYMTAB_SIZE;
	stix->option.dfl_sysdic_size = STIX_DFL_SYSDIC_SIZE;
	stix->option.dfl_procstk_size = STIX_DFL_PROCSTK_SIZE;

/* TODO: intoduct a permanent heap */
	/*stix->permheap = stix_makeheap (stix, what is the best size???);
	if (!stix->permheap) goto oops; */
	stix->curheap = stix_makeheap (stix, heapsz);
	if (!stix->curheap) goto oops;
	stix->newheap = stix_makeheap (stix, heapsz);
	if (!stix->newheap) goto oops;

	if (stix_rbt_init (&stix->pmtable, mmgr, STIX_SIZEOF(stix_ooch_t), 1) <= -1) goto oops;
	stix_rbt_setstyle (&stix->pmtable, stix_getrbtstyle(STIX_RBT_STYLE_INLINE_COPIERS));

	fill_bigint_tables (stix);

	stix->tagged_classes[STIX_OOP_TAG_SMINT] = &stix->_small_integer;
	stix->tagged_classes[STIX_OOP_TAG_CHAR] = &stix->_character;
	stix->tagged_classes[STIX_OOP_TAG_RSRC] = &stix->_resource;

	return 0;

oops:
	if (stix->newheap) stix_killheap (stix, stix->newheap);
	if (stix->curheap) stix_killheap (stix, stix->curheap);
	if (stix->permheap) stix_killheap (stix, stix->permheap);
	return -1;
}

static stix_rbt_walk_t unload_primitive_module (stix_rbt_t* rbt, stix_rbt_pair_t* pair, void* ctx)
{
	stix_t* stix = (stix_t*)ctx;
	stix_prim_mod_data_t* md;

	md = STIX_RBT_VPTR(pair);
	if (md->mod.unload) md->mod.unload (stix, &md->mod);
	if (md->handle) stix->vmprim.mod_close (stix, md->handle);

	return STIX_RBT_WALK_FORWARD;
}

void stix_fini (stix_t* stix)
{
	stix_cb_t* cb;

	if (stix->sem_list)
	{
		stix_freemem (stix, stix->sem_list);
		stix->sem_list_capa = 0;
		stix->sem_list_count = 0;
	}

	if (stix->sem_heap)
	{
		stix_freemem (stix, stix->sem_heap);
		stix->sem_heap_capa = 0;
		stix->sem_heap_count = 0;
	}

	for (cb = stix->cblist; cb; cb = cb->next)
	{
		if (cb->fini) cb->fini (stix);
	}

	stix_rbt_walk (&stix->pmtable, unload_primitive_module, stix);
	stix_rbt_fini (&stix->pmtable);

/* TOOD: persistency? storing objects to image file? */
	stix_killheap (stix, stix->newheap);
	stix_killheap (stix, stix->curheap);
	stix_killheap (stix, stix->permheap);

	if (stix->rsrc.ptr)
	{
		stix_freemem (stix, stix->rsrc.ptr);
		stix->rsrc.free = 0;
		stix->rsrc.capa = 0;
		stix->rsrc.ptr = STIX_NULL;
	}

	/* deregister all callbacks */
	while (stix->cblist) stix_deregcb (stix, stix->cblist);

	if (stix->log.ptr) 
	{
		/* make sure to flush your log message */
/* TODO: flush unwritten message */

		stix_freemem (stix, stix->log.ptr);
		stix->log.capa = 0;
		stix->log.len = 0;
	}
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
			stix->option.trait = *(const unsigned int*)value;
			return 0;

		case STIX_LOG_MASK:
			stix->option.log_mask = *(const unsigned int*)value;
			return 0;

		case STIX_SYMTAB_SIZE:
		{
			stix_oow_t w;

			w = *(stix_oow_t*)value;
			if (w <= 0 || w > STIX_SMOOI_MAX) goto einval;

			stix->option.dfl_symtab_size = *(stix_oow_t*)value;
			return 0;
		}

		case STIX_SYSDIC_SIZE:
		{
			stix_oow_t w;

			w = *(stix_oow_t*)value;
			if (w <= 0 || w > STIX_SMOOI_MAX) goto einval;

			stix->option.dfl_sysdic_size = *(stix_oow_t*)value;
			return 0;
		}

		case STIX_PROCSTK_SIZE:
		{
			stix_oow_t w;

			w = *(stix_oow_t*)value;
			if (w <= 0 || w > STIX_SMOOI_MAX) goto einval;

			stix->option.dfl_procstk_size = *(stix_oow_t*)value;
			return 0;
		}
	}

einval:
	stix->errnum = STIX_EINVAL;
	return -1;
}

int stix_getoption (stix_t* stix, stix_option_t id, void* value)
{
	switch  (id)
	{
		case STIX_TRAIT:
			*(unsigned int*)value = stix->option.trait;
			return 0;

		case STIX_LOG_MASK:
			*(unsigned int*)value = stix->option.log_mask;
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

void* stix_allocmem (stix_t* stix, stix_oow_t size)
{
	void* ptr;

	ptr = STIX_MMGR_ALLOC (stix->mmgr, size);
	if (!ptr) stix->errnum = STIX_ESYSMEM;
	return ptr;
}

void* stix_callocmem (stix_t* stix, stix_oow_t size)
{
	void* ptr;

	ptr = STIX_MMGR_ALLOC (stix->mmgr, size);
	if (!ptr) stix->errnum = STIX_ESYSMEM;
	else STIX_MEMSET (ptr, 0, size);
	return ptr;
}

void* stix_reallocmem (stix_t* stix, void* ptr, stix_oow_t size)
{
	ptr = STIX_MMGR_REALLOC (stix->mmgr, ptr, size);
	if (!ptr) stix->errnum = STIX_ESYSMEM;
	return ptr;
}

void stix_freemem (stix_t* stix, void* ptr)
{
	STIX_MMGR_FREE (stix->mmgr, ptr);
}



/* -------------------------------------------------------------------------- */

stix_oop_t stix_makersrc (stix_t* stix, stix_oow_t v)
{
	stix_oop_t imm;
	stix_oow_t avail;

	if (stix->rsrc.free >= stix->rsrc.capa)
	{
		stix_oow_t* tmp;
		stix_ooi_t newcapa, i;

		newcapa = stix->rsrc.capa + 256;

		tmp = stix_reallocmem (stix, stix->rsrc.ptr, STIX_SIZEOF(*tmp) * newcapa);
		if (!tmp) return STIX_NULL;

		for (i = stix->rsrc.capa; i < newcapa; i++) tmp[i] = i + 1;
		stix->rsrc.free = i;
		stix->rsrc.ptr = tmp;
		stix->rsrc.capa = newcapa;
	}

	avail = stix->rsrc.free;
	stix->rsrc.free = stix->rsrc.ptr[stix->rsrc.free];
	stix->rsrc.ptr[avail] = v;

	/* there must not be too many immedates in the whole system. */
	STIX_ASSERT (STIX_IN_SMOOI_RANGE(avail));
	return STIX_RSRC_TO_OOP(avail);

	return imm;
}

void stix_killrsrc (stix_t* stix, stix_oop_t imm)
{
	if (STIX_OOP_IS_RSRC(stix))
	{
		stix_ooi_t v;

		v = STIX_OOP_TO_RSRC(stix);

		/* the value of v, if properly created by stix_makeimm(), must
		 * fall in the following range. when storing and loading the values
		 * from an image file, you must take extra care not to break this
		 * assertion */
		STIX_ASSERT (v >= 0 && v < stix->rsrc.capa);
		stix->rsrc.ptr[v] = stix->rsrc.free;
		stix->rsrc.free = v;
	}
}

stix_oow_t stix_getrsrcval (stix_t* stix, stix_oop_t imm)
{
	STIX_ASSERT (STIX_OOP_IS_RSRC(imm));
	return stix->rsrc.ptr[STIX_OOP_TO_RSRC(imm)];
}

/* -------------------------------------------------------------------------- */

/* add a new primitive method */
int stix_addmethod (stix_t* stix, stix_oop_t _class, const stix_ooch_t* name, stix_prim_impl_t func)
{
	/* NOTE: this function is a subset of add_compiled_method() in comp.c */

	stix_oop_char_t nsym;
	stix_oop_method_t mth;
	stix_oop_class_t cls;
	stix_oow_t tmp_count = 0, i;
	stix_ooi_t arg_count = 0;

	STIX_ASSERT (STIX_CLASSOF(stix, _class) == stix->_class);

	cls = (stix_oop_class_t)_class;

	/* TODO: check if name is a valid method name */
	for (i = 0; name[i]; i++)
	{
		if (name[i] == ':') arg_count++;
	}
	nsym = (stix_oop_char_t)stix_makesymbol (stix, name, i);
	if (!nsym) return -1;

	stix_pushtmp (stix, (stix_oop_t*)&name); tmp_count++;

#if defined(STIX_USE_OBJECT_TRAILER)
	mth = (stix_oop_method_t)stix_instantiatewithtrailer (stix, stix->_method, 1, STIX_NULL, 0); 
#else
	mth = (stix_oop_method_t)stix_instantiate (stix, stix->_method, STIX_NULL, 1);
#endif
	if (!mth) goto oops;

	/* store the symbol name to the literal frame */
	mth->slot[0] = (stix_oop_t)nsym;

	/* add the primitive as a name primitive with index of -1.
	 * set the preamble_data to the pointer to the primitive function. */
	mth->owner = cls;
	mth->name = nsym;
	mth->preamble = STIX_SMOOI_TO_OOP(STIX_METHOD_MAKE_PREAMBLE(STIX_METHOD_PREAMBLE_NAMED_PRIMITIVE, -1));
	mth->preamble_data[0] = STIX_SMOOI_TO_OOP((stix_oow_t)func >> (STIX_OOW_BITS / 2));
	mth->preamble_data[1] = STIX_SMOOI_TO_OOP((stix_oow_t)func & STIX_LBMASK(stix_oow_t, STIX_OOW_BITS / 2));
	mth->tmpr_count = STIX_SMOOI_TO_OOP(arg_count);
	mth->tmpr_nargs = STIX_SMOOI_TO_OOP(arg_count);
	stix_poptmps (stix, tmp_count); tmp_count = 0;

/* TODO: class method? */
	/* instance method */
	if (!stix_putatdic (stix, cls->mthdic[STIX_CLASS_MTHDIC_INSTANCE], (stix_oop_t)nsym, (stix_oop_t)mth)) goto oops;
	return 0;

oops:
	stix_poptmps (stix, tmp_count);
	return -1;
}
 
