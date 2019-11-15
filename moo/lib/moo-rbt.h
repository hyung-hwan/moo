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

#ifndef _MOO_RBT_H_
#define _MOO_RBT_H_

#include <moo-cmn.h>

/** \file
 * This file provides a red-black tree encapsulated in the #moo_rbt_t type that
 * implements a self-balancing binary search tree.Its interface is very close 
 * to #moo_htb_t.
 *
 * This sample code adds a series of keys and values and print them
 * in descending key order.
 * \code
 * #include <moo-rbt.h>
 * 
 * static moo_rbt_walk_t walk (moo_rbt_t* rbt, moo_rbt_pair_t* pair, void* ctx)
 * {
 *   moo_printf (MOO_T("key = %d, value = %d\n"),
 *     *(int*)MOO_RBT_KPTR(pair), *(int*)MOO_RBT_VPTR(pair));
 *   return MOO_RBT_WALK_FORWARD;
 * }
 * 
 * int main ()
 * {
 *   moo_rbt_t* s1;
 *   int i;
 * 
 *   s1 = moo_rbt_open (MOO_MMGR_GETDFL(), 0, 1, 1); // error handling skipped
 *   moo_rbt_setstyle (s1, moo_getrbtstyle(MOO_RBT_STYLE_INLINE_COPIERS));
 * 
 *   for (i = 0; i < 20; i++)
 *   {
 *     int x = i * 20;
 *     moo_rbt_insert (s1, &i, MOO_SIZEOF(i), &x, MOO_SIZEOF(x)); // eror handling skipped
 *   }
 * 
 *   moo_rbt_rwalk (s1, walk, MOO_NULL);
 * 
 *   moo_rbt_close (s1);
 *   return 0;
 * }
 * \endcode
 */

typedef struct moo_rbt_t moo_rbt_t;
typedef struct moo_rbt_pair_t moo_rbt_pair_t;

/** 
 * The moo_rbt_walk_t type defines values that the callback function can
 * return to control moo_rbt_walk() and moo_rbt_rwalk().
 */
enum moo_rbt_walk_t
{
	MOO_RBT_WALK_STOP    = 0,
	MOO_RBT_WALK_FORWARD = 1
};
typedef enum moo_rbt_walk_t moo_rbt_walk_t;

/**
 * The moo_rbt_id_t type defines IDs to indicate a key or a value in various
 * functions
 */
enum moo_rbt_id_t
{
	MOO_RBT_KEY = 0, /**< indicate a key */
	MOO_RBT_VAL = 1  /**< indicate a value */
};
typedef enum moo_rbt_id_t moo_rbt_id_t;

/**
 * The moo_rbt_copier_t type defines a pair contruction callback.
 */
typedef void* (*moo_rbt_copier_t) (
	moo_rbt_t* rbt  /**< red-black tree */,
	void*      dptr /**< pointer to a key or a value */, 
	moo_oow_t  dlen /**< length of a key or a value */
);

/**
 * The moo_rbt_freeer_t defines a key/value destruction callback.
 */
typedef void (*moo_rbt_freeer_t) (
	moo_rbt_t* rbt,  /**< red-black tree */
	void*      dptr, /**< pointer to a key or a value */
	moo_oow_t  dlen  /**< length of a key or a value */
);

/**
 * The moo_rbt_comper_t type defines a key comparator that is called when
 * the rbt needs to compare keys. A red-black tree is created with a default
 * comparator which performs bitwise comparison of two keys.
 * The comparator should return 0 if the keys are the same, 1 if the first
 * key is greater than the second key, -1 otherwise.
 */
typedef int (*moo_rbt_comper_t) (
	const moo_rbt_t* rbt,    /**< red-black tree */ 
	const void*      kptr1,  /**< key pointer */
	moo_oow_t        klen1,  /**< key length */ 
	const void*      kptr2,  /**< key pointer */
	moo_oow_t        klen2   /**< key length */
);

/**
 * The moo_rbt_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their 
 * pointers and lengths are equal.
 */
typedef void (*moo_rbt_keeper_t) (
	moo_rbt_t* rbt,    /**< red-black tree */
	void*       vptr,   /**< value pointer */
	moo_oow_t vlen    /**< value length */
);

/**
 * The moo_rbt_walker_t defines a pair visitor.
 */
typedef moo_rbt_walk_t (*moo_rbt_walker_t) (
	moo_rbt_t*      rbt,   /**< red-black tree */
	moo_rbt_pair_t* pair,  /**< pointer to a key/value pair */
	void*            ctx    /**< pointer to user-defined data */
);

/**
 * The moo_rbt_cbserter_t type defines a callback function for moo_rbt_cbsert().
 * The moo_rbt_cbserter() function calls it to allocate a new pair for the 
 * key pointed to by \a kptr of the length \a klen and the callback context
 * \a ctx. The second parameter \a pair is passed the pointer to the existing
 * pair for the key or #MOO_NULL in case of no existing key. The callback
 * must return a pointer to a new or a reallocated pair. When reallocating the
 * existing pair, this callback must destroy the existing pair and return the 
 * newly reallocated pair. It must return #MOO_NULL for failure.
 */
typedef moo_rbt_pair_t* (*moo_rbt_cbserter_t) (
	moo_rbt_t*      rbt,    /**< red-black tree */
	moo_rbt_pair_t* pair,   /**< pair pointer */
	void*           kptr,   /**< key pointer */
	moo_oow_t       klen,   /**< key length */
	void*           ctx     /**< callback context */
);

enum moo_rbt_pair_color_t
{
	MOO_RBT_RED,
	MOO_RBT_BLACK
};
typedef enum moo_rbt_pair_color_t moo_rbt_pair_color_t;

/**
 * The moo_rbt_pair_t type defines red-black tree pair. A pair is composed 
 * of a key and a value. It maintains pointers to the beginning of a key and 
 * a value plus their length. The length is scaled down with the scale factor 
 * specified in an owning tree. Use macros defined in the 
 */
struct moo_rbt_pair_t
{
	struct
	{
		void*     ptr;
		moo_oow_t len;
	} key;

	struct
	{
		void*       ptr;
		moo_oow_t len;
	} val;

	/* management information below */
	moo_rbt_pair_color_t color;
	moo_rbt_pair_t* parent;
	moo_rbt_pair_t* child[2]; /* left and right */
};

typedef struct moo_rbt_style_t moo_rbt_style_t;

/**
 * The moo_rbt_style_t type defines callback function sets for key/value 
 * pair manipulation. 
 */
struct moo_rbt_style_t
{
	moo_rbt_copier_t copier[2]; /**< key and value copier */
	moo_rbt_freeer_t freeer[2]; /**< key and value freeer */
	moo_rbt_comper_t comper;    /**< key comparator */
	moo_rbt_keeper_t keeper;    /**< value keeper */
};

/**
 * The moo_rbt_style_kind_t type defines the type of predefined
 * callback set for pair manipulation.
 */
enum moo_rbt_style_kind_t
{
	/** store the key and the value pointer */
	MOO_RBT_STYLE_DEFAULT,
	/** copy both key and value into the pair */
	MOO_RBT_STYLE_INLINE_COPIERS,
	/** copy the key into the pair but store the value pointer */
	MOO_RBT_STYLE_INLINE_KEY_COPIER,
	/** copy the value into the pair but store the key pointer */
	MOO_RBT_STYLE_INLINE_VALUE_COPIER
};

typedef enum moo_rbt_style_kind_t  moo_rbt_style_kind_t;

/**
 * The moo_rbt_t type defines a red-black tree.
 */
struct moo_rbt_t
{
	moo_t*                 moo;
	const moo_rbt_style_t* style;
	moo_oob_t              scale[2];  /**< length scale */
	moo_rbt_pair_t         xnil;      /**< internal nil node */
	moo_oow_t              size;      /**< number of pairs */
	moo_rbt_pair_t*        root;      /**< root pair */
};

/**
 * The MOO_RBT_COPIER_SIMPLE macros defines a copier that remembers the
 * pointer and length of data in a pair.
 */
#define MOO_RBT_COPIER_SIMPLE ((moo_rbt_copier_t)1)

/**
 * The MOO_RBT_COPIER_INLINE macros defines a copier that copies data into
 * a pair.
 */
#define MOO_RBT_COPIER_INLINE ((moo_rbt_copier_t)2)

#define MOO_RBT_COPIER_DEFAULT (MOO_RBT_COPIER_SIMPLE)
#define MOO_RBT_FREEER_DEFAULT (MOO_NULL)
#define MOO_RBT_COMPER_DEFAULT (moo_rbt_dflcomp)
#define MOO_RBT_KEEPER_DEFAULT (MOO_NULL)

/**
 * The MOO_RBT_SIZE() macro returns the number of pairs in red-black tree.
 */
#define MOO_RBT_SIZE(m)   ((const moo_oow_t)(m)->size)
#define MOO_RBT_KSCALE(m) ((const int)(m)->scale[MOO_RBT_KEY])
#define MOO_RBT_VSCALE(m) ((const int)(m)->scale[MOO_RBT_VAL])

#define MOO_RBT_KPTL(p) (&(p)->key)
#define MOO_RBT_VPTL(p) (&(p)->val)

#define MOO_RBT_KPTR(p) ((p)->key.ptr)
#define MOO_RBT_KLEN(p) ((p)->key.len)
#define MOO_RBT_VPTR(p) ((p)->val.ptr)
#define MOO_RBT_VLEN(p) ((p)->val.len)

#define MOO_RBT_NEXT(p) ((p)->next)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The moo_getrbtstyle() functions returns a predefined callback set for
 * pair manipulation.
 */
MOO_EXPORT const moo_rbt_style_t* moo_getrbtstyle (
	moo_rbt_style_kind_t kind
);

/**
 * The moo_rbt_open() function creates a red-black tree.
 * \return moo_rbt_t pointer on success, MOO_NULL on failure.
 */
MOO_EXPORT moo_rbt_t* moo_rbt_open (
	moo_t*      moo,
	moo_oow_t   xtnsize, /**< extension size in bytes */
	int         kscale,  /**< key scale */
	int         vscale   /**< value scale */
);

/**
 * The moo_rbt_close() function destroys a red-black tree.
 */
MOO_EXPORT void moo_rbt_close (
	moo_rbt_t* rbt   /**< red-black tree */
);

/**
 * The moo_rbt_init() function initializes a red-black tree
 */
MOO_EXPORT int moo_rbt_init (
	moo_rbt_t*  rbt,    /**< red-black tree */
	moo_t*      moo,
	int         kscale, /**< key scale */
	int         vscale  /**< value scale */
);

/**
 * The moo_rbt_fini() funtion finalizes a red-black tree
 */
MOO_EXPORT void moo_rbt_fini (
	moo_rbt_t* rbt  /**< red-black tree */
);

MOO_EXPORT void* moo_rbt_getxtn (
	moo_rbt_t* rbt
);

/**
 * The moo_rbt_getstyle() function gets manipulation callback function set.
 */
MOO_EXPORT const moo_rbt_style_t* moo_rbt_getstyle (
	const moo_rbt_t* rbt /**< red-black tree */
);

/**
 * The moo_rbt_setstyle() function sets internal manipulation callback 
 * functions for data construction, destruction, comparison, etc.
 * The callback structure pointed to by \a style must outlive the tree
 * pointed to by \a htb as the tree doesn't copy the contents of the 
 * structure.
 */
MOO_EXPORT void moo_rbt_setstyle (
	moo_rbt_t*             rbt,  /**< red-black tree */
	const moo_rbt_style_t* style /**< callback function set */
);

/**
 * The moo_rbt_getsize() function gets the number of pairs in red-black tree.
 */
MOO_EXPORT moo_oow_t moo_rbt_getsize (
	const moo_rbt_t* rbt  /**< red-black tree */
);

/**
 * The moo_rbt_search() function searches red-black tree to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns MOO_NULL.
 * \return pointer to the pair with a maching key, 
 *         or MOO_NULL if no match is found.
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_search (
	const moo_rbt_t* rbt,   /**< red-black tree */
	const void*      kptr,  /**< key pointer */
	moo_oow_t        klen   /**< the size of the key */
);

/**
 * The moo_rbt_upsert() function searches red-black tree for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and the value given. It returns the pointer to the 
 * pair updated or inserted.
 * \return a pointer to the updated or inserted pair on success, 
 *         MOO_NULL on failure. 
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_upsert (
	moo_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	moo_oow_t  klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	moo_oow_t  vlen   /**< value length */
);

/**
 * The moo_rbt_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * \return pointer to a pair on success, MOO_NULL on failure. 
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_ensert (
	moo_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	moo_oow_t  klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	moo_oow_t  vlen   /**< value length */
);

/**
 * The moo_rbt_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * MOO_NULL without channging the value.
 * \return pointer to the pair created on success, MOO_NULL on failure. 
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_insert (
	moo_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	moo_oow_t  klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	moo_oow_t  vlen   /**< value length */
);

/**
 * The moo_rbt_update() function updates the value of an existing pair
 * with a matching key.
 * \return pointer to the pair on success, MOO_NULL on no matching pair
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_update (
	moo_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	moo_oow_t  klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	moo_oow_t  vlen   /**< value length */
);

/**
 * The moo_rbt_cbsert() function inserts a key/value pair by delegating pair 
 * allocation to a callback function. Depending on the callback function,
 * it may behave like moo_rbt_insert(), moo_rbt_upsert(), moo_rbt_update(),
 * moo_rbt_ensert(), or totally differently. The sample code below inserts
 * a new pair if the key is not found and appends the new value to the
 * existing value delimited by a comma if the key is found.
 *
 * \code
 * moo_rbt_walk_t print_map_pair (moo_rbt_t* map, moo_rbt_pair_t* pair, void* ctx)
 * {
 *   moo_printf (MOO_T("%.*s[%d] => %.*s[%d]\n"),
 *     (int)MOO_RBT_KLEN(pair), MOO_RBT_KPTR(pair), (int)MOO_RBT_KLEN(pair),
 *     (int)MOO_RBT_VLEN(pair), MOO_RBT_VPTR(pair), (int)MOO_RBT_VLEN(pair));
 *   return MOO_RBT_WALK_FORWARD;
 * }
 * 
 * moo_rbt_pair_t* cbserter (
 *   moo_rbt_t* rbt, moo_rbt_pair_t* pair,
 *   void* kptr, moo_oow_t klen, void* ctx)
 * {
 *   moo_cstr_t* v = (moo_cstr_t*)ctx;
 *   if (pair == MOO_NULL)
 *   {
 *     // no existing key for the key 
 *     return moo_rbt_allocpair (rbt, kptr, klen, v->ptr, v->len);
 *   }
 *   else
 *   {
 *     // a pair with the key exists. 
 *     // in this sample, i will append the new value to the old value 
 *     // separated by a comma
 *     moo_rbt_pair_t* new_pair;
 *     moo_ooch_t comma = MOO_T(',');
 *     moo_oob_t* vptr;
 * 
 *     // allocate a new pair, but without filling the actual value. 
 *     // note vptr is given MOO_NULL for that purpose 
 *     new_pair = moo_rbt_allocpair (
 *       rbt, kptr, klen, MOO_NULL, pair->vlen + 1 + v->len); 
 *     if (new_pair == MOO_NULL) return MOO_NULL;
 * 
 *     // fill in the value space 
 *     vptr = new_pair->vptr;
 *     moo_memcpy (vptr, pair->vptr, pair->vlen*MOO_SIZEOF(moo_ooch_t));
 *     vptr += pair->vlen*MOO_SIZEOF(moo_ooch_t);
 *     moo_memcpy (vptr, &comma, MOO_SIZEOF(moo_ooch_t));
 *     vptr += MOO_SIZEOF(moo_ooch_t);
 *     moo_memcpy (vptr, v->ptr, v->len*MOO_SIZEOF(moo_ooch_t));
 * 
 *     // this callback requires the old pair to be destroyed 
 *     moo_rbt_freepair (rbt, pair);
 * 
 *     // return the new pair 
 *     return new_pair;
 *   }
 * }
 * 
 * int main ()
 * {
 *   moo_rbt_t* s1;
 *   int i;
 *   moo_ooch_t* keys[] = { MOO_T("one"), MOO_T("two"), MOO_T("three") };
 *   moo_ooch_t* vals[] = { MOO_T("1"), MOO_T("2"), MOO_T("3"), MOO_T("4"), MOO_T("5") };
 * 
 *   s1 = moo_rbt_open (
 *     MOO_MMGR_GETDFL(), 0,
 *     MOO_SIZEOF(moo_ooch_t), MOO_SIZEOF(moo_ooch_t)
 *   ); // note error check is skipped 
 *   moo_rbt_setstyle (s1, &style1);
 * 
 *   for (i = 0; i < MOO_COUNTOF(vals); i++)
 *   {
 *     moo_cstr_t ctx;
 *     ctx.ptr = vals[i]; ctx.len = moo_strlen(vals[i]);
 *     moo_rbt_cbsert (s1,
 *       keys[i%MOO_COUNTOF(keys)], moo_strlen(keys[i%MOO_COUNTOF(keys)]),
 *       cbserter, &ctx
 *     ); // note error check is skipped
 *   }
 *   moo_rbt_walk (s1, print_map_pair, MOO_NULL);
 * 
 *   moo_rbt_close (s1);
 *   return 0;
 * }
 * \endcode
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_cbsert (
	moo_rbt_t*         rbt,      /**< red-black tree */
	void*              kptr,     /**< key pointer */
	moo_oow_t          klen,     /**< key length */
	moo_rbt_cbserter_t cbserter, /**< callback function */
	void*              ctx       /**< callback context */
);

/**
 * The moo_rbt_delete() function deletes a pair with a matching key 
 * \return 0 on success, -1 on failure
 */
MOO_EXPORT int moo_rbt_delete (
	moo_rbt_t*  rbt,   /**< red-black tree */
	const void* kptr,  /**< key pointer */
	moo_oow_t   klen   /**< key size */
);

/**
 * The moo_rbt_clear() function empties a red-black tree.
 */
MOO_EXPORT void moo_rbt_clear (
	moo_rbt_t* rbt /**< red-black tree */
);

/**
 * The moo_rbt_walk() function traverses a red-black tree in preorder 
 * from the leftmost child.
 */
MOO_EXPORT void moo_rbt_walk (
	moo_rbt_t*       rbt,    /**< red-black tree */
	moo_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

/**
 * The moo_rbt_walk() function traverses a red-black tree in preorder 
 * from the rightmost child.
 */
MOO_EXPORT void moo_rbt_rwalk (
	moo_rbt_t*       rbt,    /**< red-black tree */
	moo_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

/**
 * The moo_rbt_allocpair() function allocates a pair for a key and a value 
 * given. But it does not chain the pair allocated into the red-black tree \a rbt.
 * Use this function at your own risk. 
 *
 * Take note of he following special behavior when the copier is 
 * #MOO_RBT_COPIER_INLINE.
 * - If \a kptr is #MOO_NULL, the key space of the size \a klen is reserved but
 *   not propagated with any data.
 * - If \a vptr is #MOO_NULL, the value space of the size \a vlen is reserved
 *   but not propagated with any data.
 */
MOO_EXPORT moo_rbt_pair_t* moo_rbt_allocpair (
	moo_rbt_t*  rbt,
	void*       kptr, 
	moo_oow_t   klen,
	void*       vptr,
	moo_oow_t   vlen
);

/**
 * The moo_rbt_freepair() function destroys a pair. But it does not detach
 * the pair destroyed from the red-black tree \a rbt. Use this function at your
 * own risk.
 */
MOO_EXPORT void moo_rbt_freepair (
	moo_rbt_t*      rbt,
	moo_rbt_pair_t* pair
);

/**
 * The moo_rbt_dflcomp() function defines the default key comparator.
 */
MOO_EXPORT int moo_rbt_dflcomp (
	const moo_rbt_t* rbt,
	const void*      kptr1,
	moo_oow_t        klen1,
	const void*      kptr2,
	moo_oow_t        klen2 
);

#if defined(__cplusplus)
}
#endif

#endif
