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

#include "moo-prv.h"

moo_oop_t moo_makefpdec (moo_t* moo, moo_oop_t value, moo_ooi_t scale)
{
	moo_oop_fpdec_t fpdec;

	MOO_ASSERT (moo, moo_isint(moo, value));
	if (scale <= 0) return value; /* if scale is 0 or less, return the value as it it */

	if (scale > MOO_SMOOI_MAX)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "fpdec scale too large - %zd", scale);
		return MOO_NULL;
	}

	moo_pushvolat (moo, &value);
	fpdec = (moo_oop_fpdec_t)moo_instantiate(moo, moo->_fixed_point_decimal, MOO_NULL, 0);
	moo_popvolat (moo);
	if (!fpdec) return MOO_NULL;

	MOO_STORE_OOP (moo, &fpdec->value, value);
	fpdec->scale = MOO_SMOOI_TO_OOP(scale);

	return (moo_oop_t)fpdec;
}

static moo_ooi_t equalize_scale (moo_t* moo, moo_oop_t* x, moo_oop_t* y)
{
	moo_ooi_t xs, ys;
	moo_oop_t nv;
	moo_oop_t xv, yv;

	/* this function assumes that x and y are protected by the caller */

	xs = 0;
	xv = *x;
	if (MOO_OOP_IS_FPDEC(moo, xv))
	{
		xs = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)xv)->scale);
		xv = ((moo_oop_fpdec_t)xv)->value;
	}
	else if (!moo_isint(moo, xv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", xv);
		return -1;
	}

	ys = 0;
	yv = *y;
	if (MOO_OOP_IS_FPDEC(moo, yv))
	{
		ys = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)yv)->scale);
		yv = ((moo_oop_fpdec_t)yv)->value;
	}
	else if (!moo_isint(moo, yv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", yv);
		return -1;
	}

	if (xs < ys)
	{
		nv = xv;
		while (xs < ys)
		{
			/* TODO: optmize this. less multiplications */
			nv = moo_mulints(moo, nv, MOO_SMOOI_TO_OOP(10));
			if (!nv) return -1;
			xs++;
		}

		nv = moo_makefpdec(moo, nv, xs);
		if (!nv) return -1;

		*x = nv;
	}
	else if (xs > ys)
	{
		nv = yv;
		while (ys < xs)
		{
			nv = moo_mulints(moo, nv, MOO_SMOOI_TO_OOP(10));
			if (!nv) return -1;
			ys++;
		}

		nv = moo_makefpdec(moo, nv, ys);
		if (!nv) return -1;

		*y = nv;
	}

	return xs;
}

moo_oop_t moo_truncfpdecval (moo_t* moo, moo_oop_t iv, moo_ooi_t cs, moo_ooi_t ns)
{
	/* this function truncates an existing fixed-point decimal.
	 * it doesn't create a new object */

	if (cs > ns)
	{
		do
		{
			/* TODO: optimization... less divisions */
			iv = moo_divints(moo, iv, MOO_SMOOI_TO_OOP(10), 0, MOO_NULL);
			if (!iv) return MOO_NULL;
			cs--;
		}
		while (cs > ns);
	}

	return iv;
}

moo_oop_t moo_addnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (!MOO_OOP_IS_FPDEC(moo, x) && !MOO_OOP_IS_FPDEC(moo, y))
	{
		/* both are probably integers */
		return moo_addints(moo, x, y);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t scale;

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);

		scale = equalize_scale(moo, &x, &y);
		if (scale <= -1) 
		{
			moo_popvolats (moo, 2);
			return MOO_NULL;
		}
		v = moo_addints(moo, ((moo_oop_fpdec_t)x)->value, ((moo_oop_fpdec_t)y)->value);
		moo_popvolats (moo, 2);
		if (!v) return MOO_NULL;

		return moo_makefpdec(moo, v, scale);
	}
}

moo_oop_t moo_subnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	if (!MOO_OOP_IS_FPDEC(moo, x) && !MOO_OOP_IS_FPDEC(moo, y))
	{
		/* both are probably integers */
		return moo_subints(moo, x, y);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t scale;

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);

		scale = equalize_scale(moo, &x, &y);
		if (scale <= -1) 
		{
			moo_popvolats (moo, 2);
			return MOO_NULL;
		}
		v = moo_subints(moo, ((moo_oop_fpdec_t)x)->value, ((moo_oop_fpdec_t)y)->value);
		moo_popvolats (moo, 2);
		if (!v) return MOO_NULL;

		return moo_makefpdec(moo, v, scale);
	}
}

static moo_oop_t mul_nums (moo_t* moo, moo_oop_t x, moo_oop_t y, int mult)
{
	moo_ooi_t xs, ys, cs, ns;
	moo_oop_t nv;
	moo_oop_t xv, yv;

	xs = 0;
	xv = x;
	if (MOO_OOP_IS_FPDEC(moo, xv))
	{
		xs = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)xv)->scale);
		xv = ((moo_oop_fpdec_t)xv)->value;
	}
	else if (!moo_isint(moo, xv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", xv);
		return MOO_NULL;
	}
	
	ys = 0;
	yv = y;
	if (MOO_OOP_IS_FPDEC(moo, y))
	{
		ys = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)yv)->scale);
		yv = ((moo_oop_fpdec_t)yv)->value;
	}
	else if (!moo_isint(moo, yv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", yv);
		return MOO_NULL;
	}

	nv = moo_mulints(moo, xv, yv);
	if (!nv) return MOO_NULL;

	cs = xs + ys; 
	if (cs <= 0) return nv; /* the result must be an integer */

	ns = (mult || xs > ys)? xs: ys;

	/* cs may be larger than MOO_SMOOI_MAX. but ns is guaranteed to be
	 * equal to or less than MOO_SMOOI_MAX */
	MOO_ASSERT (moo, ns <= MOO_SMOOI_MAX);

	nv = moo_truncfpdecval(moo, nv, cs, ns);
	if (!nv) return MOO_NULL;

	return (ns <= 0)? nv: moo_makefpdec(moo, nv, ns);
}

moo_oop_t moo_mulnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	/* (* 1.00 12.123) => 12.123 */
	return mul_nums(moo, x, y, 0);
}

moo_oop_t moo_mltnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	/* (mlt 1.00 12.123) =>  12.12 */
	return mul_nums(moo, x, y, 1);
}

moo_oop_t moo_divnums (moo_t* moo, moo_oop_t x, moo_oop_t y, int modulo)
{
	moo_ooi_t xs, ys, i;
	moo_oop_t nv;
	moo_oop_t xv, yv;

	xs = 0;
	xv = x;
	if (MOO_OOP_IS_FPDEC(moo, xv))
	{
		xs = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)xv)->scale);
		xv = ((moo_oop_fpdec_t)xv)->value;
	}
	else if (!moo_isint(moo, xv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", xv);
		return MOO_NULL;
	}

	ys = 0;
	yv = y;
	if (MOO_OOP_IS_FPDEC(moo, y))
	{
		ys = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)yv)->scale);
		yv = ((moo_oop_fpdec_t)yv)->value;
	}
	else if (!moo_isint(moo, yv))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not numeric - %O", yv);
		return MOO_NULL;
	}

	nv = xv;

	moo_pushvolat (moo, &yv);
	for (i = 0; i < ys; i++)
	{
		nv = moo_mulints(moo, nv, MOO_SMOOI_TO_OOP(10));
		if (!nv) 
		{
			moo_popvolat (moo);
			return MOO_NULL;
		}
	}

	nv = moo_divints(moo, nv, yv, modulo, MOO_NULL);
	moo_popvolat (moo);
	if (!nv) return MOO_NULL;

	return moo_makefpdec(moo, nv, xs);
}

static moo_oop_t comp_nums (moo_t* moo, moo_oop_t x, moo_oop_t y, moo_oop_t (*comper) (moo_t*, moo_oop_t, moo_oop_t))
{
	if (!MOO_OOP_IS_FPDEC(moo, x) && !MOO_OOP_IS_FPDEC(moo, y))
	{
		/* both are probably integers */
		return comper(moo, x, y);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t scale;

		moo_pushvolat (moo, &x);
		moo_pushvolat (moo, &y);

		scale = equalize_scale(moo, &x, &y);
		if (scale <= -1) 
		{
			moo_popvolats (moo, 2);
			return MOO_NULL;
		}
		v = comper(moo, ((moo_oop_fpdec_t)x)->value, ((moo_oop_fpdec_t)y)->value);
		moo_popvolats (moo, 2);
		return v;
	}
}


moo_oop_t moo_gtnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_gtints);
}
moo_oop_t moo_genums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_geints);
}


moo_oop_t moo_ltnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_ltints);
}
moo_oop_t moo_lenums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_leints);
}

moo_oop_t moo_eqnums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_eqints);
}
moo_oop_t moo_nenums (moo_t* moo, moo_oop_t x, moo_oop_t y)
{
	return comp_nums(moo, x, y, moo_neints);
}


moo_oop_t moo_negatenum (moo_t* moo, moo_oop_t x)
{
	if (!MOO_OOP_IS_FPDEC(moo, x))
	{
		return moo_negateint(moo, x);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t scale;

		scale = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)x)->scale);
		v = ((moo_oop_fpdec_t)x)->value;

		v = moo_negateint(moo, v);
		if (!v) return MOO_NULL;

		return moo_makefpdec(moo, v, scale);
	}
}

moo_oop_t moo_sqrtnum (moo_t* moo, moo_oop_t x)
{
	if (!MOO_OOP_IS_FPDEC(moo, x))
	{
		return moo_sqrtint(moo, x);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t i, scale;

		scale = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)x)->scale);

		v = ((moo_oop_fpdec_t)x)->value;
		for (i = 0; i < scale ; i++)
		{
			v = moo_mulints(moo, v, MOO_SMOOI_TO_OOP(10));
			if (!v)
			{
				moo_popvolat (moo);
				return MOO_NULL;
			}
		}

		v = moo_sqrtint(moo, v);
		if (!v) return MOO_NULL;

		return moo_makefpdec(moo, v, scale);
	}
}

moo_oop_t moo_absnum (moo_t* moo, moo_oop_t x)
{
	if (!MOO_OOP_IS_FPDEC(moo, x))
	{
		return moo_absint(moo, x);
	}
	else
	{
		moo_oop_t v;
		moo_ooi_t scale;

		scale = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)x)->scale);
		v = ((moo_oop_fpdec_t)x)->value;

		v = moo_absint(moo, v);
		if (!v) return MOO_NULL;

		return moo_makefpdec(moo, v, scale);
	}
}
