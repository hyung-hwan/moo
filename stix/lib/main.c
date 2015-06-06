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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
typedef struct xtn_t xtn_t;
struct xtn_t
{
	const char* source_path;

	char bchar_buf[1024];
	stix_size_t bchar_pos;
	stix_size_t bchar_len;
};

static void* sys_alloc (stix_mmgr_t* mmgr, stix_size_t size)
{
	return malloc (size);
}

static void* sys_realloc (stix_mmgr_t* mmgr, void* ptr, stix_size_t size)
{
	return realloc (ptr, size);
}

static void sys_free (stix_mmgr_t* mmgr, void* ptr)
{
	free (ptr);
}

static stix_mmgr_t sys_mmgr =
{
	sys_alloc,
	sys_realloc,
	sys_free,
	STIX_NULL
};


static STIX_INLINE stix_ssize_t open_input (stix_t* stix, stix_ioarg_t* arg)
{
	if (arg->includer)
	{
		/* includee */
		stix_bch_t bcs[1024]; /* TODO: right buffer size */
		stix_size_t bcslen = STIX_COUNTOF(bcs);
		stix_size_t ucslen = ~(stix_size_t)0;

		if (stix_ucstoutf8 (arg->name, &ucslen, bcs, &bcslen) <= -1)
		{
			stix_seterrnum (stix, STIX_EECERR);
			return -1;
		}

		arg->handle = fopen (bcs, "r");
	}
	else
	{
		/* main stream */
		xtn_t* xtn = stix_getxtn(stix);
		arg->handle = fopen (xtn->source_path, "r");
	}

	if (!arg->handle)
	{
		stix_seterrnum (stix, STIX_EIOERR);
		return -1;
	}

	return 0;
}

static STIX_INLINE stix_ssize_t read_input (stix_t* stix, stix_ioarg_t* arg)
{
	xtn_t* xtn = stix_getxtn(stix);
	stix_size_t n, bcslen, ucslen, remlen;
	int x;

	STIX_ASSERT (arg->handle != STIX_NULL);
	n = fread (&xtn->bchar_buf[xtn->bchar_len], STIX_SIZEOF(xtn->bchar_buf[0]), STIX_COUNTOF(xtn->bchar_buf) - xtn->bchar_len, arg->handle);
	if (n == 0)
	{
		if (ferror((FILE*)arg->handle))
		{
			stix_seterrnum (stix, STIX_EIOERR);
			return -1;
		}
	}

	xtn->bchar_len += n;
	bcslen = xtn->bchar_len;
	ucslen = STIX_COUNTOF(arg->buf);
	x = stix_utf8toucs (xtn->bchar_buf, &bcslen, arg->buf, &ucslen);
	if (x <= -1 && ucslen <= 0)
	{
		stix_seterrnum (stix, STIX_EECERR);
		return -1;
	}

	remlen = xtn->bchar_len - bcslen;
	if (remlen > 0) memmove (xtn->bchar_buf, &xtn->bchar_buf[bcslen], remlen);
	xtn->bchar_len = remlen;
	return ucslen;
}

static STIX_INLINE stix_ssize_t close_input (stix_t* stix, stix_ioarg_t* arg)
{
	STIX_ASSERT (arg->handle != STIX_NULL);
	fclose ((FILE*)arg->handle);
	return 0;
}

static stix_ssize_t input_handler (stix_t* stix, stix_iocmd_t cmd, stix_ioarg_t* arg)
{
	switch (cmd)
	{
		case STIX_IO_OPEN:
			return open_input (stix, arg);
			
		case STIX_IO_CLOSE:
			return close_input (stix, arg);

		case STIX_IO_READ:
			return read_input (stix, arg);

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}
}

static void dump_symbol_table (stix_t* stix)
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
static char* syntax_error_msg[] = 
{
	"no error",
	"illegal character",
	"comment not closed",
	"string not closed",
	"no character after $",
	"no valid character after #",
	"missing colon",
	"string expected", /* string expected in place of ${1} */
	"{ expected",
	"} expected",
	"( expected",
	") expected",
	"] expected",
	". expected",
	"| expected",
	"> expected",
	"identifier expected",
	"integer expected",
	"primitive: expected",
	"wrong directive",
	"undefined class",
	"duplicate class",
	"contradictory class definition",
	"#dcl not allowed",
	"wrong method name",
	"duplicate method name",
	"duplicate argument name",
	"duplicate temporary variable name",
	"duplicate variable name",
	"cannot assign to argument",
	"undeclared variable",
	"unusable variable in compiled code",
	"inaccessible variable",
	"wrong expression primary",
	"too many arguments",
	"wrong primitive number"
};

stix_uch_t str_stix[] = { 'S', 't', 'i', 'x' };
stix_uch_t str_main[] = { 'm', 'a', 'i', 'n' };

int main (int argc, char* argv[])
{
	stix_t* stix;
	xtn_t* xtn;
	stix_ucs_t objname;
	stix_ucs_t mthname;

	printf ("Stix 1.0.0 - max named %lu max indexed %lu max class %lu max classinst %lu\n", 
		(unsigned long int)STIX_MAX_NAMED_INSTVARS, 
		(unsigned long int)STIX_MAX_INDEXED_INSTVARS(STIX_MAX_NAMED_INSTVARS),
		(unsigned long int)STIX_MAX_CLASSVARS,
		(unsigned long int)STIX_MAX_CLASSINSTVARS);

{
stix_oop_t k;
k = STIX_OOP_FROM_SMINT(-1);
printf ("%ld %ld %ld %lX\n", (long int)STIX_OOP_TO_SMINT(k), (long int)STIX_SMINT_MIN, (long int)STIX_SMINT_MAX, (long)LONG_MIN);

k = STIX_OOP_FROM_SMINT(STIX_SMINT_MAX);
printf ("%ld\n", (long int)STIX_OOP_TO_SMINT(k));

k = STIX_OOP_FROM_SMINT(STIX_SMINT_MIN);
printf ("%ld\n", (long int)STIX_OOP_TO_SMINT(k));
}

#if !defined(macintosh)
	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s filename\n", argv[0]);
		return -1;
	}
#endif

	{
	stix_oow_t x;

	printf ("%u\n", STIX_BITS_MAX(unsigned int, 5));

	x = STIX_CLASS_SPEC_MAKE (10, 1, STIX_OBJ_TYPE_CHAR);
	printf ("%lu %lu %lu %lu\n", (unsigned long int)x, (unsigned long int)STIX_OOP_FROM_SMINT(x),
		(unsigned long int)STIX_CLASS_SPEC_NAMED_INSTVAR(x),
		(unsigned long int)STIX_CLASS_SPEC_INDEXED_TYPE(x));
	}

	stix = stix_open (&sys_mmgr, STIX_SIZEOF(xtn_t), 512000lu, STIX_NULL);
	if (!stix)
	{
		printf ("cannot open stix\n");
		return -1;
	}

	{
		stix_oow_t tab_size;

		tab_size = 5000;
		stix_setoption (stix, STIX_DFL_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		stix_setoption (stix, STIX_DFL_SYSDIC_SIZE, &tab_size);
	}

	if (stix_ignite(stix) <= -1)
	{
		printf ("cannot ignite stix - %d\n", stix_geterrnum(stix));
		stix_close (stix);
		return -1;
	}

{
stix_uch_t x[] = { 'X', 't', 'r', 'i', 'n', 'g', '\0' };
stix_uch_t y[] = { 'S', 'y', 'm', 'b', 'o', 'l', '\0' };
stix_oop_t a, b, k;

a = stix_makesymbol (stix, x, 6);
b = stix_makesymbol (stix, y, 6);

printf ("%p %p\n", a, b);


	dump_symbol_table (stix);

/*
stix_pushtmp (stix, &a);
stix_pushtmp (stix, &b);
k = stix_instantiate (stix, stix->_byte_array, STIX_NULL, 100);
stix_poptmps (stix, 2);
stix_putatsysdic (stix, a, k);
*/

stix_gc (stix);
a = stix_findsymbol (stix, x, 6);
printf ("%p\n", a);
	dump_symbol_table (stix);


	dump_dictionary (stix, stix->sysdic, "System dictionary");
}

	xtn = stix_getxtn (stix);
#if !defined(macintosh)
	xtn->source_path = argv[1];
#else
	xtn->source_path = "test.st";
#endif
	if (stix_compile (stix, input_handler) <= -1)
	{
		if (stix->errnum == STIX_ESYNTAX)
		{
			stix_synerr_t synerr;
			stix_bch_t bcs[1024]; /* TODO: right buffer size */
			stix_size_t bcslen, ucslen;

			stix_getsynerr (stix, &synerr);

			printf ("ERROR: ");
			if (synerr.loc.file)
			{
				bcslen = STIX_COUNTOF(bcs);
				ucslen = ~(stix_size_t)0;
				if (stix_ucstoutf8 (synerr.loc.file, &ucslen, bcs, &bcslen) >= 0)
				{
					printf ("%.*s ", (int)bcslen, bcs);
				}
			}


			printf ("syntax error at line %lu column %lu - %s", 
				(unsigned long int)synerr.loc.line, (unsigned long int)synerr.loc.colm,
				syntax_error_msg[synerr.num]);
			if (synerr.tgt.len > 0)
			{
				bcslen = STIX_COUNTOF(bcs);
				ucslen = synerr.tgt.len;

				if (stix_ucstoutf8 (synerr.tgt.ptr, &ucslen, bcs, &bcslen) >= 0)
				{
					printf (" [%.*s]", (int)bcslen, bcs);
				}

			}
			printf ("\n");
		}
		else
		{
			printf ("ERROR: cannot compile code - %d\n", stix_geterrnum(stix));
		}
		stix_close (stix);
		return -1;
	}


	objname.ptr = str_stix;
	objname.len = 4;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (stix_invoke (stix, &objname, &mthname) <= -1)
	{
		printf ("ERROR: cannot execute code - %d\n", stix_geterrnum(stix));
		stix_close (stix);
		return -1;
	}


	dump_dictionary (stix, stix->sysdic, "System dictionary");
	stix_close (stix);


	return 0;
}
