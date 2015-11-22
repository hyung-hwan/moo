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


#if defined(STIX_USE_FULL_WORD)
	/* nothing special */
#else
#	define MAKE_WORD(hw1,hw2) ((stix_oow_t)(hw1) | (stix_oow_t)(hw2) << STIX_LIW_BITS)
#endif

/*#define IS_POWER_OF_2(ui) (((ui) > 0) && (((ui) & (~(ui)+ 1)) == (ui)))*/
#define IS_POWER_OF_2(ui) (((ui) > 0) && ((ui) & ((ui) - 1)) == 0) /* unsigned integer only */

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
	return c == stix->_small_integer ||
	       c == stix->_large_positive_integer ||
	       c == stix->_large_negative_integer;
}

static STIX_INLINE stix_oop_t make_bigint_with_ooi (stix_t* stix, stix_ooi_t i)
{
#if defined(STIX_USE_FULL_WORD)
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
#else
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

static STIX_INLINE stix_oop_t clone_bigint (stix_t* stix, stix_oop_t oop, stix_oow_t count)
{
	stix_oop_t z;
	stix_oow_t i;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	if (count <= 0) count = STIX_OBJ_GET_SIZE(oop);

	stix_pushtmp (stix, &oop);
	z = stix_instantiate (stix, STIX_OBJ_GET_CLASS(oop), STIX_NULL, count);
	stix_poptmp (stix);
	if (!z) return STIX_NULL;

	for (i = 0; i < count; i++)
	{
		((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)oop)->slot[i];
	}
	return z;
}

static STIX_INLINE stix_oop_t clone_bigint_negated (stix_t* stix, stix_oop_t oop, stix_oow_t count)
{
	stix_oop_t z, c;
	stix_oow_t i;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	if (count <= 0) count = STIX_OBJ_GET_SIZE(oop);

	if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		c = stix->_large_negative_integer;
	else
		c = stix->_large_positive_integer;

	stix_pushtmp (stix, &oop);
	z = stix_instantiate (stix, c, STIX_NULL, count);
	stix_poptmp (stix);
	if (!z) return STIX_NULL;

	for (i = 0; i < count; i++)
	{
		((stix_oop_liword_t)z)->slot[i] = ((stix_oop_liword_t)oop)->slot[i];
	}
	return z;
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

#if defined(STIX_USE_FULL_WORD)
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
#else
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

	stix_liw_t* d;
	stix_oow_t ds;

/* TODO: */
	while (!is_less_unsigned_array (d, ds, y, ys)) /* while (y <= d) */
	{
		
	}

	STIX_MEMCPY (r, d, ds);
}

static stix_oop_t add_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_liw_t* a, * b;
	stix_oow_t as, bs, zs;
	stix_oop_t z;

	as = STIX_OBJ_GET_SIZE(x);
	bs = STIX_OBJ_GET_SIZE(y);
	zs = (as >= bs? as: bs) + 1;

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = stix_instantiate (stix, STIX_OBJ_GET_CLASS(x), STIX_NULL, zs);
	stix_poptmps (stix, 2);

	if (as >= bs)
	{
		a = ((stix_oop_liword_t)x)->slot;
		b = ((stix_oop_liword_t)y)->slot;
	}
	else 
	{
		a = ((stix_oop_liword_t)y)->slot;
		b = ((stix_oop_liword_t)x)->slot;
	}

	add_unsigned_array (a, as, b, bs, ((stix_oop_liword_t)z)->slot);
	return z;
}

static stix_oop_t subtract_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	STIX_ASSERT (!is_less(stix, x, y));

	stix_pushtmp (stix, &x);
	stix_pushtmp (stix, &y);
	z = stix_instantiate (stix, stix->_large_positive_integer, STIX_NULL, STIX_OBJ_GET_SIZE(x));
	stix_poptmps (stix, 2);

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

	multiply_unsigned_array (
		((stix_oop_liword_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((stix_oop_liword_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((stix_oop_liword_t)z)->slot);
	return z;
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
				z = stix_subints (stix, y, x);
			}
			else
			{
				/* x is positive, y is negative */
				z = stix_subints (stix, x, y);
			}
			if (!z) return STIX_NULL;
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
			if (!x) return STIX_NULL;
			stix_poptmp (stix);
		}
		else if (STIX_OOP_IS_SMOOI(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMOOI(y);
			if (v == 0) return clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));

			stix_pushtmp (stix, &x);
			y = make_bigint_with_ooi (stix, v);
			if (!y) return STIX_NULL;
			stix_poptmp (stix);
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

{ int i;
printf ("MUL=>");
for (i = STIX_OBJ_GET_SIZE(z); i > 0;)
{
printf ("%0*lX ", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)z)->slot[--i]);
}
printf ("\n");
}

/*
lshift_unsigned_array (((stix_oop_liword_t)z)->slot, STIX_OBJ_GET_SIZE(z), 16 * 5 + 4);
{ int i;
printf ("LSHIFT10=>");
for (i = STIX_OBJ_GET_SIZE(z); i > 0;)
{
printf ("%0*lX ", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)z)->slot[--i]);
}
printf ("\n");
}*/


	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_divints (stix_t* stix, stix_oop_t x, stix_oop_t y, int modulo, stix_oop_t* rem)
{
	stix_oop_t t;

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

		/* In C89, integer division with a negative number  is
		 * implementation dependent. In C99, it truncates towards zero.
		 * 
		 * http://python-history.blogspot.kr/2010/08/why-pythons-integer-division-floors.html
		 *   The integer division operation (//) and its sibling, 
		 *   the modulo operation (%), go together and satisfy a nice
		 *   mathematical relationship (all variables are integers):
		 *      a/b = q with remainder r
		 *   such that
		 *      b*q + r = a and 0 <= r < b (assuming a and b are >= 0).
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
			if (r && ((yv ^ r) < 0))
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
				 * change the sign of r to the dividend's sign */
				r -= yv;
				++q;
				STIX_ASSERT (xv && ((xv ^ r) >= 0));
			}
		}

		if (rem)
		{
			STIX_ASSERT (STIX_IN_SMOOI_RANGE(r));
			*rem = STIX_SMOOI_TO_OOP(r);
		}

		return STIX_SMOOI_TO_OOP((stix_ooi_t)q);
	}
	else if (STIX_OOP_IS_SMOOI(x))
	{
		if (STIX_OOP_TO_SMOOI(x) == 0)
		{
			if (rem)
			{
				t = clone_bigint (stix, y, STIX_OBJ_GET_SIZE(y));
				if (!t) return STIX_NULL;
				*rem = t;
			}
			return STIX_SMOOI_TO_OOP(0);
		}
/* TODO: convert x to bigint */
	}
	else if (STIX_OOP_IS_SMOOI(y))
	{
		switch (STIX_OOP_TO_SMOOI(y))
		{
			case 0:
				stix->errnum = STIX_EDIVBY0;
				return STIX_NULL;

			case 1:
				t = clone_bigint (stix, x, STIX_OBJ_GET_SIZE(x));
				if (!t) return STIX_NULL;
				if (rem) *rem = STIX_SMOOI_TO_OOP(0);
				return t;
				
			case -1:
				t = clone_bigint_negated (stix, x, STIX_OBJ_GET_SIZE(x));
				if (!t) return STIX_NULL;
				if (rem) *rem = STIX_SMOOI_TO_OOP(0);
				return t;
		}
/* TODO: convert y to bigint */
	}

/* TODO: do bigint division. */
	return STIX_NULL;
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
	int neg = 0;
	const stix_ooch_t* ptr, * start, * end;
	stix_lidw_t w, v;
	stix_liw_t hw[16], * hwp = STIX_NULL;
	stix_oow_t hwlen, outlen;
	stix_oop_t res;

	if (radix < 0) 
	{
		neg = 1;
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
			neg = 1;
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

{ int i;
for (i = hwlen; i > 0;)
{
printf ("%0*lx ", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)hwp[--i]);
}
printf ("\n");
}

	STIX_ASSERT (hwlen >= 1);
#if defined(STIX_USE_FULL_WORD)
	if (hwlen == 1) 
	{
		w = hwp[0];
		if (neg) 
		{
			STIX_ASSERT (-STIX_SMOOI_MAX == STIX_SMOOI_MIN);
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(-(stix_ooi_t)w);
		}
		else 
		{
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(w);
		}
	}
#else
	if (hwlen == 1) return STIX_SMOOI_TO_OOP((stix_ooi_t)hwp[0] * -neg);
	else if (hwlen == 2)
	{
		w = MAKE_WORD(hwp[0], hwp[1]);
		if (neg) 
		{
			STIX_ASSERT (-STIX_SMOOI_MAX == STIX_SMOOI_MIN);
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(-(stix_ooi_t)w);
		}
		else 
		{
			if (w <= STIX_SMOOI_MAX) return STIX_SMOOI_TO_OOP(w);
		}
	}
#endif

	res = stix_instantiate (stix, (neg? stix->_large_negative_integer: stix->_large_positive_integer), hwp, hwlen);
	if (hwp && hw != hwp) stix_freemem (stix, hwp);
	return res;

oops_einval:
	if (hwp && hw != hwp) stix_freemem (stix, hwp);
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

stix_oop_t stix_inttostr (stix_t* stix, stix_oop_t num)
{
	return STIX_NULL;
}
