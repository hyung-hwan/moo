
#include "stix-prv.h"

/*static normalize_integer (*/

static stix_oop_t normalize (stix_t* stix, stix_oop_t z)
{
	return z;
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
	stix_oow_t as, bs, zs, ks;
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

	ks = add_unsigned_array (a, as, b, bs, ((stix_oop_halfword_t)z)->slot);
	if (ks <= zs)
	{
		/*normalize;*/
	}

	return z;
}

static stix_oop_t subtract_unsigned_integers (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

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


stix_oop_t stix_addbigints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
	stix_oop_t z;

	if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
	{
		if (STIX_OBJ_GET_CLASS(x) == stix->_large_negative_integer)
			z = stix_subbigints (stix, y, x);
		else
			z = stix_subbigints (stix, x, y);
	}
	else
	{
		z = add_unsigned_integers (stix, x, y);
	}

	return normalize(stix, z);
}

#if 0
stix_subbigints (stix_t* stix, stix_oop_t x, stix_oop_t y)
{
/* TOOD: ensure both are LargeIntgers */

	if (STIX_OBJ_GET_CLASS(x) != STIX_OBJ_GET_CLASS(y))
	{
		z = add_unsigned (stix, x, y);
	}
	else
	{
		if (is_less_unsigned (x, y))
		{
			z = subtract_unsigned (stix, y, x); /* get the opposite sign of x; */
		}
		else
		{
			
			z = subtract_unsigned (stix, x, y); /* take x's sign */
		}
	}

	return normalize (stix, z);
}
#endif
