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

#include "moo-prv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define USE_THREAD

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	if defined(MOO_HAVE_CFG_H) && defined(MOO_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#	endif
#elif defined(__OS2__)
#	define INCL_DOSMODULEMGR
#	define INCL_DOSPROCESS
#	define INCL_DOSMISC
#	define INCL_DOSDATETIME
#	define INCL_DOSERRORS
#	include <os2.h>
#	include <time.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <time.h>
#	include <io.h>
#elif defined(macintosh)
#	include <Types.h>
#	include <OSUtils.h>
#	include <Timer.h>
#else

#	if defined(MOO_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#	elif defined(HAVE_DLFCN_H)
#		include <dlfcn.h>
#		error NOT IMPLEMENTED
#	else
#		error UNSUPPORTED DYNAMIC LINKER
#	endif

#	if defined(HAVE_TIME_H)
#		include <time.h>
#	endif
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#	if defined(HAVE_SIGNAL_H)
#		include <signal.h>
#	endif

#	include <unistd.h>
#	include <fcntl.h>

#	if defined(USE_THREAD)
#		include <pthread.h>
#		include <sched.h>
#	endif

#	if defined(HAVE_SYS_DEVPOLL_H)
		/* solaris */
#		include <sys/devpoll.h>
#		define USE_DEVPOLL
#		define XPOLLIN POLLIN
#		define XPOLLOUT POLLOUT
#		define XPOLLERR POLLERR
#		define XPOLLHUP POLLHUP
#	elif defined(HAVE_SYS_EPOLL_H)
		/* linux */
#		include <sys/epoll.h>
#		define USE_EPOLL
#		define XPOLLIN EPOLLIN
#		define XPOLLOUT EPOLLOUT
#		define XPOLLERR EPOLLERR
#		define XPOLLHUP EPOLLHUP
#	elif defined(HAVE_POLL_H)
#		include <poll.h>
#		define USE_POLL
#		define XPOLLIN POLLIN
#		define XPOLLOUT POLLOUT
#		define XPOLLERR POLLERR
#		define XPOLLHUP POLLHUP
#	else
#		error UNSUPPORTED MULTIPLEXER
#	endif

#endif

#if !defined(MOO_DEFAULT_PFMODPREFIX)
#	if defined(_WIN32)
#		define MOO_DEFAULT_PFMODPREFIX "moo-"
#	elif defined(__OS2__)
#		define MOO_DEFAULT_PFMODPREFIX "moo"
#	elif defined(__DOS__)
#		define MOO_DEFAULT_PFMODPREFIX "moo"
#	else
#		define MOO_DEFAULT_PFMODPREFIX "libmoo-"
#	endif
#endif

#if !defined(MOO_DEFAULT_PFMODPOSTFIX)
#	if defined(_WIN32)
#		define MOO_DEFAULT_PFMODPOSTFIX ""
#	elif defined(__OS2__)
#		define MOO_DEFAULT_PFMODPOSTFIX ""
#	elif defined(__DOS__)
#		define MOO_DEFAULT_PFMODPOSTFIX ""
#	else
#		define MOO_DEFAULT_PFMODPOSTFIX ""
#	endif
#endif

typedef struct bb_t bb_t;
struct bb_t
{
	char buf[1024];
	moo_oow_t pos;
	moo_oow_t len;

	FILE* fp;
	moo_bch_t* fn;
};

typedef struct xtn_t xtn_t;
struct xtn_t
{
	const char* source_path; /* main source file */
#if defined(_WIN32)
	HANDLE waitable_timer;
#else

	int ep; /* /dev/poll or epoll */
#if defined(USE_DEVPOLL)
	struct
	{
		moo_oow_t capa;
		moo_ooi_t* ptr;
	} epd;
#endif

#if defined(USE_THREAD)
	int p[2]; /* pipe for signaling */
	pthread_t iothr;
	int iothr_up;
	int iothr_abort;
	struct
	{
	#if defined(USE_DEVPOLL)
		struct pollfd buf[32];
	#else
		struct epoll_event buf[32]; /*TODO: make it a dynamically allocated memory block depending on the file descriptor added. */
	#endif
		moo_oow_t len;
		pthread_mutex_t mtx;
		pthread_cond_t cnd;
		pthread_cond_t cnd2;
	} ev;
#else
	struct
	{
	#if defined(USE_DEVPOLL)
		struct pollfd buf[32];
	#else
		struct epoll_event buf[32]; /*TODO: make it a dynamically allocated memory block depending on the file descriptor added. */
	#endif
		moo_oow_t len;
	} ev;
#endif

#endif
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
	bb_t* bb = MOO_NULL;

/* TOOD: support predefined include directory as well */
	if (arg->includer)
	{
		/* includee */
		moo_oow_t ucslen, bcslen, parlen;
		const moo_bch_t* fn, * fb;

	#if defined(MOO_OOCH_IS_UCH)
		if (moo_convootobcstr (moo, arg->name, &ucslen, MOO_NULL, &bcslen) <= -1) goto oops;
	#else
		bcslen = moo_countbcstr (arg->name);
	#endif

		fn = ((bb_t*)arg->includer->handle)->fn;

		fb = get_base_name (fn);
		parlen = fb - fn;

		bb = moo_callocmem (moo, MOO_SIZEOF(*bb) + (MOO_SIZEOF(moo_bch_t) * (parlen + bcslen + 1)));
		if (!bb) goto oops;

		bb->fn = (moo_bch_t*)(bb + 1);
		moo_copybchars (bb->fn, fn, parlen);
	#if defined(MOO_OOCH_IS_UCH)
		moo_convootobcstr (moo, arg->name, &ucslen, &bb->fn[parlen], &bcslen);
	#else
		moo_copybcstr (&bb->fn[parlen], bcslen + 1, arg->name);
	#endif
	}
	else
	{
		/* main stream */
		moo_oow_t pathlen;

		pathlen = moo_countbcstr (xtn->source_path);

		bb = moo_callocmem (moo, MOO_SIZEOF(*bb) + (MOO_SIZEOF(moo_bch_t) * (pathlen + 1)));
		if (!bb) goto oops;

		bb->fn = (moo_bch_t*)(bb + 1);
		moo_copybcstr (bb->fn, pathlen + 1, xtn->source_path);
	}

#if defined(__DOS__) || defined(_WIN32) || defined(__OS2__)
	bb->fp = fopen (bb->fn, "rb");
#else
	bb->fp = fopen (bb->fn, "r");
#endif
	if (!bb->fp)
	{
		moo_seterrnum (moo, MOO_EIOERR);
		goto oops;
	}

	arg->handle = bb;
	return 0;

oops:
	if (bb) 
	{
		if (bb->fp) fclose (bb->fp);
		moo_freemem (moo, bb);
	}
	return -1;
}

static MOO_INLINE moo_ooi_t close_input (moo_t* moo, moo_ioarg_t* arg)
{
	/*xtn_t* xtn = moo_getxtn(moo);*/
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

#if defined(MOO_OOCH_IS_UCH)
	bcslen = bb->len;
	ucslen = MOO_COUNTOF(arg->buf);
	x = moo_convbtooochars (moo, bb->buf, &bcslen, arg->buf, &ucslen);
	if (x <= -1 && ucslen <= 0) return -1;
	/* if ucslen is greater than 0, i see that some characters have been
	 * converted properly */
#else
	bcslen = (bb->len < MOO_COUNTOF(arg->buf))? bb->len: MOO_COUNTOF(arg->buf);
	ucslen = bcslen;
	moo_copybchars (arg->buf, bb->buf, bcslen);
#endif

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

static void* dl_open (moo_t* moo, const moo_ooch_t* name, int flags)
{
#if defined(USE_LTDL)
	moo_bch_t stabuf[128], * bufptr;
	moo_oow_t ucslen, bcslen, bufcapa;
	void* handle;

	#if defined(MOO_OOCH_IS_UCH)
	if (moo_convootobcstr (moo, name, &ucslen, MOO_NULL, &bufcapa) <= -1) return MOO_NULL;
	/* +1 for terminating null. but it's not needed because MOO_COUNTOF(MOO_DEFAULT_PFMODPREFIX)
	 * and MOO_COUNTOF(MOO_DEFAULT_PFMODPOSTIFX) include the terminating nulls. Never mind about
	 * the extra 2 characters. */
	#else
	bufcapa = moo_countbcstr (name);
	#endif
	bufcapa += MOO_COUNTOF(MOO_DEFAULT_PFMODPREFIX) + MOO_COUNTOF(MOO_DEFAULT_PFMODPOSTFIX) + 1; 

	if (bufcapa <= MOO_COUNTOF(stabuf)) bufptr = stabuf;
	else
	{
		bufptr = moo_allocmem (moo, bufcapa * MOO_SIZEOF(*bufptr));
		if (!bufptr) return MOO_NULL;
	}

	if (flags & MOO_VMPRIM_OPENDL_PFMOD)
	{
		moo_oow_t len, i, xlen;

		/* opening a primitive function module - mostly libmoo-xxxx */
		len = moo_copybcstr (bufptr, bufcapa, MOO_DEFAULT_PFMODPREFIX);

		bcslen = bufcapa - len;
	#if defined(MOO_OOCH_IS_UCH)
		moo_convootobcstr (moo, name, &ucslen, &bufptr[len], &bcslen);
	#else
		bcslen = moo_copybcstr (&bufptr[len], bcslen, name);
	#endif

		/* length including the prefix and the name. but excluding the postfix */
		xlen  = len + bcslen; 

		for (i = len; i < xlen; i++) 
		{
			/* convert a period(.) to a dash(-) */
			if (bufptr[i] == '.') bufptr[i] = '-';
		}
 
	retry:
		moo_copybcstr (&bufptr[xlen], bufcapa - xlen, MOO_DEFAULT_PFMODPOSTFIX);

		/* both prefix and postfix attached. for instance, libmoo-xxx */
		handle = lt_dlopenext (bufptr);
		if (!handle) 
		{
			MOO_DEBUG3 (moo, "Failed to open(ext) DL %hs[%js] - %hs\n", bufptr, name, lt_dlerror());

			/* try without prefix and postfix */
			bufptr[xlen] = '\0';
			handle = lt_dlopenext (&bufptr[len]);
			if (!handle) 
			{
				moo_bch_t* dash;
				MOO_DEBUG3 (moo, "Failed to open(ext) DL %hs[%js] - %s\n", &bufptr[len], name, lt_dlerror());
				dash = moo_rfindbchar (bufptr, moo_countbcstr(bufptr), '-');
				if (dash) 
				{
					/* remove a segment at the back. 
					 * [NOTE] a dash contained in the original name before
					 *        period-to-dash transformation may cause extraneous/wrong
					 *        loading reattempts. */
					xlen = dash - bufptr;
					goto retry;
				}
			}
			else MOO_DEBUG3 (moo, "Opened(ext) DL %hs[%js] handle %p\n", &bufptr[len], name, handle);
		}
		else
		{
			MOO_DEBUG3 (moo, "Opened(ext) DL %hs[%js] handle %p\n", bufptr, name, handle);
		}
	}
	else
	{
		/* opening a raw shared object without a prefix and/or a postfix */
	#if defined(MOO_OOCH_IS_UCH)
		bcslen = bufcapa;
		moo_convootobcstr (moo, name, &ucslen, bufptr, &bcslen);
	#else
		bcslen = moo_copybcstr (bufptr, bufcapa, name);
	#endif

		if (moo_findbchar (bufptr, bcslen, '.'))
		{
			handle = lt_dlopen (bufptr);
			if (!handle) MOO_DEBUG2 (moo, "Failed to open DL %hs - %s\n", bufptr, lt_dlerror());
			else MOO_DEBUG2 (moo, "Opened DL %hs handle %p\n", bufptr, handle);
		}
		else
		{
			handle = lt_dlopenext (bufptr);
			if (!handle) MOO_DEBUG2 (moo, "Failed to open(ext) DL %hs - %s\n", bufptr, lt_dlerror());
			else MOO_DEBUG2 (moo, "Opened(ext) DL %hs handle %p\n", bufptr, handle);
		}
	}

	if (bufptr != stabuf) moo_freemem (moo, bufptr);
	return handle;

#else

/* TODO: support various platforms */
	/* TODO: implemenent this */
	MOO_DEBUG1 (moo, "Dynamic loading not implemented - cannot open %js\n", name);
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_NULL;
#endif
}

static void dl_close (moo_t* moo, void* handle)
{
#if defined(USE_LTDL)
	MOO_DEBUG1 (moo, "Closed DL handle %p\n", handle);
	lt_dlclose (handle);

#else
	/* TODO: implemenent this */
	MOO_DEBUG1 (moo, "Dynamic loading not implemented - cannot close handle %p\n", handle);
#endif
}

static void* dl_getsym (moo_t* moo, void* handle, const moo_ooch_t* name)
{
#if defined(USE_LTDL)
	moo_bch_t stabuf[64], * bufptr;
	moo_oow_t bufcapa, ucslen, bcslen, i;
	const moo_bch_t* symname;
	void* sym;

	#if defined(MOO_OOCH_IS_UCH)
	if (moo_convootobcstr (moo, name, &ucslen, MOO_NULL, &bcslen) <= -1) return MOO_NULL;
	#else
	bcslen = moo_countbcstr (name);
	#endif

	if (bcslen >= MOO_COUNTOF(stabuf) - 2)
	{
		bufcapa = bcslen + 3;
		bufptr = moo_allocmem (moo, bufcapa * MOO_SIZEOF(*bufptr));
		if (!bufptr) return MOO_NULL;
	}
	else
	{
		bufcapa = MOO_COUNTOF(stabuf);
		bufptr = stabuf;
	}

	bcslen = bufcapa - 1;
	#if defined(MOO_OOCH_IS_UCH)
	moo_convootobcstr (moo, name, &ucslen, &bufptr[1], &bcslen);
	#else
	bcslen = moo_copybcstr (&bufptr[1], bcslen, name);
	#endif

	/* convert a period(.) to an underscore(_) */
	for (i = 1; i <= bcslen; i++) if (bufptr[i] == '.') bufptr[i] = '_';

	symname = &bufptr[1]; /* try the name as it is */
	sym = lt_dlsym (handle, symname);
	if (!sym)
	{
		bufptr[0] = '_';
		symname = &bufptr[0]; /* try _name */
		sym = lt_dlsym (handle, symname);
		if (!sym)
		{
			bufptr[bcslen + 1] = '_'; 
			bufptr[bcslen + 2] = '\0';

			symname = &bufptr[1]; /* try name_ */
			sym = lt_dlsym (handle, symname);
			if (!sym)
			{
				symname = &bufptr[0]; /* try _name_ */
				sym = lt_dlsym (handle, symname);
			}
		}
	}

	if (sym) MOO_DEBUG3 (moo, "Loaded module symbol %js from handle %p - %hs\n", name, handle, symname);
	if (bufptr != stabuf) moo_freemem (moo, bufptr);
	return sym;
#else
	/* TODO: IMPLEMENT THIS */
	MOO_DEBUG2 (moo, "Dynamic loading not implemented - Cannot load module symbol %js from handle %p\n", name, handle);
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_NULL;
#endif
}

/* ========================================================================= */

#if defined(_WIN32)
	/* nothing to do */

#elif defined(macintosh)
	/* nothing to do */

#else
static int write_all (int fd, const char* ptr, moo_oow_t len)
{
	while (len > 0)
	{
		moo_ooi_t wr;

		wr = write (fd, ptr, len);

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
#endif

static void log_write (moo_t* moo, moo_oow_t mask, const moo_ooch_t* msg, moo_oow_t len)
{
#if defined(_WIN32)
#	error NOT IMPLEMENTED 
	
#elif defined(macintosh)
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
#if defined(__DOS__)
	tmp = localtime (&now);
	tslen = strftime (ts, sizeof(ts), "%Y-%m-%d %H:%M:%S ", tmp); /* no timezone info */
	if (tslen == 0) 
	{
		strcpy (ts, "0000-00-00 00:00:00");
		tslen = 19; 
	}
#else
	tmp = localtime_r (&now, &tm);
	#if defined(HAVE_STRFTIME_SMALL_Z)
	tslen = strftime (ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
	#else
	tslen = strftime (ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %Z ", tmp); 
	#endif
	if (tslen == 0) 
	{
		strcpy (ts, "0000-00-00 00:00:00 +0000");
		tslen = 25; 
	}
#endif
	write_all (1, ts, tslen);

#if defined(MOO_OOCH_IS_UCH)
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
#else
	write_all (1, msg, len);
#endif

#endif
}


/* ========================================================================= */

#if defined(USE_THREAD)
static void* iothr_main (void* arg)
{
	moo_t* moo = (moo_t*)arg;
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

	/*while (!moo->abort_req)*/
	while (!xtn->iothr_abort)
	{
		if (xtn->ev.len <= 0) /* TODO: no mutex needed for this check? */
		{
			int n;
		#if defined(USE_DEVPOLL)
			struct dvpoll dvp;
		#endif

		poll_for_event:
		
		#if defined(USE_DEVPOLL)
			dvp.dp_timeout = 10000; /* milliseconds */
			dvp.dp_fds = xtn->ev.buf;
			dvp.dp_nfds = MOO_COUNTOF(xtn->ev.buf);
			n = ioctl (xtn->ep, DP_POLL, &dvp);
		#else
			n = epoll_wait (xtn->ep, xtn->ev.buf, MOO_COUNTOF(xtn->ev.buf), 10000);
		#endif

			pthread_mutex_lock (&xtn->ev.mtx);
			if (n <= -1)
			{
				/* TODO: don't use MOO_DEBUG2. it's not thread safe... */
				MOO_DEBUG2 (moo, "Warning: epoll_wait failure - %d, %hs\n", errno, strerror(errno));
			}
			else if (n > 0)
			{
				xtn->ev.len = n;
			}
			pthread_cond_signal (&xtn->ev.cnd2);
			pthread_mutex_unlock (&xtn->ev.mtx);
		}
		else
		{
			/* the event buffer has not been emptied yet */
			struct timespec ts;

			pthread_mutex_lock (&xtn->ev.mtx);
			if (xtn->ev.len <= 0)
			{
				/* it got emptied between the if check and pthread_mutex_lock() above */
				pthread_mutex_unlock (&xtn->ev.mtx);
				goto poll_for_event;
			}

			clock_gettime (CLOCK_REALTIME, &ts);
			ts.tv_sec += 10;
			pthread_cond_timedwait (&xtn->ev.cnd, &xtn->ev.mtx, &ts);
			pthread_mutex_unlock (&xtn->ev.mtx);
		}

		/*sched_yield ();*/
	}

	return MOO_NULL;
}
#endif

static int vm_startup (moo_t* moo)
{
#if defined(_WIN32)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	xtn->waitable_timer = CreateWaitableTimer(MOO_NULL, TRUE, MOO_NULL);

#else
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
#if defined(USE_DEVPOLL)
	struct pollfd ev;
#else
	struct epoll_event ev;
#endif /* USE_DEVPOLL */
	int pcount = 0, flag;


#if defined(USE_DEVPOLL)
	xtn->ep = open ("/dev/poll", O_RDWR);
	if (xtn->ep == -1) 
	{
		moo_syserrtoerrnum (errno);
		MOO_DEBUG1 (moo, "Cannot create devpoll - %hs\n", strerror(errno));
		goto oops;
	}
#else
	#if defined(EPOLL_CLOEXEC)
	xtn->ep = epoll_create1 (EPOLL_CLOEXEC);
	#else
	xtn->ep = epoll_create (1024);
	#endif
	if (xtn->ep == -1) 
	{
		moo_syserrtoerrnum (errno);
		MOO_DEBUG1 (moo, "Cannot create epoll - %hs\n", strerror(errno));
		goto oops;
	}
#endif /* USE_DEVPOLL */


#if defined(EPOLL_CLOEXEC)
	/* do nothing */
#else
	flag = fcntl (xtn->ep, F_GETFD);
	if (flag >= 0) fcntl (xtn->ep, F_SETFD, flag | FD_CLOEXEC);
#endif

#if defined(USE_THREAD)
	if (pipe (xtn->p) == -1)
	{
		moo_syserrtoerrnum (errno);
		MOO_DEBUG1 (moo, "Cannot create pipes - %hs\n", strerror(errno));
		goto oops;
	}
	pcount = 2;

#if defined(O_CLOEXEC)
	flag = fcntl (xtn->p[0], F_GETFD);
	if (flag >= 0) fcntl (xtn->p[0], F_SETFD, flag | FD_CLOEXEC);
	flag = fcntl (xtn->p[1], F_GETFD);
	if (flag >= 0) fcntl (xtn->p[1], F_SETFD, flag | FD_CLOEXEC);
#endif
#if defined(O_NONBLOCK)
	flag = fcntl (xtn->p[0], F_GETFL);
	if (flag >= 0) fcntl (xtn->p[0], F_SETFL, flag | O_NONBLOCK);
	flag = fcntl (xtn->p[1], F_GETFL);
	if (flag >= 0) fcntl (xtn->p[1], F_SETFL, flag | O_NONBLOCK);
#endif

#if defined(USE_DEVPOLL)
	ev.fd = xtn->p[0];
	ev.events = XPOLLIN;
	ev.revents = 0;
	if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
	{
		moo_syserrtoerrnum (errno);
		MOO_DEBUG1 (moo, "Cannot add a pipe to devpoll - %hs\n", strerror(errno));
		goto oops;
	}
#else
	ev.events = XPOLLIN;
	ev.data.ptr = (void*)MOO_TYPE_MAX(moo_oow_t);
	if (epoll_ctl (xtn->ep, EPOLL_CTL_ADD, xtn->p[0], &ev) == -1)
	{
		moo_syserrtoerrnum (errno);
		MOO_DEBUG1 (moo, "Cannot add a pipe to epoll - %hs\n", strerror(errno));
		goto oops;
	}
#endif

	pthread_mutex_init (&xtn->ev.mtx, MOO_NULL);
	pthread_cond_init (&xtn->ev.cnd, MOO_NULL);
	pthread_cond_init (&xtn->ev.cnd2, MOO_NULL);

	xtn->iothr_abort = 0;
	xtn->iothr_up = 0;
	/*pthread_create (&xtn->iothr, MOO_NULL, iothr_main, moo);*/
#endif

	return 0;

oops:

#if defined(USE_THREAD)
	if (pcount)
	{
		close (xtn->p[0]);
		close (xtn->p[1]);
	}
#endif
	if (xtn->ep >= 0)
	{
		close (xtn->ep);
		xtn->ep = -1;
	}

	return -1;
#endif
}

static void vm_cleanup (moo_t* moo)
{
#if defined(_WIN32)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	if (xtn->waitable_timer)
	{
		CloseHandle (xtn->waitable_timer);
		xtn->waitable_timer = MOO_NULL;
	}
#else
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);


#if defined(USE_THREAD)
	if (xtn->iothr_up)
	{
		xtn->iothr_abort = 1;
		write (xtn->p[1], "Q", 1);
		pthread_cond_signal (&xtn->ev.cnd);
		pthread_join (xtn->iothr, MOO_NULL);
		xtn->iothr_up = 0;
	}
	pthread_cond_destroy (&xtn->ev.cnd);
	pthread_cond_destroy (&xtn->ev.cnd2);
	pthread_mutex_destroy (&xtn->ev.mtx);
#endif

	if (xtn->ep)
	{
	#if defined(USE_THREAD)
	#if defined(USE_DEVPOLL)
		struct pollfd ev;
		ev.fd = xtn->p[1];
		ev.events = POLLREMOVE;
		ev.revents = 0;
		write (xtn->ep, &ev, MOO_SIZEOF(ev));
	#else
		struct epoll_event ev;
		epoll_ctl (xtn->ep, EPOLL_CTL_DEL, xtn->p[1], &ev);
	#endif

		close (xtn->p[1]);
		close (xtn->p[0]);
	#endif /* USE_THREAD */

		close (xtn->ep);
		xtn->ep = -1;
	}

	#if defined(USE_DEVPOLL)
	if (xtn->epd.ptr)
	{
		moo_freemem (moo, xtn->epd.ptr);
		xtn->epd.ptr = MOO_NULL;
		xtn->epd.capa = 0;
	}
	#endif

#endif
}

static void vm_gettime (moo_t* moo, moo_ntime_t* now)
{
#if defined(_WIN32)
	/* TODO: */
#elif defined(__OS2__)
	ULONG out;

/* TODO: handle overflow?? */
/* TODO: use DosTmrQueryTime() and DosTmrQueryFreq()? */
	DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, &out, MOO_SIZEOF(out)); /* milliseconds */
	/* it must return NO_ERROR */
	MOO_INITNTIME (now, MOO_MSEC_TO_SEC(out), MOO_MSEC_TO_NSEC(out));
#elif defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))
	clock_t c;

/* TODO: handle overflow?? */
	c = clock ();
	now->sec = c / CLOCKS_PER_SEC;
	#if (CLOCKS_PER_SEC == 100)
		now->nsec = MOO_MSEC_TO_NSEC((c % CLOCKS_PER_SEC) * 10);
	#elif (CLOCKS_PER_SEC == 1000)
		now->nsec = MOO_MSEC_TO_NSEC(c % CLOCKS_PER_SEC);
	#elif (CLOCKS_PER_SEC == 1000000L)
		now->nsec = MOO_USEC_TO_NSEC(c % CLOCKS_PER_SEC);
	#elif (CLOCKS_PER_SEC == 1000000000L)
		now->nsec = (c % CLOCKS_PER_SEC);
	#else
	#	error UNSUPPORTED CLOCKS_PER_SEC
	#endif
#elif defined(macintosh)
	UnsignedWide tick;
	moo_uint64_t tick64;
	Microseconds (&tick);
	tick64 = *(moo_uint64_t*)&tick;
	MOO_INITNTIME (now, MOO_USEC_TO_SEC(tick64), MOO_USEC_TO_NSEC(tick64));
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	struct timespec ts;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	MOO_INITNTIME(now, ts.tv_sec, ts.tv_nsec);
#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime (CLOCK_REALTIME, &ts);
	MOO_INITNTIME(now, ts.tv_sec, ts.tv_nsec);
#else
	struct timeval tv;
	gettimeofday (&tv, MOO_NULL);
	MOO_INITNTIME(now, tv.tv_sec, MOO_USEC_TO_NSEC(tv.tv_usec));
#endif
}

#if defined(__DOS__) 
#	if defined(_INTELC32_) 
	void _halt_cpu (void);
#	elif defined(__WATCOMC__)
	void _halt_cpu (void);
#	pragma aux _halt_cpu = "hlt"
#	endif
#endif

#if defined(USE_DEVPOLL)
static int secure_devpoll_data_space (moo_t* moo, int fd)
{
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	if (fd >= xtn->epd.capa)
	{
		moo_oow_t newcapa;
		moo_ooi_t* tmp;

		newcapa = MOO_ALIGN_POW2 (fd + 1, 256);
		tmp = moo_reallocmem (moo, xtn->epd.ptr, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		xtn->epd.capa = newcapa;
		xtn->epd.ptr = tmp;
	}

	return 0;
}
#endif

static int _mux_add_or_mod (moo_t* moo, moo_oop_semaphore_t sem, int cmd)
{
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
#if defined(USE_DEVPOLL)
	struct pollfd ev;
#else
	struct epoll_event ev;
#endif
	moo_ooi_t mask;

	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_handle));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_mask));

	mask = MOO_OOP_TO_SMOOI(sem->io_mask);
	ev.events = 0; /*EPOLLET; */ /* TODO: use edge trigger(EPOLLLET)? */
	if (mask & MOO_SEMAPHORE_IO_MASK_INPUT) ev.events |= XPOLLIN; /* TODO: define io mask constants... */
	if (mask & MOO_SEMAPHORE_IO_MASK_OUTPUT) ev.events |= XPOLLOUT;

	if (ev.events == 0) 
	{
		MOO_DEBUG2 (moo, "<vm_muxadd> Invalid semaphore mask %zd on handle %zd\n", mask, MOO_OOP_TO_SMOOI(sem->io_handle));
		moo_seterrnum (moo, MOO_EINVAL);
		return -1;
	}

#if defined(USE_DEVPOLL)
	ev.fd = MOO_OOP_TO_SMOOI(sem->io_handle);
	ev.revents = 0;

	if (secure_devpoll_data_space (moo, ev.fd) <= -1)
	{
			MOO_DEBUG2 (moo, "<vm_muxadd> devpoll data set failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
			return -1;
	}

	if (cmd)
	{
		int saved_events;

		saved_events = ev.events;

		ev.events = POLLREMOVE;
		if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
		{
			moo_seterrnum (moo, moo_syserrtoerrnum (errno));
			MOO_DEBUG2 (moo, "<vm_muxadd> devpoll failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
			return -1;
		}

		ev.events = saved_events;
	}

	if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
	{
		moo_seterrnum (moo, moo_syserrtoerrnum (errno));
		MOO_DEBUG2 (moo, "<vm_muxadd> devpoll failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
		return -1;
	}

	MOO_ASSERT (moo, ev.fd == MOO_OOP_TO_SMOOI(sem->io_handle));
	MOO_ASSERT (moo, xtn->epd.capa > ev.fd);
	xtn->epd.ptr[ev.fd] = MOO_OOP_TO_SMOOI(sem->io_index);
#else
	/* don't check MOO_SEMAPHORE_IO_MASK_ERROR and MOO_SEMAPHORE_IO_MASK_HANGUP as it's implicitly enabled by epoll() */
	ev.data.ptr = (void*)MOO_OOP_TO_SMOOI(sem->io_index);

	if (epoll_ctl (xtn->ep, cmd, MOO_OOP_TO_SMOOI(sem->io_handle), &ev) == -1)
	{
		moo_seterrnum (moo, moo_syserrtoerrnum (errno));
		MOO_DEBUG2 (moo, "<vm_muxadd> epoll_ctl failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
		return -1;
	}
#endif

	return 0;
}

static int vm_muxadd (moo_t* moo, moo_oop_semaphore_t sem)
{
#if defined(USE_DEVPOLL)
	return _mux_add_or_mod (moo, sem, 0);
#else
	return _mux_add_or_mod (moo, sem, EPOLL_CTL_ADD);
#endif
}

static int vm_muxmod (moo_t* moo, moo_oop_semaphore_t sem)
{
#if defined(USE_DEVPOLL)
	return _mux_add_or_mod (moo, sem, 1);
#else
	return _mux_add_or_mod (moo, sem, EPOLL_CTL_MOD);
#endif
}

static int vm_muxdel (moo_t* moo, moo_oop_semaphore_t sem)
{
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
#if defined(USE_DEVPOLL)
	struct pollfd ev;
#else
	struct epoll_event ev;
#endif

	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_handle));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_mask));

#if defined(USE_DEVPOLL)
	ev.fd = MOO_OOP_TO_SMOOI(sem->io_handle);
	ev.events = POLLREMOVE;
	ev.revents = 0;
	if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
	{
		moo_seterrnum (moo, moo_syserrtoerrnum (errno));
		MOO_DEBUG2 (moo, "<vm_muxdel> devpoll failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
		return -1;
	}
#else
	if (epoll_ctl (xtn->ep, EPOLL_CTL_DEL, MOO_OOP_TO_SMOOI(sem->io_handle), &ev) == -1)
	{
		moo_seterrnum (moo, moo_syserrtoerrnum (errno));
		MOO_DEBUG2 (moo, "<vm_muxdel> epoll_ctl failure on handle %zd - %hs\n", MOO_OOP_TO_SMOOI(sem->io_handle), strerror(errno));
		return -1;
	}
#endif

	return 0;
}

static void vm_muxwait (moo_t* moo, const moo_ntime_t* dur, moo_vmprim_muxwait_cb_t muxwcb)
{
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

#if defined(USE_THREAD)
	int n;

	/* create a thread if mux wait is started at least once. */
	if (!xtn->iothr_up) 
	{
		xtn->iothr_up = 1;
		if (pthread_create (&xtn->iothr, MOO_NULL, iothr_main, moo) != 0)
		{
			MOO_LOG2 (moo, MOO_LOG_WARN, "Warning: pthread_create failure - %d, %hs\n", errno, strerror(errno));
			xtn->iothr_up = 0;
/* TODO: switch to the non-threaded mode? */
		}
	}

	if (xtn->ev.len <= 0) 
	{
		struct timespec ts;
		moo_ntime_t ns;

		if (!dur) return; /* immediate check is requested. and there is no event */

		clock_gettime (CLOCK_REALTIME, &ts);
		ns.sec = ts.tv_sec;
		ns.nsec = ts.tv_nsec;
		MOO_ADDNTIME (&ns, &ns, dur);
		ts.tv_sec = ns.sec;
		ts.tv_nsec = ns.nsec;

		pthread_mutex_lock (&xtn->ev.mtx);
		if (xtn->ev.len <= 0)
		{
			/* the event buffer is still empty */
			pthread_cond_timedwait (&xtn->ev.cnd2, &xtn->ev.mtx, &ts);
		}
		pthread_mutex_unlock (&xtn->ev.mtx);
	}

	n = xtn->ev.len;

	if (n > 0)
	{
		do
		{
			--n;

		#if defined(USE_DEVPOLL)
			if (xtn->ev.buf[n].fd == xtn->p[0])
		#else
			if (xtn->ev.buf[n].data.ptr == (void*)MOO_TYPE_MAX(moo_oow_t))
		#endif
			{
				moo_uint8_t u8;
				while (read (xtn->p[0], &u8, MOO_SIZEOF(u8)) > 0) 
				{
					/* consume as much as possible */;
					if (u8 == 'Q') xtn->iothr_abort = 1;
				}
			}
			else if (muxwcb)
			{
				int revents;
				moo_ooi_t mask;

		#if defined(USE_DEVPOLL)
				revents = xtn->ev.buf[n].revents;
		#else
				revents = xtn->ev.buf[n].events;
		#endif

				mask = 0;
				if (revents & XPOLLIN) mask |= MOO_SEMAPHORE_IO_MASK_INPUT;
				if (revents & XPOLLOUT) mask |= MOO_SEMAPHORE_IO_MASK_OUTPUT;
				if (revents & XPOLLERR) mask |= MOO_SEMAPHORE_IO_MASK_ERROR;
				if (revents & XPOLLHUP) mask |= MOO_SEMAPHORE_IO_MASK_HANGUP;

		#if defined(USE_DEVPOLL)
				MOO_ASSERT (moo, xtn->epd.capa > xtn->ev.buf[n].fd);
				muxwcb (moo, mask, (void*)xtn->epd.ptr[xtn->ev.buf[n].fd]);
		#else
				muxwcb (moo, mask, xtn->ev.buf[n].data.ptr);
		#endif
			}
		}
		while (n > 0);

		pthread_mutex_lock (&xtn->ev.mtx);
		xtn->ev.len = 0;
		pthread_cond_signal (&xtn->ev.cnd);
		pthread_mutex_unlock (&xtn->ev.mtx);
	}

#else
	int tmout = 0, n;

	if (dur) tmout = MOO_SECNSEC_TO_MSEC(dur->sec, dur->nsec);

	n = epoll_wait (xtn->ep, xtn->ev.buf, MOO_COUNTOF(xtn->ev.buf), tmout);

	if (n <= -1)
	{
		MOO_DEBUG2 (moo, "Warning: epoll_wait failure - %d, %hs\n", errno, strerror(errno));
	}
	else
	{
		xtn->ev.len = n;
	}

	/* the muxwcb must be valid all the time in a non-threaded mode */
	MOO_ASSERT (moo, muxwcb != MOO_NULL);

	while (n > 0)
	{
		int revents;
		moo_ooi_t mask;

		--n;

	#if defined(USE_DEVPOLL)
		revents = xtn->ev.buf[n].revents;
	#else
		revetns = xtn->ev.buf[n].events;
	#endif

		mask = 0;
		if (revents & XPOLLIN) mask |= MOO_SEMAPHORE_IO_MASK_INPUT; /* TODO define constants for IO Mask */
		if (revents & XPOLLOUT) mask |= MOO_SEMAPHORE_IO_MASK_OUTPUT;
		if (revents & XPOLLERR) mask |= MOO_SEMAPHORE_IO_MASK_ERROR;
		if (revents & XPOLLHUP) mask |= MOO_SEMAPHORE_IO_MASK_HANGUP;

	#if defined(USE_DEVPOLL)
		MOO_ASSERT (moo, xtn->epd.capa > xtn->ev.buf[n].fd);
		muxwcb (moo, mask, (void*)xtn->epd.ptr[xtn->ev.buf[n].fd]);
	#else
		muxwcb (moo, mask, xtn->ev.buf[n].data.ptr);
	#endif
	}

	xtn->ev.len = 0;
#endif
}

static void vm_sleep (moo_t* moo, const moo_ntime_t* dur)
{
#if defined(_WIN32)
	xtn_t* xtn = moo_getxtn(moo);
	if (xtn->waitable_timer)
	{
		LARGE_INTEGER li;
		li.QuadPart = -MOO_SECNSEC_TO_NSEC(dur->sec, dur->nsec);
		if(SetWaitableTimer(timer, &li, 0, MOO_NULL, MOO_NULL, FALSE) == FALSE) goto normal_sleep;
		WaitForSingleObject(timer, INFINITE);
	}
	else
	{
	normal_sleep:
		/* fallback to normal Sleep() */
		Sleep (MOO_SECNSEC_TO_MSEC(dur->sec,dur->nsec));
	}
#elif defined(__OS2__)

	/* TODO: in gui mode, this is not a desirable method??? 
	 *       this must be made event-driven coupled with the main event loop */
	DosSleep (MOO_SECNSEC_TO_MSEC(dur->sec,dur->nsec));

#elif defined(macintosh)

	/* TODO: ... */

#elif defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))

	clock_t c;

	c = clock ();
	c += dur->sec * CLOCKS_PER_SEC;

	#if (CLOCKS_PER_SEC == 100)
		c += MOO_NSEC_TO_MSEC(dur->nsec) / 10;
	#elif (CLOCKS_PER_SEC == 1000)
		c += MOO_NSEC_TO_MSEC(dur->nsec);
	#elif (CLOCKS_PER_SEC == 1000000L)
		c += MOO_NSEC_TO_USEC(dur->nsec);
	#elif (CLOCKS_PER_SEC == 1000000000L)
		c += dur->nsec;
	#else
	#	error UNSUPPORTED CLOCKS_PER_SEC
	#endif

/* TODO: handle clock overvlow */
/* TODO: check if there is abortion request or interrupt */
	while (c > clock()) 
	{
		_halt_cpu();
	}

#else

	#if defined(USE_THREAD)
	/* the sleep callback is called only if there is no IO semaphore 
	 * waiting. so i can safely call vm_muxwait() without a muxwait callback
	 * when USE_THREAD is true */
	vm_muxwait (moo, dur, MOO_NULL);
	#else
	struct timespec ts;
	ts.tv_sec = dur->sec;
	ts.tv_nsec = dur->nsec;
	nanosleep (&ts, MOO_NULL);
	#endif

#endif
}
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
	act.sa_flags = SA_RESTART;
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
#if defined(__DOS__) && (defined(_INTELC32_) || defined(__WATCOMC__))

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

static void handle_term (int sig)
{
	if (g_moo)
	{
		xtn_t* xtn = moo_getxtn(g_moo);
		write (xtn->p[1], "Q", 1);
		moo_abort (g_moo);
	}
}

static void setup_term (void)
{
	struct sigaction sa;
	memset (&sa, 0, MOO_SIZEOF(sa));
	sa.sa_handler = handle_term;
	sigaction (SIGINT, &sa, MOO_NULL);
}

/* ========================================================================= */

int main (int argc, char* argv[])
{
	static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' }; /*TODO: make this an argument */
	static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };

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

	memset (&vmprim, 0, MOO_SIZEOF(vmprim));
	vmprim.dl_open = dl_open;
	vmprim.dl_close = dl_close;
	vmprim.dl_getsym = dl_getsym;
	vmprim.log_write = log_write;
	vmprim.vm_startup = vm_startup;
	vmprim.vm_cleanup = vm_cleanup;
	vmprim.vm_gettime = vm_gettime;
	vmprim.vm_muxadd = vm_muxadd;
	vmprim.vm_muxdel = vm_muxdel;
	vmprim.vm_muxmod = vm_muxmod;
	vmprim.vm_muxwait = vm_muxwait;
	vmprim.vm_sleep = vm_sleep;

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
		moo_logbfmt (moo, MOO_LOG_ERROR, "cannot ignite moo - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
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
				#if defined(MOO_OOCH_IS_UCH)
					bcslen = MOO_COUNTOF(bcs);
					if (moo_convootobcstr (moo, synerr.loc.file, &ucslen, bcs, &bcslen) >= 0)
					{
						printf ("%.*s ", (int)bcslen, bcs);
					}
				#else
					printf ("%s ", synerr.loc.file);
				#endif
				}
				else
				{
					printf ("%s ", xtn->source_path);
				}

				printf ("syntax error at line %lu column %lu - ", 
					(unsigned long int)synerr.loc.line, (unsigned long int)synerr.loc.colm);

				bcslen = MOO_COUNTOF(bcs);
			#if defined(MOO_OOCH_IS_UCH)
				if (moo_convootobcstr (moo, moo_synerrnumtoerrstr(synerr.num), &ucslen, bcs, &bcslen) >= 0)
				{
					printf (" [%.*s]", (int)bcslen, bcs);
				}
			#else
				printf (" [%s]", moo_synerrnumtoerrstr(synerr.num));
			#endif

				if (synerr.tgt.len > 0)
				{
					bcslen = MOO_COUNTOF(bcs);
					ucslen = synerr.tgt.len;

				#if defined(MOO_OOCH_IS_UCH)
					if (moo_convootobchars (moo, synerr.tgt.ptr, &ucslen, bcs, &bcslen) >= 0)
					{
						printf (" [%.*s]", (int)bcslen, bcs);
					}
				#else
					printf (" [%.*s]", (int)synerr.tgt.len, synerr.tgt.ptr);
				#endif

				}
				printf ("\n");
			}
			else
			{
				moo_logbfmt (moo, MOO_LOG_ERROR, "ERROR: cannot compile code - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
			}
			moo_close (moo);
#if defined(USE_LTDL)
			lt_dlexit ();
#endif
			return -1;
		}
	}

	MOO_DEBUG0 (moo, "COMPILE OK. STARTING EXECUTION...\n");
	xret = 0;
	g_moo = moo;
	setup_tick ();
	setup_term ();

	objname.ptr = str_my_object;
	objname.len = 8;
	mthname.ptr = str_main;
	mthname.len = 4;
	if (moo_invoke (moo, &objname, &mthname) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_ERROR, "ERROR: cannot execute code - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
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
