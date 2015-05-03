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

static void cleanup_symbols_for_gc (stix_t* stix, stix_oop_t _nil)
{
#if 0
	stix_oop_oop_t buc;
	stix_oop_char_t sym;
	stix_oow_t tally, index, i, x, y, z;

	tally = STIX_OOP_TO_SMINT (stix->symtab->slot[STIX_SYMTAB_TALLY]);
	if (tally <= 0) return;

	buc = (stix_oop_oop_t) stix->symtab->slot[STIX_SYMTAB_BUCKET];
	for (index = 0; index < buc->size; )
	{
		if (buc->slot[index]->flags & STIX_OBJ_FLAG_MOVED) 
		{
			index++;
			continue;
		}

		STIX_ASSERT (buc->slot[index] != _nil);

{
sym = (stix_oop_char_t)buc->slot[index];
wprintf (L">> DISPOSING %d [%S] from the symbol table\n", (int)index, sym->slot);
}
		for (i = 0, x = index, y = index; i < buc->size; i++)
		{
			y = (y + 1) % buc->size;

			/* done if the slot at the current hash index is _nil */
			if (buc->slot[y] == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			sym = (stix_oop_char_t)buc->slot[y];

			STIX_ASSERT (STIX_CLASSOF(stix,sym) == (stix_oop_t)stix->cc.symbol);

			z = hash_chars (sym->slot, sym->size) % buc->size;

			/* move an element if necessary */
			if ((y > x && (z <= x || z > y)) ||
			    (y < x && (z <= x && z > y)))
			{
				buc->slot[x] = buc->slot[y];
				x = y;
			}
		}

		buc->slot[x] = _nil;
		tally--;
	}

	stix->symtab->slot[STIX_SYMTAB_TALLY] = STIX_OOP_FROM_SMINT(tally);
#endif
}

static stix_oop_t move_one (stix_t* stix, stix_oop_t oop)
{
	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));

	if (STIX_OBJ_GET_FLAGS_MOVED(oop))
	{
		/* this object has migrated to the new heap. 
		 * the class field has been updated to the new object
		 * in the 'else' block below. i can simply return it
		 * without further migration. */
		return oop->_class;
	}
	else
	{
		stix_oow_t nbytes, nbytes_aligned;
		stix_oop_t tmp;

		/* calculate the payload size in bytes */
		nbytes = (oop->_size + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop);
		nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));

		/* allocate space in the new heap */
		tmp = stix_allocheapmem (stix, stix->newheap, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);

		/* allocation here must not fail because
		 * i'm allocating the new space in a new heap for 
		 * moving an existing object in the current heap. 
		 *
		 * assuming the new heap is as large as the old heap,
		 * and garbage collection doesn't allocate more objects
		 * than in the old heap, it must not fail. */
		STIX_ASSERT (tmp != STIX_NULL); 

		/* copy the payload to the new object */
		STIX_MEMCPY (tmp, oop, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);

		/* mark the old object that it has migrated to the new heap */
		STIX_OBJ_SET_FLAGS_MOVED(oop, 1);

		/* let the class field of the old object point to the new 
		 * object allocated in the new heap. it is returned in 
		 * the 'if' block at the top of this function. */
		oop->_class = tmp;
		
		/* return the new object */
		return tmp;
	}
}

static stix_uint8_t* scan_new_heap (stix_t* stix, stix_uint8_t* ptr)
{
	while (ptr < stix->newheap->ptr)
	{
		stix_oow_t i;
		stix_oow_t nbytes, nbytes_aligned;
		stix_oop_t oop;

		oop = (stix_oop_t)ptr;
		nbytes = (oop->_size + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop);
		nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));

		oop->_class = move_one (stix, oop->_class);

		if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP)
		{
			stix_obj_oop_t* xtmp;

			xtmp = (stix_oop_oop_t)oop;
			for (i = 0; i < oop->_size; i++)
			{
				if (STIX_OOP_IS_POINTER(xtmp->slot[i]))
					xtmp->slot[i] = move_one (stix, xtmp->slot[i]);
			}
		}
		
/*wprintf (L"ptr in gc => %p size => %d, aligned size => %d\n", ptr, (int)nbytes, (int)nbytes_aligned);*/
		ptr = ptr + STIX_SIZEOF(stix_obj_t) + nbytes_aligned;
	}

	/* return the pointer to the beginning of the free space in the heap */
	return ptr; 
}

void stix_gc (stix_t* stix)
{
	/* 
	 * move a referenced object to the new heap.
	 * inspect the fields of the moved object in the new heap.
	 * move objects pointed to by the fields to the new heap.
	 * finally perform some tricky symbol table clean-up.
	 */

	stix_uint8_t* ptr;
	stix_heap_t* tmp;
	stix_oop_t old__nil;

	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */

	old__nil = stix->_nil;

	/* move _nil and the root object table */
	stix->_nil = move_one (stix, stix->_nil);
	stix->root = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->root);
	stix->_true = move_one (stix, stix->_true);
	stix->_false = move_one (stix, stix->_false);

	stix->cc.array = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.array);
	stix->cc.association = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.association);
	/*stix->cc.metaclass = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.metaclass);*/
	stix->cc.string = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.string);
	stix->cc.symbol = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.symbol);
	stix->cc.sysdic = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.sysdic);
	stix->cc.numeric[0] = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.numeric[0]);
	stix->cc.numeric[1] = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->cc.numeric[1]);

	/* scan the new heap to move referenced objects */
	ptr = (stix_uint8_t*) STIX_ALIGN ((stix_uintptr_t)stix->newheap->base, STIX_SIZEOF(stix_oop_t));
	ptr = scan_new_heap (stix, ptr);

	/* traverse the symbol table for unreferenced symbols.
	 * if the symbol has not moved to the new heap, the symbol
	 * is not referenced by any other objects than the symbol 
	 * table itself */
	cleanup_symbols_for_gc (stix, old__nil);

	/* move the symbol table itself */
	stix->symtab = (stix_oop_oop_t) move_one (stix, (stix_oop_t)stix->symtab);

	/* scan the new heap again from the end position of
	 * the previous scan to move referenced objects by 
	 * the symbol table. */
	ptr = scan_new_heap (stix, ptr);

	/* swap the current heap and old heap */
	tmp = stix->curheap;
	stix->curheap = stix->newheap;
	stix->newheap = tmp;

/*
{
stix_oow_t index;
stix_oop_oop_t buc;
printf ("=== SURVIVING SYMBOLS ===\n");
buc = (stix_oop_oop_t) stix->symtab->slot[STIX_SYMTAB_BUCKET];
for (index = 0; index < buc->size; index++)
{
	if ((stix_oop_t)buc->slot[index] != stix->_nil) 
	{
		const stix_oop_char_t* p = ((stix_oop_char_t)buc->slot[index])->slot;
		printf ("SYM [");
		while (*p) printf ("%c", *p++);
		printf ("]\n");
	}
}
printf ("===========================\n");
}
*/
}
