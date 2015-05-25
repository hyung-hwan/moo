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

#include "stix-prv.h"

static stix_oop_oop_t expand_bucket (stix_t* stix, stix_oop_oop_t old_bucket)
{
	stix_oop_oop_t new_bucket;
	stix_oow_t oldsz, newsz, index;
	stix_oop_association_t ass;
	stix_oop_char_t key;

/* TODO: derive a better growth size */
	new_bucket = (stix_oop_oop_t)stix_instantiate (stix, stix->_array, STIX_NULL, STIX_OBJ_GET_SIZE(old_bucket) * 2); 
	if (!new_bucket) return STIX_NULL;

	oldsz = STIX_OBJ_GET_SIZE(old_bucket);
	newsz = STIX_OBJ_GET_SIZE(new_bucket);

	while (oldsz > 0)
	{
		ass = (stix_oop_association_t)old_bucket->slot[--oldsz];
		if ((stix_oop_t)ass != stix->_nil)
		{
			STIX_ASSERT (STIX_CLASSOF(stix,ass) == stix->_association);

			key = (stix_oop_char_t)ass->key;
			STIX_ASSERT (STIX_CLASSOF(stix,key) == (stix_oop_t)stix->_symbol);

			index = stix_hashchars (key->slot, STIX_OBJ_GET_SIZE(key)) % newsz;
			while (new_bucket->slot[index] != stix->_nil) 
				index = (index + 1) % newsz;
			new_bucket->slot[index] = (stix_oop_t)ass;
		}
	}

	return new_bucket;
}

static stix_oop_t find_or_insert (stix_t* stix, stix_oop_char_t key, stix_oop_t value)
{
	stix_oow_t index, tally;
	stix_oop_association_t ass;
	stix_oow_t tmp_count = 0;

	/* the system dictionary is not a generic dictionary.
	 * it accepts only a symbol as a key. */
	STIX_ASSERT (STIX_CLASSOF(stix,key) == stix->_symbol);
	STIX_ASSERT (STIX_CLASSOF(stix,stix->sysdic->tally) == stix->_small_integer);
	STIX_ASSERT (STIX_CLASSOF(stix,stix->sysdic->bucket) == stix->_array);

	index = stix_hashchars(key->slot, STIX_OBJ_GET_SIZE(key)) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);

	while (stix->sysdic->bucket->slot[index] != stix->_nil) 
	{
		ass = (stix_oop_association_t)stix->sysdic->bucket->slot[index];

		STIX_ASSERT (STIX_CLASSOF(stix,ass) == stix->_association);
		STIX_ASSERT (STIX_CLASSOF(stix,ass->key) == stix->_symbol);

		if (STIX_OBJ_GET_SIZE(key) == STIX_OBJ_GET_SIZE(ass->key) &&
		    stix_equalchars (key->slot, ((stix_oop_char_t)ass->key)->slot, STIX_OBJ_GET_SIZE(key))) 
		{
			return (stix_oop_t)ass;
		}

		index = (index + 1) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);
	}

	if (value == STIX_NULL)
	{
		/* when value is STIX_NULL, perform no insertion */
		stix->errnum = STIX_ENOENT;
		return STIX_NULL;
	}

	stix_pushtmp (stix, (stix_oop_t*)&key); tmp_count++;
	stix_pushtmp (stix, &value); tmp_count++;

	tally = STIX_OOP_TO_SMINT(stix->sysdic->tally);
	if (tally + 1 >= STIX_OBJ_GET_SIZE(stix->sysdic->bucket))
	{
		stix_oop_oop_t bucket;

		/* TODO: make the growth policy configurable instead of growing
			     it just before it gets full. The polcy can be grow it	
			     if it's 70% full */

		/* Enlarge the symbol table before it gets full to
		 * make sure that it has at least one free slot left
		 * after having added a new symbol. this is to help
		 * traversal end at a _nil slot if no entry is found. */
		bucket = expand_bucket (stix, stix->sysdic->bucket);
		if (!bucket) goto oops;

		stix->sysdic->bucket = bucket;

		/* recalculate the index for the expanded bucket */
		index = stix_hashchars(key->slot, STIX_OBJ_GET_SIZE(key)) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);

		while (stix->sysdic->bucket->slot[index] != stix->_nil) 
			index = (index + 1) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);
	}

	/* create a new assocation of a key and a value since 
	 * the key isn't found in the root dictionary */
	ass = (stix_oop_association_t)stix_instantiate (stix, stix->_association, STIX_NULL, 0);
	if (!ass) goto oops;
	
	ass->key = (stix_oop_t)key;
	ass->value = value;

	stix->sysdic->tally = STIX_OOP_FROM_SMINT(tally + 1);
	stix->sysdic->bucket->slot[index] = (stix_oop_t)ass;

	stix_poptmps (stix, tmp_count);
	return (stix_oop_t)ass;

oops:
	stix_poptmps (stix, tmp_count);
	return STIX_NULL;
}

stix_oop_t stix_putatsysdic (stix_t* stix, stix_oop_t key, stix_oop_t value)
{
	STIX_ASSERT (STIX_CLASSOF(stix,key) == stix->_symbol);
	return find_or_insert (stix, (stix_oop_char_t)key, value);
}

stix_oop_t stix_getatsysdic (stix_t* stix, stix_oop_t key)
{
	STIX_ASSERT (STIX_CLASSOF(stix,key) == stix->_symbol);
	return find_or_insert (stix, (stix_oop_char_t)key, STIX_NULL);
}


stix_oop_t stix_lookupsysdic (stix_t* stix, const stix_ucs_t* name)
{
	/* this is special version of stix_getatsysdic() that performs
	 * lookup using a plain string specified */

	stix_oow_t index;
	stix_oop_association_t ass;

	STIX_ASSERT (STIX_CLASSOF(stix,stix->sysdic->tally) == stix->_small_integer);
	STIX_ASSERT (STIX_CLASSOF(stix,stix->sysdic->bucket) == stix->_array);

	index = stix_hashchars(name->ptr, name->len) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);

	while (stix->sysdic->bucket->slot[index] != stix->_nil) 
	{
		ass = (stix_oop_association_t)stix->sysdic->bucket->slot[index];

		STIX_ASSERT (STIX_CLASSOF(stix,ass) == stix->_association);
		STIX_ASSERT (STIX_CLASSOF(stix,ass->key) == stix->_symbol);

		if (name->len == STIX_OBJ_GET_SIZE(ass->key) &&
		    stix_equalchars(name->ptr, ((stix_oop_char_t)ass->key)->slot, name->len)) 
		{
			return (stix_oop_t)ass;
		}

		index = (index + 1) % STIX_OBJ_GET_SIZE(stix->sysdic->bucket);
	}

	/* when value is STIX_NULL, perform no insertion */
	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}
