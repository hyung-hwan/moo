/*
 * $Id$
 *
    Copyright (c) 2014-2015 Chung, Hyung-Hwan. All rights reserved.

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

#include "stix-prv.h"


#if (STIX_LIW_BITS == STIX_OOW_BITS)
	/* nothing special */
#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
#	define MAKE_WORD(hw1,hw2) ((stix_oow_t)(hw1) | (stix_oow_t)(hw2) << STIX_LIW_BITS)
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

/*#define IS_POWER_OF_2(ui) (((ui) > 0) && (((ui) & (~(ui)+ 1)) == (ui)))*/
#define IS_POWER_OF_2(ui) (((ui) > 0) && ((ui) & ((ui) - 1)) == 0)

#if (STIX_SIZEOF_OOW_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UADD_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uadd_overflow(a,b,c)
#elif (STIX_SIZEOF_OOW_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UADDL_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (STIX_SIZEOF_OOW_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UADDLL_OVERFLOW)
#	define oow_add_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static STIX_INLINE int oow_add_overflow (stix_oow_t a, stix_oow_t b, stix_oow_t* c)
{
	*c = a + b;
	return b > STIX_TYPE_MAX(stix_oow_t) - a;
}
#endif


#if (STIX_SIZEOF_OOW_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UMUL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umul_overflow(a,b,c)
#elif (STIX_SIZEOF_OOW_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UMULL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umull_overflow(a,b,c)
#elif (STIX_SIZEOF_OOW_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UMULLL_OVERFLOW)
#	define oow_mul_overflow(a,b,c)  __builtin_umulll_overflow(a,b,c)
#else
static STIX_INLINE int oow_mul_overflow (stix_oow_t a, stix_oow_t b, stix_oow_t* c)
{
#if (STIX_SIZEOF_UINTMAX_T > STIX_SIZEOF_OOW_T)
	stix_uintmax_t k;
	k = (stix_uintmax_t)a * (stix_uintmax_t)b;
	*c = (stix_oow_t)k;
	return (k >> STIX_OOW_BITS) > 0;
	/*return k > STIX_TYPE_MAX(stix_oow_t);*/
#else
	*c = a * b;
	return b != 0 && a > STIX_TYPE_MAX(stix_oow_t) / b; /* works for unsigned types only */
#endif
}
#endif

#if (STIX_SIZEOF_OOI_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_SMUL_OVERFLOW)
#	define smooi_mul_overflow(a,b,c)  __builtin_smul_overflow(a,b,c)
#elif (STIX_SIZEOF_OOI_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_SMULL_OVERFLOW)
#	define smooi_mul_overflow(a,b,c)  __builtin_smull_overflow(a,b,c)
#elif (STIX_SIZEOF_OOI_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_SMULLL_OVERFLOW)
#	define smooi_mul_overflow(a,b,c)  __builtin_smulll_overflow(a,b,c)
#else
static STIX_INLINE int smooi_mul_overflow (stix_ooi_t a, stix_ooi_t b, stix_ooi_t* c)
{
	/* take note that this function is not supposed to handle
	 * the whole stix_ooi_t range. it handles the smooi subrange */

#if (STIX_SIZEOF_UINTMAX_T > STIX_SIZEOF_OOI_T)
	stix_intmax_t k;

	STIX_ASSERT (STIX_IN_SMOOI_RANGE(a));
	STIX_ASSERT (STIX_IN_SMOOI_RANGE(b));

	k = (stix_intmax_t)a * (stix_intmax_t)b;
	*c = (stix_ooi_t)k;

	return k > STIX_TYPE_MAX(stix_ooi_t) || k < STIX_TYPE_MIN(stix_ooi_t);
#else

	stix_ooi_t ua, ub;

	STIX_ASSERT (STIX_IN_SMOOI_RANGE(a));
	STIX_ASSERT (STIX_IN_SMOOI_RANGE(b));

	*c = a * b;

	ub = (b >= 0)? b: -b;
	ua = (a >= 0)? a: -a;

	/* though this fomula basically works for unsigned types in principle, 
	 * the values used here are all absolute values and they fall in
	 * a safe range to apply this fomula. the safe range is guarantee because
	 * the sources are supposed to be smoois. */
	return ub != 0 && ua > STIX_TYPE_MAX(stix_ooi_t) / ub; 
#endif
}
#endif

#if (STIX_SIZEOF_LIW_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UADD_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uadd_overflow(a,b,c)
#elif (STIX_SIZEOF_LIW_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UADDL_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (STIX_SIZEOF_LIW_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UADDLL_OVERFLOW)
#	define liw_add_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static STIX_INLINE int liw_add_overflow (stix_liw_t a, stix_liw_t b, stix_liw_t* c)
{
	*c = a + b;
	return b > STIX_TYPE_MAX(stix_liw_t) - a;
}
#endif

#if (STIX_SIZEOF_LIW_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UMUL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_umul_overflow(a,b,c)
#elif (STIX_SIZEOF_LIW_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UMULL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (STIX_SIZEOF_LIW_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UMULLL_OVERFLOW)
#	define liw_mul_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static STIX_INLINE int liw_mul_overflow (stix_liw_t a, stix_liw_t b, stix_liw_t* c)
{
#if (STIX_SIZEOF_UINTMAX_T > STIX_SIZEOF_LIW_T)
	stix_uintmax_t k;
	k = (stix_uintmax_t)a * (stix_uintmax_t)b;
	*c = (stix_liw_t)k;
	return (k >> STIX_LIW_BITS) > 0;
	/*return k > STIX_TYPE_MAX(stix_liw_t);*/
#else
	*c = a * b;
	return b != 0 && a > STIX_TYPE_MAX(stix_liw_t) / b; /* works for unsigned types only */
#endif
}
#endif

static STIX_INLINE int is_integer (stix_t* stix, stix_oop_t oop)
{
	stix_oop_t c;

	c = STIX_CLASSOF(stix,oop);

/* TODO: is it better to introduce a special integer mark into the class itself */
/* TODO: or should it check if it's a subclass, subsubclass, subsubsubclass, etc of a large_integer as well? */
	return c == stix->_small_integer ||
	       c == stix->_large_positive_integer ||
	       c == stix->_large_negative_integer;
}

static STIX_INLINE stix_oop_t make_bigint_with_ooi (stix_t* stix, stix_ooi_t i)
{
#if (STIX_LIW_BITS == STIX_OOW_BITS)
	stix_oow_t w;

	STIX_ASSERT (STIX_SIZEOF(stix_oow_t) == STIX_SIZEOF(stix_liw_t));
	if (i >= 0)
	{
		w = i;
		return stix_instantiate (stix, stix->_large_positive_integer, &w, 1);
	}
	else
	{
		/* The caller must ensure that i is greater than the smallest value
		 * that stix_ooi_t can represent. otherwise, the absolute value 
		 * cannot be held in stix_ooi_t. */
		STIX_ASSERT (i > STIX_TYPE_MIN(stix_ooi_t));
		w = -i;
		return stix_instantiate (stix, stix->_large_negative_integer, &w, 1);
	}
#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
	stix_liw_t hw[2];
	stix_oow_t w;

	if (i >= 0)
	{
		w = i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,STIX_LIW_BITS);
		hw[1] = w >> STIX_LIW_BITS;
		return stix_instantiate (stix, stix->_large_positive_integer, &hw, (hw[1] > 0? 2: 1));
	}
	else
	{
		STIX_ASSERT (i > STIX_TYPE_MIN(stix_ooi_t));
		w = -i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,STIX_LIW_BITS);
		hw[1] = w >> STIX_LIW_BITS;
		return stix_instantiate (stix, stix->_large_negative_integer, &hw, (hw[1] > 0? 2: 1));
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
}

static STIX_INLINE stix_oop_t make_bigint_with_intmax (stix_t* stix, stix_intmax_t v)
{
	stix_oow_t len;
	stix_liw_t buf[STIX_SIZEOF_INTMAX_T / STIX_SIZEOF_LIW_T];
	stix_uintmax_t ui;

	/* this is not a generic function. it can't handle v 
	 * if it's STIX_TYPE_MIN(stix_intmax_t) */
	STIX_ASSERT (v > STIX_TYPE_MIN(stix_intmax_t));

	ui = (v >= 0)? v: -v;
	len = 0;
	do
	{
		buf[len++] = (stix_liw_t)ui;
		ui = ui >> STIX_LIW_BITS;
	}
	while (ui > 0);

{ int i;
printf ("MKBI-INTMAX=>");
for (i = len; i > 0;)
{

printf ("%0*lX ", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)buf[--i]);
}
printf ("\n");
}

	return stix_instantiate (stix, ((v >= 0)? stix->_large_positive_integer: stix->_large_negative_integer), buf, len);
}

static STIX_INLINE stix_oop_t _clone_bigint (stix_t* stix, stix_oop_t oop, stix_oow_t count, stix_oop_t _class)
{
	stix_oop_t z;
	stix_oow_t i;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	if (count <= 0) count = STIX_OBJ_GET_SIZE(oop);

	stix_pushtmp (stix, &oop);
	z = stix_instantiate (stix, _class, STIX_NULL, count);
	stix_poptmp (stix);
	if (!z) return STIX_NULL;

	for (i = 0; i < count; i++)
	{
		((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)oop)->slot[i];
	}
	return z;
}

static STIX_INLINE stix_oop_t clone_bigint (stix_t* stix, stix_oop_t oop, stix_oow_t count)
{
	return _clone_bigint (stix, oop, count, STIX_OBJ_GET_CLASS(oop));
}

static STIX_INLINE stix_oop_t clone_bigint_negated (stix_t* stix, stix_oop_t oop, stix_oow_t count)
{
	stix_oop_t c;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
	{
		c = stix->_large_negative_integer;
	}
	else
	{
		STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
		c = stix->_large_positive_integer;
	}

	return _clone_bigint (stix, oop, count, c);
}

static STIX_INLINE stix_oop_t clone_bigint_to_positive (stix_t* stix, stix_oop_t oop, stix_oow_t count)
{
	return _clone_bigint (stix, oop, count, stix->_large_positive_integer);
}

static STIX_INLINE stix_oow_t count_effective (stix_liw_t* x, stix_oow_t xs)
{
	stix_oow_t i;

	for (i = xs; i > 1; )
	{
		--i;
		if (x[i] != 0) return i + 1;
	}

	return 1;
}

static STIX_INLINE stix_oow_t count_effective_digits (stix_oop_t oop)
{
	stix_oow_t i;

	for (i = STIX_OBJ_GET_SIZE(oop); i > 1; )
	{
		--i;
		if (((stix_oop_liword_t)oop)->slot[i] != 0) return i + 1;
	}

	return 1;
}

static stix_oop_t normalize_bigint (stix_t* stix, stix_oop_t oop)
{
	stix_oow_t count;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	count = count_effective_digits (oop);

#if (STIX_LIW_BITS == STIX_OOW_BITS)
	if (count == 1) /* 1 word */
	{
		stix_oow_t w;

		w = ((stix_oop_liword_t)oop)->slot[0];
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(w);
		}
		else
		{
			STIX_ASSERT (-STIX_SMOOI_MAX  == STIX_SMOOI_MIN);
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(-(stix_ooi_t)w);
		}
	}
#elif (STIX_LIW_BITS == STIX_OOHW_BITS)

	if (count == 1) /* 1 half-word */
	{
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			return STIX_SMOOI_TO_OOP(((stix_oop_liword_t)oop)->slot[0]);
		}
		else
		{
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			return STIX_SMOOI_TO_OOP(-(stix_ooi_t)((stix_oop_liword_t)oop)->slot[0]);
		}
	}
	else if (count == 2) /* 2 half-words */
	{
		stix_oow_t w;

		w = MAKE_WORD (((stix_oop_liword_t)oop)->slot[0], ((stix_oop_liword_t)oop)->slot[1]);
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(w);
		}
		else
		{
			STIX_ASSERT (-STIX_SMOOI_MAX  == STIX_SMOOI_MIN);
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(-(stix_ooi_t)w);
		}
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
	if (STIX_OBJ_GET_SIZE(oop) == count)
	{
		/* no compaction is needed. return it as it is */
		return oop;
	}

	return clone_bigint (stix, oop, count);
}


static STIX_INLINE int is_less_unsigned_array (const stix_liw_t* x, stix_oow_t xs, const stix_liw_t* y, stix_oow_t ys)
{
	stix_oow_t i;

	if (xs != ys) return xs < ys;
	for (i = xs; i > 0; )
	{
		--i;
		if (x[i] != y[i]) return x[i] < y[i];
	}

	return 0;
}

static STIX_INLINE int is_less_unsigned (stix_oop_t x, stix_oop_t y)
{
	return is_less_unsigned_array (
		((stix_oop_liword_t)x)->slot, STIX_OBJ_GET_SIZE(x), 
		((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(y));
}

static STIX_INLINE int is_less (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
	{
		return STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer;
	}

	return is_less_unsigned (x, y);
}

static STIX_INLINE int is_equal (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	/* check if two large integers are equal to each other */
	return STIX_OBJ_GET_CLASS(x) == STIX_OBJ_GET_CLASS(y) && STIX_OBJ_GET_SIZE(x) == STIX_OBJ_GET_SIZE(y) &&
	       STIX_MEMCMP(((stix_oop_liword_t)x)->slot,  ((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(x) * STIX_SIZEOF(stix_liw_t)) == 0;
}

static void complement2_unsigned_array (const stix_liw_t* x, stix_oow_t xs, stix_liw_t* z)
{
	stix_oow_t i;
	stix_lidw_t w;
	stix_lidw_t carry;

	/* get 2's complement (~x + 1) */

	carry = 1; 
	for (i = 0; i < xs; i++)
	{
		w = (stix_lidw_t)(~x[i]) + carry;
		/*w = (stix_lidw_t)(x[i] ^ STIX_TYPE_MAX(stix_liw_t)) + carry;*/
		carry = w >> STIX_LIW_BITS;
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
	STIX_ASSERT (carry == 0);
}

static STIX_INLINE stix_oow_t add_unsigned_array (const stix_liw_t* x, stix_oow_t xs, const stix_liw_t* y, stix_oow_t ys, stix_liw_t* z)
{
	stix_oow_t i;
	stix_lidw_t w;
	stix_lidw_t carry = 0;

	STIX_ASSERT (xs >= ys);

	for (i = 0; i < ys; i++)
	{
		w = (stix_lidw_t)x[i] + (stix_lidw_t)y[i] + carry;
		carry = w >> STIX_LIW_BITS;
		z[i] = w /*& STIX_LBMASK(stix_lidw_t, STIX_LIW_BITS) */;
	}

	for (; i < xs; i++)
	{
		w = (stix_lidw_t)x[i] + carry;
		carry = w >> STIX_LIW_BITS;
		z[i] = w /*& STIX_LBMASK(stix_lidw_t, STIX_LIW_BITS)*/;
	}

	if (i > 1 && carry == 0) return i - 1;
	z[i] = carry;

	return i;
}

static STIX_INLINE stix_oow_t subtract_unsigned_array (const stix_liw_t* x, stix_oow_t xs, const stix_liw_t* y, stix_oow_t ys, stix_liw_t* z)
{
	stix_oow_t i;
	stix_lidw_t w;
	stix_lidw_t borrow = 0;
	stix_lidw_t borrowed_word;

	STIX_ASSERT (!is_less_unsigned_array(x, xs, y, ys));

	borrowed_word = (stix_lidw_t)1 << STIX_LIW_BITS;
	for (i = 0; i < ys; i++)
	{
		w = (stix_lidw_t)y[i] + borrow;
		if ((stix_lidw_t)x[i] >= w)
		{
			z[i] = x[i] - w;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (stix_lidw_t)x[i]) - w; 
			borrow = 1;
		}
	}

	for (; i < xs; i++)
	{
		if (x[i] >= borrow) 
		{
			z[i] = x[i] - (stix_liw_t)borrow;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (stix_lidw_t)x[i]) - borrow;
			borrow = 1;
		}
	}

	STIX_ASSERT (borrow == 0);
	return i;
}

static STIX_INLINE void multiply_unsigned_array (const stix_liw_t* x, stix_oow_t xs, const stix_liw_t* y, stix_oow_t ys, stix_liw_t* z)
{
	stix_lidw_t v;
	stix_oow_t pa;

/* TODO: implement Karatsuba  or Toom-Cook 3-way algorithm when the input length is long */

	pa = (xs < ys)? xs: ys;
	if (pa <= ((stix_oow_t)1 << (STIX_LIDW_BITS - (STIX_LIW_BITS * 2))))
	{
		/* Comba(column-array) multiplication */

		/* when the input length is too long, v may overflow. if it
		 * happens, comba's method doesn't work as carry propagation is
		 * affected badly. so we need to use this method only if
		 * the input is short enough. */

		stix_oow_t pa, ix, iy, iz, tx, ty;

		pa = xs + ys;
		v = 0;
		for (ix = 0; ix < pa; ix++)
		{
			ty = (ix < ys - 1)? ix: (ys - 1);
			tx = ix - ty;
			iy = (ty + 1 < xs - tx)? (ty + 1): (xs - tx);

			for (iz = 0; iz < iy; iz++)
			{
				v = v + (stix_lidw_t)x[tx + iz] * (stix_lidw_t)y[ty - iz];
			}

			z[ix] = (stix_liw_t)v;
			v = v >> STIX_LIW_BITS;
		}
	}
	else
	{
	#if 1
		stix_oow_t i, j;
		stix_liw_t carry;

		for (i = 0; i < ys; i++)
		{
			if (y[i] == 0)
			{
				z[xs + i] = 0;
			}
			else
			{
				carry = 0;
				for (j = 0; j < xs; j++)
				{
					v = (stix_lidw_t)x[j] * (stix_lidw_t)y[i] + (stix_lidw_t)carry + (stix_lidw_t)z[j + i];
					z[j + i] = (stix_liw_t)v;
					carry = (stix_liw_t)(v >> STIX_LIW_BITS);
				}

				z[xs + i] = carry;
			}
		}

	#else
		stix_oow_t i, j, idx;
		stix_liw_t carry;

		for (i = 0; i < ys; i++)
		{
			idx = i;

			for (j = 0; j < xs; j++)
			{
				v = (stix_lidw_t)x[j] * (stix_lidw_t)y[i] + (stix_lidw_t)carry + (stix_lidw_t)z[idx];
				z[idx] = (stix_liw_t)v;
				carry = (stix_liw_t)(v >> STIX_LIW_BITS);
				idx++;
			}

			while (carry > 0)
			{
				v = (stix_lidw_t)z[idx] + (stix_lidw_t)carry;
				z[idx] = (stix_liw_t)v;
				carry = (stix_liw_t)(v >> STIX_LIW_BITS);
				idx++;
			}

		}
	#endif
	}
}

static STIX_INLINE void lshift_unsigned_array (stix_liw_t* x, stix_oow_t xs, stix_oow_t bits)
{
	/* this function doesn't grow/shrink the array. Shifting is performed
	 * over the given array */

	stix_oow_t word_shifts, bit_shifts, bit_shifts_right;
	stix_oow_t si, di;

	/* get how many words to shift */
	word_shifts = bits / STIX_LIW_BITS;
	if (word_shifts >= xs)
	{
		STIX_MEMSET (x, 0, xs * STIX_SIZEOF(stix_liw_t));
		return;
	}

	/* get how many remaining bits to shift */
	bit_shifts = bits % STIX_LIW_BITS;
	bit_shifts_right = STIX_LIW_BITS - bit_shifts;

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
		STIX_MEMSET (x, 0, word_shifts * STIX_SIZEOF(stix_liw_t));
}

static STIX_INLINE void rshift_unsigned_array (stix_liw_t* x, stix_oow_t xs, stix_oow_t bits)
{
	/* this function doesn't grow/shrink the array. Shifting is performed
	 * over the given array */

	stix_oow_t word_shifts, bit_shifts, bit_shifts_left;
	stix_oow_t si, di, bound;

	/* get how many words to shift */
	word_shifts = bits / STIX_LIW_BITS;
	if (word_shifts >= xs)
	{
		STIX_MEMSET (x, 0, xs * STIX_SIZEOF(stix_liw_t));
		return;
	}

	/* get how many remaining bits to shift */
	bit_shifts = bits % STIX_LIW_BITS;
	bit_shifts_left = STIX_LIW_BITS - bit_shifts;

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
		STIX_MEMSET (&x[xs - word_shifts], 0, word_shifts * STIX_SIZEOF(stix_liw_t));
}

static void divide_unsigned_array (
	const stix_liw_t* x, stix_oow_t xs, 
	const stix_liw_t* y, stix_oow_t ys, 
	stix_liw_t* q, stix_liw_t* r)
{
/* TODO: this function needs to be rewritten for performance improvement. */

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

	stix_oow_t rs, i , j;

	STIX_ASSERT (xs >= ys);
	STIX_MEMSET (q, 0, STIX_SIZEOF(*q) * xs);
	STIX_MEMSET (r, 0, STIX_SIZEOF(*q) * xs);

	/*rs = 1; */
	for (i = xs; i > 0; )
	{
		--i;
		for (j = STIX_SIZEOF(stix_liw_t) * 8; j > 0;)
		{
			--j;

			lshift_unsigned_array (r, xs, 1);
			STIX_SETBITS (stix_liw_t, r[0], 0, 1, STIX_GETBITS(stix_liw_t, x[i], j, 1));

			rs = count_effective (r, xs);
			if (!is_less_unsigned_array (r, rs, y, ys))
			{
				subtract_unsigned_array (r, rs, y, ys, r);
				STIX_SETBITS (stix_liw_t, q[i], j, 1, 1);
			}
		}
		/*if (xs > rs && r[rs] > 0) rs++;*/
	}
}

static stix_oop_t add_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oow_t as, bs, zs;
	stix_oop_t z;

	as = STIX_OBJ_GET_SIZE(x);
	bs = STIX_OBJ_GET_SIZE(y);
	zs = (as >= bs? as: bs) + 1;

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = stix_instantiate (stix, STIX_OBJ_GET_CLASS(x), STIX_NULL, zs);
	stix_poptmps (stix, 2);
	if (!z) return STIX_NULL;

	if (as >= bs)
	{
		add_unsigned_array (
			((stix_oop_liword_t)x)->slot, as,
			((stix_oop_liword_t)y)->slot, bs,
			((stix_oop_liword_t)z)->slot
		);
	}
	else 
	{
		add_unsigned_array (
			((stix_oop_liword_t)y)->slot, bs,
			((stix_oop_liword_t)x)->slot, as,
			((stix_oop_liword_t)z)->slot
		);
	}

	
	return z;
}

static stix_oop_t subtract_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	STIX_ASSERT (!is_less_unsigned(x, y));

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, STIX_OBJ_GET_SIZE(x));
	stix_poptmps (stix, 2);
	if (!z) return STIX_NULL;

	subtract_unsigned_array (
		((stix_oop_liword_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((stix_oop_liword_t)z)->slot);
	return z;
}

static stix_oop_t multiply_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, STIX_OBJ_GET_SIZE(x) + STIX_OBJ_GET_SIZE(y));
	stix_poptmps (stix, 2);
	if (!z) return STIX_NULL;

	multiply_unsigned_array (
		((stix_oop_liword_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((stix_oop_liword_t)z)->slot);
	return z;
}

static stix_oop_t divide_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y, stix_oop_t* r)
{
	stix_oop_t qq, rr;

	/* the caller must ensure that x >= y */
	STIX_ASSERT (!is_less_unsigned (x, y)); 
	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	qq = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, STIX_OBJ_GET_SIZE(x));
	if (!qq) 
	{
		stix_poptmps (stix, 2);
		return STIX_NULL;
	}

	stix_pushtmp (stix, &qq);
	rr = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, STIX_OBJ_GET_SIZE(x));
	stix_poptmps (stix, 3);
	if (!rr) return STIX_NULL;

	divide_unsigned_array (
		((stix_oop_liword_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((stix_oop_liword_t)qq)->slot, ((stix_oop_liword_t)rr)->slot);

	*r = rr;
	return qq;
}

stix_oop_t stix_addints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t i;

		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		STIX_ASSERT (STIX_SMOOI_MAX + STIX_SMOOI_MAX < STIX_TYPE_MAX(stix_ooi_t));
		STIX_ASSERT (STIX_SMOOI_MIN + STIX_SMOOI_MIN > STIX_TYPE_MIN(stix_ooi_t));

		i = STIX_OOP_TO_SMOOI(x) + STIX_OOP_TO_SMOOI(y);
		if (STIX_IN_SMOOI_RANGE(i)) return STIX_SMOOI_TO_OOP(i);

		return make_bigint_with_ooi (stix, i);
	}
	else
	{
		stix_ooi_t v;

		if (STIX_OOP_IS_SMOOI(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(x);
			if (v == 0) return clone_bigint (stix, y, STIX_OBJ_GET_SIZE(y));

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMOOI(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));

			stix_pushtmp (stix, &x);
			y = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!y) return STIX_NULL;
		}
		else
		{
			if (!is_integer(stix,x)) goto oops_einval;
			if (!is_integer(stix,y)) goto oops_einval;
		}

		if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
		{
			if (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)
			{
				/* x is negative, y is positive */
				if (is_less_unsigned (x, y))
				{
					z = subtract_unsigned_integers (stix, y, x);
					if (!z) return STIX_NULL;
				}
				else
				{
					z = subtract_unsigned_integers (stix, x, y);
					if (!z) return STIX_NULL;
					STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
				}
			}
			else
			{
				/* x is positive, y is negative */
				if (is_less_unsigned (x, y))
				{
					z = subtract_unsigned_integers (stix, y, x);
					if (!z) return STIX_NULL;
					STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
				}
				else
				{
					z = subtract_unsigned_integers (stix, x, y);
					if (!z) return STIX_NULL;
				}
			}
		}
		else
		{
			int neg;
			/* both are positive or negative */
			neg = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer); 
			z = add_unsigned_integers (stix, x, y);
			if (!z) return STIX_NULL;
			if (neg) STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
		}
	}

	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_subints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t i;

		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		STIX_ASSERT (STIX_SMOOI_MAX - STIX_SMOOI_MIN < STIX_TYPE_MAX(stix_ooi_t));
		STIX_ASSERT (STIX_SMOOI_MIN - STIX_SMOOI_MAX > STIX_TYPE_MIN(stix_ooi_t));

		i = STIX_OOP_TO_SMOOI(x) - STIX_OOP_TO_SMOOI(y);
		if (STIX_IN_SMOOI_RANGE(i)) return STIX_SMOOI_TO_OOP(i);

		return make_bigint_with_ooi (stix, i);
	}
	else
	{
		stix_ooi_t v;
		int neg;

		if (STIX_OOP_IS_SMOOI(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(x);
			if (v == 0) 
			{
				/* switch the sign to the opposite and return it */
				return clone_bigint_negated (stix, y, STIX_OBJ_GET_SIZE(y));
			}

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMOOI(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));

			stix_pushtmp (stix, &x);
			y = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!y) return STIX_NULL;
		}
		else
		{
			if (!is_integer(stix,x)) goto oops_einval;
			if (!is_integer(stix,y)) goto oops_einval;
		}

		if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
		{
			neg = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer); 
			z = add_unsigned_integers (stix, x, y);
			if (!z) return STIX_NULL;
			if (neg) STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
		}
		else
		{
			/* both are positive or negative */
			if (is_less_unsigned (x, y))
			{
				neg = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer);
				z = subtract_unsigned_integers (stix, y, x);
				if (!z) return STIX_NULL;
				if (!neg) STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
			}
			else
			{
				neg = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer); 
				z = subtract_unsigned_integers (stix, x, y); /* take x's sign */
				if (!z) return STIX_NULL;
				if (neg) STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
			}
		}
	}

	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_mulints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
	#if STIX_SIZEOF_INTMAX_T > STIX_SIZEOF_OOI_T
		stix_intmax_t i;
		i = (stix_intmax_t)STIX_OOP_TO_SMOOI(x) * (stix_intmax_t)STIX_OOP_TO_SMOOI(y);
		if (STIX_IN_SMOOI_RANGE(i)) return STIX_SMOOI_TO_OOP((stix_ooi_t)i);
		return make_bigint_with_intmax (stix, i);
	#else
		stix_ooi_t i;
		stix_ooi_t xv, yv;

		xv = STIX_OOP_TO_SMOOI(x);
		yv = STIX_OOP_TO_SMOOI(y);
		if (smooi_mul_overflow (xv, yv, &i))
		{
			/* overflowed - convert x and y normal objects and carry on */

			/* no need to call stix_pushtmp before creating x because
			 * xv and yv contains actual values needed */
			x = make_bigint_with_ooi (stix, xv);
			if (!x) return STIX_NULL;

			stix_pushtmp (stix, &x); /* protect x made above */
			y = make_bigint_with_ooi (stix, yv);
			stix_poptmp (stix);
			if (!y) return STIX_NULL;

			goto normal;
		}
		else
		{
			if (STIX_IN_SMOOI_RANGE(i)) return STIX_SMOOI_TO_OOP(i);
			return make_bigint_with_ooi (stix, i);
		}
	#endif
	}
	else
	{
		stix_ooi_t v;

		if (STIX_OOP_IS_SMOOI(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(x);
			switch (v)
			{
				case 0:
					return STIX_SMOOI_TO_OOP(0);
				case 1:
					return clone_bigint (stix, y, STIX_OBJ_GET_SIZE(y));
				case -1:
					return clone_bigint_negated (stix, y, STIX_OBJ_GET_SIZE(y));
			}

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMOOI(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(y);
			switch (v)
			{
				case 0:
					return STIX_SMOOI_TO_OOP(0);
				case 1:
					return clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));
				case -1:
					return clone_bigint_negated (stix, x, STIX_OBJ_GET_SIZE(x));
			}

			stix_pushtmp (stix, &x);
			y = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!y) return STIX_NULL;
		}
		else
		{
			if (!is_integer(stix,x)) goto oops_einval;
			if (!is_integer(stix,y)) goto oops_einval;
		}

	normal:
		z = multiply_unsigned_integers (stix, x, y);
		if (!z) return STIX_NULL;
		if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
			STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
	}

	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_divints (stix_t* stix, stix_oop_t x, stix_oop_t y, int modulo, stix_oop_t* rem)
{
	stix_oop_t z, r;
	int x_neg, y_neg;

	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t xv, yv, q, r;

		xv = STIX_OOP_TO_SMOOI(x);
		yv = STIX_OOP_TO_SMOOI(y);

		if (yv == 0)
		{
			stix->errnum = STIX_EDIVBY0;
			return STIX_NULL;
		}

		if (xv == 0)
		{
			if (rem) *rem = STIX_SMOOI_TO_OOP(0);
			return STIX_SMOOI_TO_OOP(0);
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
		STIX_ASSERT (STIX_IN_SMOOI_RANGE(q));

		r = xv - yv * q; /* xv % yv; */
		if (r)
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
				if ((yv ^ r) < 0)
				{
					/* if the divisor has a different sign from r,
					 * change the sign of r to the divisor's sign */
					r += yv;
					--q;
					STIX_ASSERT (r && ((yv ^ r) >= 0));
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
				if (xv && ((xv ^ r) < 0)) 
				{
					/* if the dividend has a different sign from r,
					 * change the sign of r to the dividend's sign.
					 * all the compilers i've worked with produced
					 * the quotient and the remainder that don't need
					 * any adjustment. however, there may be an esoteric
					 * architecture. */
					r -= yv;
					++q;
					STIX_ASSERT (xv && ((xv ^ r) >= 0));
				}
			}
		}

		if (rem)
		{
			STIX_ASSERT (STIX_IN_SMOOI_RANGE(r));
			*rem = STIX_SMOOI_TO_OOP(r);
		}

		return STIX_SMOOI_TO_OOP((stix_ooi_t)q);
	}
	else 
	{
		if (STIX_OOP_IS_SMOOI(x))
		{
			stix_ooi_t v;

			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(x);
			if (v == 0)
			{
				if (rem) *rem = STIX_SMOOI_TO_OOP(0);
				return STIX_SMOOI_TO_OOP(0);
			}

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMOOI(y))
		{
			stix_ooi_t v;

			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(y);
			switch (v)
			{
				case 0:
					stix->errnum = STIX_EDIVBY0;
					return STIX_NULL;

				case 1:
					z = clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));
					if (!z) return STIX_NULL;
					if (rem) *rem = STIX_SMOOI_TO_OOP(0);
					return z;


				case -1:
					z = clone_bigint_negated (stix, x, STIX_OBJ_GET_SIZE(x));
					if (!z) return STIX_NULL;
					if (rem) *rem = STIX_SMOOI_TO_OOP(0);
					return z;

	/*
				default:
					if (IS_POWER_OF_2(v))
					{
	TODO:
	DO SHIFTING. how to get remainder..
	if v is powerof2, do shifting???

						z = clone_bigint_negated (stix, x, STIX_OBJ_GET_SIZE(x));
						rshift_unsigned_array (z, STIX_OBJ_GET_SIZE(z), 10);
					}
	*/
			}

			stix_pushtmp (stix, &x);
			y = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);
			if (!y) return STIX_NULL;
		}
		else
		{
			if (!is_integer(stix,x)) goto oops_einval;
			if (!is_integer(stix,y)) goto oops_einval;
		}
	}

	x_neg = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer);
	y_neg = (STIX_OBJ_GET_CLASS(y) == stix->_large_negative_integer);

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = divide_unsigned_integers (stix, x, y, &r);
	stix_poptmps (stix, 2);
	if (!z) return STIX_NULL;

	if (x_neg) 
	{
		/* the class on r must be set before normalize_bigint() 
		 * because it can get changed to a small integer */
		STIX_OBJ_SET_CLASS(r, stix->_large_negative_integer);
	}

	if (x_neg != y_neg)
	{
		STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);

		stix_pushtmp (stix, &z);
		stix_pushtmp (stix, &y);
		r = normalize_bigint (stix, r);
		stix_poptmps (stix, 2);
		if (!r) return STIX_NULL;

		if (r != STIX_SMOOI_TO_OOP(0) && modulo)
		{
			if (rem)
			{
				stix_pushtmp (stix, &z);
				stix_pushtmp (stix, &y);
				r = stix_addints (stix, r, y);
				stix_poptmps (stix, 2);
				if (!r) return STIX_NULL;

				stix_pushtmp (stix, &r);
				z = normalize_bigint (stix, z);
				stix_poptmp (stix);
				if (!z) return STIX_NULL;

				stix_pushtmp (stix, &r);
				z = stix_subints (stix, z, STIX_SMOOI_TO_OOP(1));
				stix_poptmp (stix);
				if (!z) return STIX_NULL;

				*rem = r;
				return z;
			}
			else
			{
				/* remainder is not needed at all */
/* TODO: subtract 1 without normalization??? */
				z = normalize_bigint (stix, z);
				if (!z) return STIX_NULL;
				return stix_subints (stix, z, STIX_SMOOI_TO_OOP(1));
			}
		}
	}
	else
	{
		stix_pushtmp (stix, &z);
		r = normalize_bigint (stix, r);
		stix_poptmp (stix);
		if (!r) return STIX_NULL;
	}

	if (rem) *rem = r;
	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_bitandints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v1, v2, v3;

		v1 = STIX_OOP_TO_SMOOI(x);
		v2 = STIX_OOP_TO_SMOOI(y);
		v3 = v1 & v2;

		if (STIX_IN_SMOOI_RANGE(v3)) return STIX_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (stix, v3);
	}
	else if (STIX_OOP_IS_SMOOI(x))
	{
		stix_ooi_t v;

		if (!is_integer(stix, y)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(x);
		if (v == 0) return STIX_SMOOI_TO_OOP(0);

		stix_pushtmp (stix, &y);
		x = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else if (STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v;

		if (!is_integer(stix, x)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(y);
		if (v == 0) return STIX_SMOOI_TO_OOP(0);

		stix_pushtmp (stix, &x);
		y = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		stix_oop_t z;
		stix_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_integer(stix,x) || !is_integer(stix, y)) goto oops_einval;

	bigint_and_bigint:
		xs = STIX_OBJ_GET_SIZE(x);
		ys = STIX_OBJ_GET_SIZE(y);

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

		negx = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)? 1: 0;
		negy = (STIX_OBJ_GET_CLASS(y) == stix->_large_negative_integer)? 1: 0;

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

		stix_pushtmp (stix, &x);
		stix_pushtmp (stix, &y);
		z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, zalloc);
		stix_poptmps (stix, 2);
		if (!z) return STIX_NULL;

		if (negx && negy)
		{
			/* both are negative */
			stix_lidw_t w[2];
			stix_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;

				w[1] = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry[1];
				carry[1] = w[1] >> STIX_LIW_BITS;

				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0] & (stix_liw_t)w[1];
			}
			STIX_ASSERT (carry[1] == 0);

			/* 2's complement on the remaining part of x. the lacking part
			 * in y is treated as if they are all 1s. */
			for (; i < xs; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0];
			}
			STIX_ASSERT (carry[0] == 0);

			/* 2's complement on the final result */
			((stix_oop_liword_t)z)->slot[zs] = STIX_TYPE_MAX(stix_liw_t);
			carry[0] = 1;
			for (i = 0; i <= zs; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)z)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0];
			}
			STIX_ASSERT (carry[0] == 0);

			STIX_OBJ_SET_CLASS (z, stix->_large_negative_integer);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			stix_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w & ((stix_oop_liword_t)y)->slot[i];
			}

			/* the lacking part in y is all 0's. the remaining part in x is
			 * just masked out when bitwise-anded with 0. so nothing is done
			 * to handle the remaining part in x */
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			stix_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] & (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

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
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i];
			}
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] & ((stix_oop_liword_t)y)->slot[i];
			}
		}

		return normalize_bigint(stix, z);
	}

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_bitorints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v1, v2, v3;

		v1 = STIX_OOP_TO_SMOOI(x);
		v2 = STIX_OOP_TO_SMOOI(y);
		v3 = v1 | v2;

		if (STIX_IN_SMOOI_RANGE(v3)) return STIX_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (stix, v3);
	}
	else if (STIX_OOP_IS_SMOOI(x))
	{
		stix_ooi_t v;

		if (!is_integer(stix, y)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(x);
		if (v == 0) return clone_bigint(stix, y, STIX_OBJ_GET_SIZE(y));

		stix_pushtmp (stix, &y);
		x = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else if (STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v;

		if (!is_integer(stix, x)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(y);
		if (v == 0) return clone_bigint(stix, x, STIX_OBJ_GET_SIZE(x));

		stix_pushtmp (stix, &x);
		y = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		stix_oop_t z;
		stix_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_integer(stix,x) || !is_integer(stix, y)) goto oops_einval;

	bigint_and_bigint:
		xs = STIX_OBJ_GET_SIZE(x);
		ys = STIX_OBJ_GET_SIZE(y);

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

		negx = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)? 1: 0;
		negy = (STIX_OBJ_GET_CLASS(y) == stix->_large_negative_integer)? 1: 0;

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

		stix_pushtmp (stix, &x);
		stix_pushtmp (stix, &y);
		z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, zalloc);
		stix_poptmps (stix, 2);
		if (!z) return STIX_NULL;

		if (negx && negy)
		{
			/* both are negative */
			stix_lidw_t w[2];
			stix_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;

				w[1] = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry[1];
				carry[1] = w[1] >> STIX_LIW_BITS;

				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0] | (stix_liw_t)w[1];
			}
			STIX_ASSERT (carry[1] == 0);

			/* do nothing about the extra part in x and the lacking part
			 * in y for the reason shown in [NOTE] in the 'else if' block
			 * further down. */

		adjust_to_negative:
			/* 2's complement on the final result */
			((stix_oop_liword_t)z)->slot[zs] = STIX_TYPE_MAX(stix_liw_t);
			carry[0] = 1;
			for (i = 0; i <= zs; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)z)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0];
			}
			STIX_ASSERT (carry[0] == 0);

			STIX_OBJ_SET_CLASS (z, stix->_large_negative_integer);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			stix_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w | ((stix_oop_liword_t)y)->slot[i];
			}

			for (; i < xs; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w;
			}

			STIX_ASSERT (carry == 0);
			goto adjust_to_negative;
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			stix_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] | (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

			/* [NOTE]
			 *  in theory, the lacking part in ys is all 1s when y is
			 *  extended to the width of x. but those 1s are inverted to
			 *  0s when another 2's complement is performed over the final
			 *  result after the jump to 'adjust_to_negative'.
			 *  setting zs to 'xs + 1' and performing the following loop is 
			 *  redundant.
			for (; i < xs; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = STIX_TYPE_MAX(stix_liw_t);
			}
			*/
			goto adjust_to_negative;
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] | ((stix_oop_liword_t)y)->slot[i];
			}

			for (; i < xs; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i];
			}
		}

		return normalize_bigint(stix, z);
	}

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_bitxorints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	if (STIX_OOP_IS_SMOOI(x) && STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v1, v2, v3;

		v1 = STIX_OOP_TO_SMOOI(x);
		v2 = STIX_OOP_TO_SMOOI(y);
		v3 = v1 ^ v2;

		if (STIX_IN_SMOOI_RANGE(v3)) return STIX_SMOOI_TO_OOP(v3);
		return make_bigint_with_ooi (stix, v3);
	}
	else if (STIX_OOP_IS_SMOOI(x))
	{
		stix_ooi_t v;

		if (!is_integer(stix, y)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(x);
		if (v == 0) return clone_bigint(stix, y, STIX_OBJ_GET_SIZE(y));

		stix_pushtmp (stix, &y);
		x = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else if (STIX_OOP_IS_SMOOI(y))
	{
		stix_ooi_t v;

		if (!is_integer(stix, x)) goto oops_einval;

		v = STIX_OOP_TO_SMOOI(y);
		if (v == 0) return clone_bigint(stix, x, STIX_OBJ_GET_SIZE(x));

		stix_pushtmp (stix, &x);
		y = make_bigint_with_ooi (stix, v);
		stix_poptmp (stix);
		if (!x) return STIX_NULL;

		goto bigint_and_bigint;
	}
	else
	{
		stix_oop_t z;
		stix_oow_t i, xs, ys, zs, zalloc;
		int negx, negy;

		if (!is_integer(stix,x) || !is_integer(stix, y)) goto oops_einval;

	bigint_and_bigint:
		xs = STIX_OBJ_GET_SIZE(x);
		ys = STIX_OBJ_GET_SIZE(y);

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

		negx = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)? 1: 0;
		negy = (STIX_OBJ_GET_CLASS(y) == stix->_large_negative_integer)? 1: 0;

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

		stix_pushtmp (stix, &x);
		stix_pushtmp (stix, &y);
		z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, zalloc);
		stix_poptmps (stix, 2);
		if (!z) return STIX_NULL;

		if (negx && negy)
		{
			/* both are negative */
			stix_lidw_t w[2];
			stix_lidw_t carry[2];

			carry[0] = 1;
			carry[1] = 1;
			/* 2's complement on both x and y and perform bitwise-and */
			for (i = 0; i < ys; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;

				w[1] = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry[1];
				carry[1] = w[1] >> STIX_LIW_BITS;

				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0] ^ (stix_liw_t)w[1];
			}
			STIX_ASSERT (carry[1] == 0);

			/* treat the lacking part in y as all 1s */
			for (; i < xs; i++)
			{
				w[0] = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry[0];
				carry[0] = w[0] >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w[0] ^ STIX_TYPE_MAX(stix_liw_t);
			}
			STIX_ASSERT (carry[0] == 0);
		}
		else if (negx)
		{
			/* x is negative, y is positive */
			stix_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w ^ ((stix_oop_liword_t)y)->slot[i];
			}

			/* treat the lacking part in y as all 0s */
			for (; i < xs; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

		adjust_to_negative:
			/* 2's complement on the final result */
			((stix_oop_liword_t)z)->slot[zs] = STIX_TYPE_MAX(stix_liw_t);
			carry = 1;
			for (i = 0; i <= zs; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)z)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

			STIX_OBJ_SET_CLASS (z, stix->_large_negative_integer);
		}
		else if (negy)
		{
			/* x is positive, y is negative  */
			stix_lidw_t w, carry;

			/* x & 2's complement on y up to ys */
			carry = 1;
			for (i = 0; i < ys; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)y)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] ^ (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

			/* treat the lacking part in y as all 1s */
			for (; i < xs; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] ^ STIX_TYPE_MAX(stix_liw_t);
			}

			goto adjust_to_negative;
		}
		else
		{
			/* both are positive */
			for (i = 0; i < ys; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i] ^ ((stix_oop_liword_t)y)->slot[i];
			}

			/* treat the lacking part in y as all 0s */
			for (; i < xs; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)x)->slot[i];
			}
		}

		return normalize_bigint(stix, z);
	}

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_bitinvint (stix_t* stix, stix_oop_t x)
{
	if (STIX_OOP_IS_SMOOI(x))
	{
		stix_ooi_t v;

		v = STIX_OOP_TO_SMOOI(x);
		v = ~v;

		if (STIX_IN_SMOOI_RANGE(v)) return STIX_SMOOI_TO_OOP(v);
		return make_bigint_with_ooi (stix, v);
	}
	else
	{
		stix_oop_t z;
		stix_oow_t i, xs, zs, zalloc;
		int negx;

		xs = STIX_OBJ_GET_SIZE(x);
		negx = (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)? 1: 0;

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

		stix_pushtmp (stix, &x);
		z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, zalloc);
		stix_poptmp (stix);
		if (!z) return STIX_NULL;

		if (negx)
		{
			stix_lidw_t w, carry;

			carry = 1;
			for (i = 0; i < xs; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)x)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = ~(stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);
		}
		else
		{
			stix_lidw_t w, carry;

			for (i = 0; i < xs; i++)
			{
				((stix_oop_liword_t)z)->slot[i] = ~((stix_oop_liword_t)x)->slot[i];
			}

			/* 2's complement on the final result */
			((stix_oop_liword_t)z)->slot[zs] = STIX_TYPE_MAX(stix_liw_t);
			carry = 1;
			for (i = 0; i <= zs; i++)
			{
				w = (stix_lidw_t)(~((stix_oop_liword_t)z)->slot[i]) + carry;
				carry = w >> STIX_LIW_BITS;
				((stix_oop_liword_t)z)->slot[i] = (stix_liw_t)w;
			}
			STIX_ASSERT (carry == 0);

			STIX_OBJ_SET_CLASS (z, stix->_large_negative_integer);
		}

		return normalize_bigint(stix, z);
	}
}

static stix_uint8_t ooch_val_tab[] =
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

stix_oop_t stix_strtoint (stix_t* stix, const stix_ooch_t* str, stix_oow_t len, int radix)
{
	int sign = 1;
	const stix_ooch_t* ptr, * start, * end;
	stix_lidw_t w, v;
	stix_liw_t hw[16], * hwp = STIX_NULL;
	stix_oow_t hwlen, outlen;
	stix_oop_t res;

	if (radix < 0) 
	{
		/* when radix is less than 0, it treats it as if '-' is preceeding */
		sign = -1;
		radix = -radix;
	}

	STIX_ASSERT (radix >= 2 && radix <= 36);

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
		return STIX_SMOOI_TO_OOP(0);
	}

	hwlen = 0;
	start = ptr; /* this is the real start */

	if (IS_POWER_OF_2(radix))
	{
		unsigned int exp;
		unsigned int bitcnt;

		/* get log2(radix) in a fast way under the fact that
		 * radix is a power of 2. the exponent acquired is
		 * the number of bits that a digit of the given radix takes up */
	#if defined(STIX_HAVE_BUILTIN_CTZ)
		exp = __builtin_ctz(radix);

	#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
		/* use the Bit Scan Forward instruction */
		__asm__ volatile (
			"bsf %1,%0\n\t"
			: "=r"(exp) /* output */
			: "r"(radix) /* input */
		);

	#elif defined(USE_UGLY_CODE) && defined(__GNUC__) && defined(__arm__) && (defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_8__))

		/* CLZ is available in ARMv5T and above. there is no instruction to
		 * count trailing zeros or something similar. using RBIT with CLZ
		 * would be good in ARMv6T2 and above to avoid further calculation
		 * afte CLZ */
		__asm__ volatile (
			"clz %0,%1\n\t"
			: "=r"(exp) /* output */
			: "r"(radix) /* input */
		);
		exp = (STIX_SIZEOF(exp) * 8) - exp - 1; 

		/* TODO: PPC - use cntlz, cntlzw, cntlzd, SPARC - use lzcnt, MIPS clz */
	#else
		static stix_uint8_t exp_tab[] = 
		{
			0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0
		};
		exp = exp_tab[radix];
	#endif

		/* bytes */
		outlen = ((stix_oow_t)(end - str) * exp + 7) / 8; 
		/* number of stix_liw_t */
		outlen = (outlen + STIX_SIZEOF(hw[0]) - 1) / STIX_SIZEOF(hw[0]);

		if (outlen > STIX_COUNTOF(hw)) 
		{
			hwp = stix_allocmem (stix, outlen * STIX_SIZEOF(hw[0]));
			if (!hwp) return STIX_NULL;
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
			if (*ptr < 0 || *ptr >= STIX_COUNTOF(ooch_val_tab)) goto oops_einval;
			v = ooch_val_tab[*ptr];
			if (v >= radix) goto oops_einval;

			w |= (v << bitcnt);
			bitcnt += exp;
			if (bitcnt >= STIX_LIW_BITS)
			{
				bitcnt -= STIX_LIW_BITS;
				hwp[hwlen++] = w; /*(stix_liw_t)(w & STIX_LBMASK(stix_lidw_t, STIX_LIW_BITS));*/
				w >>= STIX_LIW_BITS;
			}

			ptr--;
		}

		STIX_ASSERT (w <= STIX_TYPE_MAX(stix_liw_t));
		if (hwlen == 0 || w > 0) hwp[hwlen++] = w;
	}
	else
	{
		stix_lidw_t r1, r2;
		stix_liw_t multiplier;
		int dg, i, safe_ndigits;

		w = 0;
		ptr = start;

		safe_ndigits = stix->bigint[radix].safe_ndigits;
		multiplier = (stix_liw_t)stix->bigint[radix].multiplier;

		outlen = (end - str) / safe_ndigits + 1;
		if (outlen > STIX_COUNTOF(hw)) 
		{
			hwp = stix_allocmem (stix, outlen * STIX_SIZEOF(stix_liw_t));
			if (!hwp) return STIX_NULL;
		}
		else
		{
			hwp = hw;
		}

		STIX_ASSERT (ptr < end);
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

				if (*ptr < 0 || *ptr >= STIX_COUNTOF(ooch_val_tab)) goto oops_einval;
				v = ooch_val_tab[*ptr];
				if (v >= radix) goto oops_einval;

				r1 = r1 * radix + (stix_liw_t)v;
				ptr++;
			}


			r2 = r1;
			for (i = 0; i < hwlen; i++)
			{
				stix_liw_t high, low;

				v = (stix_lidw_t)hwp[i] * multiplier;
				high = (stix_liw_t)(v >> STIX_LIW_BITS);
				low = (stix_liw_t)(v /*& STIX_LBMASK(stix_oow_t, STIX_LIW_BITS)*/);

			#if defined(liw_add_overflow)
				/* use liw_add_overflow() only if it's compiler-builtin. */
				r2 = high + liw_add_overflow(low, r2, &low);
			#else
				/* don't use the fall-back version of liw_add_overflow() */
				low += r2;
				r2 = (stix_lidw_t)high + (low < r2);
			#endif

				hwp[i] = low;
			}
			if (r2) hwp[hwlen++] = (stix_liw_t)r2;
		}
		while (ptr < end);
	}

	STIX_ASSERT (hwlen >= 1);

#if (STIX_LIW_BITS == STIX_OOW_BITS)
	if (hwlen == 1) 
	{
		w = hwp[0];
		STIX_ASSERT (-STIX_SMOOI_MAX == STIX_SMOOI_MIN);
		if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP((stix_ooi_t)w * sign);
	}
#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
	if (hwlen == 1) 
	{
		STIX_ASSERT (hwp[0] <= STIX_SMOOI_MAX);
		return STIX_SMOOI_TO_OOP((stix_ooi_t)hwp[0] * sign);
	}
	else if (hwlen == 2)
	{
		w = MAKE_WORD(hwp[0], hwp[1]);
		STIX_ASSERT (-STIX_SMOOI_MAX == STIX_SMOOI_MIN);
		if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP((stix_ooi_t)w * sign);
	}
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

	res = stix_instantiate (stix, (sign < 0? stix->_large_negative_integer: stix->_large_positive_integer), hwp, hwlen);
	if (hwp && hw != hwp) stix_freemem (stix, hwp);

	return res;

oops_einval:
	if (hwp && hw != hwp) stix_freemem (stix, hwp);
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

static stix_oow_t oow_to_text (stix_oow_t w, int radix, stix_ooch_t* buf)
{
	stix_ooch_t* ptr;
	static char* digitc = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	STIX_ASSERT (radix >= 2 && radix <= 36);

	ptr = buf;
	do
	{
		*ptr++ = digitc[w % radix];
		w /= radix;
	}
	while (w > 0);

	return ptr - buf;
}

static void reverse_string (stix_ooch_t* str, stix_oow_t len)
{
	stix_ooch_t ch;
	stix_ooch_t* start = str;
	stix_ooch_t* end = str + len - 1;

	while (start < end)
	{
		ch = *start;
		*start++ = *end;
		*end-- = ch;
	}
}

stix_oop_t stix_inttostr (stix_t* stix, stix_oop_t num, int radix)
{
	stix_ooi_t v = 0;
	stix_oow_t w;
	stix_oow_t as, bs, rs;
#if STIX_LIW_BITS == STIX_OOW_BITS
	stix_liw_t b[1];
#elif STIX_LIW_BITS == STIX_OOHW_BITS
	stix_liw_t b[2];
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif
	stix_liw_t* a, * q, * r;
	stix_liw_t* t = STIX_NULL;
	stix_ooch_t* xbuf = STIX_NULL;
	stix_oow_t xlen = 0, seglen;
	stix_oop_t s;

	if (STIX_OOP_IS_SMOOI(num))
	{
		v = STIX_OOP_TO_SMOOI(num);
		if (v < 0)
		{
			w = -v;
			v = -1;
		}
		else
		{
			w = v;
			v = 1;
		}
	}
	else if (!is_integer(stix,num)) 
	{
		goto oops_einval;
	}
	else
	{
	#if (STIX_LIW_BITS == STIX_OOW_BITS)
		if (STIX_OBJ_GET_SIZE(num) == 1)
		{
			w = ((stix_oop_word_t)num)->slot[0];
			v = (STIX_OBJ_GET_CLASS(num) == stix->_large_negative_integer)? -1: 1;
		}
	#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
		STIX_ASSERT (STIX_OBJ_GET_SIZE(num) >= 2);
		if (STIX_OBJ_GET_SIZE(num) == 2)
		{
			w = MAKE_WORD (((stix_oop_halfword_t)num)->slot[0], ((stix_oop_halfword_t)num)->slot[1]);
			v = (STIX_OBJ_GET_CLASS(num) == stix->_large_negative_integer)? -1: 1;
		}
	#else
	#	error UNSUPPORTED LIW BIT SIZE
	#endif
	}

	if (v)
	{
		/* Use a static buffer for simple conversion as the largest
		 * size is known. The largest buffer is required for radix 2.
		 * For a binary conversion(radix 2), the number of bits is
		 * the maximum number of digits that can be produced. +1 is
		 * needed for the sign. */
		stix_ooch_t buf[STIX_OOW_BITS + 1];
		stix_oow_t len;

		len = oow_to_text (w, radix, buf);
		if (v < 0) buf[len++] = '-';

		reverse_string (buf, len);
		return stix_makestring (stix, buf, len);
	}

	/* Do it in a hard way */
#if (STIX_LIW_BITS == STIX_OOW_BITS)
	b[0] = stix->bigint[radix].multiplier; /* block divisor */
	bs = 1;
#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
	b[0] = stix->bigint[radix].multiplier & STIX_LBMASK(stix_oow_t, STIX_OOHW_BITS);
	b[1] = stix->bigint[radix].multiplier >> STIX_OOHW_BITS;
	bs = (b[1] > 0)? 2: 1;
#else
#	error UNSUPPORTED LIW BIT SIZE
#endif

	as = STIX_OBJ_GET_SIZE(num);

/* TODO: migrate these buffers into stix_t? */
/* TODO: find an optimial buffer size */
	xbuf = (stix_ooch_t*)stix_allocmem (stix, STIX_SIZEOF(*xbuf) * (as * STIX_LIW_BITS + 1));
	if (!xbuf) return STIX_NULL;

	t = (stix_liw_t*)stix_callocmem (stix, STIX_SIZEOF(*t) * as * 3);
	if (!t) 
	{
		stix_freemem (stix, xbuf);
		return STIX_NULL;
	}

	a = &t[0];
	q = &t[as];
	r = &t[as * 2];

	STIX_MEMCPY (a, ((stix_oop_liword_t)num)->slot, STIX_SIZEOF(*a) * as);

	do
	{
		if (is_less_unsigned_array (b, bs, a, as))
		{
			stix_liw_t* tmp;

			divide_unsigned_array (a, as, b, bs, q, r);

			/* get 'rs' before 'as' gets changed */
			rs = count_effective (r, as); 
 
			/* swap a and q for later division */
			tmp = a;
			a = q;
			q = tmp;

			as = count_effective (a, as);
		}
		else
		{
			/* it is the last block */
			r = a;
			rs = as;
		}

	#if (STIX_LIW_BITS == STIX_OOW_BITS)
		STIX_ASSERT (rs == 1);
		w = r[0];
	#elif (STIX_LIW_BITS == STIX_OOHW_BITS)
		if (rs == 1) w = r[0];
		else 
		{
			STIX_ASSERT (rs == 2);
			w = MAKE_WORD (r[0], r[1]);
		}
	#else
	#	error UNSUPPORTED LIW BIT SIZE
	#endif
		seglen = oow_to_text (w, radix, &xbuf[xlen]);
		xlen += seglen;
		if (r == a) break; /* reached the last block */

		/* fill unfilled leading digits with zeros as it's not 
		 * the last block */
		while (seglen < stix->bigint[radix].safe_ndigits)
		{
			xbuf[xlen++] = '0';
			seglen++;
		}
	}
	while (1);

	if (STIX_OBJ_GET_CLASS(num) == stix->_large_negative_integer) xbuf[xlen++] = '-';
	reverse_string (xbuf, xlen);
	s = stix_makestring (stix, xbuf, xlen);

	stix_freemem (stix, t);
	stix_freemem (stix, xbuf);
	return s;

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}
