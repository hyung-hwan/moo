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

void print_oocs (const stix_oocs_t* name)
{
	stix_oow_t i;
	for (i = 0; i < name->len; i++) printf ("%c", name->ptr[i]);
}


void print_object (stix_t* stix, unsigned int mask, stix_oop_t oop)
{
	if (oop == stix->_nil)
	{
		stix_logbfmt (stix, mask, "nil");
	}
	else if (oop == stix->_true)
	{
		stix_logbfmt (stix, mask, "true");
	}
	else if (oop == stix->_false)
	{
		stix_logbfmt (stix, mask, "false");
	}
	else if (STIX_OOP_IS_SMOOI(oop))
	{
		stix_logbfmt (stix, mask, "%zd", STIX_OOP_TO_SMOOI(oop));
	}
	else if (STIX_OOP_IS_CHAR(oop))
	{
		stix_logbfmt (stix, mask, "$%.1C", STIX_OOP_TO_CHAR(oop));
	}
	else
	{
		stix_oop_class_t c;
		stix_oow_t i;

		STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
		c = (stix_oop_class_t)STIX_OBJ_GET_CLASS(oop); /*STIX_CLASSOF(stix, oop);*/

		if ((stix_oop_t)c == stix->_large_negative_integer)
		{
			stix_oow_t i;
			stix_logbfmt (stix, mask, "-16r");
			for (i = STIX_OBJ_GET_SIZE(oop); i > 0;)
			{
				stix_logbfmt (stix, mask, "%0*lX", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if ((stix_oop_t)c == stix->_large_positive_integer)
		{
			stix_oow_t i;
			stix_logbfmt (stix, mask, "16r");
			for (i = STIX_OBJ_GET_SIZE(oop); i > 0;)
			{
				stix_logbfmt (stix, mask, "%0*lX", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_CHAR)
		{
			if ((stix_oop_t)c == stix->_symbol) stix_logbfmt (stix, mask, "#");
			else if ((stix_oop_t)c == stix->_string) stix_logbfmt (stix, mask, "'");

			stix_logbfmt (stix, mask, "%.*S", STIX_OBJ_GET_SIZE(oop), ((stix_oop_char_t)oop)->slot);
			if ((stix_oop_t)c == stix->_string) stix_logbfmt (stix,  mask, "'");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_BYTE)
		{
			stix_logbfmt (stix, mask, "#[");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				stix_logbfmt (stix, mask, " %d", ((stix_oop_byte_t)oop)->slot[i]);
			}
			stix_logbfmt (stix, mask, "]");
		}
		
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_HALFWORD)
		{
			stix_logbfmt (stix, mask, "#[["); /* TODO: fix this symbol/notation */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				stix_logbfmt (stix, mask, " %zX", (stix_oow_t)((stix_oop_halfword_t)oop)->slot[i]);
			}
			stix_logbfmt (stix, mask, "]]");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_WORD)
		{
			stix_logbfmt (stix, mask, "#[[["); /* TODO: fix this symbol/notation */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				stix_logbfmt (stix, mask, " %zX", ((stix_oop_word_t)oop)->slot[i]);
			}
			stix_logbfmt (stix, mask, "]]]");
		}
		else if ((stix_oop_t)c == stix->_array)
		{
			stix_logbfmt (stix, mask, "#(");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				stix_logbfmt (stix, mask, " ");
				print_object (stix, mask, ((stix_oop_oop_t)oop)->slot[i]);
			}
			stix_logbfmt (stix, mask, ")");
		}
		else if ((stix_oop_t)c == stix->_class)
		{
			/* print the class name */
			stix_logbfmt (stix, mask, "%.*S", STIX_OBJ_GET_SIZE(((stix_oop_class_t)oop)->name), ((stix_oop_class_t)oop)->name->slot);
		}
		else
		{
			stix_logbfmt (stix, mask, "instance of %.*S(%p)", STIX_OBJ_GET_SIZE(c->name), ((stix_oop_char_t)c->name)->slot, oop);
		}
	}
}
