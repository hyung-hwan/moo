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
	return b > 0 && a > STIX_TYPE_MAX(stix_oow_t) / b; /* works for unsigned types only */
#endif
}
#endif

#if (SIZEOF_ATOM_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UADD_OVERFLOW)
#	define atom_add_overflow(a,b,c)  __builtin_uadd_overflow(a,b,c)
#elif (SIZEOF_ATOM_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UADDL_OVERFLOW)
#	define atom_add_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (SIZEOF_ATOM_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UADDLL_OVERFLOW)
#	define atom_add_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static STIX_INLINE int atom_add_overflow (atom_t a, atom_t b, atom_t* c)
{
	*c = a + b;
	return b > STIX_TYPE_MAX(atom_t) - a;
}
#endif

#if (SIZEOF_ATOM_T == STIX_SIZEOF_INT) && defined(STIX_HAVE_BUILTIN_UMUL_OVERFLOW)
#	define atom_mul_overflow(a,b,c)  __builtin_umul_overflow(a,b,c)
#elif (SIZEOF_ATOM_T == STIX_SIZEOF_LONG) && defined(STIX_HAVE_BUILTIN_UMULL_OVERFLOW)
#	define atom_mul_overflow(a,b,c)  __builtin_uaddl_overflow(a,b,c)
#elif (SIZEOF_ATOM_T == STIX_SIZEOF_LONG_LONG) && defined(STIX_HAVE_BUILTIN_UMULLL_OVERFLOW)
#	define atom_mul_overflow(a,b,c)  __builtin_uaddll_overflow(a,b,c)
#else
static STIX_INLINE int atom_mul_overflow (atom_t a, atom_t b, atom_t* c)
{
#if (STIX_SIZEOF_UINTMAX_T > SIZEOF_ATOM_T)
	stix_uintmax_t k;
	k = (stix_uintmax_t)a * (stix_uintmax_t)b;
	*c = (atom_t)k;
	return (k >> ATOM_BITS) > 0;
	/*return k > STIX_TYPE_MAX(atom_t);*/
#else
	*c = a * b;
	return b > 0 && a > STIX_TYPE_MAX(atom_t) / b; /* works for unsigned types only */
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
#if defined(USE_FULL_WORD)
	stix_oow_t w;

	if (i >= 0)
	{
		w = i;
		return stix_instantiate (stix, stix->_large_positive_integer, &w, 1);
	}
	else
	{
		/* The caller must ensure that i is grater than the smallest value
		 * that stix_ooi_t can represent. otherwise, the absolute value 
		 * cannot be held in stix_ooi_t. */
		STIX_ASSERT (i > STIX_TYPE_MIN(stix_ooi_t));
		w = -i;
		return stix_instantiate (stix, stix->_large_negative_integer, &w, 1);
	}
#else
	atom_t hw[2];
	stix_oow_t w;

	if (i >= 0)
	{
		w = i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,ATOM_BITS);
		hw[1] = w >> ATOM_BITS;
		return stix_instantiate (stix, stix->_large_positive_integer, &hw, (hw[1] > 0? 2: 1));
	}
	else
	{
		STIX_ASSERT (i > STIX_TYPE_MIN(stix_ooi_t));
		w = -i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,ATOM_BITS);
		hw[1] = w >> ATOM_BITS;
		return stix_instantiate (stix, stix->_large_negative_integer, &hw, (hw[1] > 0? 2: 1));
	}
#endif
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
		((oop_atom_t)z)->slot[i] = ((oop_atom_t)oop)->slot[i];
	}
	return z;
}

static STIX_INLINE stix_oow_t count_effective_digits (stix_oop_t oop)
{
	stix_oow_t i;

	for (i = STIX_OBJ_GET_SIZE(oop); i > 1; )
	{
		--i;
		if (((oop_atom_t)oop)->slot[i] != 0) return i + 1;
	}

	return 1;
}

static stix_oop_t normalize_bigint (stix_t* stix, stix_oop_t oop)
{
	stix_oow_t count;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	count = count_effective_digits (oop);

#if defined(USE_FULL_WORD)
	if (count == 1)
	{
		stix_oow_t w;

		w = ((oop_atom_t)oop)->slot[0];
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			if (w <= STIX_SMINT_MAX) return STIX_OOP_FROM_SMINT(w);
		}
		else
		{
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			/*if (w <= -STIX_SMINT_MIN) */
			if (w <= ((stix_oow_t)STIX_SMINT_MAX + 1)) return STIX_OOP_FROM_SMINT(-(stix_ooi_t)w);
		}
	}
#else
	if (count == 1)
	{
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			return STIX_OOP_FROM_SMINT(((oop_atom_t)oop)->slot[0]);
		}
		else
		{
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			return STIX_OOP_FROM_SMINT(-(stix_oow_t)((oop_atom_t)oop)->slot[0]);
		}
	}
	else if (count == 2)
	{
		stix_oow_t w;

		w = MAKE_WORD (((oop_atom_t)oop)->slot[0], ((oop_atom_t)oop)->slot[1]);
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			if (w <= STIX_SMINT_MAX) return STIX_OOP_FROM_SMINT(w);
		}
		else
		{
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			/*if (w <= -STIX_SMINT_MIN) */
			if (w <= ((stix_oow_t)STIX_SMINT_MAX + 1)) return STIX_OOP_FROM_SMINT(-(stix_ooi_t)w);
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


static STIX_INLINE int is_less_unsigned_array (const atom_t* x, stix_oow_t xs, const atom_t* y, stix_oow_t ys)
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
		((oop_atom_t)x)->slot, STIX_OBJ_GET_SIZE(x), 
		((oop_atom_t)y)->slot, STIX_OBJ_GET_SIZE(y));
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
	       STIX_MEMCMP(((oop_atom_t)x)->slot,  ((oop_atom_t)y)->slot, STIX_OBJ_GET_SIZE(x) * STIX_SIZEOF(atom_t)) == 0;
}

static STIX_INLINE stix_oow_t add_unsigned_array (const atom_t* x, stix_oow_t xs, const atom_t* y, stix_oow_t ys, atom_t* z)
{
	stix_oow_t i;
	bigatom_t w;
	bigatom_t carry = 0;

	STIX_ASSERT (xs >= ys);

	for (i = 0; i < ys; i++)
	{
		w = (bigatom_t)x[i] + (bigatom_t)y[i] + carry;
		carry = w >> ATOM_BITS;
		z[i] = w /*& STIX_LBMASK(bigatom_t, ATOM_BITS) */;
	}

	for (; i < xs; i++)
	{
		w = (bigatom_t)x[i] + carry;
		carry = w >> ATOM_BITS;
		z[i] = w /*& STIX_LBMASK(bigatom_t, ATOM_BITS)*/;
	}

	if (i > 1 && carry == 0) return i - 1;
	z[i] = carry;

	return i;
}

static STIX_INLINE stix_oow_t subtract_unsigned_array (const atom_t* x, stix_oow_t xs, const atom_t* y, stix_oow_t ys, atom_t* z)
{
	stix_oow_t i;
	bigatom_t w;
	bigatom_t borrow = 0;
	bigatom_t borrowed_word;

	STIX_ASSERT (!is_less_unsigned_array(x, xs, y, ys));

	borrowed_word = (bigatom_t)1 << ATOM_BITS;
	for (i = 0; i < ys; i++)
	{
		w = (bigatom_t)y[i] + borrow;
		if ((bigatom_t)x[i] >= w)
		{
			z[i] = x[i] - w;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (bigatom_t)x[i]) - w; 
			borrow = 1;
		}
	}

	for (; i < xs; i++)
	{
		if (x[i] >= borrow) 
		{
			z[i] = x[i] - (atom_t)borrow;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (bigatom_t)x[i]) - borrow;
			borrow = 1;
		}
	}

	STIX_ASSERT (borrow == 0);
	return i;
}

static STIX_INLINE void multiply_unsigned_array (const atom_t* x, stix_oow_t xs, const atom_t* y, stix_oow_t ys, atom_t* z)
{
	bigatom_t v;
	stix_oow_t pa;

/* TODO: implement Karatsuba  or Toom-Cook 3-way algorithm when the input length is long */

	pa = (xs < ys)? xs: ys;
	if (pa <= ((stix_oow_t)1 << (BIGATOM_BITS - (ATOM_BITS * 2))))
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
				v = v + (bigatom_t)x[tx + iz] * (bigatom_t)y[ty - iz];
			}

			z[ix] = (atom_t)v;
			v = v >> ATOM_BITS;
		}
	}
	else
	{
	#if 1
		stix_oow_t i, j;
		atom_t carry;

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
					v = (bigatom_t)x[j] * (bigatom_t)y[i] + (bigatom_t)carry + (bigatom_t)z[j + i];
					z[j + i] = (atom_t)v;
					carry = (atom_t)(v >> ATOM_BITS);
				}

				z[xs + i] = carry;
			}
		}

	#else
		stix_oow_t i, j, idx;
		atom_t carry;

		for (i = 0; i < ys; i++)
		{
			idx = i;

			for (j = 0; j < xs; j++)
			{
				v = (bigatom_t)x[j] * (bigatom_t)y[i] + (bigatom_t)carry + (bigatom_t)z[idx];
				z[idx] = (atom_t)v;
				carry = (atom_t)(v >> ATOM_BITS);
				idx++;
			}

			while (carry > 0)
			{
				v = (bigatom_t)z[idx] + (bigatom_t)carry;
				z[idx] = (atom_t)v;
				carry = (atom_t)(v >> ATOM_BITS);
				idx++;
			}

		}
	#endif
	}
}

static stix_oop_t add_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	atom_t* a, * b;
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
		a = ((oop_atom_t)x)->slot;
		b = ((oop_atom_t)y)->slot;
	}
	else 
	{
		a = ((oop_atom_t)y)->slot;
		b = ((oop_atom_t)x)->slot;
	}

	add_unsigned_array (a, as, b, bs, ((oop_atom_t)z)->slot);
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
		((oop_atom_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((oop_atom_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((oop_atom_t)z)->slot);
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
		((oop_atom_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((oop_atom_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((oop_atom_t)z)->slot);
	return z;
}

stix_oop_t stix_addints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	if (STIX_OOP_IS_SMINT(x) && STIX_OOP_IS_SMINT(y))
	{
		stix_ooi_t i;
		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		i = STIX_OOP_TO_SMINT(x) + STIX_OOP_TO_SMINT(y);
		if (STIX_OOI_IN_SMINT_RANGE(i)) return STIX_OOP_FROM_SMINT(i);

		STIX_ASSERT (STIX_SMINT_MAX + STIX_SMINT_MAX < STIX_TYPE_MAX(stix_ooi_t));
		STIX_ASSERT (STIX_SMINT_MIN + STIX_SMINT_MIN > STIX_TYPE_MAX(stix_ooi_t));
		return make_bigint_with_ooi (stix, i);
	}
	else
	{
		stix_ooi_t v;
		int neg;

		if (STIX_OOP_IS_SMINT(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(x);
			if (v == 0) return y;

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);

			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMINT(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(y);
			if (v == 0) return x;

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

	if (STIX_OOP_IS_SMINT(x) && STIX_OOP_IS_SMINT(y))
	{
		stix_ooi_t i;
		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		i = STIX_OOP_TO_SMINT(x) - STIX_OOP_TO_SMINT(y);
		if (STIX_OOI_IN_SMINT_RANGE(i)) return STIX_OOP_FROM_SMINT(i);

		return make_bigint_with_ooi (stix, i);
	}
	else
	{
		stix_ooi_t v;
		int neg;

		if (STIX_OOP_IS_SMINT(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(x);
			if (v == 0) 
			{
				/* switch the sign to the opposite and return it */
				neg = (STIX_OBJ_GET_CLASS(y) == stix->_large_negative_integer);
				z = clone_bigint (stix, y, 0);
				if (!neg) STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
				return z;
			}

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			if (!x) return STIX_NULL;
			stix_poptmp (stix);
		}
		else if (STIX_OOP_IS_SMINT(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(y);
			if (v == 0) return x;

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

	if (STIX_OOP_IS_SMINT(x) && STIX_OOP_IS_SMINT(y))
	{
/* TODO: XXXXXXXXXXXXXXXXXXXXXXXxx */
		stix_ooi_t i;
		/* no integer overflow/underflow must occur as the possible integer
		 * range is narrowed by the tag bits used */
		i = STIX_OOP_TO_SMINT(x) * STIX_OOP_TO_SMINT(y);
		if (STIX_OOI_IN_SMINT_RANGE(i)) return STIX_OOP_FROM_SMINT(i);

		STIX_ASSERT (STIX_SMINT_MAX + STIX_SMINT_MAX < STIX_TYPE_MAX(stix_ooi_t));
		STIX_ASSERT (STIX_SMINT_MIN + STIX_SMINT_MIN > STIX_TYPE_MAX(stix_ooi_t));
		return make_bigint_with_ooi (stix, i);
	}
	else
	{
		stix_ooi_t v;

		if (STIX_OOP_IS_SMINT(x))
		{
			if (!is_integer(stix,y)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(x);
			if (v == 0) return STIX_OOP_FROM_SMINT(0);

			stix_pushtmp (stix, &y);
			x = make_bigint_with_ooi (stix, v);
			stix_poptmp (stix);

			if (!x) return STIX_NULL;
		}
		else if (STIX_OOP_IS_SMINT(y))
		{
			if (!is_integer(stix,x)) goto oops_einval;

			v = STIX_OOP_TO_SMINT(y);
			if (v == 0) return STIX_OOP_FROM_SMINT(0);

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

		z = multiply_unsigned_integers (stix, x, y);
		if (!z) return STIX_NULL;
		if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
			STIX_OBJ_SET_CLASS(z, stix->_large_negative_integer);
	}

{ int i;
printf ("MUL=>");
for (i = STIX_OBJ_GET_SIZE(z); i > 0;)
{
printf ("%016lX ", (unsigned long)((oop_atom_t)z)->slot[--i]);
}
printf ("\n");
}

	return normalize_bigint (stix, z);

oops_einval:
	stix->errnum = STIX_EINVAL;
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

stix_oop_t stix_strtoint (stix_t* stix, const stix_ooch_t* str, stix_oow_t len, unsigned int radix)
{
	int neg = 0;
	const stix_ooch_t* ptr, * start, * end;
	bigatom_t w, v;
	atom_t hw[16], * hwp = STIX_NULL;
	stix_oow_t hwlen, outlen;
	stix_oop_t res;

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
		return STIX_OOP_FROM_SMINT(0);
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
		/* number of atom_t */
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
			if (bitcnt >= ATOM_BITS)
			{
				bitcnt -= ATOM_BITS;
				hwp[hwlen++] = w; /*(atom_t)(w & STIX_LBMASK(bigatom_t, ATOM_BITS));*/
				w >>= ATOM_BITS;
			}

			ptr--;
		}

		STIX_ASSERT (w <= STIX_TYPE_MAX(atom_t));
		if (hwlen == 0 || w > 0) hwp[hwlen++] = w;
	}
	else
	{
		bigatom_t r1, r2;
		atom_t multiplier;
		int dg, i, safe_ndigits;

		w = 0;
		ptr = start;

		safe_ndigits = stix->bigint[radix].safe_ndigits;
		multiplier = (atom_t)stix->bigint[radix].multiplier;

		outlen = (end - str) / safe_ndigits + 1;
		if (outlen > STIX_COUNTOF(hw)) 
		{
			hwp = stix_allocmem (stix, outlen * STIX_SIZEOF(atom_t));
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

				r1 = r1 * radix + (atom_t)v;
				ptr++;
			}


			r2 = r1;
			for (i = 0; i < hwlen; i++)
			{
				atom_t high, low;

				v = (bigatom_t)hwp[i] * multiplier;
				high = (atom_t)(v >> ATOM_BITS);
				low = (atom_t)(v /*& STIX_LBMASK(stix_oow_t, ATOM_BITS)*/);

			#if defined(atom_add_overflow)
				/* use atom_add_overflow() only if it's compiler-builtin. */
				r2 = high + atom_add_overflow(low, r2, &low);
			#else
				/* don't use the fall-back version of atom_add_overflow() */
				low += r2;
				r2 = (bigatom_t)high + (low < r2);
			#endif

				hwp[i] = low;
			}
			if (r2) hwp[hwlen++] = (atom_t)r2;
		}
		while (ptr < end);
	}

{ int i;
for (i = hwlen; i > 0;)
{
printf ("%08lx ", (unsigned long)hwp[--i]);
}
printf ("\n");
}

	STIX_ASSERT (hwlen > 1);
#if defined(USE_FULL_WORD)
	if (hwlen == 1) 
	{
		w = hwp[0];
		if (neg) 
		{
			if (w <= STIX_SMINT_MAX + 1) return STIX_OOP_FROM_SMINT(-(stix_ooi_t)w);
		}
		else 
		{
			if (w <= STIX_SMINT_MAX) return STIX_OOP_FROM_SMINT(w);
		}
	}
#else
	if (hwlen == 1) return STIX_OOP_FROM_SMINT((stix_ooi_t)hwp[0] * -neg);
	else if (hwlen == 2)
	{
		w = MAKE_WORD(hwp[0], hwp[1]);
		if (neg) 
		{
			if (w <= STIX_SMINT_MAX + 1) return STIX_OOP_FROM_SMINT(-(stix_ooi_t)w);
		}
		else 
		{
			if (w <= STIX_SMINT_MAX) return STIX_OOP_FROM_SMINT(w);
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
