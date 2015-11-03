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

#define MAKE_WORD(hw1,hw2) ((stix_oow_t)(hw1) | (stix_oow_t)(hw2) << STIX_OOHW_BITS)

/*#define IS_POWER_OF_2(ui) (((ui) > 0) && (((ui) & (~(ui)+ 1)) == (ui)))*/
#define IS_POWER_OF_2(ui) (((ui) > 0) && ((ui) & ((ui) - 1)) == 0) /* unsigned integer only */

static STIX_INLINE int is_integer (stix_t* stix, stix_oop_t oop)
{
	stix_oop_t c;

	c = STIX_CLASSOF(stix,oop);

/* TODO: is it better to introduct a special integer mark into the class itself */
	return c == stix->_small_integer ||
	       c == stix->_large_positive_integer ||
	       c == stix->_large_negative_integer;
}

static STIX_INLINE stix_oop_t make_bigint_with_ooi (stix_t* stix, stix_ooi_t i)
{
	stix_oohw_t hw[2];
	stix_oow_t w;

	if (i >= 0)
	{
		w = i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,STIX_OOHW_BITS);
		hw[1] = w >> STIX_OOHW_BITS;
		return stix_instantiate (stix, stix->_large_positive_integer, &hw, (hw[1] > 0? 2: 1));
	}
	else
	{
		w = -i;
		hw[0] = w & STIX_LBMASK(stix_oow_t,STIX_OOHW_BITS);
		hw[1] = w >> STIX_OOHW_BITS;
		return stix_instantiate (stix, stix->_large_negative_integer, &hw, (hw[1] > 0? 2: 1));
	}
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
		((stix_oop_halfword_t)z)->slot[i] = ((stix_oop_halfword_t)oop)->slot[i];
	}
	return z;
}

static STIX_INLINE stix_oow_t count_effective_digits (stix_oop_t oop)
{
	stix_oow_t i;

	for (i = STIX_OBJ_GET_SIZE(oop); i > 1; )
	{
		--i;
		if (((stix_oop_halfword_t)oop)->slot[i] != 0) return i + 1;
	}

	return 1;
}

static stix_oop_t normalize_bigint (stix_t* stix, stix_oop_t oop)
{
	stix_oow_t count;

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
	count = count_effective_digits (oop);
	if (count == 1)
	{
		if (STIX_OBJ_GET_CLASS(oop) == stix->_large_positive_integer)
		{
			return STIX_OOP_FROM_SMINT(((stix_oop_halfword_t)oop)->slot[0]);
		}
		else
		{
			STIX_ASSERT (STIX_OBJ_GET_CLASS(oop) == stix->_large_negative_integer);
			return STIX_OOP_FROM_SMINT(-(stix_oow_t)((stix_oop_halfword_t)oop)->slot[0]);
		}
	}
	else if (count == 2)
	{
		stix_oow_t w;

		w = MAKE_WORD (((stix_oop_halfword_t)oop)->slot[0], ((stix_oop_halfword_t)oop)->slot[1]);
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

	if (STIX_OBJ_GET_SIZE(oop) == count)
	{
		/* no compaction is needed. return it as it is */
		return oop;
	}

	return clone_bigint (stix, oop, count);
}


static STIX_INLINE int is_less_unsigned_array (const stix_oohw_t* x, stix_oow_t xs, const stix_oohw_t* y, stix_oow_t ys)
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
		((stix_oop_halfword_t)x)->slot, STIX_OBJ_GET_SIZE(x), 
		((stix_oop_halfword_t)y)->slot, STIX_OBJ_GET_SIZE(y));
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
	       STIX_MEMCMP(((stix_oop_halfword_t)x)->slot,  ((stix_oop_halfword_t)y)->slot, STIX_OBJ_GET_SIZE(x) * STIX_SIZEOF(stix_oohw_t)) == 0;
}

static STIX_INLINE stix_oow_t add_unsigned_array (const stix_oohw_t* x, stix_oow_t xs, const stix_oohw_t* y, stix_oow_t ys, stix_oohw_t* z)
{
	stix_oow_t i, w;
	stix_oow_t carry = 0;

	STIX_ASSERT (xs >= ys);

	for (i = 0; i < ys; i++)
	{
		w = (stix_oow_t)x[i] + (stix_oow_t)y[i] + carry;
		carry = w >> STIX_OOHW_BITS;
		z[i] = w & STIX_LBMASK(stix_oow_t, STIX_OOHW_BITS);
	}

	for (; i < xs; i++)
	{
		w = (stix_oow_t)x[i] + carry;
		carry = w >> STIX_OOHW_BITS;
		z[i] = w & STIX_LBMASK(stix_oow_t, STIX_OOHW_BITS);
	}

	if (i > 1 && carry == 0) return i - 1;
	z[i] = carry;
	return i;
}

static STIX_INLINE stix_oow_t subtract_unsigned_array (const stix_oohw_t* x, stix_oow_t xs, const stix_oohw_t* y, stix_oow_t ys, stix_oohw_t* z)
{
	stix_oow_t i, w;
	stix_oow_t borrow = 0;
	stix_oow_t borrowed_word;

	STIX_ASSERT (!is_less_unsigned_array(x, xs, y, ys));

	borrowed_word = (stix_oow_t)1 << STIX_OOHW_BITS;
	for (i = 0; i < ys; i++)
	{
		w = (stix_oow_t)y[i] + borrow;
		if ((stix_oow_t)x[i] >= w)
		{
			z[i] = x[i] - w;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (stix_oow_t)x[i]) - w; 
			borrow = 1;
		}
	}

	for (; i < xs; i++)
	{
		if (x[i] >= borrow) 
		{
			z[i] = x[i] - (stix_oohw_t)borrow;
			borrow = 0;
		}
		else
		{
			z[i] = (borrowed_word + (stix_oow_t)x[i]) - borrow;
			borrow = 1;
		}
	}

	STIX_ASSERT (borrow == 0);
	return i;
}

static stix_oop_t add_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oohw_t* a, * b;
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
		a = ((stix_oop_halfword_t)x)->slot;
		b = ((stix_oop_halfword_t)y)->slot;
	}
	else 
	{
		a = ((stix_oop_halfword_t)y)->slot;
		b = ((stix_oop_halfword_t)x)->slot;
	}

	add_unsigned_array (a, as, b, bs, ((stix_oop_halfword_t)z)->slot);
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
		((stix_oop_halfword_t)x)->slot, STIX_OBJ_GET_SIZE(x),
		((stix_oop_halfword_t)y)->slot, STIX_OBJ_GET_SIZE(y),
		((stix_oop_halfword_t)z)->slot);
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



stix_oop_t stix_strtoint (stix_t* stix, const stix_ooch_t* str, stix_oow_t len, unsigned int radix)
{
	int neg = 0;
	const stix_ooch_t* ptr, * start, * end;
	stix_oow_t w, v, r;
	stix_oohw_t hw[64];
	stix_oow_t hwlen;

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

	if (IS_POWER_OF_2(radix))
	{
		unsigned int exp;

		/* get log2(radix) in a fast way under the fact that
		 * radix is a power of 2. */
	#if defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))

		/* use the Bit Scan Forward instruction */
		__asm__ volatile (
			"bsf %1,%0\n\t"
			: "=r"(exp) /* output */
			: "r"(radix) /* input */
		);


	#elif defined(USE_THIS_UGLY_CODE) && defined(__GNUC__) && defined(__arm__) && (defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_8__))

		/* clz is available in ARMv5T and above */
		__asm__ volatile (
			"clz %0,%1\n\t"
			: "=r"(exp) /* output */
			: "r"(radix) /* input */
		);

		/* TODO: in ARMv6T2 and above, RBIT can be used before clz to avoid this calculation  */
		exp = (STIX_SIZEOF(exp) * 8) - exp - 1; 


		/* TODO: PPC - use cntlz, cntlzw, cntlzd, SPARC - use lzcnt, MIPS clz */
	#else

		static unsigned int exp_tab[] = 
		{
			0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0
		};
		exp = exp_tab[radix];
	#endif

printf ("<<%d>>>>>>\n",exp);
		start = ptr; /* this is the real start */
		ptr = end - 1;

		hwlen = 0;
		w = 0;
		r = 1;
		while (ptr >= start)
		{
			if (*ptr >= '0' && *ptr <= '9') v = *ptr - '0';
			else if (*ptr >= 'A' && *ptr <= 'Z') v = *ptr - 'A' + 10;
			else if (*ptr >= 'a' && *ptr <= 'z') v = *ptr - 'a' + 10;
			else goto oops_einval;

			if (v >= radix) goto oops_einval;

	printf ("wwww=<<%lx>>", (long int)w);
			w += v * r;
	printf ("r=><<%lX>> v<<%lX>> w<<%lX>>\n", (long int)r, (long int)v, (long int)w);
			if (w > STIX_TYPE_MAX(stix_oohw_t))
			{
				hw[hwlen++] = (stix_oohw_t)(w & STIX_LBMASK(stix_oow_t, STIX_OOHW_BITS));
				w = w >> STIX_OOHW_BITS;
				if (w == 0) r = 1;
				else r = radix;
			}
			else 
			{
				r = r * radix;
			}

			ptr--;
		}

		STIX_ASSERT (w <= STIX_TYPE_MAX(stix_oohw_t));
		hw[hwlen++] = w;
	}
	else
	{
		/*  TODO: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
	}

/*
	if (hwlen == 1) return STIX_OOP_FROM_SMINT((stix_ooi_t)hw[0] * -neg);
	else if (hwlen == 2)
	{
		w = MAKE_WORD(hw[0], hw[1]);
		if (neg) 
		{
			if (w <= STIX_SMINT_MAX + 1) return STIX_OOP_FROM_SMINT(-(stix_ooi_t)w);
		}
		else 
		{
			if (w <= STIX_SMINT_MAX) return STIX_OOP_FROM_SMINT(w);
		}
	}
*/

{ int i;
for (i = hwlen; i > 0;)
{
printf ("%x ", hw[--i]);
}
printf ("\n");
}
	return stix_instantiate (stix, (neg? stix->_large_negative_integer: stix->_large_positive_integer), hw, hwlen);

oops_einval:
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}
