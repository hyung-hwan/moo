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

#include "stix-prv.h"



/* BEGIN: GENERATED WITH generr.st */

static stix_ooch_t e0[] = {'n','o',' ','e','r','r','o','r','\0'};
static stix_ooch_t e1[] = {'g','e','n','e','r','i','c',' ','e','r','r','o','r','\0'};
static stix_ooch_t e2[] = {'n','o','t',' ','i','m','p','l','e','m','e','n','t','e','d','\0'};
static stix_ooch_t e3[] = {'s','u','b','s','y','s','t','e','m',' ','e','r','r','o','r','\0'};
static stix_ooch_t e4[] = {'i','n','t','e','r','n','a','l',' ','e','r','r','o','r',' ','t','h','a','t',' ','s','h','o','u','l','d',' ','n','e','v','e','r',' ','h','a','v','e',' ','h','a','p','p','e','n','e','d','\0'};
static stix_ooch_t e5[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','s','y','s','t','e','m',' ','m','e','m','o','r','y','\0'};
static stix_ooch_t e6[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','o','b','j','e','c','t',' ','m','e','m','o','r','y','\0'};
static stix_ooch_t e7[] = {'i','n','v','a','l','i','d',' ','p','a','r','a','m','e','t','e','r',' ','o','r',' ','a','r','g','u','m','e','n','t','\0'};
static stix_ooch_t e8[] = {'d','a','t','a',' ','n','o','t',' ','f','o','u','n','d','\0'};
static stix_ooch_t e9[] = {'e','x','i','s','t','i','n','g','/','d','u','p','l','i','c','a','t','e',' ','d','a','t','a','\0'};
static stix_ooch_t e10[] = {'b','u','s','y','\0'};
static stix_ooch_t e11[] = {'a','c','c','e','s','s',' ','d','e','n','i','e','d','\0'};
static stix_ooch_t e12[] = {'o','p','e','r','a','t','i','o','n',' ','n','o','t',' ','p','e','r','m','i','t','t','e','d','\0'};
static stix_ooch_t e13[] = {'n','o','t',' ','a',' ','d','i','r','e','c','t','o','r','y','\0'};
static stix_ooch_t e14[] = {'i','n','t','e','r','r','u','p','t','e','d','\0'};
static stix_ooch_t e15[] = {'p','i','p','e',' ','e','r','r','o','r','\0'};
static stix_ooch_t e16[] = {'r','e','s','o','u','r','c','e',' ','t','e','m','p','o','r','a','r','i','l','y',' ','u','n','a','v','a','i','l','a','b','l','e','\0'};
static stix_ooch_t e17[] = {'d','a','t','a',' ','t','o','o',' ','l','a','r','g','e','\0'};
static stix_ooch_t e18[] = {'m','e','s','s','a','g','e',' ','s','e','n','d','i','n','g',' ','e','r','r','o','r','\0'};
static stix_ooch_t e19[] = {'r','a','n','g','e',' ','e','r','r','o','r','\0'};
static stix_ooch_t e20[] = {'b','y','t','e','-','c','o','d','e',' ','f','u','l','l','\0'};
static stix_ooch_t e21[] = {'d','i','c','t','i','o','n','a','r','y',' ','f','u','l','l','\0'};
static stix_ooch_t e22[] = {'p','r','o','c','e','s','s','o','r',' ','f','u','l','l','\0'};
static stix_ooch_t e23[] = {'s','e','m','a','p','h','o','r','e',' ','h','e','a','p',' ','f','u','l','l','\0'};
static stix_ooch_t e24[] = {'s','e','m','a','p','h','o','r','e',' ','l','i','s','t',' ','f','u','l','l','\0'};
static stix_ooch_t e25[] = {'d','i','v','i','d','e',' ','b','y',' ','z','e','r','o','\0'};
static stix_ooch_t e26[] = {'I','/','O',' ','e','r','r','o','r','\0'};
static stix_ooch_t e27[] = {'e','n','c','o','d','i','n','g',' ','c','o','n','v','e','r','s','i','o','n',' ','e','r','r','o','r','\0'};
static stix_ooch_t* errstr[] =
{
	e0, e1, e2, e3, e4, e5, e6, e7,
	e8, e9, e10, e11, e12, e13, e14, e15,
	e16, e17, e18, e19, e20, e21, e22, e23,
	e24, e25, e26, e27 
};

/* END: GENERATED WITH generr.st */

/* -------------------------------------------------------------------------- 
 * ERROR NUMBER TO STRING CONVERSION
 * -------------------------------------------------------------------------- */
const stix_ooch_t* stix_errnumtoerrstr (stix_errnum_t errnum)
{
	static stix_ooch_t e_unknown[] = {'u','n','k','n','o','w','n',' ','e','r','r','o','r','\0'};
	return (errnum >= 0 && errnum < STIX_COUNTOF(errstr))? errstr[errnum]: e_unknown;
}

/* -------------------------------------------------------------------------- 
 * SYSTEM DEPENDENT FUNCTIONS 
 * -------------------------------------------------------------------------- */

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSFILEMGR
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <dosfunc.h>
#elif defined(vms) || defined(__vms)
#	define __NEW_STARLET 1
#	include <starlet.h> /* (SYS$...) */
#	include <ssdef.h> /* (SS$...) */
#	include <lib$routines.h> /* (lib$...) */
#elif defined(macintosh)
#	include <Process.h>
#	include <Dialogs.h>
#	include <TextUtils.h>
#else
#	include <sys/types.h>
#	include <unistd.h>
#	include <signal.h>
#	include <errno.h>
#endif

stix_errnum_t stix_syserrtoerrnum (int e)
{
	switch (e)
	{
		case ENOMEM: return STIX_ESYSMEM;
		case EINVAL: return STIX_EINVAL;
		case EBUSY: return STIX_EBUSY;
		case EACCES: return STIX_EACCES;
		case EPERM: return STIX_EPERM;
		case ENOTDIR: return STIX_ENOTDIR;
		case ENOENT: return STIX_ENOENT;
		case EEXIST: return STIX_EEXIST;
		case EINTR:  return STIX_EINTR;
		case EPIPE:  return STIX_EPIPE;
	#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN != EWOULDBLOCK)
		case EAGAIN: 
		case EWOULDBLOCK: return STIX_EAGAIN;
	#elif defined(EAGAIN)
		case EAGAIN: return STIX_EAGAIN;
	#elif defined(EWOULDBLOCK)
		case EWOULDBLOCK: return STIX_EAGAIN;
	#endif
		default:     return STIX_ESYSERR;
	}
}

/* -------------------------------------------------------------------------- 
 * ASSERTION FAILURE HANDLER
 * -------------------------------------------------------------------------- */

void stix_assertfailed (stix_t* stix, const stix_bch_t* expr, const stix_bch_t* file, stix_oow_t line)
{
	stix_logbfmt (stix, STIX_LOG_DEBUG, "ASSERTION FAILURE: %s at %s:%zu\n", expr, file, line);

#if defined(_WIN32)
	ExitProcess (249);
#elif defined(__OS2__)
	DosExit (EXIT_PROCESS, 249);
#elif defined(__DOS__)
	{
		union REGS regs;
		regs.h.ah = DOS_EXIT;
		regs.h.al = 249;
		intdos (&regs, &regs);
	}
#elif defined(vms) || defined(__vms)
	lib$stop (SS$_ABORT); /* use SS$_OPCCUS instead? */
	/* this won't be reached since lib$stop() terminates the process */
	sys$exit (SS$_ABORT); /* this condition code can be shown with
	                       * 'show symbol $status' from the command-line. */
#elif defined(macintosh)
	ExitToShell ();
#else
	kill (getpid(), SIGABRT);
	_exit (1);
#endif
}
