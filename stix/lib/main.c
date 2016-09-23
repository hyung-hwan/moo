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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	if defined(STIX_HAVE_CFG_H)
#		include <ltdl.h>
#		define USE_LTDL
#	endif
#elif defined(__OS2__)
#	define INCL_DOSMODULEMGR
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#	include <os2.h>
#elif defined(__MSDOS__)
#	include <dos.h>
#elif defined(macintosh)
#	include <Timer.h>
#else
#	include <unistd.h>
#	include <ltdl.h>
#	define USE_LTDL

#	if defined(HAVE_TIME_H)
#		include <time.h>
#	endif
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#	if defined(HAVE_SIGNAL_H)
#		include <signal.h>
#	endif
#endif

#if !defined(STIX_DEFAULT_MODPREFIX)
#	if defined(_WIN32)
#		define STIX_DEFAULT_MODPREFIX "stix-"
#	elif defined(__OS2__)
#		define STIX_DEFAULT_MODPREFIX "st-"
#	elif defined(__MSDOS__)
#		define STIX_DEFAULT_MODPREFIX "st-"
#	else
#		define STIX_DEFAULT_MODPREFIX "libstix-"
#	endif
#endif

#if !defined(STIX_DEFAULT_MODPOSTFIX)
#	if defined(_WIN32)
#		define STIX_DEFAULT_MODPOSTFIX ""
#	elif defined(__OS2__)
#		define STIX_DEFAULT_MODPOSTFIX ""
#	elif defined(__MSDOS__)
#		define STIX_DEFAULT_MODPOSTFIX ""
#	else
#		define STIX_DEFAULT_MODPOSTFIX ""
#	endif
#endif

typedef struct bb_t bb_t;
struct bb_t
{
	char buf[1024];
	stix_oow_t pos;
	stix_oow_t len;
	FILE* fp;
};

typedef struct xtn_t xtn_t;
struct xtn_t
{
	const char* source_path; /* main source file */
};

/* ========================================================================= */

static void* sys_alloc (stix_mmgr_t* mmgr, stix_oow_t size)
{
	return malloc (size);
}

static void* sys_realloc (stix_mmgr_t* mmgr, void* ptr, stix_oow_t size)
{
	return realloc (ptr, size);
}

static void sys_free (stix_mmgr_t* mmgr, void* ptr)
{
	free (ptr);
}

static stix_mmgr_t sys_mmgr =
{
	sys_alloc,
	sys_realloc,
	sys_free,
	STIX_NULL
};

/* ========================================================================= */

static STIX_INLINE stix_ooi_t open_input (stix_t* stix, stix_ioarg_t* arg)
{
	xtn_t* xtn = stix_getxtn(stix);
	bb_t* bb;
	FILE* fp;

	if (arg->includer)
	{
		/* includee */
		stix_bch_t bcs[1024]; /* TODO: right buffer size */
		stix_oow_t bcslen = STIX_COUNTOF(bcs);
		stix_oow_t ucslen = ~(stix_oow_t)0;

		if (stix_ucstoutf8 (arg->name, &ucslen, bcs, &bcslen) <= -1)
		{
			stix_seterrnum (stix, STIX_EECERR);
			return -1;
		}

/* TODO: make bcs relative to the includer */

#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
		fp = fopen (bcs, "rb");
#else
		fp = fopen (bcs, "r");
#endif
	}
	else
	{
		/* main stream */
#if defined(__MSDOS__) || defined(_WIN32) || defined(__OS2__)
		fp = fopen (xtn->source_path, "rb");
#else
		fp = fopen (xtn->source_path, "r");
#endif
	}

	if (!fp)
	{
		stix_seterrnum (stix, STIX_EIOERR);
		return -1;
	}

	bb = stix_callocmem (stix, STIX_SIZEOF(*bb));
	if (!bb)
	{
		fclose (fp);
		return -1;
	}

	bb->fp = fp;
	arg->handle = bb;
	return 0;
}

static STIX_INLINE stix_ooi_t close_input (stix_t* stix, stix_ioarg_t* arg)
{
	xtn_t* xtn = stix_getxtn(stix);
	bb_t* bb;

	bb = (bb_t*)arg->handle;
	STIX_ASSERT (bb != STIX_NULL && bb->fp != STIX_NULL);

	fclose (bb->fp);
	stix_freemem (stix, bb);

	arg->handle = STIX_NULL;
	return 0;
}


static STIX_INLINE stix_ooi_t read_input (stix_t* stix, stix_ioarg_t* arg)
{
	/*xtn_t* xtn = hcl_getxtn(hcl);*/
	bb_t* bb;
	stix_oow_t bcslen, ucslen, remlen;
	int x;


	bb = (bb_t*)arg->handle;
	STIX_ASSERT (bb != STIX_NULL && bb->fp != STIX_NULL);
	do
	{
		x = fgetc (bb->fp);
		if (x == EOF)
		{
			if (ferror((FILE*)bb->fp))
			{
				stix_seterrnum (stix, STIX_EIOERR);
				return -1;
			}
			break;
		}

		bb->buf[bb->len++] = x;
	}
	while (bb->len < STIX_COUNTOF(bb->buf) && x != '\r' && x != '\n');

	bcslen = bb->len;
	ucslen = STIX_COUNTOF(arg->buf);
	x = stix_utf8toucs (bb->buf, &bcslen, arg->buf, &ucslen);
	if (x <= -1 && ucslen <= 0)
	{
		stix_seterrnum (stix, STIX_EECERR);
		return -1;
	}

	remlen = bb->len - bcslen;
	if (remlen > 0) memmove (bb->buf, &bb->buf[bcslen], remlen);
	bb->len = remlen;
	return ucslen;
}

static stix_ooi_t input_handler (stix_t* stix, stix_iocmd_t cmd, stix_ioarg_t* arg)
{
	switch (cmd)
	{
		case STIX_IO_OPEN:
			return open_input (stix, arg);
			
		case STIX_IO_CLOSE:
			return close_input (stix, arg);

		case STIX_IO_READ:
			return read_input (stix, arg);

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}
}
/* ========================================================================= */

static void* mod_open (stix_t* stix, const stix_ooch_t* name)
{
#if defined(USE_LTDL)
/* TODO: support various platforms */
	stix_bch_t buf[1024]; /* TODO: use a proper path buffer */
	stix_oow_t ucslen, bcslen;
	stix_oow_t len;
	void* handle;

/* TODO: using MODPREFIX isn't a good idea for all kind of modules.
 * OK to use it for a primitive module.
 * NOT OK to use it for a FFI target. 
 * Attempting /home/hyung-hwan/xxx/lib/libstix-libc.so.6 followed by libc.so.6 is bad.
 * Need to accept the type or flags?
 *
 * mod_open (stix, "xxxx", STIX_MOD_EXTERNAL);
 * if external, don't use DEFAULT_MODPERFIX and MODPOSTFIX???
 */

	len = stix_copybcstr (buf, STIX_COUNTOF(buf), STIX_DEFAULT_MODPREFIX);

/* TODO: proper error checking and overflow checking */
	ucslen = ~(stix_oow_t)0;
	bcslen = STIX_COUNTOF(buf) - len;
	stix_ucstoutf8 (name, &ucslen, &buf[len], &bcslen);

	stix_copybcstr (&buf[bcslen + len], STIX_COUNTOF(buf) - bcslen - len, STIX_DEFAULT_MODPOSTFIX);

	handle = lt_dlopenext (buf);
	if (!handle) 
	{
		buf[bcslen + len] = '\0';
		handle = lt_dlopenext (&buf[len]);
		if (handle) STIX_DEBUG2 (stix, "Opened module file %s handle %p\n", &buf[len], handle);
	}
	else
	{
		STIX_DEBUG2 (stix, "Opened module file %s handle %p\n", buf, handle);
	}

	return handle;

#else
	/* TODO: implemenent this */
	return STIX_NULL;
#endif
}

static void mod_close (stix_t* stix, void* handle)
{
	STIX_DEBUG1 (stix, "Closed module handle %p\n", handle);
#if defined(USE_LTDL)
	lt_dlclose (handle);
#elif defined(_WIN32)
	FreeLibrary ((HMODULE)handle);
#elif defined(__OS2__)
	DosFreeModule ((HMODULE)handle);
#elif defined(__MSDOS__) && defined(QSE_ENABLE_DOS_DYNAMIC_MODULE)
	FreeModule (handle);
#else
        /* nothing to do */
#endif
}

static void* mod_getsym (stix_t* stix, void* handle, const stix_ooch_t* name)
{
#if defined(USE_LTDL)
	stix_bch_t buf[1024]; /* TODO: use a proper buffer. dynamically allocated if conversion result in too a large value */
	stix_oow_t ucslen, bcslen;
	void* sym;
	const char* symname;

	buf[0] = '_';

	ucslen = ~(stix_oow_t)0;
	bcslen = STIX_COUNTOF(buf) - 2;
	stix_ucstoutf8 (name, &ucslen, &buf[1], &bcslen);
	symname = &buf[1];
	sym = lt_dlsym (handle, symname);
	if (!sym)
	{
		symname = &buf[0];
		sym = lt_dlsym (handle, symname);
		if (!sym)
		{
			buf[bcslen + 1] = '_';
			buf[bcslen + 2] = '\0';

			symname = &buf[1];
			sym = lt_dlsym (handle, symname);
			if (!sym)
			{
				symname = &buf[0];
				sym = lt_dlsym (handle, symname);
			}
		}
	}

	if (sym) STIX_DEBUG2 (stix, "Loaded module symbol %s from handle %p\n", symname, handle);
	return sym;
#else
	/* TODO: IMPLEMENT THIS */
	return STIX_NULL;
#endif
}

/* ========================================================================= */

static void log_write (stix_t* stix, stix_oow_t mask, const stix_ooch_t* msg, stix_oow_t len)
{
#if defined(_WIN32)
#	error NOT IMPLEMENTED 
	
#else
	stix_bch_t buf[256];
	stix_oow_t ucslen, bcslen, msgidx;
	int n;




/*if (mask & STIX_LOG_GC) return;*/ /* don't show gc logs */

/* TODO: beautify the log message.
 *       do classification based on mask. */

/*
{
	char ts[32];
	struct tm tm;
	time_t now;
	now = time(NULL);
	localtime_r (&now, &tm);
	strftime (ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", &tm);
	write (1, ts, strlen(ts));
}
*/
	msgidx = 0;
	while (len > 0)
	{
		ucslen = len;
		bcslen = STIX_COUNTOF(buf);

		n = stix_ucstoutf8 (&msg[msgidx], &ucslen, buf, &bcslen);
		if (n == 0 || n == -2)
		{
			stix_oow_t rem;
			const stix_bch_t* ptr;
			/* n = 0: 
			 *   converted all successfully 
			 * n == -2: 
			 *    buffer not sufficient. not all got converted yet.
			 *    write what have been converted this round. */

			STIX_ASSERT (ucslen > 0); /* if this fails, the buffer size must be increased */

			/* attempt to write all converted characters */
			rem = bcslen;
			ptr = buf;
			while (rem > 0)
			{
				stix_ooi_t wr;

				wr = write (1, ptr, rem); /* TODO: write all */
				if (wr <= -1)
				{
					/*if (errno == EAGAIN || errno == EWOULDBLOCK)
					{
						push it to internal buffers? before writing data just converted, need to write buffered data first.
					}*/
					break;
				}

				ptr += wr;
				rem -= wr;
			}

			if (n == 0) break;
			else
			{
				msgidx += ucslen;
				len -= ucslen;
			}
		}
		else if (n <= -1)
		{
			/* conversion error */
			break;
		}
	}
#endif
}

/* ========================================================================= */

static char* syntax_error_msg[] = 
{
	"no error",
	"illegal character",
	"comment not closed",
	"string not closed",
	"no character after $",
	"no valid character after #",
	"wrong character literal",
	"colon expected",
	"string expected",
	"invalid radix", 
	"invalid numeric literal",
	"byte too small or too large",
	"{ expected",
	"} expected",
	"( expected",
	") expected",
	"] expected",
	". expected",
	"| expected",
	"> expected",
	":= expected",
	"identifier expected",
	"integer expected",
	"primitive: expected",
	"wrong directive",
	"undefined class",
	"duplicate class",
	"contradictory class definition",
	"#dcl not allowed",
	"wrong method name",
	"duplicate method name",
	"duplicate argument name",
	"duplicate temporary variable name",
	"duplicate variable name",
	"duplicate block argument name",
	"cannot assign to argument",
	"undeclared variable",
	"unusable variable in compiled code",
	"inaccessible variable",
	"ambiguous variable",
	"wrong expression primary",
	"too many temporaries",
	"too many arguments",
	"too many block temporaries",
	"too many block arguments",
	"too large block",
	"wrong primitive number",
	"#include error",
	"wrong namespace name",
	"wrong pool dictionary name",
	"duplicate pool dictionary name",
	"literal expected"
};

/* ========================================================================= */

stix_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' };
stix_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };


stix_t* g_stix = STIX_NULL;

/* ========================================================================= */


#if defined(__MSDOS__) && defined(_INTELC32_)
static void (*prev_timer_intr_handler) (void);

#pragma interrupt(timer_intr_handler)
static void timer_intr_handler (void)
{
	/*
	_XSTACK *stk;
	int r;
	stk = (_XSTACK *)_get_stk_frame();
	r = (unsigned short)stk_ptr->eax;   
	*/

	/* The timer interrupt (normally) occurs 18.2 times per second. */
	if (g_stix) stix_switchprocess (g_stix);
	_chain_intr(prev_timer_intr_handler);
}

#elif defined(macintosh)

static TMTask g_tmtask;
static ProcessSerialNumber g_psn;

#define TMTASK_DELAY 50 /* milliseconds if positive, microseconds(after negation) if negative */

static pascal void timer_intr_handler (TMTask* task)
{
	if (g_stix) stix_switchprocess (g_stix);
	WakeUpProcess (&g_psn);
	PrimeTime ((QElem*)&g_tmtask, TMTASK_DELAY);
}

#else
static void arrange_process_switching (int sig)
{
	if (g_stix) stix_switchprocess (g_stix);
}
#endif

static void setup_tick (void)
{
#if defined(__MSDOS__) && defined(_INTELC32_)

	prev_timer_intr_handler = _dos_getvect (0x1C);
	_dos_setvect (0x1C, timer_intr_handler);

#elif defined(macintosh)

	GetCurrentProcess (&g_psn);
	memset (&g_tmtask, 0, STIX_SIZEOF(g_tmtask));
	g_tmtask.tmAddr = NewTimerProc (timer_intr_handler);
	InsXTime ((QElem*)&g_tmtask);

	PrimeTime ((QElem*)&g_tmtask, TMTASK_DELAY);

#elif defined(HAVE_SETITIMER) && defined(SIGVTALRM) && defined(ITIMER_VIRTUAL)
	struct itimerval itv;
	struct sigaction act;

	sigemptyset (&act.sa_mask);
	act.sa_handler = arrange_process_switching;
	act.sa_flags = 0;
	sigaction (SIGVTALRM, &act, STIX_NULL);

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 100; /* 100 microseconds */
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 100;
	setitimer (ITIMER_VIRTUAL, &itv, STIX_NULL);
#else

#	error UNSUPPORTED
#endif
}

static void cancel_tick (void)
{
#if defined(__MSDOS__) && defined(_INTELC32_)

	_dos_setvect (0x1C, prev_timer_intr_handler);

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
	setitimer (ITIMER_VIRTUAL, &itv, STIX_NULL);

	sigemptyset (&act.sa_mask); 
	act.sa_handler = SIG_IGN; /* ignore the signal potentially fired by the one-shot arrange above */
	act.sa_flags = 0;
	sigaction (SIGVTALRM, &act, STIX_NULL);

#else
#	error UNSUPPORTED
#endif
}

/* ========================================================================= */

int main (int argc, char* argv[])
{
	stix_t* stix;
	xtn_t* xtn;
	stix_oocs_t objname;
	stix_oocs_t mthname;
	stix_vmprim_t vmprim;
	int i, xret;

#if !defined(macintosh)
	if (argc < 2)
	{
		fprintf (stderr, "Usage: %s filename ...\n", argv[0]);
		return -1;
	}
#endif

	vmprim.mod_open = mod_open;
	vmprim.mod_close = mod_close;
	vmprim.mod_getsym = mod_getsym;
	vmprim.log_write = log_write;


#if defined(USE_LTDL)
	lt_dlinit ();
#endif

	stix = stix_open (&sys_mmgr, STIX_SIZEOF(xtn_t), 2048000lu, &vmprim, STIX_NULL);
	if (!stix)
	{
		printf ("cannot open stix\n");
		return -1;
	}

	{
		stix_oow_t tab_size;

		tab_size = 5000;
		stix_setoption (stix, STIX_SYMTAB_SIZE, &tab_size);
		tab_size = 5000;
		stix_setoption (stix, STIX_SYSDIC_SIZE, &tab_size);
		tab_size = 600;
		stix_setoption (stix, STIX_PROCSTK_SIZE, &tab_size);
	}

	{
		int trait = 0;

		/*trait |= STIX_NOGC;*/
		trait |= STIX_AWAIT_PROCS;
		stix_setoption (stix, STIX_TRAIT, &trait);
	}

	if (stix_ignite(stix) <= -1)
	{
		printf ("cannot ignite stix - %d\n", stix_geterrnum(stix));
		stix_close (stix);
		return -1;
	}

	xtn = stix_getxtn (stix);

#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.st";
	goto compile;
#endif

	for (i = 1; i < argc; i++)
	{
		xtn->source_path = argv[i];

	compile:

		if (stix_compile (stix, input_handler) <= -1)
		{
			if (stix->errnum == STIX_ESYNTAX)
			{
				stix_synerr_t synerr;
				stix_bch_t bcs[1024]; /* TODO: right buffer size */
				stix_oow_t bcslen, ucslen;

				stix_getsynerr (stix, &synerr);

				printf ("ERROR: ");
				if (synerr.loc.file)
				{
					bcslen = STIX_COUNTOF(bcs);
					ucslen = ~(stix_oow_t)0;
					if (stix_ucstoutf8 (synerr.loc.file, &ucslen, bcs, &bcslen) >= 0)
					{
						printf ("%.*s ", (int)bcslen, bcs);
					}
				}
				else
				{
					printf ("%s ", xtn->source_path);
				}


				printf ("syntax error at line %lu column %lu - %s", 
					(unsigned long int)synerr.loc.line, (unsigned long int)synerr.loc.colm,
					syntax_error_msg[synerr.num]);
				if (synerr.tgt.len > 0)
				{
					bcslen = STIX_COUNTOF(bcs);
					ucslen = synerr.tgt.len;

					if (stix_ucstoutf8 (synerr.tgt.ptr, &ucslen, bcs, &bcslen) >= 0)
					{
						printf (" [%.*s]", (int)bcslen, bcs);
					}

				}
				printf ("\n");
			}
			else
			{
				printf ("ERROR: cannot compile code - %d\n", stix_geterrnum(stix));
			}
			stix_close (stix);
#if defined(USE_LTDL)
			lt_dlexit ();
#endif
			return -1;
		}
	}

	printf ("COMPILE OK. STARTING EXECUTION ...\n");
	xret = 0;
	g_stix = stix;
	setup_tick ();

	objname.ptr = str_my_object;
	objname.len = 8;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (stix_invoke (stix, &objname, &mthname) <= -1)
	{
		printf ("ERROR: cannot execute code - %d\n", stix_geterrnum(stix));
		xret = -1;
	}

	cancel_tick ();
	g_stix = STIX_NULL;

	/*stix_dumpsymtab(stix);
	 *stix_dumpdic(stix, stix->sysdic, "System dictionary");*/

	stix_close (stix);

#if defined(USE_LTDL)
	lt_dlexit ();
#endif

#if defined(_WIN32) && defined(_DEBUG)
getchar();
#endif
	return xret;
}
