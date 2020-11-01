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

#include <moo-xma.h>
#include "moo-prv.h"
#include <assert.h> /* TODO: replace assert() with builtin substition */


#define ALIGN MOO_SIZEOF(moo_oow_t) /* this must be a power of 2 */
#define HDRSIZE MOO_SIZEOF(moo_xma_mblk_t)
/*#define MINBLKLEN (HDRSIZE + ALIGN + ALIGN)*/
#define MINBLKLEN MOO_SIZEOF(moo_xma_fblk_t) /* need space for the free links when the block is freeed */
#define MINCHUNKSIZE (ALIGN + ALIGN) /* as large as the free links in moo_xma_fblk_t */

#define SYS_TO_USR(_) (((moo_xma_mblk_t*)_) + 1)
#define USR_TO_SYS(_) (((moo_xma_mblk_t*)_) - 1)

/*
 * the xfree array is divided into three region
 * 0 ....................... FIXED ......................... XFIMAX-1 ... XFIMAX
 * |  small fixed-size chains |     large  chains                | huge chain |
 */
#define FIXED MOO_XMA_FIXED
#define XFIMAX(xma) (MOO_COUNTOF(xma->xfree)-1)


#define next_mblk(b) ((moo_xma_mblk_t*)((moo_uint8_t*)((moo_xma_mblk_t*)(b) + 1) + ((moo_xma_mblk_t*)(b))->size))
#define prev_mblk(b) ((moo_xma_mblk_t*)((moo_uint8_t*)b - ((moo_xma_mblk_t*)(b))->prev_size) - HDRSIZE)


struct moo_xma_mblk_t
{
	moo_oow_t prev_size;
	moo_oow_t avail: 1;
	moo_oow_t size: MOO_XMA_SIZE_BITS;/**< block size */
};

struct moo_xma_fblk_t 
{
	moo_oow_t prev_size;
	moo_oow_t avail: 1;
	moo_oow_t size: MOO_XMA_SIZE_BITS;/**< block size */

	struct
	{
		moo_xma_fblk_t* prev; /**< link to the previous free block */
		moo_xma_fblk_t* next; /**< link to the next free block */
	} f;
};

static MOO_INLINE moo_oow_t szlog2 (moo_oow_t n) 
{
	/*
	 * 2**x = n;
	 * x = log2(n);
	 * -------------------------------------------
	 * 	unsigned int x = 0;
	 * 	while((n >> x) > 1) ++x;
	 * 	return x;
	 */

#define BITS (MOO_SIZEOF_OOW_T * 8)
	int x = BITS - 1;

#if MOO_SIZEOF_OOW_T >= 128
#	error moo_oow_t too large. unsupported platform
#endif

#if MOO_SIZEOF_OOW_T >= 64
	if ((n & (~(moo_oow_t)0 << (BITS-128))) == 0) { x -= 256; n <<= 256; }
#endif
#if MOO_SIZEOF_OOW_T >= 32
	if ((n & (~(moo_oow_t)0 << (BITS-128))) == 0) { x -= 128; n <<= 128; }
#endif
#if MOO_SIZEOF_OOW_T >= 16
	if ((n & (~(moo_oow_t)0 << (BITS-64))) == 0) { x -= 64; n <<= 64; }
#endif
#if MOO_SIZEOF_OOW_T >= 8
	if ((n & (~(moo_oow_t)0 << (BITS-32))) == 0) { x -= 32; n <<= 32; }
#endif
#if MOO_SIZEOF_OOW_T >= 4 
	if ((n & (~(moo_oow_t)0 << (BITS-16))) == 0) { x -= 16; n <<= 16; }
#endif
#if MOO_SIZEOF_OOW_T >= 2
	if ((n & (~(moo_oow_t)0 << (BITS-8))) == 0) { x -= 8; n <<= 8; }
#endif
#if MOO_SIZEOF_OOW_T >= 1
	if ((n & (~(moo_oow_t)0 << (BITS-4))) == 0) { x -= 4; n <<= 4; }
#endif
	if ((n & (~(moo_oow_t)0 << (BITS-2))) == 0) { x -= 2; n <<= 2; }
	if ((n & (~(moo_oow_t)0 << (BITS-1))) == 0) { x -= 1; }

	return x;
#undef BITS
}

static MOO_INLINE moo_oow_t getxfi (moo_xma_t* xma, moo_oow_t size) 
{
	moo_oow_t xfi = ((size) / ALIGN) - 1;
	if (xfi >= FIXED) xfi = szlog2(size) - (xma)->bdec + FIXED;
	if (xfi > XFIMAX(xma)) xfi = XFIMAX(xma);
	return xfi;
}

moo_xma_t* moo_xma_open (moo_mmgr_t* mmgr, moo_oow_t xtnsize, void* zoneptr, moo_oow_t zonesize)
{
	moo_xma_t* xma;

	xma = (moo_xma_t*)MOO_MMGR_ALLOC(mmgr, MOO_SIZEOF(*xma) + xtnsize);
	if (MOO_UNLIKELY(!xma)) return MOO_NULL;

	if (moo_xma_init(xma, mmgr, zoneptr, zonesize) <= -1)
	{
		MOO_MMGR_FREE (mmgr, xma);
		return MOO_NULL;
	}

	MOO_MEMSET (xma + 1, 0, xtnsize);
	return xma;
}

void moo_xma_close (moo_xma_t* xma)
{
	moo_xma_fini (xma);
	MOO_MMGR_FREE (xma->_mmgr, xma);
}

int moo_xma_init (moo_xma_t* xma, moo_mmgr_t* mmgr, void* zoneptr, moo_oow_t zonesize)
{
	moo_xma_fblk_t* free;
	moo_oow_t xfi;

	if (!zoneptr)
	{
		/* round 'zonesize' to be the multiples of ALIGN */
		zonesize = MOO_ALIGN_POW2(zonesize, ALIGN);

		/* adjust 'zonesize' to be large enough to hold a single smallest block */
		if (zonesize < MINBLKLEN) zonesize = MINBLKLEN;

		zoneptr = MOO_MMGR_ALLOC(mmgr, zonesize);
		if (MOO_UNLIKELY(!zoneptr)) return -1;
	}
	else
	{
		xma->external = 1;
	}

	free = (moo_xma_fblk_t*)zoneptr;

	/* initialize the header part of the free chunk. the entire zone is a single free block */
	free->prev_size = 0;
	free->avail = 1;
	free->size = zonesize - HDRSIZE; /* size excluding the block header */
	free->f.prev = MOO_NULL;
	free->f.next = MOO_NULL;

	MOO_MEMSET (xma, 0, MOO_SIZEOF(*xma));
	xma->_mmgr = mmgr;
	xma->bdec = szlog2(FIXED * ALIGN); /* precalculate the decrement value */

	/* at this point, the 'free' chunk is a only block available */

	/* get the free block index */
	xfi = getxfi(xma, free->size);
	/* locate it into an apporopriate slot */
	xma->xfree[xfi] = free; 
	/* let it be the head, which is natural with only a block */
	xma->start = (moo_uint8_t*)free;
	xma->end = xma->start + zonesize;
	

	/* initialize some statistical variables */
#if defined(MOO_XMA_ENABLE_STAT)
	xma->stat.total = zonesize;
	xma->stat.alloc = 0;
	xma->stat.avail = zonesize - HDRSIZE;
	xma->stat.nfree = 1;
	xma->stat.nused = 0;
#endif
	
	return 0;
}

void moo_xma_fini (moo_xma_t* xma)
{
	/* the head must point to the free chunk allocated in init().
	 * let's deallocate it */
	if (!xma->external) MOO_MMGR_FREE (xma->_mmgr, xma->start);
	xma->start = MOO_NULL;
	xma->end = MOO_NULL;
}

static MOO_INLINE void attach_to_freelist (moo_xma_t* xma, moo_xma_fblk_t* b)
{
	/* 
	 * attach a block to a free list 
	 */

	/* get the free list index for the block size */
	moo_oow_t xfi = getxfi(xma, b->size); 

	/* let it be the head of the free list doubly-linked */
	b->f.prev = MOO_NULL; 
	b->f.next = xma->xfree[xfi];
	if (xma->xfree[xfi]) xma->xfree[xfi]->f.prev = b;
	xma->xfree[xfi] = b;
}

static MOO_INLINE void detach_from_freelist (moo_xma_t* xma, moo_xma_fblk_t* b)
{
	/*
 	 * detach a block from a free list 
 	 */
	moo_xma_fblk_t* p, * n;

	/* alias the previous and the next with short variable names */
	p = b->f.prev;
	n = b->f.next;

	if (p)
	{
		/* the previous item exists. let its 'next' pointer point to 
		 * the block's next item. */
		p->f.next = n;
	}
	else 
	{
		/* the previous item does not exist. the block is the first
 		 * item in the free list. */

		moo_oow_t xfi = getxfi(xma, b->size);
		assert (b == xma->xfree[xfi]);
		/* let's update the free list head */
		xma->xfree[xfi] = n;
	}

	/* let the 'prev' pointer of the block's next item point to the 
	 * block's previous item */
	if (n) n->f.prev = p; 
}

static moo_xma_fblk_t* alloc_from_freelist (moo_xma_t* xma, moo_oow_t xfi, moo_oow_t size)
{
	moo_xma_fblk_t* free;

	for (free = xma->xfree[xfi]; free; free = free->f.next)
	{
		if (free->size >= size)
		{
			moo_oow_t rem;

			detach_from_freelist (xma, free);

			rem = free->size - size;
			if (rem >= MINBLKLEN)
			{
				moo_xma_fblk_t* tmp;

				/* the remaining part is large enough to hold 
				 * another block. let's split it 
				 */

				/* shrink the size of the 'free' block */
				free->size = size;

				/* let 'tmp' point to the remaining part */
				tmp = (moo_xma_fblk_t*)next_mblk(free); /* get the next adjacent block */

				/* initialize some fields */
				tmp->avail = 1;
				tmp->size = rem - HDRSIZE;
				tmp->prev_size = free->size;

				/* add the remaining part to the free list */
				attach_to_freelist (xma, tmp);

#if defined(MOO_XMA_ENABLE_STAT)
				xma->stat.avail -= HDRSIZE;
#endif
			}
#if defined(MOO_XMA_ENABLE_STAT)
			else
			{
				/* decrement the number of free blocks as the current
				 * block is allocated as a whole without being split */
				xma->stat.nfree--;
			}
#endif

			free->avail = 0;
			/*
			free->f.next = MOO_NULL;
			free->f.prev = MOO_NULL;
			*/

#if defined(MOO_XMA_ENABLE_STAT)
			xma->stat.nused++;
			xma->stat.alloc += free->size;
			xma->stat.avail -= free->size;
#endif
			return free;
		}
	}

	return MOO_NULL;
}

void* moo_xma_alloc (moo_xma_t* xma, moo_oow_t size)
{
	moo_xma_fblk_t* free;
	moo_oow_t xfi;

	if (size <= 0) size = 1;

	/* round up 'size' to the multiples of ALIGN */
	if (size < MINCHUNKSIZE) size = MINCHUNKSIZE;
	size = MOO_ALIGN_POW2(size, ALIGN);

	assert (size >= ALIGN);
	xfi = getxfi(xma, size);

	/*if (xfi < XFIMAX(xma) && xma->xfree[xfi])*/
	if (xfi < FIXED && xma->xfree[xfi])
	{
		/* try the best fit */
		free = xma->xfree[xfi];

		assert (free->avail != 0);
		assert (free->size == size);

		detach_from_freelist (xma, free);
		free->avail = 0;

#if defined(MOO_XMA_ENABLE_STAT)
		xma->stat.nfree--;
		xma->stat.nused++;
		xma->stat.alloc += free->size;
		xma->stat.avail -= free->size;
#endif
	}
	else if (xfi == XFIMAX(xma))
	{
		/* huge block */
		free = alloc_from_freelist(xma, XFIMAX(xma), size);
		if (!free) return MOO_NULL;
	}
	else
	{
		if (xfi >= FIXED)
		{
			/* get the block from its own large chain */
			free = alloc_from_freelist(xma, xfi, size);
			if (!free)
			{
				/* borrow a large block from the huge block chain */
				free = alloc_from_freelist(xma, XFIMAX(xma), size);
			}
		}
		else
		{
			/* borrow a small block from the huge block chain */
			free = alloc_from_freelist(xma, XFIMAX(xma), size);
			if (!free) xfi = FIXED - 1;
		}

		if (!free)
		{
			/* try each large block chain left */
			for (++xfi; xfi < XFIMAX(xma) - 1; xfi++)
			{
				free = alloc_from_freelist(xma, xfi, size);
				if (free) break;
			}
			if (!free) return MOO_NULL;
		}
	}

	return SYS_TO_USR(free);
}

static void* _realloc_merge (moo_xma_t* xma, void* b, moo_oow_t size)
{
	moo_xma_mblk_t* blk = USR_TO_SYS(b);

	/* rounds up 'size' to be multiples of ALIGN */ 
	if (size < MINCHUNKSIZE) size = MINCHUNKSIZE;
	size = MOO_ALIGN_POW2(size, ALIGN);

	if (size > blk->size)
	{
		/* grow the current block */
		moo_oow_t req;
		moo_xma_mblk_t* n;
		moo_oow_t rem;

		req = size - blk->size;

		n = next_mblk(blk);
		/* check if the next adjacent block is available */
		if ((moo_uint8_t*)n >= xma->end || !n->avail || req > n->size) return MOO_NULL; /* no! */

		/* let's merge the current block with the next block */
		detach_from_freelist (xma, (moo_xma_fblk_t*)n);

		rem = (HDRSIZE + n->size) - req;
		if (rem >= MINBLKLEN)
		{
			/* 
			 * the remaining part of the next block is large enough 
			 * to hold a block. break the next block.
			 */

			moo_xma_fblk_t* tmp;

			tmp = (moo_xma_fblk_t*)(((moo_uint8_t*)n) + req);

			blk->size += req;

			tmp->avail = 1;
			tmp->size = rem - HDRSIZE;
			tmp->prev_size = blk->size;
			attach_to_freelist (xma, tmp);

			n = next_mblk(tmp);
			if ((moo_uint8_t*)n < xma->end) n->prev_size = tmp->size;

#if defined(MOO_XMA_ENABLE_STAT)
			xma->stat.alloc += req;
			xma->stat.avail -= req; /* req + HDRSIZE(tmp) - HDRSIZE(n) */
#endif
		}
		else
		{
			/* the remaining part of the next block is negligible.
			 * utilize the whole block by merging to the resizing block */
			blk->size += HDRSIZE + n->size;

			n = next_mblk(blk);
			if ((moo_uint8_t*)n < xma->end) n->prev_size = blk->size;

#if defined(MOO_XMA_ENABLE_STAT)
			xma->stat.nfree--;
			xma->stat.alloc += HDRSIZE + n->size;
			xma->stat.avail -= n->size;
#endif
		}
	}
	else if (size < blk->size)
	{
		/* shrink the block */

		moo_oow_t rem = blk->size - size;
		if (rem >= MINBLKLEN) 
		{
			moo_xma_fblk_t* tmp;
			moo_xma_mblk_t* n;

			n = next_mblk(blk);

			/* the leftover is large enough to hold a block of minimum size.
			 * split the current block. let 'tmp' point to the leftover. */
			tmp = (moo_xma_fblk_t*)(((moo_uint8_t*)(blk + 1)) + size);
			tmp->avail = 1;

			if ((moo_uint8_t*)n < xma->end && n->avail)
			{
				/* let the leftover block merge with the next block */
				detach_from_freelist (xma, (moo_xma_fblk_t*)n);

				blk->size = size;

				tmp->size = rem - HDRSIZE + HDRSIZE + n->size;
				tmp->prev_size = size;

				n = next_mblk(blk);
				if ((moo_uint8_t*)n < xma->end) n->prev_size = tmp->size;

#if defined(MOO_XMA_ENABLE_STAT)
				xma->stat.alloc -= rem;
				/* rem - HDRSIZE(tmp) + HDRSIZE(n) */
				xma->stat.avail += rem;
#endif
			}
			else
			{
				/* link 'tmp' to the block list */
				blk->size = size;

				tmp->size = rem - HDRSIZE;
				tmp->prev_size = blk->size;

#if defined(MOO_XMA_ENABLE_STAT)
				xma->stat.nfree++;
				xma->stat.alloc -= rem;
				xma->stat.avail += tmp->size;
#endif
			}

			/* add 'tmp' to the free list */
			attach_to_freelist (xma, tmp);
		}
	}

	return b;
}

void* moo_xma_calloc (moo_xma_t* xma, moo_oow_t size)
{
	void* ptr = moo_xma_alloc (xma, size);
	if (ptr) MOO_MEMSET (ptr, 0, size);
	return ptr;
}

void* moo_xma_realloc (moo_xma_t* xma, void* b, moo_oow_t size)
{
	void* n;

	if (b == MOO_NULL) 
	{
		/* 'realloc' with NULL is the same as 'alloc' */
		n = moo_xma_alloc (xma, size);
	}
	else
	{
		/* try reallocation by merging the adjacent continuous blocks */
		n = _realloc_merge (xma, b, size);
		if (n == MOO_NULL)
		{
			/* reallocation by merging failed. fall back to the slow
			 * allocation-copy-free scheme */
			n = moo_xma_alloc (xma, size);
			if (n)
			{
				MOO_MEMCPY (n, b, size);
				moo_xma_free (xma, b);
			}
		}
	}

	return n;
}

void moo_xma_free (moo_xma_t* xma, void* b)
{
	moo_xma_mblk_t* blk = USR_TO_SYS(b);
	moo_xma_mblk_t* x, * y;

	/*assert (blk->f.next == MOO_NULL);*/

#if defined(MOO_XMA_ENABLE_STAT)
	/* update statistical variables */
	xma->stat.nused--;
	xma->stat.alloc -= blk->size;
#endif

	x = prev_mblk(blk);
	y = next_mblk(blk);
	if (((moo_uint8_t*)x >= xma->start && x->avail) && ((moo_uint8_t*)y < xma->end && y->avail))
	{
		/*
		 * Merge the block with surrounding blocks
		 *
		 *                    blk 
		 *                     |
		 *                     v
		 * +------------+------------+------------+------------+
		 * |     X      |            |     Y      |     Z      |
		 * +------------+------------+------------+------------+
		 *         
		 *  
		 * +--------------------------------------+------------+
		 * |     X                                |     Z      |
		 * +--------------------------------------+------------+
		 *
		 */
		
		moo_xma_mblk_t* z = next_mblk(y);
		moo_oow_t ns = HDRSIZE + blk->size + HDRSIZE;
		moo_oow_t bs = ns + y->size;

		detach_from_freelist (xma, (moo_xma_fblk_t*)x);
		detach_from_freelist (xma, (moo_xma_fblk_t*)y);

		x->size += bs;
		if ((moo_uint8_t*)z < xma->end) z->prev_size = x->size;

		attach_to_freelist (xma, (moo_xma_fblk_t*)x);

#if defined(MOO_XMA_ENABLE_STAT)
		xma->stat.nfree--;
		xma->stat.avail += ns;
#endif
	}
	else if ((moo_uint8_t*)y < xma->end && y->avail)
	{
		/*
		 * Merge the block with the next block
		 *
		 *   blk
		 *    |
		 *    v
		 * +------------+------------+------------+
		 * |            |     Y      |     Z      |
		 * +------------+------------+------------+
		 * 
		 * 
		 *
		 *   blk
		 *    |
		 *    v
		 * +-------------------------+------------+
		 * |                         |     Z      |
		 * +-------------------------+------------+
		 * 
		 * 
		 */
		moo_xma_mblk_t* z = next_mblk(y);

#if defined(MOO_XMA_ENABLE_STAT)
		xma->stat.avail += blk->size + HDRSIZE;
#endif

		/* detach y from the free list */
		detach_from_freelist (xma, (moo_xma_fblk_t*)y);

		/* update the block availability */
		blk->avail = 1;
		/* update the block size. HDRSIZE for the header space in x */
		blk->size += HDRSIZE + y->size;

		/* update the backward link of Y */
		if ((moo_uint8_t*)z < xma->end) z->prev_size = blk->size;

		/* attach blk to the free list */
		attach_to_freelist (xma, (moo_xma_fblk_t*)blk);
	}
	else if ((moo_uint8_t*)x >= xma->start && x->avail)
	{
		/*
		 * Merge the block with the previous block 
		 *
		 *                   blk
		 *          +-----+   |    +-----+  
		 *          |     V   v    |     v  
		 * +------------+------------+------------+
		 * |     X      |            |     Y      |
		 * +------------+------------+------------+
		 *
		 * +-------------------------+------------+
		 * |     X                   |     Y      |
		 * +-------------------------+------------+
		 *
		 *
		 *
		 */
#if defined(MOO_XMA_ENABLE_STAT)
		xma->stat.avail += HDRSIZE + blk->size;
#endif

		detach_from_freelist (xma, (moo_xma_fblk_t*)x);

		x->size += HDRSIZE + blk->size;
		if ((moo_uint8_t*)y < xma->end) y->prev_size = x->size;

		attach_to_freelist (xma, (moo_xma_fblk_t*)x);
	}
	else
	{
		blk->avail = 1;
		attach_to_freelist (xma, (moo_xma_fblk_t*)blk);

#if defined(MOO_XMA_ENABLE_STAT)
		xma->stat.nfree++;
		xma->stat.avail += blk->size;
#endif
	}
}

void moo_xma_dump (moo_xma_t* xma, moo_xma_dumper_t dumper, void* ctx)
{
	moo_xma_mblk_t* tmp;
	moo_oow_t fsum, asum; 
#if defined(MOO_XMA_ENABLE_STAT)
	moo_oow_t isum;
#endif

	dumper (ctx, "<XMA DUMP>\n");

#if defined(MOO_XMA_ENABLE_STAT)
	dumper (ctx, "== statistics ==\n");
	dumper (ctx, "total = %zu\n", xma->stat.total);
	dumper (ctx, "alloc = %zu\n", xma->stat.alloc);
	dumper (ctx, "avail = %zu\n", xma->stat.avail);
#endif

	dumper (ctx, "== blocks ==\n");
	dumper (ctx, " size               avail address\n");
	for (tmp = (moo_xma_mblk_t*)xma->start, fsum = 0, asum = 0; (moo_uint8_t*)tmp < xma->end; tmp = next_mblk(tmp))
	{
		dumper (ctx, " %-18zu %-5u %p\n", tmp->size, (unsigned int)tmp->avail, tmp);
		if (tmp->avail) fsum += tmp->size;
		else asum += tmp->size;
	}

#if defined(MOO_XMA_ENABLE_STAT)
	isum = (xma->stat.nfree + xma->stat.nused) * HDRSIZE;
#endif

	dumper (ctx, "---------------------------------------\n");
	dumper (ctx, "Allocated blocks: %18zu bytes\n", asum);
	dumper (ctx, "Available blocks: %18zu bytes\n", fsum);


#if defined(MOO_XMA_ENABLE_STAT)
	dumper (ctx, "Internal use    : %18zu bytes\n", isum);
	dumper (ctx, "Total           : %18zu bytes\n", (asum + fsum + isum));
#endif

#if defined(MOO_XMA_ENABLE_STAT)
	assert (asum == xma->stat.alloc);
	assert (fsum == xma->stat.avail);
	assert (isum == xma->stat.total - (xma->stat.alloc + xma->stat.avail));
	assert (asum + fsum + isum == xma->stat.total);
#endif
}

