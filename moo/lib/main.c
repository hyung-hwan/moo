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

#include <moo-std.h>
#include <moo-utl.h>
#include <moo-opt.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>

#elif defined(__OS2__)
#	define INCL_DOSMODULEMGR
#	define INCL_DOSPROCESS
#	define INCL_DOSSEMAPHORES
#	define INCL_DOSEXCEPTIONS
#	define INCL_DOSMISC
#	define INCL_DOSDATETIME
#	define INCL_DOSFILEMGR
#	define INCL_DOSERRORS
#	include <os2.h>
#	include <time.h>
#	include <fcntl.h>
#	include <io.h>
#	include <errno.h>


#elif defined(__DOS__)
#	include <dos.h>
#	include <time.h>
#	include <io.h>
#	include <signal.h>
#	include <errno.h>


#elif defined(macintosh)
#	include <Types.h>
#	include <OSUtils.h>
#	include <Timer.h>

#	include <MacErrors.h>
#	include <Process.h>
#	include <Dialogs.h>
#	include <TextUtils.h>

	/* TODO: a lot to do */

#elif defined(vms) || defined(__vms)
#	define __NEW_STARLET 1
#	include <starlet.h> /* (SYS$...) */
#	include <ssdef.h> /* (SS$...) */
#	include <lib$routines.h> /* (lib$...) */

	/* TODO: a lot to do */

#else
#	include <sys/types.h>
#	include <unistd.h>
#	include <fcntl.h>
#	include <errno.h>


#	if defined(HAVE_TIME_H)
#		include <time.h>
#	endif
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#	if defined(HAVE_SIGNAL_H)
#		include <signal.h>
#	endif
#	if defined(HAVE_SYS_MMAN_H)
#		include <sys/mman.h>
#	endif

#	if defined(USE_THREAD)
#		include <pthread.h>
#		include <sched.h>
#	endif

#endif

/* ========================================================================= */

static moo_t* g_moo = MOO_NULL;

/* ========================================================================= */

#if defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))

#if defined(_INTELC32_)
static void (*prev_timer_intr_handler) (void);
#else
static void (__interrupt *prev_timer_intr_handler) (void);
#endif

#if defined(_INTELC32_)
#pragma interrupt(timer_intr_handler)
static void timer_intr_handler (void)
#else
static void __interrupt timer_intr_handler (void)
#endif
{
	/*
	_XSTACK *stk;
	int r;
	stk = (_XSTACK *)_get_stk_frame();
	r = (unsigned short)stk_ptr->eax;   
	*/

	/* The timer interrupt (normally) occurs 18.2 times per second. */
	if (g_moo) moo_switchprocess (g_moo);
	_chain_intr (prev_timer_intr_handler);
}

#elif defined(_WIN32)
static HANDLE g_tick_timer = MOO_NULL; /*INVALID_HANDLE_VALUE;*/

static VOID CALLBACK arrange_process_switching (LPVOID arg, DWORD timeLow, DWORD timeHigh) 
{
	if (g_moo) moo_switchprocess (g_moo);
}

#elif defined(__OS2__)
static TID g_tick_tid;
static HEV g_tick_sem; 
static HTIMER g_tick_timer;
static int g_tick_done = 0;

static void EXPENTRY os2_wait_for_timer_event (ULONG x)
{
	APIRET rc;
	ULONG count;

	rc = DosCreateEventSem (NULL, &g_tick_sem, DC_SEM_SHARED, FALSE);
	if (rc != NO_ERROR)
	{
		/* xxxx */
	}

	rc = DosStartTimer (1L, (HSEM)g_tick_sem, &g_tick_timer);
	if (rc != NO_ERROR)
	{
	}

	while (!g_tick_done)
	{
		rc = DosWaitEventSem((HSEM)g_tick_sem, 5000L);
		DosResetEventSem((HSEM)g_tick_sem, &count);
		if (g_moo) moo_switchprocess (g_moo);
	}

	DosStopTimer (g_tick_timer);
	DosCloseEventSem ((HSEM)g_tick_sem);

	g_tick_timer = NULL;
	g_tick_sem = NULL;
	DosExit (EXIT_THREAD, 0);
}


#elif defined(macintosh)

static TMTask g_tmtask;
static ProcessSerialNumber g_psn;

#define TMTASK_DELAY 50 /* milliseconds if positive, microseconds(after negation) if negative */

static pascal void timer_intr_handler (TMTask* task)
{
	if (g_moo) moo_switchprocess (g_moo);
	WakeUpProcess (&g_psn);
	PrimeTime ((QElem*)&g_tmtask, TMTASK_DELAY);
}

#else
static void arrange_process_switching (int sig)
{
	if (g_moo) moo_switchprocess (g_moo);
}
#endif

static void setup_tick (void)
{
#if defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))

	prev_timer_intr_handler = _dos_getvect (0x1C);
	_dos_setvect (0x1C, timer_intr_handler);

#elif defined(_WIN32)

	LARGE_INTEGER li;
	g_tick_timer = CreateWaitableTimer(MOO_NULL, TRUE, MOO_NULL);
	if (g_tick_timer)
	{
		li.QuadPart = -MOO_SECNSEC_TO_NSEC(0, 20000); /* 20000 microseconds. 0.02 seconds */
		SetWaitableTimer (g_tick_timer, &li, 0, arrange_process_switching, MOO_NULL, FALSE);
	}

#elif defined(__OS2__)
	/* TODO: Error check */
	DosCreateThread (&g_tick_tid, os2_wait_for_timer_event, 0, 0, 4096);

#elif defined(macintosh)

	GetCurrentProcess (&g_psn);
	memset (&g_tmtask, 0, MOO_SIZEOF(g_tmtask));
	g_tmtask.tmAddr = NewTimerProc (timer_intr_handler);
	InsXTime ((QElem*)&g_tmtask);
	PrimeTime ((QElem*)&g_tmtask, TMTASK_DELAY);

#elif defined(HAVE_SETITIMER) && defined(SIGVTALRM) && defined(ITIMER_VIRTUAL)
	struct itimerval itv;
	struct sigaction act;

	memset (&act, 0, sizeof(act));
	sigemptyset (&act.sa_mask);
	act.sa_handler = arrange_process_switching;
	act.sa_flags = SA_RESTART;
	sigaction (SIGVTALRM, &act, MOO_NULL);

/*#define MOO_ITIMER_TICK 10000*/ /* microseconds. 0.01 seconds */
#define MOO_ITIMER_TICK 20000 /* microseconds. 0.02 seconds. */
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = MOO_ITIMER_TICK;
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = MOO_ITIMER_TICK;
	setitimer (ITIMER_VIRTUAL, &itv, MOO_NULL);
#else

#	error UNSUPPORTED
#endif
}

static void cancel_tick (void)
{
#if defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))

	_dos_setvect (0x1C, prev_timer_intr_handler);

#elif defined(_WIN32)

	if (g_tick_timer)
	{
		CancelWaitableTimer (g_tick_timer);
		CloseHandle (g_tick_timer);
		g_tick_timer = MOO_NULL;
	}

#elif defined(__OS2__)
	if (g_tick_sem) DosPostEventSem (g_tick_sem);
	g_tick_done = 1;

#elif defined(macintosh)
	RmvTime ((QElem*)&g_tmtask);
	/*DisposeTimerProc (g_tmtask.tmAddr);*/

#elif defined(HAVE_SETITIMER) && defined(SIGVTALRM) && defined(ITIMER_VIRTUAL)
	struct itimerval itv;
	struct sigaction act;

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 0; /* make setitimer() one-shot only */
	itv.it_value.tv_usec = 0;
	setitimer (ITIMER_VIRTUAL, &itv, MOO_NULL);

	sigemptyset (&act.sa_mask); 
	act.sa_handler = SIG_IGN; /* ignore the signal potentially fired by the one-shot arrange above */
	act.sa_flags = 0;
	sigaction (SIGVTALRM, &act, MOO_NULL);

#else
#	error UNSUPPORTED
#endif
}

static MOO_INLINE void abort_moo (void)
{
	if (g_moo)
	{
	#if defined(USE_THREAD)
		xtn_t* xtn = moo_getxtn(g_moo);
		write (xtn->p[1], "Q", 1);
	#endif
		moo_abort (g_moo);
	}
}


#if defined(_WIN32)
static BOOL WINAPI handle_term (DWORD ctrl_type)
{
	if (ctrl_type == CTRL_C_EVENT ||
	    ctrl_type == CTRL_CLOSE_EVENT)
	{
		abort_moo ();
		return TRUE;
	}

	return FALSE;
}

#elif defined(__OS2__)
static EXCEPTIONREGISTRATIONRECORD os2_excrr = { 0 };

static ULONG _System handle_term (
	PEXCEPTIONREPORTRECORD p1,
	PEXCEPTIONREGISTRATIONRECORD p2,
	PCONTEXTRECORD p3,
	PVOID pv)
{
	if (p1->ExceptionNum == XCPT_SIGNAL)
	{
		if (p1->ExceptionInfo[0] == XCPT_SIGNAL_INTR ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_KILLPROC ||
		    p1->ExceptionInfo[0] == XCPT_SIGNAL_BREAK)
		 {
			APIRET rc;

			abort_moo ();

			rc = DosAcknowledgeSignalException (p1->ExceptionInfo[0]);
			return (rc != NO_ERROR)? 1: XCPT_CONTINUE_EXECUTION;
		 }
	}

	return XCPT_CONTINUE_SEARCH; /* exception not resolved */
}
#else

static void handle_term (int sig)
{
	abort_moo ();
}
#endif


static void setup_sigterm (void)
{
#if defined(_WIN32)
	SetConsoleCtrlHandler (handle_term, TRUE);
#elif defined(__OS2__)
	os2_excrr.ExceptionHandler = (ERR)handle_term;
	DosSetExceptionHandler (&os2_excrr); /* TODO: check if NO_ERROR is returned */
#elif defined(__DOS__)
	signal (SIGINT, handle_term);
#else
	struct sigaction sa;
	memset (&sa, 0, MOO_SIZEOF(sa));
	sa.sa_handler = handle_term;
	sigaction (SIGINT, &sa, MOO_NULL);
#endif
}

static void clear_sigterm (void)
{
#if defined(_WIN32)
	SetConsoleCtrlHandler (handle_term, FALSE);
#elif defined(__OS2__)
	DosUnsetExceptionHandler (&os2_excrr);
#elif defined(__DOS__)
	signal (SIGINT, SIG_DFL);
#else
	struct sigaction sa;
	memset (&sa, 0, MOO_SIZEOF(sa));
	sa.sa_handler = SIG_DFL;
	sigaction (SIGINT, &sa, MOO_NULL);
#endif
}
/* ========================================================================= */


#define MIN_MEMSIZE 2048000ul

int main (int argc, char* argv[])
{
	static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' }; /*TODO: make this an argument */
	static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };

	moo_t* moo;
	moo_cfg_t cfg;
	moo_errinf_t errinf;

	moo_oocs_t objname;
	moo_oocs_t mthname;
	int i, xret;

	moo_bci_t c;
	static moo_bopt_lng_t lopt[] =
	{
		{ ":log",         'l' },
		{ ":memsize",     'm' },
		{ "large-pages",  '\0' },
	#if defined(MOO_BUILD_DEBUG)
		{ ":debug",       '\0' }, /* NOTE: there is no short option for --debug */
	#endif
		{ MOO_NULL,       '\0' }
	};
	static moo_bopt_t opt =
	{
		"l:m:",
		lopt
	};

	setlocale (LC_ALL, "");

#if !defined(macintosh)
	if (argc < 2)
	{
	print_usage:
		fprintf (stderr, "Usage: %s [options] filename ...\n", argv[0]);
		fprintf (stderr, " --log filename[,logopts]\n");
		
	#if defined(MOO_BUILD_DEBUG)
		fprintf (stderr, " --debug dbgopts\n");
	#endif
		fprintf (stderr, " --memsize number\n");
		fprintf (stderr, " --large-pages\n");
		return -1;
	}

	memset (&cfg, 0, MOO_SIZEOF(cfg));
	cfg.memsize = MIN_MEMSIZE;

	while ((c = moo_getbopt(argc, argv, &opt)) != MOO_BCI_EOF)
	{
		switch (c)
		{
			case 'l':
				cfg.logopt = opt.arg;
				break;

			case 'm':
				cfg.memsize = strtoul(opt.arg, MOO_NULL, 0);
				if (cfg.memsize <= MIN_MEMSIZE) cfg.memsize = MIN_MEMSIZE;
				break;

			case '\0':
				if (moo_comp_bcstr(opt.lngopt, "large-pages") == 0)
				{
					cfg.large_pages = 1;
					break;
				}
			#if defined(MOO_BUILD_DEBUG)
				else if (moo_comp_bcstr(opt.lngopt, "debug") == 0)
				{
					cfg.dbgopt = opt.arg;
					break;
				}
			#endif

				goto print_usage;

			case ':':
				if (opt.lngopt)
					fprintf (stderr, "bad argument for '%s'\n", opt.lngopt);
				else
					fprintf (stderr, "bad argument for '%c'\n", opt.opt);

				return -1;

			default:
				goto print_usage;
		}
	}

	if (opt.ind >= argc) goto print_usage;
#endif

	moo = moo_openstd(0, &cfg, &errinf);
	if (!moo)
	{
		fprintf (stderr, "ERROR: cannot open moo - %d\n", (int)errinf.num);
		return -1;
	}

	{
		moo_oow_t tab_size;

		tab_size = 5000;
		moo_setoption (moo, MOO_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		moo_setoption (moo, MOO_SYSDIC_SIZE, &tab_size);
		tab_size = 600;
		moo_setoption (moo, MOO_PROCSTK_SIZE, &tab_size);
	}

	{
		moo_bitmask_t bm = 0;

		/*bm |= MOO_NOGC;*/
		bm |= MOO_AWAIT_PROCS;
		moo_setoption (moo, MOO_TRAIT, &bm);

		/* disable GC logs */
		bm = ~MOO_LOG_GC;
		moo_setoption (moo, MOO_LOG_MASK, &bm);
	}

	if (moo_ignite(moo) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot ignite moo - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
		moo_close (moo);
		return -1;
	}

/*
#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.moo";
	goto compile;
#endif
*/

	for (i = opt.ind; i < argc; i++)
	{
		moo_iostd_t instd;
		instd.type = MOO_IOSTD_FILEB;
		instd.u.fileb.path = argv[i];

	/*compile:*/
		if (moo_compilestd(moo, &instd, 1) <= -1)
		{
			if (moo->errnum == MOO_ESYNERR)
			{
				moo_synerr_t synerr;

				moo_getsynerr (moo, &synerr);

				moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: ");
				if (synerr.loc.file)
				{
					moo_logbfmt (moo, MOO_LOG_STDERR, "%js", synerr.loc.file);
				}
				else
				{
					moo_logbfmt (moo, MOO_LOG_STDERR, "%s", argv[i]);
				}

				moo_logbfmt (moo, MOO_LOG_STDERR, "[%zu,%zu] %js", 
					synerr.loc.line, synerr.loc.colm,
					(moo_geterrmsg(moo) != moo_geterrstr(moo)? moo_geterrmsg(moo): moo_geterrstr(moo))
				);

				if (synerr.tgt.len > 0)
				{
					moo_logbfmt (moo, MOO_LOG_STDERR, " - %.*js", synerr.tgt.len, synerr.tgt.ptr);
				}

				moo_logbfmt (moo, MOO_LOG_STDERR, "\n");
			}
			else
			{
				moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot compile code - [%d] %js\n", moo_geterrnum(moo), moo_geterrmsg(moo));
			}

			moo_close (moo);
			return -1;
		}
	}

	MOO_DEBUG0 (moo, "COMPILE OK. STARTING EXECUTION...\n");
	xret = 0;
	g_moo = moo;
	setup_tick ();
	setup_sigterm ();

	objname.ptr = str_my_object;
	objname.len = 8;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (moo_invoke(moo, &objname, &mthname) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot execute code - [%d] %js\n", moo_geterrnum(moo), moo_geterrmsg(moo));
		xret = -1;
	}

	cancel_tick ();
	clear_sigterm ();
	g_moo = MOO_NULL;

	/*moo_dumpsymtab(moo);
	 *moo_dumpdic(moo, moo->sysdic, "System dictionary");*/

	moo_close (moo);
	return xret;
}
