/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

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

#include "moo-prv.h"


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
	moo_oow_t  len;
	moo_ooch_t name[20];
	moo_oow_t  offset;
};
typedef struct kernel_class_info_t kernel_class_info_t;

static kernel_class_info_t kernel_classes[] = 
{
	{  4, { 'A','p','e','x'                                                                 }, MOO_OFFSETOF(moo_t,_apex) },
	{ 15, { 'U','n','d','e','f','i','n','e','d','O','b','j','e','c','t'                     }, MOO_OFFSETOF(moo_t,_undefined_object) },
	{  5, { 'C','l','a','s','s'                                                             }, MOO_OFFSETOF(moo_t,_class) },
	{  6, { 'O','b','j','e','c','t'                                                         }, MOO_OFFSETOF(moo_t,_object) },
	{  6, { 'S','t','r','i','n','g'                                                         }, MOO_OFFSETOF(moo_t,_string) },

	{  6, { 'S','y','m','b','o','l'                                                         }, MOO_OFFSETOF(moo_t,_symbol) },
	{  5, { 'A','r','r','a','y'                                                             }, MOO_OFFSETOF(moo_t,_array) },
	{  9, { 'B','y','t','e','A','r','r','a','y'                                             }, MOO_OFFSETOF(moo_t,_byte_array) },
	{  9, { 'S','y','m','b','o','l','S','e','t'                                             }, MOO_OFFSETOF(moo_t,_symbol_set) },
	{ 16, { 'S','y','s','t','e','m','D','i','c','t','i','o','n','a','r','y'                 }, MOO_OFFSETOF(moo_t, _system_dictionary) },

	{  9, { 'N','a','m','e','s','p','a','c','e'                                             }, MOO_OFFSETOF(moo_t, _namespace) },
	{ 14, { 'P','o','o','l','D','i','c','t','i','o','n','a','r','y'                         }, MOO_OFFSETOF(moo_t, _pool_dictionary) },
	{ 16, { 'M','e','t','h','o','d','D','i','c','t','i','o','n','a','r','y'                 }, MOO_OFFSETOF(moo_t, _method_dictionary) },
	{ 14, { 'C','o','m','p','i','l','e','d','M','e','t','h','o','d'                         }, MOO_OFFSETOF(moo_t, _method) },
	{ 11, { 'A','s','s','o','c','i','a','t','i','o','n'                                     }, MOO_OFFSETOF(moo_t, _association) },

	{ 13, { 'M','e','t','h','o','d','C','o','n','t','e','x','t'                             }, MOO_OFFSETOF(moo_t, _method_context) },
	{ 12, { 'B','l','o','c','k','C','o','n','t','e','x','t'                                 }, MOO_OFFSETOF(moo_t, _block_context) },
	{  7, { 'P','r','o','c','e','s','s'                                                     }, MOO_OFFSETOF(moo_t, _process) },
	{  9, { 'S','e','m','a','p','h','o','r','e'                                             }, MOO_OFFSETOF(moo_t, _semaphore) },
	{ 16, { 'P','r','o','c','e','s','s','S','c','h','e','d','u','l','e','r'                 }, MOO_OFFSETOF(moo_t, _process_scheduler) },

	{  5, { 'E','r','r','o','r'                                                             }, MOO_OFFSETOF(moo_t, _error_class) },
	{  4, { 'T','r','u','e'                                                                 }, MOO_OFFSETOF(moo_t, _true_class) },
	{  5, { 'F','a','l','s','e'                                                             }, MOO_OFFSETOF(moo_t, _false_class) },
	{  9, { 'C','h','a','r','a','c','t','e','r'                                             }, MOO_OFFSETOF(moo_t, _character) },
	{ 12, { 'S','m','a','l','l','I','n','t','e','g','e','r'                                 }, MOO_OFFSETOF(moo_t, _small_integer) },

	{ 20, { 'L','a','r','g','e','P','o','s','i','t','i','v','e','I','n','t','e','g','e','r' }, MOO_OFFSETOF(moo_t, _large_positive_integer) },
	{ 20, { 'L','a','r','g','e','N','e','g','a','t','i','v','e','I','n','t','e','g','e','r' }, MOO_OFFSETOF(moo_t, _large_negative_integer) }
};

/* ----------------------------------------------------------------------- 
 * BOOTSTRAPPER
 * ----------------------------------------------------------------------- */

static moo_oop_t alloc_kernel_class (moo_t* moo, moo_oow_t indexed_classvars, moo_oow_t spec)
{
	moo_oop_class_t c;

	c = (moo_oop_class_t)moo_allocoopobj (moo, MOO_CLASS_NAMED_INSTVARS + indexed_classvars);
	if (!c) return MOO_NULL;

	MOO_OBJ_SET_FLAGS_KERNEL (c, 1);
	MOO_OBJ_SET_CLASS (c, moo->_class);
	c->spec = MOO_SMOOI_TO_OOP(spec); 
	c->selfspec = MOO_SMOOI_TO_OOP(MOO_CLASS_SELFSPEC_MAKE(indexed_classvars, 0));

	return (moo_oop_t)c;
}

static int ignite_1 (moo_t* moo)
{
	/*
	 * Create fundamental class objects with some fields mis-initialized yet.
	 * Such fields include 'superclass', 'subclasses', 'name', etc.
	 */
	MOO_ASSERT (moo, moo->_nil != MOO_NULL);
	MOO_ASSERT (moo, MOO_OBJ_GET_CLASS(moo->_nil) == MOO_NULL);

	MOO_ASSERT (moo, moo->_class == MOO_NULL);
	/* --------------------------------------------------------------
	 * Class
	 * The instance of Class can have indexed instance variables 
	 * which are actually class variables.
	 * -------------------------------------------------------------- */
	moo->_class = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_CLASS_NAMED_INSTVARS, 1, MOO_OBJ_TYPE_OOP));
	if (!moo->_class) return -1;

	MOO_ASSERT (moo, MOO_OBJ_GET_CLASS(moo->_class) == MOO_NULL);
	MOO_OBJ_SET_CLASS (moo->_class, moo->_class);

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
	moo->_apex              = alloc_kernel_class (moo, 1, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_undefined_object  = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_object            = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_string            = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_CHAR));

	moo->_symbol            = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_CHAR));
	moo->_array             = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_OOP));
	moo->_byte_array        = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_BYTE));
	moo->_symbol_set        = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SET_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));
	moo->_system_dictionary = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SET_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));

	moo->_namespace         = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SET_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));
	moo->_pool_dictionary   = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SET_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));
	moo->_method_dictionary = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SET_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));
	moo->_method            = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_METHOD_NAMED_INSTVARS, 1, MOO_OBJ_TYPE_OOP));
	moo->_association       = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_ASSOCIATION_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));

	moo->_method_context    = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_CONTEXT_NAMED_INSTVARS, 1, MOO_OBJ_TYPE_OOP));
	moo->_block_context     = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_CONTEXT_NAMED_INSTVARS, 1, MOO_OBJ_TYPE_OOP));
	moo->_process           = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_PROCESS_NAMED_INSTVARS, 1, MOO_OBJ_TYPE_OOP));
	moo->_semaphore         = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_SEMAPHORE_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));
	moo->_process_scheduler = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(MOO_PROCESS_SCHEDULER_NAMED_INSTVARS, 0, MOO_OBJ_TYPE_OOP));

	moo->_error_class       = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_true_class        = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_false_class       = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	/* TOOD: what is a proper spec for Character and SmallInteger?
 	 *       If the fixed part is  0, its instance must be an object of 0 payload fields.
	 *       Does this make sense? */
	moo->_character         = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_small_integer     = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 0, MOO_OBJ_TYPE_OOP));
	moo->_large_positive_integer = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_LIWORD));
	moo->_large_negative_integer = alloc_kernel_class (moo, 0, MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_LIWORD));

	if (!moo->_apex              || !moo->_undefined_object  || 
	    !moo->_object            || !moo->_string            ||

	    !moo->_symbol            || !moo->_array             ||
	    !moo->_byte_array        || !moo->_symbol_set        || !moo->_system_dictionary || 

	    !moo->_namespace         || !moo->_pool_dictionary   ||
	    !moo->_method_dictionary || !moo->_method            || !moo->_association ||

	    !moo->_method_context    || !moo->_block_context     || 
	    !moo->_process           || !moo->_semaphore         || !moo->_process_scheduler ||

	    !moo->_true_class        || !moo->_false_class       || 
	    !moo->_character         || !moo->_small_integer     || 
	    !moo->_large_positive_integer  || !moo->_large_negative_integer) return -1;

	MOO_OBJ_SET_CLASS (moo->_nil, moo->_undefined_object);
	return 0;
}

static int ignite_2 (moo_t* moo)
{
	moo_oop_t tmp;

	/* Create 'true' and 'false objects */
	moo->_true = moo_instantiate (moo, moo->_true_class, MOO_NULL, 0);
	moo->_false = moo_instantiate (moo, moo->_false_class, MOO_NULL, 0);
	if (!moo->_true || !moo->_false) return -1;

	/* Create the symbol table */
	tmp = moo_instantiate (moo, moo->_symbol_set, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->symtab = (moo_oop_set_t)tmp;

	moo->symtab->tally = MOO_SMOOI_TO_OOP(0);
	/* It's important to assign the result of moo_instantiate() to a temporary
	 * variable first and then assign it to moo->symtab->bucket. 
	 * The pointer 'moo->symtab; can change in moo_instantiate() and the 
	 * target address of assignment may get set before moo_instantiate()
	 * is called. */
	tmp = moo_instantiate (moo, moo->_array, MOO_NULL, moo->option.dfl_symtab_size);
	if (!tmp) return -1;
	moo->symtab->bucket = (moo_oop_oop_t)tmp;

	/* Create the system dictionary */
	tmp = (moo_oop_t)moo_makedic (moo, moo->_system_dictionary, moo->option.dfl_sysdic_size);
	if (!tmp) return -1;
	moo->sysdic = (moo_oop_set_t)tmp;

	/* Create a nil process used to simplify nil check in GC.
	 * only accessible by VM. not exported via the global dictionary. */
	tmp = (moo_oop_t)moo_instantiate (moo, moo->_process, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->nil_process = (moo_oop_process_t)tmp;
	moo->nil_process->sp = MOO_SMOOI_TO_OOP(-1);

	/* Create a process scheduler */
	tmp = (moo_oop_t)moo_instantiate (moo, moo->_process_scheduler, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->processor = (moo_oop_process_scheduler_t)tmp;
	moo->processor->tally = MOO_SMOOI_TO_OOP(0);
	moo->processor->active = moo->nil_process;

	/* Export the system dictionary via the first class variable of the Stix class */
	((moo_oop_class_t)moo->_apex)->slot[0] = (moo_oop_t)moo->sysdic;

	return 0;
}

static int ignite_3 (moo_t* moo)
{
	/* Register kernel classes manually created so far to the system dictionary */

	static moo_ooch_t str_system[] = { 'S','y','s','t','e', 'm' };
	static moo_ooch_t str_processor[] = { 'P', 'r', 'o', 'c', 'e', 's', 's', 'o', 'r' };

	moo_oow_t i;
	moo_oop_t sym, cls;
	moo_oop_t* moo_ptr;

	for (i = 0; i < MOO_COUNTOF(kernel_classes); i++)
	{
		sym = moo_makesymbol (moo, kernel_classes[i].name, kernel_classes[i].len);
		if (!sym) return -1;

		cls = *(moo_oop_t*)((moo_uint8_t*)moo + kernel_classes[i].offset);

		if (!moo_putatsysdic(moo, sym, cls)) return -1;
		moo_ptr++;
	}

	/* Make the system dictionary available as the global name 'Stix' */
	sym = moo_makesymbol (moo, str_system, 6);
	if (!sym) return -1;
	if (!moo_putatsysdic(moo, sym, (moo_oop_t)moo->sysdic)) return -1;

	/* Make the process scheduler avaialble as the global name 'Processor' */
	sym = moo_makesymbol (moo, str_processor, 9);
	if (!sym) return -1;
	if (!moo_putatsysdic(moo, sym, (moo_oop_t)moo->processor)) return -1;

	return 0;
}

int moo_ignite (moo_t* moo)
{
	MOO_ASSERT (moo, moo->_nil == MOO_NULL);

	moo->_nil = moo_allocbytes (moo, MOO_SIZEOF(moo_obj_t));
	if (!moo->_nil) return -1;

	moo->_nil->_flags = MOO_OBJ_MAKE_FLAGS (MOO_OBJ_TYPE_OOP, MOO_SIZEOF(moo_oop_t), 0, 1, 0, 0, 0);
	moo->_nil->_size = 0;

	if (ignite_1(moo) <= -1 || ignite_2(moo) <= -1 || ignite_3(moo)) return -1;

	return 0;
}

/* ----------------------------------------------------------------------- 
 * GARBAGE COLLECTOR
 * ----------------------------------------------------------------------- */


static void compact_symbol_table (moo_t* moo, moo_oop_t _nil)
{
	moo_oop_char_t symbol;
	moo_oow_t i, x, y, z;
	moo_oow_t bucket_size, index;
	moo_ooi_t tally;

#if defined(MOO_SUPPORT_GC_DURING_IGNITION)
	if (!moo->symtab) return; /* symbol table has not been created */
#endif

	/* the symbol table doesn't allow more data items than MOO_SMOOI_MAX.
	 * so moo->symtab->tally must always be a small integer */
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(moo->symtab->tally));
	tally = MOO_OOP_TO_SMOOI(moo->symtab->tally);
	MOO_ASSERT (moo, tally >= 0); /* it must not be less than 0 */
	if (tally <= 0) return;

	/* NOTE: in theory, the bucket size can be greater than MOO_SMOOI_MAX
	 * as it is an internal header field and is of an unsigned type */
	bucket_size = MOO_OBJ_GET_SIZE(moo->symtab->bucket);

	for (index = 0; index < bucket_size; )
	{
		if (MOO_OBJ_GET_FLAGS_MOVED(moo->symtab->bucket->slot[index]))
		{
			index++;
			continue;
		}

		MOO_ASSERT (moo, moo->symtab->bucket->slot[index] != _nil);

		for (i = 0, x = index, y = index; i < bucket_size; i++)
		{
			y = (y + 1) % bucket_size;

			/* done if the slot at the current hash index is _nil */
			if (moo->symtab->bucket->slot[y] == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			symbol = (moo_oop_char_t)moo->symtab->bucket->slot[y];

			MOO_ASSERT (moo, MOO_CLASSOF(moo,symbol) == moo->_symbol);

			z = moo_hashoochars(symbol->slot, MOO_OBJ_GET_SIZE(symbol)) % bucket_size;

			/* move an element if necessary */
			if ((y > x && (z <= x || z > y)) ||
			    (y < x && (z <= x && z > y)))
			{
				moo->symtab->bucket->slot[x] = moo->symtab->bucket->slot[y];
				x = y;
			}
		}

		moo->symtab->bucket->slot[x] = _nil;
		tally--;
	}

	MOO_ASSERT (moo, tally >= 0);
	MOO_ASSERT (moo, tally <= MOO_SMOOI_MAX);
	moo->symtab->tally = MOO_SMOOI_TO_OOP(tally);
}


static MOO_INLINE moo_oow_t get_payload_bytes (moo_t* moo, moo_oop_t oop)
{
	moo_oow_t nbytes_aligned;

#if defined(MOO_USE_OBJECT_TRAILER)
	if (MOO_OBJ_GET_FLAGS_TRAILER(oop))
	{
		moo_oow_t nbytes;

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
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP);
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_UNIT(oop) == MOO_SIZEOF(moo_oow_t));
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

		nbytes = MOO_OBJ_BYTESOF(oop) + MOO_SIZEOF(moo_oow_t) + \
		         (moo_oow_t)((moo_oop_oop_t)oop)->slot[MOO_OBJ_GET_SIZE(oop)];
		nbytes_aligned = MOO_ALIGN (nbytes, MOO_SIZEOF(moo_oop_t));
	}
	else
	{
#endif
		/* calculate the payload size in bytes */
		nbytes_aligned = MOO_ALIGN (MOO_OBJ_BYTESOF(oop), MOO_SIZEOF(moo_oop_t));
#if defined(MOO_USE_OBJECT_TRAILER)
	}
#endif

	return nbytes_aligned;
}

moo_oop_t moo_moveoop (moo_t* moo, moo_oop_t oop)
{
#if defined(MOO_SUPPORT_GC_DURING_IGNITION)
	if (!oop) return oop;
#endif

	if (!MOO_OOP_IS_POINTER(oop)) return oop;

	if (MOO_OBJ_GET_FLAGS_MOVED(oop))
	{
		/* this object has migrated to the new heap. 
		 * the class field has been updated to the new object
		 * in the 'else' block below. i can simply return it
		 * without further migration. */
		return MOO_OBJ_GET_CLASS(oop);
	}
	else
	{
		moo_oow_t nbytes_aligned;
		moo_oop_t tmp;

		nbytes_aligned = get_payload_bytes (moo, oop);

		/* allocate space in the new heap */
		tmp = moo_allocheapmem (moo, moo->newheap, MOO_SIZEOF(moo_obj_t) + nbytes_aligned);

		/* allocation here must not fail because
		 * i'm allocating the new space in a new heap for 
		 * moving an existing object in the current heap. 
		 *
		 * assuming the new heap is as large as the old heap,
		 * and garbage collection doesn't allocate more objects
		 * than in the old heap, it must not fail. */
		MOO_ASSERT (moo, tmp != MOO_NULL); 

		/* copy the payload to the new object */
		MOO_MEMCPY (tmp, oop, MOO_SIZEOF(moo_obj_t) + nbytes_aligned);

		/* mark the old object that it has migrated to the new heap */
		MOO_OBJ_SET_FLAGS_MOVED(oop, 1);

		/* let the class field of the old object point to the new 
		 * object allocated in the new heap. it is returned in 
		 * the 'if' block at the top of this function. */
		MOO_OBJ_SET_CLASS (oop, tmp);

		/* return the new object */
		return tmp;
	}
}

static moo_uint8_t* scan_new_heap (moo_t* moo, moo_uint8_t* ptr)
{
	while (ptr < moo->newheap->ptr)
	{
		moo_oow_t i;
		moo_oow_t nbytes_aligned;
		moo_oop_t oop;

		oop = (moo_oop_t)ptr;

	#if defined(MOO_USE_OBJECT_TRAILER)
		if (MOO_OBJ_GET_FLAGS_TRAILER(oop))
		{
			moo_oow_t nbytes;

			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP);
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_UNIT(oop) == MOO_SIZEOF(moo_oow_t));
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

			nbytes = MOO_OBJ_BYTESOF(oop) + MOO_SIZEOF(moo_oow_t) + \
			         (moo_oow_t)((moo_oop_oop_t)oop)->slot[MOO_OBJ_GET_SIZE(oop)];
			nbytes_aligned = MOO_ALIGN (nbytes, MOO_SIZEOF(moo_oop_t));
		}
		else
		{
	#endif
			nbytes_aligned = MOO_ALIGN (MOO_OBJ_BYTESOF(oop), MOO_SIZEOF(moo_oop_t));
	#if defined(MOO_USE_OBJECT_TRAILER)
		}
	#endif

		MOO_OBJ_SET_CLASS (oop, moo_moveoop(moo, MOO_OBJ_GET_CLASS(oop)));
		if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP)
		{
			moo_oop_oop_t xtmp;
			moo_oow_t size;

			if (moo->_process && MOO_OBJ_GET_CLASS(oop) == moo->_process)
			{
				/* the stack in a process object doesn't need to be 
				 * scanned in full. the slots above the stack pointer 
				 * are garbages. */
				size = MOO_PROCESS_NAMED_INSTVARS +
				       MOO_OOP_TO_SMOOI(((moo_oop_process_t)oop)->sp) + 1;
				MOO_ASSERT (moo, size <= MOO_OBJ_GET_SIZE(oop));
			}
			else
			{
				size = MOO_OBJ_GET_SIZE(oop);
			}

			xtmp = (moo_oop_oop_t)oop;
			for (i = 0; i < size; i++)
			{
				if (MOO_OOP_IS_POINTER(xtmp->slot[i]))
					xtmp->slot[i] = moo_moveoop (moo, xtmp->slot[i]);
			}
		}

		ptr = ptr + MOO_SIZEOF(moo_obj_t) + nbytes_aligned;
	}

	/* return the pointer to the beginning of the free space in the heap */
	return ptr; 
}

void moo_gc (moo_t* moo)
{
	/* 
	 * move a referenced object to the new heap.
	 * inspect the fields of the moved object in the new heap.
	 * move objects pointed to by the fields to the new heap.
	 * finally perform some tricky symbol table clean-up.
	 */
	moo_uint8_t* ptr;
	moo_heap_t* tmp;
	moo_oop_t old_nil;
	moo_oow_t i;
	moo_cb_t* cb;

	if (moo->active_context)
	{
/* TODO: verify if this is correct */
	
		MOO_ASSERT (moo, (moo_oop_t)moo->processor != moo->_nil);
		MOO_ASSERT (moo, (moo_oop_t)moo->processor->active != moo->_nil);
		/* store the stack pointer to the active process */
		moo->processor->active->sp = MOO_SMOOI_TO_OOP(moo->sp);

		/* store the instruction pointer to the active context */
		moo->active_context->ip = MOO_SMOOI_TO_OOP(moo->ip);
	}

	MOO_LOG4 (moo, MOO_LOG_GC | MOO_LOG_INFO, 
		"Starting GC curheap base %p ptr %p newheap base %p ptr %p\n",
		moo->curheap->base, moo->curheap->ptr, moo->newheap->base, moo->newheap->ptr); 

	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */
	old_nil = moo->_nil;

	/* move _nil and the root object table */
	moo->_nil               = moo_moveoop (moo, moo->_nil);
	moo->_true              = moo_moveoop (moo, moo->_true);
	moo->_false             = moo_moveoop (moo, moo->_false);

	for (i = 0; i < MOO_COUNTOF(kernel_classes); i++)
	{
		moo_oop_t tmp;
		tmp = *(moo_oop_t*)((moo_uint8_t*)moo + kernel_classes[i].offset);
		tmp = moo_moveoop (moo, tmp);
		*(moo_oop_t*)((moo_uint8_t*)moo + kernel_classes[i].offset) = tmp;
	}

	moo->sysdic = (moo_oop_set_t) moo_moveoop (moo, (moo_oop_t)moo->sysdic);
	moo->processor = (moo_oop_process_scheduler_t) moo_moveoop (moo, (moo_oop_t)moo->processor);
	moo->nil_process = (moo_oop_process_t) moo_moveoop (moo, (moo_oop_t)moo->nil_process);

	for (i = 0; i < moo->sem_list_count; i++)
	{
		moo->sem_list[i] = (moo_oop_semaphore_t)moo_moveoop (moo, (moo_oop_t)moo->sem_list[i]);
	}

	for (i = 0; i < moo->sem_heap_count; i++)
	{
		moo->sem_heap[i] = (moo_oop_semaphore_t)moo_moveoop (moo, (moo_oop_t)moo->sem_heap[i]);
	}

	for (i = 0; i < moo->tmp_count; i++)
	{
		*moo->tmp_stack[i] = moo_moveoop (moo, *moo->tmp_stack[i]);
	}

	if (moo->initial_context)
		moo->initial_context = (moo_oop_context_t)moo_moveoop (moo, (moo_oop_t)moo->initial_context);
	if (moo->active_context)
		moo->active_context = (moo_oop_context_t)moo_moveoop (moo, (moo_oop_t)moo->active_context);
	if (moo->active_method)
		moo->active_method = (moo_oop_method_t)moo_moveoop (moo, (moo_oop_t)moo->active_method);

	for (cb = moo->cblist; cb; cb = cb->next)
	{
		if (cb->gc) cb->gc (moo);
	}

	/* scan the new heap to move referenced objects */
	ptr = (moo_uint8_t*) MOO_ALIGN ((moo_uintptr_t)moo->newheap->base, MOO_SIZEOF(moo_oop_t));
	ptr = scan_new_heap (moo, ptr);

	/* traverse the symbol table for unreferenced symbols.
	 * if the symbol has not moved to the new heap, the symbol
	 * is not referenced by any other objects than the symbol 
	 * table itself */
	compact_symbol_table (moo, old_nil);

	/* move the symbol table itself */
	moo->symtab = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->symtab);

	/* scan the new heap again from the end position of
	 * the previous scan to move referenced objects by 
	 * the symbol table. */
	ptr = scan_new_heap (moo, ptr);

	/* the contents of the current heap is not needed any more.
	 * reset the upper bound to the base. don't forget to align the heap
	 * pointer to the OOP size. See moo_makeheap() also */
	moo->curheap->ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)moo->curheap->base), MOO_SIZEOF(moo_oop_t));

	/* swap the current heap and old heap */
	tmp = moo->curheap;
	moo->curheap = moo->newheap;
	moo->newheap = tmp;

/*
	if (moo->symtab && MOO_LOG_ENABLED(moo, MOO_LOG_GC | MOO_LOG_DEBUG))
	{
		moo_oow_t index;
		moo_oop_oop_t buc;
		MOO_LOG0 (moo, MOO_LOG_GC | MOO_LOG_DEBUG, "--------- SURVIVING SYMBOLS IN GC ----------\n");
		buc = (moo_oop_oop_t) moo->symtab->bucket;
		for (index = 0; index < MOO_OBJ_GET_SIZE(buc); index++)
		{
			if ((moo_oop_t)buc->slot[index] != moo->_nil) 
			{
				MOO_LOG1 (moo, MOO_LOG_GC | MOO_LOG_DEBUG, "\t%O\n", buc->slot[index]);
			}
		}
		MOO_LOG0 (moo, MOO_LOG_GC | MOO_LOG_DEBUG, "--------------------------------------------\n");
	}
*/

	if (moo->active_method) SET_ACTIVE_METHOD_CODE (moo); /* update moo->active_code */

	/* TODO: include some gc statstics like number of live objects, gc performance, etc */
	MOO_LOG4 (moo, MOO_LOG_GC | MOO_LOG_INFO, 
		"Finished GC curheap base %p ptr %p newheap base %p ptr %p\n",
		moo->curheap->base, moo->curheap->ptr, moo->newheap->base, moo->newheap->ptr); 
}


void moo_pushtmp (moo_t* moo, moo_oop_t* oop_ptr)
{
	/* if you have too many temporaries pushed, something must be wrong.
	 * change your code not to exceede the stack limit */
	MOO_ASSERT (moo, moo->tmp_count < MOO_COUNTOF(moo->tmp_stack));
	moo->tmp_stack[moo->tmp_count++] = oop_ptr;
}

void moo_poptmp (moo_t* moo)
{
	MOO_ASSERT (moo, moo->tmp_count > 0);
	moo->tmp_count--;
}

void moo_poptmps (moo_t* moo, moo_oow_t count)
{
	MOO_ASSERT (moo, moo->tmp_count >= count);
	moo->tmp_count -= count;
}


moo_oop_t moo_shallowcopy (moo_t* moo, moo_oop_t oop)
{
	if (MOO_OOP_IS_POINTER(oop) && MOO_OBJ_GET_CLASS(oop) != moo->_symbol)
	{
#if 0
		moo_oop_t z;
		moo_oop_class_t c;

		c = (moo_oop_class_t)MOO_OBJ_GET_CLASS(oop);
		moo_pushtmp (moo, &oop);
		z = moo_instantiate (moo, (moo_oop_t)c, MOO_NULL, MOO_OBJ_GET_SIZE(oop) - MOO_CLASS_SPEC_NAMED_INSTVAR(MOO_OOP_TO_SMOOI(c->spec)));
		moo_poptmp(moo);

		if (!z) return z;

		/* copy the payload */
		MOO_MEMCPY (z + 1, oop + 1, get_payload_bytes(moo, oop));

		return z;
#else
		moo_oop_t z;
		moo_oow_t total_bytes;

		total_bytes = MOO_SIZEOF(moo_obj_t) + get_payload_bytes(moo, oop);

		moo_pushtmp (moo, &oop);
		z = moo_allocbytes (moo, total_bytes);
		moo_poptmp(moo);

		MOO_MEMCPY (z, oop, total_bytes);
		return z;
#endif 
	}

	return oop;
}
