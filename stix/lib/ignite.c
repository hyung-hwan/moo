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


#if 0
stix_oop_t stix_addclass (stix_t* stix, name, spec, ...)
{
/* convert name to the internal encoding... */

/* returned value must be also registered in the system dictionary. 
 * so it doesn't get GCed. */
	return -1;
}

int stix_delclass (stix_t* stix, stix_oop_t _class)
{
	stix_oop_class_t c = (stix_oop_class_t)_class;

	if (c->name)
	{
		/* delete c->name from system dictionary. it'll get GCed if it's not referenced */
	}

	return -1;
}

stix_oop_t stix_findclass (stix_t* stix, const char* name)
{
	/* find a class object by name */
	return STIX_NULL;
}

int stix_addmethod (stix_t* stix, stix_oop_t _class, const char* name, const bytecode)
{

}
#endif

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
	stix->_class = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE (STIX_CLASS_NAMED_INSTVARS, 1, STIX_OBJ_TYPE_OOP));
	if (!stix->_class) return -1;

	STIX_ASSERT (STIX_OBJ_GET_CLASS(stix->_class) == STIX_NULL);
	STIX_OBJ_SET_CLASS (stix->_class, stix->_class);

	/* --------------------------------------------------------------
	 * Stix      - proto-object with 1 class variable.
	 * NilObject - class for the nil object.
	 * Object    - top of all ordinary objects.
	 * Symbol
	 * Array
	 * SymbolSet
	 * Character
	 * SmallIntger
	 * -------------------------------------------------------------- */
	stix->_stix              = alloc_kernel_class (stix, 1, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_nil_object        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_object            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_array             = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_OOP));
	stix->_symbol            = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
	stix->_symbol_set        = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_system_dictionary = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_SET_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));
	stix->_association       = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(STIX_ASSOCIATION_NAMED_INSTVARS, 0, STIX_OBJ_TYPE_OOP));

	/* TOOD: what is a proper spec for Character and SmallInteger?
 	 *       If the fixed part is  0, its instance must be an object of 0 payload fields.
	 *       Does this make sense? */
	stix->_character         = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));
	stix->_small_integer     = alloc_kernel_class (stix, 0, STIX_CLASS_SPEC_MAKE(0, 0, STIX_OBJ_TYPE_OOP));

	if (!stix->_stix              || !stix->_nil_object    || !stix->_object ||
	    !stix->_array             || !stix->_symbol        || !stix->_symbol_set ||
	    !stix->_system_dictionary || !stix->_association   || 
	    !stix->_character         || !stix->_small_integer) return -1;

	STIX_OBJ_SET_CLASS (stix->_nil, stix->_nil_object);
	return 0;
}

static int ignite_2 (stix_t* stix)
{
	stix_oop_oop_t arr;

	/* Create the symbol table */
	stix->symtab = (stix_oop_set_t)stix_instantiate (stix, stix->_symbol_set, STIX_NULL, 0);
	if (!stix->symtab) return -1;

	stix->symtab->tally = STIX_OOP_FROM_SMINT(0);
	arr = (stix_oop_oop_t)stix_instantiate (stix, stix->_array, STIX_NULL, stix->option.default_symtab_size);
	if (!arr) return -1;
	stix->symtab->bucket = arr;

	/* Create the system dictionary */
	stix->sysdic = (stix_oop_set_t)stix_instantiate (stix, stix->_system_dictionary, STIX_NULL, 0);
	if (!stix->sysdic) return -1;

	stix->sysdic->tally = STIX_OOP_FROM_SMINT(0);
	arr = (stix_oop_oop_t)stix_instantiate (stix, stix->_array, STIX_NULL, stix->option.default_sysdic_size);
	if (!arr) return -1;
	stix->sysdic->bucket = arr;

	/* Export the system dictionary via the first class variable of the Stix class */
	((stix_oop_class_t)stix->_stix)->classvar[0] = (stix_oop_t)stix->sysdic;

	return 0;
}

#if 0

/*
	stix_oop_class_t c;

	c = (stix_oop_class_t)stix->_stix;
	c->classvar[0] = stix->_nil;
*/
	/* Set subclasses */

	/*
  	Set Names.
	stix->_stix_object->name = new_symbol ("ProtoObject");
	stix->_nil_object->name = new_symbol ("NilObject");
	stix->_class->name = new_symbol ("Class");
	*/

	/*
	_class->instvars = make_string or make_array('spec instmthds classmthds superclass name instvars');
	*/

	/*register 'Stix', 'NilObject' and 'Class' into the system dictionary.*/
	
#endif


int stix_ignite (stix_t* stix)
{
	STIX_ASSERT (stix->_nil == STIX_NULL);

	stix->_nil = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t));
	if (!stix->_nil) return -1;

	stix->_nil->_flags = STIX_OBJ_MAKE_FLAGS (STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 1, 0);
	stix->_nil->_size = 0;

	if (ignite_1(stix) <= -1 || ignite_2(stix) <= -1) return -1;

/*stix_addclass (stix, "True", spec, "Boolean");*/
	/*stix->_true = stix_instantiate (stix, stix->_true_class, STIX_NULL, 0);
	stix->_false = stix_instantiate (stix, stix->_false_class STIX_NULL, 0);

	if (!stix->_true || !stix->_false) return -1;*/


	return 0;
}
