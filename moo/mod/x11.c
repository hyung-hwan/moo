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

#if 0
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


#endif

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_open_display (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* x11;
	Display* disp = MOO_NULL;
	XEvent* curevt = MOO_NULL;
	char* dispname = MOO_NULL;

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

static moo_pfrc_t pf_close_display (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t a0;
	
	a0 = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OOP_IS_SMPTR(a0))
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		XCloseDisplay (MOO_OOP_TO_SMPTR(a0));
		MOO_STACK_SETRETTORCV (moo, nargs);
	}
	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_get_fd (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t a0;

	a0 = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OOP_IS_SMPTR(a0))
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		moo_ooi_t c;
		c = ConnectionNumber(MOO_OOP_TO_SMPTR(a0));

		if (!MOO_IN_SMOOI_RANGE(c))
		{
			MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		}
		else
		{
			MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(c));
		}
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_event (moo_t* moo, moo_ooi_t nargs)
{
	
	moo_oop_t a0, a1;

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* display - SmallPointer (Display*) */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* event to return - SmallPointer (XEvent*) */

	
	if (!MOO_OOP_IS_SMPTR(a0) || !MOO_OOP_IS_SMPTR(a1))
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		Display* disp;
		XEvent*  event;

		disp = MOO_OOP_TO_SMPTR(a0);
		event = MOO_OOP_TO_SMPTR(a1);
		if (XPending(disp))
		{
			XNextEvent (disp, event);
			MOO_STACK_SETRET (moo, nargs, moo->_true);
		}
		else
		{
			/* nil if there is no event */
			MOO_STACK_SETRET (moo, nargs, moo->_false);
		}
	}

	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_get_evtbuf (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* tr;

	tr = moo_getobjtrailer (moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_create_window (moo_t* moo, moo_ooi_t nargs)
{
	Window wind; /* Window -> XID, unsigned long */

	Display* disp;
	int scrn;
	Window parent;
	XSetWindowAttributes attrs;

	moo_oop_t a0, a1, a2, a3, a4, a5, a6, a7;

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* display - SmallPointer (Display*) */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* parent window - Integer or nil (Window) */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* x - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* y - SmallInteger */
	a4 = MOO_STACK_GETARG(moo, nargs, 4); /* width - SmallInteger */
	a5 = MOO_STACK_GETARG(moo, nargs, 5); /* height - SmallInteger */
	a6 = MOO_STACK_GETARG(moo, nargs, 6); /* fgcolor - SmallInteger */
	a7 = MOO_STACK_GETARG(moo, nargs, 7); /* bgcolor - SmallInteger */

	if (!MOO_OOP_IS_SMPTR(a0) || 
	    !MOO_OOP_IS_SMOOI(a2) || !MOO_OOP_IS_SMOOI(a3) || !MOO_OOP_IS_SMOOI(a4) ||
	    !MOO_OOP_IS_SMOOI(a5) || !MOO_OOP_IS_SMOOI(a6) || !MOO_OOP_IS_SMOOI(a7))
	{
	einval:
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		goto done;
	}

	disp = MOO_OOP_TO_SMPTR(a0);

	if (a1 == moo->_nil) 
	{
		scrn = DefaultScreen (disp);
		parent = RootWindow (disp, scrn);
	}
	else
	{
		moo_oow_t tmpoow;
		XWindowAttributes wa;
		if (moo_inttooow(moo, a1, &tmpoow) <= 0) goto einval;

		parent = tmpoow;
		XGetWindowAttributes (disp, parent, &wa);
		MOO_ASSERT (moo, XRootWindowOfScreen(wa.screen) == parent);
		scrn = XScreenNumberOfScreen(wa.screen);
	}

	attrs.event_mask = ExposureMask; /* TODO: accept it as a parameter??? */
	attrs.border_pixel = BlackPixel (disp, scrn); /* TODO: use a6 */
	attrs.background_pixel = WhitePixel (disp, scrn);/* TODO: use a7 */

	wind = XCreateWindow (
		disp,
		parent,
		MOO_OOP_TO_SMOOI(a2), /* x */
		MOO_OOP_TO_SMOOI(a3), /* y */
		MOO_OOP_TO_SMOOI(a4), /* width */
		MOO_OOP_TO_SMOOI(a5), /* height */
		1, /* border width */
		CopyFromParent, /* depth */
		InputOutput,    /* class */
		CopyFromParent, /* visual */
		CWEventMask | CWBackPixel | CWBorderPixel, &attrs);
	if (!wind)
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ESYSERR);
		goto done;
	}

	a0 = moo_oowtoint (moo, wind);
	if (!a0) 
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		goto done;
	}

/* TODO: remvoe this */
XMapWindow (disp, wind);
XFlush (disp);

#if 0
	if (parent == screen->root)
	{
		cookie = xcb_intern_atom(c, 1, 12, "WM_PROTOCOLS");
		reply = xcb_intern_atom_reply(c, cookie, 0);

		cookie = xcb_intern_atom(c, 0, 16, "WM_DELETE_WINDOW");
		win->dwar = xcb_intern_atom_reply(c, cookie, 0);

		xcb_change_property(c, XCB_PROP_MODE_REPLACE, wid, reply->atom, 4, 32, 1, &win->dwar->atom);
	}
#endif

	MOO_STACK_SETRET (moo, nargs, a0); 
done:
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_destroy_window (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t a0, a1;


	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* display - SmallPointer (Display*) */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* window - Integer (Window) */

	if (!MOO_OOP_IS_SMPTR(a0))
	
	{
	einval:
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		Display* disp;
		moo_oow_t wind;

		disp = MOO_OOP_TO_SMPTR(a0);
		if (moo_inttooow(moo, a1, &wind) <= 0) goto einval;

		XUnmapWindow (disp, (Window)wind); 
		XDestroyWindow (disp, (Window)wind);
XFlush (disp);

		MOO_STACK_SETRETTORCV (moo, nargs); 
	}
	return MOO_PF_SUCCESS;
}


/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{
	{ MC, { '_','c','l','o','s','e','_','d','i','s','p','l','a','y','\0' },     0, { pf_close_display,   1, 1 } },
	{ MC, { '_','c','r','e','a','t','e','_','w','i','n','d','o','w','\0' },     0, { pf_create_window,   8, 8 } },
	{ MC, { '_','d','e','s','t','r','o','y','_','w','i','n','d','o','w','\0' }, 0, { pf_destroy_window,  2, 2 } },
	{ MC, { '_','g','e','t','_','e','v','e','n','t','\0'},                      0, { pf_get_event,       2, 2 } },
	{ MI, { '_','g','e','t','_','e','v','t','b','u','f','\0'},                  0, { pf_get_evtbuf,      0, 0 } },
	{ MC, { '_','g','e','t','_','f','d','\0' },                                 0, { pf_get_fd,          1, 1 } },
	{ MC, { '_','o','p','e','n','_','d','i','s','p','l','a','y','\0' },         0, { pf_open_display,    0, 1 } }
	

#if 0
	{ MI, { '_','g','e','t','_','b','a','s','e','\0' },                    0, { pf_get_base,   0, 0 } },

	
#endif
};

static int x11_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_trailer_t), MOO_NULL) <= -1) return -1;
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
