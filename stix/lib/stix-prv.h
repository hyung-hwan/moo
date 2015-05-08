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

#ifndef _STIX_PRV_H_
#define _STIX_PRV_H_

#include "stix.h"

/* this is useful for debugging. stix_gc() can be called 
 * while stix has not been fully initialized when this is defined*/
#define STIX_SUPPORT_GC_DURING_IGNITION
#define STIX_DEBUG_GC_1


#include <stdio.h> /* TODO: delete these header inclusion lines */
#include <string.h>
#include <assert.h>

#define STIX_MEMSET(dst,src,size) memset(dst,src,size)
#define STIX_MEMCPY(dst,src,size) memcpy(dst,src,size)
#define STIX_ASSERT(x)            assert(x)

#define STIX_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))


/* ========================================================================= */
/* CLASS SPEC ENCODING                                                       */
/* ========================================================================= */

/*
 * The spec field of a class object encodes the number of the fixed part
 * and the type of the indexed part. The fixed part is the number of
 * named instance variables. If the spec of a class is indexed, the object
 * of the class can be i nstantiated with the size of the indexed part.
 *
 * For example, on a platform where sizeof(stix_oow_t) is 4, 
 * the layout of the spec field of a class as an OOP value looks like this:
 * 
 *  31                           10 9   8 7 6 5 4 3  2           1 0 
 * |number of named instance variables|indexed-type|indexability|oop-tag|
 *
 * the number of named instance variables is stored in high 23 bits.
 * the indexed type takes up bit 3 to bit 8. And the indexability is
 * stored in bit 2. 
 *
 * The maximum number of named(fixed) instance variables for a class is:
 *     2 ^ ((BITS-IN-OOW - STIX_OOP_TAG_BITS) - STIX_OBJ_TYPE_BITS - 1) - 1
 *
 * STIX_OOP_TAG_BITS are decremented from the number of bits in OOW because
 * the spec field is always encoded as a small integer.
 *
 * The number of named instance variables can be greater than 0 if the
 * class spec is not indexed or if it's a pointer indexed class
 * (indexed_type == STIX_OBJ_TYPE_OOP)
 * 
 * indexed_type is one of the #stix_obj_type_t enumerators.
 */

/*
 * The STIX_CLASS_SPEC_MAKE() macro creates a class spec value.
 *  _class->spec = STIX_OOP_FROM_SMINT(STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
 */
#define STIX_CLASS_SPEC_MAKE(named_instvar,is_indexed,indexed_type) ( \
	(((stix_oow_t)(named_instvar)) << (STIX_OBJ_FLAGS_TYPE_BITS + 1)) |  \
	(((stix_oow_t)(indexed_type)) << 1) | (((stix_oow_t)is_indexed) & 1) )

/* what is the number of named instance variables? 
 *  STIX_CLASS_SPEC_NAMED_INSTVAR(STIX_OOP_TO_SMINT(_class->spec))
 */
#define STIX_CLASS_SPEC_NAMED_INSTVAR(spec) \
	(((stix_oow_t)(spec)) >> (STIX_OBJ_FLAGS_TYPE_BITS + 1))

/* is it a user-indexable class? 
 * all objects can be indexed with basicAt:.
 * this indicates if an object can be instantiated with a dynamic size
 * (new: size) and and can be indexed with at:.
 */
#define STIX_CLASS_SPEC_IS_INDEXED(spec) (((stix_oow_t)(spec)) & 1)

/* if so, what is the indexing type? character? pointer? etc? */
#define STIX_CLASS_SPEC_INDEXED_TYPE(spec) \
	((((stix_oow_t)(spec)) >> 1) & STIX_LBMASK(stix_oow_t, STIX_OBJ_FLAGS_TYPE_BITS))

/* What is the maximum number of named instance variables?
 *   2 ^ ((BITS-IN-OOW - STIX_OOP_TAG_BITS) - STIX_OBJ_TYPE_BITS - 1) - 1 
 * This limit is set because the number must be encoded into the spec field
 * of the class with limited number of bits assigned to the number of
 * named instance variables.
 */
#define STIX_MAX_NAMED_INSTVARS \
	STIX_BITS_MAX(stix_oow_t, STIX_OOW_BITS - STIX_OOP_TAG_BITS - (STIX_OBJ_FLAGS_TYPE_BITS + 1))

/* Given the number of named instance variables, what is the maximum number 
 * of indexed instance variables? The number of indexed instance variables
 * is not stored in the spec field of the class. It only affects the actual
 * size of an object(obj->_size) selectively combined with the number of 
 * named instance variables. So it's the maximum value of obj->_size minus
 * the number of named instance variables.
 */
#define STIX_MAX_INDEXED_INSTVARS(named_instvar) ((~(stix_oow_t)0) - named_instvar)

#if defined(__cplusplus)
extern "C" {
#endif

/* ========================================================================= */
/* heap.c                                                                    */
/* ========================================================================= */

/**
 * The stix_makeheap() function creates a new heap of the \a size bytes.
 *
 * \return heap pointer on success and #STIX_NULL on failure.
 */
stix_heap_t* stix_makeheap (
	stix_t*     stix, 
	stix_size_t size
);

/**
 * The stix_killheap() function destroys the heap pointed to by \a heap.
 */
void stix_killheap (
	stix_t*      stix, 
	stix_heap_t* heap
);

/**
 * The stix_allocheapmem() function allocates \a size bytes in the heap pointed
 * to by \a heap.
 *
 * \return memory pointer on success and #STIX_NULL on failure.
 */
void* stix_allocheapmem (
	stix_t*      stix,
	stix_heap_t* heap,
	stix_size_t  size
);


/* ========================================================================= */
/* stix.c                                                                    */
/* ========================================================================= */
stix_oow_t stix_hashbytes (
	const stix_uint8_t* ptr,
	stix_oow_t          len
);

stix_oow_t stix_hashchars (
	const stix_char_t* ptr,
	stix_oow_t         len
);

int stix_equalchars (
	const stix_char_t* str1,
	const stix_char_t* str2,
	stix_oow_t         len
);

/* ========================================================================= */
/* obj.c                                                                     */
/* ========================================================================= */
void* stix_allocbytes (
	stix_t*     stix,
	stix_size_t size
);

/**
 * The stix_allocoopobj() function allocates a raw object composed of \a size
 * pointer fields excluding the header.
 */
stix_oop_t stix_allocoopobj (
	stix_t*    stix,
	stix_oow_t size
);

stix_oop_t stix_alloccharobj (
	stix_t*            stix,
	const stix_char_t* ptr,
	stix_oow_t         len
);

stix_oop_t stix_allocuint8obj (
	stix_t*             stix,
	const stix_uint8_t* ptr,
	stix_oow_t          len
);

stix_oop_t stix_allocuint16obj (
	stix_t*              stix,
	const stix_uint16_t* ptr,
	stix_oow_t           len
);

/* ========================================================================= */
/* sym.c                                                                     */
/* ========================================================================= */
stix_oop_t stix_makesymbol (
	stix_t*            stix,
	const stix_char_t* ptr,
	stix_oow_t         len
);

stix_oop_t stix_findsymbol (
	stix_t*            stix,
	const stix_char_t* ptr,
	stix_oow_t         len
);

/* ========================================================================= */
/* dic.c                                                                     */
/* ========================================================================= */
stix_oop_t stix_putatsysdic (
	stix_t*     stix,
	stix_oop_t  key,
	stix_oop_t  value
);

stix_oop_t stix_getatsysdic (
	stix_t*     stix,
	stix_oop_t  key
);

#if defined(__cplusplus)
}
#endif


#endif
