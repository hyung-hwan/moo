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
	moo_oop_association_t ass;
	moo_oop_char_t key;

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

	moo_pushvolat (moo, (moo_oop_t*)&oldbuc);
	newbuc = (moo_oop_oop_t)moo_instantiate(moo, moo->_array, MOO_NULL, newsz); 
	moo_popvolat (moo);
	if (!newbuc) return MOO_NULL;

	while (oldsz > 0)
	{
		oldsz = oldsz - 1;
		ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(oldbuc, oldsz);
		if ((moo_oop_t)ass != moo->_nil)
		{
			MOO_ASSERT (moo, MOO_CLASSOF(moo,ass) == moo->_association);

			key = (moo_oop_char_t)ass->key;
			MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol);

			index = moo_hash_oochars(MOO_OBJ_GET_CHAR_SLOT(key), MOO_OBJ_GET_SIZE(key)) % newsz;
			while (MOO_OBJ_GET_OOP_VAL(newbuc, index) != moo->_nil) index = (index + 1) % newsz;

			MOO_STORE_OOP (moo, MOO_OBJ_GET_OOP_PTR(newbuc, index), (moo_oop_t)ass);
		}
	}

	return newbuc;
}

static moo_oop_association_t find_or_upsert (moo_t* moo, moo_oop_dic_t dic, moo_oop_char_t key, moo_oop_t value)
{
	moo_ooi_t tally;
	moo_oow_t hv, index;
	moo_oop_association_t ass;
	moo_oow_t volat_count = 0;

	/* the builtin dictionary is not a generic dictionary.
	 * it accepts only a symbol or something similar as a key. */
	/*MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol);*/
	MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(key));
	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->tally) == moo->_small_integer);
	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->bucket) == moo->_array);

	hv = moo_hash_oochars(MOO_OBJ_GET_CHAR_SLOT(key), MOO_OBJ_GET_SIZE(key));
	index = hv % MOO_OBJ_GET_SIZE(dic->bucket);

	/* find */
	while ((moo_oop_t)(ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, index)) != moo->_nil) 
	{
		MOO_ASSERT (moo, MOO_CLASSOF(moo,ass) == moo->_association);
		/*MOO_ASSERT (moo, MOO_CLASSOF(moo,ass->key) == moo->_symbol);*/
		MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(ass->key));

		if (MOO_OBJ_GET_CLASS(key) == MOO_OBJ_GET_CLASS(ass->key) && 
		    MOO_OBJ_GET_SIZE(key) == MOO_OBJ_GET_SIZE(ass->key) &&
		    moo_equal_oochars(MOO_OBJ_GET_CHAR_SLOT(key), MOO_OBJ_GET_CHAR_SLOT(ass->key), MOO_OBJ_GET_SIZE(key))) 
		{
			/* the value of MOO_NULL indicates no insertion or update. */
			if (value) MOO_STORE_OOP (moo, &ass->value, value); /* update */
			return ass;
		}

		index = (index + 1) % MOO_OBJ_GET_SIZE(dic->bucket);
	}

	if (!value)
	{
		/* when value is MOO_NULL, perform no insertion.
		 * the value of MOO_NULL indicates no insertion or update. */
		moo_seterrnum (moo, MOO_ENOENT);
		return MOO_NULL;
	}

	/* the key is not found. insert it. */
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(dic->tally));
	tally = MOO_OOP_TO_SMOOI(dic->tally);
	if (tally >= MOO_SMOOI_MAX)
	{
		/* this built-in dictionary is not allowed to hold more than 
		 * MOO_SMOOI_MAX items for efficiency sake */
		moo_seterrnum (moo, MOO_EDFULL);
		return MOO_NULL;
	}

	moo_pushvolat (moo, (moo_oop_t*)&dic); volat_count++;
	moo_pushvolat (moo, (moo_oop_t*)&key); volat_count++;
	moo_pushvolat (moo, &value); volat_count++;

	/* no conversion to moo_oow_t is necessary for tally + 1.
	 * the maximum value of tally is checked to be MOO_SMOOI_MAX - 1.
	 * tally + 1 can produce at most MOO_SMOOI_MAX. above all, 
	 * MOO_SMOOI_MAX is way smaller than MOO_TYPE_MAX(moo_ooi_t). */
	if (tally + 1 >= MOO_OBJ_GET_SIZE(dic->bucket))
	{
		moo_oop_oop_t tmp;

		/* TODO: make the growth policy configurable instead of growing
			     it just before it gets full. The polcy can be grow it
			     if it's 70% full */

		/* enlarge the bucket before it gets full to
		 * make sure that it has at least one free slot left
		 * after having added a new symbol. this is to help
		 * traversal end at a _nil slot if no entry is found. */
		tmp = expand_bucket(moo, dic->bucket);
		if (!tmp) goto oops;

		MOO_STORE_OOP (moo, (moo_oop_t*)&dic->bucket, (moo_oop_t)tmp);

		/* recalculate the index for the expanded bucket */
		index = hv % MOO_OBJ_GET_SIZE(dic->bucket);

		while (MOO_OBJ_GET_OOP_VAL(dic->bucket, index) != moo->_nil) 
			index = (index + 1) % MOO_OBJ_GET_SIZE(dic->bucket);
	}

	/* create a new assocation of a key and a value since 
	 * the key isn't found in the root dictionary */
	ass = (moo_oop_association_t)moo_instantiate(moo, moo->_association, MOO_NULL, 0);
	if (!ass) goto oops;

	MOO_STORE_OOP (moo, &ass->key, (moo_oop_t)key);
	MOO_STORE_OOP (moo, &ass->value, value);

	/* the current tally must be less than the maximum value. otherwise,
	 * it overflows after increment below */
	MOO_ASSERT (moo, tally < MOO_SMOOI_MAX);
	dic->tally = MOO_SMOOI_TO_OOP(tally + 1); /* no need to use MOO_STORE_OOP as the value is not a pointer object */
	MOO_STORE_OOP (moo, MOO_OBJ_GET_OOP_PTR(dic->bucket, index), (moo_oop_t)ass);

	moo_popvolats (moo, volat_count);
	return ass;

oops:
	moo_popvolats (moo, volat_count);
	return MOO_NULL;
}

moo_oop_association_t moo_lookupdic_noseterr (moo_t* moo, moo_oop_dic_t dic, const moo_oocs_t* name)
{
	/* this is special version of moo_getatsysdic() that performs
	 * lookup using a plain string specified */

	moo_oow_t index;
	moo_oop_association_t ass;

	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->tally) == moo->_small_integer);
	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->bucket) == moo->_array);

	index = moo_hash_oochars(name->ptr, name->len) % MOO_OBJ_GET_SIZE(dic->bucket);

	while ((moo_oop_t)(ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, index)) != moo->_nil) 
	{
		MOO_ASSERT (moo, MOO_CLASSOF(moo,ass) == moo->_association);
		/*MOO_ASSERT (moo, MOO_CLASSOF(moo,ass->key) == moo->_symbol);*/
		MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(ass->key));

		if (name->len == MOO_OBJ_GET_SIZE(ass->key) &&
		    moo_equal_oochars(name->ptr, MOO_OBJ_GET_CHAR_SLOT(ass->key), name->len)) 
		{
			return ass;
		}

		index = (index + 1) % MOO_OBJ_GET_SIZE(dic->bucket);
	}

	/* when value is MOO_NULL, perform no insertion */

	/* moo_seterrXXX() is not called here. the dictionary lookup is very frequent 
	 * and so is lookup failure. for instance, moo_findmethod() calls this over 
	 * a class chain. there might be a failure at each class level. it's waste to
	 * set the error information whenever the failure occurs.
	 * the caller of this function must set the error information upon failure */
	return MOO_NULL;
}

static MOO_INLINE moo_oop_association_t lookup (moo_t* moo, moo_oop_dic_t dic, const moo_oocs_t* name)
{
	moo_oop_association_t ass = moo_lookupdic_noseterr(moo, dic, name);
	if (!ass) moo_seterrbfmt(moo, MOO_ENOENT, "unable to find %.*js in a dictionary", name->len, name->ptr);
	return ass;
}

moo_oop_association_t moo_putatsysdic (moo_t* moo, moo_oop_t key, moo_oop_t value)
{
	MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol);
	return find_or_upsert(moo, (moo_oop_dic_t)moo->sysdic, (moo_oop_char_t)key, value);
}

moo_oop_association_t moo_getatsysdic (moo_t* moo, moo_oop_t key)
{
	MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol);
	return find_or_upsert(moo, (moo_oop_dic_t)moo->sysdic, (moo_oop_char_t)key, MOO_NULL);
}

moo_oop_association_t moo_lookupsysdic (moo_t* moo, const moo_oocs_t* name)
{
	return lookup(moo, (moo_oop_dic_t)moo->sysdic, name);
}

moo_oop_association_t moo_putatdic (moo_t* moo, moo_oop_dic_t dic, moo_oop_t key, moo_oop_t value)
{
	/*MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol);*/
	MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(key));
	return find_or_upsert(moo, dic, (moo_oop_char_t)key, value);
}

moo_oop_association_t moo_getatdic (moo_t* moo, moo_oop_dic_t dic, moo_oop_t key)
{
	/*MOO_ASSERT (moo, MOO_CLASSOF(moo,key) == moo->_symbol); */
	MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(key));
	return find_or_upsert(moo, dic, (moo_oop_char_t)key, MOO_NULL);
}

moo_oop_association_t moo_lookupdic (moo_t* moo, moo_oop_dic_t dic, const moo_oocs_t* name)
{
	return lookup(moo, dic, name);
}

int moo_deletedic (moo_t* moo, moo_oop_dic_t dic, const moo_oocs_t* name)
{
	moo_ooi_t tally;
	moo_oow_t hv, index, bs, i, x, y, z;
	moo_oop_association_t ass;

	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->tally) == moo->_small_integer);
	MOO_ASSERT (moo, MOO_CLASSOF(moo,dic->bucket) == moo->_array);

	tally = MOO_OOP_TO_SMOOI(dic->tally);

	bs = MOO_OBJ_GET_SIZE(dic->bucket);
	hv = moo_hash_oochars(name->ptr, name->len) % bs;
	index = hv % bs;

	/* find */
	while ((moo_oop_t)(ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, index)) != moo->_nil) 
	{
		MOO_ASSERT (moo, MOO_CLASSOF(moo,ass) == moo->_association);
		/*MOO_ASSERT (moo, MOO_CLASSOF(moo,ass->key) == moo->_symbol);*/
		MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(ass->key));

		if (name->len == MOO_OBJ_GET_SIZE(ass->key) &&
		    moo_equal_oochars(name->ptr, MOO_OBJ_GET_CHAR_SLOT(ass->key), name->len)) 
		{
			goto found;
		}

		index = (index + 1) % bs;
	}

	moo_seterrnum (moo, MOO_ENOENT);
	return -1;

found:
	/* compact the cluster */
	for (i = 0, x = index, y = index; i < tally; i++)
	{
		y = (y + 1) % bs;

		/* done if the slot at the current index is empty */
		if ((moo_oop_t)(ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, y)) == moo->_nil)  break;

		/* otherwise get the natural hash index for the data in the slot at
		 * the current hash index */
		z = moo_hash_oochars(MOO_OBJ_GET_CHAR_SLOT(ass->key), MOO_OBJ_GET_SIZE(ass->key)) % bs;

		/* move an element if necesary */
		if ((y > x && (z <= x || z > y)) || (y < x && (z <= x && z > y)))
		{
			MOO_STORE_OOP (moo, MOO_OBJ_GET_OOP_PTR(dic->bucket, x), MOO_OBJ_GET_OOP_VAL(dic->bucket, y));
			x = y;
		}
	}

	MOO_OBJ_SET_OOP_VAL (dic->bucket, x, moo->_nil); /* the value is nil. no MOO_STORE_OOP */

	tally--;
	dic->tally = MOO_SMOOI_TO_OOP(tally);

	return 0;
}

int moo_walkdic (moo_t* moo, moo_oop_dic_t dic, moo_dic_walker_t walker, void* ctx)
{
	moo_oow_t index, count;
	moo_oop_association_t ass;

	count = MOO_OBJ_GET_SIZE(dic->bucket);
	for (index = 0; index < count; index++)
	{
		ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, index);
		if ((moo_oop_t)ass != moo->_nil)
		{

			MOO_ASSERT (moo, MOO_CLASSOF(moo,ass) == moo->_association);
			/*MOO_ASSERT (moo, MOO_CLASSOF(moo,ass->key) == moo->_symbol);*/
			MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(ass->key));

			if (walker(moo, dic, ass, ctx) <= -1) return -1;
		}
	}

	return 0;
}

moo_oop_dic_t moo_makedic (moo_t* moo, moo_oop_class_t _class, moo_oow_t size)
{
	moo_oop_dic_t dic;
	moo_oop_t tmp;

	MOO_ASSERT (moo, MOO_CLASSOF(moo,_class) == moo->_class);
	MOO_ASSERT (moo, MOO_CLASS_SPEC_NAMED_INSTVARS(MOO_OOP_TO_SMOOI(_class->spec)) >= MOO_DIC_NAMED_INSTVARS);

	dic = (moo_oop_dic_t)moo_instantiate(moo, _class, MOO_NULL, 0);
	if (!dic) return MOO_NULL;

	moo_pushvolat (moo, (moo_oop_t*)&dic);
	tmp = moo_instantiate(moo, moo->_array, MOO_NULL, size);
	moo_popvolat (moo);
	if (!tmp) return MOO_NULL;

	dic->tally = MOO_SMOOI_TO_OOP(0);
	MOO_STORE_OOP (moo, (moo_oop_t*)&dic->bucket, tmp);

	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(dic) == MOO_CLASS_SPEC_NAMED_INSTVARS(MOO_OOP_TO_SMOOI(_class->spec)));
	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(dic->bucket) == size);

	return dic;
}

moo_oop_nsdic_t moo_makensdic (moo_t* moo, moo_oop_class_t _class, moo_oow_t size)
{
	MOO_ASSERT (moo, MOO_CLASSOF(moo,_class) == moo->_class);
	MOO_ASSERT (moo, MOO_CLASS_SPEC_NAMED_INSTVARS(MOO_OOP_TO_SMOOI(_class->spec)) >= MOO_NSDIC_NAMED_INSTVARS);
	return (moo_oop_nsdic_t)moo_makedic(moo, _class, size);
}
