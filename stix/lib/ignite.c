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
 * 
 * Stix has no instance variables.
 * Stix has 1 class variable: Sysdic
 *  
 */

static stix_oop_t alloc_kernel_class (stix_t* stix, stix_oow_t indexed, stix_oow_t spec)
{
	stix_oop_class_t c;

	c = (stix_oop_class_t)stix_allocoopobj (stix, STIX_CLASS_NAMED_INSTVARS + indexed);
	if (!c) return STIX_NULL;

	STIX_OBJ_SET_FLAGS_KERNEL (c, 1);
	STIX_OBJ_SET_CLASS (c, stix->_class);
	c->spec = STIX_OOP_FROM_SMINT(spec); 
	c->selfspec = STIX_OOP_FROM_SMINT(STIX_CLASS_SELFSPEC_MAKE(indexed, 0));

	return (stix_oop_t)c;
}

static int ignite_1 (stix_t* stix)
{
	/*
	 * Create fundamental class objects with some fields mis-initialized yet.
	 * Such fields include 'superclass', 'subclasses', 'name', etc.
	 */
	STIX_ASSERT (stix->_nil != STIX_NULL);
	STIX_ASSERT (STIX_OBJ_GET_CLASS(stix->_nil) == STIX_NULL);

	/* --------------------------------------------------------------
	 * Class
	 * The instance of Class can have indexed instance variables 
	 * which are actually class variables.
	 * -------------------------------------------------------------- */
	stix->_class = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_CLASS_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	if (!stix->_class) return -1;

	STIX_ASSERT (STIX_OBJ_GET_CLASS(stix->_class) == STIX_NULL);
	STIX_OBJ_SET_CLASS (stix->_class, stix->_class);

	/* --------------------------------------------------------------
	 * Stix      - proto-object with 1 class variable.
	 * NilObject - class for the nil object.
	 * Object    - top of all ordinary objects.
	 * String
	 * Symbol
	 * Array
	 * ByteArray
	 * SymbolSet
	 * Character
	 * SmallIntger
	 * -------------------------------------------------------------- */
	stix->_stix              = alloc_kernel_class (stix, 1, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_nil_object        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_object            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_array             = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_OOP));
	stix->_byte_array        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_BYTE));
	stix->_string            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
	stix->_symbol            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
	stix->_symbol_set        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_system_dictionary = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_method_dictionary = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_method            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_METHOD_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	stix->_association       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_ASSOCIATION_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_true_class        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_false_class       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));

	/* TOOD: what is a proper spec for Character and SmallInteger?
 	 *       If the fixed part is  0, its instance must be an object of 0 payload fields.
	 *       Does this make sense? */
	stix->_character         = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_small_integer     = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));

	if (!stix->_stix              || !stix->_nil_object        || !stix->_object ||
	    !stix->_array             || !stix->_byte_array        ||
	    !stix->_string            || !stix->_symbol            ||
	    !stix->_symbol_set        || !stix->_system_dictionary || !stix->_method_dictionary ||
	    !stix->_association       || !stix->_true_class        || !stix->_false_class       ||
	    !stix->_character         || !stix->_small_integer) return -1;
	STIX_OBJ_SET_CLASS (stix->_nil, stix->_nil_object);
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

	stix->symtab->tally = STIX_OOP_FROM_SMINT(0);
	/* It's important to assign the result of stix_instantiate() to a temporary
	 * variable first and then assign it to stix->symtab->bucket. 
	 * The pointer 'stix->symtab; can change in stix_instantiate() and the 
	 * target address of assignment may get set before stix_instantiate()
	 * is called. */
	tmp = stix_instantiate (stix, stix->_array, STIX_NULL, stix->option.dfl_symtab_size);
	if (!tmp) return -1;
	stix->symtab->bucket = (stix_oop_oop_t)tmp;

	/* Create the system dictionary */
	tmp = stix_makedic (stix, stix->_system_dictionary, stix->option.dfl_sysdic_size);
	if (!tmp) return -1;
	stix->sysdic = (stix_oop_set_t)tmp;

	/* Export the system dictionary via the first class variable of the Stix class */
	((stix_oop_class_t)stix->_stix)->classvar[0] = (stix_oop_t)stix->sysdic;

	return 0;
}

static int ignite_3 (stix_t* stix)
{
	/* Register kernel classes manually created so far to the system dictionary */

	static struct symbol_name_t
	{
		stix_oow_t len;
		stix_uch_t str[16];
	} symnames[] = {
		{  4, { 'S','t','i','x'                                                  } },
		{  9, { 'N','i','l','O','b','j','e','c','t'                              } },
		{  5, { 'C','l','a','s','s'                                              } },
		{  6, { 'O','b','j','e','c','t'                                          } },
		{  6, { 'S','t','r','i','n','g'                                          } },
		{  6, { 'S','y','m','b','o','l'                                          } },
		{  5, { 'A','r','r','a','y'                                              } },
		{  9, { 'B','y','t','e','A','r','r','a','y'                              } },
		{  9, { 'S','y','m','b','o','l','S','e','t'                              } },
		{ 16, { 'S','y','s','t','e','m','D','i','c','t','i','o','n','a','r','y'  } },
		{ 16, { 'M','e','t','h','o','d','D','i','c','t','i','o','n','a','r','y'  } },
		{ 14, { 'C','o','m','p','i','l','e','d','M','e','t','h','o','d'          } },
		{ 11, { 'A','s','s','o','c','i','a','t','i','o','n'                      } },
		{  4, { 'T','r','u','e'                                                  } },
		{  5, { 'F','a','l','s','e'                                              } },
		{  9, { 'C','h','a','r','a','c','t','e','r'                              } },
		{ 12, { 'S','m','a','l','l','I','n','t','e','g','e','r'                  } }
	};

	stix_oow_t i;
	stix_oop_t sym;
	stix_oop_t* stix_ptr;

	stix_ptr = &stix->_stix;
	/* The loop here repies on the proper order of fields in stix_t.
	 * Be sure to keep in sync the order of items in symnames and 
	 * the releated fields of stix_t */
	for (i = 0; i < STIX_COUNTOF(symnames); i++)
	{
		sym = stix_makesymbol (stix, symnames[i].str, symnames[i].len);
		if (!sym) return -1;

		if (!stix_putatsysdic (stix, sym, *stix_ptr)) return -1;
		stix_ptr++;
	}

	return 0;
}

int stix_ignite (stix_t* stix)
{
	STIX_ASSERT (stix->_nil == STIX_NULL);

	stix->_nil = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t));
	if (!stix->_nil) return -1;

	stix->_nil->_flags = STIX_OBJ_MAKE_FLAGS (STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 1, 0);
	stix->_nil->_size = 0;

	if (ignite_1(stix) <= -1 || ignite_2(stix) <= -1 || ignite_3(stix)) return -1;

	return 0;
}
