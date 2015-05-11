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
	stix_oop_char_t symbol;

/* TODO: derive a better growth size */
	new_bucket = (stix_oop_oop_t)stix_instantiate (stix, stix->_array, STIX_NULL, STIX_OBJ_GET_SIZE(old_bucket) * 2); 
	if (!new_bucket) return STIX_NULL;

	oldsz = STIX_OBJ_GET_SIZE(old_bucket);
	newsz = STIX_OBJ_GET_SIZE(new_bucket);

	while (oldsz > 0)
	{
		symbol = (stix_oop_char_t)old_bucket->slot[--oldsz];
		if ((stix_oop_t)symbol != stix->_nil)
		{
			STIX_ASSERT (STIX_CLASSOF(stix,symbol) == stix->_symbol);
			/*STIX_ASSERT (sym->size > 0);*/

			index = stix_hashchars(symbol->slot, STIX_OBJ_GET_SIZE(symbol)) % newsz;
			while (new_bucket->slot[index] != stix->_nil) 
				index = (index + 1) % newsz;
			new_bucket->slot[index] = (stix_oop_t)symbol;
		}
	}

	return new_bucket;
}

static stix_oop_t find_or_make_symbol (stix_t* stix, const stix_char_t* ptr, stix_oow_t len, int create)
{
	stix_oow_t index, tally;
	stix_oop_char_t symbol;

	STIX_ASSERT (len > 0);
	if (len <= 0) 
	{
		/* i don't allow an empty symbol name */
		stix->errnum = STIX_EINVAL;
		return STIX_NULL;
	}

	STIX_ASSERT (STIX_CLASSOF(stix,stix->symtab->bucket) == stix->_array);
	index = stix_hashchars(ptr, len) % STIX_OBJ_GET_SIZE(stix->symtab->bucket);

	/* find a matching symbol in the open-addressed symbol table */
	while (stix->symtab->bucket->slot[index] != stix->_nil) 
	{
		symbol = (stix_oop_char_t)stix->symtab->bucket->slot[index];
		STIX_ASSERT (STIX_CLASSOF(stix,symbol) == (stix_oop_t)stix->_symbol);

		if (len == STIX_OBJ_GET_SIZE(symbol) &&
		    stix_equalchars (ptr, symbol->slot, len))
		{
			return (stix_oop_t)symbol;
		}

		index = (index + 1) % STIX_OBJ_GET_SIZE(stix->symtab->bucket);
	}

	if (!create) 
	{
		/*
		stix->errnum = STIX_ENOENT;
		return STIX_NULL;
		*/
		return stix->_nil;
	}

	tally = STIX_OOP_TO_SMINT(stix->symtab->tally);
	if (tally + 1 >= STIX_OBJ_GET_SIZE(stix->symtab->bucket))
	{
		stix_oop_oop_t bucket;

		/* TODO: make the growth policy configurable instead of growing
			     it just before it gets full. The polcy can be grow it
			     if it's 70% full */

		/* Enlarge the symbol table before it gets full to
		 * make sure that it has at least one free slot left
		 * after having added a new symbol. this is to help
		 * traversal end at a _nil slot if no entry is found. */
		bucket = expand_bucket (stix, stix->symtab->bucket);
		if (!bucket) return STIX_NULL;

		stix->symtab->bucket = bucket;

		/* recalculate the index for the expanded bucket */
		index = stix_hashchars(ptr, len) % STIX_OBJ_GET_SIZE(stix->symtab->bucket);

		while (stix->symtab->bucket->slot[index] != stix->_nil) 
			index = (index + 1) % STIX_OBJ_GET_SIZE(stix->symtab->bucket);
	}

	/* create a new symbol since it isn't found in the symbol table */
	symbol = (stix_oop_char_t)stix_instantiate (stix, stix->_symbol, ptr, len);
	if (symbol)
	{
		stix->symtab->tally = STIX_OOP_FROM_SMINT(tally + 1);
		stix->symtab->bucket->slot[index] = (stix_oop_t)symbol;
	}

	return (stix_oop_t)symbol;
}

stix_oop_t stix_makesymbol (stix_t* stix, const stix_char_t* ptr, stix_oow_t len)
{
	return find_or_make_symbol (stix, ptr, len, 1);
}

stix_oop_t stix_findsymbol (stix_t* stix, const stix_char_t* ptr, stix_oow_t len)
{
	return find_or_make_symbol (stix, ptr, len, 0);
}
