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

#include "_x11.h"
#include <moo-utl.h>

#include <errno.h>
#include <stdlib.h>


typedef struct x11_modctx_t x11_modctx_t;
struct x11_modctx_t
{
	moo_oop_class_t x11_class;
};

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_connect (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;
	Display* disp = MOO_NULL;
	XEvent* curevt = MOO_NULL;
	char* dispname = MOO_NULL;

	x11 = (x11_trailer_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (x11->disp)
	{
		MOO_DEBUG0 (moo, "<x11.connect> Unable to open a display multiple times\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EEXIST);
		goto oops;
	}

	if (nargs >= 1)
	{
		moo_oop_t np;

		np = MOO_STACK_GETARG(moo, nargs, 0);
		if (np != moo->_nil)
		{
			moo_oow_t bl;

			if (!MOO_OBJ_IS_CHAR_POINTER(np)) 
			{
				MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
				goto oops;
			}

			bl = MOO_OBJ_GET_SIZE(np);
			dispname = moo_dupootobcstr (moo, MOO_OBJ_GET_CHAR_SLOT(np), &bl);
			if (!dispname)
			{
				MOO_DEBUG2 (moo, "<x11.connect> Cannot convert display name %.*js\n", MOO_OBJ_GET_SIZE(np), MOO_OBJ_GET_CHAR_SLOT(np));
				MOO_STACK_SETRETTOERRNUM (moo, nargs);
				goto oops;
			}
		}
	}

	curevt = moo_allocmem (moo, MOO_SIZEOF(*x11->curevt));
	if (!curevt)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		goto oops;
	}

	disp = XOpenDisplay(dispname);
	if (!disp)
	{
		MOO_DEBUG1 (moo, "<x11.connect> Cannot connect to X11 server %hs\n", XDisplayName(dispname));
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ESYSERR);
		goto oops;
	}

	x11->disp = disp;
	x11->curevt =  curevt;

	MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(disp));

	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(disp));
	if (dispname) moo_freemem (moo, dispname);
	return MOO_PF_SUCCESS;

oops:
	if (disp) XCloseDisplay (disp);
	if (curevt) moo_freemem (moo, curevt);
	if (dispname) moo_freemem (moo, dispname);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_disconnect (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;

	x11 = (x11_trailer_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (x11->curevt) 
	{
		moo_freemem (moo, x11->curevt); 
		x11->curevt = MOO_NULL;
	}

	if (x11->disp)
	{
		XCloseDisplay (x11->disp);
		x11->disp = MOO_NULL;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_base (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;

	x11 = (x11_trailer_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (x11->disp)
	{
		MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP (x11->disp));
	}
	else
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ENOAVAIL);
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_fd (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;

	x11 = (x11_trailer_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (x11->disp)
	{
		int c;

		c = ConnectionNumber(x11->disp);
		if (!MOO_IN_SMOOI_RANGE(c))
		{
			MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		}
		else
		{
			MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(c));
		}
	}
	else
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ENOAVAIL);
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_event (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;

	x11 = (x11_trailer_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	if (XPending(x11->disp))
	{
		XNextEvent (x11->disp, x11->curevt);
		MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(x11->curevt));
	}
	else
	{
		/* nil if there is no event */
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
	}

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{

	{ MI, { '_','c','o','n','n','e','c','t','\0' },                        0, { pf_connect,    0, 1 } },
	{ MI, { '_','d','i','s','c','o','n','n','e','c','t','\0' },            0, { pf_disconnect, 0, 0 } },
	{ MI, { '_','g','e','t','_','b','a','s','e','\0' },                    0, { pf_get_base,   0, 0 } },
	{ MI, { '_','g','e','t','_','e','v','e','n','t','\0'},                 0, { pf_get_event,  0, 0 } },
	{ MI, { '_','g','e','t','_','f','d','\0' },                            0, { pf_get_fd,     0, 0 } }
};

static int x11_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_t), MOO_NULL) <= -1) return -1;
	return 0;
}

static moo_pfbase_t* x11_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase (moo, x11_pfinfo, MOO_COUNTOF(x11_pfinfo), name, namelen);
}

static void x11_unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything else? close all open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
	moo_freemem (moo, mod->ctx);
}

static void gc_mod_x11 (moo_t* moo, moo_mod_t* mod)
{
	x11_modctx_t* ctx = mod->ctx;

	MOO_ASSERT (moo, ctx != MOO_NULL);
	ctx->x11_class = (moo_oop_class_t)moo_moveoop (moo, (moo_oop_t)ctx->x11_class);
}

int moo_mod_x11 (moo_t* moo, moo_mod_t* mod)
{
	if (mod->hints & MOO_MOD_LOAD_FOR_IMPORT)
	{
		mod->gc = MOO_NULL;
		mod->ctx = MOO_NULL;
	}
	else
	{
		x11_modctx_t* ctx;

		static moo_ooch_t name_x11[] = { 'X','1','1','\0' };

		ctx = moo_callocmem (moo, MOO_SIZEOF(*ctx));
		if (!ctx) return -1;

		ctx->x11_class = (moo_oop_class_t)moo_findclass (moo, moo->sysdic, name_x11);
		if (!ctx->x11_class)
		{
			/* Not a single X11.XXX has been defined. */
			MOO_DEBUG0 (moo, "X11 class not found\n");
			moo_freemem (moo, ctx);
			return -1;
		}

		mod->gc = gc_mod_x11;
		mod->ctx = ctx;
	}

	mod->import = x11_import;
	mod->query = x11_query;
	mod->unload = x11_unload; 

	return 0;
}

/* ------------------------------------------------------------------------ */

