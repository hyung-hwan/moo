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


#include <stix.h>
#include <stix-utl.h>

#include <stdio.h>
#include <limits.h>

/* TODO: remvoe this assert use one defined in stix.h */
#include <assert.h>
#define STIX_ASSERT(x) assert(x)

typedef struct stdio_t stdio_t;
struct stdio_t
{
	STIX_OBJ_HEADER;
	FILE* fp;
};

static int pf_newinstsize (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t newinstsize = STIX_SIZEOF(stdio_t) - STIX_SIZEOF(stix_obj_t);
	STIX_STACK_SETRET (stix, nargs, STIX_SMOOI_TO_OOP(newinstsize)); 
	return 1;
}

static int pf_open (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_char_t name;
	stix_oop_char_t mode;
	stdio_t* rcv;

#if defined(STIX_OOCH_IS_UCH)
	stix_oow_t ucslen, bcslen;
	stix_bch_t namebuf[PATH_MAX];
	stix_bch_t modebuf[32]; /* TODO: dynamic-sized conversion?? */
#endif

	rcv = (stdio_t*)STIX_STACK_GETRCV(stix, nargs);
	name = (stix_oop_char_t)STIX_STACK_GETARG(stix, nargs, 0);
	mode = (stix_oop_char_t)STIX_STACK_GETARG(stix, nargs, 1);

#if defined(STIX_OOCH_IS_UCH)
/* TODO: error check on string conversion */
	ucslen = STIX_OBJ_GET_SIZE(name);
	bcslen = STIX_COUNTOF(namebuf) - 1;
	stix_oocstobcs (stix, name->slot, &ucslen, namebuf, &bcslen); /* TODO: error check */
	namebuf[bcslen] = '\0';

	ucslen = STIX_OBJ_GET_SIZE(mode);
	bcslen = STIX_COUNTOF(modebuf) - 1;
	stix_oocstobcs (stix, mode->slot, &ucslen, modebuf, &bcslen); /* TODO: error check */
	modebuf[bcslen] = '\0';

	rcv->fp = fopen (namebuf, modebuf);
#else
	rcv->fp = fopen (name->slot, mode->slot);
#endif
	if (!rcv->fp) 
	{
STIX_DEBUG2 (stix, "cannot open %s for %s\n", namebuf, modebuf);
		return -1;  /* TODO: return success with an object instead... */
	}

STIX_DEBUG3 (stix, "opened %s for %s - %p\n", namebuf, modebuf, rcv->fp);
	STIX_STACK_SETRETTORCV (stix, nargs);
	return 1;
}

static int pf_close (stix_t* stix, stix_ooi_t nargs)
{
	stdio_t* rcv;

	rcv = (stdio_t*)STIX_STACK_GETRCV(stix, nargs);
	if (rcv->fp)
	{
STIX_DEBUG1 (stix, "closing %p\n", rcv->fp);
		fclose (rcv->fp);
		rcv->fp = NULL;
	}

	STIX_STACK_SETRETTORCV (stix, nargs);
	return 1;
}

static int pf_gets (stix_t* stix, stix_ooi_t nargs)
{
	/* return how many bytes have been written.. */
	STIX_STACK_SETRETTORCV (stix, nargs);
	return 1;
}

static int pf_puts (stix_t* stix, stix_ooi_t nargs)
{
	stdio_t* rcv;
	stix_ooi_t i;
	
	

	rcv = (stdio_t*)STIX_STACK_GETRCV(stix, nargs);

	for (i = 0; i < nargs; i++)
	{
		stix_oop_char_t x;
		stix_obj_char_t tmpc;

		x = (stix_oop_char_t)STIX_STACK_GETARG(stix, nargs, i);
		if (STIX_OOP_IS_CHAR(x))
		{
			/* do some faking. */
			STIX_ASSERT (STIX_SIZEOF(tmpc) >= STIX_SIZEOF(stix_obj_t) + STIX_SIZEOF(stix_ooch_t));

			tmpc.slot[0] = STIX_OOP_TO_CHAR(x);
			x = (stix_oop_char_t)&tmpc;
			STIX_OBJ_SET_SIZE(x, 1);
			goto puts_string;
		}
		else if (STIX_OOP_IS_POINTER(x) && STIX_OBJ_GET_FLAGS_TYPE(x) == STIX_OBJ_TYPE_CHAR)
		{
			int n;
			stix_oow_t ucspos, ucsrem, ucslen, bcslen;
			stix_bch_t bcs[1024]; /* TODO: choose a better buffer size */

		puts_string:
			ucspos = 0;
			ucsrem = STIX_OBJ_GET_SIZE(x);
			while (ucsrem > 0)
			{
				ucslen = ucsrem;
				bcslen = STIX_COUNTOF(bcs);
/* TODO: implement character conversion into stdio and use it */
				if ((n = stix_oocstobcs (stix, &x->slot[ucspos], &ucslen, bcs, &bcslen)) <= -1)
				{
					if (n != -2 || ucslen <= 0) 
					{
/* TODO: 
 * 
STIX_STACK_SETRET (stix, nargs, stix->_error);
return 1;
 * 
 */
						stix_seterrnum (stix, STIX_EECERR);
						return -1;
					}
				}

				fwrite (bcs, bcslen, 1, rcv->fp); 
/* TODO; error handling...
STIX_STACK_SETRET (stix, nargs, stix->_error);
return 1;
*/
		/* TODO: abort looping for async processing???? */
				ucspos += ucslen;
				ucsrem -= ucslen;
			}
		}
		else
		{
/* TODO;
STIX_STACK_SETRET (stix, nargs, stix->_error);
return 1;
*/
STIX_DEBUG1 (stix, "ARGUMETN ISN INVALID...[%O]\n", x);
			stix_seterrnum (stix, STIX_EINVAL);
			return -1;
		}
	}

	STIX_STACK_SETRETTORCV (stix, nargs);
	return 1;
}


/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	stix_method_type_t type;
	stix_ooch_t mthname[15];
	int variadic;
	stix_pfimpl_t handler;
};

#define C STIX_METHOD_CLASS
#define I STIX_METHOD_INSTANCE

static fnctab_t fnctab[] =
{
	{ C, { '_','n','e','w','I','n','s','t','S','i','z','e','\0' }, 0, pf_newinstsize   },
	{ I, { 'c','l','o','s','e','\0' },                             0, pf_close         },
	{ I, { 'g','e','t','s','\0' },                                 0, pf_gets          },
	{ I, { 'o','p','e','n',':','f','o','r',':','\0' },             0, pf_open          },
	{ I, { 'p','u','t','s','\0' },                                 1, pf_puts          },
	{ I, { 'p','u','t','s',':','\0' },                             0, pf_puts          }
};

/* ------------------------------------------------------------------------ */

static int import (stix_t* stix, stix_mod_t* mod, stix_oop_t _class)
{
	int ret = 0;
	stix_oow_t i;

	stix_pushtmp (stix, &_class);
	for (i = 0; i < STIX_COUNTOF(fnctab); i++)
	{
		if (stix_genpfmethod (stix, mod, _class, fnctab[i].type, fnctab[i].mthname, fnctab[i].variadic, STIX_NULL) <= -1) 
		{
			/* TODO: delete pfmethod generated??? */
			ret = -1;
			break;
		}
	}

	stix_poptmp (stix);
	return ret;
}

static stix_pfimpl_t query (stix_t* stix, stix_mod_t* mod, const stix_ooch_t* name)
{
	int left, right, mid, n;

	left = 0; right = STIX_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = stix_compoocstr (name, fnctab[mid].mthname);
		if (n < 0) right = mid - 1; 
		else if (n > 0) left = mid + 1;
		else
		{
			return fnctab[mid].handler;
		}
	}

	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}

#if 0
/* TOOD: concept of a argument_check fucntion?
 * check if receiver is a certain object?
 * check if receiver is at a certain size?
 * etc...
 */
static int sanity_check (stix_t* stix)
{
}
#endif

static void unload (stix_t* stix, stix_mod_t* mod)
{
	/* TODO: close all open handle?? */
}

int stix_mod_stdio (stix_t* stix, stix_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = STIX_NULL;
	return 0;
}

