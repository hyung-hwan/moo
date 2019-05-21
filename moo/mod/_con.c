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


#include "_con.h"
#include <moo-utl.h>

#include <string.h>

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__DOS__)

#else
#	include <unistd.h>
#	include <fcntl.h> 
#	include <stdlib.h> /* getenv */
#	include <curses.h>
#	include <term.h>
#	include <sys/ioctl.h>
#endif

typedef struct console_t console_t;
struct console_t
{
	int fd;
	int fd_opened;
	char* cup;
	char* clear;
};
/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_open (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	HANDLE h;
	moo_ooi_t immv;

	h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (h == INVALID_HANDLE_VALUE) return 0;
	if (h == NULL)
	{
		/* the program doens't have an associated handle for stdin. is it a service? */
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMPTR_TO_OOP(h));

#elif defined(__DOS__)
	moo->errnum = MOO_ENOIMPL;
	return -1;
#else

	console_t* con;
	int err;
	char* term;

	con = (console_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (isatty(1))
	{
		con->fd = 1;
	}
	else
	{
		con->fd = open("/dev/tty", O_RDWR, 0);
		if (con->fd == -1)
		{
			/* TODO: failed to open /dev/stdout */
			moo_freemem (moo, con);
			return 0;
		}

		con->fd_opened = 1;
	}

	term = getenv("TERM");
	if (term && setupterm(term, con->fd, &err) == OK) 
	{
	}

	con->cup = tigetstr("cup"); /* TODO: error check */
	con->clear = tigetstr("clear");

#if 0
	{
	const char* cup, * clear;
	struct winsize wsz;

	cup = tigetstr ("cup");
	clear = tigetstr ("clear");
	ioctl (fd, TIOCGWINSZ, &wsz);

	write (fd, clear, strlen(clear));
	/*write (fd, tparm (cup, wsz.ws_row - 1, 0), strlen(tparm (cup, wsz.ws_row - 1, 0)));*/
	/*write (fd, tiparm (cup, 0, 0), strlen(tiparm (cup, 0, 0)));*/
	write (fd, tparm (cup, 0, 0, 0, 0, 0, 0, 0, 0, 0), strlen(tparm (cup, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
	}
#endif

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(con->fd));
#endif
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	moo_oop_t arg;
	HANDLE h;

	con = MOO_STACK_GETARG(moo, nargs, 0);
	h = MOO_OOP_TO_SMPTR(con);
	
	/*if (h != XXX)
	{
		CloseHandle (h);
	}*/
#elif defined(__DOS__)

	/* TODO */

#else
	console_t* con;

	con = (console_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	if (con->fd_opened) close (con->fd);

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_write (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	/* TODO: */
#elif defined(__DOS__)
	/* TODO: */
#else
	console_t* con;
	moo_oop_char_t msg;

	moo_oow_t ucspos, ucsrem, ucslen, bcslen;
	moo_bch_t bcs[1024];
	int n;

	con = (console_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	msg = (moo_oop_char_t)MOO_STACK_GETARG (moo, nargs, 0);

	if (!MOO_OBJ_IS_CHAR_POINTER(msg)) goto einval;

#if defined(MOO_OOCH_IS_UCH)
	ucspos = 0;
	ucsrem = MOO_OBJ_GET_SIZE(msg);
	while (ucsrem > 0)
	{
		ucslen = ucsrem;
		bcslen = MOO_COUNTOF(bcs);
		if ((n = moo_convootobchars (moo, &msg->slot[ucspos], &ucslen, bcs, &bcslen)) <= -1)
		{
			if (n != -2 || ucslen <= 0) return MOO_PF_HARD_FAILURE;
		}

		write (con->fd, bcs, bcslen); /* TODO: error handling */
/* TODO: abort looping for async processing???? */
		ucspos += ucslen;
		ucsrem -= ucslen;
	}
#else
	write (con->fd, MOO_GET_OBJ_CHAR_SLOT(msg), MOO_OBJ_GET_SIZE(msg)); /* TODO: error handling. incomplete write handling */
#endif

	MOO_STACK_SETRETTORCV (moo, nargs); /* TODO: change return code */
	return MOO_PF_SUCCESS;

einval:
	MOO_STACK_SETRETTOERRNUM (moo, nargs); /* TODO: be more specific about the error code */
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_clear (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
#elif defined(__DOS__)

#else
	console_t* con;

	con = (console_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	write (con->fd, con->clear, strlen(con->clear));

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_setcursor (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
#elif defined(__DOS__)

#else
	console_t* con;
	moo_oop_t x, y;
	char* cup;

	con = (console_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	x = MOO_STACK_GETARG(moo, nargs, 0);
	y = MOO_STACK_GETARG(moo, nargs, 1);

	if (!MOO_OOP_IS_SMOOI(x) || !MOO_OOP_IS_SMOOI(y)) goto einval;

	/*cup = tiparm (con->cup, MOO_OOP_TO_SMOOI(y), MOO_OOP_TO_SMOOI(x));*/
	cup = tparm (con->cup, MOO_OOP_TO_SMOOI(y), MOO_OOP_TO_SMOOI(x), 0, 0, 0, 0, 0, 0, 0);
	write (con->fd, cup, strlen(cup)); /* TODO: error check */

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

einval:
	MOO_STACK_SETRETTOERRNUM (moo, nargs); /* TODO: be more specific about the error code */
	return MOO_PF_SUCCESS;
#endif
}

/* ------------------------------------------------------------------------ */

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

static moo_pfinfo_t pfinfos[] =
{
	{ I, { 'c','l','e','a','r','\0' },                 0, { pf_clear,     0,  0 } },
	{ I, { 'c','l','o','s','e','\0' },                 0, { pf_close,     0,  0 } },
	{ I, { 'o','p','e','n','\0' },                     0, { pf_open,      0,  0 } },
	{ I, { 's','e','t','c','u','r','s','o','r','\0' }, 0, { pf_setcursor, 2,  2 } },
	{ I, { 'w','r','i','t','e','\0' },                 0, { pf_write,     1,  1 } }
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(console_t), MOO_NULL) <= -1) return -1;
	return 0;
}

static moo_pfbase_t* query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase(moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}


static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: close all open handle?? */
}

int moo_mod_con (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
