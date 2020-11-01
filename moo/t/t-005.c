#include <moo-xma.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "t.h"

static void* sys_alloc (moo_mmgr_t* mmgr, moo_oow_t size)
{
	return malloc(size);
}

static void* sys_realloc (moo_mmgr_t* mmgr, void* ptr, moo_oow_t size)
{
	return realloc(ptr, size);
}

static void sys_free (moo_mmgr_t* mmgr, void* ptr)
{
	free (ptr);
}

static moo_mmgr_t sys_mmgr =
{
	sys_alloc,
	sys_realloc,
	sys_free,
	MOO_NULL
};

void dumper (void* ctx, const char* fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);
}

int main ()
{
	moo_xma_t* xma;
	void* ptr1, * ptr2, * ptr3;


	xma = moo_xma_open(&sys_mmgr, 0, 100000);


	ptr1 = moo_xma_alloc(xma, 1000);
	ptr2 = moo_xma_alloc(xma, 50);
	ptr3 = moo_xma_alloc(xma, 1);

	moo_xma_free (xma, ptr2);
	moo_xma_free (xma, ptr1);

	moo_xma_dump (xma, dumper, MOO_NULL);
	moo_xma_close (xma);

	return 0;
}

