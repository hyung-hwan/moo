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

#include "_ffi.h"
#include <moo-utl.h>

#include <errno.h>
#include <string.h>

#define HAVE_DYNCALL

#if defined(HAVE_DYNCALL)
#	include <dyncall.h>
#endif

typedef void* ptr_t;

typedef struct ffi_t ffi_t;
struct ffi_t
{
	MOO_OBJ_HEADER;
	void* handle;
};

static moo_pfrc_t pf_newinstsize (moo_t* moo, moo_ooi_t nargs)
{
	moo_ooi_t newinstsize = MOO_SIZEOF(ffi_t) - MOO_SIZEOF(moo_obj_t);
	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(newinstsize)); 
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_open (moo_t* moo, moo_ooi_t nargs)
{
#if defined(HAVE_DYNCALL)
	ffi_t* rcv;
	moo_oop_t name;

	if (nargs != 1)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_ISTYPEOF(moo, name, MOO_OBJ_TYPE_CHAR) || !MOO_OBJ_GET_FLAGS_EXTRA(name)) /* TODO: better null check instead of FLAGS_EXTREA check */
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto reterr;
	}

	rcv->handle = moo->vmprim.dl_open (moo, ((moo_oop_char_t)name)->slot, 0);
	if (!rcv->handle) goto reterr;

	MOO_DEBUG3 (moo, "<ffi.open> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, rcv->handle);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#endif
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
#if defined(HAVE_DYNCALL)
	ffi_t* rcv;

	if (nargs != 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto reterr;
	}

	MOO_DEBUG1 (moo, "<ffi.close> %p\n", rcv->handle);

	moo->vmprim.dl_close (moo, rcv->handle);
	rcv->handle = MOO_NULL;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#endif
}

static moo_pfrc_t pf_call (moo_t* moo, moo_ooi_t nargs)
{
#if defined(HAVE_DYNCALL)
	ffi_t* rcv;
	moo_oop_t fun, sig, args;

	if (nargs < 3)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);
	fun = MOO_STACK_GETARG(moo, nargs, 0);
	sig = MOO_STACK_GETARG(moo, nargs, 1);
	args = MOO_STACK_GETARG(moo, nargs, 2);

	if (!MOO_ISTYPEOF(moo, sig, MOO_OBJ_TYPE_CHAR) || MOO_OBJ_GET_SIZE(sig) <= 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	if (MOO_OBJ_GET_SIZE(sig) > 1 && MOO_CLASSOF(moo,args) != moo->_array) /* TODO: check if arr is a kind of array??? or check if it's indexed */
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	{
		moo_oow_t i;
		DCCallVM* dc;
		void* f;
		moo_oop_oop_t arr;
		int mode_set;

		f = MOO_OOP_TO_SMPTR(fun); 
		arr = (moo_oop_oop_t)args;

		dc = dcNewCallVM (4096);
		if (!dc) 
		{
			moo_seterrnum (moo, moo_syserrtoerrnum(errno));
			goto reterr;
		}

		MOO_DEBUG2 (moo, "<ffi.call> %p in %p\n", f, rcv->handle);

		/*dcMode (dc, DC_CALL_C_DEFAULT);
		dcReset (dc);*/

		/*for (i = 1; i < MOO_OBJ_GET_SIZE(sig); i++)
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

		for (i = 1; i < MOO_OBJ_GET_SIZE(sig); i++)
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
					dcArgChar (dc, MOO_OOP_TO_CHAR(arr->slot[i - 1]));
					break;

				case 'i':
/* TODO: use moo_inttoooi () */
					dcArgInt (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 1]));
					break;

				case 'l':
/* TODO: use moo_inttoooi () */
					dcArgLong (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 1]));
					break;

				case 'L':
/* TODO: use moo_inttoooi () */
					dcArgLongLong (dc, MOO_OOP_TO_SMOOI(arr->slot[i - 1]));
					break;

#if 0
				case 'B': /* byte array */
#endif
				case 's':
				{
					moo_oow_t bcslen, ucslen;
					moo_bch_t bcs[1024];

					ucslen = MOO_OBJ_GET_SIZE(arr->slot[i - 1]);
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

		switch (((moo_oop_char_t)sig)->slot[0])
		{
/* TODO: support more types... */
/* TODO: proper return value conversion */
			case 'c':
			{
				char r = dcCallChar (dc, f);
				MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(r));
				break;
			}

			case 'i':
			{
				moo_oop_t r;
				r = moo_ooitoint (moo, dcCallInt (dc, f));
				if (!r) return MOO_PF_HARD_FAILURE;
				MOO_STACK_SETRET (moo, nargs, r);
				break;
			}

			case 'l':
			{
				moo_oop_t r;
				r = moo_ooitoint (moo, dcCallLong (dc, f));
				if (!r) return MOO_PF_HARD_FAILURE;
				MOO_STACK_SETRET (moo, nargs, r);
				break;
			}

		#if (STIX_SIZEOF_LONG_LONG > 0)
			case 'L':
			{
				long long r = dcCallLongLong (dc, f);
				mod_oop_t r;

			#if STIX_SIZEOF_LONG_LONG <= STIX_SIZEOF_LONG
				r = moo_ooitoint (moo, dcCallLongLong (dc, f));
			#else
			#	error TODO:...
			#endif
				if (!rr) return MOD_PF_HARD_FAILURE;

				MOO_STACK_SETRET (moo, nargs, r);
				break;
			}
		#endif

#if 0
			case 'b': /* byte array */
			{
			}
#endif

			case 'B':
			{
				moo_oow_t bcslen, ucslen;
				moo_ooch_t ucs[1024];
				moo_oop_t s;
				char* r = dcCallPointer (dc, f);

				bcslen = strlen(r); 
				moo_convbtooochars (moo, r, &bcslen, ucs, &ucslen); /* error check... */

				s = moo_makestring(moo, ucs, ucslen);
				if (!s) 
				{
					dcFree (dc);
					return MOO_PF_HARD_FAILURE; /* TODO: proper error h andling */
				}

				MOO_STACK_SETRET (moo, nargs, s); 
				break;
			}

			default:

				moo_seterrnum (moo, MOO_EINVAL);
				goto reterr;
				break;
		}

		dcFree (dc);
	}

	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#endif
}

static moo_pfrc_t pf_getsym (moo_t* moo, moo_ooi_t nargs)
{
#if defined(HAVE_DYNCALL)
	ffi_t* rcv;
	moo_oop_t name;
	void* sym;

	if (nargs != 1)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_ISTYPEOF(moo,name,MOO_OBJ_TYPE_CHAR)) /* TODO: null check on the symbol name? */
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}

	if (!moo->vmprim.dl_getsym)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto reterr;
	}

	sym = moo->vmprim.dl_getsym (moo, rcv->handle, ((moo_oop_char_t)name)->slot);
	if (!sym) goto reterr;

	MOO_DEBUG4 (moo, "<ffi.getsym> %.*js => %p in %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, sym, rcv->handle);

	MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(sym));
	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(sym));
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#endif
}

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	moo_method_type_t type;
	moo_ooch_t mthname[15];
	int variadic;
	moo_pfimpl_t handler;
};

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

static fnctab_t fnctab[] =
{
	{ C, { '_','n','e','w','I','n','s','t','S','i','z','e','\0' },         0, pf_newinstsize   },
	{ I, { 'c','a','l','l','\0' },                                         1, pf_call          },
	{ I, { 'c','a','l','l',':','s','i','g',':','w','i','t','h',':','\0' }, 0, pf_call          },
	{ I, { 'c','l','o','s','e','\0' },                                     0, pf_close         },
	{ I, { 'g','e','t','s','y','m',':','\0' },                             0, pf_getsym        },
	{ I, { 'o','p','e','n',':','\0' },                                     0, pf_open          }
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_t _class)
{
	int ret = 0;
	moo_oow_t i;

	moo_pushtmp (moo, &_class);
	for (i = 0; i < MOO_COUNTOF(fnctab); i++)
	{
		if (moo_genpfmethod (moo, mod, _class, fnctab[i].type, fnctab[i].mthname, fnctab[i].variadic, MOO_NULL) <= -1) 
		{
			/* TODO: delete pfmethod generated??? */
			ret = -1;
			break;
		}
	}

	moo_poptmp (moo);
	return ret;
}

static moo_pfimpl_t query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	int left, right, mid, n;

	left = 0; right = MOO_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = moo_compoocstr (name, fnctab[mid].mthname);
		if (n < 0) right = mid - 1; 
		else if (n > 0) left = mid + 1;
		else
		{
			return fnctab[mid].handler;
		}
	}

	moo->errnum = MOO_ENOENT;
	return MOO_NULL;
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
}

int moo_mod_ffi (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
