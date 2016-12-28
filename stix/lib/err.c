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

static stix_ooch_t errstr_0[] = {'n','o',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_1[] = {'g','e','n','e','r','i','c',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_2[] = {'n','o','t',' ','i','m','p','l','e','m','e','n','t','e','d','\0'};
static stix_ooch_t errstr_3[] = {'s','u','b','s','y','s','t','e','m',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_4[] = {'i','n','t','e','r','n','a','l',' ','e','r','r','o','r',' ','t','h','a','t',' ','s','h','o','u','l','d',' ','n','e','v','e','r',' ','h','a','v','e',' ','h','a','p','p','e','n','e','d','\0'};
static stix_ooch_t errstr_5[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','s','y','s','t','e','m',' ','m','e','m','o','r','y','\0'};
static stix_ooch_t errstr_6[] = {'i','n','s','u','f','f','i','c','i','e','n','t',' ','o','b','j','e','c','t',' ','m','e','m','o','r','y','\0'};
static stix_ooch_t errstr_7[] = {'i','n','v','a','l','i','d',' ','p','a','r','a','m','e','t','e','r',' ','o','r',' ','a','r','g','u','m','e','n','t','\0'};
static stix_ooch_t errstr_8[] = {'d','a','t','a',' ','n','o','t',' ','f','o','u','n','d','\0'};
static stix_ooch_t errstr_9[] = {'e','x','i','s','t','i','n','g','/','d','u','p','l','i','c','a','t','e',' ','d','a','t','a','\0'};
static stix_ooch_t errstr_10[] = {'b','u','s','y','\0'};
static stix_ooch_t errstr_11[] = {'a','c','c','e','s','s',' ','d','e','n','i','e','d','\0'};
static stix_ooch_t errstr_12[] = {'o','p','e','r','a','t','i','o','n',' ','n','o','t',' ','p','e','r','m','i','t','t','e','d','\0'};
static stix_ooch_t errstr_13[] = {'n','o','t',' ','a',' ','d','i','r','e','c','t','o','r','y','\0'};
static stix_ooch_t errstr_14[] = {'i','n','t','e','r','r','u','p','t','e','d','\0'};
static stix_ooch_t errstr_15[] = {'p','i','p','e',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_16[] = {'r','e','s','o','u','r','c','e',' ','t','e','m','p','o','r','a','r','i','l','y',' ','u','n','a','v','a','i','l','a','b','l','e','\0'};
static stix_ooch_t errstr_17[] = {'d','a','t','a',' ','t','o','o',' ','l','a','r','g','e','\0'};
static stix_ooch_t errstr_18[] = {'m','e','s','s','a','g','e',' ','s','e','n','d','i','n','g',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_19[] = {'r','a','n','g','e',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_20[] = {'b','y','t','e','-','c','o','d','e',' ','f','u','l','l','\0'};
static stix_ooch_t errstr_21[] = {'d','i','c','t','i','o','n','a','r','y',' ','f','u','l','l','\0'};
static stix_ooch_t errstr_22[] = {'p','r','o','c','e','s','s','o','r',' ','f','u','l','l','\0'};
static stix_ooch_t errstr_23[] = {'s','e','m','a','p','h','o','r','e',' ','h','e','a','p',' ','f','u','l','l','\0'};
static stix_ooch_t errstr_24[] = {'s','e','m','a','p','h','o','r','e',' ','l','i','s','t',' ','f','u','l','l','\0'};
static stix_ooch_t errstr_25[] = {'d','i','v','i','d','e',' ','b','y',' ','z','e','r','o','\0'};
static stix_ooch_t errstr_26[] = {'I','/','O',' ','e','r','r','o','r','\0'};
static stix_ooch_t errstr_27[] = {'e','n','c','o','d','i','n','g',' ','c','o','n','v','e','r','s','i','o','n',' ','e','r','r','o','r','\0'};
static stix_ooch_t* errstr[] =
{
	errstr_0, errstr_1, errstr_2, errstr_3, errstr_4, errstr_5, errstr_6, errstr_7,
	errstr_8, errstr_9, errstr_10, errstr_11, errstr_12, errstr_13, errstr_14, errstr_15,
	errstr_16, errstr_17, errstr_18, errstr_19, errstr_20, errstr_21, errstr_22, errstr_23,
	errstr_24, errstr_25, errstr_26, errstr_27 
};

#if defined(STIX_INCLUDE_COMPILER)
static stix_ooch_t synerrstr_0[] = {'n','o',' ','e','r','r','o','r','\0'};
static stix_ooch_t synerrstr_1[] = {'i','l','l','e','g','a','l',' ','c','h','a','r','a','c','t','e','r','\0'};
static stix_ooch_t synerrstr_2[] = {'c','o','m','m','e','n','t',' ','n','o','t',' ','c','l','o','s','e','d','\0'};
static stix_ooch_t synerrstr_3[] = {'s','t','r','i','n','g',' ','n','o','t',' ','c','l','o','s','e','d','\0'};
static stix_ooch_t synerrstr_4[] = {'n','o',' ','c','h','a','r','a','c','t','e','r',' ','a','f','t','e','r',' ','$','\0'};
static stix_ooch_t synerrstr_5[] = {'n','o',' ','v','a','l','i','d',' ','c','h','a','r','a','c','t','e','r',' ','a','f','t','e','r',' ','#','\0'};
static stix_ooch_t synerrstr_6[] = {'w','r','o','n','g',' ','c','h','a','r','a','c','t','e','r',' ','l','i','t','e','r','a','l','\0'};
static stix_ooch_t synerrstr_7[] = {'c','o','l','o','n',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_8[] = {'s','t','r','i','n','g',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_9[] = {'i','n','v','a','l','i','d',' ','r','a','d','i','x','\0'};
static stix_ooch_t synerrstr_10[] = {'i','n','v','a','l','i','d',' ','n','u','m','e','r','i','c',' ','l','i','t','e','r','a','l','\0'};
static stix_ooch_t synerrstr_11[] = {'b','y','t','e',' ','t','o','o',' ','s','m','a','l','l',' ','o','r',' ','t','o','o',' ','l','a','r','g','e','\0'};
static stix_ooch_t synerrstr_12[] = {'w','r','o','n','g',' ','e','r','r','o','r',' ','l','i','t','e','r','a','l','\0'};
static stix_ooch_t synerrstr_13[] = {'{',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_14[] = {'}',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_15[] = {'(',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_16[] = {')',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_17[] = {']',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_18[] = {'.',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_19[] = {' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_20[] = {'|',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_21[] = {'>',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_22[] = {':','=',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_23[] = {'i','d','e','n','t','i','f','i','e','r',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_24[] = {'i','n','t','e','g','e','r',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_25[] = {'p','r','i','m','i','t','i','v','e',':',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t synerrstr_26[] = {'w','r','o','n','g',' ','d','i','r','e','c','t','i','v','e','\0'};
static stix_ooch_t synerrstr_27[] = {'u','n','d','e','f','i','n','e','d',' ','c','l','a','s','s','\0'};
static stix_ooch_t synerrstr_28[] = {'d','u','p','l','i','c','a','t','e',' ','c','l','a','s','s','\0'};
static stix_ooch_t synerrstr_29[] = {'c','o','n','t','r','a','d','i','c','t','o','r','y',' ','c','l','a','s','s',' ','d','e','f','i','n','i','t','i','o','n','\0'};
static stix_ooch_t synerrstr_30[] = {'#','d','c','l',' ','n','o','t',' ','a','l','l','o','w','e','d','\0'};
static stix_ooch_t synerrstr_31[] = {'w','r','o','n','g',' ','m','e','t','h','o','d',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_32[] = {'d','u','p','l','i','c','a','t','e',' ','m','e','t','h','o','d',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_33[] = {'d','u','p','l','i','c','a','t','e',' ','a','r','g','u','m','e','n','t',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_34[] = {'d','u','p','l','i','c','a','t','e',' ','t','e','m','p','o','r','a','r','y',' ','v','a','r','i','a','b','l','e',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_35[] = {'d','u','p','l','i','c','a','t','e',' ','v','a','r','i','a','b','l','e',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_36[] = {'d','u','p','l','i','c','a','t','e',' ','b','l','o','c','k',' ','a','r','g','u','m','e','n','t',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_37[] = {'c','a','n','n','o','t',' ','a','s','s','i','g','n',' ','t','o',' ','a','r','g','u','m','e','n','t','\0'};
static stix_ooch_t synerrstr_38[] = {'u','n','d','e','c','l','a','r','e','d',' ','v','a','r','i','a','b','l','e','\0'};
static stix_ooch_t synerrstr_39[] = {'u','n','u','s','a','b','l','e',' ','v','a','r','i','a','b','l','e',' ','i','n',' ','c','o','m','p','i','l','e','d',' ','c','o','d','e','\0'};
static stix_ooch_t synerrstr_40[] = {'i','n','a','c','c','e','s','s','i','b','l','e',' ','v','a','r','i','a','b','l','e','\0'};
static stix_ooch_t synerrstr_41[] = {'a','m','b','i','g','u','o','u','s',' ','v','a','r','i','a','b','l','e','\0'};
static stix_ooch_t synerrstr_42[] = {'w','r','o','n','g',' ','e','x','p','r','e','s','s','i','o','n',' ','p','r','i','m','a','r','y','\0'};
static stix_ooch_t synerrstr_43[] = {'t','o','o',' ','m','a','n','y',' ','t','e','m','p','o','r','a','r','i','e','s','\0'};
static stix_ooch_t synerrstr_44[] = {'t','o','o',' ','m','a','n','y',' ','a','r','g','u','m','e','n','t','s','\0'};
static stix_ooch_t synerrstr_45[] = {'t','o','o',' ','m','a','n','y',' ','b','l','o','c','k',' ','t','e','m','p','o','r','a','r','i','e','s','\0'};
static stix_ooch_t synerrstr_46[] = {'t','o','o',' ','m','a','n','y',' ','b','l','o','c','k',' ','a','r','g','u','m','e','n','t','s','\0'};
static stix_ooch_t synerrstr_47[] = {'t','o','o',' ','l','a','r','g','e',' ','b','l','o','c','k','\0'};
static stix_ooch_t synerrstr_48[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','f','u','n','c','t','i','o','n',' ','n','u','m','b','e','r','\0'};
static stix_ooch_t synerrstr_49[] = {'w','r','o','n','g',' ','p','r','i','m','i','t','i','v','e',' ','f','u','n','c','t','i','o','n',' ','i','d','e','n','t','i','f','i','e','r','\0'};
static stix_ooch_t synerrstr_50[] = {'w','r','o','n','g',' ','m','o','d','u','l','e',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_51[] = {'#','i','n','c','l','u','d','e',' ','e','r','r','o','r','\0'};
static stix_ooch_t synerrstr_52[] = {'w','r','o','n','g',' ','n','a','m','e','s','p','a','c','e',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_53[] = {'w','r','o','n','g',' ','p','o','o','l',' ','d','i','c','t','i','o','n','a','r','y',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_54[] = {'d','u','p','l','i','c','a','t','e',' ','p','o','o','l',' ','d','i','c','t','i','o','n','a','r','y',' ','n','a','m','e','\0'};
static stix_ooch_t synerrstr_55[] = {'l','i','t','e','r','a','l',' ','e','x','p','e','c','t','e','d','\0'};
static stix_ooch_t* synerrstr[] =
{
	synerrstr_0, synerrstr_1, synerrstr_2, synerrstr_3, synerrstr_4, synerrstr_5, synerrstr_6, synerrstr_7,
	synerrstr_8, synerrstr_9, synerrstr_10, synerrstr_11, synerrstr_12, synerrstr_13, synerrstr_14, synerrstr_15,
	synerrstr_16, synerrstr_17, synerrstr_18, synerrstr_19, synerrstr_20, synerrstr_21, synerrstr_22, synerrstr_23,
	synerrstr_24, synerrstr_25, synerrstr_26, synerrstr_27, synerrstr_28, synerrstr_29, synerrstr_30, synerrstr_31,
	synerrstr_32, synerrstr_33, synerrstr_34, synerrstr_35, synerrstr_36, synerrstr_37, synerrstr_38, synerrstr_39,
	synerrstr_40, synerrstr_41, synerrstr_42, synerrstr_43, synerrstr_44, synerrstr_45, synerrstr_46, synerrstr_47,
	synerrstr_48, synerrstr_49, synerrstr_50, synerrstr_51, synerrstr_52, synerrstr_53, synerrstr_54, synerrstr_55
};
#endif
/* END: GENERATED WITH generr.st */

/* -------------------------------------------------------------------------- 
 * ERROR NUMBER TO STRING CONVERSION
 * -------------------------------------------------------------------------- */
const stix_ooch_t* stix_errnumtoerrstr (stix_errnum_t errnum)
{
	static stix_ooch_t e_unknown[] = {'u','n','k','n','o','w','n',' ','e','r','r','o','r','\0'};
	return (errnum >= 0 && errnum < STIX_COUNTOF(errstr))? errstr[errnum]: e_unknown;
}

#if defined(STIX_INCLUDE_COMPILER)
const stix_ooch_t* stix_synerrnumtoerrstr (stix_synerrnum_t errnum)
{
	static stix_ooch_t e_unknown[] = {'u','n','k','n','o','w','n',' ','e','r','r','o','r','\0'};
	return (errnum >= 0 && errnum < STIX_COUNTOF(synerrstr))? synerrstr[errnum]: e_unknown;
}
#endif

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
