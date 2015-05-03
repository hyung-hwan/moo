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

#include <stdio.h> /* TODO: delete these header inclusion lines */
#include <string.h>
#include <assert.h>

#define STIX_MEMSET(dst,src,size) memset(dst,src,size)
#define STIX_MEMCPY(dst,src,size) memcpy(dst,src,size)
#define STIX_ASSERT(x)            assert(x)

#define STIX_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))



/* ========================================================================= */
/* CLASS                                                                     */
/* ========================================================================= */

/* The stix_class_t type defines the internal structure of a class object. */
struct stix_class_t
{
	STIX_OBJ_HEADER;
	stix_oow_t      spec;
	stix_oop_oop_t  methods;
	stix_oop_oop_t  superclass;
	stix_oop_char_t name;
	stix_oop_char_t instvars;
	stix_oop_char_t classvars;

	/* variable part afterwards */
};

typedef struct stix_class_t stix_class_t;
typedef struct stix_class_t* stix_oop_class_t;

/*
 * The spec field of a class object encodes the number of the fixed part
 * and the type of the variable part. The fixed part is the number of
 * instance variables. The variable part can be specified when the object
 * is instantiated if it is variadic.
 *
 * indexed_type is one of the #stix_obj_type_t enumerators.
 *
 * The maximum number of named(fixed) instance variables for a class is:
 *     2 ^ (BITS-IN-OOW - STIX_OBJ_TYPE_BITS - 1)
 *
 * The number of named instance variables can be greater than 0 if the
 * class spec is not indexed or if it's a pointer indexed class
 * (indexed_type == STIX_OBJ_TYPE_OOP)
 */
#define STIX_CLASS_SPEC_MAKE(named_instvar,is_indexed,indexed_type) ( \
	(((stix_oow_t)(named_instvar)) << (STIX_OBJ_TYPE_BITS + 1)) |  \
	(((stix_oow_t)(indexed_type)) << 1) | (((stix_oow_t)is_indexed) & 1) )

/* what is the number of named instance variables? */
#define STIX_CLASS_SPEC_NAMED_INSTVAR(spec) ((spec) >> STIX_OBJ_FLAGS_TYPE_BITS)

/* is it a user-indexable class? 
 * all objects can be indexed with basicAt:.
 * this indicates if an object can be instantiated with a dynamic size
 * (new: size) and and can be indexed with at:.
 */
#define STIX_CLASS_SPEC_IS_INDEXED(spec) ((spec) & 1)

/* if so, what is the indexing type? character? pointer? etc? */
#define STIX_CLASS_SPEC_INDEXED_TYPE(spec) ((spec) & (STIX_LBMASK(stix_oow_t, STIX_OBJ_FLAGS_TYPE_BITS) << 1))


/* 2 ^ (BITS-IN-OOW - STIX_OBJ_TYPE_BITS - 1) */
#define STIX_MAX_NAMED_INSTVAR ((stix_oow_t)2 << (STIX_SIZEOF(stix_oow_t) * 8 - (STIX_OBJ_FLAGS_TYPE_BITS + 1) - 1))
/* Given the number of named instance variables, what is the maximum number 
 * of indexed slots? */
#define STIX_MAX_INDEXED_COUNT(named_instvar) ((~(stix_oow_t)0) - named_instvar)

/* VM-level macros */
#define STIX_CLASSOF(stix,oop) \
	(STIX_OOP_IS_NUMERIC(oop)? \
		((stix_oop_t)(stix)->cc.numeric[((stix_oow_t)oop&3)-1]): \
		(oop)->_class)

/*
#define STIX_BYTESOF(stix,oop) \
	(STIX_OOP_IS_NUMERIC(oop)? \
		(STIX_SIZEOF(stix_oow_t)): \
		(STIX_SIZEOF(stix_obj_t) + STIX_ALIGN(((oop)->size + (oop)->extra) * (oop)->unit), STIX_SIZEOF(stix_oop_t)) \
	)
*/

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
/* obj.c                                                                     */
/* ========================================================================= */
void* stix_allocbytes (
	stix_t*     stix,
	stix_size_t size
);

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


#if defined(__cplusplus)
}
#endif


#endif
