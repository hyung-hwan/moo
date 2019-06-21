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

#include "moo-rbt.h"
#include "moo-prv.h"

#define copier_t        moo_rbt_copier_t
#define freeer_t        moo_rbt_freeer_t
#define comper_t        moo_rbt_comper_t
#define keeper_t        moo_rbt_keeper_t
#define walker_t        moo_rbt_walker_t
#define cbserter_t      moo_rbt_cbserter_t

#define KPTR(p)  MOO_RBT_KPTR(p)
#define KLEN(p)  MOO_RBT_KLEN(p)
#define VPTR(p)  MOO_RBT_VPTR(p)
#define VLEN(p)  MOO_RBT_VLEN(p)

#define KTOB(rbt,len) ((len)*(rbt)->scale[MOO_RBT_KEY])
#define VTOB(rbt,len) ((len)*(rbt)->scale[MOO_RBT_VAL])

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

MOO_INLINE moo_rbt_pair_t* moo_rbt_allocpair (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen)
{
	moo_rbt_pair_t* pair;

	copier_t kcop = rbt->style->copier[MOO_RBT_KEY];
	copier_t vcop = rbt->style->copier[MOO_RBT_VAL];

	moo_oow_t as = MOO_SIZEOF(moo_rbt_pair_t);
	if (kcop == MOO_RBT_COPIER_INLINE) as += MOO_ALIGN_POW2(KTOB(rbt,klen), MOO_SIZEOF_VOID_P);
	if (vcop == MOO_RBT_COPIER_INLINE) as += VTOB(rbt,vlen);

	pair = (moo_rbt_pair_t*)moo_allocmem(rbt->moo, as);
	if (!pair) return MOO_NULL;

	pair->color = MOO_RBT_RED;
	pair->parent = MOO_NULL;
	pair->child[LEFT] = &rbt->xnil;
	pair->child[RIGHT] = &rbt->xnil;

	KLEN(pair) = klen;
	if (kcop == MOO_RBT_COPIER_SIMPLE)
	{
		KPTR(pair) = kptr;
	}
	else if (kcop == MOO_RBT_COPIER_INLINE)
	{
		KPTR(pair) = pair + 1;
		if (kptr) MOO_MEMCPY (KPTR(pair), kptr, KTOB(rbt,klen));
	}
	else
	{
		KPTR(pair) = kcop(rbt, kptr, klen);
		if (KPTR(pair) == MOO_NULL)
		{
			moo_freemem (rbt->moo, pair);
			return MOO_NULL;
		}
	}

	VLEN(pair) = vlen;
	if (vcop == MOO_RBT_COPIER_SIMPLE)
	{
		VPTR(pair) = vptr;
	}
	else if (vcop == MOO_RBT_COPIER_INLINE)
	{
		VPTR(pair) = pair + 1;
		if (kcop == MOO_RBT_COPIER_INLINE)
			VPTR(pair) = (moo_oob_t*)VPTR(pair) + MOO_ALIGN_POW2(KTOB(rbt,klen), MOO_SIZEOF_VOID_P);
		if (vptr) MOO_MEMCPY (VPTR(pair), vptr, VTOB(rbt,vlen));
	}
	else
	{
		VPTR(pair) = vcop (rbt, vptr, vlen);
		if (VPTR(pair) != MOO_NULL)
		{
			if (rbt->style->freeer[MOO_RBT_KEY] != MOO_NULL)
				rbt->style->freeer[MOO_RBT_KEY] (rbt, KPTR(pair), KLEN(pair));
			moo_freemem (rbt->moo, pair);
			return MOO_NULL;
		}
	}

	return pair;
}

MOO_INLINE void moo_rbt_freepair (moo_rbt_t* rbt, moo_rbt_pair_t* pair)
{
	if (rbt->style->freeer[MOO_RBT_KEY] != MOO_NULL)
		rbt->style->freeer[MOO_RBT_KEY] (rbt, KPTR(pair), KLEN(pair));
	if (rbt->style->freeer[MOO_RBT_VAL] != MOO_NULL)
		rbt->style->freeer[MOO_RBT_VAL] (rbt, VPTR(pair), VLEN(pair));
	moo_freemem (rbt->moo, pair);
}

static moo_rbt_style_t style[] =
{
	{
		{
			MOO_RBT_COPIER_DEFAULT,
			MOO_RBT_COPIER_DEFAULT
		},
		{
			MOO_RBT_FREEER_DEFAULT,
			MOO_RBT_FREEER_DEFAULT
		},
		MOO_RBT_COMPER_DEFAULT,
		MOO_RBT_KEEPER_DEFAULT
	},

	{
		{
			MOO_RBT_COPIER_INLINE,
			MOO_RBT_COPIER_INLINE
		},
		{
			MOO_RBT_FREEER_DEFAULT,
			MOO_RBT_FREEER_DEFAULT
		},
		MOO_RBT_COMPER_DEFAULT,
		MOO_RBT_KEEPER_DEFAULT
	},

	{
		{
			MOO_RBT_COPIER_INLINE,
			MOO_RBT_COPIER_DEFAULT
		},
		{
			MOO_RBT_FREEER_DEFAULT,
			MOO_RBT_FREEER_DEFAULT
		},
		MOO_RBT_COMPER_DEFAULT,
		MOO_RBT_KEEPER_DEFAULT
	},

	{
		{
			MOO_RBT_COPIER_DEFAULT,
			MOO_RBT_COPIER_INLINE
		},
		{
			MOO_RBT_FREEER_DEFAULT,
			MOO_RBT_FREEER_DEFAULT
		},
		MOO_RBT_COMPER_DEFAULT,
		MOO_RBT_KEEPER_DEFAULT
	}
};

const moo_rbt_style_t* moo_getrbtstyle (moo_rbt_style_kind_t kind)
{
	return &style[kind];
}

moo_rbt_t* moo_rbt_open (moo_t* moo, moo_oow_t xtnsize, int kscale, int vscale)
{
	moo_rbt_t* rbt;

	rbt = (moo_rbt_t*)moo_allocmem(moo, MOO_SIZEOF(moo_rbt_t) + xtnsize);
	if (!rbt) return MOO_NULL;

	if (moo_rbt_init(rbt, moo, kscale, vscale) <= -1)
	{
		moo_freemem (moo, rbt);
		return MOO_NULL;
	}

	MOO_MEMSET (rbt + 1, 0, xtnsize);
	return rbt;
}

void moo_rbt_close (moo_rbt_t* rbt)
{
	moo_rbt_fini (rbt);
	moo_freemem (rbt->moo, rbt);
}

int moo_rbt_init (moo_rbt_t* rbt, moo_t* moo, int kscale, int vscale)
{
	/* do not zero out the extension */
	MOO_MEMSET (rbt, 0, MOO_SIZEOF(*rbt));
	rbt->moo = moo;

	rbt->scale[MOO_RBT_KEY] = (kscale < 1)? 1: kscale;
	rbt->scale[MOO_RBT_VAL] = (vscale < 1)? 1: vscale;
	rbt->size = 0;

	rbt->style = &style[0];

	/* self-initializing nil */
	MOO_MEMSET(&rbt->xnil, 0, MOO_SIZEOF(rbt->xnil));
	rbt->xnil.color = MOO_RBT_BLACK;
	rbt->xnil.left = &rbt->xnil;
	rbt->xnil.right = &rbt->xnil;

	/* root is set to nil initially */
	rbt->root = &rbt->xnil;

	return 0;
}

void moo_rbt_fini (moo_rbt_t* rbt)
{
	moo_rbt_clear (rbt);
}

void* moo_rbt_getxtn (moo_rbt_t* rbt)
{
	return (void*)(rbt + 1);
}

const moo_rbt_style_t* moo_rbt_getstyle (const moo_rbt_t* rbt)
{
	return rbt->style;
}

void moo_rbt_setstyle (moo_rbt_t* rbt, const moo_rbt_style_t* style)
{
	MOO_ASSERT (rbt->moo, style != MOO_NULL);
	rbt->style = style;
}

moo_oow_t moo_rbt_getsize (const moo_rbt_t* rbt)
{
	return rbt->size;
}

moo_rbt_pair_t* moo_rbt_search (const moo_rbt_t* rbt, const void* kptr, moo_oow_t klen)
{
	moo_rbt_pair_t* pair = rbt->root;

	while (!IS_NIL(rbt,pair))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(pair), KLEN(pair));
		if (n == 0) return pair;

		if (n > 0) pair = pair->right;
		else /* if (n < 0) */ pair = pair->left;
	}

	return MOO_NULL;
}

static void rotate (moo_rbt_t* rbt, moo_rbt_pair_t* pivot, int leftwise)
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

	moo_rbt_pair_t* parent, * z, * c;
	int cid1, cid2;

	MOO_ASSERT (rbt->moo, pivot != MOO_NULL);

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
			MOO_ASSERT (rbt->moo, parent->right == pivot);
			parent->right = z;
		}
	}
	else
	{
		MOO_ASSERT (rbt->moo, rbt->root == pivot);
		rbt->root = z;
	}

	z->child[cid2] = pivot;
	if (!IS_NIL(rbt,pivot)) pivot->parent = z;

	pivot->child[cid1] = c;
	if (!IS_NIL(rbt,c)) c->parent = pivot;
}

static void adjust (moo_rbt_t* rbt, moo_rbt_pair_t* pair)
{
	while (pair != rbt->root)
	{
		moo_rbt_pair_t* tmp, * tmp2, * x_par;
		int leftwise;

		x_par = pair->parent;
		if (x_par->color == MOO_RBT_BLACK) break;

		MOO_ASSERT (rbt->moo, x_par->parent != MOO_NULL);

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

		if (tmp->color == MOO_RBT_RED)
		{
			x_par->color = MOO_RBT_BLACK;
			tmp->color = MOO_RBT_BLACK;
			x_par->parent->color = MOO_RBT_RED;
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

			x_par->color = MOO_RBT_BLACK;
			x_par->parent->color = MOO_RBT_RED;
			rotate (rbt, x_par->parent, !leftwise);
		}
	}
}

static moo_rbt_pair_t* change_pair_val (
	moo_rbt_t* rbt, moo_rbt_pair_t* pair, void* vptr, moo_oow_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen)
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition.
		 * No value replacement occurs. */
		if (rbt->style->keeper != MOO_NULL)
		{
			rbt->style->keeper (rbt, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = rbt->style->copier[MOO_RBT_VAL];
		void* ovptr = VPTR(pair);
		moo_oow_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == MOO_RBT_COPIER_SIMPLE)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == MOO_RBT_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				if (vptr) MOO_MEMCPY (VPTR(pair), vptr, VTOB(rbt,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				moo_rbt_pair_t* p = moo_rbt_allocpair (rbt,
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == MOO_NULL) return MOO_NULL;

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
						MOO_ASSERT (rbt->moo, pair->parent->right == pair);
						pair->parent->right = p;
					}
				}
				if (!IS_NIL(rbt,pair->left)) pair->left->parent = p;
				if (!IS_NIL(rbt,pair->right)) pair->right->parent = p;

				if (pair == rbt->root) rbt->root = p;

				moo_rbt_freepair (rbt, pair);
				return p;
			}
		}
		else
		{
			void* nvptr = vcop (rbt, vptr, vlen);
			if (nvptr == MOO_NULL) return MOO_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (rbt->style->freeer[MOO_RBT_VAL] != MOO_NULL)
		{
			rbt->style->freeer[MOO_RBT_VAL] (rbt, ovptr, ovlen);
		}
	}

	return pair;
}

static moo_rbt_pair_t* insert (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen, int opt)
{
	moo_rbt_pair_t* x_cur = rbt->root;
	moo_rbt_pair_t* x_par = MOO_NULL;
	moo_rbt_pair_t* x_new;

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
					return MOO_NULL;
			}
		}

		x_par = x_cur;

		if (n > 0) x_cur = x_cur->right;
		else /* if (n < 0) */ x_cur = x_cur->left;
	}

	if (opt == UPDATE) return MOO_NULL;

	x_new = moo_rbt_allocpair (rbt, kptr, klen, vptr, vlen);
	if (x_new == MOO_NULL) return MOO_NULL;

	if (x_par == MOO_NULL)
	{
		/* the tree contains no pair */
		MOO_ASSERT (rbt->moo, rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			MOO_ASSERT (rbt->moo, x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			MOO_ASSERT (rbt->moo, x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = MOO_RBT_BLACK;
	rbt->size++;
	return x_new;
}

moo_rbt_pair_t* moo_rbt_upsert (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPSERT);
}

moo_rbt_pair_t* moo_rbt_ensert (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, ENSERT);
}

moo_rbt_pair_t* moo_rbt_insert (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, INSERT);
}


moo_rbt_pair_t* moo_rbt_update (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, void* vptr, moo_oow_t vlen)
{
	return insert (rbt, kptr, klen, vptr, vlen, UPDATE);
}

moo_rbt_pair_t* moo_rbt_cbsert (
	moo_rbt_t* rbt, void* kptr, moo_oow_t klen, cbserter_t cbserter, void* ctx)
{
	moo_rbt_pair_t* x_cur = rbt->root;
	moo_rbt_pair_t* x_par = MOO_NULL;
	moo_rbt_pair_t* x_new;

	while (!IS_NIL(rbt,x_cur))
	{
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_cur), KLEN(x_cur));
		if (n == 0)
		{
			/* back up the contents of the current pair
			 * in case it is reallocated */
			moo_rbt_pair_t tmp;

			tmp = *x_cur;

			/* call the callback function to manipulate the pair */
			x_new = cbserter (rbt, x_cur, kptr, klen, ctx);
			if (x_new == MOO_NULL)
			{
				/* error returned by the callback function */
				return MOO_NULL;
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
						MOO_ASSERT (rbt->moo, tmp.parent->right == x_cur);
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

	x_new = cbserter (rbt, MOO_NULL, kptr, klen, ctx);
	if (x_new == MOO_NULL) return MOO_NULL;

	if (x_par == MOO_NULL)
	{
		/* the tree contains no pair */
		MOO_ASSERT (rbt->moo, rbt->root == &rbt->xnil);
		rbt->root = x_new;
	}
	else
	{
		/* perform normal binary insert */
		int n = rbt->style->comper (rbt, kptr, klen, KPTR(x_par), KLEN(x_par));
		if (n > 0)
		{
			MOO_ASSERT (rbt->moo, x_par->right == &rbt->xnil);
			x_par->right = x_new;
		}
		else
		{
			MOO_ASSERT (rbt->moo, x_par->left == &rbt->xnil);
			x_par->left = x_new;
		}

		x_new->parent = x_par;
		adjust (rbt, x_new);
	}

	rbt->root->color = MOO_RBT_BLACK;
	rbt->size++;
	return x_new;
}


static void adjust_for_delete (moo_rbt_t* rbt, moo_rbt_pair_t* pair, moo_rbt_pair_t* par)
{
	while (pair != rbt->root && pair->color == MOO_RBT_BLACK)
	{
		moo_rbt_pair_t* tmp;

		if (pair == par->left)
		{
			tmp = par->right;
			if (tmp->color == MOO_RBT_RED)
			{
				tmp->color = MOO_RBT_BLACK;
				par->color = MOO_RBT_RED;
				rotate_left (rbt, par);
				tmp = par->right;
			}

			if (tmp->left->color == MOO_RBT_BLACK &&
			    tmp->right->color == MOO_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = MOO_RBT_RED;
				pair = par;
				par = pair->parent;
			}
			else
			{
				if (tmp->right->color == MOO_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->left))
						tmp->left->color = MOO_RBT_BLACK;
					tmp->color = MOO_RBT_RED;
					rotate_right (rbt, tmp);
					tmp = par->right;
				}

				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = MOO_RBT_BLACK;
				if (tmp->right->color == MOO_RBT_RED)
					tmp->right->color = MOO_RBT_BLACK;

				rotate_left (rbt, par);
				pair = rbt->root;
			}
		}
		else
		{
			MOO_ASSERT (rbt->moo, pair == par->right);
			tmp = par->left;
			if (tmp->color == MOO_RBT_RED)
			{
				tmp->color = MOO_RBT_BLACK;
				par->color = MOO_RBT_RED;
				rotate_right (rbt, par);
				tmp = par->left;
			}

			if (tmp->left->color == MOO_RBT_BLACK &&
			    tmp->right->color == MOO_RBT_BLACK)
			{
				if (!IS_NIL(rbt,tmp)) tmp->color = MOO_RBT_RED;
				pair = par;
				par = pair->parent;
			}
			else
			{
				if (tmp->left->color == MOO_RBT_BLACK)
				{
					if (!IS_NIL(rbt,tmp->right))
						tmp->right->color = MOO_RBT_BLACK;
					tmp->color = MOO_RBT_RED;
					rotate_left (rbt, tmp);
					tmp = par->left;
				}
				tmp->color = par->color;
				if (!IS_NIL(rbt,par)) par->color = MOO_RBT_BLACK;
				if (tmp->left->color == MOO_RBT_RED)
					tmp->left->color = MOO_RBT_BLACK;

				rotate_right (rbt, par);
				pair = rbt->root;
			}
		}
	}

	pair->color = MOO_RBT_BLACK;
}

static void delete_pair (moo_rbt_t* rbt, moo_rbt_pair_t* pair)
{
	moo_rbt_pair_t* x, * y, * par;

	MOO_ASSERT (rbt->moo, pair && !IS_NIL(rbt,pair));

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
		if (y->color == MOO_RBT_BLACK && !IS_NIL(rbt,x))
			adjust_for_delete (rbt, x, par);

		moo_rbt_freepair (rbt, y);
	}
	else
	{
		if (y->color == MOO_RBT_BLACK && !IS_NIL(rbt,x))
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

		moo_rbt_freepair (rbt, pair);
	}

	rbt->size--;
}

int moo_rbt_delete (moo_rbt_t* rbt, const void* kptr, moo_oow_t klen)
{
	moo_rbt_pair_t* pair;

	pair = moo_rbt_search (rbt, kptr, klen);
	if (pair == MOO_NULL) return -1;

	delete_pair (rbt, pair);
	return 0;
}

void moo_rbt_clear (moo_rbt_t* rbt)
{
	/* TODO: improve this */
	while (!IS_NIL(rbt,rbt->root)) delete_pair (rbt, rbt->root);
}

#if 0
static MOO_INLINE moo_rbt_walk_t walk_recursively (
	moo_rbt_t* rbt, walker_t walker, void* ctx, moo_rbt_pair_t* pair)
{
	if (!IS_NIL(rbt,pair->left))
	{
		if (walk_recursively (rbt, walker, ctx, pair->left) == MOO_RBT_WALK_STOP)
			return MOO_RBT_WALK_STOP;
	}

	if (walker (rbt, pair, ctx) == MOO_RBT_WALK_STOP) return MOO_RBT_WALK_STOP;

	if (!IS_NIL(rbt,pair->right))
	{
		if (walk_recursively (rbt, walker, ctx, pair->right) == MOO_RBT_WALK_STOP)
			return MOO_RBT_WALK_STOP;
	}

	return MOO_RBT_WALK_FORWARD;
}
#endif

static MOO_INLINE void walk (moo_rbt_t* rbt, walker_t walker, void* ctx, int l, int r)
{
	moo_rbt_pair_t* x_cur = rbt->root;
	moo_rbt_pair_t* prev = rbt->root->parent;

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
				if (walker (rbt, x_cur, ctx) == MOO_RBT_WALK_STOP) break;

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

			if (walker (rbt, x_cur, ctx) == MOO_RBT_WALK_STOP) break;

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
			MOO_ASSERT (rbt->moo, prev == x_cur->child[r]);
			/* just move up to the parent */
			prev = x_cur;
			x_cur = x_cur->parent;
		}
	}
}

void moo_rbt_walk (moo_rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, LEFT, RIGHT);
}

void moo_rbt_rwalk (moo_rbt_t* rbt, walker_t walker, void* ctx)
{
	walk (rbt, walker, ctx, RIGHT, LEFT);
}

int moo_rbt_dflcomp (const moo_rbt_t* rbt, const void* kptr1, moo_oow_t klen1, const void* kptr2, moo_oow_t klen2)
{
	moo_oow_t min;
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

	n = MOO_MEMCMP (kptr1, kptr2, KTOB(rbt,min));
	if (n == 0) n = nn;
	return n;
}

