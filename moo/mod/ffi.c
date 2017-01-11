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

#if defined(HAVE_DYNCALL_LIB) && defined(HAVE_DYNCALL_H)
#	define USE_DYNCALL
#endif

#if defined(USE_DYNCALL)
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
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
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
}
/*
struct bcstr_node_t
{
	stix_bch_t* ptr;
	bcstr_node_t* next;
};

static bcstr_node_t* dupbcstr (moo_t* moo, bcstr_node_t* bcstr, const moo_bch_t* ptr)
{
	
	moo_freemem (
}
 
static void free_bcstr_node (moo_t* moo, bcstr_node_t* bcstr)
{
	moo_freemem (moo, bcstr->ptr);
	moo_freemem (moo, bcstr);
}
*/
static moo_pfrc_t pf_call (moo_t* moo, moo_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	ffi_t* rcv;
	moo_oop_t fun, sig, args;
	DCCallVM* dc = MOO_NULL;
	moo_oow_t i, j;
	void* f;
	moo_oop_oop_t arr;
	int ellipsis = 0;
	struct bcstr_node_t* bcstr;

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

#if 0
	if (MOO_OBJ_GET_SIZE(sig) > 1 && MOO_CLASSOF(moo,args) != moo->_array) /* TODO: check if arr is a kind of array??? or check if it's indexed */
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto reterr;
	}
#endif

	f = MOO_OOP_TO_SMPTR(fun); 
	arr = (moo_oop_oop_t)args;

	dc = dcNewCallVM (4096); /* TODO: right size? */
	if (!dc) 
	{
		moo_seterrnum (moo, moo_syserrtoerrnum(errno));
		goto reterr;
	}

	MOO_DEBUG2 (moo, "<ffi.call> %p in %p\n", f, rcv->handle);

	/*dcMode (dc, DC_CALL_C_DEFAULT);
	dcReset (dc);*/

	i = 0;
	if (i < MOO_OBJ_GET_SIZE(sig) && ((moo_oop_char_t)sig)->slot[i] == '|') 
	{
		dcMode (dc, DC_CALL_C_ELLIPSIS);
		if (dcGetError(dc) != DC_ERROR_NONE)
		{
			/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
			moo_seterrnum (moo, MOO_ENOIMPL);
			goto reterr;
		}
		dcReset (dc);
		ellipsis = 1;
		i++;
	}

	for (j = 0; i < MOO_OBJ_GET_SIZE(sig); i++)
	{
		moo_ooch_t fmtc;

		fmtc = ((moo_oop_char_t)sig)->slot[i];

		if (fmtc == ')') 
		{
			i++;
			break;
		}
		else if (fmtc == '|')
		{
			if (ellipsis)
			{
				dcMode (dc, DC_CALL_C_ELLIPSIS_VARARGS);

				if (dcGetError(dc) != DC_ERROR_NONE) 
				{
					/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
					moo_seterrnum (moo, MOO_ENOIMPL);
					goto reterr;
				}
			}
			continue;
		}

		if (j >= MOO_OBJ_GET_SIZE(arr))
		{
			moo_seterrnum (moo, MOO_EINVAL);
			goto reterr;
		}

		switch (fmtc)
		{
		/* TODO: support more types... */
			case 'c':
				/* TODO: sanity check on the argument type */
				dcArgChar (dc, MOO_OOP_TO_CHAR(arr->slot[j]));
				j++;
				break;

			case 'i':
/* TODO: use moo_inttoooi () */
				dcArgInt (dc, MOO_OOP_TO_SMOOI(arr->slot[j]));
				j++;
				break;

			case 'l':
/* TODO: use moo_inttoooi () */
				dcArgLong (dc, MOO_OOP_TO_SMOOI(arr->slot[j]));
				j++;
				break;

			case 'L':
/* TODO: use moo_inttoooi () */
				dcArgLongLong (dc, MOO_OOP_TO_SMOOI(arr->slot[j]));
				j++;
				break;

#if 0
			case 'B': /* byte array */
#endif
			case 's':
			{
				moo_oow_t bcslen, ucslen;
				moo_bch_t bcs[1024]; /*TODO: dynamic length.... */

				ucslen = MOO_OBJ_GET_SIZE(arr->slot[j]);
				bcslen = MOO_COUNTOF(bcs);
				moo_convootobcstr (moo, ((moo_oop_char_t)arr->slot[j])->slot, &ucslen, bcs, &bcslen); /* proper string conversion */

				bcs[bcslen] = '\0';
				dcArgPointer (dc, bcs);
				j++;
				break;
			}

			default:
				/* TODO: ERROR HANDLING */
				break;
		}
	}

	if (i >= MOO_OBJ_GET_SIZE(sig)) goto call_void;

	switch (((moo_oop_char_t)sig)->slot[i])
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
			if (!r) goto oops;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case 'l':
		{
			moo_oop_t r;
			r = moo_ooitoint (moo, dcCallLong (dc, f));
			if (!r) goto oops;
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
			if (!rr) goto oops;

			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}
	#endif

#if 0
		case 'B': /* byte array */
		{
		}
#endif

		case 's':
		{
			moo_oow_t bcslen, ucslen;
			moo_ooch_t ucs[1024]; /* TOOD: buffer size... */
			moo_oop_t s;
			char* r = dcCallPointer (dc, f);

			bcslen = strlen(r); 
			moo_convbtooochars (moo, r, &bcslen, ucs, &ucslen); /* error check... */

			s = moo_makestring(moo, ucs, ucslen);
			if (!s) goto oops;

			MOO_STACK_SETRET (moo, nargs, s); 
			break;
		}

		default:
		call_void:
			dcCallVoid (dc, f);
			MOO_STACK_SETRETTORCV (moo, nargs);
			break;
	}

	dcFree (dc);
	return MOO_PF_SUCCESS;

reterr:
	if (dc) dcFree(dc);
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

oops:
	if (dc) dcFree(dc);
	return MOO_PF_HARD_FAILURE;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_getsym (moo_t* moo, moo_ooi_t nargs)
{
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
