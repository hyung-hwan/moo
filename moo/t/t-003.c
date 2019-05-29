#include <moo-utl.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include "t.h"

static int put_bcs (moo_fmtout_t* fmtout, const moo_bch_t* c, moo_oow_t len)
{
	while (len > 0)
	{
		putchar (*c);
		c++;
		len--;
	}
	return 1;
}

static int put_ucs (moo_fmtout_t* fmtout, const moo_uch_t* c, moo_oow_t len)
{
	moo_cmgr_t* cmgr = moo_get_utf8_cmgr();
	moo_bch_t bcs[MOO_BCSIZE_MAX];
	moo_oow_t bcslen, i;
	
	while (len > 0)
	{
		bcslen = cmgr->uctobc(*c, bcs, MOO_COUNTOF(bcs));
		/*putwchar (*c);*/
		for (i = 0; i < bcslen; i++) putchar(bcs[i]);
		c++;
		len--;
	}
	return 1;
}

static moo_ooi_t bfmt_out (const moo_bch_t* fmt, ...)
{
	moo_fmtout_t fmtout;
	va_list ap;
	int n;

	memset (&fmtout, 0, MOO_SIZEOF(fmtout));
	fmtout.putbcs = put_bcs;
	fmtout.putucs = put_ucs;

	va_start (ap, fmt);
	n = moo_bfmt_outv (&fmtout, fmt, ap);
	va_end (ap);

	return (n <= -1)? -1: fmtout.count;
}

int main ()
{
	moo_uch_t x[] = { 'A', L'\uBB33', L'\uBB34', L'\uBB35', 'Q', '\0' };
	moo_ooi_t cnt;

	setlocale (LC_ALL, NULL);
	cnt = bfmt_out ("[%s %d %020X %ls %s %w %.*lk]\n", "test", 10, 0x1232, x, "code", x, MOO_SIZEOF_UCH_T * 3, x);

	/* this unit is the number of characters written. but some are written as bch and some other as uch. 
	 * if uch and bch data are mixed, the count returned doesn't really tell how many bytes or characters written */
	bfmt_out ("wrote [%ld] units\n", cnt); 
#if (MOO_SIZEOF_UCH_T == 2)
#	define EXPECTED_LEN 98
#elif (MOO_SIZEOF_UCH_T == 4)
#	define EXPECTED_LEN 122
#else
#	error UNSUPPORTED UCH SIZE
#endif
	T_ASSERT1 (cnt == EXPECTED_LEN, "bfmt_out test #1");

	return 0;

oops:
	return -1;
}
