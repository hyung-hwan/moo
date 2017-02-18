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
#include <xcb/xcb.h>
#include <stdlib.h>

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE


typedef struct x11_t x11_t;
struct x11_t
{
	xcb_connection_t* c;
	xcb_window_t w;
	xcb_intern_atom_reply_t* dwcr; /* TODO: move this to each window */
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

	if (x11->c)
	{
		MOO_DEBUG0 (moo, "<x11.connect> Unable to connect multiple times\n");
		goto softfail;
	}

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

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	MOO_DEBUG1 (moo, "<x11.disconnect> %p\n", x11->c);

	if (x11->c)
	{
		xcb_disconnect (x11->c);
		x11->c = MOO_NULL;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_getfd (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	MOO_DEBUG1 (moo, "<x11.getfd> %p\n", x11->c);

	if (x11->c)
	{
		int c;

		c = xcb_get_file_descriptor(x11->c);
		if (!MOO_IN_SMOOI_RANGE(c)) goto error;
		MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(c));
	}
	else
	{
	error:
		MOO_STACK_SETRETTOERROR (moo, nargs); /* TODO: be more specific about error code */
	}

	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_getevent (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	xcb_generic_event_t* evt;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	MOO_DEBUG1 (moo, "<x11.__getevent> %p\n", x11->c);

	evt = xcb_poll_for_event(x11->c);
	if (evt)
	{
		uint8_t evttype = evt->response_type & ~0x80;

		if (evttype == XCB_CLIENT_MESSAGE && 
		    ((xcb_client_message_event_t*)evt)->data.data32[0] == x11->dwcr->atom)
		{
			xcb_unmap_window (x11->c, x11->w);
			xcb_destroy_window (x11->c, x11->w);
			xcb_flush (x11->c);
			MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(9999)); /* TODO: translate evt to the event object */
		}
		else
		{
			MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(evttype)); /* TODO: translate evt to the event object */
		}
		free (evt);
	}
	else
	{
MOO_DEBUG0(moo, "BBBBBBBBBBBBbb GOT NO EVENT...\n");
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_makewin (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	xcb_screen_t* screen;
	uint32_t mask;
	uint32_t values[2];
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	MOO_DEBUG1 (moo, "<x11.__makewin> %p\n", x11->c);

	screen = xcb_setup_roots_iterator(xcb_get_setup(x11->c)).data;
	x11->w = xcb_generate_id (x11->c);

	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = screen->white_pixel;
	values[1] = XCB_EVENT_MASK_KEY_RELEASE |
	            XCB_EVENT_MASK_BUTTON_PRESS |
	            XCB_EVENT_MASK_EXPOSURE |
	            /*XCB_EVENT_MASK_POINTER_MOTION |*/
	            XCB_EVENT_MASK_ENTER_WINDOW |
	            XCB_EVENT_MASK_LEAVE_WINDOW |
	            XCB_EVENT_MASK_VISIBILITY_CHANGE;
	xcb_create_window (
		x11->c, 
		XCB_COPY_FROM_PARENT,
		x11->w,
		screen->root,
		0, 0, 300, 300, 10,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		mask, values);

	cookie = xcb_intern_atom(x11->c, 1, 12, "WM_PROTOCOLS");
	reply = xcb_intern_atom_reply(x11->c, cookie, 0);

	cookie = xcb_intern_atom(x11->c, 0, 16, "WM_DELETE_WINDOW");
	x11->dwcr = xcb_intern_atom_reply(x11->c, cookie, 0);

	xcb_change_property(x11->c, XCB_PROP_MODE_REPLACE, x11->w, reply->atom, 4, 32, 1, &x11->dwcr->atom);

/*TODO: use xcb_request_check() for error handling. you need to call create_x11->wdwo_checked(). xxx_checked()... */
	xcb_map_window (x11->c, x11->w);
	xcb_flush (x11->c);

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(x11->w)); /* TODO: write this function to return a window handle... */
	return MOO_PF_SUCCESS;
}


/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_win_create (moo_t* moo, moo_ooi_t nargs)
{
	MOO_STACK_SETRET (moo, nargs, moo->_nil); 
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
	{ I, { '_','_','g','e','t','e','v','e','n','t','\0'},                  0, pf_getevent      },
	{ I, { '_','_','m','a','k','e','w','i','n','\0'},                      0, pf_makewin       },
	{ I, { 'c','o','n','n','e','c','t','\0' },                             0, pf_connect       },
	{ I, { 'd','i','s','c','o','n','n','e','c','t','\0' },                 0, pf_disconnect    },
	{ I, { 'g','e','t','f','d','\0' },                                     0, pf_getfd         }
	
};

static int x11_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
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

static int x11_win_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
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
