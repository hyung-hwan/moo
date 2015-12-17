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

void print_oocs (const stix_oocs_t* name)
{
	stix_oow_t i;
	for (i = 0; i < name->len; i++) printf ("%c", name->ptr[i]);
}


void print_object (stix_t* stix, stix_oop_t oop)
{
	if (oop == stix->_nil)
	{
		printf ("nil");
	}
	else if (oop == stix->_true)
	{
		printf ("true");
	}
	else if (oop == stix->_false)
	{
		printf ("false");
	}
	else if (STIX_OOP_IS_SMOOI(oop))
	{
		printf ("%ld", (long int)STIX_OOP_TO_SMOOI(oop));
	}
	else if (STIX_OOP_IS_CHAR(oop))
	{
		stix_bch_t bcs[32];
		stix_uch_t uch;
		stix_oow_t ucslen, bcslen;

		uch = STIX_OOP_TO_CHAR(oop);
		bcslen = STIX_COUNTOF(bcs);
		ucslen = 1;
		if (stix_ucstoutf8 (&uch, &ucslen, bcs, &bcslen) >= 0)
		{
			printf ("$%.*s", (int)bcslen, bcs);
		}
	}
	else
	{
		stix_oop_class_t c;
		stix_oocs_t s;
		stix_oow_t i;
		stix_bch_t bcs[32];
		stix_oow_t ucslen, bcslen;

		STIX_ASSERT (STIX_OOP_IS_POINTER(oop));
		c = (stix_oop_class_t)STIX_OBJ_GET_CLASS(oop); /*STIX_CLASSOF(stix, oop);*/

		if ((stix_oop_t)c == stix->_large_negative_integer)
		{
			stix_oow_t i;
			printf ("-16r");
			for (i = STIX_OBJ_GET_SIZE(oop); i > 0;)
			{
				printf ("%0*lX", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if ((stix_oop_t)c == stix->_large_positive_integer)
		{
			stix_oow_t i;
			printf ("16r");
			for (i = STIX_OBJ_GET_SIZE(oop); i > 0;)
			{
				printf ("%0*lX", (int)(STIX_SIZEOF(stix_liw_t) * 2), (unsigned long)((stix_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_CHAR)
		{
			if ((stix_oop_t)c == stix->_symbol) printf ("#");
			else if ((stix_oop_t)c == stix->_string) printf ("'");

			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				bcslen = STIX_COUNTOF(bcs);
				ucslen = 1;
				if (stix_ucstoutf8 (&((stix_oop_char_t)oop)->slot[i], &ucslen, bcs, &bcslen) >= 0)
				{
					printf ("%.*s", (int)bcslen, bcs);
				}
			}
			if ((stix_oop_t)c == stix->_string) printf ("'");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_BYTE)
		{
			printf ("#[");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %d", ((stix_oop_byte_t)oop)->slot[i]);
			}
			printf ("]");
		}
		
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_HALFWORD)
		{
			printf ("#[["); /* TODO: fix this symbol */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %lX", (unsigned long int)((stix_oop_halfword_t)oop)->slot[i]);
			}
			printf ("]]");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_WORD)
		{
			printf ("#[[["); /* TODO: fix this symbol */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %lX", (unsigned long int)((stix_oop_word_t)oop)->slot[i]);
			}
			printf ("]]]");
		}
		else if ((stix_oop_t)c == stix->_array)
		{
			printf ("#(");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" ");
				print_object (stix, ((stix_oop_oop_t)oop)->slot[i]);
			}
			printf (")");
		}
		else if ((stix_oop_t)c == stix->_class)
		{
			/* print the class name */
			for (i = 0; i < STIX_OBJ_GET_SIZE(((stix_oop_class_t)oop)->name); i++)
			{
				bcslen = STIX_COUNTOF(bcs);
				ucslen = 1;
				if (stix_ucstoutf8 (&((stix_oop_class_t)oop)->name->slot[i], &ucslen, bcs, &bcslen) >= 0)
				{
					printf ("%.*s", (int)bcslen, bcs);
				}
			}
		}
		else
		{
			s.ptr = ((stix_oop_char_t)c->name)->slot;
			s.len = STIX_OBJ_GET_SIZE(c->name);
			printf ("instance of ");
			print_oocs (&s);
			printf ("- (%p)", oop);
		}
	}
}

static void __dump_object (stix_t* stix, stix_oop_t oop, int depth)
{
	stix_oop_class_t c;
	stix_oocs_t s;
	int i;

	for (i = 0; i < depth; i++) printf ("\t");
	printf ("%p instance of ", oop);

	c = (stix_oop_class_t)STIX_CLASSOF(stix, oop);
	s.ptr = ((stix_oop_char_t)c->name)->slot;
	s.len = STIX_OBJ_GET_SIZE(c->name);
	print_oocs (&s);

	if (oop == stix->_nil)
	{
		printf (" nil");
	}
	else if (oop == stix->_true)
	{
		printf (" true");
	}
	else if (oop == stix->_false)
	{
		printf (" false");
	}
	else if (STIX_OOP_IS_SMOOI(oop))
	{
		printf (" %ld", (long int)STIX_OOP_TO_SMOOI(oop));
	}
	else if (STIX_OOP_IS_CHAR(oop))
	{
		stix_bch_t bcs[32];
		stix_uch_t uch;
		stix_oow_t ucslen, bcslen;

		uch = STIX_OOP_TO_CHAR(oop);
		bcslen = STIX_COUNTOF(bcs);
		ucslen = 1;
		if (stix_ucstoutf8 (&uch, &ucslen, bcs, &bcslen) >= 0)
		{
			printf (" $%.*s", (int)bcslen, bcs);
		}
	}
	else if (STIX_OOP_IS_POINTER(oop))
	{
		if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP)
		{
/* TODO: print _Array specially using #( */
			printf ("\n");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				__dump_object (stix, ((stix_oop_oop_t)oop)->slot[i], depth + 1);
			}
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_CHAR)
		{
			if (STIX_CLASSOF(stix,oop) == stix->_symbol)
			{
				printf (" #'");
			}
			else if (STIX_CLASSOF(stix,oop) == stix->_string)
			{
				printf (" '");
			}

			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				stix_bch_t bcs[32];
				stix_uch_t uch;
				stix_oow_t ucslen, bcslen;
				uch = ((stix_oop_char_t)oop)->slot[i];
				if (uch == '\'') printf ("''");
				else
				{
					bcslen = STIX_COUNTOF(bcs);
					ucslen = 1;
					if (stix_ucstoutf8 (&uch, &ucslen, bcs, &bcslen) >= 0)
					{
						printf ("%.*s", (int)bcslen, bcs);
					}
				}
			}

			printf ("'");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_BYTE)
		{
			printf (" #[");
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %d", ((stix_oop_byte_t)oop)->slot[i]);
			}
			printf ("]");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_HALFWORD)
		{
			printf (" #["); /* TODO: different symbol for word array ?? */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %ld", (long int)((stix_oop_halfword_t)oop)->slot[i]);
			}
			printf ("]");
		}
		else if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_WORD)
		{
			printf (" #["); /* TODO: different symbol for word array ?? */
			for (i = 0; i < STIX_OBJ_GET_SIZE(oop); i++)
			{
				printf (" %ld", (long int)((stix_oop_word_t)oop)->slot[i]);
			}
			printf ("]");
		}
	}

	printf ("\n");
}

void dump_object (stix_t* stix, stix_oop_t oop, const char* title)
{
	printf ("[%s]\n", title);
	__dump_object (stix, oop, 0);
}
