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

typedef struct link_t link_t;
struct link_t
{
	link_t* next;
};

typedef struct ffi_t ffi_t;
struct ffi_t
{
	MOO_OBJ_HEADER;
	void* handle;

#if defined(USE_DYNCALL)
	DCCallVM* dc;
#endif
	link_t* ca; /* call arguments duplicated */
};


static MOO_INLINE void link_ca (ffi_t* ffi, void* ptr)
{
	link_t* l = (link_t*)((moo_oob_t*)ptr - MOO_SIZEOF_VOID_P);
	l->next = ffi->ca;
	ffi->ca = l;
}

static void free_linked_cas (moo_t* moo, ffi_t* ffi)
{
	while (ffi->ca)
	{
		link_t* ptr;
		ptr = ffi->ca;
		ffi->ca = ptr->next;
		moo_freemem (moo, ptr);
	}
}

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
	void* handle;
#if defined(USE_DYNCALL)
	DCCallVM* dc;
#endif

	if (nargs != 1)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_ISTYPEOF(moo, name, MOO_OBJ_TYPE_CHAR))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	if (rcv->handle)
	{
		moo_seterrnum (moo, MOO_EPERM); /* no allowed to open again */
		goto softfail;
	}

	handle = moo->vmprim.dl_open (moo, ((moo_oop_char_t)name)->slot, 0);
	if (!handle) goto softfail;

#if defined(USE_DYNCALL)
	dc = dcNewCallVM (4096); /* TODO: right size?  */
	if (!dc) 
	{
		moo_seterrnum (moo, moo_syserrtoerrnum(errno));
		moo->vmprim.dl_close (moo, handle);
		goto softfail;
	}
#endif

	rcv->handle = handle;

#if defined(USE_DYNCALL)
	rcv->dc = dc;
#endif

	MOO_DEBUG3 (moo, "<ffi.open> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, rcv->handle);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
	ffi_t* rcv;

	if (nargs != 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	MOO_DEBUG1 (moo, "<ffi.close> %p\n", rcv->handle);

	free_linked_cas (moo, rcv);

#if defined(USE_DYNCALL)
	dcFree (rcv->dc);
	rcv->dc = MOO_NULL;
#endif

	moo->vmprim.dl_close (moo, rcv->handle);
	rcv->handle = MOO_NULL;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_call (moo_t* moo, moo_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	ffi_t* rcv;
	moo_oop_t fun, sig, args;
	moo_oow_t i, j;
	void* f;
	moo_oop_oop_t arr;
	int ellipsis = 0;

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);

	if (nargs < 3) goto inval;
	fun = MOO_STACK_GETARG(moo, nargs, 0);
	sig = MOO_STACK_GETARG(moo, nargs, 1);
	args = MOO_STACK_GETARG(moo, nargs, 2);

	/* the signature must not be empty. at least the return type must be
	 * specified */
	if (!MOO_ISTYPEOF(moo, sig, MOO_OBJ_TYPE_CHAR) || MOO_OBJ_GET_SIZE(sig) <= 0) goto inval;

#if 0
	/* TODO: check if arr is a kind of array??? or check if it's indexed */
	if (MOO_OBJ_GET_SIZE(sig) > 1 && MOO_CLASSOF(moo,args) != moo->_array) goto inval;
#endif

	f = MOO_OOP_TO_SMPTR(fun); 
	arr = (moo_oop_oop_t)args;

	MOO_DEBUG2 (moo, "<ffi.call> %p in %p\n", f, rcv->handle);

	dcMode (rcv->dc, DC_CALL_C_DEFAULT);
	dcReset (rcv->dc);

	i = 0;
	if (i < MOO_OBJ_GET_SIZE(sig) && ((moo_oop_char_t)sig)->slot[i] == '|') 
	{
		dcMode (rcv->dc, DC_CALL_C_ELLIPSIS);

		/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
		if (dcGetError(rcv->dc) != DC_ERROR_NONE) goto noimpl;
		dcReset (rcv->dc);
		ellipsis = 1;
		i++;
	}

	for (j = 0; i < MOO_OBJ_GET_SIZE(sig); i++)
	{
		moo_ooch_t fmtc;
		moo_oop_t arg;

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
				dcMode (rcv->dc, DC_CALL_C_ELLIPSIS_VARARGS);

				/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
				if (dcGetError(rcv->dc) != DC_ERROR_NONE) goto noimpl;
			}
			continue;
		}

		/* more items in signature than the actual argument */
		if (j >= MOO_OBJ_GET_SIZE(arr)) goto inval;

		arg = arr->slot[j];
		switch (fmtc)
		{
		/* TODO: support more types... */
			case 'c':
				if (!MOO_OOP_IS_CHAR(arg)) goto inval;
				dcArgChar (rcv->dc, MOO_OOP_TO_CHAR(arr->slot[j]));
				j++;
				break;

/* TODO: added unsigned types */
			case 'i':
			{
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto inval;
				dcArgInt (rcv->dc, i);
				j++;
				break;
			}

			case 'l':
			{
				moo_ooi_t v;
			arg_as_long:
				if (moo_inttoooi(moo, arg, &v) == 0) goto inval;
				dcArgLong (rcv->dc, v);
				j++;
				break;
			}

			case 'L':
			{
			#if (MOO_SIZEOF_LONG_LONG > 0)
			#	if (MOO_SIZEOF_LONG_LONG <= MOO_SIZEOF_OOI_T)
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto inval;
			#	else
				/* TODO: moo_intmax_t v;
				if (moo_inttointmax (moo, arg, &v) == 0) goto inval; */
			#	error UNSUPPORTED MOO_SIZEOF_LONG_LONG.
			#	endif
				dcArgLongLong (rcv->dc, v);
				j++;
				break;
			#else
				goto arg_as_long;
			#endif
			}

#if 0
			case 'B': /* byte array */
#endif
			case 's':
			{
				moo_bch_t* ptr;

				if (!MOO_ISTYPEOF(moo,arg,MOO_OBJ_TYPE_CHAR)) goto inval;

			#if defined(MOO_OOCH_IS_UCH)
				ptr = moo_dupootobcharswithheadroom (moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
				if (!ptr) goto softfail; /* out of system memory or conversion error - soft failure */
				link_ca (rcv, ptr);
			#else
				ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
				/*ptr = moo_dupoochars (moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
				if (!ptr) goto softfail;*/ /* out of system memory or conversion error - soft failure */
			#endif

				dcArgPointer (rcv->dc, ptr);
				j++;
				break;
			}

			case 'S':
			{
				moo_uch_t* ptr;

				if (!MOO_ISTYPEOF(moo,arg,MOO_OBJ_TYPE_CHAR)) goto inval;

			#if defined(MOO_OOCH_IS_UCH)
				ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
				/*ptr = moo_dupoochars (moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
				if (!ptr) goto softfail; */ /* out of system memory or conversion error - soft failure */
			#else
				ptr = moo_dupootoucharswithheadroom (moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
				if (!ptr) goto softfail; /* out of system memory or conversion error - soft failure */
				link_ca (rcv, ptr);
			#endif

				dcArgPointer (rcv->dc, ptr);
				j++;
				break;
			}

			default:
				/* invalid argument signature specifier */
				goto inval;
		}
	}

	if (i >= MOO_OBJ_GET_SIZE(sig)) goto call_void;

	switch (((moo_oop_char_t)sig)->slot[i])
	{
/* TODO: support more types... */
/* TODO: proper return value conversion */
		case 'c':
		{
			char r = dcCallChar (rcv->dc, f);
			MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(r));
			break;
		}

		case 'i':
		{
			moo_oop_t r;

			r = moo_ooitoint (moo, dcCallInt (rcv->dc, f));
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case 'l':
		{
			moo_oop_t r;
		ret_as_long:
			r = moo_ooitoint (moo, dcCallLong (rcv->dc, f));
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case 'L':
		{
		#if (MOO_SIZEOF_LONG_LONG > 0)
			moo_oop_t r;
		#	if (MOO_SIZEOF_LONG_LONG <= MOO_SIZEOF_OOI_T)
			r = moo_ooitoint (moo, dcCallLongLong (rcv->dc, f));
		#	else
		#	error UNSUPPORTED MOO_SIZEOF_LONG_LONG
		#	endif
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		#else
			goto ret_as_long;
		#endif
		}
	

#if 0
		case 'B': /* byte array */
		{
		}
#endif

		case 's':
		{
			moo_oop_t s;
			moo_bch_t* r;

			r = dcCallPointer (rcv->dc, f);

		#if defined(MOO_OOCH_IS_UCH)
			s = moo_makestringwithbchars (moo, r, moo_countbcstr(r));
		#else
			s = moo_makestring(moo, r, moo_countbcstr(r));
		#endif
			if (!s) 
			{
				if (moo->errnum == MOO_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			MOO_STACK_SETRET (moo, nargs, s); 
			break;
		}

		case 'S':
		{
			moo_oop_t s;
			moo_uch_t* r;

			r = dcCallPointer (rcv->dc, f);

		#if defined(MOO_OOCH_IS_UCH)
			s = moo_makestring(moo, r, moo_countucstr(r));
		#else
			s = moo_makestringwithuchars(moo, r, moo_countucstr(r));
		#endif
			if (!s) 
			{
				if (moo->errnum == MOO_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			MOO_STACK_SETRET (moo, nargs, s); 
			break;
		}

		default:
		call_void:
			dcCallVoid (rcv->dc, f);
			MOO_STACK_SETRETTORCV (moo, nargs);
			break;
	}

	free_linked_cas (moo, rcv);
	return MOO_PF_SUCCESS;

noimpl:
	moo_seterrnum (moo, MOO_ENOIMPL);
	goto softfail;

inval:
	moo_seterrnum (moo, MOO_EINVAL);
	goto softfail;

softfail:
	free_linked_cas (moo, rcv);
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

hardfail:
	free_linked_cas (moo, rcv);
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
		goto softfail;
	}

	rcv = (ffi_t*)MOO_STACK_GETRCV(moo, nargs);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_ISTYPEOF(moo,name,MOO_OBJ_TYPE_CHAR))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	if (!moo->vmprim.dl_getsym)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	sym = moo->vmprim.dl_getsym (moo, rcv->handle, ((moo_oop_char_t)name)->slot);
	if (!sym) goto softfail;

	MOO_DEBUG4 (moo, "<ffi.getsym> %.*js => %p in %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, sym, rcv->handle);

	MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(sym));
	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(sym));
	return MOO_PF_SUCCESS;

softfail:
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
