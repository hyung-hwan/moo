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

#include "moo-prv.h"


/*
 *    Apex......................
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
 *   Apex
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
 * Apex has no instance variables.
 *  
 */

struct kernel_class_info_t
{
	moo_oow_t  len;
	moo_ooch_t name[20];
	int class_flags;
	int class_num_classvars;

	int class_spec_named_instvars;
	int class_spec_flags;
	int class_spec_indexed_type;

	moo_oow_t  offset;
};
typedef struct kernel_class_info_t kernel_class_info_t;

static kernel_class_info_t kernel_classes[] = 
{
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

	{ 4,
	  { 'A','p','e','x' },
	  0,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _apex) },

	{ 15,
	  { 'U','n','d','e','f','i','n','e','d','O','b','j','e','c','t' },
	  0,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _undefined_object) },

#define KCI_CLASS 2 /* index to the Class entry in this table */
	{ 5,
	  { 'C','l','a','s','s' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_CLASS_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _class) },

	{ 9,
	  { 'I','n','t','e','r','f','a','c','e' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_INTERFACE_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _interface) },

	{ 6,
	  { 'O','b','j','e','c','t' },
	  0,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _object) },

	{ 6,
	  { 'S','t','r','i','n','g' },
	  0,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_CHAR,
	  MOO_OFFSETOF(moo_t, _string) },

	{ 6,
	  { 'S','y','m','b','o','l' },   
	  MOO_CLASS_SELFSPEC_FLAG_FINAL | MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_IMMUTABLE,
	  MOO_OBJ_TYPE_CHAR,
	  MOO_OFFSETOF(moo_t, _symbol) },

	{ 5,
	  { 'A','r','r','a','y' },
	  0,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _array) },

	{ 9,
	  { 'B','y','t','e','A','r','r','a','y' },
	  0,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_BYTE,
	  MOO_OFFSETOF(moo_t, _byte_array) },

	{ 11,
	  { 'S','y','m','b','o','l','T','a','b','l','e' },
	  0,
	  0,
	  MOO_DIC_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _symbol_table) },

	{ 10,
	  { 'D','i','c','t','i','o','n','a','r','y' },
	  0,
	  0,
	  MOO_DIC_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _dictionary) },

	{ 11,
	  { 'A','s','s','o','c','i','a','t','i','o','n' },
	  0,
	  0,
	  MOO_ASSOCIATION_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _association) },
	  
	{ 9,
	  { 'N','a','m','e','s','p','a','c','e' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_NSDIC_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _namespace) },

	{ 14,
	  { 'P','o','o','l','D','i','c','t','i','o','n','a','r','y' },
	  0,
	  0,
	  MOO_DIC_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _pool_dictionary) },

	{ 16,
	  { 'M','e','t','h','o','d','D','i','c','t','i','o','n','a','r','y' },
	  0,
	  0,
	  MOO_DIC_NAMED_INSTVARS,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _method_dictionary) },

	{ 14,
	  { 'C','o','m','p','i','l','e','d','M','e','t','h','o','d' },
	  0,
	  0,
	  MOO_METHOD_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _method) },

	{ 15,
	  { 'M','e','t','h','o','d','S','i','g','n','a','t','u','r','e' },
	  0,
	  0,
	  MOO_METHSIG_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _methsig) },


	{ 13,
	  { 'M','e','t','h','o','d','C','o','n','t','e','x','t' },
	  MOO_CLASS_SELFSPEC_FLAG_FINAL | MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_CONTEXT_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _method_context) },

	{ 12,
	  { 'B','l','o','c','k','C','o','n','t','e','x','t' },
	  MOO_CLASS_SELFSPEC_FLAG_FINAL | MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_CONTEXT_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _block_context) },

	{ 7,
	  { 'P','r','o','c','e','s','s' },
	  MOO_CLASS_SELFSPEC_FLAG_FINAL | MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_PROCESS_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _process) },

	{ 9,
	  { 'S','e','m','a','p','h','o','r','e' },
	  0,
	  0,
	  MOO_SEMAPHORE_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _semaphore) },

	{ 14,
	  { 'S','e','m','a','p','h','o','r','e','G','r','o','u','p' },
	  0,
	  0,
	  MOO_SEMAPHORE_GROUP_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _semaphore_group) },

	{ 16,
	  { 'P','r','o','c','e','s','s','S','c','h','e','d','u','l','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_FINAL | MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_PROCESS_SCHEDULER_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_UNCOPYABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _process_scheduler) },

	{ 5,
	  { 'E','r','r','o','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _error_class) },

	{ 4,
	  { 'T','r','u','e' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED | MOO_CLASS_SELFSPEC_FLAG_FINAL,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _true_class) },

	{ 5,
	  { 'F','a','l','s','e' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED | MOO_CLASS_SELFSPEC_FLAG_FINAL,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _false_class) },

	/* TOOD: what is a proper spec for Character and SmallInteger?
 	 *       If the fixed part is  0, its instance must be an object of 0 payload fields.
	 *       Does this make sense? */
	{ 9,
	  { 'C','h','a','r','a','c','t','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _character) },

	{ 12,
	  { 'S','m','a','l','l','I','n','t','e','g','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _small_integer) },

	{ 20,
	  { 'L','a','r','g','e','P','o','s','i','t','i','v','e','I','n','t','e','g','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_IMMUTABLE,
	  MOO_OBJ_TYPE_LIWORD,
	  MOO_OFFSETOF(moo_t, _large_positive_integer) },

	{ 20,
	  { 'L','a','r','g','e','N','e','g','a','t','i','v','e','I','n','t','e','g','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  MOO_CLASS_SPEC_FLAG_INDEXED | MOO_CLASS_SPEC_FLAG_IMMUTABLE,
	  MOO_OBJ_TYPE_LIWORD,
	  MOO_OFFSETOF(moo_t, _large_negative_integer) },

	{ 17,
	  { 'F','i','x','e','d','P','o','i','n','t','D','e','c','i','m','a','l' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  MOO_FPDEC_NAMED_INSTVARS,
	  MOO_CLASS_SPEC_FLAG_IMMUTABLE,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _fixed_point_decimal) },

	{ 12,
	  { 'S','m','a','l','l','P','o','i','n','t','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _small_pointer) },

	{ 12,
	  { 'L','a','r','g','e','P','o','i','n','t','e','r' },
	  MOO_CLASS_SELFSPEC_FLAG_LIMITED,
	  0,
	  1,  /* #word(1) */
	  MOO_CLASS_SPEC_FLAG_IMMUTABLE | MOO_CLASS_SPEC_FLAG_INDEXED,
	  MOO_OBJ_TYPE_WORD,
	  MOO_OFFSETOF(moo_t, _large_pointer) },

	{ 6,
	  { 'S','y','s','t','e','m' },
	  0,
	  4, /* asyncsg, gcfin_sem, gcfin_should_exit, shr */
	  0,
	  0,
	  MOO_OBJ_TYPE_OOP,
	  MOO_OFFSETOF(moo_t, _system) }
};


static moo_oow_t move_finalizable_objects (moo_t* moo);

/* ----------------------------------------------------------------------- 
 * BOOTSTRAPPER
 * ----------------------------------------------------------------------- */

static moo_oop_class_t alloc_kernel_class (moo_t* moo, int class_flags, moo_oow_t num_classvars, moo_oow_t spec)
{
	moo_oop_class_t c;
	moo_ooi_t cspec;

	c = (moo_oop_class_t)moo_allocoopobj(moo, MOO_CLASS_NAMED_INSTVARS + num_classvars);
	if (!c) return MOO_NULL;

	MOO_OBJ_SET_FLAGS_KERNEL (c, MOO_OBJ_FLAGS_KERNEL_IMMATURE);

	cspec = kernel_classes[KCI_CLASS].class_spec_flags;
	if (MOO_CLASS_SPEC_IS_IMMUTABLE(cspec)) MOO_OBJ_SET_FLAGS_RDONLY (c, 1); /* just for completeness of code. will never be true as it's not defined in the kernel class info table */
	if (MOO_CLASS_SPEC_IS_UNCOPYABLE(cspec)) MOO_OBJ_SET_FLAGS_UNCOPYABLE (c, 1); /* class itself is uncopyable */

	MOO_OBJ_SET_CLASS (c, (moo_oop_t)moo->_class);
	c->spec = MOO_SMOOI_TO_OOP(spec); 
	c->selfspec = MOO_SMOOI_TO_OOP(MOO_CLASS_SELFSPEC_MAKE(num_classvars, 0, class_flags));

	return c;
}

static int ignite_1 (moo_t* moo)
{
	moo_oow_t i;

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
	moo->_class = alloc_kernel_class(
		moo, kernel_classes[KCI_CLASS].class_flags, 
		kernel_classes[KCI_CLASS].class_num_classvars, 
		MOO_CLASS_SPEC_MAKE(kernel_classes[KCI_CLASS].class_spec_named_instvars,
		                    kernel_classes[KCI_CLASS].class_spec_flags,
		                    kernel_classes[KCI_CLASS].class_spec_indexed_type));
	if (!moo->_class) return -1;

	MOO_ASSERT (moo, MOO_OBJ_GET_CLASS(moo->_class) == MOO_NULL);
	MOO_OBJ_SET_CLASS (moo->_class, (moo_oop_t)moo->_class);

	for (i = 0; i < MOO_COUNTOF(kernel_classes); i++)
	{
		moo_oop_class_t tmp;

		if (i == KCI_CLASS) continue; /* skip Class as it's created above */

		tmp = alloc_kernel_class(
			moo, kernel_classes[i].class_flags,
			kernel_classes[i].class_num_classvars, 
			MOO_CLASS_SPEC_MAKE(kernel_classes[i].class_spec_named_instvars,
			                    kernel_classes[i].class_spec_flags,
			                    kernel_classes[i].class_spec_indexed_type));
		if (!tmp) return -1;
		*(moo_oop_class_t*)((moo_uint8_t*)moo + kernel_classes[i].offset) = tmp;
	}

	MOO_OBJ_SET_CLASS (moo->_nil, (moo_oop_t)moo->_undefined_object);

	/* an instance of a method class stores byte codes in the trailer space.
	 * unlike other classes with trailer size set, the size of the trailer 
	 * space is not really determined by the traailer size set in the class.
	 * the compiler determines the actual size of the trailer space depending
	 * on the byte codes generated. i should set the following fields to avoid
	 * confusion at the GC phase. */
	moo->_method->trsize = MOO_SMOOI_TO_OOP(0);
	moo->_method->trgc = MOO_SMPTR_TO_OOP(0);

	return 0;
}

static int ignite_2 (moo_t* moo)
{
	moo_oop_t tmp;
	int old_igniting = moo->igniting;

	/* Create 'true' and 'false objects */
	moo->_true = moo_instantiate(moo, moo->_true_class, MOO_NULL, 0);
	moo->_false = moo_instantiate(moo, moo->_false_class, MOO_NULL, 0);
	if (!moo->_true || !moo->_false) return -1;

	/* Prevent the object instations in the permspace. 
	 *
	 * 1. The symbol table is big and it may resize after ignition.
	 *    the resizing operation will migrate the obejct out of the
	 *    permspace. The space taken by the symbol table and the
	 *    system dictionary is wasted. I'd rather allocate these
	 *    in the normal space. 
	 *  
	 * 2. For compact_symbol_table() to work properly, moo_gc() must not
	 *    scan the symbol table before it executes compact_symbol_table().
	 *    since moo_gc() scans the entire perspace, it naturally gets to 
	 *    moo->symtab, which causes problems in compact_symbol_table(). 
	 *    I may reserve a special space for only the symbol table
	 *    to overcome this issue.
	 *
	 * For now, let's just allocate the symbol table and the system dictionary
	 * in the normal space */
	moo->igniting = 0;

	/* Create the symbol table */
	tmp = moo_instantiate(moo, moo->_symbol_table, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->symtab = (moo_oop_dic_t)tmp;

	moo->symtab->tally = MOO_SMOOI_TO_OOP(0);
	/* It's important to assign the result of moo_instantiate() to a temporary
	 * variable first and then assign it to moo->symtab->bucket. 
	 * The pointer 'moo->symtab; can change in moo_instantiate() and the 
	 * target address of assignment may get set before moo_instantiate()
	 * is called. */
	tmp = moo_instantiate(moo, moo->_array, MOO_NULL, moo->option.dfl_symtab_size);
	if (!tmp) return -1;
	moo->symtab->bucket = (moo_oop_oop_t)tmp;

	/* Create the system dictionary */
	tmp = (moo_oop_t)moo_makensdic(moo, moo->_namespace, moo->option.dfl_sysdic_size);
	if (!tmp) return -1;
	moo->sysdic = (moo_oop_nsdic_t)tmp;

	moo->igniting = old_igniting; /* back to the permspace */

	/* Create a nil process used to simplify nil check in GC.
	 * only accessible by VM. not exported via the global dictionary. */
	tmp = (moo_oop_t)moo_instantiate(moo, moo->_process, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->nil_process = (moo_oop_process_t)tmp;
	moo->nil_process->sp = MOO_SMOOI_TO_OOP(-1);
	moo->nil_process->id = MOO_SMOOI_TO_OOP(-1);
	moo->nil_process->perr = MOO_ERROR_TO_OOP(MOO_ENOERR);
	moo->nil_process->perrmsg = moo->_nil;

	/* Create a process scheduler */
	tmp = (moo_oop_t)moo_instantiate(moo, moo->_process_scheduler, MOO_NULL, 0);
	if (!tmp) return -1;
	moo->processor = (moo_oop_process_scheduler_t)tmp;
	moo->processor->active = moo->nil_process;
	moo->processor->total_count = MOO_SMOOI_TO_OOP(0);
	moo->processor->runnable.count = MOO_SMOOI_TO_OOP(0);
	moo->processor->suspended.count = MOO_SMOOI_TO_OOP(0);

	return 0;
}

static int ignite_3 (moo_t* moo)
{
	/* Register kernel classes manually created so far to the system dictionary */
	static moo_ooch_t str_processor[] = { 'P', 'r', 'o', 'c', 'e', 's', 's', 'o', 'r' };
	static moo_ooch_t str_dicnew[] = { 'n', 'e', 'w', ':' };
	static moo_ooch_t str_dicputassoc[] = { '_','_','p', 'u', 't', '_', 'a', 's', 's', 'o', 'c', ':' };
	static moo_ooch_t str_does_not_understand[] = { 'd', 'o', 'e', 's', 'N', 'o', 't', 'U', 'n', 'd', 'e', 'r', 's', 't', 'a', 'n', 'd', ':' };
	static moo_ooch_t str_primitive_failed[] = {  'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 'F', 'a', 'i', 'l', 'e', 'd' };
	static moo_ooch_t str_unwindto_return[] = { 'u', 'n', 'w', 'i', 'n', 'd', 'T', 'o', ':', 'r', 'e', 't', 'u', 'r', 'n', ':' };
	
	moo_oow_t i;
	moo_oop_t sym;
	moo_oop_class_t cls;

	for (i = 0; i < MOO_COUNTOF(kernel_classes); i++)
	{
		sym = moo_makesymbol(moo, kernel_classes[i].name, kernel_classes[i].len);
		if (!sym) return -1;

		cls = *(moo_oop_class_t*)((moo_uint8_t*)moo + kernel_classes[i].offset);
		MOO_STORE_OOP (moo, (moo_oop_t*)&cls->name, sym);
		MOO_STORE_OOP (moo, (moo_oop_t*)&cls->nsup, (moo_oop_t)moo->sysdic);

		if (!moo_putatsysdic(moo, sym, (moo_oop_t)cls)) return -1;
	}

	/* Attach the system dictionary to the nsdic field of the System class */
	MOO_STORE_OOP (moo, (moo_oop_t*)&moo->_system->nsdic, (moo_oop_t)moo->sysdic);
	/* Set the name field of the system dictionary */
	MOO_STORE_OOP (moo, (moo_oop_t*)&moo->sysdic->name, (moo_oop_t)moo->_system->name);
	/* Set the owning class field of the system dictionary, it's circular here */
	MOO_STORE_OOP (moo, (moo_oop_t*)&moo->sysdic->nsup, (moo_oop_t)moo->_system);

	/* Make the process scheduler avaialble as the global name 'Processor' */
	sym = moo_makesymbol(moo, str_processor, MOO_COUNTOF(str_processor));
	if (!sym) return -1;
	if (!moo_putatsysdic(moo, sym, (moo_oop_t)moo->processor)) return -1;

	sym = moo_makesymbol(moo, str_dicnew, MOO_COUNTOF(str_dicnew));
	if (!sym) return -1;
	moo->dicnewsym = (moo_oop_char_t)sym;

	sym = moo_makesymbol(moo, str_dicputassoc, MOO_COUNTOF(str_dicputassoc));
	if (!sym) return -1;
	moo->dicputassocsym = (moo_oop_char_t)sym;

	sym = moo_makesymbol(moo, str_does_not_understand, MOO_COUNTOF(str_does_not_understand));
	if (!sym) return -1;
	moo->does_not_understand_sym = (moo_oop_char_t)sym;
	
	sym = moo_makesymbol(moo, str_primitive_failed, MOO_COUNTOF(str_primitive_failed));
	if (!sym) return -1;
	moo->primitive_failed_sym = (moo_oop_char_t)sym;
	
	sym = moo_makesymbol(moo, str_unwindto_return, MOO_COUNTOF(str_unwindto_return));
	if (!sym) return -1;
	moo->unwindto_return_sym = (moo_oop_char_t)sym;

	return 0;
}

int moo_ignite (moo_t* moo, moo_oow_t heapsz)
{
	MOO_ASSERT (moo, moo->_nil == MOO_NULL);

	if (moo->heap) moo_killheap (moo, moo->heap);
	moo->heap = moo_makeheap(moo, heapsz);
	if (!moo->heap) return -1;

	moo->igniting = 1;
	moo->_nil = moo_allocbytes(moo, MOO_SIZEOF(moo_obj_t));
	if (!moo->_nil) goto oops;

	moo->_nil->_flags = MOO_OBJ_MAKE_FLAGS(MOO_OBJ_TYPE_OOP, MOO_SIZEOF(moo_oop_t), 0, 1, moo->igniting, 0, 0, 0, 0, 0);
	moo->_nil->_size = 0;

	if (ignite_1(moo) <= -1 || ignite_2(moo) <= -1 || ignite_3(moo)) goto oops;;

	moo->igniting = 0;
	return 0;

oops:
	moo->igniting = 0;
	return -1;
}

/* ----------------------------------------------------------------------- 
 * GARBAGE COLLECTOR
 * ----------------------------------------------------------------------- */

static void compact_symbol_table (moo_t* moo, moo_oop_t _nil)
{
	moo_oop_oop_t bucket;
	moo_oop_t tmp;
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

	bucket = moo->symtab->bucket;
	/* [NOTE] in theory, the bucket size can be greater than MOO_SMOOI_MAX
	 * as it is an internal header field and is of an unsigned type */
	bucket_size = MOO_OBJ_GET_SIZE(bucket);

	for (index = 0; index < bucket_size; )
	{
		tmp = MOO_OBJ_GET_OOP_VAL(bucket, index);
		if (MOO_OBJ_GET_FLAGS_PERM(tmp) || MOO_OBJ_GET_FLAGS_MOVED(tmp))
		{
			index++;
			continue;
		}

		MOO_ASSERT (moo, tmp != _nil);
		MOO_LOG2 (moo, MOO_LOG_GC | MOO_LOG_INFO, "Compacting away a symbol - %.*js\n", MOO_OBJ_GET_SIZE(tmp), MOO_OBJ_GET_CHAR_SLOT(tmp));

		for (i = 0, x = index, y = index; i < bucket_size; i++)
		{
			y = (y + 1) % bucket_size;

			/* done if the slot at the current hash index is _nil */
			tmp = MOO_OBJ_GET_OOP_VAL(bucket, y);
			if (tmp == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			MOO_ASSERT (moo, MOO_CLASSOF(moo,tmp) == moo->_symbol);
			z = moo_hash_oochars(MOO_OBJ_GET_CHAR_SLOT(tmp), MOO_OBJ_GET_SIZE(tmp)) % bucket_size;

			/* move an element if necessary */
			if ((y > x && (z <= x || z > y)) || (y < x && (z <= x && z > y)))
			{
				tmp = MOO_OBJ_GET_OOP_VAL(bucket, y);
				/* this function is called as part of garbage collection.
				 * i must not use MOO_STORE_OOP for object relocation */
				MOO_OBJ_SET_OOP_VAL (bucket, x, tmp);
				x = y;
			}
		}

		MOO_OBJ_SET_OOP_VAL (bucket, x, _nil);
		tally--;
	}

	MOO_ASSERT (moo, tally >= 0);
	MOO_ASSERT (moo, tally <= MOO_SMOOI_MAX);
	moo->symtab->tally = MOO_SMOOI_TO_OOP(tally);
}

moo_oow_t moo_getobjpayloadbytes (moo_t* moo, moo_oop_t oop)
{
	moo_oow_t nbytes;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));

	if (MOO_OBJ_GET_FLAGS_TRAILER(oop))
	{
		/* only an OOP object can have the trailer. 
		 *
		 * | _flags    |
		 * | _size     |  <-- if it's 3
		 * | _class    |
		 * |   X       |
		 * |   X       |
		 * |   X       |
		 * |   Y       | <-- it may exist if EXTRA is set in _flags.
		 * |   Z       | <-- if TRAILER is set, it is a word indicating the number of bytes in the trailer
		 * |  |  |  |  | <-- trailer bytes. it's aligned to a word boundary.
		 * | hash      | <-- if HASH is set to 2 in _flags, a word is used to store a hash value
		 */
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP);
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_UNIT(oop) == MOO_SIZEOF(moo_oow_t));
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_EXTRA(oop) == 0); /* no 'extra' for an OOP object */

		nbytes = MOO_OBJ_BYTESOF(oop) + MOO_SIZEOF(moo_oow_t) + MOO_OBJ_GET_TRAILER_SIZE(oop);
	}
	else
	{
		/* calculate the payload size in bytes */
		nbytes = MOO_OBJ_BYTESOF(oop);
	}

	nbytes = MOO_ALIGN(nbytes, MOO_SIZEOF(moo_oop_t));
	if (MOO_OBJ_GET_FLAGS_HASH(oop) == MOO_OBJ_FLAGS_HASH_STORED) 
	{
		MOO_STATIC_ASSERT (MOO_SIZEOF(moo_oop_t) == MOO_SIZEOF(moo_oow_t));
		nbytes += MOO_SIZEOF(moo_oow_t);
	}

	return nbytes;
}

moo_oop_t moo_moveoop (moo_t* moo, moo_oop_t oop)
{
#if defined(MOO_SUPPORT_GC_DURING_IGNITION)
	if (!oop) return oop;
#endif

	if (!MOO_OOP_IS_POINTER(oop)) return oop;

	if (MOO_OBJ_GET_FLAGS_PERM(oop)) return oop;

	if (MOO_OBJ_GET_FLAGS_MOVED(oop))
	{
		/* this object has migrated to the new heap. 
		 * the class field has been updated to the new object
		 * in the 'else' block below. i can simply return it
		 * without further migration. */
		return (moo_oop_t)MOO_OBJ_GET_CLASS(oop);
	}
	else
	{
		moo_oow_t nbytes_aligned, extra_bytes = 0;
		moo_oop_t tmp;

		nbytes_aligned = moo_getobjpayloadbytes(moo, oop);

		if (MOO_OBJ_GET_FLAGS_HASH(oop) == MOO_OBJ_FLAGS_HASH_CALLED)
		{
			MOO_STATIC_ASSERT (MOO_SIZEOF(moo_oop_t) == MOO_SIZEOF(moo_oow_t)); 
			/* don't need explicit alignment since oop and oow have the same size 
			extra_bytes = MOO_ALIGN(MOO_SIZEOF(moo_oow_t), MOO_SIZEOF(moo_oop_t)); */
			extra_bytes = MOO_SIZEOF(moo_oow_t); 
		}

		/* allocate space in the new heap */
		tmp = moo_allocheapspace(moo, &moo->heap->newspace, MOO_SIZEOF(moo_obj_t) + nbytes_aligned + extra_bytes);

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
		MOO_OBJ_SET_FLAGS_MOVED (oop, 1);

		/* let the class field of the old object point to the new 
		 * object allocated in the new heap. it is returned in 
		 * the 'if' block at the top of this function. */
		MOO_OBJ_SET_CLASS (oop, tmp);

		if (extra_bytes)
		{
			MOO_OBJ_SET_FLAGS_HASH (tmp, MOO_OBJ_FLAGS_HASH_STORED);
			*(moo_oow_t*)((moo_uint8_t*)tmp + MOO_SIZEOF(moo_obj_t) + nbytes_aligned) = (moo_oow_t)oop % (MOO_SMOOI_MAX + 1);
		}

		/* return the new object */
		return tmp;
	}
}

static moo_uint8_t* scan_heap_space (moo_t* moo, moo_uint8_t* ptr, moo_uint8_t** end)
{
	while (ptr < *end) /* the end pointer may get changed, especially the new space. so it's moo_int8_t** */
	{
		moo_oow_t i;
		moo_oow_t nbytes_aligned;
		moo_oop_t oop, tmp;

		oop = (moo_oop_t)ptr;
		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));

		nbytes_aligned = moo_getobjpayloadbytes(moo, oop);

		tmp = moo_moveoop(moo, (moo_oop_t)MOO_OBJ_GET_CLASS(oop));
		MOO_OBJ_SET_CLASS (oop, tmp);

		if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP)
		{
			moo_oow_t size;

			/* is it really better to use a flag bit in the header to
			 * determine that it is an instance of process? */
			if (MOO_UNLIKELY(MOO_OBJ_GET_FLAGS_PROC(oop)))
			{
				/* the stack in a process object doesn't need to be 
				 * scanned in full. the slots above the stack pointer 
				 * are garbages. */
				size = MOO_PROCESS_NAMED_INSTVARS + MOO_OOP_TO_SMOOI(((moo_oop_process_t)oop)->sp) + 1;
				MOO_ASSERT (moo, size <= MOO_OBJ_GET_SIZE(oop));
			}
			else
			{
				size = MOO_OBJ_GET_SIZE(oop);
			}

			for (i = 0; i < size; i++)
			{
				tmp = MOO_OBJ_GET_OOP_VAL(oop, i);
				if (MOO_OOP_IS_POINTER(tmp))
				{
					/* i must not use MOO_STORE_OOP() as this operation is 
					 * part of garbage collection. */
					tmp = moo_moveoop(moo, tmp);
					MOO_OBJ_SET_OOP_VAL (oop, i, tmp);
				}
			}
		}

		if (MOO_OBJ_GET_FLAGS_TRAILER(oop))
		{
			moo_oop_class_t c;
			moo_trgc_t trgc;

			/* i use SMPTR(0) to indicate no trailer gc callback.
			 * i don't use moo->_nil because c->trgc field may not have
			 * been updated to a new nil address while moo->_nil could
			 * have been updated in the process of garbage collection.
			 * this comment will be invalidated when moo->_nil is 
			 * stored in a permanent heap or GC gets changed to
			 * a non-copying collector. no matter what GC implementation
			 * i choose, using SMPTR(0) for this purpose is safe. */
			c = MOO_OBJ_GET_CLASS(oop);
			MOO_ASSERT(moo, MOO_OOP_IS_SMPTR(c->trgc));
			trgc = MOO_OOP_TO_SMPTR(c->trgc);
			if (trgc) trgc (moo, oop);
		}

		ptr += MOO_SIZEOF(moo_obj_t) + nbytes_aligned;
	}

	/* return the pointer to the beginning of the free space in the heap */
	return ptr; 
}

static moo_rbt_walk_t call_module_gc (moo_rbt_t* rbt, moo_rbt_pair_t* pair, void* ctx)
{
	moo_t* moo = (moo_t*)ctx;
	moo_mod_data_t* mdp;

	mdp = MOO_RBT_VPTR(pair);
	MOO_ASSERT (moo, mdp != MOO_NULL);

	if (mdp->mod.gc) mdp->mod.gc (moo, &mdp->mod);

	return MOO_RBT_WALK_FORWARD;
}

void moo_gc (moo_t* moo)
{
	/* 
	 * move a referenced object to the new heap.
	 * inspect the fields of the moved object in the new heap.
	 * move objects pointed to by the fields to the new heap.
	 * finally perform some tricky symbol table clean-up.
	 */
	moo_uint8_t* newspace_scan_ptr;
	moo_space_t tmp;
	moo_oop_t old_nil;
	moo_oow_t i;
	moo_evtcb_t* cb;
	moo_oow_t gcfin_count;

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
		moo->heap->curspace.base, moo->heap->curspace.ptr, moo->heap->newspace.base, moo->heap->newspace.ptr); 

	newspace_scan_ptr = (moo_uint8_t*)MOO_ALIGN((moo_uintptr_t)moo->heap->newspace.base, MOO_SIZEOF(moo_oop_t));

	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */
	old_nil = moo->_nil;

	/* move _nil and the root object table */
	moo->_nil = moo_moveoop(moo, moo->_nil);
	moo->_true = moo_moveoop(moo, moo->_true);
	moo->_false = moo_moveoop(moo, moo->_false);

	for (i = 0; i < MOO_COUNTOF(kernel_classes); i++)
	{
		moo_oop_t tmp;
		tmp = *(moo_oop_t*)((moo_uint8_t*)moo + kernel_classes[i].offset);
		tmp = moo_moveoop(moo, tmp);
		*(moo_oop_t*)((moo_uint8_t*)moo + kernel_classes[i].offset) = tmp;
	}

	moo->sysdic = (moo_oop_nsdic_t)moo_moveoop(moo, (moo_oop_t)moo->sysdic);
	moo->processor = (moo_oop_process_scheduler_t)moo_moveoop(moo, (moo_oop_t)moo->processor);
	moo->nil_process = (moo_oop_process_t)moo_moveoop(moo, (moo_oop_t)moo->nil_process);
	moo->dicnewsym = (moo_oop_char_t)moo_moveoop(moo, (moo_oop_t)moo->dicnewsym);
	moo->dicputassocsym = (moo_oop_char_t)moo_moveoop(moo, (moo_oop_t)moo->dicputassocsym);
	moo->does_not_understand_sym = (moo_oop_char_t)moo_moveoop(moo, (moo_oop_t)moo->does_not_understand_sym);
	moo->primitive_failed_sym = (moo_oop_char_t)moo_moveoop(moo, (moo_oop_t)moo->primitive_failed_sym);
	moo->unwindto_return_sym = (moo_oop_char_t)moo_moveoop(moo, (moo_oop_t)moo->unwindto_return_sym);

	for (i = 0; i < moo->sem_list_count; i++)
	{
		moo->sem_list[i] = (moo_oop_semaphore_t)moo_moveoop(moo, (moo_oop_t)moo->sem_list[i]);
	}

	for (i = 0; i < moo->sem_heap_count; i++)
	{
		moo->sem_heap[i] = (moo_oop_semaphore_t)moo_moveoop(moo, (moo_oop_t)moo->sem_heap[i]);
	}

	for (i = 0; i < moo->sem_io_tuple_count; i++)
	{
		if (moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_INPUT])
			moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_INPUT] = (moo_oop_semaphore_t)moo_moveoop(moo, (moo_oop_t)moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_INPUT]);
		if (moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT])
			moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT] = (moo_oop_semaphore_t)moo_moveoop(moo, (moo_oop_t)moo->sem_io_tuple[i].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT]);
	}

	moo->sem_gcfin = (moo_oop_semaphore_t)moo_moveoop(moo, (moo_oop_t)moo->sem_gcfin);

	for (i = 0; i < moo->proc_map_capa; i++)
	{
		moo->proc_map[i] = moo_moveoop(moo, moo->proc_map[i]);
	}

	for (i = 0; i < moo->volat_count; i++)
	{
		*moo->volat_stack[i] = moo_moveoop(moo, *moo->volat_stack[i]);
	}

	if (moo->initial_context)
		moo->initial_context = (moo_oop_context_t)moo_moveoop(moo, (moo_oop_t)moo->initial_context);
	if (moo->active_context)
		moo->active_context = (moo_oop_context_t)moo_moveoop(moo, (moo_oop_t)moo->active_context);
	if (moo->active_method)
		moo->active_method = (moo_oop_method_t)moo_moveoop(moo, (moo_oop_t)moo->active_method);

	moo_rbt_walk (&moo->modtab, call_module_gc, moo); 

	for (cb = moo->evtcb_list; cb; cb = cb->next)
	{
		if (cb->gc) cb->gc (moo);
	}

	/* scan the objects in the permspace in case an object there points to an object outside the permspace */
	scan_heap_space (moo, (moo_uint8_t*)MOO_ALIGN((moo_uintptr_t)moo->heap->permspace.base, MOO_SIZEOF(moo_oop_t)), &moo->heap->permspace.ptr);

	/* scan the new heap to move referenced objects */
	newspace_scan_ptr = scan_heap_space(moo, newspace_scan_ptr, &moo->heap->newspace.ptr);

	/* check finalizable objects registered and scan the heap again. 
	 * symbol table compation is placed after this phase assuming that
	 * no symbol is added to be finalized. */
	gcfin_count = move_finalizable_objects(moo);
	newspace_scan_ptr = scan_heap_space(moo, newspace_scan_ptr, &moo->heap->newspace.ptr);

	/* traverse the symbol table for unreferenced symbols.
	 * if the symbol has not moved to the new heap, the symbol
	 * is not referenced by any other objects than the symbol 
	 * table itself */
	/*if (!moo->igniting) */ compact_symbol_table (moo, old_nil);

	/* move the symbol table itself */
	moo->symtab = (moo_oop_dic_t)moo_moveoop(moo, (moo_oop_t)moo->symtab);

	/* scan the new heap again from the end position of
	 * the previous scan to move referenced objects by 
	 * the symbol table. */
	newspace_scan_ptr = scan_heap_space(moo, newspace_scan_ptr, &moo->heap->newspace.ptr);

	/* the contents of the current heap is not needed any more.
	 * reset the upper bound to the base. don't forget to align the heap
	 * pointer to the OOP size. See moo_makeheap() also */
	moo->heap->curspace.ptr = (moo_uint8_t*)MOO_ALIGN(((moo_uintptr_t)moo->heap->curspace.base), MOO_SIZEOF(moo_oop_t));

	/* swap the current heap and old heap */
	tmp = moo->heap->curspace;
	moo->heap->curspace = moo->heap->newspace;
	moo->heap->newspace = tmp;

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

	if (moo->active_method) moo->active_code = MOO_METHOD_GET_CODE_BYTE(moo->active_method); /* update moo->active_code */
	if (gcfin_count > 0) moo->sem_gcfin_sigreq = 1;

	/* invalidate method cache. TODO: GCing entries on the method cache is also one way instead of full invalidation */
	moo_clearmethodcache (moo);

	/* TODO: include some gc statstics like number of live objects, gc performance, etc */
	MOO_LOG4 (moo, MOO_LOG_GC | MOO_LOG_INFO, 
		"Finished GC curheap base %p ptr %p newheap base %p ptr %p\n",
		moo->heap->curspace.base, moo->heap->curspace.ptr, moo->heap->newspace.base, moo->heap->newspace.ptr); 
}

void moo_pushvolat (moo_t* moo, moo_oop_t* oop_ptr)
{
	/* if you have too many temporaries pushed, something must be wrong.
	 * change your code not to exceede the stack limit */
	MOO_ASSERT (moo, moo->volat_count < MOO_COUNTOF(moo->volat_stack));
	moo->volat_stack[moo->volat_count++] = oop_ptr;
}

void moo_popvolat (moo_t* moo)
{
	MOO_ASSERT (moo, moo->volat_count > 0);
	moo->volat_count--;
}

void moo_popvolats (moo_t* moo, moo_oow_t count)
{
	MOO_ASSERT (moo, moo->volat_count >= count);
	moo->volat_count -= count;
}

moo_oop_t moo_shallowcopy (moo_t* moo, moo_oop_t oop)
{
	if (MOO_OOP_IS_POINTER(oop) && MOO_OBJ_GET_CLASS(oop) != moo->_symbol)
	{
		moo_oop_t z;
		moo_oow_t total_bytes;

		if (MOO_OBJ_GET_FLAGS_UNCOPYABLE(oop))
		{
/* TOOD: should i disallow this or return without copying? */
			moo_seterrbfmt (moo, MOO_EPERM, "uncopyable object");
			return MOO_NULL;
		}

		total_bytes = MOO_SIZEOF(moo_obj_t) + moo_getobjpayloadbytes(moo, oop);
		if (MOO_OBJ_GET_FLAGS_HASH(oop) == MOO_OBJ_FLAGS_HASH_STORED)
		{
			/* exclude the hash value field from copying */
			total_bytes -= MOO_SIZEOF(moo_oow_t);
		}

		moo_pushvolat (moo, &oop);
		z = (moo_oop_t)moo_allocbytes(moo, total_bytes);
		moo_popvolat(moo);

		MOO_MEMCPY (z, oop, total_bytes);
		MOO_OBJ_SET_FLAGS_RDONLY (z, 0); /* a copied object is not read-only */
		MOO_OBJ_SET_FLAGS_HASH (z, 0); /* no hash field */
		return z; 
	}

	return oop;
}


int moo_regfinalizable (moo_t* moo, moo_oop_t oop)
{
	moo_finalizable_t* x;

	if (!MOO_OOP_IS_POINTER(oop) || (MOO_OBJ_GET_FLAGS_GCFIN(oop) & (MOO_GCFIN_FINALIZABLE | MOO_GCFIN_FINALIZED)))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		return -1;
	}

	x = (moo_finalizable_t*)moo_allocmem(moo, MOO_SIZEOF(*x));
	if (!x) return -1;

	MOO_OBJ_SET_FLAGS_GCFIN (oop, MOO_GCFIN_FINALIZABLE);
	x->oop = oop;

	MOO_APPEND_TO_LIST (&moo->finalizable, x);

	return 0;
}


int moo_deregfinalizable (moo_t* moo, moo_oop_t oop)
{
	moo_finalizable_t* x;

	if (!MOO_OOP_IS_POINTER(oop) || ((MOO_OBJ_GET_FLAGS_GCFIN(oop) & (MOO_GCFIN_FINALIZABLE | MOO_GCFIN_FINALIZED)) != MOO_GCFIN_FINALIZABLE))
	{
		moo_seterrnum (moo, MOO_EINVAL);
		return -1;
	}

	x = moo->finalizable.first;
	while (x)
	{
		if (x->oop == oop)
		{
			MOO_OBJ_SET_FLAGS_GCFIN(oop, (MOO_OBJ_GET_FLAGS_GCFIN(oop) & ~MOO_GCFIN_FINALIZABLE));
			MOO_DELETE_FROM_LIST (&moo->finalizable, x);
			moo_freemem (moo, x);
			return  0;
		}
		x = x->next;
	}

	moo_seterrnum (moo, MOO_ENOENT);
	return -1;
}

void moo_deregallfinalizables (moo_t* moo)
{
	moo_finalizable_t* x, * nx;

	x = moo->finalizable.first;
	while (x)
	{
		nx = x->next;
		MOO_OBJ_SET_FLAGS_GCFIN(x->oop, (MOO_OBJ_GET_FLAGS_GCFIN(x->oop) & ~MOO_GCFIN_FINALIZABLE));
		MOO_DELETE_FROM_LIST (&moo->finalizable, x);
		moo_freemem (moo, x);
		x = nx;
	}

	MOO_ASSERT (moo, moo->finalizable.first == MOO_NULL);
	MOO_ASSERT (moo, moo->finalizable.last == MOO_NULL);
}

static moo_oow_t move_finalizable_objects (moo_t* moo)
{
	moo_finalizable_t* x, * y;
	moo_oow_t count = 0;

	for (x = moo->collectable.first; x; x = x->next)
	{
		MOO_ASSERT (moo, (MOO_OBJ_GET_FLAGS_GCFIN(x->oop) & (MOO_GCFIN_FINALIZABLE | MOO_GCFIN_FINALIZED)) == MOO_GCFIN_FINALIZABLE);
		x->oop = moo_moveoop(moo, x->oop);
	}

	for (x = moo->finalizable.first; x; )
	{
		y = x->next;

		MOO_ASSERT (moo, (MOO_OBJ_GET_FLAGS_GCFIN(x->oop) & (MOO_GCFIN_FINALIZABLE | MOO_GCFIN_FINALIZED)) == MOO_GCFIN_FINALIZABLE);

		if (!MOO_OBJ_GET_FLAGS_MOVED(x->oop))
		{
			/* the object has not been moved. it means this object is not reachable
			 * from the root except this finalizable list. this object would be
			 * garbage if not for finalizatin. it's almost collectable. but it
			 * will survive this cycle for finalization.
			 * 
			 * if garbages consist of finalizable objects only, GC should fail miserably.
			 * however this is quite unlikely because some key objects for VM execution 
			 * like context objects doesn't require finalization. */

			x->oop = moo_moveoop(moo, x->oop);

			/* remove it from the finalizable list */
			MOO_DELETE_FROM_LIST (&moo->finalizable, x);

			/* add it to the collectable list */
			MOO_APPEND_TO_LIST (&moo->collectable, x);

			count++;
		}
		else
		{
			x->oop = moo_moveoop (moo, x->oop);
		}

		x = y;
	}

	return count;
}
