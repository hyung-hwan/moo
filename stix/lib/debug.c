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
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
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

void dump_symbol_table (stix_t* stix)
{
	stix_oow_t i, j;
	stix_oop_char_t symbol;

	printf ("--------------------------------------------\n");
	printf ("Stix Symbol Table %lu\n", (unsigned long int)STIX_OBJ_GET_SIZE(stix->symtab->bucket));
	printf ("--------------------------------------------\n");

	for (i = 0; i < STIX_OBJ_GET_SIZE(stix->symtab->bucket); i++)
	{
		symbol = (stix_oop_char_t)stix->symtab->bucket->slot[i];
		if ((stix_oop_t)symbol != stix->_nil)
		{
			printf (" %lu [", (unsigned long int)i);
			for (j = 0; j < STIX_OBJ_GET_SIZE(symbol); j++)
			{
				printf ("%c", symbol->slot[j]);
			}
			printf ("]\n");
		}
	}
	printf ("--------------------------------------------\n");
}

void dump_dictionary (stix_t* stix, stix_oop_set_t dic, const char* title)
{
	stix_oow_t i, j;
	stix_oop_association_t ass;

	printf ("--------------------------------------------\n");
	printf ("%s %lu\n", title, (unsigned long int)STIX_OBJ_GET_SIZE(dic->bucket));
	printf ("--------------------------------------------\n");

	for (i = 0; i < STIX_OBJ_GET_SIZE(dic->bucket); i++)
	{
		ass = (stix_oop_association_t)dic->bucket->slot[i];
		if ((stix_oop_t)ass != stix->_nil)
		{
			printf (" %lu [", (unsigned long int)i);
			for (j = 0; j < STIX_OBJ_GET_SIZE(ass->key); j++)
			{
				printf ("%c", ((stix_oop_char_t)ass->key)->slot[j]);
			}
			printf ("]\n");
		}
	}
	printf ("--------------------------------------------\n");
}

void print_ucs (const stix_ucs_t* name)
{
	stix_size_t i;
	for (i = 0; i < name->len; i++) printf ("%c", name->ptr[i]);
}


void __dump_object (stix_t* stix, stix_oop_t oop, int depth)
{
	stix_oop_class_t c;
	stix_ucs_t s;
	int i;

	for (i = 0; i < depth; i++) printf ("\t");
	printf ("%p instance of ", oop);

	c = (stix_oop_class_t)STIX_CLASSOF(stix, oop);
	s.ptr = ((stix_oop_char_t)c->name)->slot;
	s.len = STIX_OBJ_GET_SIZE(c->name);
	print_ucs (&s);
	printf ("\n");

	if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP)
	{
		for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
		{
			__dump_object (stix, ((stix_oop_oop_t)oop)->slot[i], depth + 1);
		}
	}
}

void dump_object (stix_t* stix, stix_oop_t oop, const char* title)
{
	printf ("[%s]\n", title);
	__dump_object (stix, oop, 0);
}
