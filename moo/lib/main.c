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

#include "moo-prv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	if defined(MOO_HAVE_CFG_H)
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

#if !defined(MOO_DEFAULT_MODPREFIX)
#	if defined(_WIN32)
#		define MOO_DEFAULT_MODPREFIX "moo-"
#	elif defined(__OS2__)
#		define MOO_DEFAULT_MODPREFIX "st-"
#	elif defined(__MSDOS__)
#		define MOO_DEFAULT_MODPREFIX "st-"
#	else
#		define MOO_DEFAULT_MODPREFIX "libmoo-"
#	endif
#endif

#if !defined(MOO_DEFAULT_MODPOSTFIX)
#	if defined(_WIN32)
#		define MOO_DEFAULT_MODPOSTFIX ""
#	elif defined(__OS2__)
#		define MOO_DEFAULT_MODPOSTFIX ""
#	elif defined(__MSDOS__)
#		define MOO_DEFAULT_MODPOSTFIX ""
#	else
#		define MOO_DEFAULT_MODPOSTFIX ""
#	endif
#endif

typedef struct bb_t bb_t;
struct bb_t
{
	char buf[1024];
	moo_oow_t pos;
	moo_oow_t len;
	FILE* fp;
};

typedef struct xtn_t xtn_t;
struct xtn_t
{
	const char* source_path; /* main source file */
};

/* ========================================================================= */

static void* sys_alloc (moo_mmgr_t* mmgr, moo_oow_t size)
{
	return malloc (size);
}

static void* sys_realloc (moo_mmgr_t* mmgr, void* ptr, moo_oow_t size)
{
	return realloc (ptr, size);
}

static void sys_free (moo_mmgr_t* mmgr, void* ptr)
{
	free (ptr);
}

static moo_mmgr_t sys_mmgr =
{
	sys_alloc,
	sys_realloc,
	sys_free,
	MOO_NULL
};

/* ========================================================================= */

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#else
#	define IS_PATH_SEP(c) ((c) == '/')
#endif


static const moo_bch_t* get_base_name (const moo_bch_t* path)
{
	const moo_bch_t* p, * last = MOO_NULL;

	for (p = path; *p != '\0'; p++)
	{
		if (IS_PATH_SEP(*p)) last = p;
	}

	return (last == MOO_NULL)? path: (last + 1);
}

static MOO_INLINE moo_ooi_t open_input (moo_t* moo, moo_ioarg_t* arg)
{
	xtn_t* xtn = moo_getxtn(moo);
	bb_t* bb;
	FILE* fp;

	if (arg->includer)
	{
		/* includee */

		moo_bch_t bcs[1024]; /* TODO: right buffer size */
		moo_oow_t bcslen = MOO_COUNTOF(bcs);
		moo_oow_t ucslen;

		if (moo_convootobcstr (moo, arg->name, &ucslen, bcs, &bcslen) <= -1)
		{ 
			moo_seterrnum (moo, MOO_EECERR);
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
		moo_seterrnum (moo, MOO_EIOERR);
		return -1;
	}

	bb = moo_callocmem (moo, MOO_SIZEOF(*bb));
	if (!bb)
	{
		fclose (fp);
		return -1;
	}

	bb->fp = fp;
	arg->handle = bb;
	return 0;
}

static MOO_INLINE moo_ooi_t close_input (moo_t* moo, moo_ioarg_t* arg)
{
	xtn_t* xtn = moo_getxtn(moo);
	bb_t* bb;

	bb = (bb_t*)arg->handle;
	MOO_ASSERT (moo, bb != MOO_NULL && bb->fp != MOO_NULL);

	fclose (bb->fp);
	moo_freemem (moo, bb);

	arg->handle = MOO_NULL;
	return 0;
}


static MOO_INLINE moo_ooi_t read_input (moo_t* moo, moo_ioarg_t* arg)
{
	/*xtn_t* xtn = hcl_getxtn(hcl);*/
	bb_t* bb;
	moo_oow_t bcslen, ucslen, remlen;
	int x;


	bb = (bb_t*)arg->handle;
	MOO_ASSERT (moo, bb != MOO_NULL && bb->fp != MOO_NULL);
	do
	{
		x = fgetc (bb->fp);
		if (x == EOF)
		{
			if (ferror((FILE*)bb->fp))
			{
				moo_seterrnum (moo, MOO_EIOERR);
				return -1;
			}
			break;
		}

		bb->buf[bb->len++] = x;
	}
	while (bb->len < MOO_COUNTOF(bb->buf) && x != '\r' && x != '\n');

	bcslen = bb->len;
	ucslen = MOO_COUNTOF(arg->buf);
	x = moo_convbtooochars (moo, bb->buf, &bcslen, arg->buf, &ucslen);
	x = moo_convbtooochars (moo, bb->buf, &bcslen, arg->buf, &ucslen);
	if (x <= -1 && ucslen <= 0)
	{
		moo_seterrnum (moo, MOO_EECERR);
		return -1;
	}

	remlen = bb->len - bcslen;
	if (remlen > 0) memmove (bb->buf, &bb->buf[bcslen], remlen);
	bb->len = remlen;
	return ucslen;
}

static moo_ooi_t input_handler (moo_t* moo, moo_iocmd_t cmd, moo_ioarg_t* arg)
{
	switch (cmd)
	{
		case MOO_IO_OPEN:
			return open_input (moo, arg);
			
		case MOO_IO_CLOSE:
			return close_input (moo, arg);

		case MOO_IO_READ:
			return read_input (moo, arg);

		default:
			moo->errnum = MOO_EINTERN;
			return -1;
	}
}
/* ========================================================================= */

static void* dl_open (moo_t* moo, const moo_ooch_t* name)
{
#if defined(USE_LTDL)
/* TODO: support various platforms */
	moo_bch_t buf[1024]; /* TODO: use a proper path buffer */
	moo_oow_t ucslen, bcslen;
	moo_oow_t len;
	void* handle;

/* TODO: using MODPREFIX isn't a good idea for all kind of modules.
 * OK to use it for a primitive module.
 * NOT OK to use it for a FFI target. 
 * Attempting /home/hyung-hwan/xxx/lib/libmoo-libc.so.6 followed by libc.so.6 is bad.
 * Need to accept the type or flags?
 *
 * dl_open (moo, "xxxx", MOO_MOD_EXTERNAL);
 * if external, don't use DEFAULT_MODPERFIX and MODPOSTFIX???
 */

	len = moo_copybcstr (buf, MOO_COUNTOF(buf), MOO_DEFAULT_MODPREFIX);

/* TODO: proper error checking and overflow checking */
	bcslen = MOO_COUNTOF(buf) - len;
	moo_convootobcstr (moo, name, &ucslen, &buf[len], &bcslen);

	moo_copybcstr (&buf[bcslen + len], MOO_COUNTOF(buf) - bcslen - len, MOO_DEFAULT_MODPOSTFIX);

	handle = lt_dlopenext (buf);
	if (!handle) 
	{
		buf[bcslen + len] = '\0';
		handle = lt_dlopenext (&buf[len]);
		if (handle) MOO_DEBUG2 (moo, "Opened module file %s handle %p\n", &buf[len], handle);
	}
	else
	{
		MOO_DEBUG2 (moo, "Opened module file %s handle %p\n", buf, handle);
	}

	return handle;

#else
	/* TODO: implemenent this */
	return MOO_NULL;
#endif
}

static void dl_close (moo_t* moo, void* handle)
{
	MOO_DEBUG1 (moo, "Closed module handle %p\n", handle);
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

static void* dl_getsym (moo_t* moo, void* handle, const moo_ooch_t* name)
{
#if defined(USE_LTDL)
	moo_bch_t buf[1024]; /* TODO: use a proper buffer. dynamically allocated if conversion result in too a large value */
	moo_oow_t ucslen, bcslen;
	void* sym;
	const char* symname;

	buf[0] = '_';

	bcslen = MOO_COUNTOF(buf) - 2;
	moo_convootobcstr (moo, name, &ucslen, &buf[1], &bcslen); /* TODO: error check */
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

	if (sym) MOO_DEBUG2 (moo, "Loaded module symbol %s from handle %p\n", symname, handle);
	return sym;
#else
	/* TODO: IMPLEMENT THIS */
	return MOO_NULL;
#endif
}

/* ========================================================================= */

static int write_all (int fd, const char* ptr, moo_oow_t len)
{
	while (len > 0)
	{
		moo_ooi_t wr;

		wr = write (1, ptr, len);

		if (wr <= -1)
		{
		#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN == EWOULDBLOCK)
			/* TODO: push it to internal buffers? before writing data just converted, need to write buffered data first. */
			if (errno == EAGAIN) continue;
		#else

		#if defined(EAGAIN)
			if (errno == EAGAIN) continue;
		#endif
		#if defined(EWOULDBLOCK)
			if (errno == EWOULDBLOCK) continue;
		#endif

		#endif
			return -1;
		}

		ptr += wr;
		len -= wr;
	}

	return 0;
}

static void log_write (moo_t* moo, moo_oow_t mask, const moo_ooch_t* msg, moo_oow_t len)
{
#if defined(_WIN32)
#	error NOT IMPLEMENTED 
	
#else
	moo_bch_t buf[256];
	moo_oow_t ucslen, bcslen, msgidx;
	int n;
	char ts[32];
	size_t tslen;
	struct tm tm, *tmp;
	time_t now;

if (mask & MOO_LOG_GC) return; /* don't show gc logs */

/* TODO: beautify the log message.
 *       do classification based on mask. */

	now = time(NULL);
#if defined(__MSDOS__)
	tmp = localtime (&now);
#else
	tmp = localtime_r (&now, &tm);
#endif
	tslen = strftime (ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
	if (tslen == 0) 
	{
		strcpy (ts, "0000-00-00 00:00:00 +0000");
		tslen = 25; 
	}
	write_all (1, ts, tslen);

	msgidx = 0;
	while (len > 0)
	{
		ucslen = len;
		bcslen = MOO_COUNTOF(buf);

		n = moo_convootobchars (moo, &msg[msgidx], &ucslen, buf, &bcslen);
		if (n == 0 || n == -2)
		{
			/* n = 0: 
			 *   converted all successfully 
			 * n == -2: 
			 *    buffer not sufficient. not all got converted yet.
			 *    write what have been converted this round. */

			MOO_ASSERT (moo, ucslen > 0); /* if this fails, the buffer size must be increased */

			/* attempt to write all converted characters */
			if (write_all (1, buf, bcslen) <= -1) break;

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

static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' };
static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };
static moo_t* g_moo = MOO_NULL;

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
	if (g_moo) moo_switchprocess (g_moo);
	_chain_intr(prev_timer_intr_handler);
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
#if defined(__MSDOS__) && defined(_INTELC32_)

	prev_timer_intr_handler = _dos_getvect (0x1C);
	_dos_setvect (0x1C, timer_intr_handler);

#elif defined(macintosh)

	GetCurrentProcess (&g_psn);
	memset (&g_tmtask, 0, MOO_SIZEOF(g_tmtask));
	g_tmtask.tmAddr = NewTimerProc (timer_intr_handler);
	InsXTime ((QElem*)&g_tmtask);

	PrimeTime ((QElem*)&g_tmtask, TMTASK_DELAY);

#elif defined(HAVE_SETITIMER) && defined(SIGVTALRM) && defined(ITIMER_VIRTUAL)
	struct itimerval itv;
	struct sigaction act;

	sigemptyset (&act.sa_mask);
	act.sa_handler = arrange_process_switching;
	act.sa_flags = 0;
	sigaction (SIGVTALRM, &act, MOO_NULL);

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 100; /* 100 microseconds */
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 100;
	setitimer (ITIMER_VIRTUAL, &itv, MOO_NULL);
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
	setitimer (ITIMER_VIRTUAL, &itv, MOO_NULL);

	sigemptyset (&act.sa_mask); 
	act.sa_handler = SIG_IGN; /* ignore the signal potentially fired by the one-shot arrange above */
	act.sa_flags = 0;
	sigaction (SIGVTALRM, &act, MOO_NULL);

#else
#	error UNSUPPORTED
#endif
}

/* ========================================================================= */

int main (int argc, char* argv[])
{
	moo_t* moo;
	xtn_t* xtn;
	moo_oocs_t objname;
	moo_oocs_t mthname;
	moo_vmprim_t vmprim;
	int i, xret;

#if !defined(macintosh)
	if (argc < 2)
	{
		fprintf (stderr, "Usage: %s filename ...\n", argv[0]);
		return -1;
	}
#endif

	vmprim.dl_open = dl_open;
	vmprim.dl_close = dl_close;
	vmprim.dl_getsym = dl_getsym;
	vmprim.log_write = log_write;


#if defined(USE_LTDL)
	lt_dlinit ();
#endif

	moo = moo_open (&sys_mmgr, MOO_SIZEOF(xtn_t), 2048000lu, &vmprim, MOO_NULL);
	if (!moo)
	{
		printf ("cannot open moo\n");
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
		int trait = 0;

		/*trait |= MOO_NOGC;*/
		trait |= MOO_AWAIT_PROCS;
		moo_setoption (moo, MOO_TRAIT, &trait);
	}

	if (moo_ignite(moo) <= -1)
	{
		printf ("cannot ignite moo - %d\n", moo_geterrnum(moo));
		moo_close (moo);
		return -1;
	}

	xtn = moo_getxtn (moo);

#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.st";
	goto compile;
#endif

	for (i = 1; i < argc; i++)
	{
		xtn->source_path = argv[i];

	compile:

		if (moo_compile (moo, input_handler) <= -1)
		{
			if (moo->errnum == MOO_ESYNTAX)
			{
				moo_synerr_t synerr;
				moo_bch_t bcs[1024]; /* TODO: right buffer size */
				moo_oow_t bcslen, ucslen;

				moo_getsynerr (moo, &synerr);

				printf ("ERROR: ");
				if (synerr.loc.file)
				{
					bcslen = MOO_COUNTOF(bcs);
					if (moo_convootobcstr (moo, synerr.loc.file, &ucslen, bcs, &bcslen) >= 0)
					{
						printf ("%.*s ", (int)bcslen, bcs);
					}
				}
				else
				{
					printf ("%s ", xtn->source_path);
				}


				printf ("syntax error at line %lu column %lu - ", 
					(unsigned long int)synerr.loc.line, (unsigned long int)synerr.loc.colm);

				bcslen = MOO_COUNTOF(bcs);
				if (moo_convootobcstr (moo, moo_synerrnumtoerrstr(synerr.num), &ucslen, bcs, &bcslen) >= 0)
				{
					printf (" [%.*s]", (int)bcslen, bcs);
				}

				if (synerr.tgt.len > 0)
				{
					bcslen = MOO_COUNTOF(bcs);
					ucslen = synerr.tgt.len;

					if (moo_convootobchars (moo, synerr.tgt.ptr, &ucslen, bcs, &bcslen) >= 0)
					{
						printf (" [%.*s]", (int)bcslen, bcs);
					}

				}
				printf ("\n");
			}
			else
			{
				printf ("ERROR: cannot compile code - %d\n", moo_geterrnum(moo));
			}
			moo_close (moo);
#if defined(USE_LTDL)
			lt_dlexit ();
#endif
			return -1;
		}
	}

	printf ("COMPILE OK. STARTING EXECUTION ...\n");
	xret = 0;
	g_moo = moo;
	setup_tick ();

	objname.ptr = str_my_object;
	objname.len = 8;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (moo_invoke (moo, &objname, &mthname) <= -1)
	{
		printf ("ERROR: cannot execute code - %d\n", moo_geterrnum(moo));
		xret = -1;
	}

	cancel_tick ();
	g_moo = MOO_NULL;

	/*moo_dumpsymtab(moo);
	 *moo_dumpdic(moo, moo->sysdic, "System dictionary");*/

	moo_close (moo);

#if defined(USE_LTDL)
	lt_dlexit ();
#endif

#if defined(_WIN32) && defined(_DEBUG)
getchar();
#endif
	return xret;
}
