/*
 * $Id$
 *
    Copyright (c) 2014-2018 Chung, Hyung-Hwan. All rights reserved.

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

#include <X11/Xutil.h>
#include <errno.h>
#include <stdlib.h>


typedef struct x11_modctx_t x11_modctx_t;
struct x11_modctx_t
{
	moo_oop_class_t x11_class;
};

/* TODO: bchars_to_xchar2bstr??? */
static XChar2b* uchars_to_xchar2bstr (moo_t* moo, const moo_uch_t* inptr, moo_oow_t inlen, moo_oow_t* outlen)
{
	moo_uch_t uch;
	const moo_uch_t* endptr;
	XChar2b* outbuf, * outptr;

	outbuf = moo_allocmem(moo, (inlen + 1) * MOO_SIZEOF(*outptr));
	if (!outbuf) return MOO_NULL;

	outptr = outbuf;
	endptr = inptr + inlen;
	while (inptr < endptr)
	{
		uch = *inptr++;

	#if (MOO_SIZEOF_UCH_T > 2)
		if (uch > 0xFFFF) uch = 0xFFFD; /* unicode replacement character */
	#endif

		outptr->byte1 = (uch >> 8) & 0xFF;
		outptr->byte2 = uch & 0xFF;
		outptr++;
	}

	outptr->byte1 = 0;
	outptr->byte2 = 0;

	if (outlen) *outlen = outptr - outbuf;
	return outbuf;
}

/* ------------------------------------------------------------------------ */
static moo_pfrc_t pf_open_display (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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
		#if defined(MOO_OOCH_IS_UCH)
			dispname = moo_dupootobcstr(moo, MOO_OBJ_GET_CHAR_SLOT(np), &bl);
			if (!dispname)
			{
				MOO_DEBUG2 (moo, "<x11.connect> Cannot convert display name %.*js\n", MOO_OBJ_GET_SIZE(np), MOO_OBJ_GET_CHAR_SLOT(np));
				MOO_STACK_SETRETTOERRNUM (moo, nargs);
				goto oops;
			}
		#else
			dispname = MOO_OBJ_GET_CHAR_SLOT(np);
		#endif
		}
	}

	event = moo_allocmem(moo, MOO_SIZEOF(*event));
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

#if defined(MOO_OOCH_IS_UCH)
	if (dispname) moo_freemem (moo, dispname);
#endif
	return MOO_PF_SUCCESS;

oops:
	if (disp) XCloseDisplay (disp);
	if (event) moo_freemem (moo, event);
#if defined(MOO_OOCH_IS_UCH)
	if (dispname) moo_freemem (moo, dispname);
#endif
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close_display (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	x11_trailer_t* tr;

// TODO: CHECK if the receiver is an X11 object

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	if (x11->display != moo->_nil)
	{
		MOO_ASSERT (moo, MOO_OOP_IS_SMPTR(x11->display));
		XCloseDisplay (MOO_OOP_TO_SMPTR(x11->display));
		x11->display = moo->_nil;
	}


	tr = moo_getobjtrailer (moo, MOO_STACK_GETRCV(moo,nargs), MOO_NULL);
	if (tr->event)
	{
		moo_freemem (moo, tr->event);
		tr->event = MOO_NULL;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_fd (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	x11_trailer_t* tr;
// TODO: CHECK if the receiver is an X11 object
	tr = moo_getobjtrailer (moo, MOO_STACK_GETRCV(moo,nargs), MOO_NULL);
	MOO_STACK_SETRET(moo, nargs, MOO_SMOOI_TO_OOP(tr->connection_number));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_llevent (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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
		 * e must be proteced with moo_pushvolat(). 
		 * also x11, tr must be refetched from the stack. */

		switch (event->type)
		{
			case ClientMessage:
				if (event->xclient.data.l[0] == tr->wm_delete_window)
				{
					e->type = MOO_SMOOI_TO_OOP(65537); /* match SHELL_CLOSE in X11.LLEventType */
					e->window = MOO_SMOOI_TO_OOP(event->xclient.window);
					/* WINDOW CLSOE EVENT */
				}

				break;

			case Expose:
			{
				XRectangle rect;

				rect.x      = event->xexpose.x;
				rect.y      = event->xexpose.y;
				rect.width  = event->xexpose.width;
				rect.height = event->xexpose.height;
				if (XCheckWindowEvent (disp, event->xexpose.window, ExposureMask, event))
				{
					Region reg;

					/* merge all expose events in the event queue */
					reg = XCreateRegion ();
					XUnionRectWithRegion (&rect, reg, reg);

					do
					{
						rect.x      = event->xexpose.x;
						rect.y      = event->xexpose.y;
						rect.width  = event->xexpose.width;
						rect.height = event->xexpose.height;
						XUnionRectWithRegion (&rect, reg, reg);
					}
					while (XCheckWindowEvent (disp, event->xexpose.window, ExposureMask, event));

					XClipBox (reg, &rect);
					XDestroyRegion (reg);
				}

				e->window = MOO_SMOOI_TO_OOP(event->xexpose.window);
				e->x = MOO_SMOOI_TO_OOP(rect.x);
				e->y = MOO_SMOOI_TO_OOP(rect.y);
				e->width = MOO_SMOOI_TO_OOP(rect.width);
				e->height = MOO_SMOOI_TO_OOP(rect.height);
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
MOO_DEBUG0 (moo, "NO PENDING EVENT....\n");
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_create_window (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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
		return MOO_PF_SUCCESS;
	}

	tr = moo_getobjtrailer (moo, (moo_oop_t)x11, MOO_NULL);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	/* ScreenCount (disp); -> the number of screens available in this display server */

	if (a0 == moo->_nil) 
	{
		scrn = DefaultScreen (disp);
		parent = RootWindow (disp, scrn);
	}
	else if (!MOO_OOP_IS_SMOOI(a0)) goto einval;
	else
	{
		XWindowAttributes wa;

		parent = MOO_OOP_TO_SMOOI(a0);
		XGetWindowAttributes (disp, parent, &wa);
		scrn = XScreenNumberOfScreen(wa.screen);
	}

	attrs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ExposureMask/* | StructureNotifyMask*/; /* TODO: accept it as a parameter??? */
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
		return MOO_PF_SUCCESS;
	}

	if (parent == RootWindow(disp, scrn))
	{
		XSetWMProtocols (disp, wind, &tr->wm_delete_window, 1);
	}

	/*if (!MOO_IN_SMOOI_RANGE ((moo_ooi_t)wind))*/
	if (wind > MOO_SMOOI_MAX)
	{
		XDestroyWindow (disp, wind);
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		return MOO_PF_SUCCESS;
	}

	XMapWindow (disp, wind);
	XFlush (disp);

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(wind)); 
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_destroy_window (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	moo_oop_t a0;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* window - Integer (Window) */

	if (!MOO_OOP_IS_SMOOI(a0)) 
	{
		MOO_DEBUG0 (moo, "<x11.destroy_window> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
	}
	else
	{
		Display* disp;
		Window wind;

		disp = MOO_OOP_TO_SMPTR(x11->display);
		wind = MOO_OOP_TO_SMOOI(a0);

		XUnmapWindow (disp, wind); 
		XDestroyWindow (disp, wind);

		MOO_STACK_SETRETTORCV (moo, nargs); 
	}
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_create_gc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	Display* disp;
	Window wind;
	GC gc;
	XWindowAttributes wa;

	oop_x11_t x11;
	moo_oop_t a0;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* Window - SmallInteger */

	if (!MOO_OOP_IS_SMOOI(a0))
	{
		MOO_DEBUG0 (moo, "<x11.create_gc> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	wind = MOO_OOP_TO_SMOOI(a0);

	gc = XCreateGC (disp, wind, 0, MOO_NULL);
	if (!MOO_IN_SMPTR_RANGE(gc))
	{
		MOO_DEBUG0 (moo, "<x11.create_gc> GC pointer not in small pointer range\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
		return MOO_PF_SUCCESS;
	}

/*	XGetWindowAttributes (disp, wind, &wa);
	XCopyGC (disp, DefaultGC(disp, XScreenNumberOfScreen(wa.screen)), GCForeground | GCBackground | GCLineWidth | GCLineStyle, gc);*/

	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(gc));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_destroy_gc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	oop_x11_gc_t gc;

	Display* disp;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	gc = (oop_x11_gc_t)MOO_STACK_GETARG(moo, nargs, 0); /* GC object */

	disp = MOO_OOP_TO_SMPTR(x11->display);
	if (MOO_OOP_IS_SMPTR(gc->font_ptr))
	{
		XFreeFont (disp, MOO_OOP_TO_SMPTR(gc->font_ptr));
		gc->font_ptr = moo->_nil;
	}

	if (MOO_OOP_IS_SMPTR(gc->font_set))
	{
		XFreeFontSet (disp, MOO_OOP_TO_SMPTR(gc->font_set));
		gc->font_set = moo->_nil;
MOO_DEBUG0 (moo, "Freed Font Set\n");
	}

	if (MOO_OOP_IS_SMPTR(gc->gc_handle))
	{
		XFreeGC (disp, MOO_OOP_TO_SMPTR(gc->gc_handle));
		gc->gc_handle= moo->_nil;
	}
	MOO_STACK_SETRETTORCV (moo, nargs); 
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_apply_gc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	oop_x11_gc_t a0;

	Display* disp;
	GC gc;
	unsigned long int mask = 0;
	XGCValues v;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	a0 = (oop_x11_gc_t)MOO_STACK_GETARG(moo, nargs, 0); 
/* TODO check if a0 is an instance of X11.GC */

	if (!MOO_OOP_IS_SMPTR(a0->gc_handle)) 
	{
		MOO_DEBUG0 (moo, "<x11.apply_gc> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	disp = MOO_OOP_TO_SMPTR(x11->display);
	gc = MOO_OOP_TO_SMPTR(a0->gc_handle);

	if (MOO_OBJ_IS_CHAR_POINTER(a0->font_name) && MOO_OBJ_GET_SIZE(a0->font_name) > 0)
	{
		XFontSet fs;
		char **missing_charsets;
		int num_missing_charsets = 0;
		char *default_string;

/* TODO: don't create this again  and again */ 
/* TODO: use font name */
		fs = XCreateFontSet (disp, "-adobe-*-medium-r-normal-*-14-*-*-*-*-*-*-*,-baekmuk-*-medium-r-normal-*-14-*-*-*-*-*-*-*,-*-*-medium-r-normal-*-*-*-*-*-*-*-*-*",
			&missing_charsets, &num_missing_charsets, &default_string);
		if (num_missing_charsets)
		{
			int i;

			MOO_DEBUG0 (moo, "The following charsets are missing:\n");
			for(i = 0; i < num_missing_charsets; i++)
			   MOO_DEBUG1 (moo, "\t%s\n",  missing_charsets[i]);
			MOO_DEBUG1 (moo, "The string %s will be used in place of any characters from those set\n", default_string);
			XFreeStringList(missing_charsets);
		}

		if (fs)
		{
/* TODO: error handling. rollback upon failure... etc */
			MOO_ASSERT (moo, MOO_IN_SMPTR_RANGE(fs));
			if (MOO_OOP_IS_SMPTR(a0->font_set)) 
			{
MOO_DEBUG0 (moo, "Freed Font Set ..\n");
				XFreeFontSet (disp, MOO_OOP_TO_SMPTR(a0->font_set));
			}
			a0->font_set = MOO_SMPTR_TO_OOP (fs);
MOO_DEBUG0 (moo, "XCreateFontSet ok....\n");
		}
		else
		{
			XFontStruct* font;

	/* TODO: .... use font_name */
			font = XLoadQueryFont (disp, "-misc-fixed-medium-r-normal-ko-18-120-100-100-c-180-iso10646-1");
			if (font)
			{
				if (!MOO_IN_SMPTR_RANGE(font))
				{
					MOO_DEBUG0 (moo, "<x11.apply_gc> Font pointer not in small pointer range\n");
					XFreeFont (disp, font);
					MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ERANGE);
					return MOO_PF_SUCCESS;
				}
				else
				{
					XSetFont (disp, gc, font->fid);
					if (MOO_OOP_IS_SMPTR(a0->font_ptr)) XFreeFont (disp, MOO_OOP_TO_SMPTR(a0->font_ptr));
					a0->font_ptr = MOO_SMPTR_TO_OOP(font);
				}
			}
			else
			{
				MOO_DEBUG2 (moo, "<x11.apply_gc> Cannot load font - %.*js\n", MOO_OBJ_GET_SIZE(a0->font_name), MOO_OBJ_GET_CHAR_SLOT(a0->font_name));
				MOO_STACK_SETRETTOERROR (moo, nargs, MOO_ESYSERR);
				return MOO_PF_SUCCESS;
			}
		}
	}

/* TODO: accept mask as an option parameter. then only apply fields that matches this given mask */
	if (MOO_OOP_IS_SMOOI(a0->foreground))
	{
		mask |= GCForeground;
		v.foreground = MOO_OOP_TO_SMOOI(a0->foreground);
	}
	if (MOO_OOP_IS_SMOOI(a0->background))
	{
		mask |= GCBackground;
		v.background = MOO_OOP_TO_SMOOI(a0->background);
	}
	if (MOO_OOP_IS_SMOOI(a0->line_width))
	{
		mask |= GCLineWidth;
		v.line_width = MOO_OOP_TO_SMOOI(a0->line_width);
	}
	if (MOO_OOP_IS_SMOOI(a0->line_style))
	{
		mask |= GCLineStyle;
		v.line_style = MOO_OOP_TO_SMOOI(a0->line_style);
	}
	if (MOO_OOP_IS_SMOOI(a0->fill_style))
	{
		mask |= GCFillStyle;
		v.fill_style = MOO_OOP_TO_SMOOI(a0->fill_style);
	}
	XChangeGC (disp, gc, mask, &v);

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_draw_rectangle (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	Display* disp;
	moo_oop_t a0, a1, a2, a3, a4, a5;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* Window - SmallInteger */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* GC - SMPTR */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* x - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* y - SmallInteger */
	a4 = MOO_STACK_GETARG(moo, nargs, 4); /* width - SmallInteger */
	a5 = MOO_STACK_GETARG(moo, nargs, 5); /* height - SmallInteger */

	if (!MOO_OOP_IS_SMOOI(a0) || !MOO_OOP_IS_SMPTR(a1) ||
	    !MOO_OOP_IS_SMOOI(a2) || !MOO_OOP_IS_SMOOI(a3) ||
	    !MOO_OOP_IS_SMOOI(a4) || !MOO_OOP_IS_SMOOI(a5))
	{
		MOO_DEBUG0 (moo, "<x11.draw_rectangle> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	XDrawRectangle (disp, (Window)MOO_OOP_TO_SMOOI(a0), (GC)MOO_OOP_TO_SMPTR(a1), 
		MOO_OOP_TO_SMOOI(a2), MOO_OOP_TO_SMOOI(a3),
		MOO_OOP_TO_SMOOI(a4), MOO_OOP_TO_SMOOI(a5));
	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_fill_rectangle (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	Display* disp;
	moo_oop_t a0, a1, a2, a3, a4, a5;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	a0 = MOO_STACK_GETARG(moo, nargs, 0); /* Window - SmallInteger */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* GC - SMPTR */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* x - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* y - SmallInteger */
	a4 = MOO_STACK_GETARG(moo, nargs, 4); /* width - SmallInteger */
	a5 = MOO_STACK_GETARG(moo, nargs, 5); /* height - SmallInteger */

	if (!MOO_OOP_IS_SMOOI(a0) || !MOO_OOP_IS_SMPTR(a1) ||
	    !MOO_OOP_IS_SMOOI(a2) || !MOO_OOP_IS_SMOOI(a3) ||
	    !MOO_OOP_IS_SMOOI(a4) || !MOO_OOP_IS_SMOOI(a5))
	{
		MOO_DEBUG0 (moo, "<x11.fill_rectangle> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	XFillRectangle (disp, (Window)MOO_OOP_TO_SMOOI(a0), (GC)MOO_OOP_TO_SMPTR(a1), 
		MOO_OOP_TO_SMOOI(a2), MOO_OOP_TO_SMOOI(a3),
		MOO_OOP_TO_SMOOI(a4), MOO_OOP_TO_SMOOI(a5));
	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_draw_string (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_x11_t x11;
	oop_x11_gc_t gc;
	moo_oop_t a1, a2, a3;

	Display* disp;

	x11 = (oop_x11_t)MOO_STACK_GETRCV(moo, nargs);
	disp = MOO_OOP_TO_SMPTR(x11->display);

	gc = (oop_x11_gc_t)MOO_STACK_GETARG(moo, nargs, 0); /* GC object */
	a1 = MOO_STACK_GETARG(moo, nargs, 1); /* x - SmallInteger */
	a2 = MOO_STACK_GETARG(moo, nargs, 2); /* y - SmallInteger */
	a3 = MOO_STACK_GETARG(moo, nargs, 3); /* string */

/* TODO: check if gc is an instance of X11.GC */

	if (!MOO_OOP_IS_SMOOI(a1) || !MOO_OOP_IS_SMOOI(a2) || 
	    !MOO_OBJ_IS_CHAR_POINTER(a3))
	{
		MOO_DEBUG0 (moo, "<x11.draw_string> Invalid parameters\n");
		MOO_STACK_SETRETTOERROR (moo, nargs, MOO_EINVAL);
		return MOO_PF_SUCCESS;
	}

	if (MOO_OOP_IS_SMPTR(gc->font_set))
	{
		moo_oow_t bcslen;
		moo_bch_t* bb;
		int ascent = 10;
		XRectangle r;

	#if defined(MOO_OOCH_IS_UCH)
		moo_oow_t oocslen;

		oocslen = MOO_OBJ_GET_SIZE(a3);
		if (moo_convootobchars(moo, MOO_OBJ_GET_CHAR_SLOT(a3), &oocslen, MOO_NULL, &bcslen) <= -1 ||
		    !(bb = moo_allocmem(moo, MOO_SIZEOF(moo_bch_t) * bcslen)))
		{
			MOO_DEBUG0 (moo, "<x11.draw_string> Error in converting a string\n");
			MOO_STACK_SETRETTOERRNUM (moo, nargs);
			return MOO_PF_SUCCESS;
		}
		moo_convootobchars (moo, MOO_OBJ_GET_CHAR_SLOT(a3), &oocslen, bb, &bcslen);
	#else
		bb = MOO_OBJ_GET_CHAR_SLOT(a3);
		bcslen = MOO_OBJ_GET_SIZE(a3);
	#endif

		XmbTextExtents(MOO_OOP_TO_SMPTR(gc->font_set), bb, bcslen, MOO_NULL, &r);
		ascent = r.height;

		/* what about Xutf8DrawString? */
		XmbDrawString (disp, (Window)MOO_OOP_TO_SMOOI(((oop_x11_widget_t)gc->widget)->window_handle), 
			MOO_OOP_TO_SMPTR(gc->font_set), MOO_OOP_TO_SMPTR(gc->gc_handle), 
			MOO_OOP_TO_SMOOI(a1), MOO_OOP_TO_SMOOI(a2) + ascent,  bb, bcslen);

	#if defined(MOO_OOCH_IS_UCH)
		moo_freemem (moo, bb);
	#endif
	}
	else
	{
	#if defined(MOO_OOCH_IS_UCH)
		XChar2b* stptr;
		moo_oow_t stlen;
		int ascent = 0;

	/* TODO: draw string chunk by chunk to avoid memory allocation in uchars_to_xchars2bstr */
		stptr = uchars_to_xchar2bstr(moo, MOO_OBJ_GET_CHAR_SLOT(a3), MOO_OBJ_GET_SIZE(a3), &stlen);
		if (!stptr)
		{
			MOO_DEBUG0 (moo, "<x11.draw_string> Error in converting a string\n");
			MOO_STACK_SETRETTOERRNUM (moo, nargs);
			return MOO_PF_SUCCESS;
		}

		if (MOO_OOP_IS_SMPTR(gc->font_ptr))
		{
			int direction, descent;
			XCharStruct overall;
			XTextExtents16 (MOO_OOP_TO_SMPTR(gc->font_ptr), stptr, stlen, &direction, &ascent, &descent, &overall);
		}

		XDrawString16 (disp, (Window)MOO_OOP_TO_SMOOI(((oop_x11_widget_t)gc->widget)->window_handle), MOO_OOP_TO_SMPTR(gc->gc_handle),
			MOO_OOP_TO_SMOOI(a1), MOO_OOP_TO_SMOOI(a2) + ascent, stptr, stlen);

		moo_freemem (moo, stptr);
	#else
		int ascent = 0;

		if (MOO_OOP_IS_SMPTR(gc->font_ptr))
		{
			int direction, descent;
			XCharStruct overall;
			XTextExtents16 (MOO_OOP_TO_SMPTR(gc->font_ptr), MOO_OBJ_GET_CHAR_SLOT(a3), MOO_OBJ_GET_SIZE(a3), &direction, &ascent, &descent, &overall);
		}

		XDrawString (disp, (Window)MOO_OOP_TO_SMOOI(((oop_x11_widget_t)gc->widget)->window_handle), MOO_OOP_TO_SMPTR(gc->gc_handle),
			MOO_OOP_TO_SMOOI(a1), MOO_OOP_TO_SMOOI(a2) + ascent, MOO_OBJ_GET_CHAR_SLOT(a3), MOO_OBJ_GET_SIZE(a3));
	#endif
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{
	{ MI, { 'a','p','p','l','y','_','g','c','\0' },                         0, { pf_apply_gc,        1, 1 } },
	{ MI, { 'c','l','o','s','e','_','d','i','s','p','l','a','y','\0' },     0, { pf_close_display,   0, 0 } },
	{ MI, { 'c','r','e','a','t','e','_','g','c','\0' },                     0, { pf_create_gc,       1, 1 } },
	{ MI, { 'c','r','e','a','t','e','_','w','i','n','d','o','w','\0' },     0, { pf_create_window,   7, 7 } },
	{ MI, { 'd','e','s','t','r','o','y','_','g','c','\0' },                 0, { pf_destroy_gc,      1, 1 } },
	{ MI, { 'd','e','s','t','r','o','y','_','w','i','n','d','o','w','\0' }, 0, { pf_destroy_window,  1, 1 } },

	{ MI, { 'd','r','a','w','_','r','e','c','t','a','n','g','l','e','\0' }, 0, { pf_draw_rectangle,  6, 6 } },
	{ MI, { 'd','r','a','w','_','s','t','r','i','n','g','\0' },             0, { pf_draw_string,     4, 4 } },
	{ MI, { 'f','i','l','l','_','r','e','c','t','a','n','g','l','e','\0' }, 0, { pf_fill_rectangle,  6, 6 } },
	{ MI, { 'g','e','t','_','f','d','\0' },                                 0, { pf_get_fd,          0, 0 } },
	{ MI, { 'g','e','t','_','l','l','e','v','e','n','t','\0'},              0, { pf_get_llevent,     1, 1 } },
	{ MI, { 'o','p','e','n','_','d','i','s','p','l','a','y','\0' },         0, { pf_open_display,    0, 1 } }
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
	ctx->x11_class = (moo_oop_class_t)moo_moveoop(moo, (moo_oop_t)ctx->x11_class);
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
		moo_oop_t tmp;

		static moo_ooch_t name_x11[] = { 'X','1','1','\0' };

		ctx = moo_callocmem (moo, MOO_SIZEOF(*ctx));
		if (!ctx) return -1;

		tmp = moo_findclass(moo, moo->sysdic, name_x11);
		if (!tmp)
		{
			/* Not a single X11.XXX has been defined. */
			MOO_DEBUG0 (moo, "X11 class not found\n");
			moo_freemem (moo, ctx);
			return -1;
		}
		MOO_STORE_OOP (moo, (moo_oop_t*)&ctx->x11_class, tmp);

		mod->gc = gc_mod_x11;
		mod->ctx = ctx;
	}

	mod->import = x11_import;
	mod->query = x11_query;
	mod->unload = x11_unload; 

	return 0;
}

/* ------------------------------------------------------------------------ */
