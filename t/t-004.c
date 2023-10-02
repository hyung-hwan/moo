#include <moo-std.h>
#include <moo-utl.h>
#include <stdio.h>
#include "t.h"

int main ()
{
	moo_t* moo = MOO_NULL;
	moo_uch_t ufmt1[] = { '%', '0', '5', 'd', ' ', '%', '-', '9', 'h', 's', '\0' };
	moo_uch_t ufmt2[] = { '%', '0', '5', 'd', ' ', '%', '-', '9', 'h', 's', ' ', '%','O','\0' };

	moo = moo_openstd(0, MOO_NULL, MOO_NULL);
	if (!moo)
	{
		fprintf (stderr, "Unable to open moo\n");
		return -1;
	}

	moo_seterrbfmt (moo, MOO_EINVAL, "%d %ld %s %hs", 10, 20L, "moo", "moo");
	T_ASSERT1 (moo_comp_oocstr_bcstr(moo_geterrmsg(moo), "10 20 moo moo") == 0, "moo seterrbfmt #1");
	moo_logbfmt (moo, MOO_LOG_STDERR, "[%js]\n", moo_geterrmsg(moo));

	moo_seterrufmt (moo, MOO_EINVAL, ufmt1, 9923, "moo");
	T_ASSERT1 (moo_comp_oocstr_bcstr(moo_geterrmsg(moo), "09923 moo      ") == 0, "moo seterrufmt #1");
	moo_logbfmt (moo, MOO_LOG_STDERR, "[%js]\n", moo_geterrmsg(moo));

	moo_seterrufmt (moo, MOO_EINVAL, ufmt2, 9923, "moo", MOO_SMPTR_TO_OOP(0x12345678));
	T_ASSERT1 (moo_comp_oocstr_bcstr(moo_geterrmsg(moo), "09923 moo       #\\p12345678") == 0, "moo seterrufmt #1");
	moo_logbfmt (moo, MOO_LOG_STDERR, "[%js]\n", moo_geterrmsg(moo));

	moo_close (moo);
	return 0;

oops:
	if (moo) moo_close (moo);
	return -1;
}
