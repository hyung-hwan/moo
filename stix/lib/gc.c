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

static void compact_symbol_table (stix_t* stix, stix_oop_t _nil)
{
	stix_oop_char_t symbol;
	stix_oow_t i, x, y, z;
	stix_oow_t bucket_size, index;
	stix_ooi_t tally;

#if defined(STIX_SUPPORT_GC_DURING_IGNITION)
	if (!stix->symtab) return; /* symbol table has not been created */
#endif

	/* the symbol table doesn't allow more data items than STIX_SMOOI_MAX.
	 * so stix->symtab->tally must always be a small integer */
	STIX_ASSERT (STIX_OOP_IS_SMOOI(stix->symtab->tally));
	tally = STIX_OOP_TO_SMOOI(stix->symtab->tally);
	STIX_ASSERT (tally >= 0); /* it must not be less than 0 */
	if (tally <= 0) return;

	/* NOTE: in theory, the bucket size can be greater than STIX_SMOOI_MAX
	 * as it is an internal header field and is of an unsigned type */
	bucket_size = STIX_OBJ_GET_SIZE(stix->symtab->bucket);

	for (index = 0; index < bucket_size; )
	{
		if (STIX_OBJ_GET_FLAGS_MOVED(stix->symtab->bucket->slot[index]))
		{
			index++;
			continue;
		}

		STIX_ASSERT (stix->symtab->bucket->slot[index] != _nil);

		for (i = 0, x = index, y = index; i < bucket_size; i++)
		{
			y = (y + 1) % bucket_size;

			/* done if the slot at the current hash index is _nil */
			if (stix->symtab->bucket->slot[y] == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			symbol = (stix_oop_char_t)stix->symtab->bucket->slot[y];

			STIX_ASSERT (STIX_CLASSOF(stix,symbol) == stix->_symbol);

			z = stix_hashchars(symbol->slot, STIX_OBJ_GET_SIZE(symbol)) % bucket_size;

			/* move an element if necessary */
			if ((y > x && (z <= x || z > y)) ||
			    (y < x && (z <= x && z > y)))
			{
				stix->symtab->bucket->slot[x] = stix->symtab->bucket->slot[y];
				x = y;
			}
		}

		stix->symtab->bucket->slot[x] = _nil;
		tally--;
	}

	STIX_ASSERT (tally >= 0);
	STIX_ASSERT (tally <= STIX_SMOOI_MAX);
	stix->symtab->tally = STIX_SMOOI_TO_OOP(tally);
}


static STIX_INLINE stix_oow_t get_payload_bytes (stix_t* stix, stix_oop_t oop)
{
	stix_oow_t nbytes_aligned;

#if defined(STIX_USE_OBJECT_TRAILER)
	if (STIX_OBJ_GET_FLAGS_TRAILER(oop))
	{
		stix_oow_t nbytes;

		/* only an OOP object can have the trailer. 
		 *
		 * | _flags    |
		 * | _size     |  <-- if it's 3
		 * | _class    |
		 * |   X       |
		 * |   X       |
		 * |   X       |
		 * |   Y       | <-- it may exist if EXTRA is set in _flags.
		 * |   Z       | <-- if TRAILER is set, it is the number of bytes in the trailer
		 * |  |  |  |  | 
		 */
		STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP);
		STIX_ASSERT (STIX_OBJ_GET_FLAGS_UNIT(oop) == STIX_SIZEOF(stix_oow_t));
		STIX_ASSERT (STIX_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

		nbytes = STIX_OBJ_BYTESOF(oop) + STIX_SIZEOF(stix_oow_t) + \
		         (stix_oow_t)((stix_oop_oop_t)oop)->slot[STIX_OBJ_GET_SIZE(oop)];
		nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));
	}
	else
	{
#endif
		/* calculate the payload size in bytes */
		nbytes_aligned = STIX_ALIGN (STIX_OBJ_BYTESOF(oop), STIX_SIZEOF(stix_oop_t));
#if defined(STIX_USE_OBJECT_TRAILER)
	}
#endif

	return nbytes_aligned;
}

stix_oop_t stix_moveoop (stix_t* stix, stix_oop_t oop)
{
#if defined(STIX_SUPPORT_GC_DURING_IGNITION)
	if (!oop) return oop;
#endif

	if (!STIX_OOP_IS_POINTER(oop)) return oop;

	if (STIX_OBJ_GET_FLAGS_MOVED(oop))
	{
		/* this object has migrated to the new heap. 
		 * the class field has been updated to the new object
		 * in the 'else' block below. i can simply return it
		 * without further migration. */
		return STIX_OBJ_GET_CLASS(oop);
	}
	else
	{
		stix_oow_t nbytes_aligned;
		stix_oop_t tmp;

		nbytes_aligned = get_payload_bytes (stix, oop);

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
		STIX_OBJ_SET_CLASS (oop, tmp);

		/* return the new object */
		return tmp;
	}
}

static stix_uint8_t* scan_new_heap (stix_t* stix, stix_uint8_t* ptr)
{
	/*while (ptr < stix->newheap->ptr)*/
	while (STIX_LTPTR(stix_uint8_t, ptr, stix->newheap->ptr))
	{
		stix_oow_t i;
		stix_oow_t nbytes_aligned;
		stix_oop_t oop;

		oop = (stix_oop_t)ptr;

#if defined(STIX_USE_OBJECT_TRAILER)
		if (STIX_OBJ_GET_FLAGS_TRAILER(oop))
		{
			stix_oow_t nbytes;

			STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP);
			STIX_ASSERT (STIX_OBJ_GET_FLAGS_UNIT(oop) == STIX_SIZEOF(stix_oow_t));
			STIX_ASSERT (STIX_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

			nbytes = STIX_OBJ_BYTESOF(oop) + STIX_SIZEOF(stix_oow_t) + \
			         (stix_oow_t)((stix_oop_oop_t)oop)->slot[STIX_OBJ_GET_SIZE(oop)];
			nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));
		}
		else
		{
#endif
			nbytes_aligned = STIX_ALIGN (STIX_OBJ_BYTESOF(oop), STIX_SIZEOF(stix_oop_t));
#if defined(STIX_USE_OBJECT_TRAILER)
		}
#endif

		STIX_OBJ_SET_CLASS (oop, stix_moveoop(stix, STIX_OBJ_GET_CLASS(oop)));
		if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP)
		{
			stix_oop_oop_t xtmp;
			stix_oow_t size;

			if ((stix->_method_context && STIX_OBJ_GET_CLASS(oop) == stix->_method_context) ||
			    (stix->_block_context && STIX_OBJ_GET_CLASS(oop) == stix->_block_context))
			{
				/* the stack in the context object doesn't need to be 
				 * scanned in full. the slots above the stack pointer 
				 * are garbages. */
				size = STIX_CONTEXT_NAMED_INSTVARS +
				       STIX_OOP_TO_SMOOI(((stix_oop_context_t)oop)->sp) + 1;
			}
			else
			{
				size = STIX_OBJ_GET_SIZE(oop);
			}

			xtmp = (stix_oop_oop_t)oop;
			for (i = 0; i < size; i++)
			{
				if (STIX_OOP_IS_POINTER(xtmp->slot[i]))
					xtmp->slot[i] = stix_moveoop (stix, xtmp->slot[i]);
			}
		}

		/*ptr = ptr + STIX_SIZEOF(stix_obj_t) + nbytes_aligned;*/
		ptr = STIX_INCPTR (stix_uint8_t, ptr, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);
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
	stix_oop_t old_nil;
	stix_oow_t i;
	stix_cb_t* cb;

	if (stix->active_context)
	{
		/* store the stack pointer to the actual active context */
/* TODO: verify if this is correct */
		stix->active_context->sp = STIX_SMOOI_TO_OOP(stix->sp);
		stix->active_context->ip = STIX_SMOOI_TO_OOP(stix->ip);
	}

/*printf ("STARTING GC curheap base %p ptr %p newheap base %p ptr %p\n",
	stix->curheap->base, stix->curheap->ptr, stix->newheap->base, stix->newheap->ptr);*/
	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */
	old_nil = stix->_nil;

	/* move _nil and the root object table */
	stix->_nil               = stix_moveoop (stix, stix->_nil);
	stix->_true              = stix_moveoop (stix, stix->_true);
	stix->_false             = stix_moveoop (stix, stix->_false);

	stix->_apex              = stix_moveoop (stix, stix->_apex);
	stix->_class             = stix_moveoop (stix, stix->_class);
	stix->_undefined_object  = stix_moveoop (stix, stix->_undefined_object);
	stix->_object            = stix_moveoop (stix, stix->_object);
	stix->_array             = stix_moveoop (stix, stix->_array);
	stix->_byte_array        = stix_moveoop (stix, stix->_byte_array);
	stix->_string            = stix_moveoop (stix, stix->_string);
	stix->_symbol            = stix_moveoop (stix, stix->_symbol);
	stix->_symbol_set        = stix_moveoop (stix, stix->_symbol_set);
	stix->_system_dictionary = stix_moveoop (stix, stix->_system_dictionary);
	stix->_namespace         = stix_moveoop (stix, stix->_namespace);
	stix->_pool_dictionary   = stix_moveoop (stix, stix->_pool_dictionary);
	stix->_method_dictionary = stix_moveoop (stix, stix->_method_dictionary);
	stix->_method            = stix_moveoop (stix, stix->_method);
	stix->_association       = stix_moveoop (stix, stix->_association);
	stix->_method_context    = stix_moveoop (stix, stix->_method_context);
	stix->_block_context     = stix_moveoop (stix, stix->_block_context);
	stix->_process           = stix_moveoop (stix, stix->_process);
	stix->_process_scheduler = stix_moveoop (stix, stix->_process_scheduler);
	stix->_true_class        = stix_moveoop (stix, stix->_true_class);
	stix->_false_class       = stix_moveoop (stix, stix->_false_class);
	stix->_character         = stix_moveoop (stix, stix->_character);

	stix->_small_integer          = stix_moveoop (stix, stix->_small_integer);
	stix->_large_positive_integer = stix_moveoop (stix, stix->_large_positive_integer);
	stix->_large_negative_integer = stix_moveoop (stix, stix->_large_negative_integer);

	stix->sysdic = (stix_oop_set_t) stix_moveoop (stix, (stix_oop_t)stix->sysdic);
	stix->processor = (stix_oop_process_scheduler_t) stix_moveoop (stix, (stix_oop_t)stix->processor);

	for (i = 0; i < stix->tmp_count; i++)
	{
		*stix->tmp_stack[i] = stix_moveoop (stix, *stix->tmp_stack[i]);
	}

	stix->active_context = (stix_oop_context_t)stix_moveoop (stix, (stix_oop_t)stix->active_context);
	stix->active_method = (stix_oop_method_t)stix_moveoop (stix, (stix_oop_t)stix->active_method);

	for (cb = stix->cblist; cb; cb = cb->next)
	{
		if (cb->gc) cb->gc (stix);
	}

	/* scan the new heap to move referenced objects */
	ptr = (stix_uint8_t*) STIX_ALIGN ((stix_uintptr_t)stix->newheap->base, STIX_SIZEOF(stix_oop_t));
	ptr = scan_new_heap (stix, ptr);

	/* traverse the symbol table for unreferenced symbols.
	 * if the symbol has not moved to the new heap, the symbol
	 * is not referenced by any other objects than the symbol 
	 * table itself */
	compact_symbol_table (stix, old_nil);

	/* move the symbol table itself */
	stix->symtab = (stix_oop_set_t)stix_moveoop (stix, (stix_oop_t)stix->symtab);

	/* scan the new heap again from the end position of
	 * the previous scan to move referenced objects by 
	 * the symbol table. */
	ptr = scan_new_heap (stix, ptr);

	/* the contents of the current heap is not needed any more.
	 * reset the upper bound to the base. don't forget to align the heap
	 * pointer to the OOP size. See stix_makeheap() also */
	stix->curheap->ptr = (stix_uint8_t*)STIX_ALIGN(((stix_uintptr_t)stix->curheap->base), STIX_SIZEOF(stix_oop_t));

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
	if (stix->active_method) SET_ACTIVE_METHOD_CODE (stix); /* update stix->active_code */
}


void stix_pushtmp (stix_t* stix, stix_oop_t* oop_ptr)
{
	/* if you have too many temporaries pushed, something must be wrong.
	 * change your code not to exceede the stack limit */
	STIX_ASSERT (stix->tmp_count < STIX_COUNTOF(stix->tmp_stack));
	stix->tmp_stack[stix->tmp_count++] = oop_ptr;
}

void stix_poptmp (stix_t* stix)
{
	STIX_ASSERT (stix->tmp_count > 0);
	stix->tmp_count--;
}

void stix_poptmps (stix_t* stix, stix_oow_t count)
{
	STIX_ASSERT (stix->tmp_count >= count);
	stix->tmp_count -= count;
}


stix_oop_t stix_shallowcopy (stix_t* stix, stix_oop_t oop)
{
	if (STIX_OOP_IS_POINTER(oop) && STIX_OBJ_GET_CLASS(oop) != stix->_symbol)
	{
#if 0
		stix_oop_t z;
		stix_oop_class_t c;

		c = (stix_oop_class_t)STIX_OBJ_GET_CLASS(oop);
		stix_pushtmp (stix, &oop);
		z = stix_instantiate (stix, (stix_oop_t)c, STIX_NULL, STIX_OBJ_GET_SIZE(oop) - STIX_CLASS_SPEC_NAMED_INSTVAR(STIX_OOP_TO_SMOOI(c->spec)));
		stix_poptmp(stix);

		if (!z) return z;

		/* copy the payload */
		STIX_MEMCPY (z + 1, oop + 1, get_payload_bytes(stix, oop));

		return z;
#else
		stix_oop_t z;
		stix_oow_t total_bytes;

		total_bytes = STIX_SIZEOF(stix_obj_t) + get_payload_bytes(stix, oop);

		stix_pushtmp (stix, &oop);
		z = stix_allocbytes (stix, total_bytes);
		stix_poptmp(stix);

		STIX_MEMCPY (z, oop, total_bytes);
		return z;
#endif 
	}

	return oop;
}
