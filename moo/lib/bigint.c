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

/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* 
 * Copyright 1994 - 1996 LongView Technologies L.L.C. $Revision: 1.5 $ 
 * Copyright (c) 2006, Sun Microsystems, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
      disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Sun Microsystems nor the names of its contributors may be used to endorse or promote products derived 
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT 
NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/

#include "moo-prv.h"

#if (MOO_LIW_BITS == MOO_OOW_BITS)
	/* nothing special */
#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
#	define MAKE_WORD(hw1,hw2) ((moo_oow_t)(hw1) | (moo_oow_t)(hw2) << MOO_LIW_BITS)
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

#define IS_SIGN_DIFF(x,y) (((x) ^ (y)) < 0)

/*#define IS_POW2(ui) (((ui) > 0) && (((ui) & (~(ui)+ 1)) == (ui)))*/
#define IS_POW2(ui) (((ui) > 0) && ((ui) & ((ui) - 1)) == 0)

/* digit character array */
static char* _digitc_array[] =
{
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
	"0123456789abcdefghijklmnopqrstuvwxyz"
};

/* exponent table for pow2 between 1 and 32 inclusive. */
static moo_uint8_t _exp_tab[32] = 
{
	0, 1, 0, 2, 0, 0, 0, 3,
	0, 0, 0, 0, 0, 0, 0, 4,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 5
};

static const moo_uint8_t debruijn_32[32] = 
{
	0, 1, 28, 2, 29, 14, 24, 3,
	30, 22, 20, 15, 25, 17, 4, 8, 
	31, 27, 13, 23, 21, 19, 16, 7,
	26, 12, 18, 6, 11, 5, 10, 9
};

static const moo_uint8_t debruijn_64[64] = 
{
	0, 1,  2, 53,  3,  7, 54, 27,
	4, 38, 41,  8, 34, 55, 48, 28,
	62,  5, 39, 46, 44, 42, 22,  9,
	24, 35, 59, 56, 49, 18, 29, 11,
	63, 52,  6, 26, 37, 40, 33, 47,
	61, 45, 43, 21, 23, 58, 17, 10,
	51, 25, 36, 32, 60, 20, 57, 16,
	50, 31, 19, 15, 30, 14, 13, 12
};

#if defined(MOO_HAVE_UINT32_T)
#	define LOG2_FOR_POW2_32(x) (debruijn_32[(moo_uint32_t)((moo_uint32_t)(x) * 0x077CB531) >> 27])
#endif

#if defined(MOO_HAVE_UINT64_T)
#	define LOG2_FOR_POW2_64(x) (debruijn_64[(moo_uint64_t)((moo_uint64_t)(x) * 0x022fdd63cc95386d) >> 58])
#endif

#if defined(MOO_HAVE_UINT32_T) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_UINT32_T)
#	define LOG2_FOR_POW2(x) LOG2_FOR_POW2_32(x)
#elif defined(MOO_HAVE_UINT64_T) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_UINT64_T)
#	define LOG2_FOR_POW2(x) LOG2_FOR_POW2_64(x)
#else
#	define LOG2_FOR_POW2(x) moo_get_pos_of_msb_set_pow2(x)
#endif

#if (MOO_SIZEOF_OOW_T == MOO_SIZEOF_INT) && defined(MOO_HAVE_BUILTIN_UADD_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uadd_overflow(a,b,c)
#elif (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG) && defined(MOO_HAVE_BUILTIN_UADDL_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG_LONG) && defined(MOO_HAVE_BUILTIN_UADDLL_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static MOO_INLINE int oow_add_overflow (moo_oow_t a, moo_oow_t b, moo_oow_t* c)
{
	*c = a + b;
	return b > MOO_TYPE_MAX(moo_oow_t) - a;
}
#endif


#if (MOO_SIZEOF_OOW_T == MOO_SIZEOF_INT) && defined(MOO_HAVE_BUILTIN_UMUL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umul_overflow(a,b,c)
#elif (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG) && defined(MOO_HAVE_BUILTIN_UMULL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umull_overflow(a,b,c)
#elif (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG_LONG) && defined(MOO_HAVE_BUILTIN_UMULLL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umulll_overflow(a,b,c)
#else
static MOO_INLINE int oow_mul_overflow (moo_oow_t a, moo_oow_t b, moo_oow_t* c)
{
#if (MOO_SIZEOF_UINTMAX_T > MOO_SIZEOF_OOW_T)
	moo_uintmax_t k;
	k = (moo_uintmax_t)a * (moo_uintmax_t)b;
	*c = (moo_oow_t)k;
	return (k >> MOO_OOW_BITS) > 0;
	/*return k > MOO_TYPE_MAX(moo_oow_t);*/
#else
	*c = a * b;
	return b != 0 && a > MOO_TYPE_MAX(moo_oow_t) / b; /* works for unsigned types only */
#endif
}
#endif

#if (MOO_SIZEOF_OOI_T == MOO_SIZEOF_INT) && defined(MOO_HAVE_BUILTIN_SMUL_OVERFLOW)
#	define smooi_mul_overflow(moo,a,b,c)  __builtin_smul_overflow(a,b,c)
#elif (MOO_SIZEOF_OOI_T == MOO_SIZEOF_LONG) && defined(MOO_HAVE_BUILTIN_SMULL_OVERFLOW)
#	define smooi_mul_overflow(moo,a,b,c)  __builtin_smull_overflow(a,b,c)
#elif (MOO_SIZEOF_OOI_T == MOO_SIZEOF_LONG_LONG) && defined(MOO_HAVE_BUILTIN_SMULLL_OVERFLOW)
#	define smooi_mul_overflow(moo,a,b,c)  __builtin_smulll_overflow(a,b,c)
#else
static MOO_INLINE int smooi_mul_overflow (moo_t* moo, moo_ooi_t a, moo_ooi_t b, moo_ooi_t* c)
{
	/* take note that this function is not supposed to handle
	 * the whole moo_ooi_t range. it handles the smooi subrange */

#if (MOO_SIZEOF_UINTMAX_T > MOO_SIZEOF_OOI_T)
	moo_intmax_t k;

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(a));
	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(b));

	k = (moo_intmax_t)a * (moo_intmax_t)b;
	*c = (moo_ooi_t)k;

	return k > MOO_TYPE_MAX(moo_ooi_t) || k < MOO_TYPE_MIN(moo_ooi_t);
#else

	moo_ooi_t ua, ub;

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(a));
	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(b));

	*c = a * b;

	ub = (b >= 0)? b: -b;
	ua = (a >= 0)? a: -a;

	/* though this fomula basically works for unsigned types in principle, 
	 * the values used here are all absolute values and they fall in
	 * a safe range to apply this fomula. the safe range is guaranteed because
	 * the sources are supposed to be smoois. */
	return ub != 0 && ua > MOO_TYPE_MAX(moo_ooi_t) / ub; 
#endif
}
#endif

#if (MOO_SIZEOF_LIW_T == MOO_SIZEOF_INT) && defined(MOO_HAVE_BUILTIN_UADD_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uadd_overflow(a,b,c)
#elif (MOO_SIZEOF_LIW_T == MOO_SIZEOF_LONG) && defined(MOO_HAVE_BUILTIN_UADDL_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (MOO_SIZEOF_LIW_T == MOO_SIZEOF_LONG_LONG) && defined(MOO_HAVE_BUILTIN_UADDLL_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static MOO_INLINE int liw_add_overflow (moo_liw_t a, moo_liw_t b, moo_liw_t* c)
{
	*c = a + b;
	return b > MOO_TYPE_MAX(moo_liw_t) - a;
}
#endif

#if (MOO_SIZEOF_LIW_T == MOO_SIZEOF_INT) && defined(MOO_HAVE_BUILTIN_UMUL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_umul_overflow(a,b,c)
#elif (MOO_SIZEOF_LIW_T == MOO_SIZEOF_LONG) && defined(MOO_HAVE_BUILTIN_UMULL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (MOO_SIZEOF_LIW_T == MOO_SIZEOF_LONG_LONG) && defined(MOO_HAVE_BUILTIN_UMULLL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static MOO_INLINE int liw_mul_overflow (moo_liw_t a, moo_liw_t b, moo_liw_t* c)
{
#if (MOO_SIZEOF_UINTMAX_T > MOO_SIZEOF_LIW_T)
	moo_uintmax_t k;
	k = (moo_uintmax_t)a * (moo_uintmax_t)b;
	*c = (moo_liw_t)k;
	return (k >> MOO_LIW_BITS) > 0;
	/*return k > MOO_TYPE_MAX(moo_liw_t);*/
#else
	*c = a * b;
	return b != 0 && a > MOO_TYPE_MAX(moo_liw_t) / b; /* works for unsigned types only */
#endif
}
#endif

static int is_normalized_integer (moo_t* moo, moo_oop_t oop)
{
	if (MOO_OOP_IS_SMOOI(oop)) return 1;
	if (MOO_OOP_IS_POINTER(oop))
	{
/* TODO: is it better to introduce a special integer mark into the class itself */
/* TODO: or should it check if it's a subclass, subsubclass, subsubsubclass, etc of a large_integer as well? */
		if (MOO_POINTER_IS_BIGINT(moo, oop))
		{
			moo_oow_t sz;

			sz = MOO_OBJ_GET_SIZE(oop);
			MOO_ASSERT (moo, sz >= 1);

			return MOO_OBJ_GET_LIWORD_SLOT(oop)[sz - 1] == 0? 0: 1;
		}
	}

	return 0;
}

MOO_INLINE static int is_bigint (moo_t* moo, moo_oop_t x)
{
	if (!MOO_OOP_IS_POINTER(x)) return 0;

/* TODO: is it better to introduce a special integer mark into the class itself */
/* TODO: or should it check if it's a subclass, subsubclass, subsubsubclass, etc of a large_integer as well? */
	return MOO_POINTER_IS_BIGINT(moo, x);
}

MOO_INLINE int moo_isint (moo_t* moo, moo_oop_t x)
{
	if (MOO_OOP_IS_SMOOI(x)) return 1;
	if (MOO_OOP_IS_POINTER(x)) return is_bigint(moo, x);
	return 0;
}

static MOO_INLINE int bigint_to_oow (moo_t* moo, moo_oop_t num, moo_oow_t* w)
{
	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(num));
	MOO_ASSERT (moo, MOO_POINTER_IS_PBIGINT(moo, num) || MOO_POINTER_IS_NBIGINT(moo, num));

#if (MOO_LIW_BITS == MOO_OOW_BITS)
	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(num) >= 1);
	if (MOO_OBJ_GET_SIZE(num) == 1)
	{
		*w = MOO_OBJ_GET_WORD_SLOT(num)[0];
		return (MOO_POINTER_IS_NBIGINT(moo, num))? -1: 1;
	}

#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
	/* this function must be called with a real large integer.
	 * a real large integer is at least 2 half-word long.
	 * you must not call this function with an unnormalized
	 * large integer. */

	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(num) >= 2);
	if (MOO_OBJ_GET_SIZE(num) == 2)
	{
		*w = MAKE_WORD (MOO_OBJ_GET_HALFWORD_SLOT(num)[0], MOO_OBJ_GET_HALFWORD_SLOT(num)[1]);
		return (MOO_POINTER_IS_NBIGINT(moo, num))? -1: 1;
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

	moo_seterrnum (moo, MOO_ERANGE);
	return 0; /* not convertable */
}

static MOO_INLINE int integer_to_oow (moo_t* moo, moo_oop_t x, moo_oow_t* w)
{
	/* return value 
	 *   1 - a positive number including 0 that can fit into moo_oow_t
	 *  -1 - a negative number whose absolute value can fit into moo_oow_t
	 *   0 - number too large or too small
	 */

	if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		v = MOO_OOP_TO_SMOOI(x);
		if (v < 0)
		{
			*w = -v;
			return -1;
		}
		else
		{
			*w = v;
			return 1;
		}
	}

	MOO_ASSERT (moo, is_bigint(moo, x));
	return bigint_to_oow(moo, x, w);
}

int moo_inttooow (moo_t* moo, moo_oop_t x, moo_oow_t* w)
{
	if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		v = MOO_OOP_TO_SMOOI(x);
		if (v < 0)
		{
			*w = -v;
			return -1; /* negative number negated - kind of an error */
		}
		else
		{
			*w = v;
			return 1; /* zero or positive number */
		}
	}

	if (is_bigint(moo, x)) return bigint_to_oow(moo, x, w);

	moo_seterrbfmt (moo, MOO_EINVAL, "not an integer - %O", x);
	return 0; /* not convertable - too big, too small, or not an integer */
}

int moo_inttoooi (moo_t* moo, moo_oop_t x, moo_ooi_t* i)
{
	moo_oow_t w;
	int n;

	n = moo_inttooow (moo, x, &w);
	if (n < 0) 
	{
		MOO_ASSERT (moo, MOO_TYPE_MAX(moo_ooi_t) + MOO_TYPE_MIN(moo_ooi_t) == -1); /* assume 2's complement */
		if (w > (moo_oow_t)MOO_TYPE_MAX(moo_ooi_t) + 1)
		{
			moo_seterrnum (moo, MOO_ERANGE); /* not convertable. number too small */
			return 0;
		}
		*i = -w;
	}
	else if (n > 0) 
	{
		if (w > MOO_TYPE_MAX(moo_ooi_t)) 
		{
			moo_seterrnum (moo, MOO_ERANGE); /* not convertable. number too big */
			return 0;
		}
		*i = w;
	}

	return n;
}

static MOO_INLINE moo_oop_t make_bigint_with_oow (moo_t* moo, moo_oow_t w)
{
#if (MOO_LIW_BITS == MOO_OOW_BITS)
	MOO_ASSERT (moo, MOO_SIZEOF(moo_oow_t) == MOO_SIZEOF(moo_liw_t));
	return moo_instantiate(moo, moo->_large_positive_integer, &w, 1);
#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
	moo_liw_t hw[2];
	hw[0] = w /*& MOO_LBMASK(moo_oow_t,MOO_LIW_BITS)*/;
	hw[1] = w >> MOO_LIW_BITS;
	return moo_instantiate(moo, moo->_large_positive_integer, &hw, (hw[1] > 0? 2: 1));
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
}

static MOO_INLINE moo_oop_t make_bigint_with_ooi (moo_t* moo, moo_ooi_t i)
{
#if (MOO_LIW_BITS == MOO_OOW_BITS)
	moo_oow_t w;

	MOO_ASSERT (moo, MOO_SIZEOF(moo_oow_t) == MOO_SIZEOF(moo_liw_t));
	if (i >= 0)
	{
		w = i;
		return moo_instantiate(moo, moo->_large_positive_integer, &w, 1);
	}
	else
	{
		/* The caller must ensure that i is greater than the smallest value
		 * that moo_ooi_t can represent. otherwise, the absolute value 
		 * cannot be held in moo_ooi_t. */
		MOO_ASSERT (moo, i > MOO_TYPE_MIN(moo_ooi_t));
		w = -i;
		return moo_instantiate(moo, moo->_large_negative_integer, &w, 1);
	}
#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
	moo_liw_t hw[2];
	moo_oow_t w;

	if (i >= 0)
	{
		w = i;
		hw[0] = w /*& MOO_LBMASK(moo_oow_t,MOO_LIW_BITS)*/;
		hw[1] = w >> MOO_LIW_BITS;
		return moo_instantiate(moo, moo->_large_positive_integer, &hw, (hw[1] > 0? 2: 1));
	}
	else
	{
		MOO_ASSERT (moo, i > MOO_TYPE_MIN(moo_ooi_t));
		w = -i;
		hw[0] = w /*& MOO_LBMASK(moo_oow_t,MOO_LIW_BITS)*/;
		hw[1] = w >> MOO_LIW_BITS;
		return moo_instantiate(moo, moo->_large_negative_integer, &hw, (hw[1] > 0? 2: 1));
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
}

static MOO_INLINE moo_oop_t make_bloated_bigint_with_ooi (moo_t* moo, moo_ooi_t i, moo_oow_t extra)
{
#if (MOO_LIW_BITS == MOO_OOW_BITS)
	moo_oow_t w;
	moo_oop_t z;

	MOO_ASSERT (moo, extra <= MOO_OBJ_SIZE_MAX - 1); 
	MOO_ASSERT (moo, MOO_SIZEOF(moo_oow_t) == MOO_SIZEOF(moo_liw_t));
	if (i >= 0)
	{
		w = i;
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, 1 + extra);
	}
	else
	{
		MOO_ASSERT (moo, i > MOO_TYPE_MIN(moo_ooi_t));
		w = -i;
		z = moo_instantiate(moo, moo->_large_negative_integer, MOO_NULL, 1 + extra);
	}

	if (!z) return MOO_NULL;
	MOO_OBJ_GET_LIWORD_SLOT(z)[0] = w;
	return z;

#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
	moo_liw_t hw[2];
	moo_oow_t w;
	moo_oop_t z;

	MOO_ASSERT (moo, extra <= MOO_OBJ_SIZE_MAX - 2); 
	if (i >= 0)
	{
		w = i;
		hw[0] = w /*& MOO_LBMASK(moo_oow_t,MOO_LIW_BITS)*/;
		hw[1] = w >> MOO_LIW_BITS;
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, (hw[1] > 0? 2: 1) + extra);
	}
	else
	{
		MOO_ASSERT (moo, i > MOO_TYPE_MIN(moo_ooi_t));
		w = -i;
		hw[0] = w /*& MOO_LBMASK(moo_oow_t,MOO_LIW_BITS)*/;
		hw[1] = w >> MOO_LIW_BITS;
		z = moo_instantiate(moo, moo->_large_negative_integer, MOO_NULL, (hw[1] > 0? 2: 1) + extra);
	}

	if (!z) return MOO_NULL;
	MOO_OBJ_GET_LIWORD_SLOT(z)[0] = hw[0];
	if (hw[1] > 0) MOO_OBJ_GET_LIWORD_SLOT(z)[1] = hw[1];
	return z;
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
}

static MOO_INLINE moo_oop_t make_bigint_with_intmax (moo_t* moo, moo_intmax_t v)
{
	moo_oow_t len;
	moo_liw_t buf[MOO_SIZEOF_INTMAX_T / MOO_SIZEOF_LIW_T];
	moo_uintmax_t ui;

	/* this is not a generic function. it can't handle v 
	 * if it's MOO_TYPE_MIN(moo_intmax_t) */
	MOO_ASSERT (moo, v > MOO_TYPE_MIN(moo_intmax_t));

	ui = (v >= 0)? v: -v;
	len = 0;
	do
	{
		buf[len++] = (moo_liw_t)ui;
		ui = ui >> MOO_LIW_BITS;
	}
	while (ui > 0);

	return moo_instantiate(moo, ((v >= 0)? moo->_large_positive_integer: moo->_large_negative_integer), buf, len);
}

moo_oop_t moo_oowtoint (moo_t* moo, moo_oow_t w)
{
	MOO_ASSERT (moo, MOO_TYPE_IS_UNSIGNED(moo_oow_t));
	/*if (MOO_IN_SMOOI_RANGE(w))*/
	if (w <= MOO_SMOOI_MAX)
	{
		return MOO_SMOOI_TO_OOP(w);
	}
	else
	{
		return make_bigint_with_oow(moo, w);
	}
}

moo_oop_t moo_ooitoint (moo_t* moo, moo_ooi_t i)
{
	if (MOO_IN_SMOOI_RANGE(i))
	{
		return MOO_SMOOI_TO_OOP(i);
	}
	else
	{
		return make_bigint_with_ooi(moo, i);
	}
}

static MOO_INLINE moo_oop_t expand_bigint (moo_t* moo, moo_oop_t oop, moo_oow_t inc)
{
	moo_oop_t z;
	moo_oow_t i;
	moo_oow_t count;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
	count = MOO_OBJ_GET_SIZE(oop);

	if (inc > MOO_OBJ_SIZE_MAX - count)
	{
		moo_seterrnum (moo, MOO_EOOMEM); /* TODO: is it a soft failure or a hard failure? is this error code proper? */
		return MOO_NULL;
	}

	moo_pushvolat (moo, &oop);
	z = moo_instantiate(moo, MOO_OBJ_GET_CLASS(oop), MOO_NULL, count + inc);
	moo_popvolat (moo);
	if (!z) return MOO_NULL;

	for (i = 0; i < count; i++)
	{
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(oop)[i];
	}
	return z;
}


static MOO_INLINE moo_oop_t _clone_bigint (moo_t* moo, moo_oop_t oop, moo_oow_t count, moo_oop_class_t _class)
{
	moo_oop_t z;
	moo_oow_t i;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
	if (count <= 0) count = MOO_OBJ_GET_SIZE(oop);

	moo_pushvolat (moo, &oop);
	z = moo_instantiate(moo, _class, MOO_NULL, count);
	moo_popvolat (moo);
	if (!z) return MOO_NULL;

	for (i = 0; i < count; i++)
	{
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(oop)[i];
	}
	return z;
}

static MOO_INLINE moo_oop_t clone_bigint (moo_t* moo, moo_oop_t oop, moo_oow_t count)
{
	return _clone_bigint (moo, oop, count, MOO_OBJ_GET_CLASS(oop));
}

static MOO_INLINE moo_oop_t clone_bigint_negated (moo_t* moo, moo_oop_t oop, moo_oow_t count)
{
	moo_oop_class_t _class;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
	if (MOO_POINTER_IS_PBIGINT(moo, oop))
	{
		_class = moo->_large_negative_integer;
	}
	else
	{
		MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, oop));
		_class = moo->_large_positive_integer;
	}

	return _clone_bigint (moo, oop, count, _class);
}

static MOO_INLINE moo_oop_t clone_bigint_to_positive (moo_t* moo, moo_oop_t oop, moo_oow_t count)
{
	return _clone_bigint (moo, oop, count, moo->_large_positive_integer);
}

static MOO_INLINE moo_oow_t count_effective (moo_liw_t* x, moo_oow_t xs)
{
#if 0
	while (xs > 1 && x[xs - 1] == 0) xs--;
	return xs;
#else
	while (xs > 1) { if (x[--xs]) return xs + 1; }
	return 1;
#endif
}

static MOO_INLINE moo_oow_t count_effective_digits (moo_oop_t oop)
{
	moo_oow_t i;

	for (i = MOO_OBJ_GET_SIZE(oop); i > 1; )
	{
		--i;
		if (MOO_OBJ_GET_LIWORD_SLOT(oop)[i]) return i + 1;
	}

	return 1;
}

static moo_oop_t normalize_bigint (moo_t* moo, moo_oop_t oop)
{
	moo_oow_t count;

	if (MOO_OOP_IS_SMOOI(oop)) return oop;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
	count = count_effective_digits(oop);

#if (MOO_LIW_BITS == MOO_OOW_BITS)
	if (count == 1) /* 1 word */
	{
		moo_oow_t w;

		w = MOO_OBJ_GET_LIWORD_SLOT(oop)[0];
		if (MOO_POINTER_IS_PBIGINT(moo, oop))
		{
			if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP(w);
		}
		else
		{
			MOO_ASSERT (moo, -MOO_SMOOI_MAX  == MOO_SMOOI_MIN);
			MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, oop));
			if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP(-(moo_ooi_t)w);
		}
	}
#elif (MOO_LIW_BITS == MOO_OOHW_BITS)

	if (count == 1) /* 1 half-word */
	{
		if (MOO_POINTER_IS_PBIGINT(moo, oop))
		{
			return MOO_SMOOI_TO_OOP(MOO_OBJ_GET_LIWORD_SLOT(oop)[0]);
		}
		else
		{
			MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, oop));
			return MOO_SMOOI_TO_OOP(-(moo_ooi_t)MOO_OBJ_GET_LIWORD_SLOT(oop)[0]);
		}
	}
	else if (count == 2) /* 2 half-words */
	{
		moo_oow_t w;

		w = MAKE_WORD (MOO_OBJ_GET_LIWORD_SLOT(oop)[0], MOO_OBJ_GET_LIWORD_SLOT(oop)[1]);
		if (MOO_POINTER_IS_PBIGINT(moo, oop))
		{
			if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP(w);
		}
		else
		{
			MOO_ASSERT (moo, -MOO_SMOOI_MAX  == MOO_SMOOI_MIN);
			MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, oop));
			if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP(-(moo_ooi_t)w);
		}
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
	if (MOO_OBJ_GET_SIZE(oop) == count)
	{
		/* no compaction is needed. return it as it is */
		return oop;
	}

	return clone_bigint(moo, oop, count);
}

static MOO_INLINE int is_less_unsigned_array (const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys)
{
	moo_oow_t i;

	if (xs != ys) return xs < ys;
	for (i = xs; i > 0; )
	{
		--i;
		if (x[i] != y[i]) return x[i] < y[i];
	}

	return 0;
}

static MOO_INLINE int is_less_unsigned (moo_oop_t x, moo_oop_t y)
{
	return is_less_unsigned_array (
		MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x), 
		MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y));
}

static MOO_INLINE int is_less (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OBJ_GET_CLASS(x) != MOO_OBJ_GET_CLASS(y))
	{
		return MOO_POINTER_IS_NBIGINT(moo, x);
	}

	if (MOO_POINTER_IS_PBIGINT(moo, x))
	{
		return is_less_unsigned(x, y);
	}
	else
	{
		return is_less_unsigned(y, x);
	}
}

static MOO_INLINE int is_greater_unsigned_array (const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys)
{
	moo_oow_t i;

	if (xs != ys) return xs > ys;
	for (i = xs; i > 0; )
	{
		--i;
		if (x[i] != y[i]) return x[i] > y[i];
	}

	return 0;
}

static MOO_INLINE int is_greater_unsigned (moo_oop_t x, moo_oop_t y)
{
	return is_greater_unsigned_array(
		MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x), 
		MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y));
}

static MOO_INLINE int is_greater (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OBJ_GET_CLASS(x) != MOO_OBJ_GET_CLASS(y))
	{
		return MOO_POINTER_IS_NBIGINT(moo, y);
	}

	if (MOO_POINTER_IS_PBIGINT(moo, x))
	{
		return is_greater_unsigned(x, y);
	}
	else
	{
		return is_greater_unsigned(y, x);
	}
}

static MOO_INLINE int is_equal_unsigned_array (const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys)
{
	return xs == ys && MOO_MEMCMP(x, y, xs * MOO_SIZEOF(*x)) == 0;
}

static MOO_INLINE int is_equal_unsigned (moo_oop_t x, moo_oop_t y)
{
	return is_equal_unsigned_array(
		MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x), 
		MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y));
}

static MOO_INLINE int is_equal (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	/* check if two large integers are equal to each other */
	/*return MOO_OBJ_GET_CLASS(x) == MOO_OBJ_GET_CLASS(y) && MOO_OBJ_GET_SIZE(x) == MOO_OBJ_GET_SIZE(y) &&
	       MOO_MEMCMP(MOO_OBJ_GET_LIWORD_SLOT(x),  MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(x) * MOO_SIZEOF(moo_liw_t)) == 0;*/
	return MOO_OBJ_GET_CLASS(x) == MOO_OBJ_GET_CLASS(y) && is_equal_unsigned(x, y);
}

static void complement2_unsigned_array (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, moo_liw_t* z)
{
	moo_oow_t i;
	moo_lidw_t w;
	moo_lidw_t carry;

	/* get 2's complement (~x + 1) */

	carry = 1; 
	for (i = 0; i < xs; i++)
	{
		w = (moo_lidw_t)(~x[i]) + carry;
		/*w = (moo_lidw_t)(x[i] ^ (~(moo_liw_t)0)) + carry;*/
		carry = w >> MOO_LIW_BITS;
		z[i] = w;
	}

	/* if the array pointed to by x contains all zeros, carry will be
	 * 1 here and it actually requires 1 more slot. Let't take this 8-bit
	 * zero for instance:
	 *    2r00000000  -> 2r11111111 + 1 => 2r0000000100000000
	 * 
	 * this function is not designed to handle such a case.
	 * in fact, 0 is a small integer and it must not stand a change 
	 * to be given to this function */
	MOO_ASSERT (moo, carry == 0);
}

static MOO_INLINE moo_oow_t add_unsigned_array (const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* z)
{
#if 1
	register moo_oow_t i;
	moo_lidw_t w;

	if (xs < ys)
	{
		/* swap x and y */
		i = xs;
		xs = ys;
		ys = i;

		i = (moo_oow_t)x;
		x = y;
		y = (moo_liw_t*)i;
	}

	w = 0;
	i = 0;
	while (i < ys)
	{
		w += (moo_lidw_t)x[i] + (moo_lidw_t)y[i];
		z[i++] = w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS);
		w >>= MOO_LIW_BITS;
	}

	while (w && i < xs)
	{
		w += x[i];
		z[i++] = w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS);
		w >>= MOO_LIW_BITS;
	}

	while (i < xs)
	{
		z[i] = x[i];
		i++;
	}
	if (w) z[i++] = w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS);
	return i;

#else
	register moo_oow_t i;
	moo_lidw_t w;
	moo_liw_t carry = 0;

	if (xs < ys)
	{
		/* swap x and y */
		i = xs;
		xs = ys;
		ys = i;

		i = (moo_oow_t)x;
		x = y;
		y = (moo_liw_t*)i;
	}


	for (i = 0; i < ys; i++)
	{
		w = (moo_lidw_t)x[i] + (moo_lidw_t)y[i] + carry;
		carry = w >> MOO_LIW_BITS;
		z[i] = w /*& MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS) */;
	}

	if (x == z)
	{
		for (; carry && i < xs; i++)
		{
			w = (moo_lidw_t)x[i] + carry;
			carry = w >> MOO_LIW_BITS;
			z[i] = w /*& MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS) */;
		}
		i = xs;
	}
	else
	{
		for (; i < xs; i++)
		{
			w = (moo_lidw_t)x[i] + carry;
			carry = w >> MOO_LIW_BITS;
			z[i] = w /*& MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS)*/;
		}
	}

	if (carry) z[i++] = carry;
	return i; /* the number of effective digits in the result */
#endif
}

static MOO_INLINE moo_oow_t subtract_unsigned_array (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* z)
{
#if 1
	moo_oow_t i;
	moo_lidi_t w = 0;

	if (x == y)
	{
		MOO_ASSERT (moo, xs == ys);
		z[0] = 0;
		return 1;
	}

	MOO_ASSERT (moo, !is_less_unsigned_array(x, xs, y, ys));

	for (i = 0; i < ys; i++)
	{
		w += (moo_lidi_t)x[i] - (moo_lidi_t)y[i];
		z[i] = w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS);
		w >>= MOO_LIW_BITS;
	}

	while (w && i < xs)
	{
		w += x[i];
		z[i++] = w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS);
		w >>= MOO_LIW_BITS;
	}

	while (i < xs)
	{
		z[i] = x[i];
		i++;
	}

	while (i > 1 && z[i - 1] == 0) i--;
	return i;

#else
	moo_oow_t i;
	moo_lidw_t w;
	moo_lidw_t borrow = 0;
	moo_lidw_t borrowed_word;

	if (x == y)
	{
		MOO_ASSERT (moo, xs == ys);
		z[0] = 0;
		return 1;
	}

	MOO_ASSERT (moo, !is_less_unsigned_array(x, xs, y, ys));

	borrowed_word = (moo_lidw_t)1 << MOO_LIW_BITS;
	for (i = 0; i < ys; i++)
	{
		w = (moo_lidw_t)y[i] + borrow;
		if ((moo_lidw_t)x[i] >= w)
		{
			z[i] = x[i] - w;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (moo_lidw_t)x[i]) - w; 
			borrow = 1;
		}
	}

	for (; i < xs; i++)
	{
		if (x[i] >= borrow) 
		{
			z[i] = x[i] - (moo_liw_t)borrow;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (moo_lidw_t)x[i]) - borrow;
			borrow = 1;
		}
	}

	MOO_ASSERT (moo, borrow == 0);

	while (i > 1 && z[i - 1] == 0) i--;
	return i; /* the number of effective digits in the result */
#endif
}

static MOO_INLINE void multiply_unsigned_array (const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* z)
{
	moo_lidw_t v;
	moo_oow_t pa;

	if (xs < ys)
	{
		moo_oow_t i;

		/* swap x and y */
		i = xs;
		xs = ys;
		ys = i;

		i = (moo_oow_t)x;
		x = y;
		y = (moo_liw_t*)i;
	}

	if (ys == 1)
	{
		moo_lidw_t dw;
		moo_liw_t carry = 0;
		moo_oow_t i;

		for (i = 0; i < xs; i++)
		{
			dw = ((moo_lidw_t)x[i] * y[0]) + carry;
			carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
			z[i] = (moo_liw_t)dw;
		}

		z[i] = carry;
		return;
	}

	pa = xs;
	if (pa <= ((moo_oow_t)1 << (MOO_LIDW_BITS - (MOO_LIW_BITS * 2))))
	{
		/* Comba(column-array) multiplication */

		/* when the input length is too long, v may overflow. if it
		 * happens, comba's method doesn't work as carry propagation is
		 * affected badly. so we need to use this method only if
		 * the input is short enough. */

		moo_oow_t pa, ix, iy, iz, tx, ty;

		pa = xs + ys;
		v = 0;
		for (ix = 0; ix < pa; ix++)
		{
			ty = (ix < ys - 1)? ix: (ys - 1);
			tx = ix - ty;
			iy = (ty + 1 < xs - tx)? (ty + 1): (xs - tx);

			for (iz = 0; iz < iy; iz++)
			{
				v = v + (moo_lidw_t)x[tx + iz] * (moo_lidw_t)y[ty - iz];
			}

			z[ix] = (moo_liw_t)v;
			v = v >> MOO_LIW_BITS;
		}
	}
	else
	{
		moo_oow_t i, j;
		moo_liw_t carry;

		for (i = 0; i < xs; i++)
		{
			if (x[i] == 0)
			{
				z[i + ys] = 0;
			}
			else
			{
				carry = 0;

				for (j = 0; j < ys; j++)
				{
					v = (moo_lidw_t)x[i] * (moo_lidw_t)y[j] + (moo_lidw_t)carry + (moo_lidw_t)z[i + j];
					z[i + j] = (moo_liw_t)v;
					carry = (moo_liw_t)(v >> MOO_LIW_BITS);
				}

				z[i + j] = carry;
			}
		}
	}
}

/* KARATSUBA MULTIPLICATION
 * 
 * c = |a| * |b|
 *
 * Let B represent the radix(2^DIGIT_BITS)
 * Let n represent half the number of digits
 *
 * a = a1 * B^n + a0
 * b = b1 * B^n + b0
 * a * b => a1b1 * B^2n + ((a1 + a0)(b1 + b0) - (a0b0 + a1b1)) * B^n + a0b0
 *
 * --------------------------------------------------------------------
 * For example, for 2 number 0xFAC2 and 0xABCD => A848635A
 *   DIGIT_BITS = 8 (1 byte, each digit is 1 byte long)
 *   B = 2^8 = 0x100
 *   n = 1 (half the digits of 2 digit numbers)
 *   B^n = 0x100 ^ 1 = 0x100
 *   B^2n = 0x100 ^ 2 = 0x10000
 *   0xFAC2 = 0xFA * 0x100 + 0xC2
 *   0xABCD = 0xAB * 0x100 + 0xCD
 *   a1 = 0xFA, a0 = 0xC2
 *   b1 = 0xAB, b0 = 0xCD
 *   a1b1 = 0xFA * 0xAB = 0xA6FE
 *   a0b0 = 0xC2 * 0xCD = 0x9B5A
 *   a1 + a0 = 0xFA + 0xC2 = 0x1BC
 *   b1 + b0 = 0xAB + 0xCD = 0x178
 * --------------------------------------------------------------------
 *   (A6FE * 10000) + (((1BC * 178) - (985A + A6FE)) * 100) + 9B5A =
 *   (A6FE << (8 * 2)) + (((1BC * 178) - (985A + A6FE)) << (8 * 1)) =
 *   A6FE0000 + 14CC800 + 9B5A = 9848635A 
 * --------------------------------------------------------------------
 *
 * 0xABCD9876 * 0xEFEFABAB => 0xA105C97C9755A8D2
 * B = 2^8 = 0x100
 * n = 2
 * B^n = 0x100 ^ 2 = 0x10000
 * B^2n = 0x100 ^ 4 = 0x100000000
 * 0xABCD9876 = 0xABCD * 0x10000 + 0x9876
 * 0xEFEFABAB = 0xEFEF * 0x10000 + 0xABAB
 * a1 = 0xABCD, a0 = 0x9876
 * b1 - 0xEFEF, b0 = 0xABAB
 * a1b1 = 0xA104C763
 * a0b0 = 0x663CA8D2
 * a1 + a0 = 0x14443
 * b1 + b0 = 0x19B9A
 * --------------------------------------------------------------------
 * (A104C763 * 100000000) + (((14443 * 19B9A) - (663CA8D2 + A104C763)) * 10000) + 663CA8D2 =
 * (A104C763 << (8 * 4)) + (((14443 * 19B9A) - (663CA8D2 + A104C763)) << (8 * 2)) + 663CA8D2 = A105C97C9755A8D2
 * --------------------------------------------------------------------
 *
 *  Multiplying by B is t same as shifting by DIGIT_BITS.
 *  DIGIT_BITS in this implementation is MOO_LIW_BITS
 *  B => 2^MOO_LIW_BITS
 *  X * B^n => X << (MOO_LIW_BITS * n)
 *  X * B^2n => X << (MOO_LIW_BITS * n * 2)
 * --------------------------------------------------------------------
 */
 
#if defined(MOO_BUILD_DEBUG)
#define CANNOT_KARATSUBA(moo,xs,ys) \
	((xs) < (moo)->option.karatsuba_cutoff || (ys) < (moo)->option.karatsuba_cutoff || \
	((xs) > (ys) && (ys) <= (((xs) + 1) / 2)) || \
	((xs) < (ys) && (xs) <= (((ys) + 1) / 2)))
#else
#define CANNOT_KARATSUBA(moo,xs,ys) \
	((xs) < MOO_KARATSUBA_CUTOFF || (ys) < MOO_KARATSUBA_CUTOFF || \
	((xs) > (ys) && (ys) <= (((xs) + 1) / 2)) || \
	((xs) < (ys) && (xs) <= (((ys) + 1) / 2)))
#endif

static MOO_INLINE moo_oow_t multiply_unsigned_array_karatsuba (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* z)
{
#if 1
	moo_lidw_t nshifts;
	moo_lidw_t ndigits_xh, ndigits_xl;
	moo_lidw_t ndigits_yh, ndigits_yl;
	moo_liw_t* tmp[2] = { MOO_NULL, MOO_NULL};
	moo_liw_t* zsp; 
	moo_oow_t tmplen[2];
	moo_oow_t xlen, zcapa;

	zcapa = xs + ys; /* the caller ensures this capacity for z at the minimum*/

	if (xs < ys)
	{
		moo_oow_t i;

		/* swap x and y */
		i = xs;
		xs = ys;
		ys = i;

		i = (moo_oow_t)x;
		x = y;
		y = (moo_liw_t*)i;
	}

	if (ys == 1)
	{
		moo_lidw_t dw;
		moo_liw_t carry = 0;
		moo_oow_t i;

		for (i = 0; i < xs; i++)
		{
			dw = ((moo_lidw_t)x[i] * y[0]) + carry;
			carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
			z[i] = (moo_liw_t)dw;
		}

		z[i] = carry;
		return count_effective(z, xs + 1);
	}

	/* calculate value of nshifts, that is 2^(MOO_LIW_BITS*nshifts) */
	nshifts = (xs + 1) / 2;

	ndigits_xl = nshifts; /* ndigits of lower part of x */
	ndigits_xh = xs - nshifts; /* ndigits of upper part of x */
	ndigits_yl = nshifts; /* ndigits of lower part of y */
	ndigits_yh = ys - nshifts; /* ndigits of uppoer part of y */

	MOO_ASSERT (moo, ndigits_xl >= ndigits_xh);
	MOO_ASSERT (moo, ndigits_yl >= ndigits_yh);

	/* make a temporary buffer for (b0 + b1) and (a1 * b1) */
	tmplen[0] = ndigits_xh + ndigits_yh;
	tmplen[1] = ndigits_yl + ndigits_yh + 1; 
	if (tmplen[1] < tmplen[0]) tmplen[1] = tmplen[0];
	tmp[1] = (moo_liw_t*)moo_callocmem(moo, MOO_SIZEOF(moo_liw_t) * tmplen[1]); /* TODO: should i use the object memory? if not, reuse the buffer and minimize memory allocation */
	if (!tmp[1]) goto oops;

	/* make a temporary for (a0 + a1) and (a0 * b0) */
	tmplen[0] = ndigits_xl + ndigits_yl + 1;
	tmp[0] = (moo_liw_t*)moo_callocmem(moo, MOO_SIZEOF(moo_liw_t) * tmplen[0]);
	if (!tmp[0]) goto oops;

	/* tmp[0] = a0 + a1 */
	tmplen[0] = add_unsigned_array(x, ndigits_xl, x + nshifts, ndigits_xh, tmp[0]);

	/* tmp[1] = b0 + b1 */
	tmplen[1] = add_unsigned_array(y, ndigits_yl, y + nshifts, ndigits_yh, tmp[1]);

	/*MOO_DEBUG6 (moo, "karatsuba t %p u %p ndigits_xl %d ndigits_xh %d ndigits_yl %d ndigits_yh %d\n", tmp[0], tmp[1], (int)ndigits_xl, (int)ndigits_xh, (int)ndigits_yl, (int)ndigits_yh);*/
	/*MOO_DEBUG5 (moo, "zcapa %d, tmplen[0] %d tmplen[1] %d nshifts %d total %d\n", (int)zcapa, (int)tmplen[0], (int)tmplen[1], (int)nshifts, (int)(tmplen[0] + tmplen[1] + nshifts));*/

	/* place (a0 + a1) * (b0 + b1) at the shifted position */
	zsp = z + nshifts;
	if (CANNOT_KARATSUBA(moo, tmplen[0], tmplen[1]))
	{
		multiply_unsigned_array (tmp[0], tmplen[0], tmp[1], tmplen[1], zsp);
		xlen = count_effective(zsp, tmplen[0] + tmplen[1]);
	}
	else 
	{
		xlen = multiply_unsigned_array_karatsuba(moo, tmp[0], tmplen[0], tmp[1], tmplen[1], zsp);
		if (xlen == 0) goto oops;
	}

	/* tmp[0] = a0 * b0 */
	tmplen[0] = ndigits_xl + ndigits_yl;
	MOO_MEMSET (tmp[0], 0, sizeof(moo_liw_t) * tmplen[0]);
	if (CANNOT_KARATSUBA(moo, ndigits_xl, ndigits_yl))
	{
		multiply_unsigned_array (x, ndigits_xl, y, ndigits_yl, tmp[0]);
		tmplen[0] = count_effective(tmp[0], tmplen[0]);
	}
	else
	{
		tmplen[0] = multiply_unsigned_array_karatsuba(moo, x, ndigits_xl, y, ndigits_yl, tmp[0]);
		if (tmplen[0] <= 0) goto oops;
	}

	/* tmp[1] = a1 * b1 */
	tmplen[1] = ndigits_xh + ndigits_yh;
	MOO_MEMSET (tmp[1], 0, sizeof(moo_liw_t) * tmplen[1]);
	if (CANNOT_KARATSUBA(moo, ndigits_xh, ndigits_yh))
	{
		multiply_unsigned_array (x + nshifts, ndigits_xh, y + nshifts, ndigits_yh, tmp[1]);
		tmplen[1] = count_effective(tmp[1], tmplen[1]);
	}
	else
	{
		tmplen[1] = multiply_unsigned_array_karatsuba(moo, x + nshifts, ndigits_xh, y + nshifts, ndigits_yh, tmp[1]);
		if (tmplen[1] <= 0) goto oops;
	}

	/* (a0+a1)*(b0+b1) -(a0*b0) */
	xlen = subtract_unsigned_array(moo, zsp, xlen, tmp[0], tmplen[0], zsp);

	/* (a0+a1)*(b0+b1) - (a0*b0) - (a1*b1) */
	xlen = subtract_unsigned_array(moo, zsp, xlen, tmp[1], tmplen[1], zsp);
	/* a1b1 is in tmp[1]. add (a1b1 * B^2n) to the high part of 'z' */
	zsp = z + (nshifts * 2); /* emulate shifting for "* B^2n". */
	xlen = zcapa - (nshifts * 2);
	xlen = add_unsigned_array(zsp, xlen, tmp[1], tmplen[1], zsp);

	/* z = z + a0b0. a0b0 is in tmp[0] */
	xlen = add_unsigned_array(z, zcapa, tmp[0], tmplen[0], z);

	moo_freemem (moo, tmp[1]);
	moo_freemem (moo, tmp[0]);
	return count_effective(z, xlen);

oops:
	if (tmp[1]) moo_freemem (moo, tmp[1]);
	if (tmp[0]) moo_freemem (moo, tmp[0]);
	return 0;

#else
	moo_lidw_t nshifts;
	moo_lidw_t ndigits_xh, ndigits_xl;
	moo_lidw_t ndigits_yh, ndigits_yl;
	moo_liw_t* tmp[3] = { MOO_NULL, MOO_NULL, MOO_NULL };
	moo_liw_t* zsp; 
	moo_oow_t tmplen[3];
	moo_oow_t xlen, zcapa;

	zcapa = xs + ys; /* the caller ensures this capacity for z at the minimum*/

	if (xs < ys)
	{
		moo_oow_t i;

		/* swap x and y */
		i = xs;
		xs = ys;
		ys = i;

		i = (moo_oow_t)x;
		x = y;
		y = (moo_liw_t*)i;
	}

	if (ys == 1)
	{
		moo_lidw_t dw;
		moo_liw_t carry = 0;
		moo_oow_t i;

		for (i = 0; i < xs; i++)
		{
			dw = ((moo_lidw_t)x[i] * y[0]) + carry;
			carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
			z[i] = (moo_liw_t)dw;
		}

		z[i] = carry;
		return;
	}

	/* calculate value of nshifts, that is 2^(MOO_LIW_BITS*nshifts) */
	nshifts = (xs + 1) / 2;

	ndigits_xl = nshifts; /* ndigits of lower part of x */
	ndigits_xh = xs - nshifts; /* ndigits of upper part of x */
	ndigits_yl = nshifts; /* ndigits of lower part of y */
	ndigits_yh = ys - nshifts; /* ndigits of uppoer part of y */

	MOO_ASSERT (moo, ndigits_xl >= ndigits_xh);
	MOO_ASSERT (moo, ndigits_yl >= ndigits_yh);

	/* make a temporary buffer for (b0 + b1) and (a1 * b1) */
	tmplen[0] = ndigits_yl + ndigits_yh + 1; 
	tmplen[1] = ndigits_xh + ndigits_yh;
	if (tmplen[1] < tmplen[0]) tmplen[1] = tmplen[0];
	tmp[1] = (moo_liw_t*)moo_callocmem(moo, MOO_SIZEOF(moo_liw_t) * tmplen[1]);
	if (!tmp[1]) goto oops;

	/* make a temporary for (a0 + a1) and (a0 * b0) */
	tmplen[0] = ndigits_xl + ndigits_yl;
	tmp[0] = (moo_liw_t*)moo_callocmem(moo, MOO_SIZEOF(moo_liw_t) * tmplen[0]);
	if (!tmp[0]) goto oops;

	/* tmp[0] = a0 + a1 */
	tmplen[0] = add_unsigned_array (x, ndigits_xl, x + nshifts, ndigits_xh, tmp[0]);

	/* tmp[1] = b0 + b1 */
	tmplen[1] = add_unsigned_array (y, ndigits_yl, y + nshifts, ndigits_yh, tmp[1]);

	/* tmp[2] = (a0 + a1) * (b0 + b1) */
	tmplen[2] = tmplen[0] + tmplen[1]; 
	tmp[2] = moo_callocmem(moo, MOO_SIZEOF(moo_liw_t) * tmplen[2]);
	if (!tmp[2]) goto oops;
	if (CANNOT_KARATSUBA(moo, tmplen[0], tmplen[1]))
	{
		multiply_unsigned_array (tmp[0], tmplen[0], tmp[1], tmplen[1], tmp[2]);
		xlen = count_effective(tmp[2], tmplen[2]);
	}
	else 
	{
		xlen = multiply_unsigned_array_karatsuba(moo, tmp[0], tmplen[0], tmp[1], tmplen[1], tmp[2]);
		if (xlen == 0) goto oops;
	}

	/* tmp[0] = a0 * b0 */
	tmplen[0] = ndigits_xl + ndigits_yl;
	MOO_MEMSET (tmp[0], 0, sizeof(moo_liw_t) * tmplen[0]);
	if (CANNOT_KARATSUBA(moo, ndigits_xl, ndigits_yl))
	{
		multiply_unsigned_array (x, ndigits_xl, y, ndigits_yl, tmp[0]);
		tmplen[0] = count_effective(tmp[0], tmplen[0]);
	}
	else
	{
		tmplen[0] = multiply_unsigned_array_karatsuba (moo, x, ndigits_xl, y, ndigits_yl, tmp[0]);
		if (tmplen[0] <= 0) goto oops;
	}

	/* tmp[1] = a1 * b1 */
	tmplen[1] = ndigits_xh + ndigits_yh;
	MOO_MEMSET (tmp[1], 0, sizeof(moo_liw_t) * tmplen[1]);
	if (CANNOT_KARATSUBA(moo, ndigits_xh, ndigits_yh))
	{
		multiply_unsigned_array (x + nshifts, ndigits_xh, y + nshifts, ndigits_yh, tmp[1]);
		tmplen[1] = count_effective(tmp[1], tmplen[1]);
	}
	else
	{
		tmplen[1] = multiply_unsigned_array_karatsuba (moo, x + nshifts, ndigits_xh, y + nshifts, ndigits_yh, tmp[1]);
		if (tmplen[1] <= 0) goto oops;
	}

	/* w = w - tmp[0] */
	xlen = subtract_unsigned_array(moo, tmp[2], xlen, tmp[0], tmplen[0], tmp[2]);

	/* r = w - tmp[1] */
	zsp = z + nshifts; /* emulate shifting for "* B^n" */
	xlen = subtract_unsigned_array(moo, tmp[2], xlen, tmp[1], tmplen[1], zsp);

	/* a1b1 is in tmp[1]. add (a1b1 * B^2n) to the high part of 'z' */
	zsp = z + (nshifts * 2); /* emulate shifting for "* B^2n". */
	xlen = zcapa - (nshifts * 2);
	xlen = add_unsigned_array (zsp, xlen, tmp[1], tmplen[1], zsp);

	/* z = z + a0b0. a0b0 is in tmp[0] */
	xlen = add_unsigned_array(z, zcapa, tmp[0], tmplen[0], z);

	moo_freemem (moo, tmp[2]);
	moo_freemem (moo, tmp[1]);
	moo_freemem (moo, tmp[0]);

	return count_effective(z, xlen);

oops:
	if (tmp[2]) moo_freemem (moo, tmp[2]);
	if (tmp[1]) moo_freemem (moo, tmp[1]);
	if (tmp[0]) moo_freemem (moo, tmp[0]);
	return 0;
#endif
}

static MOO_INLINE void lshift_unsigned_array (moo_liw_t* x, moo_oow_t xs, moo_oow_t bits)
{
	/* this function doesn't grow/shrink the array. Shifting is performed
	 * over the given array */

	moo_oow_t word_shifts, bit_shifts, bit_shifts_right;
	moo_oow_t si, di;

	/* get how many words to shift */
	word_shifts = bits / MOO_LIW_BITS;
	if (word_shifts >= xs)
	{
		MOO_MEMSET (x, 0, xs * MOO_SIZEOF(moo_liw_t));
		return;
	}

	/* get how many remaining bits to shift */
	bit_shifts = bits % MOO_LIW_BITS;
	bit_shifts_right = MOO_LIW_BITS - bit_shifts;

	/* shift words and bits */
	di = xs - 1;
	si = di - word_shifts;
	x[di] = x[si] << bit_shifts;
	while (di > word_shifts)
	{
		x[di] = x[di] | (x[--si] >> bit_shifts_right);
		x[--di] = x[si] << bit_shifts;
	}

	/* fill the remaining part with zeros */
	if (word_shifts > 0)
		MOO_MEMSET (x, 0, word_shifts * MOO_SIZEOF(moo_liw_t));
}

static MOO_INLINE void rshift_unsigned_array (moo_liw_t* x, moo_oow_t xs, moo_oow_t bits)
{
	/* this function doesn't grow/shrink the array. Shifting is performed
	 * over the given array */

	moo_oow_t word_shifts, bit_shifts, bit_shifts_left;
	moo_oow_t si, di, bound;

	/* get how many words to shift */
	word_shifts = bits / MOO_LIW_BITS;
	if (word_shifts >= xs)
	{
		MOO_MEMSET (x, 0, xs * MOO_SIZEOF(moo_liw_t));
		return;
	}

	/* get how many remaining bits to shift */
	bit_shifts = bits % MOO_LIW_BITS;
	bit_shifts_left = MOO_LIW_BITS - bit_shifts;

/* TODO: verify this function */
	/* shift words and bits */
	di = 0;
	si = word_shifts;
	x[di] = x[si] >> bit_shifts;
	bound = xs - word_shifts - 1;
	while (di < bound)
	{
		x[di] = x[di] | (x[++si] << bit_shifts_left);
		x[++di] = x[si] >> bit_shifts;
	}

	/* fill the remaining part with zeros */
	if (word_shifts > 0)
		MOO_MEMSET (&x[xs - word_shifts], 0, word_shifts * MOO_SIZEOF(moo_liw_t));
}

static void divide_unsigned_array (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* q, moo_liw_t* r)
{
/* TODO: this function needs to be rewritten for performance improvement. 
 *       the binary long division is extremely slow for a big number */

	/* Perform binary long division.
	 * http://en.wikipedia.org/wiki/Division_algorithm
	 * ---------------------------------------------------------------------
	 * Q := 0                 initialize quotient and remainder to zero
	 * R := 0                     
	 * for i = n-1...0 do     where n is number of bits in N
	 *   R := R << 1          left-shift R by 1 bit    
	 *   R(0) := X(i)         set the least-significant bit of R equal to bit i of the numerator
	 *   if R >= Y then
	 *     R = R - Y 
	 *     Q(i) := 1
	 *   end
	 * end 
	 */

	moo_oow_t rs, rrs, i , j;

	MOO_ASSERT (moo, xs >= ys);

	/* the caller must ensure:
	 *   - q and r are all zeros. can skip memset() with zero.
	 *   - q is as large as xs in size.
	 *   - r is as large as ys + 1 in size  */
	/*MOO_MEMSET (q, 0, MOO_SIZEOF(*q) * xs);
	MOO_MEMSET (r, 0, MOO_SIZEOF(*q) * ys);*/

	rrs = ys + 1;
	for (i = xs; i > 0; )
	{
		--i;
		for (j = MOO_LIW_BITS; j > 0;)
		{
			--j;

			/* the value of the remainder 'r' may get bigger than the 
			 * divisor 'y' temporarily until subtraction is performed
			 * below. so ys + 1(kept in rrs) is needed for shifting here. */
			lshift_unsigned_array (r, rrs, 1); 
			MOO_SETBITS (moo_liw_t, r[0], 0, 1, MOO_GETBITS(moo_liw_t, x[i], j, 1));

			rs = count_effective(r, rrs);
			if (!is_less_unsigned_array(r, rs, y, ys))
			{
				subtract_unsigned_array (moo, r, rs, y, ys, r);
				MOO_SETBITS (moo_liw_t, q[i], j, 1, 1);
			}
		}
	}
}

static MOO_INLINE moo_liw_t calculate_remainder (moo_t* moo, moo_liw_t* qr, moo_liw_t* y, moo_liw_t quo, int qr_start, int stop)
{
	moo_lidw_t dw;
	moo_liw_t b, c, c2, qyk;
	moo_oow_t j, k;

	for (b = 0, c = 0, c2 = 0, j = qr_start, k = 0; k < stop; j++, k++)
	{
		dw = (moo_lidw_t)qr[j] - b;
		b = (moo_liw_t)((dw >> MOO_LIW_BITS) & 1); /* b = -(dw mod BASE) */
		qr[j] = (moo_liw_t)dw;

		dw = ((moo_lidw_t)y[k] * quo) + c;
		c = (moo_liw_t)(dw >> MOO_LIW_BITS);
		qyk = (moo_liw_t)dw;

		dw = (moo_lidw_t)qr[j] - qyk;
		c2 = (moo_liw_t)((dw >> MOO_LIW_BITS) & 1);
		qr[j] = (moo_liw_t)dw;

		dw = (moo_lidw_t)b + c2 + c;
		c = (moo_liw_t)(dw >> MOO_LIW_BITS);
		b = (moo_liw_t)dw;

		MOO_ASSERT (moo, c == 0);
	}
	return b;
}

static void divide_unsigned_array2 (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* q, moo_liw_t* r)
{
	moo_oow_t i;
	moo_liw_t d, y1, y2;

	/* the caller must ensure:
	 *  - q can hold 'xs + 1' words and r can hold 'ys' words. 
	 *  - q and r are set to all zeros. */
	MOO_ASSERT (moo, xs >= ys);

	if (ys == 1)
	{
		/* the divisor has a single word only. perform simple division */
		moo_lidw_t dw;
		moo_liw_t carry = 0;
		for (i = xs; i > 0; )
		{
			--i;
			dw = ((moo_lidw_t)carry << MOO_LIW_BITS) + x[i];
			q[i] = (moo_liw_t)(dw / y[0]);
			carry = (moo_liw_t)(dw % y[0]);
		}
		r[0] = carry;
		return;
	}

	for (i = 0; i < xs; i++) q[i] = x[i]; /* copy x to q */
	q[xs] = 0; /* store zero in the last extra word */
	for (i = 0; i < ys; i++) r[i] = y[i]; /* copy y to r */

	y1 = r[ys - 1]; /* highest divisor word */

	/*d = (y1 == MOO_TYPE_MAX(moo_liw_t)? ((moo_liw_t)1): ((moo_liw_t)(((moo_lidw_t)1 << MOO_LIW_BITS) / (y1 + 1))));*/
	d = (moo_liw_t)(((moo_lidw_t)1 << MOO_LIW_BITS) / ((moo_lidw_t)y1 + 1));
	if (d > 1)
	{
		moo_lidw_t dw;
		moo_liw_t carry;

		/* shift the divisor such that its high-order bit is on.
		 * shift the dividend the same amount as the previous step */

		/* r = r * d */
		for (carry = 0, i = 0; i < ys; i++)
		{
			dw = ((moo_lidw_t)r[i] * d) + carry;
			carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
			r[i] = (moo_liw_t)dw;
		}
		MOO_ASSERT (moo, carry == 0);

		/* q = q * d */
		for (carry = 0, i = 0; i < xs; i++)
		{
			dw = ((moo_lidw_t)q[i] * d) + carry;
			carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
			q[i] = (moo_liw_t)dw;
		}
		q[xs] = carry;
	}

	y1 = r[ys - 1];
	y2 = r[ys - 2];

	for (i = xs; i >= ys; --i)
	{
		moo_lidw_t dw, quo, rem;
		moo_liw_t b, xhi, xlo;

		/* ---------------------------------------------------------- */
		/* estimate the quotient.
		 *  2-current-dividend-words / 2-most-significant-divisor-words */

		xhi = q[i];
		xlo = q[i - 1];

		/* adjust the quotient if over-estimated */
		dw = ((moo_lidw_t)xhi << MOO_LIW_BITS) + xlo;
		/* TODO: optimize it with ASM - no seperate / and % */
		quo = dw / y1;
		rem = dw % y1;

	adjust_quotient:
		if (quo > MOO_TYPE_MAX(moo_liw_t) || (quo * y2) > ((rem << MOO_LIW_BITS) + q[i - 2]))
		{
			--quo;
			rem += y1;
			if (rem <= MOO_TYPE_MAX(moo_liw_t)) goto adjust_quotient;
		}

		/* ---------------------------------------------------------- */
		b = calculate_remainder(moo, q, r, quo, i - ys, ys);

		b = (moo_liw_t)((((moo_lidw_t)xhi - b) >> MOO_LIW_BITS) & 1); /* is the sign bit set? */
		if (b)
		{
			/* yes. underflow */
			moo_lidw_t dw;
			moo_liw_t carry;
			moo_oow_t j, k;

			for (carry = 0, j = i - ys, k = 0; k < ys; j++, k++)
			{
				dw = (moo_lidw_t)q[j] + r[k] + carry;
				carry = (moo_liw_t)(dw >> MOO_LIW_BITS);
				q[j] = (moo_liw_t)dw;
			}

			MOO_ASSERT (moo, carry == 1);
			q[i] = quo - 1;
		}
		else
		{
			q[i] = quo;
		}
	}

	if (d > 1)
	{
		moo_lidw_t dw;
		moo_liw_t carry;
		for (carry = 0, i = ys; i > 0; )
		{
			--i;
			dw = ((moo_lidw_t)carry << MOO_LIW_BITS) + q[i];
			/* TODO: optimize it with ASM - no seperate / and % */
			q[i] = (moo_liw_t)(dw / d);
			carry = (moo_liw_t)(dw % d);
		}
	}

	/* split quotient and remainder held in q to q and r respectively 
	 *   q      [<--- quotient     ---->|<-- remainder     -->]
	 *  index   |xs  xs-1  ...  ys+1  ys|ys-1  ys-2  ...  1  0|
	 */
	for (i = 0; i < ys; i++) { r[i] = q[i]; q[i] = 0;  }
	for (; i <= xs; i++) { q[i - ys] = q[i]; q[i] = 0; }

}

static void divide_unsigned_array3 (moo_t* moo, const moo_liw_t* x, moo_oow_t xs, const moo_liw_t* y, moo_oow_t ys, moo_liw_t* q, moo_liw_t* r)
{
	moo_oow_t s, i, j, g, k;
	moo_lidw_t dw, qhat, rhat;
	moo_lidi_t di, ci;
	moo_liw_t* qq, y1, y2;

	/* the caller must ensure:
	 *  - q can hold 'xs + 1' words and r can hold 'ys' words. 
	 *  - q and r are set to all zeros. */
	MOO_ASSERT (moo, xs >= ys);

	if (ys == 1)
	{
		/* the divisor has a single word only. perform simple division */
		moo_lidw_t dw;
		moo_liw_t carry = 0;
		for (i = xs; i > 0; )
		{
			--i;
			dw = ((moo_lidw_t)carry << MOO_LIW_BITS) + x[i];
			q[i] = (moo_liw_t)(dw / y[0]);
			carry = (moo_liw_t)(dw % y[0]);
		}
		r[0] = carry;
		return;
	}

#define SHARED_QQ

#if defined(SHARED_QQ)
	/* as long as q is 2 words longer than x, this algorithm can store
	 * both quotient and remainder in q at the same time. */
	qq = q;
#else
	/* this part requires an extra buffer. proper error handling isn't easy
	 * since the return type of this function is void */
	if (moo->inttostr.t.capa <= xs)
	{
		moo_liw_t* t;
		moo_oow_t reqcapa;

		reqcapa = MOO_ALIGN_POW2(xs + 1, 32);
		t = (moo_liw_t*)moo_reallocmem(moo, moo->inttostr.t.ptr, reqcapa * MOO_SIZEOF(*t));
		/* TODO: TODO: TODO: ERROR HANDLING
		if (!t) return -1; */

		moo->inttostr.t.capa = xs + 1;
		moo->inttostr.t.ptr = t;
	}
	qq = moo->inttostr.t.ptr;
#endif

	y1 = y[ys - 1];
	/*s = MOO_LIW_BITS - ((y1 == 0)? -1: moo_get_pos_of_msb_set(y1)) - 1;*/
	MOO_ASSERT (moo, y1 > 0); /* the highest word can't be non-zero in the context where this function is called */
	s = MOO_LIW_BITS - moo_get_pos_of_msb_set(y1) - 1;
	for (i = ys; i > 1; )
	{
		--i;
		r[i] = (y[i] << s) | ((moo_lidw_t)y[i - 1] >> (MOO_LIW_BITS - s));
	}
	r[0] = y[0] << s;

	qq[xs] = (moo_lidw_t)x[xs - 1] >> (MOO_LIW_BITS - s);
	for (i = xs; i > 1; )
	{
		--i;
		qq[i] = (x[i] << s) | ((moo_lidw_t)x[i - 1] >> (MOO_LIW_BITS - s));
	}
	qq[0] = x[0] << s;

	y1 = r[ys - 1];
	y2 = r[ys - 2];

	for (j = xs; j >= ys; --j)
	{
		g = j - ys; /* position where remainder begins in qq */

		/* estimate */
		dw = ((moo_lidw_t)qq[j] << MOO_LIW_BITS) + qq[j - 1];
		qhat = dw / y1;
		rhat = dw - (qhat * y1);

	adjust_quotient:
		if (qhat > MOO_TYPE_MAX(moo_liw_t) || (qhat * y2) > ((rhat << MOO_LIW_BITS) + qq[j - 2]))
		{
			qhat = qhat - 1;
			rhat = rhat + y1;
			if (rhat <= MOO_TYPE_MAX(moo_liw_t)) goto adjust_quotient;
		}

		/* multiply and subtract */
		for (ci = 0, i = g, k = 0; k < ys; i++, k++)
		{
			dw = qhat * r[k];
			di = qq[i] - ci - (dw & MOO_TYPE_MAX(moo_liw_t));
			ci = (dw >> MOO_LIW_BITS) - (di >> MOO_LIW_BITS);
			qq[i] = (moo_liw_t)di;
		}
		MOO_ASSERT (moo, i == j);
		di = qq[i] - ci;
		qq[i] = di;

		/* test remainder */
		if (di < 0)
		{
			for (ci = 0, i = g, k = 0; k < ys; i++, k++)
			{
				di = (moo_lidw_t)qq[i] + r[k] + ci;
				ci = (moo_liw_t)(di >> MOO_LIW_BITS);
				qq[i] = (moo_liw_t)di;
			}

			MOO_ASSERT (moo, i == j);
			/*MOO_ASSERT (moo, ci == 1);*/
			qq[i] += ci;

		#if defined(SHARED_QQ)
			/* store the quotient word right after the remainder in q */
			q[i + 1] = qhat - 1;
		#else
			q[g] = qhat - 1;
		#endif
		}
		else
		{
		#if defined(SHARED_QQ)
			/* store the quotient word right after the remainder in q */
			q[i + 1] = qhat;
		#else
			q[g] = qhat;
		#endif
		}
	}

	for (i = 0; i < ys - 1; i++)
	{
		r[i] = (qq[i] >> s) | ((moo_lidw_t)qq[i + 1] << (MOO_LIW_BITS - s));
	}
	r[i] = qq[i] >> s;

#if defined(SHARED_QQ)
	for (i = 0; i <= ys; i++) { q[i] = 0;  }
	for (; i <= xs + 1; i++) { q[i - ys - 1] = q[i]; q[i] = 0; }
#endif

}

/* ======================================================================== */

static moo_oop_t add_unsigned_integers (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oow_t as, bs, zs;
	moo_oop_t z;

	as = MOO_OBJ_GET_SIZE(x);
	bs = MOO_OBJ_GET_SIZE(y);
	zs = (as >= bs? as: bs);

	if (zs >= MOO_OBJ_SIZE_MAX)
	{
		moo_seterrnum (moo, MOO_EOOMEM); /* TOOD: is it a soft failure or hard failure? */
		return MOO_NULL;
	}
	zs++;

	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &y);
	z = moo_instantiate(moo, MOO_OBJ_GET_CLASS(x), MOO_NULL, zs);
	moo_popvolats (moo, 2);
	if (!z) return MOO_NULL;

	add_unsigned_array (
		MOO_OBJ_GET_LIWORD_SLOT(x), as,
		MOO_OBJ_GET_LIWORD_SLOT(y), bs,
		MOO_OBJ_GET_LIWORD_SLOT(z)
	);

	return z;
}

static moo_oop_t subtract_unsigned_integers (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;

	MOO_ASSERT (moo, !is_less_unsigned(x, y));

	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &y);
	z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(x));
	moo_popvolats (moo, 2);
	if (!z) return MOO_NULL;

	subtract_unsigned_array (moo, 
		MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x),
		MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y),
		MOO_OBJ_GET_LIWORD_SLOT(z));
	return z;
}

static moo_oop_t multiply_unsigned_integers (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;
	moo_oow_t xs, ys;

	xs = MOO_OBJ_GET_SIZE(x);
	ys = MOO_OBJ_GET_SIZE(y);

	if (ys > MOO_OBJ_SIZE_MAX - xs)
	{
		moo_seterrnum (moo, MOO_EOOMEM); /* TOOD: is it a soft failure or hard failure? */
		return MOO_NULL;
	}

	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &y);
	z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, xs + ys);
	moo_popvolats (moo, 2);
	if (!z) return MOO_NULL;

#if defined(MOO_ENABLE_KARATSUBA)
	if (CANNOT_KARATSUBA(moo, xs, ys))
	{
#endif
		multiply_unsigned_array (
			MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x),
			MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y),
			MOO_OBJ_GET_LIWORD_SLOT(z));
#if defined(MOO_ENABLE_KARATSUBA)
	}
	else
	{
		if (multiply_unsigned_array_karatsuba (
			moo,
			MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x),
			MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y),
			MOO_OBJ_GET_LIWORD_SLOT(z)) == 0) return MOO_NULL;
	}
#endif
	return z;
}

static moo_oop_t divide_unsigned_integers (moo_t* moo, moo_oop_t x, moo_oop_t y, moo_oop_t* r)
{
	moo_oop_t qq, rr;

	if (is_less_unsigned(x, y))
	{
		rr = clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));
		if (!rr) return MOO_NULL;

		moo_pushvolat (moo, &rr);
		qq = make_bigint_with_ooi(moo, 0); /* TODO: inefficient. no need to create a bigint object for zero. */
		moo_popvolat (moo);

		if (qq) *r = rr;
		return qq;
	}
	else if (is_equal_unsigned(x, y))
	{
		rr = make_bigint_with_ooi(moo, 0); /* TODO: inefficient. no need to create a bigint object for zero. */
		if (!rr) return MOO_NULL;

		moo_pushvolat (moo, &rr);
		qq = make_bigint_with_ooi(moo, 1); /* TODO: inefficient. no need to create a bigint object for zero. */
		moo_popvolat (moo);

		if (qq) *r = rr;
		return qq;
	}

	/* the caller must ensure that x >= y */
	MOO_ASSERT (moo, !is_less_unsigned(x, y)); 
	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &y);

#define USE_DIVIDE_UNSIGNED_ARRAY2
/*#define USE_DIVIDE_UNSIGNED_ARRAY3*/

#if defined(USE_DIVIDE_UNSIGNED_ARRAY3)
	qq = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(x) + 2);
#elif defined(USE_DIVIDE_UNSIGNED_ARRAY2)
	qq = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(x) + 1);
#else
	qq = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(x));
#endif
	if (!qq) 
	{
		moo_popvolats (moo, 2);
		return MOO_NULL;
	}

	moo_pushvolat (moo, &qq);
#if defined(USE_DIVIDE_UNSIGNED_ARRAY3)
	rr = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(y));
#elif defined(USE_DIVIDE_UNSIGNED_ARRAY2) 
	rr = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(y));
#else
	rr = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, MOO_OBJ_GET_SIZE(y) + 1);
#endif
	moo_popvolats (moo, 3);
	if (!rr) return MOO_NULL;

#if defined(USE_DIVIDE_UNSIGNED_ARRAY3)
	divide_unsigned_array3 (moo,
#elif defined(USE_DIVIDE_UNSIGNED_ARRAY2)
	divide_unsigned_array2 (moo,
#else
	divide_unsigned_array (moo,
#endif
		MOO_OBJ_GET_LIWORD_SLOT(x), MOO_OBJ_GET_SIZE(x),
		MOO_OBJ_GET_LIWORD_SLOT(y), MOO_OBJ_GET_SIZE(y),
		MOO_OBJ_GET_LIWORD_SLOT(qq), MOO_OBJ_GET_LIWORD_SLOT(rr)
	);

	*r = rr;
	return qq;
}

/* ======================================================================== */

moo_oop_t moo_addints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t i;

		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		MOO_ASSERT (moo, MOO_SMOOI_MAX + MOO_SMOOI_MAX < MOO_TYPE_MAX(moo_ooi_t));
		MOO_ASSERT (moo, MOO_SMOOI_MIN + MOO_SMOOI_MIN > MOO_TYPE_MIN(moo_ooi_t));

		i = MOO_OOP_TO_SMOOI(x) + MOO_OOP_TO_SMOOI(y);
		if (MOO_IN_SMOOI_RANGE(i)) return MOO_SMOOI_TO_OOP(i);

		return make_bigint_with_ooi(moo, i);
	}
	else
	{
		moo_ooi_t v;

		if (MOO_OOP_IS_SMOOI(x))
		{
			if (!is_bigint(moo,y)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(x);
			if (v == 0) return clone_bigint(moo, y, MOO_OBJ_GET_SIZE(y));

			moo_pushvolat (moo, &y);
			x = make_bigint_with_ooi(moo, v);
			moo_popvolat (moo);
			if (!x) return MOO_NULL;
		}
		else if (MOO_OOP_IS_SMOOI(y))
		{
			if (!is_bigint(moo,x)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));

			moo_pushvolat (moo, &x);
			y = make_bigint_with_ooi(moo, v);
			moo_popvolat (moo);
			if (!y) return MOO_NULL;
		}
		else
		{
			if (!is_bigint(moo,x)) goto oops_einval;
			if (!is_bigint(moo,y)) goto oops_einval;
		}

		if (MOO_OBJ_GET_CLASS(x) != MOO_OBJ_GET_CLASS(y))
		{
			if (MOO_POINTER_IS_NBIGINT(moo, x))
			{
				/* x is negative, y is positive */
				if (is_less_unsigned(x, y))
				{
					z = subtract_unsigned_integers(moo, y, x);
					if (!z) return MOO_NULL;
				}
				else
				{
					z = subtract_unsigned_integers(moo, x, y);
					if (!z) return MOO_NULL;
					MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
				}
			}
			else
			{
				/* x is positive, y is negative */
				if (is_less_unsigned(x, y))
				{
					z = subtract_unsigned_integers(moo, y, x);
					if (!z) return MOO_NULL;
					MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
				}
				else
				{
					z = subtract_unsigned_integers(moo, x, y);
					if (!z) return MOO_NULL;
				}
			}
		}
		else
		{
			int neg;
			/* both are positive or negative */
			neg = (MOO_POINTER_IS_NBIGINT(moo, x)); 
			z = add_unsigned_integers(moo, x, y);
			if (!z) return MOO_NULL;
			if (neg) MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
		}
	}

	return normalize_bigint(moo, z);

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_subints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t i;

		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		MOO_ASSERT (moo, MOO_SMOOI_MAX - MOO_SMOOI_MIN < MOO_TYPE_MAX(moo_ooi_t));
		MOO_ASSERT (moo, MOO_SMOOI_MIN - MOO_SMOOI_MAX > MOO_TYPE_MIN(moo_ooi_t));

		i = MOO_OOP_TO_SMOOI(x) - MOO_OOP_TO_SMOOI(y);
		if (MOO_IN_SMOOI_RANGE(i)) return MOO_SMOOI_TO_OOP(i);

		return make_bigint_with_ooi(moo, i);
	}
	else
	{
		moo_ooi_t v;
		int neg;

		if (MOO_OOP_IS_SMOOI(x))
		{
			if (!is_bigint(moo,y)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(x);
			if (v == 0) 
			{
				/* switch the sign to the opposite and return it */
				return clone_bigint_negated(moo, y, MOO_OBJ_GET_SIZE(y));
			}

			moo_pushvolat (moo, &y);
			x = make_bigint_with_ooi(moo, v);
			moo_popvolat (moo);
			if (!x) return MOO_NULL;
		}
		else if (MOO_OOP_IS_SMOOI(y))
		{
			if (!is_bigint(moo,x)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));

			moo_pushvolat (moo, &x);
			y = make_bigint_with_ooi(moo, v);
			moo_popvolat (moo);
			if (!y) return MOO_NULL;
		}
		else
		{
			if (!is_bigint(moo,x)) goto oops_einval;
			if (!is_bigint(moo,y)) goto oops_einval;
		}

		if (MOO_OBJ_GET_CLASS(x) != MOO_OBJ_GET_CLASS(y))
		{
			neg = (MOO_POINTER_IS_NBIGINT(moo, x)); 
			z = add_unsigned_integers(moo, x, y);
			if (!z) return MOO_NULL;
			if (neg) MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
		}
		else
		{
			/* both are positive or negative */
			if (is_less_unsigned (x, y))
			{
				neg = (MOO_POINTER_IS_NBIGINT(moo, x));
				z = subtract_unsigned_integers (moo, y, x);
				if (!z) return MOO_NULL;
				if (!neg) MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
			}
			else
			{
				neg = (MOO_POINTER_IS_NBIGINT(moo, x)); 
				z = subtract_unsigned_integers (moo, x, y); /* take x's sign */
				if (!z) return MOO_NULL;
				if (neg) MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
			}
		}
	}

	return normalize_bigint (moo, z);

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_mulints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
	#if (MOO_SIZEOF_INTMAX_T > MOO_SIZEOF_OOI_T)
		moo_intmax_t i;
		i = (moo_intmax_t)MOO_OOP_TO_SMOOI(x) * (moo_intmax_t)MOO_OOP_TO_SMOOI(y);
		if (MOO_IN_SMOOI_RANGE(i)) return MOO_SMOOI_TO_OOP((moo_ooi_t)i);
		return make_bigint_with_intmax(moo, i);
	#else
		moo_ooi_t i;
		moo_ooi_t xv, yv;

		xv = MOO_OOP_TO_SMOOI(x);
		yv = MOO_OOP_TO_SMOOI(y);
		if (smooi_mul_overflow(moo, xv, yv, &i))
		{
			/* overflowed - convert x and y normal objects and carry on */

			/* no need to call moo_pushvolat before creating x because
			 * xv and yv contains actual values needed */
			x = make_bigint_with_ooi (moo, xv);
			if (!x) return MOO_NULL;

			moo_pushvolat (moo, &x); /* protect x made above */
			y = make_bigint_with_ooi(moo, yv);
			moo_popvolat (moo);
			if (!y) return MOO_NULL;

			goto full_multiply;
		}
		else
		{
			if (MOO_IN_SMOOI_RANGE(i)) return MOO_SMOOI_TO_OOP(i);
			return make_bigint_with_ooi(moo, i);
		}
	#endif
	}
	else
	{
		moo_ooi_t v;
		int neg;

		if (MOO_OOP_IS_SMOOI(x))
		{
			if (!is_bigint(moo,y)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(x);
			switch (v)
			{
				case 0:
					return MOO_SMOOI_TO_OOP(0);
				case 1:
					return clone_bigint(moo, y, MOO_OBJ_GET_SIZE(y));
				case -1:
					return clone_bigint_negated(moo, y, MOO_OBJ_GET_SIZE(y));
			}

			moo_pushvolat (moo, &y);
			x = make_bigint_with_ooi(moo, v);
			moo_popvolat (moo);
			if (!x) return MOO_NULL;
		}
		else if (MOO_OOP_IS_SMOOI(y))
		{
			if (!is_bigint(moo,x)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(y);
			switch (v)
			{
				case 0:
					return MOO_SMOOI_TO_OOP(0);
				case 1:
					return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));
				case -1:
					return clone_bigint_negated(moo, x, MOO_OBJ_GET_SIZE(x));
			}

			moo_pushvolat (moo, &x);
			y = make_bigint_with_ooi (moo, v);
			moo_popvolat (moo);
			if (!y) return MOO_NULL;
		}
		else
		{
			if (!is_bigint(moo,x)) goto oops_einval;
			if (!is_bigint(moo,y)) goto oops_einval;
		}

	full_multiply:
		neg = (MOO_OBJ_GET_CLASS(x) != MOO_OBJ_GET_CLASS(y)); /* checking sign before multication. no need to preserve x and y */
		z = multiply_unsigned_integers(moo, x, y);
		if (!z) return MOO_NULL;
		if (neg) MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
	}

	return normalize_bigint(moo, z);

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_divints (moo_t* moo, moo_oop_t x, moo_oop_t y, int modulo, moo_oop_t* rem)
{
	moo_oop_t z, r;
	int x_neg_sign, y_neg_sign;

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t xv, yv, q, ri;

		xv = MOO_OOP_TO_SMOOI(x);
		yv = MOO_OOP_TO_SMOOI(y);

		if (yv == 0)
		{
			moo_seterrnum (moo, MOO_EDIVBY0);
			return MOO_NULL;
		}

		if (xv == 0)
		{
			if (rem) *rem = MOO_SMOOI_TO_OOP(0);
			return MOO_SMOOI_TO_OOP(0);
		}

		/* In C89, integer division with a negative number  is
		 * implementation dependent. In C99, it truncates towards zero.
		 * 
		 * http://python-history.blogspot.kr/2010/08/why-pythons-integer-division-floors.html
		 *   The integer division operation (//) and its sibling, 
		 *   the modulo operation (%), go together and satisfy a nice
		 *   mathematical relationship (all variables are integers):
		 *      a/b = q with remainder r
		 *   such that
		 *      b*q + r = a and 0 <= r < b (assuming- a and b are >= 0).
		 * 
		 *   If you want the relationship to extend for negative a
		 *   (keeping b positive), you have two choices: if you truncate q
		 *   towards zero, r will become negative, so that the invariant
		 *   changes to 0 <= abs(r) < abs(b). otherwise, you can floor q
		 *   towards negative infinity, and the invariant remains 0 <= r < b. 
		 */

		q = xv / yv;
		MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(q));

		ri = xv - yv * q; /* xv % yv; */
		if (ri)
		{
			if (modulo)
			{
				/* modulo */
				/*
					xv      yv      q       r
					-------------------------
					 7       3      2       1
					-7       3     -3       2
					 7      -3     -3      -2
					-7      -3      2      -1
				 */

				/* r must be floored. that is, it rounds away from zero 
				 * and towards negative infinity */
				if (IS_SIGN_DIFF(yv, ri))
				{
					/* if the divisor has a different sign from r,
					 * change the sign of r to the divisor's sign */
					ri += yv;
					--q;
					MOO_ASSERT (moo, ri && !IS_SIGN_DIFF(yv, ri));
				}
			}
			else
			{
				/* remainder */
				/*
					xv      yv      q       r
					-------------------------
					 7       3      2       1
					-7       3     -2      -1
					 7      -3     -2       1
					-7      -3      2      -1
				 */
				if (xv && IS_SIGN_DIFF(xv, ri)) 
				{
					/* if the dividend has a different sign from r,
					 * change the sign of r to the dividend's sign.
					 * all the compilers i've worked with produced
					 * the quotient and the remainder that don't need
					 * any adjustment. however, there may be an esoteric
					 * architecture. */
					ri -= yv;
					++q;
					MOO_ASSERT (moo, xv && !IS_SIGN_DIFF(xv, ri));
				}
			}
		}

		if (rem)
		{
			MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(ri));
			*rem = MOO_SMOOI_TO_OOP(ri);
		}

		return MOO_SMOOI_TO_OOP((moo_ooi_t)q);
	}
	else 
	{
		moo_oop_t r;

		if (MOO_OOP_IS_SMOOI(x))
		{
			moo_ooi_t xv;

			if (!is_bigint(moo,y)) goto oops_einval;

			/* divide a small integer by a big integer. 
			 * the dividend is guaranteed to be greater than the divisor
			 * if both are positive. */

			xv = MOO_OOP_TO_SMOOI(x);
			x_neg_sign = (xv < 0);
			y_neg_sign = MOO_POINTER_IS_NBIGINT(moo, y);
			if (x_neg_sign == y_neg_sign || !modulo)
			{
				/* simple. the quotient is zero and the
				 * dividend becomes the remainder as a whole. */
				if (rem) *rem = x;
				return MOO_SMOOI_TO_OOP(0);
			}

			/* carry on to the full bigint division */
			moo_pushvolat (moo, &y);
			x = make_bigint_with_ooi(moo, xv);
			moo_popvolat (moo);
			if (!x) return MOO_NULL;
		}
		else if (MOO_OOP_IS_SMOOI(y))
		{
			moo_ooi_t yv;

			if (!is_bigint(moo,x)) goto oops_einval;

			/* divide a big integer by a small integer. */

			yv = MOO_OOP_TO_SMOOI(y);
			switch (yv)
			{
				case 0: /* divide by 0 */
					moo_seterrnum (moo, MOO_EDIVBY0);
					return MOO_NULL;

				case 1: /* divide by 1 */
					z = clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));
					if (!z) return MOO_NULL;
					if (rem) *rem = MOO_SMOOI_TO_OOP(0);
					return z;

				case -1: /* divide by -1 */
					z = clone_bigint_negated(moo, x, MOO_OBJ_GET_SIZE(x));
					if (!z) return MOO_NULL;
					if (rem) *rem = MOO_SMOOI_TO_OOP(0);
					return z;

				default:
				{
					moo_lidw_t dw;
					moo_liw_t carry = 0;
					moo_liw_t* zw;
					moo_oow_t zs, i;
					moo_ooi_t yv_abs, ri;

					yv_abs = (yv < 0)? -yv: yv;
				#if (MOO_LIW_BITS < MOO_OOI_BITS)
					if (yv_abs > MOO_TYPE_MAX(moo_liw_t)) break;
				#endif

					x_neg_sign = (MOO_POINTER_IS_NBIGINT(moo, x));
					y_neg_sign = (yv < 0);

					z = clone_bigint_to_positive(moo, x, MOO_OBJ_GET_SIZE(x));
					if (!z) return MOO_NULL;

					zw = MOO_OBJ_GET_LIWORD_SLOT(z);
					zs = MOO_OBJ_GET_SIZE(z);
					for (i = zs; i > 0; )
					{
						--i;
						dw = ((moo_lidw_t)carry << MOO_LIW_BITS) + zw[i];
						/* TODO: optimize it with ASM - no seperate / and % */
						zw[i] = (moo_liw_t)(dw / yv_abs);
						carry = (moo_liw_t)(dw % yv_abs);
					}
					/*if (zw[zs - 1] == 0) zs--;*/

					MOO_ASSERT (moo, carry <= MOO_SMOOI_MAX);
					ri = carry;
					if (x_neg_sign) ri = -ri;

					z = normalize_bigint(moo, z);
					if (!z) return MOO_NULL;

					if (x_neg_sign != y_neg_sign)
					{
						MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);
						if (ri && modulo)
						{
							z = moo_subints(moo, z, MOO_SMOOI_TO_OOP(1));
							if (!z) return MOO_NULL;
							if (rem)
							{
								moo_pushvolat (moo, &z);
								r = moo_addints(moo, MOO_SMOOI_TO_OOP(ri), MOO_SMOOI_TO_OOP(yv));
								moo_popvolat (moo);
								if (!r) return MOO_NULL;
								*rem = r;
							}
							return z;
						}
					}

					if (rem) *rem = MOO_SMOOI_TO_OOP(ri);
					return z;
				}
			}

			/* carry on to the full bigint division */
			moo_pushvolat (moo, &x);
			y = make_bigint_with_ooi(moo, yv);
			moo_popvolat (moo);
			if (!y) return MOO_NULL;
		}
		else
		{
			if (!is_bigint(moo,x)) goto oops_einval;
			if (!is_bigint(moo,y)) goto oops_einval;
		}
	}

	x_neg_sign = (MOO_POINTER_IS_NBIGINT(moo, x));
	y_neg_sign = (MOO_POINTER_IS_NBIGINT(moo, y));

	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &y);
	z = divide_unsigned_integers(moo, x, y, &r);
	moo_popvolats (moo, 2);
	if (!z) return MOO_NULL;

	if (x_neg_sign) 
	{

		/* the class on r must be set before normalize_bigint() 
		 * because it can get changed to a small integer */
		MOO_OBJ_SET_CLASS(r, moo->_large_negative_integer);
	}

	if (x_neg_sign != y_neg_sign)
	{
		MOO_OBJ_SET_CLASS(z, moo->_large_negative_integer);

		moo_pushvolat (moo, &z);
		moo_pushvolat (moo, &y);
		r = normalize_bigint(moo, r);
		moo_popvolats (moo, 2);
		if (!r) return MOO_NULL;

		if (r != MOO_SMOOI_TO_OOP(0) && modulo)
		{
			if (rem)
			{
				moo_pushvolat (moo, &z);
				moo_pushvolat (moo, &y);
				r = moo_addints(moo, r, y);
				moo_popvolats (moo, 2);
				if (!r) return MOO_NULL;

				moo_pushvolat (moo, &r);
				z = normalize_bigint(moo, z);
				moo_popvolat (moo);
				if (!z) return MOO_NULL;

				moo_pushvolat (moo, &r);
				z = moo_subints(moo, z, MOO_SMOOI_TO_OOP(1));
				moo_popvolat (moo);
				if (!z) return MOO_NULL;

				*rem = r;
				return z;
			}
			else
			{
				/* remainder is not needed at all */
/* TODO: subtract 1 without normalization??? */
				z = normalize_bigint(moo, z);
				if (!z) return MOO_NULL;
				return moo_subints(moo, z, MOO_SMOOI_TO_OOP(1));
			}
		}
	}
	else
	{
		moo_pushvolat (moo, &z);
		r = normalize_bigint(moo, r);
		moo_popvolat (moo);
		if (!r) return MOO_NULL;
	}

	if (rem) *rem = r;
	return normalize_bigint(moo, z);

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_negateint (moo_t* moo, moo_oop_t x)
{
	if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;
		v = MOO_OOP_TO_SMOOI(x);
		return MOO_SMOOI_TO_OOP(-v);
	}
	else
	{
		if (!is_bigint(moo, x)) goto oops_einval;
		return clone_bigint_negated (moo, x, MOO_OBJ_GET_SIZE(x));
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameter - %O", x);
	return MOO_NULL;
}

moo_oop_t moo_bitatint (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	/* y is 0-based */

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v1, v2, v3;

		v1 = MOO_OOP_TO_SMOOI(x);
		v2 = MOO_OOP_TO_SMOOI(y);

		if (v2 < 0) return MOO_SMOOI_TO_OOP(0);
		if (v1 >= 0)
		{
			/* the absolute value may be composed of up to 
			 * MOO_SMOOI_BITS - 1 bits as there is a sign bit.*/
			if (v2 >= MOO_SMOOI_BITS - 1) return MOO_SMOOI_TO_OOP(0);
			v3 = ((moo_oow_t)v1 >> v2) & 1;
		}
		else
		{
			if (v2 >= MOO_SMOOI_BITS - 1) return MOO_SMOOI_TO_OOP(1);
			v3 = ((~(moo_oow_t)-v1 + 1) >> v2) & 1;
		}
		return MOO_SMOOI_TO_OOP(v3);
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		if (!is_bigint(moo, y)) goto oops_einval;

		if (MOO_POINTER_IS_NBIGINT(moo, y)) return MOO_SMOOI_TO_OOP(0);

		/* y is definitely >= MOO_SMOOI_BITS */
		if (MOO_OOP_TO_SMOOI(x) >= 0) 
			return MOO_SMOOI_TO_OOP(0);
		else 
			return MOO_SMOOI_TO_OOP(1);
	}
	else if (MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v;
		moo_oow_t wp, bp, xs;

		if (!is_bigint(moo, x)) goto oops_einval;
		v = MOO_OOP_TO_SMOOI(y);

		if (v < 0) return MOO_SMOOI_TO_OOP(0);
		wp = v / MOO_LIW_BITS;
		bp = v - (wp * MOO_LIW_BITS);

		xs = MOO_OBJ_GET_SIZE(x);
		if (MOO_POINTER_IS_PBIGINT(moo, x))
		{
			if (wp >= xs) return MOO_SMOOI_TO_OOP(0);
			v = (MOO_OBJ_GET_LIWORD_SLOT(x)[wp] >> bp) & 1;
		}
		else
		{
			moo_lidw_t w, carry;
			moo_oow_t i;

			if (wp >= xs) return MOO_SMOOI_TO_OOP(1);

			carry = 1;
			for (i = 0; i <= wp; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
			}
			v = ((moo_oow_t)w >> bp) & 1;
		}

		return MOO_SMOOI_TO_OOP(v);
	}
	else
	{
	#if defined(MOO_LIMIT_OBJ_SIZE)
		/* nothing */
	#else
		moo_oow_t w, wp, bp, xs;
		moo_ooi_t v;
		int sign;
	#endif

		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;

	#if defined(MOO_LIMIT_OBJ_SIZE)
		if (MOO_POINTER_IS_NBIGINT(moo, y)) return MOO_SMOOI_TO_OOP(0);

		MOO_ASSERT (moo, MOO_OBJ_SIZE_BITS_MAX <= MOO_TYPE_MAX(moo_oow_t));
		if (MOO_POINTER_IS_PBIGINT(moo, x))
		{
			return MOO_SMOOI_TO_OOP (0);
		}
		else
		{
			return MOO_SMOOI_TO_OOP (1);
		}
	#else
		xs = MOO_OBJ_GET_SIZE(x);

		if (MOO_POINTER_IS_NBIGINT(moo, y)) return MOO_SMOOI_TO_OOP(0);

		sign = bigint_to_oow(moo, y, &w);
		MOO_ASSERT (moo, sign >= 0);
		if (sign >= 1)
		{
			wp = w / MOO_LIW_BITS;
			bp = w - (wp * MOO_LIW_BITS);
		}
		else
		{
			moo_oop_t quo, rem;

			MOO_ASSERT (moo, sign == 0);

			moo_pushvolat (moo, &x);
			quo = moo_divints (moo, y, MOO_SMOOI_TO_OOP(MOO_LIW_BITS), 0, &rem);
			moo_popvolat (moo);
			if (!quo) return MOO_NULL;

			sign = integer_to_oow (moo, quo, &wp);
			MOO_ASSERT (moo, sign >= 0);
			if (sign == 0)
			{
				/* too large. set it to xs so that it gets out of
				 * the valid range */
				wp = xs;
			}

			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(rem));
			bp = MOO_OOP_TO_SMOOI(rem);
			MOO_ASSERT (moo, bp >= 0 && bp < MOO_LIW_BITS);
		}

		if (MOO_POINTER_IS_PBIGINT(moo, x))
		{
			if (wp >= xs) return MOO_SMOOI_TO_OOP(0);
			v = (MOO_OBJ_GET_LIWORD_SLOT(x)[wp] >> bp) & 1;
		}
		else
		{
			moo_lidw_t w, carry;
			moo_oow_t i;

			if (wp >= xs) return MOO_SMOOI_TO_OOP(1);

			carry = 1;
			for (i = 0; i <= wp; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
			}
			v = ((moo_oow_t)w >> bp) & 1;
		}

		return MOO_SMOOI_TO_OOP(v);
	#endif
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_bitandints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v1, v2, v3;

		v1 = MOO_OOP_TO_SMOOI(x);
		v2 = MOO_OOP_TO_SMOOI(y);
		v3 = v1 & v2;

		if (MOO_IN_SMOOI_RANGE(v3)) return MOO_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (moo, v3);
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, y)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(x);
		if (v == 0) return MOO_SMOOI_TO_OOP(0);

		moo_pushvolat (moo, &y);
		x = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else if (MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, x)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(y);
		if (v == 0) return MOO_SMOOI_TO_OOP(0);

		moo_pushvolat (moo, &x);
		y = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		moo_oop_t z;
		moo_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_bigint(moo,x) || !is_bigint(moo, y)) goto oops_einval;

	bigint_and_bigint:
		xs = MOO_OBJ_GET_SIZE(x);
		ys = MOO_OBJ_GET_SIZE(y);

		if (xs < ys)
		{
			/* make sure that x is greater than or equal to y */
			z = x;
			x = y;
			y = z;
			zs = ys;
			ys = xs;
			xs = zs;
		}

		negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;
		negy = (MOO_POINTER_IS_NBIGINT(moo, y))? 1: 0;

		if (negx && negy)
		{
			zalloc = xs + 1;
			zs = xs;
		}
		else if (negx)
		{
			zalloc = ys;
			zs = ys;
		}
		else if (negy)
		{
			zalloc = xs;
			zs = xs;
		}
		else
		{
			zalloc = ys;
			zs = ys;
		}

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, zalloc);
		moo_popvolats (moo, 2);
		if (!z) return MOO_NULL;

		if (negx && negy)
		{
			/* both are negative */
			moo_lidw_t w[2];
			moo_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;

				w[1] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry[1];
				carry[1] = w[1] >> MOO_LIW_BITS;

				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0] & (moo_liw_t)w[1];
			}
			MOO_ASSERT (moo, carry[1] == 0);

			/* 2's complement on the remaining part of x. the lacking part
			 * in y is treated as if they are all 1s. */
			for (; i < xs; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0];
			}
			MOO_ASSERT (moo, carry[0] == 0);

			/* 2's complement on the final result */
			MOO_OBJ_GET_LIWORD_SLOT(z)[zs] = ~(moo_liw_t)0;
			carry[0] = 1;
			for (i = 0; i <= zs; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0];
			}
			MOO_ASSERT (moo, carry[0] == 0);

			MOO_OBJ_SET_CLASS (z, moo->_large_negative_integer);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			moo_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w & MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}

			/* the lacking part in y is all 0's. the remaining part in x is
			 * just masked out when bitwise-anded with 0. so nothing is done
			 * to handle the remaining part in x */
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			moo_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] & (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);

			/* handle the longer part in x than y
			 * 
			 * For example,
			 *  x => + 1010 1100
			 *  y => -      0011
			 * 
			 * If y is extended to the same length as x, 
			 * it is a negative 0000 0001. 
			 * 2's complement is performed on this imaginary extension.
			 * the result is '1111 1101' (1111 1100 + 1).
			 * 
			 * when y is shorter and negative, the lacking part can be
			 * treated as all 1s in the 2's complement format.
			 * 
			 * the remaining part in x can be just copied to the 
			 * final result 'z'.
			 */
			for (; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i];
			}
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] & MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}
		}

		return normalize_bigint(moo, z);
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_bitorints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v1, v2, v3;

		v1 = MOO_OOP_TO_SMOOI(x);
		v2 = MOO_OOP_TO_SMOOI(y);
		v3 = v1 | v2;

		if (MOO_IN_SMOOI_RANGE(v3)) return MOO_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (moo, v3);
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, y)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(x);
		if (v == 0) return clone_bigint(moo, y, MOO_OBJ_GET_SIZE(y));

		moo_pushvolat (moo, &y);
		x = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else if (MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, x)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(y);
		if (v == 0) return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));

		moo_pushvolat (moo, &x);
		y = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		moo_oop_t z;
		moo_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_bigint(moo,x) || !is_bigint(moo, y)) goto oops_einval;

	bigint_and_bigint:
		xs = MOO_OBJ_GET_SIZE(x);
		ys = MOO_OBJ_GET_SIZE(y);

		if (xs < ys)
		{
			/* make sure that x is greater than or equal to y */
			z = x;
			x = y;
			y = z;
			zs = ys;
			ys = xs;
			xs = zs;
		}

		negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;
		negy = (MOO_POINTER_IS_NBIGINT(moo, y))? 1: 0;

		if (negx && negy)
		{
			zalloc = ys + 1;
			zs = ys;
		}
		else if (negx)
		{
			zalloc = xs + 1;
			zs = xs;
		}
		else if (negy)
		{
			zalloc = ys + 1;
			zs = ys;
		}
		else
		{
			zalloc = xs;
			zs = xs;
		}

		if (zalloc < zs)
		{
			/* overflow in zalloc calculation above */
			moo_seterrnum (moo, MOO_EOOMEM); /* TODO: is it a soft failure or hard failure? */
			return MOO_NULL;
		}

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, zalloc);
		moo_popvolats (moo, 2);
		if (!z) return MOO_NULL;

		if (negx && negy)
		{
			/* both are negative */
			moo_lidw_t w[2];
			moo_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;

				w[1] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry[1];
				carry[1] = w[1] >> MOO_LIW_BITS;

				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0] | (moo_liw_t)w[1];
			}
			MOO_ASSERT (moo, carry[1] == 0);

			/* do nothing about the extra part in x and the lacking part
			 * in y for the reason shown in [NOTE] in the 'else if' block
			 * further down. */

		adjust_to_negative:
			/* 2's complement on the final result */
			MOO_OBJ_GET_LIWORD_SLOT(z)[zs] = ~(moo_liw_t)0;
			carry[0] = 1;
			for (i = 0; i <= zs; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0];
			}
			MOO_ASSERT (moo, carry[0] == 0);

			MOO_OBJ_SET_CLASS (z, moo->_large_negative_integer);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			moo_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w | MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}

			for (; i < xs; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
			}

			MOO_ASSERT (moo, carry == 0);
			goto adjust_to_negative;
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			moo_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] | (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);

			/* [NOTE]
			 *  in theory, the lacking part in ys is all 1s when y is
			 *  extended to the width of x. but those 1s are inverted to
			 *  0s when another 2's complement is performed over the final
			 *  result after the jump to 'adjust_to_negative'.
			 *  setting zs to 'xs + 1' and performing the following loop is 
			 *  redundant.
			for (; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = ~(moo_liw_t)0;
			}
			*/
			goto adjust_to_negative;
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] | MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}

			for (; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i];
			}
		}

		return normalize_bigint(moo, z);
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_bitxorints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v1, v2, v3;

		v1 = MOO_OOP_TO_SMOOI(x);
		v2 = MOO_OOP_TO_SMOOI(y);
		v3 = v1 ^ v2;

		if (MOO_IN_SMOOI_RANGE(v3)) return MOO_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (moo, v3);
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, y)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(x);
		if (v == 0) return clone_bigint(moo, y, MOO_OBJ_GET_SIZE(y));

		moo_pushvolat (moo, &y);
		x = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else if (MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v;

		if (!is_bigint(moo, x)) goto oops_einval;

		v = MOO_OOP_TO_SMOOI(y);
		if (v == 0) return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));

		moo_pushvolat (moo, &x);
		y = make_bigint_with_ooi (moo, v);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		moo_oop_t z;
		moo_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_bigint(moo,x) || !is_bigint(moo, y)) goto oops_einval;

	bigint_and_bigint:
		xs = MOO_OBJ_GET_SIZE(x);
		ys = MOO_OBJ_GET_SIZE(y);

		if (xs < ys)
		{
			/* make sure that x is greater than or equal to y */
			z = x;
			x = y;
			y = z;
			zs = ys;
			ys = xs;
			xs = zs;
		}

		negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;
		negy = (MOO_POINTER_IS_NBIGINT(moo, y))? 1: 0;

		if (negx && negy)
		{
			zalloc = xs;
			zs = xs;
		}
		else if (negx)
		{
			zalloc = xs + 1;
			zs = xs;
		}
		else if (negy)
		{
			zalloc = xs + 1;
			zs = xs;
		}
		else
		{
			zalloc = xs;
			zs = xs;
		}

		if (zalloc < zs)
		{
			/* overflow in zalloc calculation above */
			moo_seterrnum (moo, MOO_EOOMEM); /* TODO: is it a soft failure or hard failure? */
			return MOO_NULL;
		}

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, zalloc);
		moo_popvolats (moo, 2);
		if (!z) return MOO_NULL;

		if (negx && negy)
		{
			/* both are negative */
			moo_lidw_t w[2];
			moo_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;

				w[1] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry[1];
				carry[1] = w[1] >> MOO_LIW_BITS;

				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0] ^ (moo_liw_t)w[1];
			}
			MOO_ASSERT (moo, carry[1] == 0);

			/* treat the lacking part in y as all 1s */
			for (; i < xs; i++)
			{
				w[0] = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry[0];
				carry[0] = w[0] >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w[0] ^ (~(moo_liw_t)0);
			}
			MOO_ASSERT (moo, carry[0] == 0);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			moo_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w ^ MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}

			/* treat the lacking part in y as all 0s */
			for (; i < xs; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);

		adjust_to_negative:
			/* 2's complement on the final result */
			MOO_OBJ_GET_LIWORD_SLOT(z)[zs] = ~(moo_liw_t)0;
			carry = 1;
			for (i = 0; i <= zs; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);

			MOO_OBJ_SET_CLASS (z, moo->_large_negative_integer);
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			moo_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(y)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] ^ (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);

			/* treat the lacking part in y as all 1s */
			for (; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] ^ (~(moo_liw_t)0);
			}

			goto adjust_to_negative;
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i] ^ MOO_OBJ_GET_LIWORD_SLOT(y)[i];
			}

			/* treat the lacking part in y as all 0s */
			for (; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = MOO_OBJ_GET_LIWORD_SLOT(x)[i];
			}
		}

		return normalize_bigint(moo, z);
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_bitinvint (moo_t* moo, moo_oop_t x)
{
	if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;

		v = MOO_OOP_TO_SMOOI(x);
		v = ~v;

		if (MOO_IN_SMOOI_RANGE(v)) return MOO_SMOOI_TO_OOP(v);
		return make_bigint_with_ooi (moo, v);
	}
	else
	{
		moo_oop_t z;
		moo_oow_t i, xs, zs, zalloc;
		int negx;

		if (!is_bigint(moo,x)) goto oops_einval;

		xs = MOO_OBJ_GET_SIZE(x);
		negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;

		if (negx)
		{
			zalloc = xs;
			zs = xs;
		}
		else
		{
			zalloc = xs + 1;
			zs = xs;
		}

		if (zalloc < zs)
		{
			/* overflow in zalloc calculation above */
			moo_seterrnum (moo, MOO_EOOMEM); /* TODO: is it a soft failure or hard failure? */
			return MOO_NULL;
		}

		moo_pushvolat (moo, &x);
		z = moo_instantiate(moo, moo->_large_positive_integer, MOO_NULL, zalloc);
		moo_popvolat (moo);
		if (!z) return MOO_NULL;

		if (negx)
		{
			moo_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < xs; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = ~(moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);
		}
		else
		{
			moo_lidw_t w, carry;

		#if 0
			for (i = 0; i < xs; i++)
			{
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = ~MOO_OBJ_GET_LIWORD_SLOT(x)[i];
			}

			MOO_OBJ_GET_LIWORD_SLOT(z)[zs] = ~(moo_liw_t)0;
			carry = 1;
			for (i = 0; i <= zs; i++)
			{
				w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
			}
			MOO_ASSERT (moo, carry == 0);
		#else
			carry = 1;
			for (i = 0; i < xs; i++)
			{
				w = (moo_lidw_t)(MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
				carry = w >> MOO_LIW_BITS;
				MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
			}
			MOO_ASSERT (moo, i == zs);
			MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)carry;
			MOO_ASSERT (moo, (carry >> MOO_LIW_BITS) == 0);
		#endif

			MOO_OBJ_SET_CLASS (z, moo->_large_negative_integer);
		}

		return normalize_bigint(moo, z);
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameter - %O", x);
	return MOO_NULL;
}

static MOO_INLINE moo_oop_t rshift_negative_bigint (moo_t* moo, moo_oop_t x, moo_oow_t shift)
{
	moo_oop_t z;
	moo_lidw_t w;
	moo_lidw_t carry;
	moo_oow_t i, xs;

	MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, x));
	xs = MOO_OBJ_GET_SIZE(x);

	moo_pushvolat (moo, &x);
	/* +1 for the second inversion below */
	z = moo_instantiate(moo, moo->_large_negative_integer, MOO_NULL, xs + 1);
	moo_popvolat (moo);
	if (!z) return MOO_NULL;

	/* the following lines roughly for 'z = moo_bitinv (moo, x)' */
	carry = 1;
	for (i = 0; i < xs; i++)
	{
		w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(x)[i]) + carry;
		carry = w >> MOO_LIW_BITS;
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = ~(moo_liw_t)w;
	}
	MOO_ASSERT (moo, carry == 0);

	/* shift to the right */
	rshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), xs, shift);

	/* the following lines roughly for 'z = moo_bitinv (moo, z)' */
#if 0
	for (i = 0; i < xs; i++)
	{
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = ~MOO_OBJ_GET_LIWORD_SLOT(z)[i];
	}
	MOO_OBJ_GET_LIWORD_SLOT(z)[xs] = ~(moo_liw_t)0;

	carry = 1;
	for (i = 0; i <= xs; i++)
	{
		w = (moo_lidw_t)((moo_liw_t)~MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry;
		carry = w >> MOO_LIW_BITS;
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
	}
	MOO_ASSERT (moo, carry == 0);
#else
	carry = 1;
	for (i = 0; i < xs; i++)
	{
		w = (moo_lidw_t)(MOO_OBJ_GET_LIWORD_SLOT(z)[i]) + carry;
		carry = w >> MOO_LIW_BITS;
		MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)w;
	}
	MOO_OBJ_GET_LIWORD_SLOT(z)[i] = (moo_liw_t)carry;
	MOO_ASSERT (moo, (carry >> MOO_LIW_BITS) == 0);
#endif

	return z; /* z is not normalized */
}

#if defined(MOO_LIMIT_OBJ_SIZE)
	/* nothing */
#else

static MOO_INLINE moo_oop_t rshift_negative_bigint_and_normalize (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;
	moo_oow_t shift;
	int sign;

	MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, x));
	MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, y));

	/* for convenience in subtraction below. 
	 * it could be MOO_TYPE_MAX(moo_oow_t) 
	 * if make_bigint_with_intmax() or something
	 * similar were used instead of MOO_SMOOI_TO_OOP().*/
	shift = MOO_SMOOI_MAX;
	do
	{
		moo_pushvolat (moo, &y);
		z = rshift_negative_bigint (moo, x, shift);
		moo_popvolat (moo);
		if (!z) return MOO_NULL;

		/* y is a negative number. use moo_addints() until it becomes 0 */
		moo_pushvolat (moo, &z);
		y = moo_addints (moo, y, MOO_SMOOI_TO_OOP(shift));
		moo_popvolat (moo);
		if (!y) return MOO_NULL;

		sign = integer_to_oow (moo, y, &shift);
		if (sign == 0) shift = MOO_SMOOI_MAX;
		else 
		{
			if (shift == 0)
			{
				/* no more shift */
				return normalize_bigint (moo, z);
			}
			MOO_ASSERT (moo, sign <= -1);
		}

		moo_pushvolat (moo, &y);
		x = normalize_bigint (moo, z);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		if (MOO_OOP_IS_SMOOI(x))
		{
			/* for normaization above, x can become a small integer */
			moo_ooi_t v;

			v = MOO_OOP_TO_SMOOI(x);
			MOO_ASSERT (moo, v < 0);

			/* normal right shift of a small negative integer */
			if (shift >= MOO_OOI_BITS - 1) 
			{
				/* when y is still a large integer, this condition is met
				 * met as MOO_SMOOI_MAX > MOO_OOI_BITS. so i can simly 
				 * terminate the loop after this */
				return MOO_SMOOI_TO_OOP(-1);
			}
			else 
			{
				v = (moo_ooi_t)(((moo_oow_t)v >> shift) | MOO_HBMASK(moo_oow_t, shift));
				if (MOO_IN_SMOOI_RANGE(v)) 
					return MOO_SMOOI_TO_OOP(v);
				else 
					return make_bigint_with_ooi (moo, v);
			}
		}
	}
	while (1);

	/* this part must not be reached */
	MOO_ASSERT (moo, !"internal error - must not happen");
	moo_seterrnum (moo, MOO_EINTERN);
	return MOO_NULL;
}

static MOO_INLINE moo_oop_t rshift_positive_bigint_and_normalize (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;
	moo_oow_t zs, shift;
	int sign;

	MOO_ASSERT (moo, MOO_POINTER_IS_PBIGINT(moo, x));
	MOO_ASSERT (moo, MOO_POINTER_IS_NBIGINT(moo, y));

	zs = MOO_OBJ_GET_SIZE(x);

	moo_pushvolat (moo, &y);
	z = clone_bigint(moo, x, zs);
	moo_popvolat (moo);
	if (!z) return MOO_NULL;

	/* for convenience in subtraction below. 
	 * it could be MOO_TYPE_MAX(moo_oow_t) 
	 * if make_bigint_with_intmax() or something
	 * similar were used instead of MOO_SMOOI_TO_OOP().*/
	shift = MOO_SMOOI_MAX;
	do
	{
		rshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), zs, shift);
		if (count_effective(MOO_OBJ_GET_LIWORD_SLOT(z), zs) == 1 &&
		    MOO_OBJ_GET_LIWORD_SLOT(z)[0] == 0) 
		{
			/* if z is 0, i don't have to go on */
			break;
		}

		/* y is a negative number. use moo_addints() until it becomes 0 */
		moo_pushvolat (moo, &z);
		y = moo_addints(moo, y, MOO_SMOOI_TO_OOP(shift));
		moo_popvolat (moo);
		if (!y) return MOO_NULL;

		sign = integer_to_oow(moo, y, &shift);
		if (sign == 0) shift = MOO_SMOOI_MAX;
		else 
		{
			if (shift == 0) break;
			MOO_ASSERT (moo, sign <= -1);
		}
	}
	while (1);

	return normalize_bigint(moo, z);
}

static MOO_INLINE moo_oop_t lshift_bigint_and_normalize (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	moo_oop_t z;
	moo_oow_t wshift, shift;
	int sign;

	MOO_ASSERT (moo, MOO_POINTER_IS_PBIGINT(moo, y));

	/* this loop is very inefficient as shifting is repeated
	 * with lshift_unsigned_array(). however, this part of the
	 * code is not likey to be useful because the amount of
	 * memory available is certainly not enough to support 
	 * huge shifts greater than MOO_TYPE_MAX(moo_oow_t) */
	shift = MOO_SMOOI_MAX;
	do
	{
		/* for convenience only in subtraction below. 
		 * should it be between MOO_SMOOI_MAX and MOO_TYPE_MAX(moo_oow_t),
		 * the second parameter to moo_subints() can't be composed
		 * using MOO_SMOOI_TO_OOP() */
		wshift = shift / MOO_LIW_BITS;
		if (shift > wshift * MOO_LIW_BITS) wshift++;

		moo_pushvolat (moo, &y);
		z = expand_bigint(moo, x, wshift);
		moo_popvolat (moo);
		if (!z) return MOO_NULL;

		lshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), MOO_OBJ_GET_SIZE(z), shift);

		moo_pushvolat (moo, &y);
		x = normalize_bigint(moo, z);
		moo_popvolat (moo);
		if (!x) return MOO_NULL;

		moo_pushvolat (moo, &x);
		y = moo_subints(moo, y, MOO_SMOOI_TO_OOP(shift));
		moo_popvolat (moo);
		if (!y) return MOO_NULL;

		sign = integer_to_oow(moo, y, &shift);
		if (sign == 0) shift = MOO_SMOOI_MAX;
		else
		{
			if (shift == 0) 
			{
				MOO_ASSERT (moo, is_normalized_integer (moo, x));
				return x;
			}
			MOO_ASSERT (moo, sign >= 1);
		}
	}
	while (1);

	/* this part must not be reached */
	MOO_ASSERT (moo, !"internal error - must not happen");
	moo_seterrnum (moo, MOO_EINTERN);
	return MOO_NULL;
}

#endif

moo_oop_t moo_bitshiftint (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	/* left shift if y is positive,
	 * right shift if y is negative */

	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		moo_ooi_t v1, v2;

		v1 = MOO_OOP_TO_SMOOI(x);
		v2 = MOO_OOP_TO_SMOOI(y);
		if (v1 == 0 || v2 == 0) 
		{
			/* return without cloning as x is a small integer */
			return x; 
		}

		if (v2 > 0)
		{
			/* left shift */
			moo_oop_t z;
			moo_oow_t wshift;

			wshift = v2 / MOO_LIW_BITS;
			if (v2 > wshift * MOO_LIW_BITS) wshift++;

			z = make_bloated_bigint_with_ooi(moo, v1, wshift);
			if (!z) return MOO_NULL;

			lshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), MOO_OBJ_GET_SIZE(z), v2);
			return normalize_bigint(moo, z);
		}
		else
		{
			/* right shift */
			moo_ooi_t v;

			v2 = -v2;
			if (v1 < 0)
			{
				/* guarantee arithmetic shift preserving the sign bit
				 * regardless of compiler implementation.
				 *
				 *    binary    decimal   shifted by
				 *   -------------------------------
				 *   10000011    (-125)    0
				 *   11000001    (-63)     1
				 *   11100000    (-32)     2
				 *   11110000    (-16)     3
				 *   11111000    (-8)      4
				 *   11111100    (-4)      5
				 *   11111110    (-2)      6
				 *   11111111    (-1)      7
				 *   11111111    (-1)      8
				 */
				
				if (v2 >= MOO_OOI_BITS - 1) v = -1;
				else 
				{
					/* MOO_HBMASK_SAFE(moo_oow_t, v2 + 1) could also be
					 * used as a mask. but the sign bit is shifted in. 
					 * so, masking up to 'v2' bits is sufficient */
					v = (moo_ooi_t)(((moo_oow_t)v1 >> v2) | MOO_HBMASK(moo_oow_t, v2));
				}
			}
			else
			{
				if (v2 >= MOO_OOI_BITS) v = 0;
				else v = v1 >> v2;
			}
			if (MOO_IN_SMOOI_RANGE(v)) return MOO_SMOOI_TO_OOP(v);
			return make_bigint_with_ooi(moo, v);
		}
	}
	else
	{
		int sign, negx, negy;
		moo_oow_t shift;

		if (MOO_OOP_IS_SMOOI(x))
		{
			moo_ooi_t v;

			if (!is_bigint(moo,y)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(x);
			if (v == 0) return MOO_SMOOI_TO_OOP(0);

			if (MOO_POINTER_IS_NBIGINT(moo, y))
			{
				/* right shift - special case.
				 * x is a small integer. it is just a few bytes long.
				 * y is a large negative integer. its smallest absolute value
				 * is MOO_SMOOI_MAX. i know the final answer. */
				return (v < 0)? MOO_SMOOI_TO_OOP(-1): MOO_SMOOI_TO_OOP(0);
			}

			moo_pushvolat (moo, &y);
			x = make_bigint_with_ooi (moo, v);
			moo_popvolat (moo);
			if (!x) return MOO_NULL;

			goto bigint_and_bigint;
		}
		else if (MOO_OOP_IS_SMOOI(y))
		{
			moo_ooi_t v;

			if (!is_bigint(moo,x)) goto oops_einval;

			v = MOO_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));

			negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;
			if (v > 0)
			{
				sign = 1;
				negy = 0;
				shift = v;
				goto bigint_and_positive_oow;
			}
			else 
			{
				sign = -1;
				negy = 1;
				shift = -v;
				goto bigint_and_negative_oow;
			}
		}
		else
		{
			moo_oop_t z;

			if (!is_bigint(moo,x) || !is_bigint(moo, y)) goto oops_einval;

		bigint_and_bigint:
			negx = (MOO_POINTER_IS_NBIGINT(moo, x))? 1: 0;
			negy = (MOO_POINTER_IS_NBIGINT(moo, y))? 1: 0;

			sign = bigint_to_oow(moo, y, &shift);
			if (sign == 0)
			{
				/* y is too big or too small */
				if (negy)
				{
					/* right shift */
				#if defined(MOO_LIMIT_OBJ_SIZE)
					/* the maximum number of bit shifts are guaranteed to be
					 * small enough to fit into the moo_oow_t type. so i can 
					 * easily assume that all bits are shifted out */
					MOO_ASSERT (moo, MOO_OBJ_SIZE_BITS_MAX <= MOO_TYPE_MAX(moo_oow_t));
					return (negx)? MOO_SMOOI_TO_OOP(-1): MOO_SMOOI_TO_OOP(0);
				#else
					if (negx)
						return rshift_negative_bigint_and_normalize(moo, x, y);
					else
						return rshift_positive_bigint_and_normalize(moo, x, y);
				#endif
				}
				else
				{
					/* left shift */
				#if defined(MOO_LIMIT_OBJ_SIZE)
					/* the maximum number of bit shifts are guaranteed to be
					 * small enough to fit into the moo_oow_t type. so i can 
					 * simply return a failure here becuase it's surely too 
					 * large after shifting */
					MOO_ASSERT (moo, MOO_TYPE_MAX(moo_oow_t) >= MOO_OBJ_SIZE_BITS_MAX);
					moo_seterrnum (moo, MOO_EOOMEM); /* is it a soft failure or a hard failure? is this error code proper? */
					return MOO_NULL;
				#else
					return lshift_bigint_and_normalize(moo, x, y);
				#endif
				}
			}
			else if (sign >= 1)
			{
				/* left shift */
				moo_oow_t wshift;

			bigint_and_positive_oow:
				wshift = shift / MOO_LIW_BITS;
				if (shift > wshift * MOO_LIW_BITS) wshift++;

				z = expand_bigint(moo, x, wshift);
				if (!z) return MOO_NULL;

				lshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), MOO_OBJ_GET_SIZE(z), shift);
			}
			else 
			{
				/* right shift */
			bigint_and_negative_oow:

				MOO_ASSERT (moo, sign <= -1);

				if (negx)
				{
					z = rshift_negative_bigint(moo, x, shift);
					if (!z) return MOO_NULL;
				}
				else
				{
					z = clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x));
					if (!z) return MOO_NULL;
					rshift_unsigned_array (MOO_OBJ_GET_LIWORD_SLOT(z), MOO_OBJ_GET_SIZE(z), shift);
				}
			}

			return normalize_bigint(moo, z);
		}
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "invalid parameters - %O, %O", x, y);
	return MOO_NULL;
}

static moo_uint8_t ooch_val_tab[] =
{
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
	99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 99, 99, 99, 99, 99,
	99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 99, 99, 99, 99, 99
};

moo_oop_t moo_strtoint (moo_t* moo, const moo_ooch_t* str, moo_oow_t len, int radix)
{
	int sign = 1;
	const moo_ooch_t* ptr, * start, * end;
	moo_lidw_t w, v;
	moo_liw_t hw[16], * hwp = MOO_NULL;
	moo_oow_t hwlen, outlen;
	moo_oop_t res;

	if (radix < 0) 
	{
		/* when radix is less than 0, it treats it as if '-' is preceeding */
		sign = -1;
		radix = -radix;
	}

	MOO_ASSERT (moo, radix >= 2 && radix <= 36);

	ptr = str;
	end = str + len;

	if (ptr < end)
	{
		if (*ptr == '+') ptr++;
		else if (*ptr == '-') 
		{
			ptr++; 
			sign = -1;
		}
	}

	if (ptr >= end) goto oops_einval; /* no digits */

	while (ptr < end && *ptr == '0') 
	{
		/* skip leading zeros */
		ptr++;
	}
	if (ptr >= end)
	{
		/* all zeros */
		return MOO_SMOOI_TO_OOP(0);
	}

	hwlen = 0;
	start = ptr; /* this is the real start */

	if (IS_POW2(radix))
	{
		unsigned int exp;
		unsigned int bitcnt;

		/* get log2(radix) in a fast way under the fact that
		 * radix is a power of 2. the exponent acquired is
		 * the number of bits that a digit of the given radix takes up */
		/*exp = LOG2_FOR_POW2(radix);*/
		exp = _exp_tab[radix - 1];

		/* bytes */
		outlen = ((moo_oow_t)(end - str) * exp + 7) / 8; 
		/* number of moo_liw_t */
		outlen = (outlen + MOO_SIZEOF(hw[0]) - 1) / MOO_SIZEOF(hw[0]);

		if (outlen > MOO_COUNTOF(hw)) 
		{
/* TODO: reuse this buffer? */
			hwp = moo_allocmem(moo, outlen * MOO_SIZEOF(hw[0]));
			if (!hwp) return MOO_NULL;
		}
		else
		{
			hwp = hw;
		}

		w = 0;
		bitcnt = 0;
		ptr = end - 1;

		while (ptr >= start)
		{
			if (*ptr < 0 || *ptr >= MOO_COUNTOF(ooch_val_tab)) goto oops_einval;
			v = ooch_val_tab[*ptr];
			if (v >= radix) goto oops_einval;

			w |= (v << bitcnt);
			bitcnt += exp;
			if (bitcnt >= MOO_LIW_BITS)
			{
				bitcnt -= MOO_LIW_BITS;
				hwp[hwlen++] = w; /*(moo_liw_t)(w & MOO_LBMASK(moo_lidw_t, MOO_LIW_BITS));*/
				w >>= MOO_LIW_BITS;
			}

			ptr--;
		}

		MOO_ASSERT (moo, w <= MOO_TYPE_MAX(moo_liw_t));
		if (hwlen == 0 || w > 0) hwp[hwlen++] = w;
	}
	else
	{
		moo_lidw_t r1, r2;
		moo_liw_t multiplier;
		int dg, i, safe_ndigits;

		w = 0;
		ptr = start;

		safe_ndigits = moo->bigint[radix].safe_ndigits;
		multiplier = (moo_liw_t)moo->bigint[radix].multiplier;

		outlen = (end - str) / safe_ndigits + 1;
		if (outlen > MOO_COUNTOF(hw)) 
		{
			hwp = moo_allocmem(moo, outlen * MOO_SIZEOF(moo_liw_t));
			if (!hwp) return MOO_NULL;
		}
		else
		{
			hwp = hw;
		}

		MOO_ASSERT (moo, ptr < end);
		do
		{
			r1 = 0;
			for (dg = 0; dg < safe_ndigits; dg++)
			{
				if (ptr >= end) 
				{
					multiplier = 1;
					for (i = 0; i < dg; i++) multiplier *= radix;
					break;
				}

				if (*ptr < 0 || *ptr >= MOO_COUNTOF(ooch_val_tab)) goto oops_einval;
				v = ooch_val_tab[*ptr];
				if (v >= radix) goto oops_einval;

				r1 = r1 * radix + (moo_liw_t)v;
				ptr++;
			}

			r2 = r1;
			for (i = 0; i < hwlen; i++)
			{
				moo_liw_t high, low;

				v = (moo_lidw_t)hwp[i] * multiplier;
				high = (moo_liw_t)(v >> MOO_LIW_BITS);
				low = (moo_liw_t)(v /*& MOO_LBMASK(moo_oow_t, MOO_LIW_BITS)*/);

			#if defined(liw_add_overflow)
				/* use liw_add_overflow() only if it's compiler-builtin. */
				r2 = high + liw_add_overflow(low, r2, &low);
			#else
				/* don't use the fall-back version of liw_add_overflow() */
				low += r2;
				r2 = (moo_lidw_t)high + (low < r2);
			#endif

				hwp[i] = low;
			}
			if (r2) hwp[hwlen++] = (moo_liw_t)r2;
		}
		while (ptr < end);
	}

	MOO_ASSERT (moo, hwlen >= 1);

#if (MOO_LIW_BITS == MOO_OOW_BITS)
	if (hwlen == 1) 
	{
		w = hwp[0];
		MOO_ASSERT (moo, -MOO_SMOOI_MAX == MOO_SMOOI_MIN);
		if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP((moo_ooi_t)w * sign);
	}
#elif (MOO_LIW_BITS == MOO_OOHW_BITS)
	if (hwlen == 1) 
	{
		MOO_ASSERT (moo, hwp[0] <= MOO_SMOOI_MAX);
		return MOO_SMOOI_TO_OOP((moo_ooi_t)hwp[0] * sign);
	}
	else if (hwlen == 2)
	{
		w = MAKE_WORD(hwp[0], hwp[1]);
		MOO_ASSERT (moo, -MOO_SMOOI_MAX == MOO_SMOOI_MIN);
		if (w <= MOO_SMOOI_MAX) return MOO_SMOOI_TO_OOP((moo_ooi_t)w * sign);
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

	res = moo_instantiate(moo, (sign < 0? moo->_large_negative_integer: moo->_large_positive_integer), hwp, hwlen);
	if (hwp && hw != hwp) moo_freemem (moo, hwp);

	return res;

oops_einval:
	if (hwp && hw != hwp) moo_freemem (moo, hwp);
	moo_seterrbfmt (moo, MOO_EINVAL, "unable to convert to integer %.*js", len, str);
	return MOO_NULL;
}

static moo_oow_t oow_to_text (moo_t* moo, moo_oow_t w, int flagged_radix, moo_ooch_t* buf)
{
	moo_ooch_t* ptr;
	const char* _digitc;
	int radix;

	radix = flagged_radix & MOO_INTTOSTR_RADIXMASK;
	_digitc =  _digitc_array[!!(flagged_radix & MOO_INTTOSTR_LOWERCASE)];
	MOO_ASSERT (moo, radix >= 2 && radix <= 36);

	ptr = buf;
	do
	{
		*ptr++ = _digitc[w % radix];
		w /= radix;
	}
	while (w > 0);

	return ptr - buf;
}

static void reverse_string (moo_ooch_t* str, moo_oow_t len)
{
	moo_ooch_t ch;
	moo_ooch_t* start = str;
	moo_ooch_t* end = str + len - 1;

	while (start < end)
	{
		ch = *start;
		*start++ = *end;
		*end-- = ch;
	}
}

moo_oop_t moo_eqints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) == MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x) || MOO_OOP_IS_SMOOI(y)) 
	{
		return moo->_false;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return is_equal(moo, x, y)? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_neints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) != MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x) || MOO_OOP_IS_SMOOI(y)) 
	{
		return moo->_true;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return !is_equal(moo, x, y)? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_gtints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) > MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		if (!is_bigint(moo, y)) goto oops_einval;
		return (MOO_POINTER_IS_NBIGINT(moo, y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(y)) 
	{
		if (!is_bigint(moo, x)) goto oops_einval;
		return (MOO_POINTER_IS_PBIGINT(moo, x))? moo->_true: moo->_false;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return is_greater(moo, x, y)? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_geints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) >= MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		if (!is_bigint(moo, y)) goto oops_einval;
		return (MOO_POINTER_IS_NBIGINT(moo, y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(y)) 
	{
		if (!is_bigint(moo, x)) goto oops_einval;
		return (MOO_POINTER_IS_PBIGINT(moo, x))? moo->_true: moo->_false;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return (is_greater(moo, x, y) || is_equal(moo, x, y))? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_ltints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) < MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		if (!is_bigint(moo, y)) goto oops_einval;
		return (MOO_POINTER_IS_PBIGINT(moo, y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(y)) 
	{
		if (!is_bigint(moo, x)) goto oops_einval;
		return (MOO_POINTER_IS_NBIGINT(moo, x))? moo->_true: moo->_false;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return is_less(moo, x, y)? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_leints (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (MOO_OOP_IS_SMOOI(x) && MOO_OOP_IS_SMOOI(y))
	{
		return (MOO_OOP_TO_SMOOI(x) <= MOO_OOP_TO_SMOOI(y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(x))
	{
		if (!is_bigint(moo, y)) goto oops_einval;
		return (MOO_POINTER_IS_PBIGINT(moo, y))? moo->_true: moo->_false;
	}
	else if (MOO_OOP_IS_SMOOI(y)) 
	{
		if (!is_bigint(moo, x)) goto oops_einval;
		return (MOO_POINTER_IS_NBIGINT(moo, x))? moo->_true: moo->_false;
	}
	else 
	{
		if (!is_bigint(moo, x) || !is_bigint(moo, y)) goto oops_einval;
		return (is_less(moo, x, y) || is_equal(moo, x, y))? moo->_true: moo->_false;
	}

oops_einval:
	moo_seterrbfmt (moo, MOO_EINVAL, "parameters not integer - %O, %O", x, y);
	return MOO_NULL;
}

moo_oop_t moo_sqrtint (moo_t* moo, moo_oop_t x)
{
	/* TODO: find a faster and more efficient algorithm??? */
	moo_oop_t a, b, m, m2, t;
	int neg;

	if (!moo_isint(moo, x)) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not integer - %O", x);
		return MOO_NULL;
	}

	a = moo->_nil;
	b = moo->_nil;
	m = moo->_nil;
	m2 = moo->_nil;

	moo_pushvolat (moo, &x);
	moo_pushvolat (moo, &a);
	moo_pushvolat (moo, &b);
	moo_pushvolat (moo, &m);
	moo_pushvolat (moo, &m2);

	a = moo_ltints(moo, x, MOO_SMOOI_TO_OOP(0));
	if (!a) goto oops;
	if (a == moo->_true)
	{
		/* the given number is a negative number.
		 * i will arrange the return value to be negative. */
		x = moo_negateint(moo, x);
		if (!x) goto oops;
		neg = 1;
	}
	else neg = 0;

	a = MOO_SMOOI_TO_OOP(1);
	b = moo_bitshiftint(moo, x, MOO_SMOOI_TO_OOP(-5));
	if (!b) goto oops;
	b = moo_addints(moo, b, MOO_SMOOI_TO_OOP(8));
	if (!b) goto oops;

	while (1)
	{
		t = moo_geints(moo, b, a);
		if (!t) return MOO_NULL;
		if (t == moo->_false) break;

		m = moo_addints(moo, a, b);
		if (!m) goto oops;
		m = moo_bitshiftint(moo, m, MOO_SMOOI_TO_OOP(-1));
		if (!m) goto oops;
		m2 = moo_mulints(moo, m, m);
		if (!m2) goto oops;
		t = moo_gtints(moo, m2, x);
		if (!t) return MOO_NULL;
		if (t == moo->_true)
		{
			b = moo_subints(moo, m, MOO_SMOOI_TO_OOP(1));
			if (!b) goto oops;
		}
		else
		{
			a = moo_addints(moo, m, MOO_SMOOI_TO_OOP(1));
			if (!a) goto oops;
		}
	}

	moo_popvolats (moo, 5);
	x = moo_subints(moo, a, MOO_SMOOI_TO_OOP(1));
	if (!x) return MOO_NULL;

	if (neg) x = moo_negateint(moo, x);
	return x;

oops:
	moo_popvolats (moo, 5);
	return MOO_NULL;
}

moo_oop_t moo_absint (moo_t* moo, moo_oop_t x)
{
	if (MOO_OOP_IS_SMOOI(x))
	{
		moo_ooi_t v;
		v = MOO_OOP_TO_SMOOI(x);
		if (v < 0) 
		{
			v = -v;
			x = MOO_SMOOI_TO_OOP(v);
		}
	}
	else if (MOO_POINTER_IS_NBIGINT(moo, x))
	{
		x = _clone_bigint(moo, x, MOO_OBJ_GET_SIZE(x), moo->_large_positive_integer);
	}
	else if (MOO_POINTER_IS_PBIGINT(moo, x))
	{
		/* do nothing. return x without change.
		 * [THINK] but do i need to clone a positive bigint? */
	}
	else
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not integer - %O", x);
		return MOO_NULL;
	}

	return x;
}

static MOO_INLINE moo_liw_t get_last_digit (moo_t* moo, moo_liw_t* x, moo_oow_t* xs, int radix)
{
	/* this function changes the contents of the large integer word array */
	moo_oow_t oxs = *xs;
	moo_liw_t carry = 0;
	moo_oow_t i;
	moo_lidw_t dw;

	MOO_ASSERT (moo, oxs > 0);

	for (i = oxs; i > 0; )
	{
		--i;
		dw = ((moo_lidw_t)carry << MOO_LIW_BITS) + x[i];
		/* TODO: optimize it with ASM - no seperate / and % */
		x[i] = (moo_liw_t)(dw / radix);
		carry = (moo_liw_t)(dw % radix);
	}
	if (/*oxs > 0 &&*/ x[oxs - 1] == 0) *xs = oxs - 1;
	return carry;
}

moo_oop_t moo_inttostr (moo_t* moo, moo_oop_t num, int flagged_radix)
{
	moo_ooi_t v = 0;
	moo_oow_t w;
	moo_oow_t as;
	moo_liw_t* t = MOO_NULL;
	moo_ooch_t* xbuf = MOO_NULL;
	moo_oow_t xlen = 0, reqcapa;

	int radix;
	const char* _digitc;

	radix = flagged_radix & MOO_INTTOSTR_RADIXMASK;
	_digitc =  _digitc_array[!!(flagged_radix & MOO_INTTOSTR_LOWERCASE)];
	MOO_ASSERT (moo, radix >= 2 && radix <= 36);

	if (!moo_isint(moo,num)) goto oops_einval;
	v = integer_to_oow(moo, num, &w);

	if (v)
	{
		/* The largest buffer is required for radix 2.
		 * For a binary conversion(radix 2), the number of bits is
		 * the maximum number of digits that can be produced. +1 is
		 * needed for the sign. */

		reqcapa = MOO_OOW_BITS + 1;
		if (moo->inttostr.xbuf.capa < reqcapa)
		{
			xbuf = (moo_ooch_t*)moo_reallocmem(moo, moo->inttostr.xbuf.ptr, reqcapa * MOO_SIZEOF(*xbuf));
			if (!xbuf) return MOO_NULL;
			moo->inttostr.xbuf.capa = reqcapa;
			moo->inttostr.xbuf.ptr = xbuf;
		}
		else
		{
			xbuf = moo->inttostr.xbuf.ptr;
		}

		xlen = oow_to_text(moo, w, flagged_radix, xbuf);
		if (v < 0) xbuf[xlen++] = '-';

		reverse_string (xbuf, xlen);
		if (flagged_radix & MOO_INTTOSTR_NONEWOBJ)
		{
			/* special case. don't create a new object.
			 * the caller can use the data left in moo->inttostr.xbuf */
			moo->inttostr.xbuf.len = xlen;
			return moo->_nil;
		}
		return moo_makestring(moo, xbuf, xlen);
	}

	as = MOO_OBJ_GET_SIZE(num);

	reqcapa = as * MOO_LIW_BITS + 1; 
	if (moo->inttostr.xbuf.capa < reqcapa)
	{
		xbuf = (moo_ooch_t*)moo_reallocmem(moo, moo->inttostr.xbuf.ptr, reqcapa * MOO_SIZEOF(*xbuf));
		if (!xbuf) return MOO_NULL;
		moo->inttostr.xbuf.capa = reqcapa;
		moo->inttostr.xbuf.ptr = xbuf;
	}
	else
	{
		xbuf = moo->inttostr.xbuf.ptr;
	}

	if (moo->inttostr.t.capa < as)
	{
		t = (moo_liw_t*)moo_reallocmem(moo, moo->inttostr.t.ptr, reqcapa * MOO_SIZEOF(*t));
		if (!t) return MOO_NULL;
		moo->inttostr.t.capa = as;
		moo->inttostr.t.ptr = t;
	}
	else 
	{
		t = moo->inttostr.t.ptr;
	}

	MOO_MEMCPY (t, MOO_OBJ_GET_LIWORD_SLOT(num), MOO_SIZEOF(*t) * as);

	do
	{
		moo_liw_t dv = get_last_digit(moo, t, &as, radix);
		xbuf[xlen++] = _digitc[dv];
	}
	while (as > 0);

	if (MOO_POINTER_IS_NBIGINT(moo, num)) xbuf[xlen++] = '-';
	reverse_string (xbuf, xlen);
	if (flagged_radix & MOO_INTTOSTR_NONEWOBJ)
	{
		/* special case. don't create a new object.
		 * the caller can use the data left in moo->inttostr.xbuf */
		moo->inttostr.xbuf.len = xlen;
		return moo->_nil;
	}

	return moo_makestring(moo, xbuf, xlen);

oops_einval:
	moo_seterrnum (moo, MOO_EINVAL);
	return MOO_NULL;
}
