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

moo_t* moo_open (moo_mmgr_t* mmgr, moo_oow_t xtnsize, moo_oow_t heapsize, const moo_vmprim_t* vmprim, moo_errnum_t* errnum)
{
	moo_t* moo;

	/* if this assertion fails, correct the type definition in moo.h */
	MOO_ASSERT (moo, MOO_SIZEOF(moo_oow_t) == MOO_SIZEOF(moo_oop_t));

	moo = (moo_t*)MOO_MMGR_ALLOC(mmgr, MOO_SIZEOF(*moo) + xtnsize);
	if (moo)
	{
		if (moo_init(moo, mmgr, heapsize, vmprim) <= -1)
		{
			if (errnum) *errnum = moo->errnum;
			MOO_MMGR_FREE (mmgr, moo);
			moo = MOO_NULL;
		}
		else MOO_MEMSET (moo + 1, 0, xtnsize);
	}
	else if (errnum) *errnum = MOO_ESYSMEM;

	return moo;
}

void moo_close (moo_t* moo)
{
	moo_fini (moo);
	MOO_MMGR_FREE (moo->mmgr, moo);
}

static void fill_bigint_tables (moo_t* moo)
{
	int radix, safe_ndigits;
	moo_oow_t ub, w;
	moo_oow_t multiplier;

	for (radix = 2; radix <= 36; radix++)
	{
		w = 0;
		ub = (moo_oow_t)MOO_TYPE_MAX(moo_liw_t) / radix - (radix - 1);
		multiplier = 1;
		safe_ndigits = 0;

		while (w <= ub)
		{
			w = w * radix + (radix - 1);
			multiplier *= radix;
			safe_ndigits++;
		}

		/* safe_ndigits contains the number of digits that never
		 * cause overflow when computed normally with a primitive type. */
		moo->bigint[radix].safe_ndigits = safe_ndigits;
		moo->bigint[radix].multiplier = multiplier;
	}
}

static void* alloc_heap (moo_t* moo, moo_oow_t size)
{
	return MOO_MMGR_ALLOC(moo->mmgr, size);
}

static void free_heap (moo_t* moo, void* ptr)
{
	MOO_MMGR_FREE(moo->mmgr, ptr);
}

int moo_init (moo_t* moo, moo_mmgr_t* mmgr, moo_oow_t heapsz, const moo_vmprim_t* vmprim)
{
	int modtab_inited = 0;

	if (!vmprim->syserrstrb && !vmprim->syserrstru)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		return -1;
	}

	MOO_MEMSET (moo, 0, MOO_SIZEOF(*moo));
	moo->mmgr = mmgr;
	moo->cmgr = moo_get_utf8_cmgr();
	moo->vmprim = *vmprim;
	if (!moo->vmprim.alloc_heap) moo->vmprim.alloc_heap = alloc_heap;
	if (!moo->vmprim.free_heap) moo->vmprim.free_heap = free_heap;

	moo->option.log_mask = ~0u;
	moo->option.log_maxcapa = MOO_DFL_LOG_MAXCAPA;
	moo->option.dfl_symtab_size = MOO_DFL_SYMTAB_SIZE;
	moo->option.dfl_sysdic_size = MOO_DFL_SYSDIC_SIZE;
	moo->option.dfl_procstk_size = MOO_DFL_PROCSTK_SIZE;
#if defined(MOO_BUILD_DEBUG)
	moo->option.karatsuba_cutoff = MOO_KARATSUBA_CUTOFF;
#endif

	moo->log.capa = MOO_ALIGN_POW2(1, MOO_LOG_CAPA_ALIGN); /* TODO: is this a good initial size? */
	/* alloate the log buffer in advance though it may get reallocated
	 * in put_oocs and put_ooch in logfmt.c. this is to let the logging
	 * routine still function despite some side-effects when
	 * reallocation fails */
	/* +1 required for consistency with put_oocs and put_ooch in logfmt.c */
	moo->log.ptr = moo_allocmem (moo, (moo->log.capa + 1) * MOO_SIZEOF(*moo->log.ptr)); 
	if (!moo->log.ptr) goto oops;

	if (moo_rbt_init (&moo->modtab, moo, MOO_SIZEOF(moo_ooch_t), 1) <= -1) goto oops;
	modtab_inited = 1;
	moo_rbt_setstyle (&moo->modtab, moo_getrbtstyle(MOO_RBT_STYLE_INLINE_COPIERS));

	fill_bigint_tables (moo);

	moo->tagged_classes[MOO_OOP_TAG_SMOOI] = &moo->_small_integer;
	moo->tagged_classes[MOO_OOP_TAG_SMPTR] = &moo->_small_pointer;
	moo->tagged_classes[MOO_OOP_TAG_CHAR] = &moo->_character;
	moo->tagged_classes[MOO_OOP_TAG_ERROR] = &moo->_error_class;

	moo->proc_map_free_first = -1;
	moo->proc_map_free_last = -1;

/* TODO: introduce a permanent heap */
	/*moo->permheap = moo_makeheap (moo, what is the best size???);
	if (!moo->permheap) goto oops; */
	moo->curheap = moo_makeheap (moo, heapsz);
	if (!moo->curheap) goto oops;
	moo->newheap = moo_makeheap (moo, heapsz);
	if (!moo->newheap) goto oops;

	return 0;

oops:
	if (modtab_inited) moo_rbt_fini (&moo->modtab);
	if (moo->newheap) moo_killheap (moo, moo->newheap);
	if (moo->curheap) moo_killheap (moo, moo->curheap);
	if (moo->permheap) moo_killheap (moo, moo->permheap);
	if (moo->log.ptr) moo_freemem (moo, moo->log.ptr);
	moo->log.capa = 0;
	return -1;
}

static moo_rbt_walk_t unload_module (moo_rbt_t* rbt, moo_rbt_pair_t* pair, void* ctx)
{
	moo_t* moo = (moo_t*)ctx;
	moo_mod_data_t* mdp;

	mdp = MOO_RBT_VPTR(pair);
	MOO_ASSERT (moo, mdp != MOO_NULL);

	mdp->pair = MOO_NULL; /* to prevent moo_closemod() from calling  moo_rbt_delete() */
	moo_closemod (moo, mdp);

	return MOO_RBT_WALK_FORWARD;
}

void moo_fini (moo_t* moo)
{
	moo_cb_t* cb;
	moo_oow_t i;

	moo_rbt_walk (&moo->modtab, unload_module, moo); /* unload all modules */
	moo_rbt_fini (&moo->modtab);

	if (moo->log.len > 0)
	{
		/* flush pending log messages just in case. */
		MOO_ASSERT (moo, moo->log.ptr != MOO_NULL);
		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
	}

	for (cb = moo->cblist; cb; cb = cb->next)
	{
		/* execute callbacks for finalization */
		if (cb->fini) cb->fini (moo);
	}

	if (moo->log.len > 0)
	{
		/* flush pending log message that could be generated by the fini 
		 * callbacks. however, the actual logging might not be produced at
		 * this point because one of the callbacks could arrange to stop
		 * logging */
		MOO_ASSERT (moo, moo->log.ptr != MOO_NULL);
		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
	}

	/* deregister all callbacks */
	while (moo->cblist) moo_deregcb (moo, moo->cblist);

	/* free up internal data structures. this is done after all callbacks
	 * are completed */
	if (moo->sem_list)
	{
		moo_freemem (moo, moo->sem_list);
		moo->sem_list_capa = 0;
		moo->sem_list_count = 0;
	}

	if (moo->sem_heap)
	{
		moo_freemem (moo, moo->sem_heap);
		moo->sem_heap_capa = 0;
		moo->sem_heap_count = 0;
	}

	if (moo->sem_io_tuple)
	{
		moo_freemem (moo, moo->sem_io_tuple);
		moo->sem_io_tuple_capa = 0;
		moo->sem_io_tuple_count = 0;
	}

	if (moo->sem_io_map)
	{
		moo_freemem (moo, moo->sem_io_map);
		moo->sem_io_map_capa = 0;
	}

	if (moo->proc_map)
	{
		moo_freemem (moo, moo->proc_map);
		moo->proc_map_capa = 0;
		moo->proc_map_free_first = -1;
		moo->proc_map_free_last = -1;
	}

/* TOOD: persistency? storing objects to image file? */

	moo_killheap (moo, moo->newheap);
	moo_killheap (moo, moo->curheap);
	if (moo->permheap) moo_killheap (moo, moo->permheap);

	for (i = 0; i < MOO_COUNTOF(moo->sbuf); i++)
	{
		if (moo->sbuf[i].ptr)
		{
			moo_freemem (moo, moo->sbuf[i].ptr);
			moo->sbuf[i].ptr = MOO_NULL;
			moo->sbuf[i].len = 0;
			moo->sbuf[i].capa = 0;
		}
	}

	if (moo->log.ptr) 
	{
		moo_freemem (moo, moo->log.ptr);
		moo->log.capa = 0;
		moo->log.len = 0;
	}

	if (moo->inttostr.xbuf.ptr)
	{
		moo_freemem (moo, moo->inttostr.xbuf.ptr);
		moo->inttostr.xbuf.ptr = MOO_NULL;
		moo->inttostr.xbuf.capa = 0;
		moo->inttostr.xbuf.len = 0;
	}
 
	if (moo->inttostr.t.ptr)
	{
		moo_freemem (moo, moo->inttostr.t.ptr);
		moo->inttostr.t.ptr = MOO_NULL;
		moo->inttostr.t.capa = 0;
	}
}

int moo_setoption (moo_t* moo, moo_option_t id, const void* value)
{
	switch (id)
	{
		case MOO_TRAIT:
			moo->option.trait = *(moo_bitmask_t*)value;
		#if defined(MOO_BUILD_DEBUG)
			moo->option.karatsuba_cutoff = ((moo->option.trait & MOO_DEBUG_BIGINT)? MOO_KARATSUBA_CUTOFF_DEBUG: MOO_KARATSUBA_CUTOFF);
		#endif
			return 0;

		case MOO_LOG_MASK:
			moo->option.log_mask = *(moo_bitmask_t*)value;
			return 0;

		case MOO_LOG_MAXCAPA:
			moo->option.log_maxcapa = *(moo_oow_t*)value;
			return 0;

		case MOO_SYMTAB_SIZE:
		{
			moo_oow_t w;

			w = *(moo_oow_t*)value;
			if (w <= 0 || w > MOO_SMOOI_MAX) goto einval;

			moo->option.dfl_symtab_size = *(moo_oow_t*)value;
			return 0;
		}

		case MOO_SYSDIC_SIZE:
		{
			moo_oow_t w;

			w = *(moo_oow_t*)value;
			if (w <= 0 || w > MOO_SMOOI_MAX) goto einval;

			moo->option.dfl_sysdic_size = *(moo_oow_t*)value;
			return 0;
		}

		case MOO_PROCSTK_SIZE:
		{
			moo_oow_t w;

			w = *(moo_oow_t*)value;
			if (w <= 0 || w > MOO_SMOOI_MAX) goto einval;

			moo->option.dfl_procstk_size = *(moo_oow_t*)value;
			return 0;
		}

		case MOO_MOD_INCTX:
			moo->option.mod_inctx = *(void**)value;
			return 0;
	}

einval:
	moo_seterrnum (moo, MOO_EINVAL);
	return -1;
}

int moo_getoption (moo_t* moo, moo_option_t id, void* value)
{
	switch  (id)
	{
		case MOO_TRAIT:
			*(moo_bitmask_t*)value = moo->option.trait;
			return 0;

		case MOO_LOG_MASK:
			*(moo_bitmask_t*)value = moo->option.log_mask;
			return 0;

		case MOO_LOG_MAXCAPA:
			*(moo_oow_t*)value = moo->option.log_maxcapa;
			return 0;

		case MOO_SYMTAB_SIZE:
			*(moo_oow_t*)value = moo->option.dfl_symtab_size;
			return 0;

		case MOO_SYSDIC_SIZE:
			*(moo_oow_t*)value = moo->option.dfl_sysdic_size;
			return 0;

		case MOO_PROCSTK_SIZE:
			*(moo_oow_t*)value = moo->option.dfl_procstk_size;
			return 0;

		case MOO_MOD_INCTX:
			*(void**)value = moo->option.mod_inctx;
			return 0;
	};

	moo_seterrnum (moo, MOO_EINVAL);
	return -1;
}

moo_cb_t* moo_regcb (moo_t* moo, moo_cb_t* tmpl)
{
	moo_cb_t* actual;

	actual = (moo_cb_t*)moo_allocmem(moo, MOO_SIZEOF(*actual));
	if (!actual) return MOO_NULL;

	*actual = *tmpl;
	actual->next = moo->cblist;
	actual->prev = MOO_NULL;
	moo->cblist = actual;

	return actual;
}

void moo_deregcb (moo_t* moo, moo_cb_t* cb)
{
	if (cb == moo->cblist)
	{
		moo->cblist = moo->cblist->next;
		if (moo->cblist) moo->cblist->prev = MOO_NULL;
	}
	else
	{
		if (cb->next) cb->next->prev = cb->prev;
		if (cb->prev) cb->prev->next = cb->next;
	}

	moo_freemem (moo, cb);
}

void* moo_allocmem (moo_t* moo, moo_oow_t size)
{
	void* ptr;

	ptr = MOO_MMGR_ALLOC (moo->mmgr, size);
	if (!ptr) moo_seterrnum (moo, MOO_ESYSMEM);
	return ptr;
}

void* moo_callocmem (moo_t* moo, moo_oow_t size)
{
	void* ptr;

	ptr = MOO_MMGR_ALLOC (moo->mmgr, size);
	if (!ptr) moo_seterrnum (moo, MOO_ESYSMEM);
	else MOO_MEMSET (ptr, 0, size);
	return ptr;
}

void* moo_reallocmem (moo_t* moo, void* ptr, moo_oow_t size)
{
	ptr = MOO_MMGR_REALLOC (moo->mmgr, ptr, size);
	if (!ptr) moo_seterrnum (moo, MOO_ESYSMEM);
	return ptr;
}

void moo_freemem (moo_t* moo, void* ptr)
{
	MOO_MMGR_FREE (moo->mmgr, ptr);
}

/* -------------------------------------------------------------------------- */

#define MOD_PREFIX "moo_mod_"
#define MOD_PREFIX_LEN 8

#if defined(MOO_ENABLE_STATIC_MODULE)

#if defined(MOO_ENABLE_MOD_CON)
#	include "../mod/_con.h"
#endif
#if defined(MOO_ENABLE_MOD_FFI)
#	include "../mod/_ffi.h"
#endif
#if defined(MOO_ENABLE_MOD_SCK)
#	include "../mod/_sck.h"
#endif
#include "../mod/_stdio.h"
#if defined(MOO_ENABLE_MOD_X11)
#	include "../mod/_x11.h"
#endif

static struct
{
	moo_bch_t* modname;
	int (*modload) (moo_t* moo, moo_mod_t* mod);
}
static_modtab[] = 
{
#if defined(MOO_ENABLE_MOD_CON)
	{ "con",        moo_mod_con },
#endif
#if defined(MOO_ENABLE_MOD_FFI)
	{ "ffi",        moo_mod_ffi },
#endif
#if defined(MOO_ENABLE_MOD_SCK)
	{ "sck",        moo_mod_sck },
	{ "sck.addr",   moo_mod_sck_addr },
#endif
	{ "stdio",      moo_mod_stdio },
#if defined(MOO_ENABLE_MOD_X11)
	{ "x11",        moo_mod_x11 },
	/*{ "x11.win",    moo_mod_x11_win },*/
#endif
};
#endif

moo_mod_data_t* moo_openmod (moo_t* moo, const moo_ooch_t* name, moo_oow_t namelen, int hints)
{
	moo_rbt_pair_t* pair;
	moo_mod_data_t* mdp;
	moo_mod_data_t md;
	moo_mod_load_t load = MOO_NULL;
#if defined(MOO_ENABLE_STATIC_MODULE)
	int n;
#endif

	/* maximum module name length is MOO_MOD_NAME_LEN_MAX. 
	 *   MOD_PREFIX_LEN for MOD_PREFIX
	 *   1 for _ at the end when moo_mod_xxx_ is attempted.
	 *   1 for the terminating '\0'.
	 */
	moo_ooch_t buf[MOD_PREFIX_LEN + MOO_MOD_NAME_LEN_MAX + 1 + 1]; 

	/* copy instead of encoding conversion. MOD_PREFIX must not
	 * include a character that requires encoding conversion.
	 * note the terminating null isn't needed in buf here. */
	moo_copy_bchars_to_oochars (buf, MOD_PREFIX, MOD_PREFIX_LEN); 

	if (namelen > MOO_COUNTOF(buf) - (MOD_PREFIX_LEN + 1 + 1))
	{
		/* module name too long  */
		moo_seterrnum (moo, MOO_EINVAL); /* TODO: change the  error number to something more specific */
		return MOO_NULL;
	}

	moo_copy_oochars (&buf[MOD_PREFIX_LEN], name, namelen);
	buf[MOD_PREFIX_LEN + namelen] = '\0';

#if defined(MOO_ENABLE_STATIC_MODULE)
	/* attempt to find a statically linked module */

	/* TODO: binary search ... */
	for (n = 0; n < MOO_COUNTOF(static_modtab); n++)
	{
		if (moo_comp_oochars_bcstr(name, namelen, static_modtab[n].modname) == 0) 
		{
			load = static_modtab[n].modload;
			break;
		}
	}

	if (load)
	{
		/* found the module in the staic module table */

		MOO_MEMSET (&md, 0, MOO_SIZEOF(md));
		md.mod.inctx = moo->option.mod_inctx;
		moo_copy_oochars ((moo_ooch_t*)md.mod.name, name, namelen);
		/* Note md.handle is MOO_NULL for a static module */

		/* i copy-insert 'md' into the table before calling 'load'.
		 * to pass the same address to load(), query(), etc */
		pair = moo_rbt_insert(&moo->modtab, (moo_ooch_t*)name, namelen, &md, MOO_SIZEOF(md));
		if (pair == MOO_NULL)
		{
			moo_seterrnum (moo, MOO_ESYSMEM);
			return MOO_NULL;
		}

		mdp = (moo_mod_data_t*)MOO_RBT_VPTR(pair);
		MOO_ASSERT (moo, MOO_SIZEOF(mdp->mod.hints) == MOO_SIZEOF(int));
		mdp->mod.hints = hints;
		if (load(moo, &mdp->mod) <= -1)
		{
			moo_rbt_delete (&moo->modtab, (moo_ooch_t*)name, namelen);
			return MOO_NULL;
		}

		mdp->pair = pair;

		MOO_DEBUG1 (moo, "Opened a static module [%js]\n", mdp->mod.name);
		return mdp;
	}
	else
	{
	#if !defined(MOO_ENABLE_DYNAMIC_MODULE)
		MOO_DEBUG2 (moo, "Cannot find a static module [%.*js]\n", namelen, name);
		moo_seterrbfmt (moo, MOO_ENOENT, "unable to find a static module [%.*js]", namelen, name);
		return MOO_NULL;
	#endif
	}
#endif

#if !defined(MOO_ENABLE_DYNAMIC_MODULE)
	MOO_DEBUG2 (moo, "Cannot open module [%.*js] - module loading disabled\n", namelen, name);
	moo_seterrbfmt (moo, MOO_ENOIMPL, "unable to open module [%.*js] - module loading disabled", namelen, name);
	return MOO_NULL;
#endif

	/* attempt to find a dynamic external module */
	MOO_MEMSET (&md, 0, MOO_SIZEOF(md));
	md.mod.inctx = moo->option.mod_inctx;
	moo_copy_oochars ((moo_ooch_t*)md.mod.name, name, namelen);
	if (moo->vmprim.dl_open && moo->vmprim.dl_getsym && moo->vmprim.dl_close)
	{
		md.handle = moo->vmprim.dl_open(moo, &buf[MOD_PREFIX_LEN], MOO_VMPRIM_DLOPEN_PFMOD);
	}

	if (md.handle == MOO_NULL) 
	{
		MOO_DEBUG2 (moo, "Cannot open a module [%.*js]\n", namelen, name);
		moo_seterrbfmt (moo, MOO_ENOENT, "unable to open a module [%.*js]", namelen, name);
		return MOO_NULL;
	}

	/* attempt to get moo_mod_xxx where xxx is the module name*/
	load = moo->vmprim.dl_getsym(moo, md.handle, buf);
	if (!load) 
	{
		moo_seterrbfmt (moo, moo_geterrnum(moo), "unable to get module symbol [%js] in [%.*js] - %js", buf,namelen, name);
		MOO_DEBUG3 (moo, "Cannot get a module symbol [%js] in [%.*js]\n", buf, namelen, name);
		moo->vmprim.dl_close (moo, md.handle);
		return MOO_NULL;
	}

	/* i copy-insert 'md' into the table before calling 'load'.
	 * to pass the same address to load(), query(), etc */
	pair = moo_rbt_insert(&moo->modtab, (void*)name, namelen, &md, MOO_SIZEOF(md));
	if (pair == MOO_NULL)
	{
		MOO_DEBUG2 (moo, "Cannot register a module [%.*js]\n", namelen, name);
		moo_seterrbfmt (moo, MOO_ESYSMEM, "unable to register a module [%.*js] for memory shortage", namelen, name);
		moo->vmprim.dl_close (moo, md.handle);
		return MOO_NULL;
	}

	mdp = (moo_mod_data_t*)MOO_RBT_VPTR(pair);
	MOO_ASSERT (moo, MOO_SIZEOF(mdp->mod.hints) == MOO_SIZEOF(int));
	mdp->mod.hints = hints;
	if (load(moo, &mdp->mod) <= -1)
	{
		const moo_ooch_t* oldmsg = moo_backuperrmsg (moo);
		moo_seterrbfmt (moo, moo_geterrnum(moo), "module initializer [%js] returned failure in [%.*js] - %js", buf, namelen, name, oldmsg); 
		MOO_DEBUG3 (moo, "Module function [%js] returned failure in [%.*js]\n", buf, namelen, name);
		moo_rbt_delete (&moo->modtab, name, namelen);
		moo->vmprim.dl_close (moo, mdp->handle);
		return MOO_NULL;
	}

	mdp->pair = pair;

	MOO_DEBUG2 (moo, "Opened a module [%js] - %p\n", mdp->mod.name, mdp->handle);

	/* the module loader must ensure to set a proper query handler */
	MOO_ASSERT (moo, mdp->mod.query != MOO_NULL);

	return mdp;
}

void moo_closemod (moo_t* moo, moo_mod_data_t* mdp)
{
	if (mdp->mod.unload) mdp->mod.unload (moo, &mdp->mod);

	if (mdp->handle) 
	{
		moo->vmprim.dl_close (moo, mdp->handle);
		MOO_DEBUG2 (moo, "Closed a module [%js] - %p\n", mdp->mod.name, mdp->handle);
		mdp->handle = MOO_NULL;
	}
	else
	{
		MOO_DEBUG1 (moo, "Closed a static module [%js]\n", mdp->mod.name);
	}

	if (mdp->pair)
	{
		/*mdp->pair = MOO_NULL;*/ /* this reset isn't needed as the area will get freed by moo_rbt_delete()) */
		moo_rbt_delete (&moo->modtab, mdp->mod.name, moo_count_oocstr(mdp->mod.name));
	}
}

int moo_importmod (moo_t* moo, moo_oop_class_t _class, const moo_ooch_t* name, moo_oow_t len)
{
	moo_rbt_pair_t* pair;
	moo_mod_data_t* mdp;
	int r = -1;

	/* moo_openmod(), moo_closemod(), etc call a user-defined callback.
	 * i need to protect _class in case the user-defined callback allocates 
	 * a OOP memory chunk and GC occurs. */

	moo_pushtmp (moo, (moo_oop_t*)&_class);

	pair = moo_rbt_search(&moo->modtab, name, len);
	if (pair)
	{
		mdp = (moo_mod_data_t*)MOO_RBT_VPTR(pair);
		MOO_ASSERT (moo, mdp != MOO_NULL);

		MOO_DEBUG1 (moo, "Cannot import module [%js] - already active\n", mdp->mod.name);
		moo_seterrbfmt (moo, MOO_EPERM, "unable to import module [%js] - already active", mdp->mod.name);
		goto done2;
	}

	mdp = moo_openmod(moo, name, len, MOO_MOD_LOAD_FOR_IMPORT);
	if (!mdp) goto done2;

	if (!mdp->mod.import)
	{
		MOO_DEBUG1 (moo, "Cannot import module [%js] - importing not supported by the module\n", mdp->mod.name);
		moo_seterrbfmt (moo, MOO_ENOIMPL, "unable to import module [%js] - not supported by the module", mdp->mod.name);
		goto done;
	}

	if (mdp->mod.import(moo, &mdp->mod, _class) <= -1)
	{
		MOO_DEBUG1 (moo, "Cannot import module [%js] - module's import() returned failure\n", mdp->mod.name);
		goto done;
	}

	r = 0; /* everything successful */

done:
	/* close the module opened above.
	 * [NOTE] if the import callback calls the moo_querymod(), the returned
	 *        function pointers will get all invalidated here. so never do 
	 *        anything like that */
	moo_closemod (moo, mdp);

done2:
	moo_poptmp (moo);
	return r;
}

moo_pfbase_t* moo_querymod (moo_t* moo, const moo_ooch_t* pfid, moo_oow_t pfidlen, moo_mod_t** mod)
{
	/* primitive function identifier
	 *   _funcname
	 *   modname_funcname
	 */
	moo_rbt_pair_t* pair;
	moo_mod_data_t* mdp;
	const moo_ooch_t* sep;

	moo_oow_t mod_name_len;
	moo_pfbase_t* pfbase;

	sep = moo_rfind_oochar(pfid, pfidlen, '.');
	if (!sep)
	{
		/* i'm writing a conservative code here. the compiler should 
		 * guarantee that a period is included in an primitive function identifer.
		 * what if the compiler is broken? imagine a buggy compiler rewritten
		 * in moo itself? */
		MOO_DEBUG2 (moo, "Internal error - no period in a primitive function identifier [%.*js] - buggy compiler?\n", pfidlen, pfid);
		moo_seterrbfmt (moo, MOO_EINTERN, "no period in a primitive function identifier [%.*js]", pfidlen, pfid);
		return MOO_NULL;
	}

	mod_name_len = sep - pfid;

	/* the first segment through the segment before the last compose a
	 * module id. the last segment is the primitive function name.
	 * for instance, in con.window.open, con.window is a module id and
	 * open is the primitive function name. */
	pair = moo_rbt_search(&moo->modtab, pfid, mod_name_len);
	if (pair)
	{
		mdp = (moo_mod_data_t*)MOO_RBT_VPTR(pair);
		MOO_ASSERT (moo, mdp != MOO_NULL);
	}
	else
	{
		/* open a module using the part before the last period */
		mdp = moo_openmod(moo, pfid, mod_name_len, 0);
		if (!mdp) return MOO_NULL;
	}

	if ((pfbase = mdp->mod.query(moo, &mdp->mod, sep + 1, pfidlen - mod_name_len - 1)) == MOO_NULL) 
	{
		/* the primitive function is not found. but keep the module open even if it's opened above */
		MOO_DEBUG3 (moo, "Cannot find a primitive function [%.*js] in a module [%js]\n", pfidlen - mod_name_len - 1, sep + 1, mdp->mod.name);
		moo_seterrbfmt (moo, MOO_ENOENT, "unable to find a primitive function [%.*js] in a module [%js]", pfidlen - mod_name_len - 1, sep + 1, mdp->mod.name); 
		return MOO_NULL;
	}

	if (mod) *mod = &mdp->mod;

	MOO_DEBUG4 (moo, "Found a primitive function [%.*js] in a module [%js] - %p\n",
		pfidlen - mod_name_len - 1, sep + 1, mdp->mod.name, pfbase);
	return pfbase;
}

/* -------------------------------------------------------------------------- */

#if 0
/* add a new primitive method */
int moo_genpfmethod (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class, moo_method_type_t type, const moo_ooch_t* mthname, int variadic, const moo_ooch_t* pfname)
{
	/* NOTE: this function is a subset of add_compiled_method() in comp.c */

	moo_oop_char_t mnsym, pfidsym;
	moo_oop_method_t mth;
	moo_oow_t tmp_count = 0, i;
	moo_ooi_t arg_count = 0;
	moo_oocs_t cs;
	moo_ooi_t preamble_flags = 0;
	static moo_ooch_t dot[] = { '.', '\0' };

	MOO_ASSERT (moo, MOO_CLASSOF(moo, _class) == moo->_class);

	if (!pfname) pfname = mthname;

	moo_pushtmp (moo, (moo_oop_t*)&_class); tmp_count++;
	MOO_ASSERT (moo, MOO_CLASSOF(moo, (moo_oop_t)_class->mthdic[type]) == moo->_method_dictionary);

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
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - invalid name\n", mthname, _class->name);
		moo_seterrnum (moo, MOO_EINVAL);
		goto oops;
	}

	cs.ptr = (moo_ooch_t*)mthname;
	cs.len = i;
	if (moo_lookupdic (moo, _class->mthdic[type], &cs) != MOO_NULL)
	{
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - duplicate\n", mthname, _class->name);
		moo_seterrnum (moo, MOO_EEXIST);
		goto oops;
	}

	mnsym = (moo_oop_char_t)moo_makesymbol (moo, mthname, i);
	if (!mnsym) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&mnsym); tmp_count++;

	/* compose a full primitive function identifier to VM's string buffer.
	 *   pfid => mod->name + '.' + pfname */
	if (moo_copyoocstrtosbuf(moo, mod->name, 0) <= -1 ||
	    moo_concatoocstrtosbuf(moo, dot, 0) <= -1 ||
	    moo_concatoocstrtosbuf(moo, pfname, 0) <=  -1) 
	{
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - VM memory shortage\n", mthname, _class->name);
		return -1;
	}

	pfidsym = (moo_oop_char_t)moo_makesymbol (moo, moo->sbuf[0].ptr, moo->sbuf[0].len);
	if (!pfidsym) 
	{
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - symbol instantiation failure\n", mthname, _class->name);
		goto oops;
	}
	moo_pushtmp (moo, (moo_oop_t*)&pfidsym); tmp_count++;

#if defined(MOO_USE_METHOD_TRAILER)
	mth = (moo_oop_method_t)moo_instantiatewithtrailer (moo, moo->_method, 1, MOO_NULL, 0); 
#else
	mth = (moo_oop_method_t)moo_instantiate (moo, moo->_method, MOO_NULL, 1);
#endif
	if (!mth)
	{
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - method instantiation failure\n", mthname, _class->name);
		goto oops;
	}

	/* store the primitive function name symbol to the literal frame */
	mth->slot[0] = (moo_oop_t)pfidsym;

	/* premable should contain the index to the literal frame which is always 0 */
	mth->owner = _class;
	mth->name = mnsym;
	if (variadic) preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_VARIADIC;
	mth->preamble = MOO_SMOOI_TO_OOP(MOO_METHOD_MAKE_PREAMBLE(MOO_METHOD_PREAMBLE_NAMED_PRIMITIVE, 0, preamble_flags));
	/*mth->preamble_data[0] = MOO_SMOOI_TO_OOP(0);
	mth->preamble_data[1] = MOO_SMOOI_TO_OOP(0);*/
	mth->preamble_data[0] = MOO_SMPTR_TO_OOP(0);
	mth->preamble_data[1] = MOO_SMPTR_TO_OOP(0);
	mth->tmpr_count = MOO_SMOOI_TO_OOP(arg_count);
	mth->tmpr_nargs = MOO_SMOOI_TO_OOP(arg_count);

/* TODO: emit BCODE_RETURN_NIL as a fallback or self primitiveFailed? or anything else?? */

	if (!moo_putatdic (moo, _class->mthdic[type], (moo_oop_t)mnsym, (moo_oop_t)mth)) 
	{
		MOO_DEBUG2 (moo, "Cannot generate primitive function method [%js] in [%O] - failed to add to method dictionary\n", mthname, _class->name);
		goto oops;
	}

	MOO_DEBUG2 (moo, "Generated primitive function method [%js] in [%O]\n", mthname, _class->name);

	moo_poptmps (moo, tmp_count); tmp_count = 0;
	return 0;

oops:
	moo_poptmps (moo, tmp_count);
	return -1;
}

int moo_genpfmethods (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class, const moo_pfinfo_t* pfinfo, moo_oow_t pfcount)
{
	int ret = 0;
	moo_oow_t i;

	moo_pushtmp (moo, (moo_oop_t*)&_class);
	for (i = 0; i < pfcount; i++)
	{
		if (moo_genpfmethod (moo, mod, _class, pfinfo[i].type, pfinfo[i].mthname, pfinfo[i].variadic, MOO_NULL) <= -1) 
		{
			/* TODO: delete pfmethod generated??? */
			ret = -1;
			break;
		}
	}

	moo_poptmp (moo);
	return ret;
}
#endif

moo_pfbase_t* moo_findpfbase (moo_t* moo, moo_pfinfo_t* pfinfo, moo_oow_t pfcount, const moo_ooch_t* name, moo_oow_t namelen)
{
	int n;

	/* binary search */
#if 0
	/* [NOTE] this algorithm is NOT underflow safe with moo_oow_t types */
	int left, right, mid;

	for (left = 0, right = pfcount - 1; left <= right; )
	{
		/*mid = (left + right) / 2;*/
		mid = left + ((right - left) / 2);

		n = moo_comp_oochars_oocstr (name, namelen, pfinfo[mid].mthname);
		if (n < 0) right = mid - 1; /* this substraction can make right negative. so i can't use moo_oow_t for the variable */
		else if (n > 0) left = mid + 1;
		else return &pfinfo[mid].base;
	}
#else
	/* [NOTE] this algorithm is underflow safe with moo_oow_t types */
	moo_oow_t base, mid, lim;

	for (base = 0, lim = pfcount; lim > 0; lim >>= 1)
	{
		mid = base + (lim >> 1);
		n = moo_comp_oochars_oocstr (name, namelen, pfinfo[mid].mthname);
		if (n == 0) return &pfinfo[mid].base;
		if (n > 0) { base = mid + 1; lim--; }
	}
#endif

	moo_seterrnum (moo, MOO_ENOENT);
	return MOO_NULL;
}

/* -------------------------------------------------------------------------- */

int moo_setclasstrsize (moo_t* moo, moo_oop_class_t _class, moo_oow_t size, moo_trgc_t trgc)
{
	moo_oop_class_t sc;
	moo_oow_t spec;

	MOO_ASSERT (moo, MOO_CLASSOF(moo, _class) == moo->_class);
	MOO_ASSERT (moo, size <= MOO_SMOOI_MAX);	
 
	if (_class == moo->_method) 
	{
		/* the bytes code emitted by the compiler go to the trailer part
		 * regardless of the trailer size. you're not allowed to change it */
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to set trailer size to %zu on the %.*js class", 
			size, MOO_OBJ_GET_SIZE(_class->name), MOO_OBJ_GET_CHAR_SLOT(_class->name));
		return -1;
	}

	spec = MOO_OOP_TO_SMOOI(_class->spec);
	if (MOO_CLASS_SPEC_IS_INDEXED(spec) && MOO_CLASS_SPEC_INDEXED_TYPE(spec) != MOO_OBJ_TYPE_OOP)
	{
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to set trailer size to %zu on the non-pointer class %.*js", 
			size, MOO_OBJ_GET_SIZE(_class->name), MOO_OBJ_GET_CHAR_SLOT(_class->name));
		return -1;
	}

	if (_class->trsize != moo->_nil)
	{
		MOO_ASSERT (moo, _class->trgc != moo->_nil);
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to double-set trailer size to %zu on the %.*js class", 
			size, MOO_OBJ_GET_SIZE(_class->name), MOO_OBJ_GET_CHAR_SLOT(_class->name));
		return -1;
	}
	MOO_ASSERT (moo, _class->trgc == moo->_nil);

	sc = (moo_oop_class_t)_class->superclass;
	if (MOO_OOP_IS_SMOOI(sc->trsize) && size < MOO_OOP_TO_SMOOI(sc->trsize))
	{
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to set the trailer size of %.*js to be smaller(%zu) than that(%zu) of the superclass %.*js",
			size, MOO_OBJ_GET_SIZE(_class->name), MOO_OBJ_GET_CHAR_SLOT(_class->name),
			MOO_OOP_TO_SMOOI(sc->trsize), MOO_OBJ_GET_SIZE(sc->name), MOO_OBJ_GET_CHAR_SLOT(sc->name));
		return -1;
	}

	/* you can only set the trailer size once when it's not set yet */
	_class->trsize = MOO_SMOOI_TO_OOP(size);
	_class->trgc = MOO_SMPTR_TO_OOP(trgc); /* i don't replace NULL by nil for GC safety. */

	MOO_DEBUG5 (moo, "Set trailer size to %zu on the %.*js class with gc callback of %p(%p)\n", 
		size,
		MOO_OBJ_GET_SIZE(_class->name),
		MOO_OBJ_GET_CHAR_SLOT(_class->name),
		MOO_SMPTR_TO_OOP(trgc), trgc);
	return 0;
}

void* moo_getobjtrailer (moo_t* moo, moo_oop_t obj, moo_oow_t* size)
{
	if (!MOO_OBJ_IS_OOP_POINTER(obj) || !MOO_OBJ_GET_FLAGS_TRAILER(obj)) return MOO_NULL;
	if (size) *size = MOO_OBJ_GET_TRAILER_SIZE(obj);
	return MOO_OBJ_GET_TRAILER_BYTE(obj);
}

moo_oop_t moo_findclass (moo_t* moo, moo_oop_nsdic_t nsdic, const moo_ooch_t* name)
{
	moo_oop_association_t ass;
	moo_oocs_t n;

	n.ptr = (moo_ooch_t*)name;
	n.len = moo_count_oocstr(name);

	ass = moo_lookupdic (moo, (moo_oop_dic_t)nsdic, &n);
	if (!ass || MOO_CLASSOF(moo,ass->value) != moo->_class) 
	{
		moo_seterrnum (moo, MOO_ENOENT);
		return MOO_NULL;
	}

	return ass->value;
}

int moo_iskindof (moo_t* moo, moo_oop_t obj, moo_oop_class_t _class)
{
	moo_oop_class_t c;

	MOO_ASSERT (moo, MOO_CLASSOF(moo,_class) == moo->_class);

	c = MOO_CLASSOF(moo,obj); /* c := self class */
	if (c == moo->_class) 
	{
		/* object is a class */
		if (_class == moo->_class) return 1; /* obj isKindOf: Class */
	}
	else
	{
		if (c == _class) return 1; 
	}

	c = (moo_oop_class_t)c->superclass;
	while ((moo_oop_t)c != moo->_nil)
	{
		if (c == _class) return 1;
		c = (moo_oop_class_t)c->superclass;
	}
	return 0;
}
