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

void* stix_allocbytes (stix_t* stix, stix_size_t size)
{
	stix_uint8_t* ptr;

#if defined(STIX_DEBUG_GC_001)
	if (!(stix->option.trait & STIX_NOGC)) stix_gc (stix);
#endif

	ptr = stix_allocheapmem (stix, stix->curheap, size);
	if (!ptr && !(stix->option.trait & STIX_NOGC))
	{
		stix_gc (stix);
		ptr = stix_allocheapmem (stix, stix->curheap, size);

/* TODO: grow heap if ptr is still null. */
	}

	return ptr;
}

stix_oop_t stix_allocoopobj (stix_t* stix, stix_oow_t size)
{
	stix_oop_oop_t hdr;
	stix_oow_t nbytes, nbytes_aligned;

	nbytes = size * STIX_SIZEOF(stix_oop_t);

	/* this isn't really necessary since nbytes must be 
	 * aligned already. */
	nbytes_aligned = STIX_ALIGN(nbytes, STIX_SIZEOF(stix_oop_t)); 

	/* making the number of bytes to allocate a multiple of
	 * STIX_SIZEOF(stix_oop_t) will guarantee the starting address
	 * of the allocated space to be an even number. 
	 * see STIX_OOP_IS_NUMERIC() and STIX_OOP_IS_POINTER() */
	hdr = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
	if (!hdr) return STIX_NULL;

	hdr->_flags = STIX_OBJ_MAKE_FLAGS(STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 0, 0, 0);
	STIX_OBJ_SET_SIZE (hdr, size);
	STIX_OBJ_SET_CLASS (hdr, stix->_nil);

	while (size > 0) hdr->slot[--size] = stix->_nil;

	return (stix_oop_t)hdr;
}

#if defined(STIX_USE_OBJECT_TRAILER)
stix_oop_t stix_allocoopobjwithtrailer (stix_t* stix, stix_oow_t size, const stix_oob_t* bptr, stix_oow_t blen)
{
	stix_oop_oop_t hdr;
	stix_oow_t nbytes, nbytes_aligned;
	stix_oow_t i;

	/* +1 for the trailer size of the stix_oow_t type */
	nbytes = (size + 1) * STIX_SIZEOF(stix_oop_t) + blen;
	nbytes_aligned = STIX_ALIGN(nbytes, STIX_SIZEOF(stix_oop_t)); 

	hdr = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
	if (!hdr) return STIX_NULL;

	hdr->_flags = STIX_OBJ_MAKE_FLAGS(STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 0, 0, 1);
	STIX_OBJ_SET_SIZE (hdr, size);
	STIX_OBJ_SET_CLASS (hdr, stix->_nil);

	for (i = 0; i < size; i++) hdr->slot[i] = stix->_nil;

	/* [NOTE] this is not converted to a SmallInteger object */
	hdr->slot[size] = (stix_oop_t)blen; 

	if (bptr)
	{
		STIX_MEMCPY (&hdr->slot[size + 1], bptr, blen);
	}
	else
	{
		STIX_MEMSET (&hdr->slot[size + 1], 0, blen);
	}

	return (stix_oop_t)hdr;
}
#endif

static STIX_INLINE stix_oop_t alloc_numeric_array (stix_t* stix, const void* ptr, stix_oow_t len, stix_obj_type_t type, stix_oow_t unit, int extra)
{
	/* allocate a variable object */

	stix_oop_t hdr;
	stix_oow_t xbytes, nbytes, nbytes_aligned;

	xbytes = len * unit; 
	/* 'extra' indicates an extra unit to append at the end.
	 * it's useful to store a string with a terminating null */
	nbytes = extra? xbytes + len: xbytes; 
	nbytes_aligned = STIX_ALIGN(nbytes, STIX_SIZEOF(stix_oop_t));
/* TODO: check overflow in size calculation*/

	/* making the number of bytes to allocate a multiple of
	 * STIX_SIZEOF(stix_oop_t) will guarantee the starting address
	 * of the allocated space to be an even number. 
	 * see STIX_OOP_IS_NUMERIC() and STIX_OOP_IS_POINTER() */
	hdr = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
	if (!hdr) return STIX_NULL;

	hdr->_flags = STIX_OBJ_MAKE_FLAGS(type, unit, extra, 0, 0, 0);
	hdr->_size = len;
	STIX_OBJ_SET_SIZE (hdr, len);
	STIX_OBJ_SET_CLASS (hdr, stix->_nil);

	if (ptr)
	{
		/* copy data */
		STIX_MEMCPY (hdr + 1, ptr, xbytes);
		STIX_MEMSET ((stix_uint8_t*)(hdr + 1) + xbytes, 0, nbytes_aligned - xbytes);
	}
	else
	{
		/* initialize with zeros when the string pointer is not given */
		STIX_MEMSET ((hdr + 1), 0, nbytes_aligned);
	}

	return hdr;
}

stix_oop_t stix_alloccharobj (stix_t* stix, const stix_ooch_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_CHAR, STIX_SIZEOF(stix_ooch_t), 1);
}

stix_oop_t stix_allocbyteobj (stix_t* stix, const stix_oob_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_BYTE, STIX_SIZEOF(stix_oob_t), 0);
}

stix_oop_t stix_allochalfwordobj (stix_t* stix, const stix_oohw_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_HALFWORD, STIX_SIZEOF(stix_oohw_t), 0);
}

stix_oop_t stix_allocwordobj (stix_t* stix, const stix_oow_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_WORD, STIX_SIZEOF(stix_oow_t), 0);
}

stix_oop_t stix_instantiate (stix_t* stix, stix_oop_t _class, const void* vptr, stix_oow_t vlen)
{
	stix_oop_t oop;
	stix_oow_t spec;
	stix_oow_t named_instvar;
	stix_obj_type_t indexed_type;
	stix_oow_t tmp_count = 0;

	STIX_ASSERT (stix->_nil != STIX_NULL);

	STIX_ASSERT (STIX_OOP_IS_POINTER(_class));
	STIX_ASSERT (STIX_CLASSOF(stix, _class) == stix->_class);

	STIX_ASSERT (STIX_OOP_IS_SMOOI(((stix_oop_class_t)_class)->spec));
	spec = STIX_OOP_TO_SMOOI(((stix_oop_class_t)_class)->spec);

	named_instvar = STIX_CLASS_SPEC_NAMED_INSTVAR(spec); /* size of the named_instvar part */

	if (STIX_CLASS_SPEC_IS_INDEXED(spec)) 
	{
		indexed_type = STIX_CLASS_SPEC_INDEXED_TYPE(spec);

		if (indexed_type == STIX_OBJ_TYPE_OOP)
		{
			if (named_instvar > STIX_MAX_NAMED_INSTVARS ||
			    vlen > STIX_MAX_INDEXED_INSTVARS(named_instvar))
			{
				goto einval;
			}
		}
		else
		{
			/* a non-pointer indexed class can't have named instance variables */
			if (named_instvar > 0) goto einval;
		}
	}
	else
	{
		/* named instance variables only. treat it as if it is an
		 * indexable class with no variable data */
		indexed_type = STIX_OBJ_TYPE_OOP;
		vlen = 0;

		if (named_instvar > STIX_MAX_NAMED_INSTVARS) goto einval;
	}

	stix_pushtmp (stix, &_class); tmp_count++;

	switch (indexed_type)
	{
		case STIX_OBJ_TYPE_OOP:
			/* both the fixed part(named instance variables) and 
			 * the variable part(indexed instance variables) are allowed. */
			oop = stix_allocoopobj (stix, named_instvar + vlen);

			STIX_ASSERT (vptr == STIX_NULL);
			/*
			This function is not GC-safe. so i don't want to initialize
			the payload of a pointer object. The caller can call this
			function and initialize payloads then.
			if (oop && vptr && vlen > 0)
			{
				stix_oop_oop_t hdr = (stix_oop_oop_t)oop;
				STIX_MEMCPY (&hdr->slot[named_instvar], vptr, vlen * STIX_SIZEOF(stix_oop_t));
			}

			For the above code to work, it should protect the elements of 
			the vptr array with stix_pushtmp(). So it might be better 
			to disallow a non-NULL vptr when indexed_type is OOP. See
			the assertion above this comment block.
			*/
			break;

		case STIX_OBJ_TYPE_CHAR:
			oop = stix_alloccharobj (stix, vptr, vlen);
			break;

		case STIX_OBJ_TYPE_BYTE:
			oop = stix_allocbyteobj (stix, vptr, vlen);
			break;

		case STIX_OBJ_TYPE_HALFWORD:
			oop = stix_allochalfwordobj (stix, vptr, vlen);
			break;

		case STIX_OBJ_TYPE_WORD:
			oop = stix_allocwordobj (stix, vptr, vlen);
			break;

		default:
			stix->errnum = STIX_EINTERN;
			oop = STIX_NULL;
			break;
	}

	if (oop) STIX_OBJ_SET_CLASS (oop, _class);
	stix_poptmps (stix, tmp_count);
	return oop;

einval:
	STIX_ASSERT (tmp_count <= 0);
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;
}

#if defined(STIX_USE_OBJECT_TRAILER)

stix_oop_t stix_instantiatewithtrailer (stix_t* stix, stix_oop_t _class, stix_oow_t vlen, const stix_oob_t* tptr, stix_oow_t tlen)
{
	stix_oop_t oop;
	stix_oow_t spec;
	stix_oow_t named_instvar;
	stix_obj_type_t indexed_type;
	stix_oow_t tmp_count = 0;

	STIX_ASSERT (stix->_nil != STIX_NULL);

	STIX_ASSERT (STIX_OOP_IS_POINTER(_class));
	STIX_ASSERT (STIX_CLASSOF(stix, _class) == stix->_class);

	STIX_ASSERT (STIX_OOP_IS_SMOOI(((stix_oop_class_t)_class)->spec));
	spec = STIX_OOP_TO_SMOOI(((stix_oop_class_t)_class)->spec);

	named_instvar = STIX_CLASS_SPEC_NAMED_INSTVAR(spec); /* size of the named_instvar part */

	if (STIX_CLASS_SPEC_IS_INDEXED(spec)) 
	{
		indexed_type = STIX_CLASS_SPEC_INDEXED_TYPE(spec);

		if (indexed_type == STIX_OBJ_TYPE_OOP)
		{
			if (named_instvar > STIX_MAX_NAMED_INSTVARS ||
			    vlen > STIX_MAX_INDEXED_INSTVARS(named_instvar))
			{
				goto einval;
			}
		}
		else
		{
			/* not a class for an OOP object */
			goto einval;
		}
	}
	else
	{
		/* named instance variables only. treat it as if it is an
		 * indexable class with no variable data */
		indexed_type = STIX_OBJ_TYPE_OOP;
		vlen = 0;

		if (named_instvar > STIX_MAX_NAMED_INSTVARS) goto einval;
	}

	stix_pushtmp (stix, &_class); tmp_count++;

	switch (indexed_type)
	{
		case STIX_OBJ_TYPE_OOP:
			oop = stix_allocoopobjwithtrailer(stix, named_instvar + vlen, tptr, tlen);
			break;

		default:
			stix->errnum = STIX_EINTERN;
			oop = STIX_NULL;
			break;
	}

	if (oop) STIX_OBJ_SET_CLASS (oop, _class);
	stix_poptmps (stix, tmp_count);
	return oop;

einval:
	STIX_ASSERT (tmp_count <= 0);
	stix->errnum = STIX_EINVAL;
	return STIX_NULL;

}
#endif

