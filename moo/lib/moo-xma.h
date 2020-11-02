/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _MOO_XMA_H_
#define _MOO_XMA_H_

/** @file
 * This file defines an extravagant memory allocator. Why? It may be so.
 * The memory allocator allows you to maintain memory blocks from a
 * larger memory chunk allocated with an outer memory allocator.
 * Typically, an outer memory allocator is a standard memory allocator
 * like malloc(). You can isolate memory blocks into a particular chunk.
 *
 * See the example below. Note it omits error handling.
 *
 * @code
 * #include <qse/cmn/xma.h>
 * #include <qse/cmn/stdio.h>
 * int main ()
 * {
 *   moo_xma_t* xma;
 *   void* ptr1, * ptr2;
 *
 *   // create a new memory allocator obtaining a 100K byte zone 
 *   // with the default memory allocator
 *   xma = moo_xma_open(MOO_NULL, 0, 100000L); 
 *
 *   ptr1 = moo_xma_alloc(xma, 5000); // allocate a 5K block from the zone
 *   ptr2 = moo_xma_alloc(xma, 1000); // allocate a 1K block from the zone
 *   ptr1 = moo_xma_realloc(xma, ptr1, 6000); // resize the 5K block to 6K.
 *
 *   moo_xma_dump (xma, moo_fprintf, MOO_STDOUT); // dump memory blocks 
 *
 *   // the following two lines are not actually needed as the allocator
 *   // is closed after them.
 *   moo_xma_free (xma, ptr2); // dispose of the 1K block
 *   moo_xma_free (xma, ptr1); // dispose of the 6K block
 *
 *   moo_xma_close (xma); //  destroy the memory allocator
 *   return 0;
 * }
 * @endcode
 */
#include <moo-cmn.h>

#ifdef MOO_BUILD_DEBUG
#	define MOO_XMA_ENABLE_STAT
#endif

/** @struct moo_xma_t
 * The moo_xma_t type defines a simple memory allocator over a memory zone.
 * It can obtain a relatively large zone of memory and manage it.
 */
typedef struct moo_xma_t moo_xma_t;

/**
 * The moo_xma_fblk_t type defines a memory block allocated.
 */
typedef struct moo_xma_fblk_t moo_xma_fblk_t;
typedef struct moo_xma_mblk_t moo_xma_mblk_t;

#define MOO_XMA_FIXED 32
#define MOO_XMA_SIZE_BITS ((MOO_SIZEOF_OOW_T*8)-1)

struct moo_xma_t
{
	moo_mmgr_t* _mmgr;

	moo_uint8_t* start; /* zone beginning */
	moo_uint8_t* end; /* zone end */
	int          internal;

	/** pointer array to free memory blocks */
	moo_xma_fblk_t* xfree[MOO_XMA_FIXED + MOO_XMA_SIZE_BITS + 1]; 

	/** pre-computed value for fast xfree index calculation */
	moo_oow_t     bdec;

#ifdef MOO_XMA_ENABLE_STAT
	struct
	{
		moo_oow_t total;
		moo_oow_t alloc;
		moo_oow_t avail;
		moo_oow_t nused;
		moo_oow_t nfree;
	} stat;
#endif
};

/**
 * The moo_xma_dumper_t type defines a printf-like output function
 * for moo_xma_dump().
 */
typedef int (*moo_xma_dumper_t) (
	void*            ctx,
	const moo_bch_t* fmt,
	...
);

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The moo_xma_open() function creates a memory allocator. It obtains a memory
 * zone of the @a zonesize bytes with the memory manager @a mmgr. It also makes
 * available the extension area of the @a xtnsize bytes that you can get the
 * pointer to with moo_xma_getxtn().
 *
 * @return pointer to a memory allocator on success, #MOO_NULL on failure
 */
MOO_EXPORT moo_xma_t* moo_xma_open (
	moo_mmgr_t* mmgr,    /**< memory manager */
	moo_oow_t   xtnsize, /**< extension size in bytes */
	void*       zoneptr,
	moo_oow_t   zonesize /**< zone size in bytes */
);

/**
 * The moo_xma_close() function destroys a memory allocator. It also frees
 * the memory zone obtained, which invalidates the memory blocks within 
 * the zone. Call this function to destroy a memory allocator created with
 * moo_xma_open().
 */
MOO_EXPORT void moo_xma_close (
	moo_xma_t* xma /**< memory allocator */
);

#if defined(MOO_HAVE_INLINE)
static MOO_INLINE moo_mmgr_t* moo_xma_getmmgr (moo_xma_t* xma) { return xma->_mmgr; }
#else
#	define moo_xma_getmmgr(xma) (((moo_xma_t*)(xma))->_mmgr)
#endif

#if defined(MOO_HAVE_INLINE)
static MOO_INLINE void* moo_xma_getxtn (moo_xma_t* xma) { return (void*)(xma + 1); }
#else
#define moo_xma_getxtn(xma) ((void*)((moo_xma_t*)(xma) + 1))
#endif

/**
 * The moo_xma_init() initializes a memory allocator. If you have the moo_xma_t
 * structure statically declared or already allocated, you may pass the pointer
 * to this function instead of calling moo_xma_open(). It obtains a memory zone
 * of @a zonesize bytes with the memory manager @a mmgr. Unlike moo_xma_open(),
 * it does not accept the extension size, thus not creating an extention area.
 * @return 0 on success, -1 on failure
 */
MOO_EXPORT int moo_xma_init (
	moo_xma_t*  xma,     /**< memory allocator */
	moo_mmgr_t* mmgr,    /**< memory manager */
	void*       zoneptr,  /**< pointer to memory zone. if #MOO_NULL, a zone is auto-created */
	moo_oow_t   zonesize /**< zone size in bytes */
);

/**
 * The moo_xma_fini() function finalizes a memory allocator. Call this 
 * function to finalize a memory allocator initialized with moo_xma_init().
 */
MOO_EXPORT void moo_xma_fini (
	moo_xma_t* xma /**< memory allocator */
);

/**
 * The moo_xma_alloc() function allocates @a size bytes.
 * @return pointer to a memory block on success, #MOO_NULL on failure
 */
MOO_EXPORT void* moo_xma_alloc (
	moo_xma_t* xma, /**< memory allocator */
	moo_oow_t  size /**< size in bytes */
);

MOO_EXPORT void* moo_xma_calloc (
	moo_xma_t* xma,
	moo_oow_t  size
);

/**
 * The moo_xma_alloc() function resizes the memory block @a b to @a size bytes.
 * @return pointer to a resized memory block on success, #MOO_NULL on failure
 */
MOO_EXPORT void* moo_xma_realloc (
	moo_xma_t* xma,  /**< memory allocator */
	void*      b,    /**< memory block */
	moo_oow_t  size  /**< new size in bytes */
);

/**
 * The moo_xma_alloc() function frees the memory block @a b.
 */
MOO_EXPORT void moo_xma_free (
	moo_xma_t* xma, /**< memory allocator */
	void*      b    /**< memory block */
);

/**
 * The moo_xma_dump() function dumps the contents of the memory zone
 * with the output function @a dumper provided. The debug build shows
 * more statistical counters.
 */
MOO_EXPORT void moo_xma_dump (
	moo_xma_t*       xma,    /**< memory allocator */
	moo_xma_dumper_t dumper, /**< output function */
	void*            ctx     /**< first parameter to output function */
);

#if defined(__cplusplus)
}
#endif

#endif
