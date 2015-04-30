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

static void* alloc_bytes (stix_t* stix, stix_size_t size)
{
	stix_uint8_t* ptr;

	ptr = stix_allocheapmem (stix, stix->curheap, size);
	if (!ptr && !(stix->option.trait & STIX_NOGC))
	{
		stix_gc (stix);
		ptr = stix_allocheapmem (stix, stix->curheap, size);
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
	hdr = alloc_bytes (stix, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
	if (!hdr) return STIX_NULL;

	hdr->flags = 0;
	hdr->type = STIX_OBJ_TYPE_OOP;
	hdr->extra = 0;
	hdr->unit = STIX_SIZEOF(stix_oop_t);  
	hdr->size = size;
	hdr->_class = stix->_nil;

	while (size > 0) hdr->slot[--size] = stix->_nil;

	return (stix_oop_t)hdr;
}

static stix_oop_t alloc_numeric_array (stix_t* stix, const void* ptr, stix_oow_t len, stix_obj_type_t type, stix_oow_t unit, int extra)
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
	hdr = alloc_bytes (stix, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
	if (!hdr) return STIX_NULL;

	hdr->flags = 0;
	hdr->type = type;
	hdr->extra = extra;
	hdr->unit = unit;
	hdr->size = len;
	hdr->_class = stix->_nil;

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

stix_oop_t stix_alloccharobj (stix_t* stix, const stix_char_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_CHAR, STIX_SIZEOF(stix_char_t), 1);
}

stix_oop_t stix_allocuint8obj (stix_t* stix, const stix_uint8_t* ptr, stix_oow_t len)
{

	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_UINT8, STIX_SIZEOF(stix_uint8_t), 0);
}

stix_oop_t stix_allocuint16obj (stix_t* stix, const stix_uint16_t* ptr, stix_oow_t len)
{
	return alloc_numeric_array (stix, ptr, len, STIX_OBJ_TYPE_UINT16, STIX_SIZEOF(stix_uint16_t), 0);
}

stix_oop_t stix_instantiate (stix_t* stix, stix_oop_t _class, const void* vptr, stix_oow_t vlen)
{
	stix_oop_t oop;
	stix_oow_t spec;
	stix_oow_t fixed;
	stix_obj_type_t vtype;

	STIX_ASSERT (stix->_nil != STIX_NULL);

	STIX_ASSERT (STIX_OOP_IS_POINTER(_class));
	/*STIX_ASSERT (STIX_CLASSOF(_class) == stix->_class); */

	STIX_ASSERT (STIX_OOP_IS_SMINT(((stix_oop_class_t)_class)->spec));
	spec = STIX_OOP_TO_SMINT(((stix_oop_class_t)_class)->spec);

	fixed = STIX_CLASS_SPEC_FIXED(spec); /* size of the fixed part */

	if (!STIX_CLASS_SPEC_ISVAR(spec)) 
	{
		/* named instance variables only. treat it as if it is a 
		 * variable-pointer class with no variable data */
		vtype = STIX_OBJ_TYPE_OOP;
		vlen = 0;
	}
	else
	{
		vtype = STIX_CLASS_SPEC_VTYPE(spec);
		if (vtype != STIX_OBJ_TYPE_OOP && fixed > 0)
		{
			/* a variable-non-pointer class can't have named instance variables */
			stix->errnum = STIX_EINVAL;
			return STIX_NULL;
		}
	}

	switch (vtype)
	{
		case STIX_OBJ_TYPE_OOP:
			/* class for a variable object. 
			 * both the fixed-sized part and the variable part are allowed. */
			oop = stix_allocoopobj(stix, fixed + vlen);
			if (!oop) return STIX_NULL;

			if (vlen > 0)
			{
				stix_oop_oop_t hdr = (stix_oop_oop_t)oop;
				STIX_MEMCPY (&hdr->slot[fixed], vptr, vlen * STIX_SIZEOF(stix_oop_t));
			}
			break;

		case STIX_OBJ_TYPE_CHAR:
			/* variable-char class can't have instance variables */
			STIX_ASSERT (fixed == 0);
			oop = stix_alloccharobj(stix, vptr, vlen);
			if (!oop) return STIX_NULL;
			break;

		case STIX_OBJ_TYPE_UINT8:
			/* variable-byte class can't have instance variables */
			STIX_ASSERT (fixed == 0);
			oop = stix_allocuint8obj(stix, vptr, vlen);
			if (!oop) return STIX_NULL;
			break;

		case STIX_OBJ_TYPE_UINT16:
			STIX_ASSERT (fixed == 0);
			oop = stix_allocuint16obj(stix, vptr, vlen);
			if (!oop) return STIX_NULL;
			break;

		default:
			stix->errnum = STIX_EINTERN;
			return STIX_NULL;
	}

	oop->_class = _class;
	return oop;
}
