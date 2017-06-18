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

#ifndef _MOO_MOD_X11_H_
#define _MOO_MOD_X11_H_

#include <moo.h>


#include <X11/Xlib.h>

/* [NOTE] this header is for internal use only. 
 * so most of the declarations here don't have the moo_ prefix */

#define MC MOO_METHOD_CLASS
#define MI MOO_METHOD_INSTANCE

typedef struct x11_win_t x11_win_t;
struct x11_win_t
{
	Display* disp;
	Window wind;
};


typedef struct x11_t* oop_x11_t;
struct x11_t
{
	Display* disp;
};

typedef struct x11_trailer_t x11_trailer_t;
struct x11_trailer_t
{
	XEvent* curevt;
};

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT int moo_mod_x11 (moo_t* moo, moo_mod_t* mod);
MOO_EXPORT int moo_mod_x11_win (moo_t* moo, moo_mod_t* mod);

#if defined(__cplusplus)
}
#endif

#endif

