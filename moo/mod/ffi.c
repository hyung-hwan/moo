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

#if 0

#include "_ffi.h"
#include <moo-utl.h>

#if defined(USE_DYNCALL)
/* TODO: defined dcAllocMem and dcFreeMeme before builing the dynload and dyncall library */
#	include <dyncall.h> /* TODO: remove this. make dyXXXX calls to callbacks */
#endif


	{   1,  1,  pf_ffi_open,                         "_ffi_open"            },
	{   1,  1,  pf_ffi_close,                        "_ffi_close"           },
	{   2,  2,  pf_ffi_getsym,                       "_ffi_getsym"          },
	{   3,  3,  pf_ffi_call,                         "_ffi_call"            }



static moo_pfrc_t pf_ffi_open (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg;
	void* handle;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_ISTYPEOF(moo, arg, MOO_OBJ_TYPE_CHAR))
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	if (!moo->vmprim.dl_open)
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}


/* TODO: check null-termination... */
	handle = moo->vmprim.dl_open (moo, ((moo_oop_char_t)arg)->slot);
	if (!handle)
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	MOO_STACK_POP (moo);
/* TODO: how to hold an address? as an integer????  or a byte array? fix this not to loose accuracy*/
	MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(handle));

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_ffi_close (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg;
	void* handle;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);


	if (!MOO_OOP_IS_SMOOI(arg))
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	MOO_STACK_POP (moo);

	handle = (void*)MOO_OOP_TO_SMOOI(arg); /* TODO: how to store void* ???. fix this not to loose accuracy */
	if (moo->vmprim.dl_close) moo->vmprim.dl_close (moo, handle);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_ffi_call (moo_t* moo, moo_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	moo_oop_t rcv, fun, sig, args;

	MOO_ASSERT (moo, nargs == 3);

	rcv = MOO_STACK_GET(moo, moo->sp - 3);
	fun = MOO_STACK_GET(moo, moo->sp - 2);
	sig = MOO_STACK_GET(moo, moo->sp - 1);
	args = MOO_STACK_GET(moo, moo->sp);

	if (!MOO_OOP_IS_SMOOI(fun)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	if (!MOO_ISTYPEOF(moo, sig, MOO_OBJ_TYPE_CHAR) || MOO_OBJ_GET_SIZE(sig) <= 0)
	{
MOO_DEBUG0 (moo, "FFI: wrong signature...\n");
		return MOO_PF_FAILURE;
	}

	if (MOO_CLASSOF(moo,args) != moo->_array) /* TODO: check if arr is a kind of array??? or check if it's indexed */
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	{
		moo_oow_t i;
		DCCallVM* dc;
		void* f;
		moo_oop_oop_t arr;
		int mode_set;

		f = MOO_OOP_TO_SMOOI(fun); /* TODO: decode pointer properly */
		arr = (moo_oop_oop_t)args;

		dc = dcNewCallVM (4096);
		if (!dc) return MOO_PF_HARD_FAILURE; /* TODO: proper error handling */

MOO_DEBUG1 (moo, "FFI: CALLING............%p\n", f);
		/*dcMode (dc, DC_CALL_C_DEFAULT);
		dcReset (dc);*/

		/*for (i = 2; i < MOO_OBJ_GET_SIZE(sig); i++)
		{
			if (((moo_oop_char_t)sig)->slot[i] == '|') 
			{
				dcMode (dc, DC_CALL_C_ELLIPSIS);
MOO_DEBUG0 (moo, "CALL MODE 111 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				mode_set = 1;
				break;
			}
		}
		if (!mode_set) */ dcMode (dc, DC_CALL_C_DEFAULT);

		for (i = 2; i < MOO_OBJ_GET_SIZE(sig); i++)
		{
MOO_DEBUG1 (moo, "FFI: CALLING ARG %c\n", ((moo_oop_char_t)sig)->slot[i]);
			switch (((moo_oop_char_t)sig)->slot[i])
			{
			/* TODO: support more types... */
				/*
				case '|':
					dcMode (dc, DC_CALL_C_ELLIPSIS_VARARGS);
MOO_DEBUG2 (moo, "CALL MODE 222 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
					break;
				*/

				case 'c':
					/* TODO: sanity check on the argument type */
					dcArgChar (dc, MOO_OOP_TO_CHAR(arr->slot[i - 2]));
					break;

				case 'i':
					dcArgInt (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'l':
					dcArgLong (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'L':
					dcArgLongLong (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 's':
				{
					moo_oow_t bcslen, ucslen;
					moo_bch_t bcs[1024];

					ucslen = MOO_OBJ_GET_SIZE(arr->slot[i - 2]);
					moo_convootobchars (moo, ((moo_oop_char_t)arr->slot[i - 2])->slot, &ucslen, bcs, &bcslen); /* proper string conversion */

					bcs[bcslen] = '\0';
					dcArgPointer (dc, bcs);
					break;
				}

				default:
					/* TODO: ERROR HANDLING */
					break;
			}

		}

		MOO_STACK_POPS (moo, nargs);

		switch (((moo_oop_char_t)sig)->slot[0])
		{
/* TODO: support more types... */
/* TODO: proper return value conversion */
			case 'c':
			{
				char r = dcCallChar (dc, f);
				MOO_STACK_SETTOP (moo, MOO_CHAR_TO_OOP(r));
				break;
			}

			case 'i':
			{
				int r = dcCallInt (dc, f);
MOO_DEBUG1 (moo, "CALLED... %d\n", r);
MOO_DEBUG2 (moo, "CALL ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(r));
				break;
			}

			case 'l':
			{
				long r = dcCallLong (dc, f);
				MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(r));
				break;
			}

			case 'L':
			{
				long long r = dcCallLongLong (dc, f);
				MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(r));
				break;
			}

			case 's':
			{
				moo_oow_t bcslen, ucslen;
				moo_ooch_t ucs[1024];
				moo_oop_t s;
				char* r = dcCallPointer (dc, f);

				bcslen = strlen(r); 
				moo_convbtooochars (moo, r, &bcslen, ucs, &ucslen); /* proper string conversion */

				s = moo_makestring(moo, ucs, ucslen)
				if (!s) 
				{
					dcFree (dc);
					return MOO_PF_HARD_FAILURE; /* TODO: proper error h andling */
				}

				MOO_STACK_SETTOP (moo, s); 
				break;
			}

			default:
				/* TOOD: ERROR HANDLING */
				break;
		}

		dcFree (dc);
	}

	return MOO_PF_SUCCESS;
#else
	return MOO_PF_FAILURE;
#endif
}

static moo_pfrc_t pf_ffi_getsym (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, hnd, fun;
	void* sym;

	MOO_ASSERT (moo, nargs == 2);

	rcv = MOO_STACK_GET(moo, moo->sp - 2);
	fun = MOO_STACK_GET(moo, moo->sp - 1);
	hnd = MOO_STACK_GET(moo, moo->sp);

	if (!MOO_OOP_IS_SMOOI(hnd)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return MOO_PF_FAILURE;
	}

	if (!MOO_ISTYPEOF(moo,fun,MOO_OBJ_TYPE_CHAR))
	{
MOO_DEBUG0 (moo, "wrong function name...\n");
		return MOO_PF_FAILURE;
	}

	if (!moo->vmprim.dl_getsym)
	{
		return MOO_PF_FAILURE;
	}

	sym = moo->vmprim.dl_getsym (moo, (void*)MOO_OOP_TO_SMOOI(hnd), ((moo_oop_char_t)fun)->slot);
	if (!sym)
	{
		return MOO_PF_FAILURE;
	}

/* TODO: how to hold an address? as an integer????  or a byte array? */
	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(sym));

	return MOO_PF_SUCCESS;
}
#endif
