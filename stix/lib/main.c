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
		if (ferror(arg->handle))
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
	fclose (arg->handle);
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

int main (int argc, char* argv[])
{
	stix_t* stix;
	xtn_t* xtn;

	printf ("Stix 1.0.0 - max named %lu max indexed %lu\n", 
		(unsigned long int)STIX_MAX_NAMED_INSTVARS, (unsigned long int)STIX_MAX_INDEXED_INSTVARS(STIX_MAX_NAMED_INSTVARS));


	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s filename\n", argv[0]);
		return -1;
	}


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
		stix_oow_t symtab_size = 5000;
		stix_setoption (stix, STIX_DFL_SYMTAB_SIZE, &symtab_size);
		stix_setoption (stix, STIX_DFL_SYSDIC_SIZE, &symtab_size);
	}

	if (stix_ignite (stix) <= -1)
	{
		printf ("cannot ignite stix\n");
		stix_close (stix);
		return -1;
	}

{
stix_uch_t x[] = { 'S', 't', 'r', 'i', 'n', 'g', '\0' };
stix_uch_t y[] = { 'S', 'y', 'm', 'b', 'o', 'l', '\0' };
stix_oop_t a, b;

a = stix_makesymbol (stix, x, 6);
b = stix_makesymbol (stix, y, 6);

printf ("%p %p\n", a, b);


	dump_symbol_table (stix);


stix_gc (stix);
a = stix_findsymbol (stix, x, 6);
printf ("%p\n", a);
	dump_symbol_table (stix);
}

	xtn = stix_getxtn (stix);
	xtn->source_path = argv[1];
	if (stix_compile (stix, input_handler) <= -1)
	{
		printf ("cannot compile code\n");
		stix_close (stix);
		return -1;
	}

	stix_close (stix);

	return 0;
}
