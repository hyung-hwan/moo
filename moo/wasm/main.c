#include <emscripten/emscripten.h>
#include <moo.h>
#include <stdlib.h>
#include <string.h>

static void* sys_alloc (moo_mmgr_t* mmgr, moo_oow_t size)
{
	return malloc(size);
}

static void* sys_realloc (moo_mmgr_t* mmgr, void* ptr, moo_oow_t size)
{
	return realloc(ptr, size);
}

static void sys_free (moo_mmgr_t* mmgr, void* ptr)
{
	free (ptr);
}

static moo_mmgr_t sys_mmgr =
{
	sys_alloc,
	sys_realloc,
	sys_free,
	MOO_NULL
};


/* ========================================================================= */

extern void jsGetTime (moo_ntime_t* now);
extern void jsSleep (const moo_ntime_t* now);

static void vm_gettime (moo_t* moo, moo_ntime_t* now)
{
	jsGetTime(now);
}
static void vm_sleep (moo_t* moo, const moo_ntime_t* dur)
{
	jsSleep (dur);
}


EMSCRIPTEN_KEEPALIVE int open_moo (void)
{
	moo_ntime_t now;
	vm_gettime (NULL, &now);
	now.sec = 1;
	now.nsec = 500;
	vm_sleep (NULL, &now);
	return now.sec;
#if 0
	moo_t* moo;
	moo_vmprim_t vmprim;
	moo_evtcb_t evtcb;
	xtn_t* xtn;

	MOO_MEMSET(&vmprim, 0, MOO_SIZEOF(vmprim));
	if (cfg && cfg->large_pages)
	{
		vmprim.alloc_heap = alloc_heap;
		vmprim.free_heap = free_heap;
	}
	vmprim.log_write = ((cfg && cfg->log_write)? cfg->log_write: log_write);
	vmprim.syserrstrb = syserrstrb;
	vmprim.assertfail = assert_fail;
	vmprim.dl_startup = dl_startup;
	vmprim.dl_cleanup = dl_cleanup;
	vmprim.dl_open = dl_open;
	vmprim.dl_close = dl_close;
	vmprim.dl_getsym = dl_getsym;
	vmprim.vm_startup = vm_startup;
	vmprim.vm_cleanup = vm_cleanup;
	vmprim.vm_gettime = vm_gettime;
	vmprim.vm_muxadd = vm_muxadd;
	vmprim.vm_muxdel = vm_muxdel;
	vmprim.vm_muxmod = vm_muxmod;
	vmprim.vm_muxwait = vm_muxwait;
	vmprim.vm_sleep = vm_sleep;

	moo = moo_open(&sys_mmgr, MOO_SIZEOF(xtn_t) + xtnsize, ((cfg && cfg->cmgr)? cfg->cmgr: moo_get_utf8_cmgr()), &vmprim, errinfo);
	if (!moo) return MOO_NULL;

	/* adjust the object size by the sizeof xtn_t so that qse_getxtn() returns the right pointer. */
	moo->_instsize += MOO_SIZEOF(xtn_t);

	xtn = GET_XTN(moo);

	MOO_MEMSET (&evtcb, 0, MOO_SIZEOF(evtcb));
	evtcb.fini = fini_moo;
	moo_regevtcb (moo, &evtcb);


	{
		moo_bitmask_t bm = 0;

		moo_getoption (moo, MOO_OPTION_TRAIT, &bm);
		/*bm |= MOO_TRAIT_NOGC;*/
		bm |= MOO_TRAIT_AWAIT_PROCS;
		moo_setoption (moo, MOO_OPTION_TRAIT, &bm);

		/* disable GC logs */
		moo_getoption (moo, MOO_OPTION_LOG_MASK, &bm);
		bm = ~MOO_LOG_GC;
		moo_setoption (moo, MOO_OPTION_LOG_MASK, &bm);
	}

	return moo;
#endif
}

EMSCRIPTEN_KEEPALIVE moo_bch_t* get_errmsg_from_moo (moo_t* moo)
{
#if defined(MOO_OOCH_IS_UCH)
/* TODO: no static.... error check... */
	static moo_bch_t bm[256];
	moo_oow_t bl = MOO_COUNTOF(bm);
	moo_convutobcstr (moo, moo_geterrmsg(moo), MOO_NULL, bm, &bl);
	return bm;
#else
	return moo_geterrmsg(moo);
#endif
}

EMSCRIPTEN_KEEPALIVE void switch_process_in_moo (moo_t* moo)
{
	moo_switchprocess(moo); 
}
