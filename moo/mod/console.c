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


#include "console.h"
#include <moo-utl.h>

#include <string.h>
#if defined(_WIN32)

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

static moo_pfrc_t pf_open (moo_t* moo, moo_ooi_t nargs)
{
#if defined(_WIN32)
	HANDLE h;
	moo_ooi_t immv;

	h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == INVALID_HANDLE_VALUE) return 0;
	if (h == NULL)
	{
	}

	imm = moo_makeimm (moo, h);
	if (imm <= -1) 
	{
		/* error */
	}

#elif defined(__DOS__)
	moo->errnum = MOO_ENOIMPL;
	return -1;
#else

	console_t* con;
	int err;
	char* term;

	con = moo_callocmem (moo, MOO_SIZEOF(*con));
	if (!con) return 0;

	if (isatty(1))
	{
		con->fd = 1;
	}
	else
	{
		con->fd = open ("/dev/tty", O_RDWR, 0);
		if (con->fd == -1)
		{
			/* TODO: failed to open /dev/stdout */
			moo_freemem (moo, con);
			return 0;
		}

		con->fd_opened = 1;
	}

	term = getenv ("TERM");
	if (term && setupterm (term, con->fd, &err) == OK)
	{
	}

	con->cup = tigetstr ("cup"); /* TODO: error check */
	con->clear = tigetstr ("clear");

#if 0
	{
	const char* cup, * clear;
	struct winsize wsz;

	cup = tigetstr ("cup");
	clear = tigetstr ("clear");
	ioctl (fd, TIOCGWINSZ, &wsz);

	write (fd, clear, strlen(clear));
	/*write (fd, tparm (cup, wsz.ws_row - 1, 0), strlen(tparm (cup, wsz.ws_row - 1, 0)));*/
	write (fd, tiparm (cup, 0, 0), strlen(tiparm (cup, 0, 0)));
	}
#endif

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP((moo_oow_t)con));
#endif
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_close (moo_t* moo, moo_ooi_t nargs)
{
#if defined(_WIN32)
	HANDLE h;

	h = MOO_STACK_GETARG (moo, nargs, 0);
#elif defined(__DOS__)

	/* TODO */

#else
	console_t* con;

	con = (console_t*)MOO_OOP_TO_SMOOI(MOO_STACK_GETARG (moo, nargs, 0));
	/* TODO: sanity check */

	if (con->fd_opened) close (con->fd);

	moo_freemem (moo, con);
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_write (moo_t* moo, moo_ooi_t nargs)
{
#if defined(_WIN32)
#elif defined(__DOS__)

#else
	console_t* con;
	moo_oop_char_t oomsg;

	moo_oow_t ucspos, ucsrem, ucslen, bcslen;
	moo_bch_t bcs[1024];
	int n;

	con = MOO_OOP_TO_SMOOI(MOO_STACK_GETARG (moo, nargs, 0));
	oomsg = (moo_oop_char_t)MOO_STACK_GETARG (moo, nargs, 1);

	if (MOO_CLASSOF(moo,oomsg) != moo->_string)
	{
/* TODO: invalid message */
		return MOO_PF_FAILURE;
	}

	ucspos = 0;
	ucsrem = MOO_OBJ_GET_SIZE(oomsg);
	while (ucsrem > 0)
	{
		ucslen = ucsrem;
		bcslen = MOO_COUNTOF(bcs);
		if ((n = moo_convootobchars (moo, &oomsg->slot[ucspos], &ucslen, bcs, &bcslen)) <= -1)
		{
			if (n != -2 || ucslen <= 0) 
			{
				moo_seterrnum (moo, MOO_EECERR);
				return MOO_PF_HARD_FAILURE;
			}
		}

		write (con->fd, bcs, bcslen); /* TODO: error handling */
/* TODO: abort looping for async processing???? */
		ucspos += ucslen;
		ucsrem -= ucslen;
	}

	MOO_STACK_SETRETTORCV (moo, nargs); /* TODO: change return code */
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_clear (moo_t* moo, moo_ooi_t nargs)
{
#if defined(_WIN32)
#elif defined(__DOS__)

#else
	console_t* con;

	con = MOO_OOP_TO_SMOOI(MOO_STACK_GETARG(moo, nargs, 0));

	write (con->fd, con->clear, strlen(con->clear));

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_setcursor (moo_t* moo, moo_ooi_t nargs)
{
#if defined(_WIN32)
#elif defined(__DOS__)

#else
	console_t* con;
	moo_oop_oop_t point;
	char* cup;

	con = MOO_OOP_TO_SMOOI(MOO_STACK_GETARG(moo, nargs, 0));
	point = MOO_STACK_GETARG(moo, nargs, 1);

/* TODO: error check, class check, size check.. */
	if (MOO_OBJ_GET_SIZE(point) != 2)
	{
		return MOO_PF_FAILURE;
	}

	cup = tiparm (con->cup, MOO_OOP_TO_SMOOI(point->slot[1]), MOO_OOP_TO_SMOOI(point->slot[0]));
	write (con->fd, cup, strlen(cup)); /* TODO: error check */

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
#endif
}

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const moo_bch_t* name;
	moo_pfimpl_t handler;
};

static fnctab_t fnctab[] =
{
	{ "clear",      pf_clear     },
	{ "close",      pf_close     },
	{ "open",       pf_open      },
	{ "setcursor",  pf_setcursor },
	{ "write",      pf_write     },
};

/* ------------------------------------------------------------------------ */

static moo_pfimpl_t query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name)
{
	int left, right, mid, n;

	left = 0; right = MOO_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = moo_compoocbcstr (name, fnctab[mid].name);
		if (n < 0) right = mid - 1; 
		else if (n > 0) left = mid + 1;
		else
		{
			return fnctab[mid].handler;
		}
	}

	moo->errnum = MOO_ENOENT;
	return MOO_NULL;
}


static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: close all open handle?? */
}

int moo_mod_console (moo_t* moo, moo_mod_t* mod)
{
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
