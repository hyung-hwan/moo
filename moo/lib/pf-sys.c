/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#define _GNU_SOURCE


#include "moo-prv.h"

/* TODO: experimental */
#include <signal.h>
#include <sys/ucontext.h>

static int segfault = 0;
void on_segfault (int sig, siginfo_t* si, void* ctx)
{
	ucontext_t* context = (ucontext_t*)ctx;

	printf ("SEG FAULT AT %p instruction at %p\n", si->si_addr, context->uc_mcontext.gregs[REG_RIP]);

	/*printf ("SEG FAULT AT %p instruction at %p\n", si->si_addr, context->uc_mcontext.arm_pc); arm */
	segfault = 1;
	context->uc_mcontext.gregs[REG_RIP] += 1; /* very platform and instruction specific... */
}

moo_pfrc_t moo_pf_system_collect_garbage (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_gc (moo);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_system_pop_collectable (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	if (moo->collectable.first)
	{
		moo_finalizable_t* first;

		first = moo->collectable.first;

		/* TODO: if it's already fininalized, delete it from collectable */
		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(first->oop));
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_GCFIN(first->oop) & MOO_GCFIN_FINALIZABLE);

		MOO_STACK_SETRET (moo, nargs, first->oop);
		MOO_OBJ_SET_FLAGS_GCFIN (first->oop, MOO_OBJ_GET_FLAGS_GCFIN(first->oop) | MOO_GCFIN_FINALIZED);

		MOO_DELETE_FROM_LIST (&moo->collectable, first);
		moo_freemem (moo, first); /* TODO: move it to the free list instead... */
	}
	else
	{
		/*MOO_STACK_SETRET (moo, nargs, moo->_nil);*/
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ENOENT);
	}

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------------------- */

moo_pfrc_t moo_pf_system_get_sigfd (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_ooi_t fd;
	fd = moo->vmprim.vm_getsigfd(moo);
	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(fd));
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_system_get_sig (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_uint8_t sig;
	int n;

	n = moo->vmprim.vm_getsig(moo, &sig);
	if (n <= -1) return MOO_PF_FAILURE;

	if (n == 0) MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ENOENT);
	else	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP((moo_ooi_t)sig));

	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_system_set_sig (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;
	moo_uint8_t sig;
	int n;

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_SMOOI(tmp));

	sig = (moo_uint8_t)MOO_OOP_TO_SMOOI(tmp);
	n = moo->vmprim.vm_setsig(moo, sig);
	if (n <= -1) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP((moo_ooi_t)sig));

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------------------- */

static MOO_INLINE moo_pfrc_t _system_alloc (moo_t* moo, moo_ooi_t nargs, int clear)
{
	moo_oop_t tmp;
	void* ptr;

	MOO_ASSERT (moo, nargs == 1);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (!MOO_OOP_IS_SMOOI(tmp) || MOO_OOP_TO_SMOOI(tmp) < 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid size %O for raw memory allocation", tmp);
		return MOO_PF_FAILURE;
	}

	ptr = clear? moo_callocmem (moo, MOO_OOP_TO_SMOOI(tmp)):
	             moo_allocmem (moo, MOO_OOP_TO_SMOOI(tmp));
	if (!ptr) return MOO_PF_FAILURE;

	if (!MOO_IN_SMPTR_RANGE(ptr))
	{
		moo_seterrbfmt (moo, MOO_ERANGE, "%p not in smptr range for raw memory allocation", ptr);
		moo_freemem (moo, ptr);
		return MOO_PF_FAILURE;
	}
	
	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(ptr));
	return MOO_PF_SUCCESS;
} 

moo_pfrc_t moo_pf_system_calloc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system);*/
	return _system_alloc (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_malloc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system);*/
	return _system_alloc (moo, nargs, 0);
}

moo_pfrc_t moo_pf_system_free (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;
	void* rawptr;

	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system);*/

	MOO_ASSERT (moo, nargs == 1);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (MOO_OOP_IS_SMPTR(tmp)) 
	{
		moo_freemem (moo, MOO_OOP_TO_SMPTR(tmp));
	}
	else if (moo_inttooow(moo, tmp, (moo_oow_t*)&rawptr) >= 1)
	{
		moo_freemem (moo, rawptr);
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_smptr_free (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;

	MOO_ASSERT (moo, nargs == 0);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp));

	moo_freemem (moo, MOO_OOP_TO_SMPTR(tmp));

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------------------- */

#include "pack1.h"
struct st_int8_t    { moo_int8_t  v; };
struct st_int16_t   { moo_int16_t v; };
struct st_int32_t   { moo_int32_t v; };

struct st_uint8_t   { moo_uint8_t  v; };
struct st_uint16_t  { moo_uint16_t v; };
struct st_uint32_t  { moo_uint32_t v; };

#if defined(MOO_HAVE_UINT64_T)
struct st_int64_t   { moo_int64_t v; };
struct st_uint64_t  { moo_uint64_t v; };
#endif
#if defined(MOO_HAVE_UINT128_T)
struct st_int128_t   { moo_int128_t v; };
struct st_uint128_t  { moo_uint128_t v; };
#endif
#include "unpack.h"


static MOO_INLINE moo_oop_t _fetch_raw_int (moo_t* moo, moo_int8_t* rawptr, moo_oow_t offset, int size)
{
	moo_ooi_t v;

	switch (size)
	{
		case 1: 
			v = ((struct st_int8_t*)&rawptr[offset])->v;
			break;

		case 2:
			v = ((struct st_int16_t*)&rawptr[offset])->v;
			break;

		case 4: 
			v = ((struct st_int32_t*)&rawptr[offset])->v;
			break;

	#if defined(MOO_HAVE_INT64_T) &&  (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_INT64_T)
		case 8: 
			v = ((struct st_int64_t*)&rawptr[offset])->v;
			break;
	#endif

	#if defined(MOO_HAVE_INT128_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_INT128_T)
		case 16: 
			v = ((struct st_int128_t*)&rawptr[offset])->v;
			break;
	#endif

		default:
			moo_seterrbfmt (moo, MOO_EINVAL, "unsupported size %d for raw signed memory fetch",  size);
			return MOO_NULL;
	}

	return moo_ooitoint (moo, v);
}


static MOO_INLINE moo_oop_t _fetch_raw_uint (moo_t* moo, moo_uint8_t* rawptr, moo_oow_t offset, int size)
{
	moo_oow_t v;

	switch (size)
	{
		case 1: 
			v = ((struct st_uint8_t*)&rawptr[offset])->v;
			break;

		case 2:
			v = ((struct st_uint16_t*)&rawptr[offset])->v;
			break;

		case 4: 
			v = ((struct st_uint32_t*)&rawptr[offset])->v;
			break;

	#if defined(MOO_HAVE_UINT64_T) &&  (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT64_T)
		case 8: 
			v = ((struct st_uint64_t*)&rawptr[offset])->v;
			break;
	#endif

	#if defined(MOO_HAVE_UINT128_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT128_T)
		case 16: 
			v = ((struct st_uint128_t*)&rawptr[offset])->v;
			break;
	#endif

		default:
			moo_seterrbfmt (moo, MOO_EINVAL, "unsupported size %d for raw unsigned memory fetch",  size);
			return MOO_NULL;
	}

	return moo_oowtoint(moo, v);
}



static MOO_INLINE int _store_raw_int (moo_t* moo, moo_uint8_t* rawptr, moo_oow_t offset, int size, moo_oop_t voop)
{
	moo_ooi_t w, max, min;

	if (moo_inttoooi(moo, voop, &w) == 0) 
	{
		moo_seterrbfmt (moo, moo_geterrnum(moo), "invalid value %O for raw signed memory store", voop);
		return -1;
	}

	/* assume 2's complement */
	max = (moo_ooi_t)(~(moo_oow_t)0 >> ((MOO_SIZEOF_OOW_T - size) * MOO_BITS_PER_BYTE  + 1));
	min = -max - 1;

	if (w > max || w < min) 
	{
		moo_seterrbfmt (moo, MOO_ERANGE, "value %zd out of supported range for raw signed memory store",  w);
		return -1;
	}

	switch (size)
	{ 
		case 1:
			((struct st_int8_t*)&rawptr[offset])->v = w;
			return 0;

		case 2:
			((struct st_int16_t*)&rawptr[offset])->v = w;
			return 0;

		case 4:
			((struct st_int32_t*)&rawptr[offset])->v = w;
			return 0;

	#if defined(MOO_HAVE_INT64_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_INT64_T)
		case 8:
			((struct st_int64_t*)&rawptr[offset])->v = w; 
			return 0;
	#endif

	#if defined(MOO_HAVE_INT128_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_INT128_T)
		case 16:
			((struct st_int128_t*)&rawptr[offset])->v = w; 
			return 0;
	#endif
	}

	moo_seterrbfmt (moo, MOO_EINVAL, "unsupported size %d for raw signed memory store",  size);
	return -1;
}

static MOO_INLINE int _store_raw_uint (moo_t* moo, moo_uint8_t* rawptr, moo_oow_t offset, int size, moo_oop_t voop)
{
	int n;
	moo_oow_t w, max;

	if ((n = moo_inttooow(moo, voop, &w)) <= 0) 
	{
		if (n <= -1) 
		{
			moo_seterrbfmt (moo, MOO_ERANGE, "negative value %O for raw unsigned memory store", voop);
		}
		else
		{
			moo_seterrbfmt (moo, moo_geterrnum(moo), "invalid value %O for raw unsigned memory store", voop);
		}
		return -1;
	}

	max = (~(moo_oow_t)0 >> ((MOO_SIZEOF_OOW_T - size) * MOO_BITS_PER_BYTE));
	if (w > max) 
	{
		moo_seterrbfmt (moo, MOO_ERANGE, "value %ju out of supported range for raw unsigned memory store",  w);
		return -1;
	}

#if defined(_WIN32)
	__try
	{
#else
	struct sigaction sa, oldsa1, oldsa2;
	MOO_MEMSET (&sa, 0, MOO_SIZEOF(sa));
	sigemptyset (&sa.sa_mask);
	sa.sa_sigaction = on_segfault;
	sa.sa_flags = SA_SIGINFO;
	sigaction (SIGSEGV, &sa, &oldsa1);
	sigaction (SIGBUS, &sa, &oldsa2);
	segfault = 0;
#endif

	n = 1; /* ok */
	switch (size)
	{ 
		case 1:
			((struct st_uint8_t*)&rawptr[offset])->v = w;
			break;

		case 2:
			((struct st_uint16_t*)&rawptr[offset])->v = w;
			break;

		case 4:
			((struct st_uint32_t*)&rawptr[offset])->v = w;
			break;

	#if defined(MOO_HAVE_UINT64_T) &&  (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT64_T)
		case 8:
			((struct st_uint64_t*)&rawptr[offset])->v = w;
			break;
	#endif

	#if defined(MOO_HAVE_UINT128_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT128_T)
		case 16:
			((struct st_uint128_t*)&rawptr[offset])->v = w;
			break;
	#endif

		default:
			n = 0; /* not ok */
			break;
	}

#if defined(_WIN32)
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		moo_seterrbfmt (moo, MOO_EACCES, "invalid memory access at %p",  (moo_uint8_t*)rawptr + offset);
		return -1;
	}
#else
	if (n)
	{
		sigaction (SIGSEGV, &oldsa1, MOO_NULL);
		sigaction (SIGBUS, &oldsa2, MOO_NULL);
		if (segfault)
		{
			moo_seterrbfmt (moo, MOO_EACCES, "invalid memory access at %p",  (moo_uint8_t*)rawptr + offset);
			return -1;
		}

		return 0;
	}
#endif

	moo_seterrbfmt (moo, MOO_EINVAL, "unsupported size %d for raw unsigned memory store",  size);
	return -1;
}

/* ------------------------------------------------------------------------------------- */

static moo_pfrc_t _get_system_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_int8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 2);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw signed memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow(moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw signed memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = _fetch_raw_int(moo, rawptr, offset, size);
	if (!tmp) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, tmp);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t _get_system_uint (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 2);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw unsigned memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow (moo, tmp, &offset) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw unsigned memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = _fetch_raw_uint(moo, rawptr, offset, size);
	if (!tmp) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, tmp);
	return MOO_PF_SUCCESS;
}


moo_pfrc_t moo_pf_system_get_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_get_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_get_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_get_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_system_get_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_get_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_get_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_get_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 8);
}

static moo_pfrc_t _put_system_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 3);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw signed memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow(moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw signed memory store", tmp);
		return MOO_PF_FAILURE;
	}

	if (_store_raw_int(moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 2)) <= -1) 
	{
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t _put_system_uint (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 3);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw unsigned memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow(moo, tmp, &offset) <= 0)
	{
			moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw unsigned memory store", tmp);
		return MOO_PF_FAILURE;
	}

	if (_store_raw_uint (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 2)) <= -1)
	{
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_system_put_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_put_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_put_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_put_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_system_put_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_put_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_put_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_put_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 8);
}

/* ------------------------------------------------------------------------------------- */


moo_pfrc_t moo_pf_system_get_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset, offset_in_buffer, len_in_buffer;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 5);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow(moo, tmp, &offset) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 3);
	if (moo_inttooow(moo, tmp, &offset_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 4);
	if (moo_inttooow(moo, tmp, &len_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer length %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 2);
	if (!MOO_OBJ_IS_BYTE_POINTER(tmp))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	if (offset_in_buffer < MOO_OBJ_GET_SIZE(tmp))
	{
		moo_oow_t max_len = MOO_OBJ_GET_SIZE(tmp) - offset_in_buffer;
		if (len_in_buffer > max_len) len_in_buffer = max_len;
		if (len_in_buffer > MOO_SMOOI_MAX) len_in_buffer = MOO_SMOOI_MAX;
	}
	else len_in_buffer = 0;
	
	if (len_in_buffer > 0)
	{
		MOO_MEMCPY (MOO_OBJ_GET_BYTE_PTR(tmp, offset_in_buffer), &rawptr[offset], len_in_buffer);
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(len_in_buffer));
	return MOO_PF_SUCCESS;
}


moo_pfrc_t moo_pf_system_put_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset, offset_in_buffer, len_in_buffer;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 5);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttooow(moo, tmp, &offset) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 3);
	if (moo_inttooow(moo, tmp, &offset_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 4);
	if (moo_inttooow(moo, tmp, &len_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer length %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 2);
	if (!MOO_OBJ_IS_BYTE_POINTER(tmp))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}
	
	if (offset_in_buffer < MOO_OBJ_GET_SIZE(tmp))
	{
		moo_oow_t max_len = MOO_OBJ_GET_SIZE(tmp) - offset_in_buffer;
		if (len_in_buffer > max_len) len_in_buffer = max_len;
		if (len_in_buffer > MOO_SMOOI_MAX) len_in_buffer = MOO_SMOOI_MAX;
	}
	else len_in_buffer = 0;
	
	if (len_in_buffer > 0)
	{
		MOO_MEMCPY (&rawptr[offset], MOO_OBJ_GET_BYTE_PTR(tmp, offset_in_buffer), len_in_buffer);
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(len_in_buffer));
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------------------- */

static moo_pfrc_t _get_smptr_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_oop_t tmp;
	moo_int8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t result;

	MOO_ASSERT (moo, nargs == 1);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow (moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw signed memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	result = _fetch_raw_int (moo, rawptr, offset, size);
	if (!result) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, result);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t _get_smptr_uint (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_oop_t tmp;
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t result;

	MOO_ASSERT (moo, nargs == 1);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow (moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw unsigned memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	result = _fetch_raw_uint (moo, rawptr, offset, size);
	if (!result) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, result);
	return MOO_PF_SUCCESS;
}

 moo_pfrc_t moo_pf_smptr_get_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 1);
}

 moo_pfrc_t moo_pf_smptr_get_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 2);
}

 moo_pfrc_t moo_pf_smptr_get_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 4);
}

 moo_pfrc_t moo_pf_smptr_get_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 8);
}

 moo_pfrc_t moo_pf_smptr_get_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 1);
}

 moo_pfrc_t moo_pf_smptr_get_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 2);
}

 moo_pfrc_t moo_pf_smptr_get_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 4);
}

 moo_pfrc_t moo_pf_smptr_get_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 8);
}

static moo_pfrc_t _put_smptr_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	MOO_ASSERT (moo, nargs == 2);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow(moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw signed memory store", tmp);
		return MOO_PF_FAILURE;
	}

	if (_store_raw_int (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 1)) <= -1)
	{
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t _put_smptr_uint (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	MOO_ASSERT (moo, nargs == 2);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow(moo, tmp, &offset) <= 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw unsigned memory store", tmp);
		return MOO_PF_FAILURE;
	}

	if (_store_raw_uint (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 1)) <= -1)
	{
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_smptr_put_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_smptr_put_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_smptr_put_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_smptr_put_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_smptr_put_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_smptr_put_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_smptr_put_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_smptr_put_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 8);
}


/* ------------------------------------------------------------------------------------- */

moo_pfrc_t moo_pf_smptr_get_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset, offset_in_buffer, len_in_buffer;
	moo_oop_t tmp;

	MOO_ASSERT (moo, nargs == 4);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow(moo, tmp, &offset) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 2);
	if (moo_inttooow(moo, tmp, &offset_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 3);
	if (moo_inttooow(moo, tmp, &len_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer length %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (!MOO_OBJ_IS_BYTE_POINTER(tmp))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	if (offset_in_buffer < MOO_OBJ_GET_SIZE(tmp))
	{
		moo_oow_t max_len = MOO_OBJ_GET_SIZE(tmp) - offset_in_buffer;
		if (len_in_buffer > max_len) len_in_buffer = max_len;
		if (len_in_buffer > MOO_SMOOI_MAX) len_in_buffer = MOO_SMOOI_MAX;
	}
	else len_in_buffer = 0;
	
	if (len_in_buffer > 0)
	{
		MOO_MEMCPY (MOO_OBJ_GET_BYTE_PTR(tmp, offset_in_buffer), &rawptr[offset], len_in_buffer);
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(len_in_buffer));
	return MOO_PF_SUCCESS;
}


moo_pfrc_t moo_pf_smptr_put_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_uint8_t* rawptr;
	moo_oow_t offset, offset_in_buffer, len_in_buffer;
	moo_oop_t tmp;

	MOO_ASSERT (moo, nargs == 4);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow(moo, tmp, &offset) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 2);
	if (moo_inttooow(moo, tmp, &offset_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer offset %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 3);
	if (moo_inttooow(moo, tmp, &len_in_buffer) <= 0) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer length %O for raw memory fetch", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (!MOO_OBJ_IS_BYTE_POINTER(tmp))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid buffer %O for raw memory store", tmp);
		return MOO_PF_FAILURE;
	}
	
	if (offset_in_buffer < MOO_OBJ_GET_SIZE(tmp))
	{
		moo_oow_t max_len = MOO_OBJ_GET_SIZE(tmp) - offset_in_buffer;
		if (len_in_buffer > max_len) len_in_buffer = max_len;
		if (len_in_buffer > MOO_SMOOI_MAX) len_in_buffer = MOO_SMOOI_MAX;
	}
	else len_in_buffer = 0;
	
	if (len_in_buffer > 0)
	{
		MOO_MEMCPY (&rawptr[offset], MOO_OBJ_GET_BYTE_PTR(tmp, offset_in_buffer), len_in_buffer);
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(len_in_buffer));
	return MOO_PF_SUCCESS;
}


static void sprintptr (moo_ooch_t* nbuf, moo_oow_t num, moo_oow_t *lenp)
{
	static const moo_ooch_t hex2ascii_upper[] = 
	{
		'0','1','2','3','4','5','6','7','8','9',
		'A','B','C','D','E','F','G','H','I','J','K','L','M',
		'N','O','P','Q','R','S','T','U','V','W','X','H','Z'
	};
	moo_ooch_t* p, * end, ch;

	p = nbuf;
	*p = '\0';
	do { *++p = hex2ascii_upper[num % 16]; } while (num /= 16);
	*++p = 'r';
	*++p = '6';
	*++p = '1';
	*lenp = p - nbuf;

	end = p;
	p = nbuf;
	while (p <= end)
	{
		ch = *p;
		*p++ = *end;
		*end-- = ch;
	}
}

moo_pfrc_t moo_pf_smptr_as_string (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;
	void* rawptr;
	moo_ooch_t buf[MOO_SIZEOF_OOW_T * 2 + 4];
	moo_oow_t len;
	moo_oop_t ss;

	MOO_ASSERT (moo, nargs == 0);

	tmp = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp) || MOO_CLASSOF(moo, tmp) == moo->_large_pointer);

	if (moo_ptrtooow(moo, tmp, (moo_oow_t*)&rawptr) <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pointer %O for string conversion", tmp);
		return MOO_PF_FAILURE;
	}

	sprintptr (buf, (moo_oow_t)rawptr, &len);

	ss = moo_makestring(moo, buf, len);
	if (!ss) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, ss);
	return MOO_PF_SUCCESS;
}
