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

#include "stix-prv.h"


/*
 *    Stix .....................
 *    ^ ^ ^                    :   .......
 *    | | |                    v   v     :
 *    | | +------------------- Class .....
 *    | |                       ^  ^
 *    | +-------- NilObject ......:  :
 *    |                 ^........ nil  :
 *   Object ...........................:
 *    ^  
 *    |  
 *
 * The class hierarchy is roughly as follows:
 * 
 *   Stix
 *      Class
 *      NilObject
 *      Object
 *         Collection
 *            IndexedCollection
 *               FixedSizedCollection
 *                  Array
 *                  ByteArray
 *                     String
 *                        Symbol
 *            Set
 *               Dictionary
 *                  SystemDictionary
 *               SymbolSet
 *         Magnitude
 *            Association
 *            Character
 *            Number
 *               Integer
 *                  SmallInteger
 *                  LargeInteger
 *                     LargePositiveInteger
 *                     LargeNegativeInteger
 * 
 * Stix has no instance variables.
 * Stix has 1 class variable: Sysdic
 *  
 */

struct kernel_class_info_t
{
	stix_oow_t  len;
	stix_ooch_t name[20];
	stix_oow_t  offset;
};
typedef struct kernel_class_info_t kernel_class_info_t;

static kernel_class_info_t kernel_classes[] = 
{
	{  4, { 'A','p','e','x'                                                                 }, STIX_OFFSETOF(stix_t,_apex) },
	{ 15, { 'U','n','d','e','f','i','n','e','d','O','b','j','e','c','t'                     }, STIX_OFFSETOF(stix_t,_undefined_object) },
	{  5, { 'C','l','a','s','s'                                                             }, STIX_OFFSETOF(stix_t,_class) },
	{  6, { 'O','b','j','e','c','t'                                                         }, STIX_OFFSETOF(stix_t,_object) },
	{  6, { 'S','t','r','i','n','g'                                                         }, STIX_OFFSETOF(stix_t,_string) },

	{  6, { 'S','y','m','b','o','l'                                                         }, STIX_OFFSETOF(stix_t,_symbol) },
	{  5, { 'A','r','r','a','y'                                                             }, STIX_OFFSETOF(stix_t,_array) },
	{  9, { 'B','y','t','e','A','r','r','a','y'                                             }, STIX_OFFSETOF(stix_t,_byte_array) },
	{  9, { 'S','y','m','b','o','l','S','e','t'                                             }, STIX_OFFSETOF(stix_t,_symbol_set) },
	{ 16, { 'S','y','s','t','e','m','D','i','c','t','i','o','n','a','r','y'                 }, STIX_OFFSETOF(stix_t, _system_dictionary) },

	{  9, { 'N','a','m','e','s','p','a','c','e'                                             }, STIX_OFFSETOF(stix_t, _namespace) },
	{ 14, { 'P','o','o','l','D','i','c','t','i','o','n','a','r','y'                         }, STIX_OFFSETOF(stix_t, _pool_dictionary) },
	{ 16, { 'M','e','t','h','o','d','D','i','c','t','i','o','n','a','r','y'                 }, STIX_OFFSETOF(stix_t, _method_dictionary) },
	{ 14, { 'C','o','m','p','i','l','e','d','M','e','t','h','o','d'                         }, STIX_OFFSETOF(stix_t, _method) },
	{ 11, { 'A','s','s','o','c','i','a','t','i','o','n'                                     }, STIX_OFFSETOF(stix_t, _association) },

	{ 13, { 'M','e','t','h','o','d','C','o','n','t','e','x','t'                             }, STIX_OFFSETOF(stix_t, _method_context) },
	{ 12, { 'B','l','o','c','k','C','o','n','t','e','x','t'                                 }, STIX_OFFSETOF(stix_t, _block_context) },
	{  7, { 'P','r','o','c','e','s','s'                                                     }, STIX_OFFSETOF(stix_t, _process) },
	{  9, { 'S','e','m','a','p','h','o','r','e'                                             }, STIX_OFFSETOF(stix_t, _semaphore) },
	{ 16, { 'P','r','o','c','e','s','s','S','c','h','e','d','u','l','e','r'                 }, STIX_OFFSETOF(stix_t, _process_scheduler) },

	{  5, { 'E','r','r','o','r'                                                             }, STIX_OFFSETOF(stix_t, _error_class) },
	{  4, { 'T','r','u','e'                                                                 }, STIX_OFFSETOF(stix_t, _true_class) },
	{  5, { 'F','a','l','s','e'                                                             }, STIX_OFFSETOF(stix_t, _false_class) },
	{  9, { 'C','h','a','r','a','c','t','e','r'                                             }, STIX_OFFSETOF(stix_t, _character) },
	{ 12, { 'S','m','a','l','l','I','n','t','e','g','e','r'                                 }, STIX_OFFSETOF(stix_t, _small_integer) },

	{ 20, { 'L','a','r','g','e','P','o','s','i','t','i','v','e','I','n','t','e','g','e','r' }, STIX_OFFSETOF(stix_t, _large_positive_integer) },
	{ 20, { 'L','a','r','g','e','N','e','g','a','t','i','v','e','I','n','t','e','g','e','r' }, STIX_OFFSETOF(stix_t, _large_negative_integer) }
};

/* ----------------------------------------------------------------------- 
 * BOOTSTRAPPER
 * ----------------------------------------------------------------------- */

static stix_oop_t alloc_kernel_class (stix_t* stix, stix_oow_t indexed_classvars, stix_oow_t spec)
{
	stix_oop_class_t c;

	c = (stix_oop_class_t)stix_allocoopobj (stix, STIX_CLASS_NAMED_INSTVARS + indexed_classvars);
	if (!c) return STIX_NULL;

	STIX_OBJ_SET_FLAGS_KERNEL (c, 1);
	STIX_OBJ_SET_CLASS (c, stix->_class);
	c->spec = STIX_SMOOI_TO_OOP(spec); 
	c->selfspec = STIX_SMOOI_TO_OOP(STIX_CLASS_SELFSPEC_MAKE(indexed_classvars, 0));

	return (stix_oop_t)c;
}

static int ignite_1 (stix_t* stix)
{
	/*
	 * Create fundamental class objects with some fields mis-initialized yet.
	 * Such fields include 'superclass', 'subclasses', 'name', etc.
	 */
	STIX_ASSERT (stix, stix->_nil != STIX_NULL);
	STIX_ASSERT (stix, STIX_OBJ_GET_CLASS(stix->_nil) == STIX_NULL);

	STIX_ASSERT (stix, stix->_class == STIX_NULL);
	/* --------------------------------------------------------------
	 * Class
	 * The instance of Class can have indexed instance variables 
	 * which are actually class variables.
	 * -------------------------------------------------------------- */
	stix->_class = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_CLASS_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	if (!stix->_class) return -1;

	STIX_ASSERT (stix, STIX_OBJ_GET_CLASS(stix->_class) == STIX_NULL);
	STIX_OBJ_SET_CLASS (stix->_class, stix->_class);

	/* --------------------------------------------------------------
	 * Apex            - proto-object with 1 class variable.
	 * UndefinedObject - class for the nil object.
	 * Object          - top of all ordinary objects.
	 * String
	 * Symbol
	 * Array
	 * ByteArray
	 * SymbolSet
	 * Character
	 * SmallIntger
	 * -------------------------------------------------------------- */
	stix->_apex              = alloc_kernel_class (stix, 1, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_undefined_object  = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_object            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_string            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));

	stix->_symbol            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
	stix->_array             = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_OOP));
	stix->_byte_array        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_BYTE));
	stix->_symbol_set        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_system_dictionary = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));

	stix->_namespace         = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_pool_dictionary   = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_method_dictionary = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_method            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_METHOD_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	stix->_association       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_ASSOCIATION_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));

	stix->_method_context    = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_CONTEXT_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	stix->_block_context     = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_CONTEXT_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	stix->_process           = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_PROCESS_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	stix->_semaphore         = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SEMAPHORE_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_process_scheduler = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_PROCESS_SCHEDULER_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));

	stix->_error_class       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_true_class        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_false_class       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	/* TOOD: what is a proper spec for Character and SmallInteger?
 	 *       If the fixed part is  0, its instance must be an object of 0 payload fields.
	 *       Does this make sense? */
	stix->_character         = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_small_integer     = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_large_positive_integer = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_LIWORD));
	stix->_large_negative_integer = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_LIWORD));

	if (!stix->_apex              || !stix->_undefined_object  || 
	    !stix->_object            || !stix->_string            ||

	    !stix->_symbol            || !stix->_array             ||
	    !stix->_byte_array        || !stix->_symbol_set        || !stix->_system_dictionary || 

	    !stix->_namespace         || !stix->_pool_dictionary   ||
	    !stix->_method_dictionary || !stix->_method            || !stix->_association ||

	    !stix->_method_context    || !stix->_block_context     || 
	    !stix->_process           || !stix->_semaphore         || !stix->_process_scheduler ||

	    !stix->_true_class        || !stix->_false_class       || 
	    !stix->_character         || !stix->_small_integer     || 
	    !stix->_large_positive_integer  || !stix->_large_negative_integer) return -1;

	STIX_OBJ_SET_CLASS (stix->_nil, stix->_undefined_object);
	return 0;
}

static int ignite_2 (stix_t* stix)
{
	stix_oop_t tmp;

	/* Create 'true' and 'false objects */
	stix->_true = stix_instantiate (stix, stix->_true_class, STIX_NULL, 0);
	stix->_false = stix_instantiate (stix, stix->_false_class, STIX_NULL, 0);
	if (!stix->_true || !stix->_false) return -1;

	/* Create the symbol table */
	tmp = stix_instantiate (stix, stix->_symbol_set, STIX_NULL, 0);
	if (!tmp) return -1;
	stix->symtab = (stix_oop_set_t)tmp;

	stix->symtab->tally = STIX_SMOOI_TO_OOP(0);
	/* It's important to assign the result of stix_instantiate() to a temporary
	 * variable first and then assign it to stix->symtab->bucket. 
	 * The pointer 'stix->symtab; can change in stix_instantiate() and the 
	 * target address of assignment may get set before stix_instantiate()
	 * is called. */
	tmp = stix_instantiate (stix, stix->_array, STIX_NULL, stix->option.dfl_symtab_size);
	if (!tmp) return -1;
	stix->symtab->bucket = (stix_oop_oop_t)tmp;

	/* Create the system dictionary */
	tmp = (stix_oop_t)stix_makedic (stix, stix->_system_dictionary, stix->option.dfl_sysdic_size);
	if (!tmp) return -1;
	stix->sysdic = (stix_oop_set_t)tmp;

	/* Create a nil process used to simplify nil check in GC.
	 * only accessible by VM. not exported via the global dictionary. */
	tmp = (stix_oop_t)stix_instantiate (stix, stix->_process, STIX_NULL, 0);
	if (!tmp) return -1;
	stix->nil_process = (stix_oop_process_t)tmp;
	stix->nil_process->sp = STIX_SMOOI_TO_OOP(-1);

	/* Create a process scheduler */
	tmp = (stix_oop_t)stix_instantiate (stix, stix->_process_scheduler, STIX_NULL, 0);
	if (!tmp) return -1;
	stix->processor = (stix_oop_process_scheduler_t)tmp;
	stix->processor->tally = STIX_SMOOI_TO_OOP(0);
	stix->processor->active = stix->nil_process;

	/* Export the system dictionary via the first class variable of the Stix class */
	((stix_oop_class_t)stix->_apex)->slot[0] = (stix_oop_t)stix->sysdic;

	return 0;
}

static int ignite_3 (stix_t* stix)
{
	/* Register kernel classes manually created so far to the system dictionary */

	static stix_ooch_t str_system[] = { 'S','y','s','t','e', 'm' };
	static stix_ooch_t str_processor[] = { 'P', 'r', 'o', 'c', 'e', 's', 's', 'o', 'r' };

	stix_oow_t i;
	stix_oop_t sym, cls;
	stix_oop_t* stix_ptr;

	for (i = 0; i < STIX_COUNTOF(kernel_classes); i++)
	{
		sym = stix_makesymbol (stix, kernel_classes[i].name, kernel_classes[i].len);
		if (!sym) return -1;

		cls = *(stix_oop_t*)((stix_uint8_t*)stix + kernel_classes[i].offset);

		if (!stix_putatsysdic(stix, sym, cls)) return -1;
		stix_ptr++;
	}

	/* Make the system dictionary available as the global name 'Stix' */
	sym = stix_makesymbol (stix, str_system, 6);
	if (!sym) return -1;
	if (!stix_putatsysdic(stix, sym, (stix_oop_t)stix->sysdic)) return -1;

	/* Make the process scheduler avaialble as the global name 'Processor' */
	sym = stix_makesymbol (stix, str_processor, 9);
	if (!sym) return -1;
	if (!stix_putatsysdic(stix, sym, (stix_oop_t)stix->processor)) return -1;

	return 0;
}

int stix_ignite (stix_t* stix)
{
	STIX_ASSERT (stix, stix->_nil == STIX_NULL);

	stix->_nil = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t));
	if (!stix->_nil) return -1;

	stix->_nil->_flags = STIX_OBJ_MAKE_FLAGS (STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 1, 0, 0, 0);
	stix->_nil->_size = 0;

	if (ignite_1(stix) <= -1 || ignite_2(stix) <= -1 || ignite_3(stix)) return -1;

	return 0;
}

/* ----------------------------------------------------------------------- 
 * GARBAGE COLLECTOR
 * ----------------------------------------------------------------------- */


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
	STIX_ASSERT (stix, STIX_OOP_IS_SMOOI(stix->symtab->tally));
	tally = STIX_OOP_TO_SMOOI(stix->symtab->tally);
	STIX_ASSERT (stix, tally >= 0); /* it must not be less than 0 */
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

		STIX_ASSERT (stix, stix->symtab->bucket->slot[index] != _nil);

		for (i = 0, x = index, y = index; i < bucket_size; i++)
		{
			y = (y + 1) % bucket_size;

			/* done if the slot at the current hash index is _nil */
			if (stix->symtab->bucket->slot[y] == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			symbol = (stix_oop_char_t)stix->symtab->bucket->slot[y];

			STIX_ASSERT (stix, STIX_CLASSOF(stix,symbol) == stix->_symbol);

			z = stix_hashoochars(symbol->slot, STIX_OBJ_GET_SIZE(symbol)) % bucket_size;

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

	STIX_ASSERT (stix, tally >= 0);
	STIX_ASSERT (stix, tally <= STIX_SMOOI_MAX);
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
		STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP);
		STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_UNIT(oop) == STIX_SIZEOF(stix_oow_t));
		STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

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
		STIX_ASSERT (stix, tmp != STIX_NULL); 

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
	while (ptr < stix->newheap->ptr)
	{
		stix_oow_t i;
		stix_oow_t nbytes_aligned;
		stix_oop_t oop;

		oop = (stix_oop_t)ptr;

	#if defined(STIX_USE_OBJECT_TRAILER)
		if (STIX_OBJ_GET_FLAGS_TRAILER(oop))
		{
			stix_oow_t nbytes;

			STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP);
			STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_UNIT(oop) == STIX_SIZEOF(stix_oow_t));
			STIX_ASSERT (stix, STIX_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

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

			if (stix->_process && STIX_OBJ_GET_CLASS(oop) == stix->_process)
			{
				/* the stack in a process object doesn't need to be 
				 * scanned in full. the slots above the stack pointer 
				 * are garbages. */
				size = STIX_PROCESS_NAMED_INSTVARS +
				       STIX_OOP_TO_SMOOI(((stix_oop_process_t)oop)->sp) + 1;
				STIX_ASSERT (stix, size <= STIX_OBJ_GET_SIZE(oop));
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
	stix_oop_t old_nil;
	stix_oow_t i;
	stix_cb_t* cb;

	if (stix->active_context)
	{
/* TODO: verify if this is correct */
	
		STIX_ASSERT (stix, (stix_oop_t)stix->processor != stix->_nil);
		STIX_ASSERT (stix, (stix_oop_t)stix->processor->active != stix->_nil);
		/* store the stack pointer to the active process */
		stix->processor->active->sp = STIX_SMOOI_TO_OOP(stix->sp);

		/* store the instruction pointer to the active context */
		stix->active_context->ip = STIX_SMOOI_TO_OOP(stix->ip);
	}

	STIX_LOG4 (stix, STIX_LOG_GC | STIX_LOG_INFO, 
		"Starting GC curheap base %p ptr %p newheap base %p ptr %p\n",
		stix->curheap->base, stix->curheap->ptr, stix->newheap->base, stix->newheap->ptr); 

	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */
	old_nil = stix->_nil;

	/* move _nil and the root object table */
	stix->_nil               = stix_moveoop (stix, stix->_nil);
	stix->_true              = stix_moveoop (stix, stix->_true);
	stix->_false             = stix_moveoop (stix, stix->_false);

	for (i = 0; i < STIX_COUNTOF(kernel_classes); i++)
	{
		stix_oop_t tmp;
		tmp = *(stix_oop_t*)((stix_uint8_t*)stix + kernel_classes[i].offset);
		tmp = stix_moveoop (stix, tmp);
		*(stix_oop_t*)((stix_uint8_t*)stix + kernel_classes[i].offset) = tmp;
	}

	stix->sysdic = (stix_oop_set_t) stix_moveoop (stix, (stix_oop_t)stix->sysdic);
	stix->processor = (stix_oop_process_scheduler_t) stix_moveoop (stix, (stix_oop_t)stix->processor);
	stix->nil_process = (stix_oop_process_t) stix_moveoop (stix, (stix_oop_t)stix->nil_process);

	for (i = 0; i < stix->sem_list_count; i++)
	{
		stix->sem_list[i] = (stix_oop_semaphore_t)stix_moveoop (stix, (stix_oop_t)stix->sem_list[i]);
	}

	for (i = 0; i < stix->sem_heap_count; i++)
	{
		stix->sem_heap[i] = (stix_oop_semaphore_t)stix_moveoop (stix, (stix_oop_t)stix->sem_heap[i]);
	}

	for (i = 0; i < stix->tmp_count; i++)
	{
		*stix->tmp_stack[i] = stix_moveoop (stix, *stix->tmp_stack[i]);
	}

	if (stix->initial_context)
		stix->initial_context = (stix_oop_context_t)stix_moveoop (stix, (stix_oop_t)stix->initial_context);
	if (stix->active_context)
		stix->active_context = (stix_oop_context_t)stix_moveoop (stix, (stix_oop_t)stix->active_context);
	if (stix->active_method)
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
	if (stix->symtab && STIX_LOG_ENABLED(stix, STIX_LOG_GC | STIX_LOG_DEBUG))
	{
		stix_oow_t index;
		stix_oop_oop_t buc;
		STIX_LOG0 (stix, STIX_LOG_GC | STIX_LOG_DEBUG, "--------- SURVIVING SYMBOLS IN GC ----------\n");
		buc = (stix_oop_oop_t) stix->symtab->bucket;
		for (index = 0; index < STIX_OBJ_GET_SIZE(buc); index++)
		{
			if ((stix_oop_t)buc->slot[index] != stix->_nil) 
			{
				STIX_LOG1 (stix, STIX_LOG_GC | STIX_LOG_DEBUG, "\t%O\n", buc->slot[index]);
			}
		}
		STIX_LOG0 (stix, STIX_LOG_GC | STIX_LOG_DEBUG, "--------------------------------------------\n");
	}
*/

	if (stix->active_method) SET_ACTIVE_METHOD_CODE (stix); /* update stix->active_code */

	/* TODO: include some gc statstics like number of live objects, gc performance, etc */
	STIX_LOG4 (stix, STIX_LOG_GC | STIX_LOG_INFO, 
		"Finished GC curheap base %p ptr %p newheap base %p ptr %p\n",
		stix->curheap->base, stix->curheap->ptr, stix->newheap->base, stix->newheap->ptr); 
}


void stix_pushtmp (stix_t* stix, stix_oop_t* oop_ptr)
{
	/* if you have too many temporaries pushed, something must be wrong.
	 * change your code not to exceede the stack limit */
	STIX_ASSERT (stix, stix->tmp_count < STIX_COUNTOF(stix->tmp_stack));
	stix->tmp_stack[stix->tmp_count++] = oop_ptr;
}

void stix_poptmp (stix_t* stix)
{
	STIX_ASSERT (stix, stix->tmp_count > 0);
	stix->tmp_count--;
}

void stix_poptmps (stix_t* stix, stix_oow_t count)
{
	STIX_ASSERT (stix, stix->tmp_count >= count);
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
