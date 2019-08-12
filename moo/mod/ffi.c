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

#include "_ffi.h"
#include <moo-utl.h>

#include <errno.h>
#include <string.h>

#if defined(HAVE_DYNCALL_LIB) && defined(HAVE_DYNCALL_H)
#	define USE_DYNCALL
#elif defined(HAVE_FFI_LIB) && defined(HAVE_FFI_H)
#	define USE_LIBFFI
#endif

#if defined(USE_DYNCALL)
#	include <dyncall.h>
#elif defined(USE_LIBFFI)
#	include <ffi.h>
#	if (MOO_SIZEOF_LONG_LONG > 0) && !defined(ffi_type_ulonglong)
#		if MOO_SIZEOF_LONG_LONG == MOO_SIZEOF_INT32_T
#			define ffi_type_ulonglong ffi_type_uint32
#			define ffi_type_slonglong ffi_type_sint32
#		elif MOO_SIZEOF_LONG_LONG == MOO_SIZEOF_INT64_T
#			define ffi_type_ulonglong ffi_type_uint64
#			define ffi_type_slonglong ffi_type_sint64
#		endif
#	endif
#endif

#define FMTC_NULL '\0' /* internal use only */
#define FMTC_CHAR 'c'
#define FMTC_SHORT 'h'
#define FMTC_INT 'i'
#define FMTC_LONG 'l'
#define FMTC_LONGLONG 'L'
#define FMTC_BCS 's'
#define FMTC_UCS 'S'
#define FMTC_BLOB 'b'
#define FMTC_POINTER 'p'

typedef struct link_t link_t;
struct link_t
{
	link_t* next;
};

#if defined(USE_LIBFFI)
typedef union ffi_sv_t ffi_sv_t;
union ffi_sv_t
{
	void* p;
	unsigned char uc;
	char c;
	unsigned short int uh;
	short h;
	unsigned int ui;
	int i;
	unsigned long int ul;
	long int l;
#if (MOO_SIZEOF_LONG_LONG > 0)
	unsigned long long int ull;
	long long int ll;
#endif
};
#endif

typedef struct ffi_t ffi_t;
struct ffi_t
{
	void* handle;

#if defined(USE_DYNCALL)
	DCCallVM* dc;
#elif defined(USE_LIBFFI)
	moo_oow_t arg_count;
	moo_oow_t arg_max;
	ffi_type** arg_types;
	void** arg_values;
	ffi_sv_t* arg_svs;

	ffi_sv_t ret_sv;
	ffi_cif cif;
	ffi_type* fmtc_to_type[2][128];
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

static moo_pfrc_t pf_open (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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

	handle = moo->vmprim.dl_open(moo, MOO_OBJ_GET_CHAR_SLOT(name), 0);
	if (!handle) goto softfail;

#if defined(USE_DYNCALL)
	dc = dcNewCallVM(4096); /* TODO: right size?  */
	if (!dc) 
	{
		moo_seterrwithsyserr (moo, 0, errno);
		moo->vmprim.dl_close (moo, handle);
		goto softfail;
	}
#endif

	ffi->handle = handle;

#if defined(USE_DYNCALL)
	ffi->dc = dc;
#elif defined(USE_LIBFFI)
	ffi->fmtc_to_type[0][FMTC_NULL] = &ffi_type_void;
	ffi->fmtc_to_type[1][FMTC_NULL] = &ffi_type_void;

	ffi->fmtc_to_type[0][FMTC_CHAR] = &ffi_type_schar;
	ffi->fmtc_to_type[1][FMTC_CHAR] = &ffi_type_uchar;
	ffi->fmtc_to_type[0][FMTC_SHORT] = &ffi_type_sshort;
	ffi->fmtc_to_type[1][FMTC_SHORT] = &ffi_type_ushort;
	ffi->fmtc_to_type[0][FMTC_INT] = &ffi_type_sint;
	ffi->fmtc_to_type[1][FMTC_INT] = &ffi_type_uint;
	ffi->fmtc_to_type[0][FMTC_LONG] = &ffi_type_slong;
	ffi->fmtc_to_type[1][FMTC_LONG] = &ffi_type_ulong;
	#if (MOO_SIZEOF_LONG_LONG > 0)
	ffi->fmtc_to_type[0][FMTC_LONGLONG] = &ffi_type_slonglong;
	ffi->fmtc_to_type[1][FMTC_LONGLONG] = &ffi_type_ulonglong;
	#endif

	ffi->fmtc_to_type[0][FMTC_POINTER] = &ffi_type_pointer;
	ffi->fmtc_to_type[1][FMTC_POINTER] = &ffi_type_pointer;
	ffi->fmtc_to_type[0][FMTC_BCS] = &ffi_type_pointer;
	ffi->fmtc_to_type[1][FMTC_BCS] = &ffi_type_pointer;
	ffi->fmtc_to_type[0][FMTC_UCS] = &ffi_type_pointer;
	ffi->fmtc_to_type[1][FMTC_UCS] = &ffi_type_pointer;
#endif

	MOO_DEBUG3 (moo, "<ffi.open> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), MOO_OBJ_GET_CHAR_SLOT(name), ffi->handle);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERRNUM (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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
#elif defined(USE_LIBFFI)
	if (ffi->arg_types)
	{
		moo_freemem (moo, ffi->arg_types);
		ffi->arg_types = MOO_NULL;
	}
	if (ffi->arg_values)
	{
		moo_freemem (moo, ffi->arg_values);
		ffi->arg_values = MOO_NULL;
	}
	if (ffi->arg_svs)
	{
		moo_freemem (moo, ffi->arg_svs);
		ffi->arg_svs = MOO_NULL;
	}
	ffi->arg_max = 0;
	ffi->arg_count = 0;
#endif

	moo->vmprim.dl_close (moo, ffi->handle);
	ffi->handle = MOO_NULL;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERRNUM (moo, nargs);
	return MOO_PF_SUCCESS;
}

static MOO_INLINE int add_ffi_arg (moo_t* moo, ffi_t* ffi, moo_ooch_t fmtc, int _unsigned, moo_oop_t arg)
{
#if defined(USE_LIBFFI)
	if (ffi->arg_count >= ffi->arg_max)
	{
		ffi_type** ttmp;
		void** vtmp;
		ffi_sv_t* stmp;

		moo_oow_t newmax;

		newmax = ffi->arg_max + 16; /* TODO: adjust this? */
		ttmp = moo_reallocmem(moo, ffi->arg_types, MOO_SIZEOF(*ttmp) * newmax);
		if (!ttmp) goto oops;
		vtmp = moo_reallocmem(moo, ffi->arg_values, MOO_SIZEOF(*vtmp) * newmax);
		if (!vtmp) goto oops;
		stmp = moo_reallocmem(moo, ffi->arg_svs, MOO_SIZEOF(*stmp) * newmax);
		if (!stmp) goto oops;

		ffi->arg_types = ttmp;
		ffi->arg_values = vtmp;
		ffi->arg_svs = stmp;
		ffi->arg_max = newmax;
	}
#endif

	switch (fmtc)
	{
		case FMTC_CHAR:
			if (!MOO_OOP_IS_CHAR(arg)) goto inval_arg_value;
			if (_unsigned)
			{
			#if defined(USE_DYNCALL)
				dcArgChar (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].uc;
				ffi->arg_svs[ffi->arg_count].uc = MOO_OOP_TO_CHAR(arg);
			#endif
			}
			else
			{
			#if defined(USE_DYNCALL)
				dcArgChar (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].c;
				ffi->arg_svs[ffi->arg_count].c = MOO_OOP_TO_CHAR(arg);
			#endif
			}
			break;

		case FMTC_SHORT:
			if (_unsigned)
			{
				moo_oow_t v;
				if (moo_inttooow(moo, arg, &v) == 0) goto inval_arg_value;
			#if defined(USE_DYNCALL)
				dcArgShort (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].uh;
				ffi->arg_svs[ffi->arg_count].uh = v;
			#endif
			}
			else
			{
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgShort (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].h;
				ffi->arg_svs[ffi->arg_count].h = v;
			#endif
			}
			break;

		case FMTC_INT:
			if (_unsigned)
			{
				moo_oow_t v;
				if (moo_inttooow(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgInt (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ui;
				ffi->arg_svs[ffi->arg_count].ui = v;
			#endif
			}
			else
			{
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgInt (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].i;
				ffi->arg_svs[ffi->arg_count].i = v;
			#endif
			}
			break;

		case FMTC_LONG:
		arg_as_long:
			if (_unsigned)
			{
				moo_oow_t v;
				if (moo_inttooow(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ul;
				ffi->arg_svs[ffi->arg_count].ul = v;
			#endif
			}
			else 
			{
				moo_ooi_t v;
				if (moo_inttoooi(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].l;
				ffi->arg_svs[ffi->arg_count].l = v;
			#endif
			}
			break;

		case FMTC_LONGLONG:
		#if (MOO_SIZEOF_LONG_LONG <= MOO_SIZEOF_LONG)
			goto arg_as_long;
		#else
			if (_unsigned)
			{
				moo_uintmax_t v;
				if (moo_inttouintmax(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgLongLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ull;
				ffi->arg_svs[ffi->arg_count].ull = v;
			#endif
			}
			else
			{
				moo_intmax_t v;
				if (moo_inttointmax(moo, arg, &v) == 0) goto oops;
			#if defined(USE_DYNCALL)
				dcArgLongLong (ffi->dc, v);
			#elif defined(USE_LIBFFI)
				ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].ll;
				ffi->arg_svs[ffi->arg_count].ll = v;
			#endif
			}
			break;
		#endif

		case FMTC_BCS:
		{
			moo_bch_t* ptr;

			if (!MOO_OBJ_IS_CHAR_POINTER(arg)) goto inval_arg_value;

		#if defined(MOO_OOCH_IS_UCH)
			ptr = moo_dupootobcharswithheadroom(moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
			if (!ptr) goto oops; /* out of system memory or conversion error - soft failure */
			link_ca (ffi, ptr);
		#else
			ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
			/*ptr = moo_dupoochars(moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
			if (!ptr) goto oops;*/ /* out of system memory or conversion error - soft failure */
		#endif

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_UCS:
		{
			moo_uch_t* ptr;

			if (!MOO_OBJ_IS_CHAR_POINTER(arg)) goto inval_arg_value;

		#if defined(MOO_OOCH_IS_UCH)
			ptr = MOO_OBJ_GET_CHAR_SLOT(arg);
			/*ptr = moo_dupoochars(moo, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg));
			if (!ptr) goto oops; */ /* out of system memory or conversion error - soft failure */
		#else
			ptr = moo_dupootoucharswithheadroom(moo, MOO_SIZEOF_VOID_P, MOO_OBJ_GET_CHAR_SLOT(arg), MOO_OBJ_GET_SIZE(arg), MOO_NULL);
			if (!ptr) goto oops; /* out of system memory or conversion error - soft failure */
			link_ca (ffi, ptr);
		#endif

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_BLOB:
		{
			void* ptr;

			if (MOO_OBJ_IS_BYTE_POINTER(arg)) goto inval_arg_value;
			ptr = MOO_OBJ_GET_BYTE_SLOT(arg);

		#if defined(USE_DYNCALL)
			dcArgPointer (ffi->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		case FMTC_POINTER:
		{
			void* ptr;

			/* TODO: map nil to NULL? or should #\p0 be enough? */
			if (!MOO_OOP_IS_SMPTR(arg)) goto inval_arg_value;
			ptr = MOO_OOP_TO_SMPTR(arg);

		#if defined(USE_DYNCALL)
			dcArgPointer (ffic->dc, ptr);
		#elif defined(USE_LIBFFI)
			ffi->arg_values[ffi->arg_count] = &ffi->arg_svs[ffi->arg_count].p;
			ffi->arg_svs[ffi->arg_count].p = ptr;
		#endif
			break;
		}

		default:
			/* invalid argument signature specifier */
			moo_seterrbfmt (moo, MOO_EINVAL, "invalid signature character - %jc", fmtc);
			goto oops;
	}

#if defined(USE_LIBFFI)
	ffi->arg_types[ffi->arg_count] = ffi->fmtc_to_type[_unsigned][fmtc];
	ffi->arg_count++;
#endif
	return 0;

inval_arg_value:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid argument value - %O", arg);

oops:
	return -1;
}


static moo_pfrc_t pf_call (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(USE_DYNCALL) || defined(USE_LIBFFI)
	ffi_t* ffi;
	moo_oop_t fun, sig, args;
	moo_oow_t i, j, nfixedargs;
	void* f;
	moo_oop_oop_t arr;
	int vbar = 0;
	moo_ooch_t fmtc;
	#if defined(USE_LIBFFI)
	ffi_status fs;
	#endif

	ffi = (ffi_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	fun = MOO_STACK_GETARG(moo, nargs, 0);
	sig = MOO_STACK_GETARG(moo, nargs, 1);
	args = MOO_STACK_GETARG(moo, nargs, 2);

	if (!MOO_OOP_IS_SMPTR(fun)) goto inval;

	/* the signature must not be empty. at least the return type must be
	 * specified */
	if (!MOO_OBJ_IS_CHAR_POINTER(sig) || MOO_OBJ_GET_SIZE(sig) <= 0) goto inval;

#if 0
	/* TODO: check if arr is a kind of array??? or check if it's indexed */
	if (MOO_OBJ_GET_SIZE(sig) > 1 && MOO_CLASSOF(moo,args) != moo->_array) goto inval;
#endif

	f = MOO_OOP_TO_SMPTR(fun); 
	arr = (moo_oop_oop_t)args;

	/*MOO_DEBUG2 (moo, "<ffi.call> %p in %p\n", f, ffi->handle);*/

#if defined(USE_DYNCALL)
	dcMode (ffi->dc, DC_CALL_C_DEFAULT);
	dcReset (ffi->dc);

	for (i = 0; i < MOO_OBJ_GET_SIZE(sig); i++)
	{
		fmtc = MOO_OBJ_GET_CHAR_VAL(sig, i);
		if (fmtc == '>') break; /* end of arguments. start of return type */
		if (fmtc == '|')
		{
			dcMode (ffi->dc, DC_CALL_C_ELLIPSIS); /* variadic. for arguments before ... */
			/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
			if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
			dcReset (ffi->dc);
			break;
		}
	}
#else
	ffi->arg_count = 0;
#endif

	/* check argument signature */
	for (i = 0, j = 0, nfixedargs = 0; i < MOO_OBJ_GET_SIZE(sig); i++)
	{
		fmtc = MOO_OBJ_GET_CHAR_VAL(sig, i);
		if (fmtc == ' ')
		{
			continue;
		}
		if (fmtc == '>') 
		{
			i++;
			if (!vbar) nfixedargs = j;
			break;
		}
		else if (fmtc == '|')
		{
			if (!vbar)
			{
			#if defined(USE_DYNCALL)
				dcMode (ffi->dc, DC_CALL_C_ELLIPSIS_VARARGS); /* start of arguments that fall to the ... part */
				/* the error code should be DC_ERROR_UNSUPPORTED_MODE */
				if (dcGetError(ffi->dc) != DC_ERROR_NONE) goto noimpl;
			#endif
				nfixedargs = j;
				vbar = 1;
			}
			continue;
		}

		/* more items in signature than the actual argument */  
		if (j >= MOO_OBJ_GET_SIZE(arr)) goto inval;

		if (add_ffi_arg(moo, ffi, fmtc, 0, MOO_OBJ_GET_OOP_VAL(arr, j)) <= -1) goto softfail;
		j++;
	}

	while (i < MOO_OBJ_GET_SIZE(sig) && MOO_OBJ_GET_CHAR_VAL(sig, i) == ' ') i++; /* skip all spaces after > */

	fmtc = (i >= MOO_OBJ_GET_SIZE(sig)? FMTC_NULL: MOO_OBJ_GET_CHAR_VAL(sig, i));
#if defined(USE_LIBFFI)
	fs = (nfixedargs == j)? ffi_prep_cif(&ffi->cif, FFI_DEFAULT_ABI, j, ffi->fmtc_to_type[0][fmtc], ffi->arg_types):
	                        ffi_prep_cif_var(&ffi->cif, FFI_DEFAULT_ABI, nfixedargs, j, ffi->fmtc_to_type[0][fmtc], ffi->arg_types);
	if (fs != FFI_OK)
	{
		moo_seterrnum (moo, MOO_ESYSERR);
		goto softfail;
	}

	ffi_call (&ffi->cif, FFI_FN(f), &ffi->ret_sv, ffi->arg_values);
#endif

	/* check the return value type in signature */
	switch (fmtc)
	{
/* TODO: support more types... */
/* TODO: proper return value conversion */
/* TODO: handle unsigned */
		case FMTC_CHAR:
		{
		#if defined(USE_DYNCALL)
			char r = dcCallChar(ffi->dc, f);
			MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(r));
		#elif defined(USE_LIBFFI)
			MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(ffi->ret_sv.c));
		#endif
			break;
		}

		case FMTC_SHORT:
		{
			moo_oop_t r;
		#if defined(USE_DYNCALL)
			r = moo_ooitoint(moo, dcCallShort(ffi->dc, f));
		#elif defined(USE_LIBFFI)
			r = moo_ooitoint(moo, ffi->ret_sv.h);
		#endif
			if (!r) goto hardfail;
		
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case FMTC_INT:
		{
			moo_oop_t r;
		#if defined(USE_DYNCALL)
			r = moo_ooitoint(moo, dcCallInt(ffi->dc, f));
		#elif defined(USE_LIBFFI)
			r = moo_ooitoint(moo, ffi->ret_sv.i);
		#endif
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case FMTC_LONG:
		{
			moo_oop_t r;
		ret_as_long:
		#if defined(USE_DYNCALL)
			r = moo_ooitoint(moo, dcCallLong(ffi->dc, f));
		#elif defined(USE_LIBFFI)
			r = moo_ooitoint(moo, ffi->ret_sv.l);
		#endif
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		}

		case FMTC_LONGLONG:
		{
		#if (MOO_SIZEOF_LONG_LONG <= MOO_SIZEOF_LONG)
			goto ret_as_long;
		#else
			moo_oop_t r;
		#if defined(USE_DYNCALL)
			r = moo_intmaxtoint(moo, dcCallLongLong(ffi->dc, f));
		#elif defined(USE_LIBFFI)
			/* TODO: unsigned  */
			r = moo_intmaxtoint(moo, ffi->ret_sv.ll);
		#endif
			if (!r) goto hardfail;
			MOO_STACK_SETRET (moo, nargs, r);
			break;
		#endif
		}
	

#if 0
		case 'B': /* byte array */
		{
		}
#endif
		case FMTC_BCS:
		{
			moo_oop_t s;
			moo_bch_t* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

		#if defined(MOO_OOCH_IS_UCH)
			s = moo_makestringwithbchars(moo, r, moo_count_bcstr(r));
		#else
			s = moo_makestring(moo, r, moo_count_bcstr(r));
		#endif
			if (!s) 
			{
				if (moo->errnum == MOO_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			MOO_STACK_SETRET (moo, nargs, s); 
			break;
		}

		case FMTC_UCS:
		{
			moo_oop_t s;
			moo_uch_t* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

		#if defined(MOO_OOCH_IS_UCH)
			s = moo_makestring(moo, r, moo_count_ucstr(r));
		#else
			s = moo_makestringwithuchars(moo, r, moo_count_ucstr(r));
		#endif
			if (!s) 
			{
				if (moo->errnum == MOO_EOOMEM) goto hardfail; /* out of object memory - hard failure*/
				goto softfail;
			}

			MOO_STACK_SETRET (moo, nargs, s); 
			break;
		}


#if 0
		case FMTC_BLOB:
		{
			/* blob as a return type isn't sufficient as it lacks the size information. 
			 * blob as an argument is just a pointer and the size can be yet another argument.
			 * it there a good way to represent this here?... TODO: */
			break;
		}
#endif

		case FMTC_POINTER:
		{
			void* r;

		#if defined(USE_DYNCALL)
			r = dcCallPointer(ffi->dc, f);
		#elif defined(USE_LIBFFI)
			r = ffi->ret_sv.p;
		#endif

			if (!MOO_IN_SMPTR_RANGE(r)) 
			{
				/* the returned pointer is not aligned */
				goto softfail;
			}

			MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(r));
			break;
		}

		default:
		#if defined(USE_DYNCALL)
			dcCallVoid (ffi->dc, f);
		#endif
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
	MOO_STACK_SETRETTOERRNUM (moo, nargs);
	return MOO_PF_SUCCESS;

hardfail:
	free_linked_cas (moo, ffi);
	return MOO_PF_HARD_FAILURE;

#else
	moo_seterrnum (moo, MOO_ENOIMPL);
	MOO_STACK_SETRETTOERRNUM (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_getsym (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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

	sym = moo->vmprim.dl_getsym(moo, ffi->handle, MOO_OBJ_GET_CHAR_SLOT(name));
	if (!sym) goto softfail;

	MOO_DEBUG4 (moo, "<ffi.getsym> %.*js => %p in %p\n", MOO_OBJ_GET_SIZE(name), MOO_OBJ_GET_CHAR_SLOT(name), sym, ffi->handle);

	MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(sym));
	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(sym));
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERRNUM (moo, nargs);
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
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(ffi_t), MOO_NULL) <= -1) return -1;
	return 0;
}

static moo_pfbase_t* query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase (moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
	/* TODO: track all opened shared objects... clear all external memories/resources created but not cleared (for lacking the call to 'close') */
}

int moo_mod_ffi (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
