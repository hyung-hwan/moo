/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _STIX_RBT_H_
#define _STIX_RBT_H_

#include "stix-cmn.h"

/**@file
 * This file provides a red-black tree encapsulated in the #stix_rbt_t type that
 * implements a self-balancing binary search tree.Its interface is very close 
 * to #stix_htb_t.
 *
 * This sample code adds a series of keys and values and print them
 * in descending key order.
 * @code
 * #include <stix/cmn/rbt.h>
 * #include <stix/cmn/mem.h>
 * #include <stix/cmn/sio.h>
 * 
 * static stix_rbt_walk_t walk (stix_rbt_t* rbt, stix_rbt_pair_t* pair, void* ctx)
 * {
 *   stix_printf (STIX_T("key = %d, value = %d\n"),
 *     *(int*)STIX_RBT_KPTR(pair), *(int*)STIX_RBT_VPTR(pair));
 *   return STIX_RBT_WALK_FORWARD;
 * }
 * 
 * int main ()
 * {
 *   stix_rbt_t* s1;
 *   int i;
 * 
 *   s1 = stix_rbt_open (STIX_MMGR_GETDFL(), 0, 1, 1); // error handling skipped
 *   stix_rbt_setstyle (s1, stix_getrbtstyle(STIX_RBT_STYLE_INLINE_COPIERS));
 * 
 *   for (i = 0; i < 20; i++)
 *   {
 *     int x = i * 20;
 *     stix_rbt_insert (s1, &i, STIX_SIZEOF(i), &x, STIX_SIZEOF(x)); // eror handling skipped
 *   }
 * 
 *   stix_rbt_rwalk (s1, walk, STIX_NULL);
 * 
 *   stix_rbt_close (s1);
 *   return 0;
 * }
 * @endcode
 */

typedef struct stix_rbt_t stix_rbt_t;
typedef struct stix_rbt_pair_t stix_rbt_pair_t;

/** 
 * The stix_rbt_walk_t type defines values that the callback function can
 * return to control stix_rbt_walk() and stix_rbt_rwalk().
 */
enum stix_rbt_walk_t
{
	STIX_RBT_WALK_STOP    = 0,
	STIX_RBT_WALK_FORWARD = 1
};
typedef enum stix_rbt_walk_t stix_rbt_walk_t;

/**
 * The stix_rbt_id_t type defines IDs to indicate a key or a value in various
 * functions
 */
enum stix_rbt_id_t
{
	STIX_RBT_KEY = 0, /**< indicate a key */
	STIX_RBT_VAL = 1  /**< indicate a value */
};
typedef enum stix_rbt_id_t stix_rbt_id_t;

/**
 * The stix_rbt_copier_t type defines a pair contruction callback.
 */
typedef void* (*stix_rbt_copier_t) (
	stix_rbt_t* rbt  /* red-black tree */,
	void*       dptr /* pointer to a key or a value */, 
	stix_oow_t  dlen /* length of a key or a value */
);

/**
 * The stix_rbt_freeer_t defines a key/value destruction callback.
 */
typedef void (*stix_rbt_freeer_t) (
	stix_rbt_t* rbt,  /**< red-black tree */
	void*       dptr, /**< pointer to a key or a value */
	stix_oow_t  dlen  /**< length of a key or a value */
);

/**
 * The stix_rbt_comper_t type defines a key comparator that is called when
 * the rbt needs to compare keys. A red-black tree is created with a default
 * comparator which performs bitwise comparison of two keys.
 * The comparator should return 0 if the keys are the same, 1 if the first
 * key is greater than the second key, -1 otherwise.
 */
typedef int (*stix_rbt_comper_t) (
	const stix_rbt_t* rbt,    /**< red-black tree */ 
	const void*      kptr1,  /**< key pointer */
	stix_oow_t       klen1,  /**< key length */ 
	const void*      kptr2,  /**< key pointer */
	stix_oow_t       klen2   /**< key length */
);

/**
 * The stix_rbt_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their 
 * pointers and lengths are equal.
 */
typedef void (*stix_rbt_keeper_t) (
	stix_rbt_t* rbt,    /**< red-black tree */
	void*       vptr,   /**< value pointer */
	stix_oow_t vlen    /**< value length */
);

/**
 * The stix_rbt_walker_t defines a pair visitor.
 */
typedef stix_rbt_walk_t (*stix_rbt_walker_t) (
	stix_rbt_t*      rbt,   /**< red-black tree */
	stix_rbt_pair_t* pair,  /**< pointer to a key/value pair */
	void*            ctx    /**< pointer to user-defined data */
);

/**
 * The stix_rbt_cbserter_t type defines a callback function for stix_rbt_cbsert().
 * The stix_rbt_cbserter() function calls it to allocate a new pair for the 
 * key pointed to by @a kptr of the length @a klen and the callback context
 * @a ctx. The second parameter @a pair is passed the pointer to the existing
 * pair for the key or #STIX_NULL in case of no existing key. The callback
 * must return a pointer to a new or a reallocated pair. When reallocating the
 * existing pair, this callback must destroy the existing pair and return the 
 * newly reallocated pair. It must return #STIX_NULL for failure.
 */
typedef stix_rbt_pair_t* (*stix_rbt_cbserter_t) (
	stix_rbt_t*      rbt,    /**< red-black tree */
	stix_rbt_pair_t* pair,   /**< pair pointer */
	void*            kptr,   /**< key pointer */
	stix_oow_t      klen,   /**< key length */
	void*            ctx     /**< callback context */
);

/**
 * The stix_rbt_pair_t type defines red-black tree pair. A pair is composed 
 * of a key and a value. It maintains pointers to the beginning of a key and 
 * a value plus their length. The length is scaled down with the scale factor 
 * specified in an owning tree. Use macros defined in the 
 */
struct stix_rbt_pair_t
{
	struct
	{
		void*       ptr;
		stix_oow_t len;
	} key;

	struct
	{
		void*       ptr;
		stix_oow_t len;
	} val;

	/* management information below */
	enum
	{
		STIX_RBT_RED,
		STIX_RBT_BLACK
	} color;
	stix_rbt_pair_t* parent;
	stix_rbt_pair_t* child[2]; /* left and right */
};

typedef struct stix_rbt_style_t stix_rbt_style_t;

/**
 * The stix_rbt_style_t type defines callback function sets for key/value 
 * pair manipulation. 
 */
struct stix_rbt_style_t
{
	stix_rbt_copier_t copier[2]; /**< key and value copier */
	stix_rbt_freeer_t freeer[2]; /**< key and value freeer */
	stix_rbt_comper_t comper;    /**< key comparator */
	stix_rbt_keeper_t keeper;    /**< value keeper */
};

/**
 * The stix_rbt_style_kind_t type defines the type of predefined
 * callback set for pair manipulation.
 */
enum stix_rbt_style_kind_t
{
	/** store the key and the value pointer */
	STIX_RBT_STYLE_DEFAULT,
	/** copy both key and value into the pair */
	STIX_RBT_STYLE_INLINE_COPIERS,
	/** copy the key into the pair but store the value pointer */
	STIX_RBT_STYLE_INLINE_KEY_COPIER,
	/** copy the value into the pair but store the key pointer */
	STIX_RBT_STYLE_INLINE_VALUE_COPIER
};

typedef enum stix_rbt_style_kind_t  stix_rbt_style_kind_t;

/**
 * The stix_rbt_t type defines a red-black tree.
 */
struct stix_rbt_t
{
	stix_t*                 stix;
	const stix_rbt_style_t* style;
	stix_oob_t              scale[2];  /**< length scale */
	stix_rbt_pair_t         xnil;      /**< internal nil node */
	stix_oow_t              size;      /**< number of pairs */
	stix_rbt_pair_t*        root;      /**< root pair */
};

/**
 * The STIX_RBT_COPIER_SIMPLE macros defines a copier that remembers the
 * pointer and length of data in a pair.
 */
#define STIX_RBT_COPIER_SIMPLE ((stix_rbt_copier_t)1)

/**
 * The STIX_RBT_COPIER_INLINE macros defines a copier that copies data into
 * a pair.
 */
#define STIX_RBT_COPIER_INLINE ((stix_rbt_copier_t)2)

#define STIX_RBT_COPIER_DEFAULT (STIX_RBT_COPIER_SIMPLE)
#define STIX_RBT_FREEER_DEFAULT (STIX_NULL)
#define STIX_RBT_COMPER_DEFAULT (stix_rbt_dflcomp)
#define STIX_RBT_KEEPER_DEFAULT (STIX_NULL)

/**
 * The STIX_RBT_SIZE() macro returns the number of pairs in red-black tree.
 */
#define STIX_RBT_SIZE(m)   ((const stix_oow_t)(m)->size)
#define STIX_RBT_KSCALE(m) ((const int)(m)->scale[STIX_RBT_KEY])
#define STIX_RBT_VSCALE(m) ((const int)(m)->scale[STIX_RBT_VAL])

#define STIX_RBT_KPTL(p) (&(p)->key)
#define STIX_RBT_VPTL(p) (&(p)->val)

#define STIX_RBT_KPTR(p) ((p)->key.ptr)
#define STIX_RBT_KLEN(p) ((p)->key.len)
#define STIX_RBT_VPTR(p) ((p)->val.ptr)
#define STIX_RBT_VLEN(p) ((p)->val.len)

#define STIX_RBT_NEXT(p) ((p)->next)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The stix_getrbtstyle() functions returns a predefined callback set for
 * pair manipulation.
 */
STIX_EXPORT const stix_rbt_style_t* stix_getrbtstyle (
	stix_rbt_style_kind_t kind
);

/**
 * The stix_rbt_open() function creates a red-black tree.
 * @return stix_rbt_t pointer on success, STIX_NULL on failure.
 */
STIX_EXPORT stix_rbt_t* stix_rbt_open (
	stix_t*      stix,
	stix_oow_t   xtnsize, /**< extension size in bytes */
	int          kscale,  /**< key scale */
	int          vscale   /**< value scale */
);

/**
 * The stix_rbt_close() function destroys a red-black tree.
 */
STIX_EXPORT void stix_rbt_close (
	stix_rbt_t* rbt   /**< red-black tree */
);

/**
 * The stix_rbt_init() function initializes a red-black tree
 */
STIX_EXPORT int stix_rbt_init (
	stix_rbt_t*  rbt,    /**< red-black tree */
	stix_t*      stix,
	int          kscale, /**< key scale */
	int          vscale  /**< value scale */
);

/**
 * The stix_rbt_fini() funtion finalizes a red-black tree
 */
STIX_EXPORT void stix_rbt_fini (
	stix_rbt_t* rbt  /**< red-black tree */
);

STIX_EXPORT void* stix_rbt_getxtn (
	stix_rbt_t* rbt
);

/**
 * The stix_rbt_getstyle() function gets manipulation callback function set.
 */
STIX_EXPORT const stix_rbt_style_t* stix_rbt_getstyle (
	const stix_rbt_t* rbt /**< red-black tree */
);

/**
 * The stix_rbt_setstyle() function sets internal manipulation callback 
 * functions for data construction, destruction, comparison, etc.
 * The callback structure pointed to by \a style must outlive the tree
 * pointed to by \a htb as the tree doesn't copy the contents of the 
 * structure.
 */
STIX_EXPORT void stix_rbt_setstyle (
	stix_rbt_t*             rbt,  /**< red-black tree */
	const stix_rbt_style_t* style /**< callback function set */
);

/**
 * The stix_rbt_getsize() function gets the number of pairs in red-black tree.
 */
STIX_EXPORT stix_oow_t stix_rbt_getsize (
	const stix_rbt_t* rbt  /**< red-black tree */
);

/**
 * The stix_rbt_search() function searches red-black tree to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns STIX_NULL.
 * @return pointer to the pair with a maching key, 
 *         or STIX_NULL if no match is found.
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_search (
	const stix_rbt_t* rbt,   /**< red-black tree */
	const void*       kptr,  /**< key pointer */
	stix_oow_t       klen   /**< the size of the key */
);

/**
 * The stix_rbt_upsert() function searches red-black tree for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and the value given. It returns the pointer to the 
 * pair updated or inserted.
 * @return a pointer to the updated or inserted pair on success, 
 *         STIX_NULL on failure. 
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_upsert (
	stix_rbt_t* rbt,   /**< red-black tree */
	void*       kptr,  /**< key pointer */
	stix_oow_t klen,  /**< key length */
	void*       vptr,  /**< value pointer */
	stix_oow_t vlen   /**< value length */
);

/**
 * The stix_rbt_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * @return pointer to a pair on success, STIX_NULL on failure. 
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_ensert (
	stix_rbt_t* rbt,   /**< red-black tree */
	void*       kptr,  /**< key pointer */
	stix_oow_t klen,  /**< key length */
	void*       vptr,  /**< value pointer */
	stix_oow_t vlen   /**< value length */
);

/**
 * The stix_rbt_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * STIX_NULL without channging the value.
 * @return pointer to the pair created on success, STIX_NULL on failure. 
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_insert (
	stix_rbt_t* rbt,   /**< red-black tree */
	void*       kptr,  /**< key pointer */
	stix_oow_t klen,  /**< key length */
	void*       vptr,  /**< value pointer */
	stix_oow_t vlen   /**< value length */
);

/**
 * The stix_rbt_update() function updates the value of an existing pair
 * with a matching key.
 * @return pointer to the pair on success, STIX_NULL on no matching pair
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_update (
	stix_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	stix_oow_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	stix_oow_t vlen   /**< value length */
);

/**
 * The stix_rbt_cbsert() function inserts a key/value pair by delegating pair 
 * allocation to a callback function. Depending on the callback function,
 * it may behave like stix_rbt_insert(), stix_rbt_upsert(), stix_rbt_update(),
 * stix_rbt_ensert(), or totally differently. The sample code below inserts
 * a new pair if the key is not found and appends the new value to the
 * existing value delimited by a comma if the key is found.
 *
 * @code
 * stix_rbt_walk_t print_map_pair (stix_rbt_t* map, stix_rbt_pair_t* pair, void* ctx)
 * {
 *   stix_printf (STIX_T("%.*s[%d] => %.*s[%d]\n"),
 *     (int)STIX_RBT_KLEN(pair), STIX_RBT_KPTR(pair), (int)STIX_RBT_KLEN(pair),
 *     (int)STIX_RBT_VLEN(pair), STIX_RBT_VPTR(pair), (int)STIX_RBT_VLEN(pair));
 *   return STIX_RBT_WALK_FORWARD;
 * }
 * 
 * stix_rbt_pair_t* cbserter (
 *   stix_rbt_t* rbt, stix_rbt_pair_t* pair,
 *   void* kptr, stix_oow_t klen, void* ctx)
 * {
 *   stix_cstr_t* v = (stix_cstr_t*)ctx;
 *   if (pair == STIX_NULL)
 *   {
 *     // no existing key for the key 
 *     return stix_rbt_allocpair (rbt, kptr, klen, v->ptr, v->len);
 *   }
 *   else
 *   {
 *     // a pair with the key exists. 
 *     // in this sample, i will append the new value to the old value 
 *     // separated by a comma
 *     stix_rbt_pair_t* new_pair;
 *     stix_char_t comma = STIX_T(',');
 *     stix_oob_t* vptr;
 * 
 *     // allocate a new pair, but without filling the actual value. 
 *     // note vptr is given STIX_NULL for that purpose 
 *     new_pair = stix_rbt_allocpair (
 *       rbt, kptr, klen, STIX_NULL, pair->vlen + 1 + v->len); 
 *     if (new_pair == STIX_NULL) return STIX_NULL;
 * 
 *     // fill in the value space 
 *     vptr = new_pair->vptr;
 *     stix_memcpy (vptr, pair->vptr, pair->vlen*STIX_SIZEOF(stix_char_t));
 *     vptr += pair->vlen*STIX_SIZEOF(stix_char_t);
 *     stix_memcpy (vptr, &comma, STIX_SIZEOF(stix_char_t));
 *     vptr += STIX_SIZEOF(stix_char_t);
 *     stix_memcpy (vptr, v->ptr, v->len*STIX_SIZEOF(stix_char_t));
 * 
 *     // this callback requires the old pair to be destroyed 
 *     stix_rbt_freepair (rbt, pair);
 * 
 *     // return the new pair 
 *     return new_pair;
 *   }
 * }
 * 
 * int main ()
 * {
 *   stix_rbt_t* s1;
 *   int i;
 *   stix_char_t* keys[] = { STIX_T("one"), STIX_T("two"), STIX_T("three") };
 *   stix_char_t* vals[] = { STIX_T("1"), STIX_T("2"), STIX_T("3"), STIX_T("4"), STIX_T("5") };
 * 
 *   s1 = stix_rbt_open (
 *     STIX_MMGR_GETDFL(), 0,
 *     STIX_SIZEOF(stix_char_t), STIX_SIZEOF(stix_char_t)
 *   ); // note error check is skipped 
 *   stix_rbt_setstyle (s1, &style1);
 * 
 *   for (i = 0; i < STIX_COUNTOF(vals); i++)
 *   {
 *     stix_cstr_t ctx;
 *     ctx.ptr = vals[i]; ctx.len = stix_strlen(vals[i]);
 *     stix_rbt_cbsert (s1,
 *       keys[i%STIX_COUNTOF(keys)], stix_strlen(keys[i%STIX_COUNTOF(keys)]),
 *       cbserter, &ctx
 *     ); // note error check is skipped
 *   }
 *   stix_rbt_walk (s1, print_map_pair, STIX_NULL);
 * 
 *   stix_rbt_close (s1);
 *   return 0;
 * }
 * @endcode
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_cbsert (
	stix_rbt_t*         rbt,      /**< red-black tree */
	void*               kptr,     /**< key pointer */
	stix_oow_t         klen,     /**< key length */
	stix_rbt_cbserter_t cbserter, /**< callback function */
	void*               ctx       /**< callback context */
);

/**
 * The stix_rbt_delete() function deletes a pair with a matching key 
 * @return 0 on success, -1 on failure
 */
STIX_EXPORT int stix_rbt_delete (
	stix_rbt_t* rbt,   /**< red-black tree */
	const void* kptr, /**< key pointer */
	stix_oow_t klen   /**< key size */
);

/**
 * The stix_rbt_clear() function empties a red-black tree.
 */
STIX_EXPORT void stix_rbt_clear (
	stix_rbt_t* rbt /**< red-black tree */
);

/**
 * The stix_rbt_walk() function traverses a red-black tree in preorder 
 * from the leftmost child.
 */
STIX_EXPORT void stix_rbt_walk (
	stix_rbt_t*       rbt,    /**< red-black tree */
	stix_rbt_walker_t walker, /**< callback function for each pair */
	void*             ctx     /**< pointer to user-specific data */
);

/**
 * The stix_rbt_walk() function traverses a red-black tree in preorder 
 * from the rightmost child.
 */
STIX_EXPORT void stix_rbt_rwalk (
	stix_rbt_t*       rbt,    /**< red-black tree */
	stix_rbt_walker_t walker, /**< callback function for each pair */
	void*             ctx     /**< pointer to user-specific data */
);

/**
 * The stix_rbt_allocpair() function allocates a pair for a key and a value 
 * given. But it does not chain the pair allocated into the red-black tree @a rbt.
 * Use this function at your own risk. 
 *
 * Take note of he following special behavior when the copier is 
 * #STIX_RBT_COPIER_INLINE.
 * - If @a kptr is #STIX_NULL, the key space of the size @a klen is reserved but
 *   not propagated with any data.
 * - If @a vptr is #STIX_NULL, the value space of the size @a vlen is reserved
 *   but not propagated with any data.
 */
STIX_EXPORT stix_rbt_pair_t* stix_rbt_allocpair (
	stix_rbt_t* rbt,
	void*       kptr, 
	stix_oow_t klen,
	void*       vptr,
	stix_oow_t vlen
);

/**
 * The stix_rbt_freepair() function destroys a pair. But it does not detach
 * the pair destroyed from the red-black tree @a rbt. Use this function at your
 * own risk.
 */
STIX_EXPORT void stix_rbt_freepair (
	stix_rbt_t*      rbt,
	stix_rbt_pair_t* pair
);

/**
 * The stix_rbt_dflcomp() function defines the default key comparator.
 */
STIX_EXPORT int stix_rbt_dflcomp (
	const stix_rbt_t* rbt,
	const void*       kptr1,
	stix_oow_t       klen1,
	const void*       kptr2,
	stix_oow_t       klen2
);

#if defined(__cplusplus)
}
#endif

#endif
