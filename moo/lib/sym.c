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

static moo_oop_oop_t expand_bucket (moo_t* moo, moo_oop_oop_t oldbuc)
{
	moo_oop_oop_t newbuc;
	moo_oow_t oldsz, newsz, index;
	moo_oop_char_t symbol;

	oldsz = MOO_OBJ_GET_SIZE(oldbuc);

/* TODO: better growth policy? */
	if (oldsz < 5000) newsz = oldsz + oldsz;
	else if (oldsz < 50000) newsz = oldsz + (oldsz / 2);
	else if (oldsz < 100000) newsz = oldsz + (oldsz / 4);
	else if (oldsz < 200000) newsz = oldsz + (oldsz / 8);
	else if (oldsz < 400000) newsz = oldsz + (oldsz / 16);
	else if (oldsz < 800000) newsz = oldsz + (oldsz / 32);
	else if (oldsz < 1600000) newsz = oldsz + (oldsz / 64);
	else 
	{
		moo_oow_t inc, inc_max;

		inc = oldsz / 128;
		inc_max = MOO_OBJ_SIZE_MAX - oldsz;
		if (inc > inc_max) 
		{
			if (inc_max > 0) inc = inc_max;
			else
			{
				moo_seterrnum (moo, MOO_EOOMEM);
				return MOO_NULL;
			}
		}
		newsz = oldsz + inc;
	}

	moo_pushtmp (moo, (moo_oop_t*)&oldbuc);
	newbuc = (moo_oop_oop_t)moo_instantiate (moo, moo->_array, MOO_NULL, newsz); 
	moo_poptmp (moo);
	if (!newbuc) return MOO_NULL;

	while (oldsz > 0)
	{
		symbol = (moo_oop_char_t)oldbuc->slot[--oldsz];
		if ((moo_oop_t)symbol != moo->_nil)
		{
			MOO_ASSERT (moo, MOO_CLASSOF(moo,symbol) == moo->_symbol);
			/*MOO_ASSERT (moo, sym->size > 0);*/

			index = moo_hashoochars(symbol->slot, MOO_OBJ_GET_SIZE(symbol)) % newsz;
			while (newbuc->slot[index] != moo->_nil) index = (index + 1) % newsz;
			newbuc->slot[index] = (moo_oop_t)symbol;
		}
	}

	return newbuc;
}

static moo_oop_t find_or_make_symbol (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len, int create)
{
	moo_ooi_t tally;
	moo_oow_t index;
	moo_oop_char_t symbol;

	MOO_ASSERT (moo, MOO_CLASSOF(moo,moo->symtab->bucket) == moo->_array);
	index = moo_hashoochars(ptr, len) % MOO_OBJ_GET_SIZE(moo->symtab->bucket);

	/* find a matching symbol in the open-addressed symbol table */
	while (moo->symtab->bucket->slot[index] != moo->_nil) 
	{
		symbol = (moo_oop_char_t)moo->symtab->bucket->slot[index];
		MOO_ASSERT (moo, MOO_CLASSOF(moo,symbol) == moo->_symbol);

		if (len == MOO_OBJ_GET_SIZE(symbol) &&
		    moo_equaloochars (ptr, symbol->slot, len))
		{
			return (moo_oop_t)symbol;
		}

		index = (index + 1) % MOO_OBJ_GET_SIZE(moo->symtab->bucket);
	}

	if (!create) 
	{
		moo_seterrnum (moo, MOO_ENOENT);
		return MOO_NULL;
	}

	/* make a new symbol and insert it */
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(moo->symtab->tally));
	tally = MOO_OOP_TO_SMOOI(moo->symtab->tally);
	if (tally >= MOO_SMOOI_MAX)
	{
		/* this built-in table is not allowed to hold more than 
		 * MOO_SMOOI_MAX items for efficiency sake */
		moo_seterrnum (moo, MOO_EDFULL);
		return MOO_NULL;
	}

	/* no conversion to moo_oow_t is necessary for tally + 1.
	 * the maximum value of tally is checked to be MOO_SMOOI_MAX - 1.
	 * tally + 1 can produce at most MOO_SMOOI_MAX. above all, 
	 * MOO_SMOOI_MAX is way smaller than MOO_TYPE_MAX(moo_ooi_t). */
	if (tally + 1 >= MOO_OBJ_GET_SIZE(moo->symtab->bucket))
	{
		moo_oop_oop_t bucket;

		/* TODO: make the growth policy configurable instead of growing
		         it just before it gets full. The polcy can be grow it
		         if it's 70% full */

		/* enlarge the symbol table before it gets full to
		 * make sure that it has at least one free slot left
		 * after having added a new symbol. this is to help
		 * traversal end at a _nil slot if no entry is found. */
		bucket = expand_bucket(moo, moo->symtab->bucket);
		if (!bucket) return MOO_NULL;

		moo->symtab->bucket = bucket;

		/* recalculate the index for the expanded bucket */
		index = moo_hashoochars(ptr, len) % MOO_OBJ_GET_SIZE(moo->symtab->bucket);

		while (moo->symtab->bucket->slot[index] != moo->_nil) 
			index = (index + 1) % MOO_OBJ_GET_SIZE(moo->symtab->bucket);
	}

	/* create a new symbol since it isn't found in the symbol table */
	symbol = (moo_oop_char_t)moo_instantiate(moo, moo->_symbol, ptr, len);
	if (symbol)
	{
		MOO_ASSERT (moo, tally < MOO_SMOOI_MAX);
		moo->symtab->tally = MOO_SMOOI_TO_OOP(tally + 1);
		moo->symtab->bucket->slot[index] = (moo_oop_t)symbol;
	}

	return (moo_oop_t)symbol;
}

moo_oop_t moo_makesymbol (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len)
{
	return find_or_make_symbol (moo, ptr, len, 1);
}

moo_oop_t moo_findsymbol (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len)
{
	return find_or_make_symbol (moo, ptr, len, 0);
}

moo_oop_t moo_makestringwithbchars (moo_t* moo, const moo_bch_t* ptr, moo_oow_t len)
{
#if defined(MOO_OOCH_IS_UCH)
	moo_oow_t inlen, outlen;
	moo_oop_t obj;

	inlen = len;
	if (moo_convbtouchars (moo, ptr, &inlen, MOO_NULL, &outlen) <= -1) return MOO_NULL;
	obj = moo_instantiate (moo, moo->_string, MOO_NULL, outlen);
	if (!obj) return MOO_NULL;

	inlen = len;
	moo_convbtouchars (moo, ptr, &inlen, MOO_OBJ_GET_CHAR_SLOT(obj), &outlen);
	return obj;
#else
	return moo_instantiate (moo, moo->_string, ptr, len);
#endif
}

moo_oop_t moo_makestringwithuchars (moo_t* moo, const moo_uch_t* ptr, moo_oow_t len)
{
#if defined(MOO_OOCH_IS_UCH)
	return moo_instantiate (moo, moo->_string, ptr, len);
#else
	moo_oow_t inlen, outlen;
	moo_oop_t obj;

	inlen = len;
	if (moo_convutobchars (moo, ptr, &inlen, MOO_NULL, &outlen) <= -1) return MOO_NULL;
	obj = moo_instantiate (moo, moo->_string, MOO_NULL, outlen);
	if (!obj) return MOO_NULL;

	inlen = len;
	moo_convutobchars (moo, ptr, &inlen, MOO_OBJ_GET_CHAR_SLOT(obj), &outlen);
	return obj;
#endif
}
