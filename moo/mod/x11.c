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
#include <string.h>

#include <xcb/xcb.h>

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE


typedef struct x11_t x11_t;
struct x11_t
{
	xcb_connection_t* c;
};

typedef struct x11_win_t x11_win_t;
struct x11_win_t
{
	xcb_window_t w;
};

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_connect (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	xcb_connection_t* c;

/* TODO: accept display target as a parameter */
	if (nargs != 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	/*
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OBJ_IS_CHAR_POINTER(name)) 
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	MOO_DEBUG3 (moo, "<x11.connect> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, rcv->handle);
	*/
	c = xcb_connect (MOO_NULL, MOO_NULL);
	if (!c) 
	{
		MOO_DEBUG0 (moo, "<x11.connect> Cannot connect to X11 server\n");
		goto softfail;
	}

	x11->c = c;
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_disconnect (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;

	if (nargs != 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	MOO_DEBUG1 (moo, "<x11.disconnect> %p\n", x11->c);

	xcb_disconnect (x11->c);
	x11->c = MOO_NULL;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_win_create (moo_t* moo, moo_ooi_t nargs)
{
	MOO_STACK_SETRET (moo, nargs, moo->_nil); 
#if 0
	screen = xcb_setup_roots_iterator(xcb_get_setup(x11->c)).data;
	x11->mw = xcb_generate_id (xtn->c);

	xcb_create_window (x11->c, 
		XCB_COPY_FROM_PARENT,
		xtn->mw,
		screen->root,
		0, 0, 300, 300, 10,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		0, MOO_NULL);

	xcb_map_window (xtn->xcb, xtn->mw);
	xcb_flush (xtn->xcb);

	xcb_generic_event_t* evt;
	while ((evt = xcb_poll_for_event(xtn->xcb, 0)) 
	{
		/* TODO: dispatch event read */
	}
#endif
MOO_DEBUG0 (moo, "x11.window.create....\n");
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_win_destroy (moo_t* moo, moo_ooi_t nargs)
{
	MOO_STACK_SETRET (moo, nargs, moo->_nil); 
MOO_DEBUG0 (moo, "x11.window.destroy....\n");
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{
	{ I, { 'c','o','n','n','e','c','t','\0' },                             0, pf_connect       },
	{ I, { 'd','i','s','c','o','n','n','e','c','t','\0' },                 0, pf_disconnect    }
};

static int x11_import (moo_t* moo, moo_mod_t* mod, moo_oop_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_t)) <= -1) return -1;
	return moo_genpfmethods(moo, mod, _class, x11_pfinfo, MOO_COUNTOF(x11_pfinfo));
}

static moo_pfimpl_t x11_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	return moo_findpfimpl (moo, x11_pfinfo, MOO_COUNTOF(x11_pfinfo), name);
}

static void x11_unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
}

int moo_mod_x11 (moo_t* moo, moo_mod_t* mod)
{
	mod->import = x11_import;
	mod->query = x11_query;
	mod->unload = x11_unload; 
	mod->ctx = MOO_NULL;
	return 0;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_win_pfinfo[] =
{
	{ I, { 'c','r','e','a','t','e','\0' },                                 0, pf_win_create        },
	{ I, { 'd','i','s','t','r','o','y','\0' },                             0, pf_win_destroy       }
};

static int x11_win_import (moo_t* moo, moo_mod_t* mod, moo_oop_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_win_t)) <= -1) return -1;
	return moo_genpfmethods(moo, mod, _class, x11_win_pfinfo, MOO_COUNTOF(x11_win_pfinfo));
}

static moo_pfimpl_t x11_win_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	return moo_findpfimpl(moo, x11_win_pfinfo, MOO_COUNTOF(x11_win_pfinfo), name);
}

static void x11_win_unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? */
}

int moo_mod_x11_win (moo_t* moo, moo_mod_t* mod)
{
	mod->import = x11_win_import;
	mod->query = x11_win_query;
	mod->unload = x11_win_unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
