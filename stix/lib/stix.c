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
	stix->cmgr = stix_getutf8cmgr ();
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

	if (stix_rbt_init (&stix->modtab, mmgr, STIX_SIZEOF(stix_ooch_t), 1) <= -1) goto oops;
	stix_rbt_setstyle (&stix->modtab, stix_getrbtstyle(STIX_RBT_STYLE_INLINE_COPIERS));

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

static stix_rbt_walk_t unload_module (stix_rbt_t* rbt, stix_rbt_pair_t* pair, void* ctx)
{
	stix_t* stix = (stix_t*)ctx;
	stix_mod_data_t* mdp;

	mdp = STIX_RBT_VPTR(pair);
	STIX_ASSERT (mdp != STIX_NULL);

	mdp->pair = STIX_NULL; /* to prevent stix_closemod() from calling  stix_rbt_delete() */
	stix_closemod (stix, mdp);

	return STIX_RBT_WALK_FORWARD;
}

void stix_fini (stix_t* stix)
{
	stix_cb_t* cb;
	stix_oow_t i;

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

	stix_rbt_walk (&stix->modtab, unload_module, stix); /* unload all modules */
	stix_rbt_fini (&stix->modtab);

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

	for (i = 0; i < STIX_COUNTOF(stix->sbuf); i++)
	{
		if (stix->sbuf[i].ptr)
		{
			stix_freemem (stix, stix->sbuf[i].ptr);
			stix->sbuf[i].ptr = STIX_NULL;
			stix->sbuf[i].len = 0;
			stix->sbuf[i].capa = 0;
		}
	}

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

stix_cmgr_t* stix_getcmgr (stix_t* stix)
{
	return stix->cmgr;
}

void stix_setcmgr (stix_t* stix, stix_cmgr_t* cmgr)
{
	stix->cmgr = cmgr;
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

/* TODO: remove RSRS code. so far, i find this not useful */
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

#define MOD_PREFIX "stix_mod_"
#define MOD_PREFIX_LEN 9

stix_mod_data_t* stix_openmod (stix_t* stix, const stix_ooch_t* name, stix_oow_t namelen)
{
	stix_rbt_pair_t* pair;
	stix_mod_data_t* mdp;
	stix_mod_data_t md;
	stix_mod_load_t load = STIX_NULL;
#if defined(STIX_ENABLE_STATIC_MODULE)
	int n;
#endif

	/* maximum module name length is STIX_MOD_NAME_LEN_MAX. 
	 *   MOD_PREFIX_LEN for MOD_PREFIX
	 *   1 for _ at the end when stix_mod_xxx_ is attempted.
	 *   1 for the terminating '\0'.
	 */
	stix_ooch_t buf[MOD_PREFIX_LEN + STIX_MOD_NAME_LEN_MAX + 1 + 1]; 

	/* the terminating null isn't needed in buf here */
	stix_copybctooochars (buf, MOD_PREFIX, MOD_PREFIX_LEN); 

	if (namelen > STIX_COUNTOF(buf) - (MOD_PREFIX_LEN + 1 + 1))
	{
		/* module name too long  */
		stix->errnum = STIX_EINVAL; /* TODO: change the  error number to something more specific */
		return STIX_NULL;
	}

	stix_copyoochars (&buf[MOD_PREFIX_LEN], name, namelen);
	buf[MOD_PREFIX_LEN + namelen] = '\0';

#if defined(STIX_ENABLE_STATIC_MODULE)
	/* attempt to find a statically linked module */

/*TODO: CHANGE THIS PART */

	/* TODO: binary search ... */
	for (n = 0; n < STIX_COUNTOF(static_modtab); n++)
	{
		if (stix_compoocstr (static_modtab[n].modname, name, name_len....) == 0) 
		{
			load = static_modtab[n].modload;
			break;
		}
	}

	if (n >= STIX_COUNTOF(static_modtab))
	{
		stix->errnum = STIX_ENOENT;
		return STIX_NULL;
	}

	if (load)
	{
		/* found the module in the staic module table */

		STIX_MEMSET (&md, 0, STIX_SIZEOF(md));
		stix_copyoochars (md.name, name, namelen);
		/* Note md.handle is STIX_NULL for a static module */

		/* i copy-insert 'md' into the table before calling 'load'.
		 * to pass the same address to load(), query(), etc */
		pair = stix_rbt_insert (stix->modtab, name, namelen, &md, STIX_SIZEOF(md));
		if (pair == STIX_NULL)
		{
			stix->errnum = STIX_ESYSMEM;
			return STIX_NULL;
		}

		mdp = (stix_mod_data_t*)STIX_RBT_VPTR(pair);
		if (load (&mdp->mod, stix) <= -1)
		{
			stix_rbt_delete (stix->modtab, segs[0].ptr, segs[0].len);
			return STIX_NULL;
		}

		return mdp;
	}
#endif

	/* attempt to find an external module */
	STIX_MEMSET (&md, 0, STIX_SIZEOF(md));
	stix_copyoochars ((stix_ooch_t*)md.mod.name, name, namelen);
	if (stix->vmprim.dl_open && stix->vmprim.dl_getsym && stix->vmprim.dl_close)
	{
		md.handle = stix->vmprim.dl_open (stix, &buf[MOD_PREFIX_LEN]);
	}

	if (md.handle == STIX_NULL) 
	{
		STIX_DEBUG2 (stix, "Cannot open a module [%.*S]\n", namelen, name);
		stix->errnum = STIX_ENOENT; /* TODO: be more descriptive about the error */
		return STIX_NULL;
	}

	/* attempt to get stix_mod_xxx where xxx is the module name*/
	load = stix->vmprim.dl_getsym (stix, md.handle, buf);
	if (!load) 
	{
		STIX_DEBUG3 (stix, "Cannot get a module symbol [%S] in [%.*S]\n", buf, namelen, name);
		stix->errnum = STIX_ENOENT; /* TODO: be more descriptive about the error */
		stix->vmprim.dl_close (stix, md.handle);
		return STIX_NULL;
	}

	/* i copy-insert 'md' into the table before calling 'load'.
	 * to pass the same address to load(), query(), etc */
	pair = stix_rbt_insert (&stix->modtab, (void*)name, namelen, &md, STIX_SIZEOF(md));
	if (pair == STIX_NULL)
	{
		STIX_DEBUG2 (stix, "Cannot register a module [%.*S]\n", namelen, name);
		stix->errnum = STIX_ESYSMEM;
		stix->vmprim.dl_close (stix, md.handle);
		return STIX_NULL;
	}

	mdp = (stix_mod_data_t*)STIX_RBT_VPTR(pair);
	if (load (stix, &mdp->mod) <= -1)
	{
		STIX_DEBUG3 (stix, "Module function [%S] returned failure in [%.*S]\n", buf, namelen, name);
		stix->errnum = STIX_ENOENT; /* TODO: proper error code and handling */
		stix_rbt_delete (&stix->modtab, name, namelen);
		stix->vmprim.dl_close (stix, mdp->handle);
		return STIX_NULL;
	}

	mdp->pair = pair;

	STIX_DEBUG2 (stix, "Opened a module [%S] - %p\n", mdp->mod.name, mdp->handle);

	/* the module loader must ensure to set a proper query handler */
	STIX_ASSERT (mdp->mod.query != STIX_NULL);

	return mdp;
}

void stix_closemod (stix_t* stix, stix_mod_data_t* mdp)
{
	if (mdp->mod.unload) mdp->mod.unload (stix, &mdp->mod);

	if (mdp->handle) 
	{
		stix->vmprim.dl_close (stix, mdp->handle);
		STIX_DEBUG2 (stix, "Closed a module [%S] - %p\n", mdp->mod.name, mdp->handle);
		mdp->handle = STIX_NULL;
	}

	if (mdp->pair)
	{
		/*mdp->pair = STIX_NULL;*/ /* this reset isn't needed as the area will get freed by stix_rbt_delete()) */
		stix_rbt_delete (&stix->modtab, mdp->mod.name, stix_countoocstr(mdp->mod.name));
	}
}

int stix_importmod (stix_t* stix, stix_oop_t _class, const stix_ooch_t* name, stix_oow_t len)
{
	stix_rbt_pair_t* pair;
	stix_mod_data_t* mdp;
	int r = 0;

	/* stix_openmod(), stix_closemod(), etc calls a user-defined callback.
	 * i need to protect _class in case the user-defined callback allocates 
	 * a OOP memory chunk and GC occurs */
	stix_pushtmp (stix, &_class);

	pair = stix_rbt_search (&stix->modtab, name, len);
	if (pair)
	{
		mdp = (stix_mod_data_t*)STIX_RBT_VPTR(pair);
		STIX_ASSERT (mdp != STIX_NULL);
	}
	else
	{
		mdp = stix_openmod (stix, name, len);
		if (!mdp) 
		{
			r = -1;
			goto done;
		}
	}

	if (!mdp->mod.import)
	{
		STIX_DEBUG1 (stix, "Cannot import module [%S] - importing not supported by the module\n", mdp->mod.name);
		stix->errnum = STIX_ENOIMPL;
		r = -1;
		goto done;
	}

	if (mdp->mod.import (stix, &mdp->mod, _class) <= -1)
	{
		STIX_DEBUG1 (stix, "Cannot import module [%S] - module's import() returned failure\n", mdp->mod.name);
		r = -1;
		goto done;
	}

done:
	if (!pair) 
	{
		/* clsoe the module if it has been opened in this function. */
		stix_closemod (stix, mdp);
	}

	stix_poptmp (stix);
	return r;
}

stix_pfimpl_t stix_querymod (stix_t* stix, const stix_ooch_t* pfid, stix_oow_t pfidlen)
{
	/* primitive function identifier
	 *   _funcname
	 *   modname_funcname
	 */
	stix_rbt_pair_t* pair;
	stix_mod_data_t* mdp;
	const stix_ooch_t* sep;

	stix_oow_t mod_name_len;
	stix_pfimpl_t handler;

	sep = stix_findoochar (pfid, pfidlen, '.');
	if (!sep)
	{
		/* i'm writing a conservative code here. the compiler should 
		 * guarantee that an underscore is included in an primitive identifer.
		 * what if the compiler is broken? imagine a buggy compiler rewritten
		 * in stix itself? */
		STIX_DEBUG2 (stix, "Internal error - no period in a primitive function identifier [%.*S] - buggy compiler?\n", pfidlen, pfid);
		stix->errnum = STIX_EINTERN;
		return STIX_NULL;
	}

	mod_name_len = sep - pfid;

	pair = stix_rbt_search (&stix->modtab, pfid, mod_name_len);
	if (pair)
	{
		mdp = (stix_mod_data_t*)STIX_RBT_VPTR(pair);
		STIX_ASSERT (mdp != STIX_NULL);
	}
	else
	{
		mdp = stix_openmod (stix, pfid, mod_name_len);
		if (!mdp) return STIX_NULL;
	}

	if ((handler = mdp->mod.query (stix, &mdp->mod, sep + 1)) == STIX_NULL) 
	{
		/* the primitive function is not found. keep the module open */
		STIX_DEBUG2 (stix, "Cannot find a primitive function [%S] in a module [%S]\n", sep + 1, mdp->mod.name);
		stix->errnum = STIX_ENOENT; /* TODO: proper error code and handling */
		return STIX_NULL;
	}

	STIX_DEBUG3 (stix, "Found a primitive function [%S] in a module [%S] - %p\n", sep + 1, mdp->mod.name, handler);
	return handler;
}

/* -------------------------------------------------------------------------- */

/* add a new primitive method */
int stix_genpfmethod (stix_t* stix, stix_mod_t* mod, stix_oop_t _class, stix_method_type_t type, const stix_ooch_t* mthname, int variadic, const stix_ooch_t* pfname)
{
	/* NOTE: this function is a subset of add_compiled_method() in comp.c */

	stix_oop_char_t mnsym, pfidsym;
	stix_oop_method_t mth;
	stix_oop_class_t cls;
	stix_oow_t tmp_count = 0, i;
	stix_ooi_t arg_count = 0;
	stix_oocs_t cs;
	stix_ooi_t preamble_flags = 0;
	static stix_ooch_t dot[] = { '.', '\0' };

	STIX_ASSERT (STIX_CLASSOF(stix, _class) == stix->_class);

	if (!pfname) pfname = mthname;

	cls = (stix_oop_class_t)_class;
	stix_pushtmp (stix, (stix_oop_t*)&cls); tmp_count++;
	STIX_ASSERT (STIX_CLASSOF(stix, (stix_oop_t)cls->mthdic[type]) == stix->_method_dictionary);

	for (i = 0; mthname[i]; i++)
	{
		if (mthname[i] == ':') 
		{
			if (variadic) goto oops_inval;
			arg_count++;
		}
	}
/* TODO: check if name is a valid method name - more checks... */
/* TOOD: if the method name is a binary selector, it can still have an argument.. so the check below is invalid... */
	if (arg_count > 0 && mthname[i - 1] != ':') 
	{
	oops_inval:
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - invalid name\n", mthname, cls->name);
		stix->errnum = STIX_EINVAL;
		goto oops;
	}

	cs.ptr = (stix_ooch_t*)mthname;
	cs.len = i;
	if (stix_lookupdic (stix, cls->mthdic[type], &cs) != STIX_NULL)
	{
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - duplicate\n", mthname, cls->name);
		stix->errnum = STIX_EEXIST;
		goto oops;
	}

	mnsym = (stix_oop_char_t)stix_makesymbol (stix, mthname, i);
	if (!mnsym) goto oops;
	stix_pushtmp (stix, (stix_oop_t*)&mnsym); tmp_count++;

	/* compose a full primitive function identifier to VM's string buffer.
	 *   pfid => mod->name + '.' + pfname */
	if (stix_copyoocstrtosbuf(stix, mod->name, 0) <= -1 ||
	    stix_concatoocstrtosbuf(stix, dot, 0) <= -1 ||
	    stix_concatoocstrtosbuf(stix, pfname, 0) <=  -1) 
	{
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - VM memory shortage\n", mthname, cls->name);
		return -1;
	}

	pfidsym = (stix_oop_char_t)stix_makesymbol (stix, stix->sbuf[0].ptr, stix->sbuf[0].len);
	if (!pfidsym) 
	{
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - symbol instantiation failure\n", mthname, cls->name);
		goto oops;
	}
	stix_pushtmp (stix, (stix_oop_t*)&pfidsym); tmp_count++;

#if defined(STIX_USE_OBJECT_TRAILER)
	mth = (stix_oop_method_t)stix_instantiatewithtrailer (stix, stix->_method, 1, STIX_NULL, 0); 
#else
	mth = (stix_oop_method_t)stix_instantiate (stix, stix->_method, STIX_NULL, 1);
#endif
	if (!mth)
	{
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - method instantiation failure\n", mthname, cls->name);
		goto oops;
	}

	/* store the primitive function name symbol to the literal frame */
	mth->slot[0] = (stix_oop_t)pfidsym;

	/* premable should contain the index to the literal frame which is always 0 */
	mth->owner = cls;
	mth->name = mnsym;
	if (variadic) preamble_flags |= STIX_METHOD_PREAMBLE_FLAG_VARIADIC;
	mth->preamble = STIX_SMOOI_TO_OOP(STIX_METHOD_MAKE_PREAMBLE(STIX_METHOD_PREAMBLE_NAMED_PRIMITIVE, 0, preamble_flags));
	mth->preamble_data[0] = STIX_SMOOI_TO_OOP(0);
	mth->preamble_data[1] = STIX_SMOOI_TO_OOP(0);
	mth->tmpr_count = STIX_SMOOI_TO_OOP(arg_count);
	mth->tmpr_nargs = STIX_SMOOI_TO_OOP(arg_count);

/* TODO: emit BCODE_RETURN_NIL ? */

	if (!stix_putatdic (stix, cls->mthdic[type], (stix_oop_t)mnsym, (stix_oop_t)mth)) 
	{
		STIX_DEBUG2 (stix, "Cannot generate primitive function method [%S] in [%O] - failed to add to method dictionary\n", mthname, cls->name);
		goto oops;
	}

	STIX_DEBUG2 (stix, "Generated primitive function method [%S] in [%O]\n", mthname, cls->name);

	stix_poptmps (stix, tmp_count); tmp_count = 0;
	return 0;

oops:
	stix_poptmps (stix, tmp_count);
	return -1;
}
 
