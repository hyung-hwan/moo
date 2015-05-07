#include "stix-prv.h"

#include <stdio.h>
#include <stdlib.h>

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
	printf ("Stix 1.0.0 - max named %lu max indexed %lu\n", 
		(unsigned long int)STIX_MAX_NAMED_INSTVARS, (unsigned long int)STIX_MAX_INDEXED_INSTVARS(STIX_MAX_NAMED_INSTVARS));


	{
	stix_oow_t x;

	printf ("%u\n", STIX_BITS_MAX(unsigned int, 5));

	x = STIX_CLASS_SPEC_MAKE (10, 1, STIX_OBJ_TYPE_CHAR);
	printf ("%lu %lu %lu %lu\n", (unsigned long int)x, (unsigned long int)STIX_OOP_FROM_SMINT(x),
		(unsigned long int)STIX_CLASS_SPEC_NAMED_INSTVAR(x),
		(unsigned long int)STIX_CLASS_SPEC_INDEXED_TYPE(x));
	}

	stix = stix_open (&sys_mmgr, 0, 1000000lu, STIX_NULL);
	if (!stix)
	{
		printf ("cannot open stix\n");
		return -1;
	}

	{
		stix_oow_t symtab_size = 5000;
		stix_setoption (stix, STIX_DEFAULT_SYMTAB_SIZE, &symtab_size);
		stix_setoption (stix, STIX_DEFAULT_SYSDIC_SIZE, &symtab_size);
	}

	if (stix_ignite(stix) <= -1)
	{
		printf ("cannot ignite stix\n");
		stix_close (stix);
		return -1;
	}

{
stix_char_t x[] = { 'S', 't', 'r', 'i', 'n', 'g', '\0' };
stix_char_t y[] = { 'S', 'y', 'm', 'b', 'o', 'l', '\0' };
stix_oop_t a, b;

a = stix_makesymbol (stix, x, 6);
b = stix_makesymbol (stix, y, 6);

printf ("%p %p\n", a, b);
}

	dump_symbol_table (stix);
stix_gc (stix);
	dump_symbol_table (stix);

	stix_close (stix);

#if defined(__BORLANDC__)
	printf ("Press the enter key...\n");
	getchar ();
#endif

	return 0;
}
