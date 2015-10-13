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
#include <dlfcn.h>

#include <limits.h>
#include <dlfcn.h>

#if defined(_WIN32)
#	define DEFAULT_MODPREFIX "stix-"
#elif defined(__OS2__)
#	define DEFAULT_MODPREFIX "st-"
#elif defined(__DOS__)
#	define DEFAULT_MODPREFIX "st-"
#else
#	define DEFAULT_MODPREFIX "libstix-"
#endif


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


static STIX_INLINE stix_ssize_t open_input (stix_t* stix, stix_io_arg_t* arg)
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

/* TODO: make bcs relative to the includer */

#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
		arg->handle = fopen (bcs, "rb");
#else
		arg->handle = fopen (bcs, "r");
#endif
	}
	else
	{
		/* main stream */
		xtn_t* xtn = stix_getxtn(stix);
#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
		arg->handle = fopen (xtn->source_path, "rb");
#else
		arg->handle = fopen (xtn->source_path, "r");
#endif
	}

	if (!arg->handle)
	{
		stix_seterrnum (stix, STIX_EIOERR);
		return -1;
	}

	return 0;
}

static STIX_INLINE stix_ssize_t read_input (stix_t* stix, stix_io_arg_t* arg)
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

static STIX_INLINE stix_ssize_t close_input (stix_t* stix, stix_io_arg_t* arg)
{
	STIX_ASSERT (arg->handle != STIX_NULL);
	fclose ((FILE*)arg->handle);
	return 0;
}

static stix_ssize_t input_handler (stix_t* stix, stix_io_cmd_t cmd, stix_io_arg_t* arg)
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

static void* mod_open (stix_t* stix, const stix_uch_t* name)
{
/* TODO: support various platforms */
	stix_bch_t buf[1024]; /* TODO: use a proper path buffer */
	stix_size_t ucslen, bcslen;
	stix_size_t len;

	len = stix_copybcstr (buf, STIX_COUNTOF(buf), DEFAULT_MODPREFIX);

/* TODO: proper error checking and overflow checking */
	ucslen = ~(stix_size_t)0;
	bcslen = STIX_COUNTOF(buf) - len;
	stix_ucstoutf8 (name, &ucslen, &buf[len], &bcslen);

printf ("MOD-OPENING %s\n", buf);
	return dlopen (buf, RTLD_NOW);
}

static void mod_close (stix_t* stix, void* handle)
{
	dlclose (handle);
}

static void* mod_getsym (stix_t* stix, void* handle, const stix_uch_t* name)
{
	stix_bch_t buf[1024]; /* TODO: use a proper buffer. dynamically allocated if conversion result in too a large value */
	stix_size_t ucslen, bcslen;
	void* sym;

	buf[0] = '_';

	ucslen = ~(stix_size_t)0;
	bcslen = STIX_COUNTOF(buf) - 2;
	stix_ucstoutf8 (name, &ucslen, &buf[1], &bcslen);
printf ("MOD_GETSYM [%s]\n", &buf[1]);
	sym = dlsym (handle, &buf[1]);
	if (!sym)
	{
printf ("MOD_GETSYM [%s]\n", &buf[0]);
		sym = dlsym (handle, &buf[0]);
		if (!sym)
		{
			buf[bcslen + 1] = '_';
			buf[bcslen + 2] = '\0';
printf ("MOD_GETSYM [%s]\n", &buf[1]);
			sym = dlsym (handle, &buf[1]);
			if (!sym)
			{
printf ("MOD_GETSYM [%s]\n", &buf[0]);
				sym = dlsym (handle, &buf[0]);
			}
		}
	}

	return sym;
}

static char* syntax_error_msg[] = 
{
	"no error",
	"illegal character",
	"comment not closed",
	"string not closed",
	"no character after $",
	"no valid character after #",
	"wrong character literal",
	"colon expected",
	"string expected",
	"invalid radix", 
	"invalid numeric literal",
	"byte too small or too large",
	"{ expected",
	"} expected",
	"( expected",
	") expected",
	"] expected",
	". expected",
	"| expected",
	"> expected",
	":= expected",
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
	"duplicate block argument name",
	"cannot assign to argument",
	"undeclared variable",
	"unusable variable in compiled code",
	"inaccessible variable",
	"ambiguous variable",
	"wrong expression primary",
	"too many temporaries",
	"too many arguments",
	"too many block temporaries",
	"too many block arguments",
	"too large block",
	"wrong primitive number",
	"#include error",
	"wrong namespace name",
	"wrong pool dictionary name",
	"duplicate pool dictionary name",
	"literal expected"
};

stix_uch_t str_stix[] = { 'S', 't', 'i', 'x' };
stix_uch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' };
stix_uch_t str_main[] = { 'm', 'a', 'i', 'n' };

int main (int argc, char* argv[])
{
	stix_t* stix;
	xtn_t* xtn;
	stix_ucs_t objname;
	stix_ucs_t mthname;
	stix_vmprim_t vmprim;
	int i;

	printf ("Stix 1.0.0 - max named %lu max indexed %lu max class %lu max classinst %lu\n", 
		(unsigned long int)STIX_MAX_NAMED_INSTVARS, 
		(unsigned long int)STIX_MAX_INDEXED_INSTVARS(STIX_MAX_NAMED_INSTVARS),
		(unsigned long int)STIX_MAX_CLASSVARS,
		(unsigned long int)STIX_MAX_CLASSINSTVARS);


#if !defined(macintosh)
	if (argc < 2)
	{
		fprintf (stderr, "Usage: %s filename ...\n", argv[0]);
		return -1;
	}
#endif

	/*
	{
	stix_oop_t k;
	stix_oow_t x;

	k = STIX_OOP_FROM_SMINT(-1);
	printf ("%ld %ld %ld %lX\n", (long int)STIX_OOP_TO_SMINT(k), (long int)STIX_SMINT_MIN, (long int)STIX_SMINT_MAX, (long)LONG_MIN);

	k = STIX_OOP_FROM_SMINT(STIX_SMINT_MAX);
	printf ("%ld\n", (long int)STIX_OOP_TO_SMINT(k));

	k = STIX_OOP_FROM_SMINT(STIX_SMINT_MIN);
	printf ("%ld\n", (long int)STIX_OOP_TO_SMINT(k));

	printf ("%u\n", STIX_BITS_MAX(unsigned int, 5));
	x = STIX_CLASS_SPEC_MAKE (10, 1, STIX_OBJ_TYPE_CHAR);
	printf ("%lu %lu %lu %lu\n", (unsigned long int)x, (unsigned long int)STIX_OOP_FROM_SMINT(x),
		(unsigned long int)STIX_CLASS_SPEC_NAMED_INSTVAR(x),
		(unsigned long int)STIX_CLASS_SPEC_INDEXED_TYPE(x));
	}*/

	vmprim.mod_open = mod_open;
	vmprim.mod_close = mod_close;
	vmprim.mod_getsym = mod_getsym;

	stix = stix_open (&sys_mmgr, STIX_SIZEOF(xtn_t), 512000lu, &vmprim, STIX_NULL);
	if (!stix)
	{
		printf ("cannot open stix\n");
		return -1;
	}

	{
		stix_oow_t tab_size;

		tab_size = 5000;
		stix_setoption (stix, STIX_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		stix_setoption (stix, STIX_SYSDIC_SIZE, &tab_size);
	}

	{
		int trait = 0;

		/*trait |= STIX_NOGC;*/
		stix_setoption (stix, STIX_TRAIT, &trait);
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

#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.st";
	goto compile;
#endif

	for (i = 1; i < argc; i++)
	{
		xtn->source_path = argv[i];

	compile:

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
				else
				{
					printf ("%s ", xtn->source_path);
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
	}

/*	objname.ptr = str_stix;
	objname.len = 4;*/
	objname.ptr = str_my_object;
	objname.len = 8;
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
