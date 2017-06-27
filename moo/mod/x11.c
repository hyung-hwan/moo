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
static moo_pfrc_t pf_open_display (moo_t* moo, moo_ooi_t nargs)
{
	oop_x11_t x11;
	x11_trailer_t* tr;

	Display* disp = MOO_NULL;
	XEvent* event = MOO_NULL;
	char* dispname = MOO_NULL;
	moo_ooi_t connno;

// TODO: CHECK if the receiver is an X11 object

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

	event = moo_allocmem (moo, MOO_SIZEOF(*event));
	if (!event)
	{
		MOO_STACK_SETRETTOERRNUM (moo, nargs);
		goto oops;
	}

	disp = XOpenDisplay(dispname);
	if (!disp)
	{
		MOO_DEBUG1 (moo, "<x11.open_display> Cannot connect to X11 server %hs\n", XDisplayName(dispname));
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ESYSERR);
		goto oops;
	}

	if (!MOO_IN_SMPTR_RANGE(disp))
	{
		MOO_DEBUG1 (moo, "<x11.open_display> Display pointer to %hs not in small pointer range\n", XDisplayName(dispname));
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		goto oops;
	}

	connno = ConnectionNumber(disp);
	if (!MOO_IN_SMOOI_RANGE(connno))
	{
		MOO_DEBUG1 (moo, "<x11.open_display> Connection number to %hs out of small integer range\n", XDisplayName(dispname));
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		goto oops;
	}

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);

	tr = (x11_trailer_t*)moo_getobjtrailer (moo, (moo_oop_t)x11, MOO_NULL);
	tr->event = event;
	tr->connection_number = connno;
	tr->wm_delete_window = XInternAtom (disp, "WM_DELETE_WINDOW", False);

	MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(disp));
	x11->display = MOO_SMPTR_TO_OOP(disp);
	MOO_STACK_SETRETTORCV (moo, nargs);

	if (dispname) moo_freemem (moo, dispname);
	return MOO_PF_SUCCESS;

oops:
	if (disp) XCloseDisplay (disp);
	if (event) moo_freemem (moo, event);
	if (dispname) moo_freemem (moo, dispname);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close_display (moo_t* moo, moo_ooi_t nargs)
{
	oop_x11_t x11;

// TODO: CHECK if the receiver is an X11 object

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	if (x11->display != moo->_nil)
	{
		MOO_ASSERT (moo, MOO_OOP_IS_SMPTR(x11->display));
		XCloseDisplay (MOO_OOP_TO_SMPTR(x11->display));
		x11->display = moo->_nil;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_fd (moo_t* moo, moo_ooi_t nargs)
{
	x11_trailer_t* tr;
// TODO: CHECK if the receiver is an X11 object
	tr = moo_getobjtrailer (moo, MOO_STACK_GETRCV(moo,nargs), MOO_NULL);
	MOO_STACK_SETRET(moo, nargs, MOO_SMOOI_TO_OOP(tr->connection_number));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_llevent (moo_t* moo, moo_ooi_t nargs)
{
	oop_x11_t x11;
	x11_trailer_t* tr;

	Display* disp;
	XEvent*  event;

// TODO: CHECK if the receiver is an X11 object
	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
//MOO_ASSERT (moo, MOO_CLASSOF(moo,x11) == modctx->x11_class);
	tr = moo_getobjtrailer (moo, (moo_oop_t)x11, MOO_NULL);

	disp = MOO_OOP_TO_SMPTR(x11->display);
	event = tr->event;
	if (XPending(disp))
	{
		oop_x11_llevent_t e;

		XNextEvent (disp, event);

		e = (oop_x11_llevent_t)MOO_STACK_GETARG(moo, nargs, 0);
		/* TOOD: check if e is an instance of X11.LLEvent */
		e->type = MOO_SMOOI_TO_OOP(event->type);
		e->window = MOO_SMOOI_TO_OOP(0);

		/* if the following is going to trigger GC directly or indirectly,
		 * e must be proteced with moo_pushtmp() first */

		switch (event->type)
		{
			case ClientMessage:
				if (event->xclient.data.l[0] == tr->wm_delete_window)
				{
					e->window = MOO_SMOOI_TO_OOP(event->xclient.window);
					/* WINDOW CLSOE EVENT */
				}

				break;

			case Expose:
			{
				e->window = MOO_SMOOI_TO_OOP(event->xexpose.window);
				e->x = MOO_SMOOI_TO_OOP(event->xexpose.x);
				e->y = MOO_SMOOI_TO_OOP(event->xexpose.y);
				e->width = MOO_SMOOI_TO_OOP(event->xexpose.width);
				e->height = MOO_SMOOI_TO_OOP(event->xexpose.height);
				break;
			}

			case ButtonPress:
			case ButtonRelease:
			{
				e->window = MOO_SMOOI_TO_OOP(event->xbutton.window);
				e->x = MOO_SMOOI_TO_OOP(event->xbutton.x);
				e->y = MOO_SMOOI_TO_OOP(event->xbutton.y);
				break;
			}
		}

		MOO_STACK_SETRET (moo, nargs, (moo_oop_t)e);
	}
	else
	{
		/* nil if there is no event */
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_create_window (moo_t* moo, moo_ooi_t nargs)
{
	Display* disp;
	Window wind; /* Window -> XID, unsigned long */
	int scrn;
	Window parent;
	XSetWindowAttributes attrs;

	oop_x11_t x11;
	x11_trailer_t* tr;
	moo_oop_t a0, a1, a2, a3, a4, a5, a6;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* parent window - Integer or nil (Window) */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* x - SmallInteger */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* y - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* width - SmallInteger */
	a4 = MOO_STACK_GETARG(moo, nargs, 4); /* height - SmallInteger */
	a5 = MOO_STACK_GETARG(moo, nargs, 5); /* fgcolor - SmallInteger */
	a6 = MOO_STACK_GETARG(moo, nargs, 6); /* bgcolor - SmallInteger */

	if (!MOO_OOP_IS_SMOOI(a1) || !MOO_OOP_IS_SMOOI(a2) || !MOO_OOP_IS_SMOOI(a3) ||
	    !MOO_OOP_IS_SMOOI(a4) || !MOO_OOP_IS_SMOOI(a5) || !MOO_OOP_IS_SMOOI(a6))
	{
	einval:
		MOO_DEBUG0 (moo, "<x11.create_window> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		goto done;
	}

	tr = moo_getobjtrailer (moo, (moo_oop_t)x11, MOO_NULL);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	/* ScreenCount (disp); -> the number of screens available in this display server */

	if (a0 == moo->_nil) 
	{
		scrn = DefaultScreen (disp);
		parent = RootWindow (disp, scrn);
	}
	else
	{
		moo_oow_t tmpoow;
		XWindowAttributes wa;
		if (moo_inttooow(moo, a0, &tmpoow) <= 0) goto einval;

		parent = tmpoow;
		XGetWindowAttributes (disp, parent, &wa);
		MOO_ASSERT (moo, XRootWindowOfScreen(wa.screen) == parent);
		scrn = XScreenNumberOfScreen(wa.screen);
	}

	attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask; /* TODO: accept it as a parameter??? */
	attrs.border_pixel = BlackPixel (disp, scrn); /* TODO: use a6 */
	attrs.background_pixel = WhitePixel (disp, scrn);/* TODO: use a7 */

	wind = XCreateWindow (
		disp,
		parent,
		MOO_OOP_TO_SMOOI(a1), /* x */
		MOO_OOP_TO_SMOOI(a2), /* y */
		MOO_OOP_TO_SMOOI(a3), /* width */
		MOO_OOP_TO_SMOOI(a4), /* height */
		0, /* border width */
		CopyFromParent, /* depth */
		InputOutput,    /* class */
		CopyFromParent, /* visual */
		CWEventMask | CWBackPixel | CWBorderPixel, &attrs);
	if (!wind)
	{
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ESYSERR);
		goto done;
	}

	if (parent == RootWindow(disp, scrn))
	{
		XSetWMProtocols (disp, wind, &tr->wm_delete_window, 1);
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

	MOO_STACK_SETRET (moo, nargs, a0); 
done:
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_destroy_window (moo_t* moo, moo_ooi_t nargs)
{
	oop_x11_t x11;
	moo_oop_t a0;

	moo_oow_t wind;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	a0 = MOO_STACK_GETARG(moo, nargs, 1); /* window - Integer (Window) */

	if (moo_inttooow(moo, a0, &wind) <= 0) 
	{
		MOO_DEBUG0 (moo, "<x11.destroy_window> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		Display* disp;

		disp = MOO_OOP_TO_SMPTR(x11->display);
		XUnmapWindow (disp, (Window)wind); 
		XDestroyWindow (disp, (Window)wind);
XFlush (disp); /* TODO: is XFlush() needed here? */

		MOO_STACK_SETRETTORCV (moo, nargs); 
	}
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_create_gc (moo_t* moo, moo_ooi_t nargs)
{
	Display* disp;
	moo_oow_t wind;

	oop_x11_t x11;
	moo_oop_t a0;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	disp = MOO_OOP_TO_SMPTR(x11);

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* parent window - Integer or nil (Window) */

	if (moo_inttooow(moo, a0, &wind) <= 0) 
	{
		MOO_DEBUG0 (moo, "<x11.create_gc> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		GC gc;

/* TODO: copy from default GC(DefaultGC...) explicitly??? */
		gc = XCreateGC (disp, wind, 0, MOO_NULL);
		if (!MOO_IN_SMPTR_RANGE(gc))
		{
			MOO_DEBUG0 (moo, "<x11.create_gc> GC pointer not in small pointer range\n");
			MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		}
		else
		{
			MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(gc));
		}
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_destroy_gc (moo_t* moo, moo_ooi_t nargs)
{
	oop_x11_t x11;
	moo_oop_t a0;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	a0 = MOO_STACK_GETARG(moo, nargs, 1); /* GC */

	if (!MOO_OOP_IS_SMPTR(a0)) 
	{
		MOO_DEBUG0 (moo, "<x11.destroy_gc> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		Display* disp;
		GC gc;

		disp = MOO_OOP_TO_SMPTR(x11->display);
		gc = MOO_OOP_TO_SMPTR(a0);

		XFreeGC (disp, gc);
		MOO_STACK_SETRETTORCV (moo, nargs); 
	}
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_draw_rectangle (moo_t* moo, moo_ooi_t nargs)
{
	Display* disp;
	moo_oow_t wind, gc;
	moo_oop_t a0, a1, a2, a3, a4, a5;

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* Window */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* GC */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* x - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* y - SmallInteger */
	a4 = MOO_STACK_GETARG(moo, nargs, 4); /* width - SmallInteger */
	a5 = MOO_STACK_GETARG(moo, nargs, 5); /* height - SmallInteger */

	if (!MOO_OOP_IS_SMOOI(a2) || !MOO_OOP_IS_SMOOI(a3) ||
	    !MOO_OOP_IS_SMOOI(a4) || !MOO_OOP_IS_SMOOI(a5) ||
	    moo_inttooow(moo, a0, &wind) <= 0 ||
	    moo_inttooow(moo, a1, &gc) <= 0)
	{
		MOO_DEBUG0 (moo, "<x11.draw_rectangle> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	disp = MOO_OOP_TO_SMPTR(MOO_STACK_GETRCV(moo,nargs));
	XDrawRectangle (disp, (Window)wind, (GC)gc, 
		MOO_OOP_TO_SMOOI(a2), MOO_OOP_TO_SMOOI(a3),
		MOO_OOP_TO_SMOOI(a4), MOO_OOP_TO_SMOOI(a5));

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{
	{ MI, { '_','c','l','o','s','e','_','d','i','s','p','l','a','y','\0' },     0, { pf_close_display,   0, 0 } },
	{ MI, { '_','c','r','e','a','t','e','_','g','c','\0' },                     0, { pf_create_gc,       1, 1 } },
	{ MI, { '_','c','r','e','a','t','e','_','w','i','n','d','o','w','\0' },     0, { pf_create_window,   7, 7 } },
	{ MI, { '_','d','e','s','t','r','o','y','_','g','c','\0' },                 0, { pf_destroy_gc,      1, 1 } },
	{ MI, { '_','d','e','s','t','r','o','y','_','w','i','n','d','o','w','\0' }, 0, { pf_destroy_window,  1, 1 } },
	{ MI, { '_','d','r','a','w','_','r','e','c','t','a','n','g','l','e','\0' }, 0, { pf_draw_rectangle,  6, 6 } },
	//{ MI, { '_','f','i','l','l','_','r','e','c','t','a','n','g','l','e','\0' }, 0, { pf_fill_rectangle,  6, 6 } },
	{ MI, { '_','g','e','t','_','f','d','\0' },                                 0, { pf_get_fd,          0, 0 } },
	{ MI, { '_','g','e','t','_','l','l','e','v','e','n','t','\0'},              0, { pf_get_llevent,     1, 1 } },
	{ MI, { '_','o','p','e','n','_','d','i','s','p','l','a','y','\0' },         0, { pf_open_display,    0, 1 } }
	
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
