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

static moo_pfrc_t pf_open (moo_t* moo, moo_ooi_t nargs)
{
	ffi_t* ffi;
	moo_oop_t name;
	void* handle;
#if defined(USE_DYNCALL)
	DCCallVM* dc;
#endif

	MOO_ASSERT (moo, nargs == 1);

	ffi = (ffi_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OBJ_IS_CHAR_POINTER(name))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	if (ffi->handle)
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

	ffi->handle = handle;

#if defined(USE_DYNCALL)
	ffi->dc = dc;
#endif

	MOO_DEBUG3 (moo, "<ffi.open> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, ffi->handle);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
	ffi_t* ffi;

	MOO_ASSERT (moo, nargs == 0);

	ffi = (ffi_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (!moo->vmprim.dl_open)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	MOO_DEBUG1 (moo, "<ffi.close> %p\n", ffi->handle);

	free_linked_cas (moo, ffi);

#if defined(USE_DYNCALL)
	dcFree (ffi->dc);
	ffi->dc = MOO_NULL;
#endif

	moo->vmprim.dl_close (moo, ffi->handle);
	ffi->handle = MOO_NULL;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_call (moo_t* moo, moo_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	ffi_t* ffi;
	moo_oop_t fun, sig, args;
	moo_oow_t i, j;
	void* f;
	moo_oop_oop_t arr;
	int ellipsis = 0;

	ffi = (ffi_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	MOO_ASSERT (moo, nargs == 3);
	fun = MOO_STACK_GETARG(moo, nargs, 0);
	sig = MOO_STACK_GETARG(moo, nargs, 1);
	args = MOO_STACK_GETARG(moo, nargs, 2);

	/* the signature must not be empty. at least the return type must be
	 * specified */
	if (!MOO_OBJ_IS_CHAR_POINTER(sig) || MOO_OBJ_GET_SIZE(sig) <= 0) goto inval;

#if 0
	/* TODO: check if arr is a kind of array??? or check if it's indexed */
	if (MOO_OBJ_GET_SIZE(sig) > 1 && MOO_CLASSOF(moo,args) != moo->_array) goto inval;
#endif

	f = MOO_OOP_TO_SMPTR(fun); 
	arr = (moo_oop_oop_t)args;

	MOO_DEBUG2 (moo, "<ffi.call> %p in %p\n", f, ffi->handle);

	dcMode (ffi->dc, DC_CALL_C_DEFAULT);
	dcReset (ffi->dc);

	i = 0;
	if (i < MOO_OBJ_GET_SIZE(sig) && ((moo_oop_char_t)sig)->slot[i] == '|') 
	{
		dcMode (ffi->dc, DC_CALL_C_ELLIPSIS);

		/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
		if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
		dcReset (ffi->dc);
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
				dcMode (ffi->dc, DC_CALL_C_ELLIPSIS_VARARGS);

				/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
				if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
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
				dcArgChar (ffi->dc, MOO_OOP_TO_CHAR(arr->slot[j]));
				j++;
				break;

/* TODO: added unsigned types */
			case 'i':
			{
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto inval;
				dcArgInt (ffi->dc, i);
				j++;
				break;
			}

			case 'l':
			{
				moo_ooi_t v;
			arg_as_long:
				if (moo_inttoooi(moo, arg, &v) == 0) goto inval;
				dcArgLong (ffi->dc, v);
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
				dcArgLongLong (ffi->dc, v);
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

				if (!MOO_OBJ_IS_CHAR_POINTER(arg)) goto inval;

			#if defined(MOO_OOCH_IS_UCH)
				ptr = moo_dupootobcharswithheadroom (moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
				if (!ptr) goto softfail; /* out of system memory or conversion error - soft failure */
				link_ca (ffi, ptr);
			#else
				ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
				/*ptr = moo_dupoochars (moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
				if (!ptr) goto softfail;*/ /* out of system memory or conversion error - soft failure */
			#endif

				dcArgPointer (ffi->dc, ptr);
				j++;
				break;
			}

			case 'S':
			{
				moo_uch_t* ptr;

				if (!MOO_OBJ_IS_CHAR_POINTER(arg)) goto inval;

			#if defined(MOO_OOCH_IS_UCH)
				ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
				/*ptr = moo_dupoochars (moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
				if (!ptr) goto softfail; */ /* out of system memory or conversion error - soft failure */
			#else
				ptr = moo_dupootoucharswithheadroom (moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
				if (!ptr) goto softfail; /* out of system memory or conversion error - soft failure */
				link_ca (ffi, ptr);
			#endif

				dcArgPointer (ffi->dc, ptr);
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
			char r = dcCallChar (ffi->dc, f);
			MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(r));
			break;
		}

		case 'i':
		{
			moo_oop_t r;

			r = moo_ooitoint (moo, dcCallInt (ffi->dc, f));
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case 'l':
		{
			moo_oop_t r;
		ret_as_long:
			r = moo_ooitoint (moo, dcCallLong (ffi->dc, f));
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case 'L':
		{
		#if (MOO_SIZEOF_LONG_LONG > 0)
			moo_oop_t r;
		#	if (MOO_SIZEOF_LONG_LONG <= MOO_SIZEOF_OOI_T)
			r = moo_ooitoint (moo, dcCallLongLong (ffi->dc, f));
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

			r = dcCallPointer (ffi->dc, f);

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

			r = dcCallPointer (ffi->dc, f);

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
			dcCallVoid (ffi->dc, f);
			MOO_STACK_SETRETTORCV (moo, nargs);
			break;
	}

	free_linked_cas (moo, ffi);
	return MOO_PF_SUCCESS;

noimpl:
	moo_seterrnum (moo, MOO_ENOIMPL);
	goto softfail;

inval:
	moo_seterrnum (moo, MOO_EINVAL);
	goto softfail;

softfail:
	free_linked_cas (moo, ffi);
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;

hardfail:
	free_linked_cas (moo, ffi);
	return MOO_PF_HARD_FAILURE;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_getsym (moo_t* moo, moo_ooi_t nargs)
{
	ffi_t* ffi;
	moo_oop_t name;
	void* sym;

	MOO_ASSERT (moo, nargs == 1);

	ffi = (ffi_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OBJ_IS_CHAR_POINTER(name))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	if (!moo->vmprim.dl_getsym)
	{
		moo_seterrnum (moo, MOO_ENOIMPL);
		goto softfail;
	}

	sym = moo->vmprim.dl_getsym (moo, ffi->handle, ((moo_oop_char_t)name)->slot);
	if (!sym) goto softfail;

	MOO_DEBUG4 (moo, "<ffi.getsym> %.*js => %p in %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, sym, ffi->handle);

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

#define MA MOO_TYPE_MAX(moo_oow_t)

static moo_pfinfo_t pfinfos[] =
{
	{ I, { 'c','a','l','l','\0' },           0, { pf_call,    3, 3  }  },
	{ I, { 'c','l','o','s','e','\0' },       0, { pf_close,   0, 0  }  },
	{ I, { 'g','e','t','s','y','m','\0' },   0, { pf_getsym,  1, 1  }  },
	{ I, { 'o','p','e','n','\0' },           0, { pf_open,    1, 1  }  }
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(ffi_t)) <= -1) return -1;
	return 0;
}

static moo_pfbase_t* query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	return moo_findpfbase (moo, pfinfos, MOO_COUNTOF(pfinfos), name);
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
