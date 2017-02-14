/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

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


#include "_stdio.h"
#include <moo-utl.h>

#include <stdio.h>
#include <errno.h>
#include <limits.h>

#if !defined(PATH_MAX)
#	define PATH_MAX 256
#endif

typedef struct stdio_t stdio_t;
struct stdio_t
{
	FILE* fp;
};

static moo_pfrc_t pf_open (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_char_t name;
	moo_oop_char_t mode;
	stdio_t* stdio;

#if defined(MOO_OOCH_IS_UCH)
	moo_oow_t ucslen, bcslen;
	moo_bch_t namebuf[PATH_MAX];
	moo_bch_t modebuf[32]; /* TODO: mode should not be long but use dynamic-sized conversion?? */
#endif

	stdio = (stdio_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	name = (moo_oop_char_t)MOO_STACK_GETARG(moo, nargs, 0);
	mode = (moo_oop_char_t)MOO_STACK_GETARG(moo, nargs, 1);

#if defined(MOO_OOCH_IS_UCH)
	ucslen = MOO_OBJ_GET_SIZE(name);
	bcslen = MOO_COUNTOF(namebuf) - 1;
	if (moo_convootobchars (moo, name->slot, &ucslen, namebuf, &bcslen) <= -1) goto softfail;
	namebuf[bcslen] = '\0';

	ucslen = MOO_OBJ_GET_SIZE(mode);
	bcslen = MOO_COUNTOF(modebuf) - 1;
	if (moo_convootobchars (moo, mode->slot, &ucslen, modebuf, &bcslen) <= -1) goto softfail;
	modebuf[bcslen] = '\0';

	stdio->fp = fopen (namebuf, modebuf);
#else
	stdio->fp = fopen (name->slot, mode->slot);
#endif
	if (!stdio->fp) 
	{
		moo_seterrnum (moo, moo_syserrtoerrnum(errno));
		goto softfail;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
	stdio_t* stdio;

	stdio = (stdio_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	if (stdio->fp)
	{
		fclose (stdio->fp);
		stdio->fp = NULL;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gets (moo_t* moo, moo_ooi_t nargs)
{
	/* TODO: ...*/
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t __pf_puts (moo_t* moo, moo_ooi_t nargs, moo_oow_t limit)
{
	stdio_t* stdio;
	moo_ooi_t i;

	stdio = (stdio_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	for (i = 0; i < nargs; i++)
	{
		moo_oop_char_t x;
		moo_obj_char_t tmpc;

		x = (moo_oop_char_t)MOO_STACK_GETARG(moo, nargs, i);
		if (MOO_OOP_IS_CHAR(x))
		{
			/* do some faking. */
			MOO_ASSERT (moo, MOO_SIZEOF(tmpc) >= MOO_SIZEOF(moo_obj_t) + MOO_SIZEOF(moo_ooch_t));

			tmpc.slot[0] = MOO_OOP_TO_CHAR(x);
			x = (moo_oop_char_t)&tmpc;
			MOO_OBJ_SET_SIZE(x, 1);
			goto puts_string;
		}
		else if (MOO_OOP_IS_POINTER(x) && MOO_OBJ_GET_FLAGS_TYPE(x) == MOO_OBJ_TYPE_CHAR)
		{
		#if defined(MOO_OOCH_IS_UCH)
			int n;
			moo_oow_t ucspos, ucsrem, ucslen, bcslen;
			moo_bch_t bcs[1024]; /* TODO: choose a better buffer size */

		puts_string:
			ucspos = 0;
			ucsrem = MOO_OBJ_GET_SIZE(x);
			if (ucsrem > limit) ucsrem = limit;

			while (ucsrem > 0)
			{
				ucslen = ucsrem;
				bcslen = MOO_COUNTOF(bcs);

/* TODO: implement character conversion into stdio and use it instead of vm's conversion facility. */
				if ((n = moo_convootobchars (moo, &x->slot[ucspos], &ucslen, bcs, &bcslen)) <= -1)
				{
					if (n != -2 || ucslen <= 0) goto softfail;
				}

				if (fwrite (bcs, 1, bcslen, stdio->fp) < bcslen)
				{
					moo_seterrnum (moo, moo_syserrtoerrnum(errno));
					goto softfail;
				}

		/* TODO: abort looping for async processing???? */
				ucspos += ucslen;
				ucsrem -= ucslen;
			}
		#else
		puts_string:
			if (fwrite (x->slot, 1, MOO_OBJ_GET_SIZE(x), stdio->fp) < MOO_OBJ_GET_SIZE(x))
			{
				moo_seterrnum (moo, moo_syserrtoerrnum(errno));
				moo_seterrnum (moo, moo_syserrtoerrnum(errno));
				goto softfail;
			}
		#endif
		}
		else
		{
			moo_seterrnum (moo, MOO_EINVAL);
			goto softfail;
		}
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_putc (moo_t* moo, moo_ooi_t nargs)
{
	return __pf_puts (moo, nargs, 1);
}

static moo_pfrc_t pf_puts (moo_t* moo, moo_ooi_t nargs)
{
	return __pf_puts (moo, nargs, MOO_TYPE_MAX(moo_oow_t));
}

/*TODO: add print function that can accept ByteArray
 *      add format function for formatted output  */


/* ------------------------------------------------------------------------ */

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

static moo_pfinfo_t pfinfos[] =
{
	{ I, { 'c','l','o','s','e','\0' },                             0, pf_close         },
	{ I, { 'g','e','t','s','\0' },                                 0, pf_gets          },
	{ I, { 'o','p','e','n',':','f','o','r',':','\0' },             0, pf_open          },
	{ I, { 'p','u','t','c','\0' },                                 1, pf_putc          },
	{ I, { 'p','u','t','c',':','\0' },                             0, pf_putc          },
	{ I, { 'p','u','t','s','\0' },                                 1, pf_puts          },
	{ I, { 'p','u','t','s',':','\0' },                             0, pf_puts          }
};

/* ------------------------------------------------------------------------ */
static int import (moo_t* moo, moo_mod_t* mod, moo_oop_t _class)
{
	if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(stdio_t)) <= -1) return -1;
	return moo_genpfmethods (moo, mod, _class, pfinfos, MOO_COUNTOF(pfinfos));
}

static moo_pfimpl_t query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	return moo_findpfimpl (moo, pfinfos, MOO_COUNTOF(pfinfos), name);
}

#if 0
/* TOOD: concept of a argument_check fucntion?
 * check if receiver is a certain object?
 * check if receiver is at a certain size?
 * etc...
 */
static int sanity_check (moo_t* moo)
{
}
#endif

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: close all open handle?? */
}

int moo_mod_stdio (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}

