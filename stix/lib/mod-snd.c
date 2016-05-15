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


#include "mod-snd.h"
#include <stix-utl.h>

/* ------------------------------------------------------------------------ */

/* TODO: don't use these macros... */
#define ACTIVE_STACK_PUSH(stix,v) \
	do { \
		(stix)->sp = (stix)->sp + 1; \
		(stix)->active_context->slot[(stix)->sp] = v; \
	} while (0)

#define ACTIVE_STACK_POP(stix) ((stix)->sp = (stix)->sp - 1)
#define ACTIVE_STACK_UNPOP(stix) ((stix)->sp = (stix)->sp + 1)
#define ACTIVE_STACK_POPS(stix,count) ((stix)->sp = (stix)->sp - (count))

#define ACTIVE_STACK_GET(stix,v_sp) ((stix)->active_context->slot[v_sp])
#define ACTIVE_STACK_SET(stix,v_sp,v_obj) ((stix)->active_context->slot[v_sp] = v_obj)
#define ACTIVE_STACK_GETTOP(stix) ACTIVE_STACK_GET(stix, (stix)->sp)
#define ACTIVE_STACK_SETTOP(stix,v_obj) ACTIVE_STACK_SET(stix, (stix)->sp, v_obj)



static int prim_open (stix_t* stix, stix_ooi_t nargs)
{
printf ("<<<<<<<<<<<SND OPEN>>>>>>>>>>>>>>>>>>\n");
	ACTIVE_STACK_POPS (stix, nargs);
	return 1;
}

static int prim_close (stix_t* stix, stix_ooi_t nargs)
{
printf ("<<<<<<<<<<<<<<<<<<<<SND CLOSE>>>>>>>>>>>>>>>>>>>>>>\n");
	ACTIVE_STACK_POPS (stix, nargs);
	return 1;
}

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const stix_bch_t* name;
	stix_prim_impl_t handler;
};

static fnctab_t fnctab[] =
{
	{ "close",      prim_close },
	{ "open",       prim_open  }
};


/* ------------------------------------------------------------------------ */

static stix_prim_impl_t query (stix_t* stix, stix_prim_mod_t* mod, const stix_ooch_t* name)
{
	int left, right, mid, n;

	left = 0; right = STIX_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = stix_compoocbcstr (name, fnctab[mid].name);
		if (n < 0) right = mid - 1; 
		else if (n > 0) left = mid + 1;
		else
		{
			return fnctab[mid].handler;
		}
	}


/*
	ea.ptr = (stix_char_t*)name;
	ea.len = stix_strlen(name);
	stix_seterror (stix, STIX_ENOENT, &ea, QSE_NULL);
*/
	return STIX_NULL;
}


static void unload (stix_t* stix, stix_prim_mod_t* mod)
{
}


int stix_prim_mod_snd (stix_t* stix, stix_prim_mod_t* mod)
{
	mod->query = query;
	mod->unload = unload;
	mod->ctx = STIX_NULL;
	return 0;
}
