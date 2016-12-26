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

#include "stix-rbt.h"
#include "stix-prv.h"

#define copier_t        stix_rbt_copier_t
#define freeer_t        stix_rbt_freeer_t
#define comper_t        stix_rbt_comper_t
#define keeper_t        stix_rbt_keeper_t
#define walker_t        stix_rbt_walker_t
#define cbserter_t      stix_rbt_cbserter_t

#define KPTR(p)  STIX_RBT_KPTR(p)
#define KLEN(p)  STIX_RBT_KLEN(p)
#define VPTR(p)  STIX_RBT_VPTR(p)
#define VLEN(p)  STIX_RBT_VLEN(p)

#define KTOB(rbt,len) ((len)*(rbt)->scale[STIX_RBT_KEY])
#define VTOB(rbt,len) ((len)*(rbt)->scale[STIX_RBT_VAL])

#define UPSERT 1
#define UPDATE 2
#define ENSERT 3
#define INSERT 4

#define IS_NIL(rbt,x) ((x) == &((rbt)->xnil))
#define LEFT 0
#define RIGHT 1
#define left child[LEFT]
#define right child[RIGHT]
#define rotate_left(rbt,pivot) rotate(rbt,pivot,1);
#define rotate_right(rbt,pivot) rotate(rbt,pivot,0);

STIX_INLINE stix_rbt_pair_t* stix_rbt_allocpair (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen)
{
	stix_rbt_pair_t* n;

	copier_t kcop = rbt->style->copier[STIX_RBT_KEY];
	copier_t vcop = rbt->style->copier[STIX_RBT_VAL];

	stix_oow_t as = STIX_SIZEOF(stix_rbt_pair_t);
	if (kcop == STIX_RBT_COPIER_INLINE) as += KTOB(rbt,klen);
	if (vcop == STIX_RBT_COPIER_INLINE) as += VTOB(rbt,vlen);

	n = (stix_rbt_pair_t*) STIX_MMGR_ALLOC (rbt->stix->mmgr, as);
	if (n == STIX_NULL) return STIX_NULL;

	n->color = STIX_RBT_RED;
	n->parent = STIX_NULL;
	n->child[LEFT] = &rbt->xnil;
	n->child[RIGHT] = &rbt->xnil;

	KLEN(n) = klen;
	if (kcop == STIX_RBT_COPIER_SIMPLE)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == STIX_RBT_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		if (kptr) STIX_MEMCPY (KPTR(n), kptr, KTOB(rbt,klen));
	}
	else
	{
		KPTR(n) = kcop (rbt, kptr, klen);
		if (KPTR(n) == STIX_NULL)
		{
			STIX_MMGR_FREE (rbt->stix->mmgr, n);
			return STIX_NULL;
		}
	}

	VLEN(n) = vlen;
	if (vcop == STIX_RBT_COPIER_SIMPLE)
	{
		VPTR(n) = vptr;
	}
	else if (vcop == STIX_RBT_COPIER_INLINE)
	{
		VPTR(n) = n + 1;
		if (kcop == STIX_RBT_COPIER_INLINE)
			VPTR(n) = (stix_oob_t*)VPTR(n) + KTOB(rbt,klen);
		if (vptr) STIX_MEMCPY (VPTR(n), vptr, VTOB(rbt,vlen));
	}
	else
	{
		VPTR(n) = vcop (rbt, vptr, vlen);
		if (VPTR(n) != STIX_NULL)
		{
			if (rbt->style->freeer[STIX_RBT_KEY] != STIX_NULL)
				rbt->style->freeer[STIX_RBT_KEY] (rbt, KPTR(n), KLEN(n));
			STIX_MMGR_FREE (rbt->stix->mmgr, n);
			return STIX_NULL;
		}
	}

	return n;
}

STIX_INLINE void stix_rbt_freepair (stix_rbt_t* rbt, stix_rbt_pair_t* pair)
{
	if (rbt->style->freeer[STIX_RBT_KEY] != STIX_NULL)
		rbt->style->freeer[STIX_RBT_KEY] (rbt, KPTR(pair), KLEN(pair));
	if (rbt->style->freeer[STIX_RBT_VAL] != STIX_NULL)
		rbt->style->freeer[STIX_RBT_VAL] (rbt, VPTR(pair), VLEN(pair));
	STIX_MMGR_FREE (rbt->stix->mmgr, pair);
}

static stix_rbt_style_t style[] =
{
	{
		{
			STIX_RBT_COPIER_DEFAULT,
			STIX_RBT_COPIER_DEFAULT
		},
		{
			STIX_RBT_FREEER_DEFAULT,
			STIX_RBT_FREEER_DEFAULT
		},
		STIX_RBT_COMPER_DEFAULT,
		STIX_RBT_KEEPER_DEFAULT
	},

	{
		{
			STIX_RBT_COPIER_INLINE,
			STIX_RBT_COPIER_INLINE
		},
		{
			STIX_RBT_FREEER_DEFAULT,
			STIX_RBT_FREEER_DEFAULT
		},
		STIX_RBT_COMPER_DEFAULT,
		STIX_RBT_KEEPER_DEFAULT
	},

	{
		{
			STIX_RBT_COPIER_INLINE,
			STIX_RBT_COPIER_DEFAULT
		},
		{
			STIX_RBT_FREEER_DEFAULT,
			STIX_RBT_FREEER_DEFAULT
		},
		STIX_RBT_COMPER_DEFAULT,
		STIX_RBT_KEEPER_DEFAULT
	},

	{
		{
			STIX_RBT_COPIER_DEFAULT,
			STIX_RBT_COPIER_INLINE
		},
		{
			STIX_RBT_FREEER_DEFAULT,
			STIX_RBT_FREEER_DEFAULT
		},
		STIX_RBT_COMPER_DEFAULT,
		STIX_RBT_KEEPER_DEFAULT
	}
};

const stix_rbt_style_t* stix_getrbtstyle (stix_rbt_style_kind_t kind)
{
	return &style[kind];
}

stix_rbt_t* stix_rbt_open (stix_t* stix, stix_oow_t xtnsize, int kscale, int vscale)
{
	stix_rbt_t* rbt;

	rbt = (stix_rbt_t*) STIX_MMGR_ALLOC (stix->mmgr, STIX_SIZEOF(stix_rbt_t) + xtnsize);
	if (rbt == STIX_NULL) return STIX_NULL;

	if (stix_rbt_init (rbt, stix, kscale, vscale) <= -1)
	{
		STIX_MMGR_FREE (stix->mmgr, rbt);
		return STIX_NULL;
	}

	STIX_MEMSET (rbt + 1, 0, xtnsize);
	return rbt;
}

void stix_rbt_close (stix_rbt_t* rbt)
{
	stix_rbt_fini (rbt);
	STIX_MMGR_FREE (rbt->stix->mmgr, rbt);
}

int stix_rbt_init (stix_rbt_t* rbt, stix_t* stix, int kscale, int vscale)
{
	/* do not zero out the extension */
	STIX_MEMSET (rbt, 0, STIX_SIZEOF(*rbt));
	rbt->stix = stix;

	rbt->scale[STIX_RBT_KEY] = (kscale < 1)? 1: kscale;
	rbt->scale[STIX_RBT_VAL] = (vscale < 1)? 1: vscale;
	rbt->size = 0;

	rbt->style = &style[0];

	/* self-initializing nil */
	STIX_MEMSET(&rbt->xnil, 0, STIX_SIZEOF(rbt->xnil));
	rbt->xnil.color = STIX_RBT_BLACK;
	rbt->xnil.left = &rbt->xnil;
	rbt->xnil.right = &rbt->xnil;

	/* root is set to nil initially */
	rbt->root = &rbt->xnil;

	return 0;
}

void stix_rbt_fini (stix_rbt_t* rbt)
{
	stix_rbt_clear (rbt);
}

void* stix_rbt_getxtn (stix_rbt_t* rbt)
{
	return (void*)(rbt + 1);
}

const stix_rbt_style_t* stix_rbt_getstyle (const stix_rbt_t* rbt)
{
	return rbt->style;
}

void stix_rbt_setstyle (stix_rbt_t* rbt, const stix_rbt_style_t* style)
{
	STIX_ASSERT (rbt->stix, style != STIX_NULL);
	rbt->style = style;
}

stix_oow_t stix_rbt_getsize (const stix_rbt_t* rbt)
{
	return rbt->size;
}

stix_rbt_pair_t* stix_rbt_search (const stix_rbt_t* rbt, const void* kptr, stix_oow_t klen)
{
	stix_rbt_pair_t* pair = rbt->root;

	while (!IS_NIL(rbt,pair))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(pair), KLEN(pair));
		if (n == 0) return pair;

		if (n > 0) pair = pair->right;
		else /* if (n < 0) */ pair = pair->left;
	}

	return STIX_NULL;
}

static void rotate (stix_rbt_t* rbt, stix_rbt_pair_t* pivot, int leftwise)
{
	/*
	 * == leftwise rotation
	 * move the pivot pair down to the poistion of the pivot's original
	 * left child(x). move the pivot's right child(y) to the pivot's original
	 * position. as 'c1' is between 'y' and 'pivot', move it to the right
	 * of the new pivot position.
	 *       parent                   parent
	 *        | | (left or right?)      | |
	 *       pivot                      y
	 *       /  \                     /  \
	 *     x     y    =====>      pivot   c2
	 *          / \               /  \
	 *         c1  c2            x   c1
	 *
	 * == rightwise rotation
	 * move the pivot pair down to the poistion of the pivot's original
	 * right child(y). move the pivot's left child(x) to the pivot's original
	 * position. as 'c2' is between 'x' and 'pivot', move it to the left
	 * of the new pivot position.
	 *
	 *       parent                   parent
	 *        | | (left or right?)      | |
	 *       pivot                      x
	 *       /  \                     /  \
	 *     x     y    =====>        c1   pivot
	 *    / \                            /  \
	 *   c1  c2                         c2   y
	 *
	 *
	 * the actual implementation here resolves the pivot's relationship to
	 * its parent by comparaing pointers as it is not known if the pivot pair
	 * is the left child or the right child of its parent,
	 */

	stix_rbt_pair_t* parent, * z, * c;
	int cid1, cid2;

	STIX_ASSERT (rbt->stix, pivot != STIX_NULL);

	if (leftwise)
	{
		cid1 = RIGHT;
		cid2 = LEFT;
	}
	else
	{
		cid1 = LEFT;
		cid2 = RIGHT;
	}

	parent = pivot->parent;
	/* y for leftwise rotation, x for rightwise rotation */
	z = pivot->child[cid1];
	/* c1 for leftwise rotation, c2 for rightwise rotation */
	c = z->child[cid2];

	z->parent = parent;
	if (parent)
	{
		if (parent->left == pivot)
		{
			parent->left = z;
		}
		else
		{
			STIX_ASSERT (rbt->stix, parent->right == pivot);
			parent->right = z;
		}
	}
	else
	{
		STIX_ASSERT (rbt->stix, rbt->root == pivot);
		rbt->root = z;
	}

	z->child[cid2] = pivot;
	if (!IS_NIL(rbt,pivot)) pivot->parent = z;

	pivot->child[cid1] = c;
	if (!IS_NIL(rbt,c)) c->parent = pivot;
}

static void adjust (stix_rbt_t* rbt, stix_rbt_pair_t* pair)
{
	while (pair != rbt->root)
	{
		stix_rbt_pair_t* tmp, * tmp2, * x_par;
		int leftwise;

		x_par = pair->parent;
		if (x_par->color == STIX_RBT_BLACK) break;

		STIX_ASSERT (rbt->stix, x_par->parent != STIX_NULL);

		if (x_par == x_par->parent->child[LEFT])
		{
			tmp = x_par->parent->child[RIGHT];
			tmp2 = x_par->child[RIGHT];
			leftwise = 1;
		}
		else
		{
			tmp = x_par->parent->child[LEFT];
			tmp2 = x_par->child[LEFT];
			leftwise = 0;
		}

		if (tmp->color == STIX_RBT_RED)
		{
			x_par->color = STIX_RBT_BLACK;
			tmp->color = STIX_RBT_BLACK;
			x_par->parent->color = STIX_RBT_RED;
			pair = x_par->parent;
		}
		else
		{
			if (pair == tmp2)
			{
				pair = x_par;
				rotate (rbt, pair, leftwise);
				x_par = pair->parent;
			}

			x_par->color = STIX_RBT_BLACK;
			x_par->parent->color = STIX_RBT_RED;
			rotate (rbt, x_par->parent, !leftwise);
		}
	}
}

static stix_rbt_pair_t* change_pair_val (
	stix_rbt_t* rbt, stix_rbt_pair_t* pair, void* vptr, stix_oow_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen)
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition.
		 * No value replacement occurs. */
		if (rbt->style->keeper != STIX_NULL)
		{
			rbt->style->keeper (rbt, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = rbt->style->copier[STIX_RBT_VAL];
		void* ovptr = VPTR(pair);
		stix_oow_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == STIX_RBT_COPIER_SIMPLE)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == STIX_RBT_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				if (vptr) STIX_MEMCPY (VPTR(pair), vptr, VTOB(rbt,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				stix_rbt_pair_t* p = stix_rbt_allocpair (rbt,
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == STIX_NULL) return STIX_NULL;

				p->color = pair->color;
				p->left = pair->left;
				p->right = pair->right;
				p->parent = pair->parent;

				if (pair->parent)
				{
					if (pair->parent->left == pair)
					{
						pair->parent->left = p;
					}
					else
					{
						STIX_ASSERT (rbt->stix, pair->parent->right == pair);
						pair->parent->right = p;
					}
				}
				if (!IS_NIL(rbt,pair->left)) pair->left->parent = p;
				if (!IS_NIL(rbt,pair->right)) pair->right->parent = p;

				if (pair == rbt->root) rbt->root = p;

				stix_rbt_freepair (rbt, pair);
				return p;
			}
		}
		else
		{
			void* nvptr = vcop (rbt, vptr, vlen);
			if (nvptr == STIX_NULL) return STIX_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (rbt->style->freeer[STIX_RBT_VAL] != STIX_NULL)
		{
			rbt->style->freeer[STIX_RBT_VAL] (rbt, ovptr, ovlen);
		}
	}

	return pair;
}

static stix_rbt_pair_t* insert (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen, int opt)
{
	stix_rbt_pair_t* x_cur = rbt->root;
	stix_rbt_pair_t* x_par = STIX_NULL;
	stix_rbt_pair_t* x_new;

	while (!IS_NIL(rbt,x_cur))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_cur), KLEN(x_cur));
		if (n == 0)
		{
			switch (opt)
			{
				case UPSERT:
				case UPDATE:
					return change_pair_val (rbt, x_cur, vptr, vlen);

				case ENSERT:
					/* return existing pair */
					return x_cur;

				case INSERT:
					/* return failure */
					return STIX_NULL;
			}
		}

		x_par = x_cur;

		if (n > 0) x_cur = x_cur->right;
		else /* if (n < 0) */ x_cur = x_cur->left;
	}

	if (opt == UPDATE) return STIX_NULL;

	x_new = stix_rbt_allocpair (rbt, kptr, klen, vptr, vlen);
	if (x_new == STIX_NULL) return STIX_NULL;

	if (x_par == STIX_NULL)
	{
		/* the tree contains no pair */
		STIX_ASSERT (rbt->stix, rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			STIX_ASSERT (rbt->stix, x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			STIX_ASSERT (rbt->stix, x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = STIX_RBT_BLACK;
	rbt->size++;
	return x_new;
}

stix_rbt_pair_t* stix_rbt_upsert (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPSERT);
}

stix_rbt_pair_t* stix_rbt_ensert (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, ENSERT);
}

stix_rbt_pair_t* stix_rbt_insert (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, INSERT);
}


stix_rbt_pair_t* stix_rbt_update (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, void* vptr, stix_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPDATE);
}

stix_rbt_pair_t* stix_rbt_cbsert (
	stix_rbt_t* rbt, void* kptr, stix_oow_t klen, cbserter_t cbserter, void* ctx)
{
	stix_rbt_pair_t* x_cur = rbt->root;
	stix_rbt_pair_t* x_par = STIX_NULL;
	stix_rbt_pair_t* x_new;

	while (!IS_NIL(rbt,x_cur))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_cur), KLEN(x_cur));
		if (n == 0)
		{
			/* back up the contents of the current pair
			 * in case it is reallocated */
			stix_rbt_pair_t tmp;

			tmp = *x_cur;

			/* call the callback function to manipulate the pair */
			x_new = cbserter (rbt, x_cur, kptr, klen, ctx);
			if (x_new == STIX_NULL)
			{
				/* error returned by the callback function */
				return STIX_NULL;
			}

			if (x_new != x_cur)
			{
				/* the current pair has been reallocated, which implicitly
				 * means the previous contents were wiped out. so the contents
				 * backed up will be used for restoration/migration */

				x_new->color = tmp.color;
				x_new->left = tmp.left;
				x_new->right = tmp.right;
				x_new->parent = tmp.parent;

				if (tmp.parent)
				{
					if (tmp.parent->left == x_cur)
					{
						tmp.parent->left = x_new;
					}
					else
					{
						STIX_ASSERT (rbt->stix, tmp.parent->right == x_cur);
						tmp.parent->right = x_new;
					}
				}
				if (!IS_NIL(rbt,tmp.left)) tmp.left->parent = x_new;
				if (!IS_NIL(rbt,tmp.right)) tmp.right->parent = x_new;

				if (x_cur == rbt->root) rbt->root = x_new;
			}

			return x_new;
		}

		x_par = x_cur;

		if (n > 0) x_cur = x_cur->right;
		else /* if (n < 0) */ x_cur = x_cur->left;
	}

	x_new = cbserter (rbt, STIX_NULL, kptr, klen, ctx);
	if (x_new == STIX_NULL) return STIX_NULL;

	if (x_par == STIX_NULL)
	{
		/* the tree contains no pair */
		STIX_ASSERT (rbt->stix, rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			STIX_ASSERT (rbt->stix, x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			STIX_ASSERT (rbt->stix, x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = STIX_RBT_BLACK;
	rbt->size++;
	return x_new;
}


static void adjust_for_delete (stix_rbt_t* rbt, stix_rbt_pair_t* pair, stix_rbt_pair_t* par)
{
	while (pair != rbt->root && pair->color == STIX_RBT_BLACK)
	{
		stix_rbt_pair_t* tmp;

		if (pair == par->left)
		{
			tmp = par->right;
			if (tmp->color == STIX_RBT_RED)
			{
				tmp->color = STIX_RBT_BLACK;
				par->color = STIX_RBT_RED;
				rotate_left (rbt, par);
				tmp = par->right;
			}

			if (tmp->left->color == STIX_RBT_BLACK &&
			    tmp->right->color == STIX_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = STIX_RBT_RED;
				pair = par;
				par = pair->parent;
			}
			else
			{
				if (tmp->right->color == STIX_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->left))
						tmp->left->color = STIX_RBT_BLACK;
					tmp->color = STIX_RBT_RED;
					rotate_right (rbt, tmp);
					tmp = par->right;
				}

				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = STIX_RBT_BLACK;
				if (tmp->right->color == STIX_RBT_RED)
					tmp->right->color = STIX_RBT_BLACK;

				rotate_left (rbt, par);
				pair = rbt->root;
			}
		}
		else
		{
			STIX_ASSERT (rbt->stix, pair == par->right);
			tmp = par->left;
			if (tmp->color == STIX_RBT_RED)
			{
				tmp->color = STIX_RBT_BLACK;
				par->color = STIX_RBT_RED;
				rotate_right (rbt, par);
				tmp = par->left;
			}

			if (tmp->left->color == STIX_RBT_BLACK &&
			    tmp->right->color == STIX_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = STIX_RBT_RED;
				pair = par;
				par = pair->parent;
			}
			else
			{
				if (tmp->left->color == STIX_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->right))
						tmp->right->color = STIX_RBT_BLACK;
					tmp->color = STIX_RBT_RED;
					rotate_left (rbt, tmp);
					tmp = par->left;
				}
				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = STIX_RBT_BLACK;
				if (tmp->left->color == STIX_RBT_RED)
					tmp->left->color = STIX_RBT_BLACK;

				rotate_right (rbt, par);
				pair = rbt->root;
			}
		}
	}

	pair->color = STIX_RBT_BLACK;
}

static void delete_pair (stix_rbt_t* rbt, stix_rbt_pair_t* pair)
{
	stix_rbt_pair_t* x, * y, * par;

	STIX_ASSERT (rbt->stix, pair && !IS_NIL(rbt,pair));

	if (IS_NIL(rbt,pair->left) || IS_NIL(rbt,pair->right))
	{
		y = pair;
	}
	else
	{
		/* find a successor with NIL as a child */
		y = pair->right;
		while (!IS_NIL(rbt,y->left)) y = y->left;
	}

	x = IS_NIL(rbt,y->left)? y->right: y->left;

	par = y->parent;
	if (!IS_NIL(rbt,x)) x->parent = par;

	if (par)
	{
		if (y == par->left)
			par->left = x;
		else
			par->right = x;
	}
	else
	{
		rbt->root = x;
	}

	if (y == pair)
	{
		if (y->color == STIX_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

		stix_rbt_freepair (rbt, y);
	}
	else
	{
		if (y->color == STIX_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

#if 1
		if (pair->parent)
		{
			if (pair->parent->left == pair) pair->parent->left = y;
			if (pair->parent->right == pair) pair->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		y->parent = pair->parent;
		y->left = pair->left;
		y->right = pair->right;
		y->color = pair->color;

		if (pair->left->parent == pair) pair->left->parent = y;
		if (pair->right->parent == pair) pair->right->parent = y;
#else
		*y = *pair;
		if (y->parent)
		{
			if (y->parent->left == pair) y->parent->left = y;
			if (y->parent->right == pair) y->parent->right = y;
		}
		else
		{
			rbt->root = y;
		}

		if (y->left->parent == pair) y->left->parent = y;
		if (y->right->parent == pair) y->right->parent = y;
#endif

		stix_rbt_freepair (rbt, pair);
	}

	rbt->size--;
}

int stix_rbt_delete (stix_rbt_t* rbt, const void* kptr, stix_oow_t klen)
{
	stix_rbt_pair_t* pair;

	pair = stix_rbt_search (rbt, kptr, klen);
	if (pair == STIX_NULL) return -1;

	delete_pair (rbt, pair);
	return 0;
}

void stix_rbt_clear (stix_rbt_t* rbt)
{
	/* TODO: improve this */
	while (!IS_NIL(rbt,rbt->root)) delete_pair (rbt, rbt->root);
}

#if 0
static STIX_INLINE stix_rbt_walk_t walk_recursively (
	stix_rbt_t* rbt, walker_t walker, void* ctx, stix_rbt_pair_t* pair)
{
	if (!IS_NIL(rbt,pair->left))
	{
		if (walk_recursively (rbt, walker, ctx, pair->left) == STIX_RBT_WALK_STOP)
			return STIX_RBT_WALK_STOP;
	}

	if (walker (rbt, pair, ctx) == STIX_RBT_WALK_STOP) return STIX_RBT_WALK_STOP;

	if (!IS_NIL(rbt,pair->right))
	{
		if (walk_recursively (rbt, walker, ctx, pair->right) == STIX_RBT_WALK_STOP)
			return STIX_RBT_WALK_STOP;
	}

	return STIX_RBT_WALK_FORWARD;
}
#endif

static STIX_INLINE void walk (stix_rbt_t* rbt, walker_t walker, void* ctx, int l, int r)
{
	stix_rbt_pair_t* x_cur = rbt->root;
	stix_rbt_pair_t* prev = rbt->root->parent;

	while (x_cur && !IS_NIL(rbt,x_cur))
	{
		if (prev == x_cur->parent)
		{
			/* the previous node is the parent of the current node.
			 * it indicates that we're going down to the child[l] */
			if (!IS_NIL(rbt,x_cur->child[l]))
			{
				/* go to the child[l] child */
				prev = x_cur;
				x_cur = x_cur->child[l];
			}
			else
			{
				if (walker (rbt, x_cur, ctx) == STIX_RBT_WALK_STOP) break;

				if (!IS_NIL(rbt,x_cur->child[r]))
				{
					/* go down to the right node if exists */
					prev = x_cur;
					x_cur = x_cur->child[r];
				}
				else
				{
					/* otherwise, move up to the parent */
					prev = x_cur;
					x_cur = x_cur->parent;
				}
			}
		}
		else if (prev == x_cur->child[l])
		{
			/* the left child has been already traversed */

			if (walker (rbt, x_cur, ctx) == STIX_RBT_WALK_STOP) break;

			if (!IS_NIL(rbt,x_cur->child[r]))
			{
				/* go down to the right node if it exists */
				prev = x_cur;
				x_cur = x_cur->child[r];
			}
			else
			{
				/* otherwise, move up to the parent */
				prev = x_cur;
				x_cur = x_cur->parent;
			}
		}
		else
		{
			/* both the left child and the right child have been traversed */
			STIX_ASSERT (rbt->stix, prev == x_cur->child[r]);
			/* just move up to the parent */
			prev = x_cur;
			x_cur = x_cur->parent;
		}
	}
}

void stix_rbt_walk (stix_rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, LEFT, RIGHT);
}

void stix_rbt_rwalk (stix_rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, RIGHT, LEFT);
}

int stix_rbt_dflcomp (const stix_rbt_t* rbt, const void* kptr1, stix_oow_t klen1, const void* kptr2, stix_oow_t klen2)
{
	stix_oow_t min;
	int n, nn;

	if (klen1 < klen2)
	{
		min = klen1;
		nn = -1;
	}
	else
	{
		min = klen2;
		nn = (klen1 == klen2)? 0: 1;
	}

	n = STIX_MEMCMP (kptr1, kptr2, KTOB(rbt,min));
	if (n == 0) n = nn;
	return n;
}

