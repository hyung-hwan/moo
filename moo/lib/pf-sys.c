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


#include "moo-prv.h"

static MOO_INLINE moo_pfrc_t _system_alloc (moo_t* moo, moo_ooi_t nargs, int clear)
{
	moo_oop_t tmp;
	void* ptr;

	MOO_ASSERT (moo, nargs == 1);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (!MOO_OOP_IS_SMOOI(tmp)) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid size - %O", tmp);
		return MOO_PF_FAILURE;
	}

	ptr = clear? moo_callocmem (moo, MOO_OOP_TO_SMOOI(tmp)):
	             moo_allocmem (moo, MOO_OOP_TO_SMOOI(tmp));
	if (!ptr) return MOO_PF_FAILURE;

	if (!MOO_IN_SMPTR_RANGE(ptr))
	{
		moo_seterrbfmt (moo, MOO_ERANGE, "%p not in smptr range", ptr);
		moo_freemem (moo, ptr);
		return MOO_PF_FAILURE;
	}
	
	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(ptr));
	return MOO_PF_SUCCESS;
} 

moo_pfrc_t moo_pf_system_calloc (moo_t* moo, moo_ooi_t nargs)
{
	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system);*/
	return _system_alloc (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_malloc (moo_t* moo, moo_ooi_t nargs)
{
	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system);*/
	return _system_alloc (moo, nargs, 0);
}

moo_pfrc_t moo_pf_system_free (moo_t* moo, moo_ooi_t nargs)
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

moo_pfrc_t moo_pf_smptr_free (moo_t* moo, moo_ooi_t nargs)
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
			moo_seterrnum (moo, MOO_EINVAL);
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
			moo_seterrnum (moo, MOO_EINVAL);
			return MOO_NULL;
	}

	return moo_oowtoint (moo, v);
}

static MOO_INLINE int _store_raw_int (moo_t* moo, moo_uint8_t* rawptr, moo_oow_t offset, int size, moo_oop_t voop)
{
	moo_ooi_t w, max, min;

	if (moo_inttoooi (moo, voop, &w) == 0) return -1;

	/* assume 2's complement */
	max = (moo_ooi_t)(~(moo_oow_t)0 >> ((MOO_SIZEOF_OOW_T - size) * 8  + 1));
	min = -max - 1;

	if (w > max || w < min) 
	{
		moo_seterrnum (moo, MOO_ERANGE); 
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

	moo_seterrnum (moo, MOO_EINVAL);
	return -1;
}

static MOO_INLINE int _store_raw_uint (moo_t* moo, moo_uint8_t* rawptr, moo_oow_t offset, int size, moo_oop_t voop)
{
	int n;
	moo_oow_t w, max;

	if ((n = moo_inttooow (moo, voop, &w)) <= 0) 
	{
		if (n <= -1) moo_seterrnum (moo, MOO_ERANGE); /* negative number */
		return -1;
	}

	max = (~(moo_oow_t)0 >> ((MOO_SIZEOF_OOW_T - size) * 8));
	if (w > max) 
	{
		moo_seterrnum (moo, MOO_ERANGE); 
		return -1;
	}

	switch (size)
	{ 
		case 1:
			((struct st_uint8_t*)&rawptr[offset])->v = w;
			return 0;

		case 2:
			((struct st_uint16_t*)&rawptr[offset])->v = w;
			return 0;

		case 4:
			((struct st_uint32_t*)&rawptr[offset])->v = w;
			return 0;

	#if defined(MOO_HAVE_UINT64_T) &&  (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT64_T)
		case 8:
			((struct st_uint64_t*)&rawptr[offset])->v = w;
			return 0;
	#endif

	#if defined(MOO_HAVE_UINT128_T) && (MOO_SIZEOF_OOW_T >= MOO_SIZEOF_UINT128_T)
		case 16:
			((struct st_uint128_t*)&rawptr[offset])->v = w; 
			return 0;
	#endif
	}

	moo_seterrnum (moo, MOO_EINVAL);
	return -1;
}


static moo_pfrc_t _get_system_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_int8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t tmp;

	/* MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->_system); */

	MOO_ASSERT (moo, nargs == 2);
	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (MOO_OOP_IS_SMPTR(tmp)) rawptr = MOO_OOP_TO_SMPTR(tmp);
	else if (moo_inttooow (moo, tmp, (moo_oow_t*)&rawptr) <= 0) goto einval;

	if (moo_inttooow (moo, MOO_STACK_GETARG(moo, nargs, 1), &offset) <= 0)
	{
	einval:
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	tmp = _fetch_raw_int (moo, rawptr, offset, size);
	if (!tmp) 
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

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
	if (MOO_OOP_IS_SMPTR(tmp)) rawptr = MOO_OOP_TO_SMPTR(tmp);
	else if (moo_inttooow (moo, tmp, (moo_oow_t*)&rawptr) <= 0) goto einval;

	if (moo_inttooow (moo, MOO_STACK_GETARG(moo, nargs, 1), &offset) <= 0) 
	{
	einval:
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	tmp = _fetch_raw_uint (moo, rawptr, offset, size);
	if (!tmp) 
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRET (moo, nargs, tmp);
	return MOO_PF_SUCCESS;
}



moo_pfrc_t moo_pf_system_get_int8 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_get_int16 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_get_int32 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_get_int64 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_system_get_uint8 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_get_uint16 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_get_uint32 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_system_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_get_uint64 (moo_t* moo, moo_ooi_t nargs)
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
	if (MOO_OOP_IS_SMPTR(tmp)) rawptr = MOO_OOP_TO_SMPTR(tmp);
	else if (moo_inttooow (moo, tmp, (moo_oow_t*)&rawptr) <= 0) goto einval;

	if (moo_inttooow(moo, MOO_STACK_GETARG(moo, nargs, 1), &offset) <= 0) goto einval;

	if (_store_raw_int (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 2)) <= -1)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

einval:
	MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
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
	if (MOO_OOP_IS_SMPTR(tmp)) rawptr = MOO_OOP_TO_SMPTR(tmp);
	else if (moo_inttooow (moo, tmp, (moo_oow_t*)&rawptr) <= 0) goto einval;

	if (moo_inttooow(moo, MOO_STACK_GETARG(moo, nargs, 1), &offset) <= 0) goto einval;

	if (_store_raw_uint (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 2)) <= -1)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

einval:
	MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_system_put_int8 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_put_int16 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_put_int32 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_put_int64 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_system_put_uint8 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_system_put_uint16 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_system_put_uint32 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_system_put_uint64 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_system_uint (moo, nargs, 8);
}




/* ------------------------------------------------------------------------------------- */



static moo_pfrc_t _get_smptr_int (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_oop_t rcv;
	moo_int8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t result;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(rcv));

	if (moo_inttooow (moo, MOO_STACK_GETARG(moo, nargs, 0), &offset) <= 0)
	{
		MOO_STACK_SETRETTOERROR(moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	rawptr = MOO_OOP_TO_SMPTR(rcv);

	result = _fetch_raw_int (moo, rawptr, offset, size);
	if (!result) 
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRET (moo, nargs, result);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t _get_smptr_uint (moo_t* moo, moo_ooi_t nargs, int size)
{
	moo_oop_t rcv;
	moo_uint8_t* rawptr;
	moo_oow_t offset;
	moo_oop_t result;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(rcv));

	if (moo_inttooow (moo, MOO_STACK_GETARG(moo, nargs, 0), &offset) <= 0)
	{
		MOO_STACK_SETRETTOERROR(moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	rawptr = MOO_OOP_TO_SMPTR(rcv);

	result = _fetch_raw_uint (moo, rawptr, offset, size);
	if (!result) 
	{
		MOO_STACK_SETRETTOERRNUM(moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRET (moo, nargs, result);
	return MOO_PF_SUCCESS;
}

 moo_pfrc_t moo_pf_smptr_get_int8 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 1);
}

 moo_pfrc_t moo_pf_smptr_get_int16 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 2);
}

 moo_pfrc_t moo_pf_smptr_get_int32 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 4);
}

 moo_pfrc_t moo_pf_smptr_get_int64 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_int (moo, nargs, 8);
}

 moo_pfrc_t moo_pf_smptr_get_uint8 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 1);
}

 moo_pfrc_t moo_pf_smptr_get_uint16 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 2);
}

 moo_pfrc_t moo_pf_smptr_get_uint32 (moo_t* moo, moo_ooi_t nargs)
{
	return _get_smptr_uint (moo, nargs, 4);
}

 moo_pfrc_t moo_pf_smptr_get_uint64 (moo_t* moo, moo_ooi_t nargs)
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
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp));

	rawptr = MOO_OOP_TO_SMPTR(tmp);

	if (moo_inttooow(moo, MOO_STACK_GETARG(moo, nargs, 0), &offset) <= 0)
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	if (_store_raw_int (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 1)) <= -1)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
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
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMPTR(tmp));

	rawptr = MOO_OOP_TO_SMPTR(tmp);

	if (moo_inttooow(moo, MOO_STACK_GETARG(moo, nargs, 0), &offset) <= 0)
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	if (_store_raw_uint (moo, rawptr, offset, size, MOO_STACK_GETARG(moo, nargs, 1)) <= -1)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_smptr_put_int8 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 1);
}

moo_pfrc_t moo_pf_smptr_put_int16 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 2);
}

moo_pfrc_t moo_pf_smptr_put_int32 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 4);
}

moo_pfrc_t moo_pf_smptr_put_int64 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_int (moo, nargs, 8);
}

moo_pfrc_t moo_pf_smptr_put_uint8 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 1);
}

moo_pfrc_t moo_pf_smptr_put_uint16 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 2);
}

moo_pfrc_t moo_pf_smptr_put_uint32 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 4);
}

moo_pfrc_t moo_pf_smptr_put_uint64 (moo_t* moo, moo_ooi_t nargs)
{
	return _put_smptr_uint (moo, nargs, 8);
}


/* ------------------------------------------------------------------------------------- */

