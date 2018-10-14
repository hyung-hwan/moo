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

#include "moo-prv.h"
#include "moo-opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <locale.h>

#if !defined(__DOS__) && defined(HAVE_PTHREAD) && defined(HAVE_STRERROR_R)
#	define USE_THREAD
#endif

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <time.h>
#	include <io.h>
#	include <fcntl.h>
#	if defined(MOO_HAVE_CFG_H) && defined(MOO_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#	endif

#	include "poll-msw.h"
#	define USE_POLL
#	define XPOLLIN POLLIN
#	define XPOLLOUT POLLOUT
#	define XPOLLERR POLLERR
#	define XPOLLHUP POLLHUP

#elif defined(__OS2__)
#	define INCL_DOSMODULEMGR
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_DOSMISC
#	define INCL_DOSDATETIME
#	define INCL_DOSERRORS
#	include <os2.h>
#	include <time.h>
#	include <fcntl.h>
#	include <io.h>

	/* fake XPOLLXXX values */
#	define XPOLLIN  (1 << 0)
#	define XPOLLOUT (1 << 1)
#	define XPOLLERR (1 << 2)
#	define XPOLLHUP (1 << 3)

#elif defined(__DOS__)
#	include <dos.h>
#	include <time.h>
#	include <io.h>
#	include <signal.h>
#	include <errno.h>

	/* fake XPOLLXXX values */
#	define XPOLLIN  (1 << 0)
#	define XPOLLOUT (1 << 1)
#	define XPOLLERR (1 << 2)
#	define XPOLLHUP (1 << 3)

#elif defined(macintosh)
#	include <Types.h>
#	include <OSUtils.h>
#	include <Timer.h>

#else

#	if defined(MOO_ENABLE_LIBLTDL)
#		include <ltdl.h>
#		define USE_LTDL
#		define sys_dl_error() lt_dlerror()
#		define sys_dl_open(x) lt_dlopen(x)
#		define sys_dl_openext(x) lt_dlopenext(x)
#		define sys_dl_close(x) lt_dlclose(x)
#		define sys_dl_getsym(x,n) lt_dlsym(x,n)
#	elif defined(HAVE_DLFCN_H)
#		include <dlfcn.h>
#		define USE_DLFCN
#		define sys_dl_error() dlerror()
#		define sys_dl_open(x) dlopen(x,RTLD_NOW)
#		define sys_dl_openext(x) dlopen(x,RTLD_NOW)
#		define sys_dl_close(x) dlclose(x)
#		define sys_dl_getsym(x,n) dlsym(x,n)
#	elif defined(__APPLE__) || defined(__MACOSX__)
#		define USE_MACH_O
#		include <mach-o/dyld.h>
#		define sys_dl_error() mach_dlerror()
#		define sys_dl_open(x) mach_dlopen(x)
#		define sys_dl_openext(x) mach_dlopen(x)
#		define sys_dl_close(x) mach_dlclose(x)
#		define sys_dl_getsym(x,n) mach_dlsym(x,n)
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
#	if defined(HAVE_SYS_MMAN_H)
#		include <sys/mman.h>
#	endif

#	include <errno.h>
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
#		define USE_SELECT
		/* fake XPOLLXXX values */
#		define XPOLLIN  (1 << 0)
#		define XPOLLOUT (1 << 1)
#		define XPOLLERR (1 << 2)
#		define XPOLLHUP (1 << 3)
#	endif

#endif

#if !defined(MOO_DEFAULT_PFMODDIR)
#	define MOO_DEFAULT_PFMODDIR ""
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
#		if defined(USE_DLFCN)
#			define MOO_DEFAULT_PFMODPOSTFIX ".so"
#		elif defined(USE_MACH_O)
#			define MOO_DEFAULT_PFMODPOSTFIX ".dylib"
#		else
#			define MOO_DEFAULT_PFMODPOSTFIX ""
#		endif
#	endif
#endif

#if defined(USE_THREAD)
#	define MUTEX_INIT(x) pthread_mutex_init((x), MOO_NULL)
#	define MUTEX_DESTROY(x) pthread_mutex_destroy(x)
#	define MUTEX_LOCK(x) pthread_mutex_lock(x)
#	define MUTEX_UNLOCK(x) pthread_mutex_unlock(x)
#else
#	define MUTEX_INIT(x)
#	define MUTEX_DESTROY(x)
#	define MUTEX_LOCK(x) 
#	define MUTEX_UNLOCK(x) 
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

#if defined(USE_SELECT)
struct select_fd_t
{
	int fd;
	int events;
};
#endif

typedef struct xtn_t xtn_t;
struct xtn_t
{
	const char* source_path; /* main source file */
	int vm_running;

	moo_bitmask_t logmask;
	int logfd;
	int logfd_istty;
	
	struct
	{
		moo_bch_t buf[4096];
		moo_oow_t len;
	} logbuf;

	#if defined(_WIN32)
	HANDLE waitable_timer;
	#endif

	#if defined(USE_DEVPOLL)
	int ep; /* /dev/poll */
	#elif defined(USE_EPOLL)
	int ep; /* epoll */
	#elif defined(USE_POLL)
	/* nothing */
	#elif defined(USE_SELECT)
	/* nothing */
	#endif

	#if defined(USE_THREAD)
	int p[2]; /* pipe for signaling */
	pthread_t iothr;
	int iothr_up;
	int iothr_abort;
	#endif

	struct
	{
	#if defined(USE_DEVPOLL)
		/*TODO: make it dynamically changeable depending on the number of
		 *      file descriptors added */
		struct pollfd buf[64]; /* buffer for reading events */
	#elif defined(USE_EPOLL) 
		/*TODO: make it dynamically changeable depending on the number of
		 *      file descriptors added */
		struct epoll_event buf[64]; /* buffer for reading events */
	#elif defined(USE_POLL)
		struct
		{
			struct pollfd* ptr;
			moo_oow_t capa;
			moo_oow_t len;
		#if defined(USE_THREAD)
			pthread_mutex_t pmtx;
		#endif
		} reg; /* registrar */
		struct pollfd* buf;

	#elif defined(USE_SELECT)
		struct
		{
			fd_set rfds;
			fd_set wfds;
			int maxfd;
		#if defined(USE_THREAD)
			pthread_mutex_t smtx;
		#endif
		} reg;

		struct select_fd_t buf[FD_SETSIZE];
	#endif

		moo_oow_t len;

	#if defined(USE_THREAD)
		pthread_mutex_t mtx;
		pthread_cond_t cnd;
		pthread_cond_t cnd2;
	#endif
	} ev;
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
		bcslen = moo_count_bcstr (arg->name);
	#endif

		fn = ((bb_t*)arg->includer->handle)->fn;

		fb = get_base_name (fn);
		parlen = fb - fn;

		bb = moo_callocmem (moo, MOO_SIZEOF(*bb) + (MOO_SIZEOF(moo_bch_t) * (parlen + bcslen + 1)));
		if (!bb) goto oops;

		bb->fn = (moo_bch_t*)(bb + 1);
		moo_copy_bchars (bb->fn, fn, parlen);
	#if defined(MOO_OOCH_IS_UCH)
		moo_convootobcstr (moo, arg->name, &ucslen, &bb->fn[parlen], &bcslen);
	#else
		moo_copy_bcstr (&bb->fn[parlen], bcslen + 1, arg->name);
	#endif
	}
	else
	{
		/* main stream */
		moo_oow_t pathlen;

		pathlen = moo_count_bcstr (xtn->source_path);

		bb = moo_callocmem (moo, MOO_SIZEOF(*bb) + (MOO_SIZEOF(moo_bch_t) * (pathlen + 1)));
		if (!bb) goto oops;

		bb->fn = (moo_bch_t*)(bb + 1);
		moo_copy_bcstr (bb->fn, pathlen + 1, xtn->source_path);
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
	/*xtn_t* xtn = moo_getxtn(moo);*/
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
	moo_copy_bchars (arg->buf, bb->buf, bcslen);
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
			moo_seterrnum (moo, MOO_EINTERN);
			return -1;
	}
}

/* ========================================================================= */

static void* alloc_heap (moo_t* moo, moo_oow_t size)
{
#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP) && defined(MAP_ANONYMOUS)
	/* It's called via moo_makeheap() when HCL creates a GC heap.
	 * The heap is large in size. I can use a different memory allocation
	 * function instead of an ordinary malloc.
	 * upon failure, it doesn't require to set error information as moo_makeheap()
	 * set the error number to MOO_EOOMEM. */

#if !defined(MAP_HUGETLB) && (defined(__amd64__) || defined(__x86_64__))
#	define MAP_HUGETLB 0x40000
#endif

	moo_oow_t* ptr;
	int flags;
	moo_oow_t actual_size;

	flags = MAP_PRIVATE | MAP_ANONYMOUS;

	#if defined(MAP_HUGETLB)
	flags |= MAP_HUGETLB;
	#endif

	#if defined(MAP_UNINITIALIZED)
	flags |= MAP_UNINITIALIZED;
	#endif

	actual_size = MOO_SIZEOF(moo_oow_t) + size;
	actual_size = MOO_ALIGN_POW2(actual_size, 2 * 1024 * 1024);
	ptr = (moo_oow_t*)mmap(NULL, actual_size, PROT_READ | PROT_WRITE, flags, -1, 0);
	if (ptr == MAP_FAILED) 
	{
	#if defined(MAP_HUGETLB)
		flags &= ~MAP_HUGETLB;
		ptr = (moo_oow_t*)mmap(NULL, actual_size, PROT_READ | PROT_WRITE, flags, -1, 0);
		if (ptr == MAP_FAILED) return MOO_NULL;
	#else
		return MOO_NULL;
	#endif
	}
	*ptr = actual_size;

	return (void*)(ptr + 1);

#else
	return MOO_MMGR_ALLOC(moo->mmgr, size);
#endif
}

static void free_heap (moo_t* moo, void* ptr)
{
#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
	moo_oow_t* actual_ptr;
	actual_ptr = (moo_oow_t*)ptr - 1;
	munmap (actual_ptr, *actual_ptr);
#else
	MOO_MMGR_FREE(moo->mmgr, ptr);
#endif
}

static int write_all (int fd, const moo_bch_t* ptr, moo_oow_t len)
{
	while (len > 0)
	{
		moo_ooi_t wr;

		wr = write(fd, ptr, len);

		if (wr <= -1)
		{
		#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN == EWOULDBLOCK)
			if (errno == EAGAIN) continue;
		#else
			#if defined(EAGAIN)
			if (errno == EAGAIN) continue;
			#elif defined(EWOULDBLOCK)
			if (errno == EWOULDBLOCK) continue;
			#endif
		#endif

		#if defined(EINTR)
			/* TODO: would this interfere with non-blocking nature of this VM? */
			if (errno == EINTR) continue;
		#endif
			return -1;
		}

		ptr += wr;
		len -= wr;
	}

	return 0;
}

static int write_log (moo_t* moo, int fd, const moo_bch_t* ptr, moo_oow_t len)
{
	xtn_t* xtn;

	xtn = moo_getxtn(moo);

	while (len > 0)
	{
		if (xtn->logbuf.len > 0)
		{
			moo_oow_t rcapa, cplen;

			rcapa = MOO_COUNTOF(xtn->logbuf.buf) - xtn->logbuf.len;
			cplen = (len >= rcapa)? rcapa: len;

			memcpy (&xtn->logbuf.buf[xtn->logbuf.len], ptr, cplen);
			xtn->logbuf.len += cplen;
			ptr += cplen;
			len -= cplen;

			if (xtn->logbuf.len >= MOO_COUNTOF(xtn->logbuf.buf))
			{
				int n;
				n = write_all(fd, xtn->logbuf.buf, xtn->logbuf.len);
				xtn->logbuf.len = 0;
				if (n <= -1) return -1;
			}
		}
		else
		{
			moo_oow_t rcapa;

			rcapa = MOO_COUNTOF(xtn->logbuf.buf);
			if (len >= rcapa)
			{
				if (write_all(fd, ptr, rcapa) <= -1) return -1;
				ptr += rcapa;
				len -= rcapa;
			}
			else
			{
				memcpy (xtn->logbuf.buf, ptr, len);
				xtn->logbuf.len += len;
				ptr += len;
				len -= len;
				
			}
		}
	}

	return 0;
}

static void flush_log (moo_t* moo, int fd)
{
	xtn_t* xtn;
	xtn = moo_getxtn(moo);
	if (xtn->logbuf.len > 0)
	{
		write_all (fd, xtn->logbuf.buf, xtn->logbuf.len);
		xtn->logbuf.len = 0;
	}
}

static void log_write (moo_t* moo, moo_bitmask_t mask, const moo_ooch_t* msg, moo_oow_t len)
{
	moo_bch_t buf[256];
	moo_oow_t ucslen, bcslen, msgidx;
	int n;

	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	int logfd;

	if (mask & MOO_LOG_STDERR)
	{
		/* the messages that go to STDERR don't get masked out */
		logfd = 2;
	}
	else
	{
		if (!(xtn->logmask & mask & ~MOO_LOG_ALL_LEVELS)) return;  /* check log types */
		if (!(xtn->logmask & mask & ~MOO_LOG_ALL_TYPES)) return;  /* check log levels */

		if (mask & MOO_LOG_STDOUT) logfd = 1;
		else
		{
			logfd = xtn->logfd;
			if (logfd <= -1) return;
		}
	}

/* TODO: beautify the log message.
 *       do classification based on mask. */
	if (!(mask & (MOO_LOG_STDOUT | MOO_LOG_STDERR)))
	{
		time_t now;
		char ts[32];
		size_t tslen;
		struct tm tm, *tmp;

		now = time(NULL);
	#if defined(_WIN32) || defined(__DOS__)
		tmp = localtime(&now);
		tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S ", tmp); /* no timezone info */
		if (tslen == 0) 
		{
			strcpy (ts, "0000-00-00 00:00:00 ");
			tslen = 20; 
		}
	#else
		#if defined(__OS2__)
		tmp = _localtime(&now, &tm);
		#elif defined(HAVE_LOCALTIME_R)
		tmp = localtime_r(&now, &tm);
		#else
		tmp = localtime(&now);
		#endif
		#if defined(HAVE_STRFTIME_SMALL_Z)
		tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %z ", tmp);
		#else
		tslen = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S %Z ", tmp); 
		#endif
		if (tslen == 0) 
		{
			strcpy (ts, "0000-00-00 00:00:00 +0000 ");
			tslen = 26; 
		}
	#endif
		write_log (moo, logfd, ts, tslen);
	}

	if (logfd == xtn->logfd && xtn->logfd_istty)
	{
		if (mask & MOO_LOG_FATAL) write_log (moo, logfd, "\x1B[1;31m", 7);
		else if (mask & MOO_LOG_ERROR) write_log (moo, logfd, "\x1B[1;32m", 7);
		else if (mask & MOO_LOG_WARN) write_log (moo, logfd, "\x1B[1;33m", 7);
	}

#if defined(MOO_OOCH_IS_UCH)
	msgidx = 0;
	while (len > 0)
	{
		ucslen = len;
		bcslen = MOO_COUNTOF(buf);

		n = moo_convootobchars(moo, &msg[msgidx], &ucslen, buf, &bcslen);
		if (n == 0 || n == -2)
		{
			/* n = 0: 
			 *   converted all successfully 
			 * n == -2: 
			 *    buffer not sufficient. not all got converted yet.
			 *    write what have been converted this round. */

			MOO_ASSERT (moo, ucslen > 0); /* if this fails, the buffer size must be increased */

			/* attempt to write all converted characters */
			if (write_log(moo, logfd, buf, bcslen) <= -1) break;

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
	write_log (moo, logfd, msg, len);
#endif

	if (logfd == xtn->logfd && xtn->logfd_istty)
	{
		if (mask & (MOO_LOG_FATAL | MOO_LOG_ERROR | MOO_LOG_WARN)) write_log (moo, logfd, "\x1B[0m", 4);
	}

	flush_log (moo, logfd);
}

static void syserrstrb (moo_t* moo, int syserr, moo_bch_t* buf, moo_oow_t len)
{
#if defined(HAVE_STRERROR_R)
	strerror_r (syserr, buf, len);
#else
	/* this is not thread safe */
	moo_copy_bcstr (buf, len, strerror(syserr));
#endif
}

/* ========================================================================= */

#if defined(USE_MACH_O)
static const char* mach_dlerror_str = "";

static void* mach_dlopen (const char* path)
{
	NSObjectFileImage image;
	NSObjectFileImageReturnCode rc;
	void* handle;

	mach_dlerror_str = "";
	if ((rc = NSCreateObjectFileImageFromFile(path, &image)) != NSObjectFileImageSuccess) 
	{
		switch (rc)
		{
			case NSObjectFileImageFailure:
			case NSObjectFileImageFormat:
				mach_dlerror_str = "unable to crate object file image";
				break;

			case NSObjectFileImageInappropriateFile:
				mach_dlerror_str = "inappropriate file";
				break;

			case NSObjectFileImageArch:
				mach_dlerror_str = "incompatible architecture";
				break;

			case NSObjectFileImageAccess:
				mach_dlerror_str = "inaccessible file";
				break;
				
			default:
				mach_dlerror_str = "unknown error";
				break;
		}
		return MOO_NULL;
	}
	handle = (void*)NSLinkModule(image, path, NSLINKMODULE_OPTION_PRIVATE | NSLINKMODULE_OPTION_RETURN_ON_ERROR);
	NSDestroyObjectFileImage (image);
	return handle;
}

static MOO_INLINE void mach_dlclose (void* handle)
{
	mach_dlerror_str = "";
	NSUnLinkModule (handle, NSUNLINKMODULE_OPTION_NONE);
}

static MOO_INLINE void* mach_dlsym (void* handle, const char* name)
{
	mach_dlerror_str = "";
	return (void*)NSAddressOfSymbol(NSLookupSymbolInModule(handle, name));
}

static const char* mach_dlerror (void)
{
	int err_no;
	const char* err_file;
	NSLinkEditErrors err;

	if (mach_dlerror_str[0] == '\0')
		NSLinkEditError (&err, &err_no, &err_file, &mach_dlerror_str);

	return mach_dlerror_str;
}
#endif

static void* dl_open (moo_t* moo, const moo_ooch_t* name, int flags)
{
#if defined(USE_LTDL) || defined(USE_DLFCN) || defined(USE_MACH_O)
	moo_bch_t stabuf[128], * bufptr;
	moo_oow_t ucslen, bcslen, bufcapa;
	void* handle;

	#if defined(MOO_OOCH_IS_UCH)
	if (moo_convootobcstr(moo, name, &ucslen, MOO_NULL, &bufcapa) <= -1) return MOO_NULL;
	/* +1 for terminating null. but it's not needed because MOO_COUNTOF(MOO_DEFAULT_PFMODPREFIX)
	 * and MOO_COUNTOF(MOO_DEFAULT_PFMODPOSTIFX) include the terminating nulls. Never mind about
	 * the extra 2 characters. */
	#else
	bufcapa = moo_count_bcstr(name);
	#endif
	bufcapa += MOO_COUNTOF(MOO_DEFAULT_PFMODDIR) + MOO_COUNTOF(MOO_DEFAULT_PFMODPREFIX) + MOO_COUNTOF(MOO_DEFAULT_PFMODPOSTFIX) + 1; 

	if (bufcapa <= MOO_COUNTOF(stabuf)) bufptr = stabuf;
	else
	{
		bufptr = moo_allocmem(moo, bufcapa * MOO_SIZEOF(*bufptr));
		if (!bufptr) return MOO_NULL;
	}

	if (flags & MOO_VMPRIM_DLOPEN_PFMOD)
	{
		moo_oow_t len, i, xlen, dlen;

		/* opening a primitive function module - mostly libmoo-xxxx */
		dlen = moo_copy_bcstr(bufptr, bufcapa, MOO_DEFAULT_PFMODDIR);
		len = moo_copy_bcstr(&bufptr[dlen], bufcapa - dlen, MOO_DEFAULT_PFMODPREFIX);
		len += dlen;

		bcslen = bufcapa - len;
	#if defined(MOO_OOCH_IS_UCH)
		moo_convootobcstr(moo, name, &ucslen, &bufptr[len], &bcslen);
	#else
		bcslen = moo_copy_bcstr(&bufptr[len], bcslen, name);
	#endif

		/* length including the directory, the prefix and the name. but excluding the postfix */
		xlen = len + bcslen; 

		for (i = len; i < xlen; i++) 
		{
			/* convert a period(.) to a dash(-) */
			if (bufptr[i] == '.') bufptr[i] = '-';
		}
 
	retry:
		moo_copy_bcstr (&bufptr[xlen], bufcapa - xlen, MOO_DEFAULT_PFMODPOSTFIX);

		/* both prefix and postfix attached. for instance, libmoo-xxx */
		handle = sys_dl_openext(&bufptr[dlen]);
		if (!handle) 
		{
			MOO_DEBUG3 (moo, "Unable to open(ext) PFMOD %hs[%js] - %hs\n", &bufptr[dlen], name, sys_dl_error());

		#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
			if (dlen > 0 && bufptr[len] != '/' && bufptr[len != '\\')
		#else
			if (dlen > 0 && bufptr[len] != '/')
		#endif
			{
				handle = sys_dl_openext(&bufptr[0]);
				if (handle) goto pfmod_open_ok;
				MOO_DEBUG3 (moo, "Unable to open(ext) PFMOD %hs[%js] - %hs\n", &bufptr[0], name, sys_dl_error());
			}

			/* try without prefix and postfix */
			bufptr[xlen] = '\0';
			handle = sys_dl_openext(&bufptr[len]);
			if (!handle) 
			{
				moo_bch_t* dash;
				const moo_bch_t* dl_errstr;
				dl_errstr = sys_dl_error();
				MOO_DEBUG3 (moo, "Unable to open(ext) PFMOD %hs[%js] - %hs\n", &bufptr[len], name, dl_errstr);
				moo_seterrbfmt (moo, MOO_ESYSERR, "unable to open(ext) PFMOD %js - %hs", name, dl_errstr);
				dash = moo_rfind_bchar(bufptr, moo_count_bcstr(bufptr), '-');
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
			else 
			{
				MOO_DEBUG3 (moo, "Opened(ext) PFMOD %hs[%js] handle %p\n", &bufptr[len], name, handle);
			}
		}
		else
		{
		pfmod_open_ok:
			MOO_DEBUG3 (moo, "Opened(ext) PFMOD %hs[%js] handle %p\n", &bufptr[dlen], name, handle);
		}
	}
	else
	{
		/* opening a raw shared object without a prefix and/or a postfix */
	#if defined(MOO_OOCH_IS_UCH)
		bcslen = bufcapa;
		moo_convootobcstr(moo, name, &ucslen, bufptr, &bcslen);
	#else
		bcslen = moo_copy_bcstr(bufptr, bufcapa, name);
	#endif

		if (moo_find_bchar (bufptr, bcslen, '.'))
		{
			handle = sys_dl_open(bufptr);
			if (!handle) 
			{
				const moo_bch_t* dl_errstr;
				dl_errstr = sys_dl_error();
				MOO_DEBUG2 (moo, "Unable to open DL %hs - %hs\n", bufptr, dl_errstr);
				moo_seterrbfmt (moo, MOO_ESYSERR, "unable to open DL %js - %hs", name, dl_errstr);
			}
			else MOO_DEBUG2 (moo, "Opened DL %hs handle %p\n", bufptr, handle);
		}
		else
		{
			handle = sys_dl_openext(bufptr);
			if (!handle) 
			{
				const moo_bch_t* dl_errstr;
				dl_errstr = sys_dl_error();
				MOO_DEBUG2 (moo, "Unable to open(ext) DL %hs - %hs\n", bufptr, dl_errstr);
				moo_seterrbfmt (moo, MOO_ESYSERR, "unable to open(ext) DL %js - %hs", name, dl_errstr);
			}
			else MOO_DEBUG2 (moo, "Opened(ext) DL %hs handle %p\n", bufptr, handle);
		}
	}

	if (bufptr != stabuf) moo_freemem (moo, bufptr);
	return handle;

#else

/* TODO: support various platforms */
	/* TODO: implemenent this */
	MOO_DEBUG1 (moo, "Dynamic loading not implemented - cannot open %js\n", name);
	moo_seterrbfmt (moo, MOO_ENOIMPL, "dynamic loading not implemented - cannot open %js", name);
	return MOO_NULL;
#endif
}

static void dl_close (moo_t* moo, void* handle)
{
#if defined(USE_LTDL) || defined(USE_DLFCN) || defined(USE_MACH_O)
	MOO_DEBUG1 (moo, "Closed DL handle %p\n", handle);
	sys_dl_close (handle);

#else
	/* TODO: implemenent this */
	MOO_DEBUG1 (moo, "Dynamic loading not implemented - cannot close handle %p\n", handle);
#endif
}

static void* dl_getsym (moo_t* moo, void* handle, const moo_ooch_t* name)
{
#if defined(USE_LTDL) || defined(USE_DLFCN) || defined(USE_MACH_O)
	moo_bch_t stabuf[64], * bufptr;
	moo_oow_t bufcapa, ucslen, bcslen, i;
	const moo_bch_t* symname;
	void* sym;

	#if defined(MOO_OOCH_IS_UCH)
	if (moo_convootobcstr(moo, name, &ucslen, MOO_NULL, &bcslen) <= -1) return MOO_NULL;
	#else
	bcslen = moo_count_bcstr (name);
	#endif

	if (bcslen >= MOO_COUNTOF(stabuf) - 2)
	{
		bufcapa = bcslen + 3;
		bufptr = moo_allocmem(moo, bufcapa * MOO_SIZEOF(*bufptr));
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
	bcslen = moo_copy_bcstr(&bufptr[1], bcslen, name);
	#endif

	/* convert a period(.) to an underscore(_) */
	for (i = 1; i <= bcslen; i++) if (bufptr[i] == '.') bufptr[i] = '_';

	symname = &bufptr[1]; /* try the name as it is */
	sym = sys_dl_getsym(handle, symname);
	if (!sym)
	{
		bufptr[0] = '_';
		symname = &bufptr[0]; /* try _name */
		sym = sys_dl_getsym(handle, symname);
		if (!sym)
		{
			bufptr[bcslen + 1] = '_'; 
			bufptr[bcslen + 2] = '\0';

			symname = &bufptr[1]; /* try name_ */
			sym = sys_dl_getsym(handle, symname);

			if (!sym)
			{
				symname = &bufptr[0]; /* try _name_ */
				sym = sys_dl_getsym(handle, symname);
				if (!sym)
				{
					const moo_bch_t* dl_errstr;
					dl_errstr = sys_dl_error();
					MOO_DEBUG3 (moo, "Unable to get module symbol %js from handle %p - %hs\n", name, handle, dl_errstr);
					moo_seterrbfmt (moo, MOO_ENOENT, "unable to get module symbol %hs - %hs", symname, dl_errstr);
				}
			}
		}
	}

	if (sym) MOO_DEBUG3 (moo, "Loaded module symbol %js from handle %p - %hs\n", name, handle, symname);
	if (bufptr != stabuf) moo_freemem (moo, bufptr);
	return sym;

#else
	/* TODO: IMPLEMENT THIS */
	MOO_DEBUG2 (moo, "Dynamic loading not implemented - Cannot load module symbol %js from handle %p\n", name, handle);
	moo_seterrbfmt (moo, MOO_ENOIMPL, "dynamic loading not implemented - Cannot load module symbol %js from handle %p", name, handle);
	return MOO_NULL;
#endif
}

/* ========================================================================= */
static int _add_poll_fd (moo_t* moo, int fd, int event_mask)
{
#if defined(USE_DEVPOLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	struct pollfd ev;

	MOO_ASSERT (moo, xtn->ep >= 0);
	ev.fd = fd;
	ev.events = event_mask;
	ev.revents = 0;
	if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
	{
		moo_seterrwithsyserr (moo, errno);
		MOO_DEBUG2 (moo, "Cannot add file descriptor %d to devpoll - %hs\n", fd, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(USE_EPOLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	struct epoll_event ev;

	MOO_ASSERT (moo, xtn->ep >= 0);
	memset (&ev, 0, MOO_SIZEOF(ev));
	ev.events = event_mask;
	#if defined(USE_THREAD) && defined(EPOLLET)
	/* epoll_wait may return again if the worker thread consumes events.
	 * switch to level-trigger. */
	/* TODO: verify if EPOLLLET is desired */
	ev.events |= EPOLLET/*  | EPOLLRDHUP | EPOLLHUP */;
	#endif
	/*ev.data.ptr = (void*)event_data;*/
	ev.data.fd = fd;
	if (epoll_ctl(xtn->ep, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		moo_seterrwithsyserr (moo, errno);
		MOO_DEBUG2 (moo, "Cannot add file descriptor %d to epoll - %hs\n", fd, strerror(errno));
		return -1;
	}
	return 0;

#elif defined(USE_POLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

	MUTEX_LOCK (&xtn->ev.reg.pmtx);
	if (xtn->ev.reg.len >= xtn->ev.reg.capa)
	{
		struct pollfd* tmp, * tmp2;
		moo_oow_t newcapa;

		newcapa = MOO_ALIGN_POW2 (xtn->ev.reg.len + 1, 256);
		tmp = (struct pollfd*)moo_reallocmem (moo, xtn->ev.reg.ptr, newcapa * MOO_SIZEOF(*tmp));
		tmp2 = (struct pollfd*)moo_reallocmem (moo, xtn->ev.buf, newcapa * MOO_SIZEOF(*tmp2));
		if (!tmp || !tmp2)
		{
			MOO_DEBUG2 (moo, "Cannot add file descriptor %d to poll - %hs\n", fd, strerror(errno));
			MUTEX_UNLOCK (&xtn->ev.reg.pmtx);
			if (tmp) moo_freemem (moo, tmp);
			return -1;
		}

		xtn->ev.reg.ptr = tmp;
		xtn->ev.reg.capa = newcapa;

		xtn->ev.buf = tmp2;
	}

	xtn->ev.reg.ptr[xtn->ev.reg.len].fd = fd;
	xtn->ev.reg.ptr[xtn->ev.reg.len].events = event_mask;
	xtn->ev.reg.ptr[xtn->ev.reg.len].revents = 0;
	xtn->ev.reg.len++;
	MUTEX_UNLOCK (&xtn->ev.reg.pmtx);

	return 0;

#elif defined(USE_SELECT)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

	MUTEX_LOCK (&xtn->ev.reg.smtx);
	if (event_mask & XPOLLIN) 
	{
		FD_SET (fd, &xtn->ev.reg.rfds);
		if (fd > xtn->ev.reg.maxfd) xtn->ev.reg.maxfd = fd;
	}
	if (event_mask & XPOLLOUT) 
	{
		FD_SET (fd, &xtn->ev.reg.wfds);
		if (fd > xtn->ev.reg.maxfd) xtn->ev.reg.maxfd = fd;
	}
	MUTEX_UNLOCK (&xtn->ev.reg.smtx);

	return 0;

#else

	MOO_DEBUG1 (moo, "Cannot add file descriptor %d to poll - not implemented\n", fd);
	moo_seterrnum (moo, MOO_ENOIMPL);
	return -1;
#endif

}

static int _del_poll_fd (moo_t* moo, int fd)
{

#if defined(USE_DEVPOLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	struct pollfd ev;

	MOO_ASSERT (moo, xtn->ep >= 0);
	ev.fd = fd;
	ev.events = POLLREMOVE;
	ev.revents = 0;
	if (write (xtn->ep, &ev, MOO_SIZEOF(ev)) != MOO_SIZEOF(ev))
	{
		moo_syserr_to_errnum (errno);
		MOO_DEBUG2 (moo, "Cannot remove file descriptor %d from devpoll - %hs\n", fd, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(USE_EPOLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	struct epoll_event ev;

	MOO_ASSERT (moo, xtn->ep >= 0);
	memset (&ev, 0, MOO_SIZEOF(ev));
	if (epoll_ctl (xtn->ep, EPOLL_CTL_DEL, fd, &ev) == -1)
	{
		moo_syserr_to_errnum (errno);
		MOO_DEBUG2 (moo, "Cannot remove file descriptor %d from epoll - %hs\n", fd, strerror(errno));
		return -1;
	}
	return 0;

#elif defined(USE_POLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	moo_oow_t i;

	/* TODO: performance boost. no linear search */
	MUTEX_LOCK (&xtn->ev.reg.pmtx);
	for (i = 0; i < xtn->ev.reg.len; i++)
	{
		if (xtn->ev.reg.ptr[i].fd == fd)
		{
			xtn->ev.reg.len--;
			memmove (&xtn->ev.reg.ptr[i], &xtn->ev.reg.ptr[i+1], (xtn->ev.reg.len - i) * MOO_SIZEOF(*xtn->ev.reg.ptr));
			MUTEX_UNLOCK (&xtn->ev.reg.pmtx);
			return 0;
		}
	}
	MUTEX_UNLOCK (&xtn->ev.reg.pmtx);


	MOO_DEBUG1 (moo, "Cannot remove file descriptor %d from poll - not found\n", fd);
	moo_seterrnum (moo, MOO_ENOENT);
	return -1;

#elif defined(USE_SELECT)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

	MUTEX_LOCK (&xtn->ev.reg.smtx);
	FD_CLR (fd, &xtn->ev.reg.rfds);
	FD_CLR (fd, &xtn->ev.reg.wfds);
	if (fd >= xtn->ev.reg.maxfd)
	{
		int i;
		/* TODO: any way to make this search faster or to do without the search like this */
		for (i = fd - 1; i >= 0; i--)
		{
			if (FD_ISSET(i, &xtn->ev.reg.rfds) || FD_ISSET(i, &xtn->ev.reg.wfds)) break;
		}
		xtn->ev.reg.maxfd = i;
	}
	MUTEX_UNLOCK (&xtn->ev.reg.smtx);

	return 0;

#else

	MOO_DEBUG1 (moo, "Cannot remove file descriptor %d from poll - not implemented\n", fd);
	moo_seterrnum (moo, MOO_ENOIMPL);
	return -1;
#endif
}


static int _mod_poll_fd (moo_t* moo, int fd, int event_mask)
{
#if defined(USE_DEVPOLL)

	if (_del_poll_fd (moo, fd) <= -1) return -1;

	if (_add_poll_fd (moo, fd, event_mask, event_data) <= -1) 
	{
		/* TODO: any good way to rollback successful deletion? */
		return -1;
	}

	return 0;

#elif defined(USE_EPOLL)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	struct epoll_event ev;

	MOO_ASSERT (moo, xtn->ep >= 0);
	memset (&ev, 0, MOO_SIZEOF(ev));
	ev.events = event_mask;
	#if defined(USE_THREAD) && defined(EPOLLET)
	/* epoll_wait may return again if the worker thread consumes events.
	 * switch to level-trigger. */
	/* TODO: verify if EPOLLLET is desired */
	ev.events |= EPOLLET;
	#endif
	ev.data.fd = fd;
	if (epoll_ctl (xtn->ep, EPOLL_CTL_MOD, fd, &ev) == -1)
	{
		moo_seterrwithsyserr (moo, errno);
		MOO_DEBUG2 (moo, "Cannot modify file descriptor %d in epoll - %hs\n", fd, strerror(errno));
		return -1;
	}

	return 0;

#elif defined(USE_POLL)

	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	moo_oow_t i;

	MUTEX_LOCK (&xtn->ev.reg.pmtx);
	for (i = 0; i < xtn->ev.reg.len; i++)
	{
		if (xtn->ev.reg.ptr[i].fd == fd)
		{
			memmove (&xtn->ev.reg.ptr[i], &xtn->ev.reg.ptr[i+1], (xtn->ev.reg.len - i - 1) * MOO_SIZEOF(*xtn->ev.reg.ptr));
			xtn->ev.reg.ptr[i].fd = fd;
			xtn->ev.reg.ptr[i].events = event_mask;
			xtn->ev.reg.ptr[i].revents = 0;
			MUTEX_UNLOCK (&xtn->ev.reg.pmtx);

			return 0;
		}
	}
	MUTEX_UNLOCK (&xtn->ev.reg.pmtx);

	MOO_DEBUG1 (moo, "Cannot modify file descriptor %d in poll - not found\n", fd);
	moo_seterrnum (moo, MOO_ENOENT);
	return -1;

#elif defined(USE_SELECT)

	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);

	MUTEX_LOCK (&xtn->ev.reg.smtx);
	MOO_ASSERT (moo, fd <= xtn->ev.reg.maxfd);

	if (event_mask & XPOLLIN) 
		FD_SET (fd, &xtn->ev.reg.rfds);
	else 
		FD_CLR (fd, &xtn->ev.reg.rfds);

	if (event_mask & XPOLLOUT) 
		FD_SET (fd, &xtn->ev.reg.wfds);
	else
		FD_CLR (fd, &xtn->ev.reg.wfds);
	MUTEX_UNLOCK (&xtn->ev.reg.smtx);

	return 0;

#else
	MOO_DEBUG1 (moo, "Cannot modify file descriptor %d in poll - not implemented\n", fd);
	moo_seterrnum (moo, MOO_ENOIMPL);
	return -1;
#endif
}


static int vm_startup (moo_t* moo)
{
#if defined(_WIN32)
	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	xtn->waitable_timer = CreateWaitableTimer(MOO_NULL, TRUE, MOO_NULL);

#else

	xtn_t* xtn = (xtn_t*)moo_getxtn(moo);
	int pcount = 0, flag;

#if defined(USE_DEVPOLL)
	xtn->ep = open ("/dev/poll", O_RDWR);
	if (xtn->ep == -1) 
	{
		moo_syserr_to_errnum (errno);
		MOO_DEBUG1 (moo, "Cannot create devpoll - %hs\n", strerror(errno));
		goto oops;
	}

	flag = fcntl (xtn->ep, F_GETFD);
	if (flag >= 0) fcntl (xtn->ep, F_SETFD, flag | FD_CLOEXEC);

#elif defined(USE_EPOLL)
	#if defined(EPOLL_CLOEXEC)
	xtn->ep = epoll_create1(EPOLL_CLOEXEC);
	#else
	xtn->ep = epoll_create(1024);
	#endif
	if (xtn->ep == -1) 
	{
		moo_syserr_to_errnum (errno);
		MOO_DEBUG1 (moo, "Cannot create epoll - %hs\n", strerror(errno));
		goto oops;
	}

	#if defined(EPOLL_CLOEXEC)
	/* do nothing */
	#else
	flag = fcntl(xtn->ep, F_GETFD);
	if (flag >= 0) fcntl (xtn->ep, F_SETFD, flag | FD_CLOEXEC);
	#endif

#elif defined(USE_POLL)

	MUTEX_INIT (&xtn->ev.reg.pmtx);

#elif defined(USE_SELECT)
	FD_ZERO (&xtn->ev.reg.rfds);
	FD_ZERO (&xtn->ev.reg.wfds);
	xtn->ev.reg.maxfd = -1;
	MUTEX_INIT (&xtn->ev.reg.smtx);
#endif /* USE_DEVPOLL */

#if defined(USE_THREAD)
	if (pipe(xtn->p) == -1)
	{
		moo_syserr_to_errnum (errno);
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

	if (_add_poll_fd(moo, xtn->p[0], XPOLLIN) <= -1) goto oops;

	pthread_mutex_init (&xtn->ev.mtx, MOO_NULL);
	pthread_cond_init (&xtn->ev.cnd, MOO_NULL);
	pthread_cond_init (&xtn->ev.cnd2, MOO_NULL);

	xtn->iothr_abort = 0;
	xtn->iothr_up = 0;
	/*pthread_create (&xtn->iothr, MOO_NULL, iothr_main, moo);*/

	
#endif /* USE_THREAD */

	xtn->vm_running = 1;
	return 0;

oops:

#if defined(USE_THREAD)
	if (pcount > 0)
	{
		close (xtn->p[0]);
		close (xtn->p[1]);
	}
#endif

#if defined(USE_DEVPOLL) || defined(USE_EPOLL)
	if (xtn->ep >= 0)
	{
		close (xtn->ep);
		xtn->ep = -1;
	}
#endif

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

	xtn->vm_running = 0;

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

	_del_poll_fd (moo, xtn->p[0]);
	close (xtn->p[1]);
	close (xtn->p[0]);
#endif /* USE_THREAD */

#if defined(USE_DEVPOLL) 
	if (xtn->ep >= 0)
	{
		close (xtn->ep);
		xtn->ep = -1;
	}
	/*destroy_poll_data_space (moo);*/
#elif defined(USE_EPOLL)
	if (xtn->ep >= 0)
	{
		close (xtn->ep);
		xtn->ep = -1;
	}
#elif defined(USE_POLL)
	if (xtn->ev.reg.ptr)
	{
		moo_freemem (moo, xtn->ev.reg.ptr);
		xtn->ev.reg.ptr = MOO_NULL;
		xtn->ev.reg.len = 0;
		xtn->ev.reg.capa = 0;
	}
	if (xtn->ev.buf)
	{
		moo_freemem (moo, xtn->ev.buf);
		xtn->ev.buf = MOO_NULL;
	}
	/*destroy_poll_data_space (moo);*/
	MUTEX_DESTROY (&xtn->ev.reg.pmtx);
#elif defined(USE_SELECT)
	FD_ZERO (&xtn->ev.reg.rfds);
	FD_ZERO (&xtn->ev.reg.wfds);
	xtn->ev.reg.maxfd = -1;
	MUTEX_DESTROY (&xtn->ev.reg.smtx);
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

static int vm_muxadd (moo_t* moo, moo_ooi_t io_handle, moo_ooi_t mask)
{
	int event_mask;

	event_mask = 0;
	if (mask & MOO_SEMAPHORE_IO_MASK_INPUT) event_mask |= XPOLLIN; 
	if (mask & MOO_SEMAPHORE_IO_MASK_OUTPUT) event_mask |= XPOLLOUT;

	if (event_mask == 0)
	{
		MOO_DEBUG2 (moo, "<vm_muxadd> Invalid semaphore mask %zd on handle %zd\n", mask, io_handle);
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid semaphore mask %zd on handle %zd", mask, io_handle);
		return -1;
	}

	return _add_poll_fd(moo, io_handle, event_mask);
}

static int vm_muxmod (moo_t* moo, moo_ooi_t io_handle, moo_ooi_t mask)
{
	int event_mask;

	event_mask = 0;
	if (mask & MOO_SEMAPHORE_IO_MASK_INPUT) event_mask |= XPOLLIN; 
	if (mask & MOO_SEMAPHORE_IO_MASK_OUTPUT) event_mask |= XPOLLOUT;

	if (event_mask == 0)
	{
		MOO_DEBUG2 (moo, "<vm_muxadd> Invalid semaphore mask %zd on handle %zd\n", mask, io_handle);
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid semaphore mask %zd on handle %zd", mask, io_handle);
		return -1;
	}

	return _mod_poll_fd(moo, io_handle, event_mask);
}

static int vm_muxdel (moo_t* moo, moo_ooi_t io_handle)
{
	return _del_poll_fd(moo, io_handle);
}

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
		#elif defined(USE_POLL)
			moo_oow_t nfds;
		#elif defined(USE_SELECT)
			struct timeval tv;
			fd_set rfds;
			fd_set wfds;
			int maxfd;
		#endif

		poll_for_event:
		
		#if defined(USE_DEVPOLL)
			dvp.dp_timeout = 10000; /* milliseconds */
			dvp.dp_fds = xtn->ev.buf;
			dvp.dp_nfds = MOO_COUNTOF(xtn->ev.buf);
			n = ioctl (xtn->ep, DP_POLL, &dvp);
		#elif defined(USE_EPOLL)
			n = epoll_wait (xtn->ep, xtn->ev.buf, MOO_COUNTOF(xtn->ev.buf), 10000); /* TODO: make this timeout value in the io thread */
		#elif defined(USE_POLL)
			MUTEX_LOCK (&xtn->ev.reg.pmtx);
			memcpy (xtn->ev.buf, xtn->ev.reg.ptr, xtn->ev.reg.len * MOO_SIZEOF(*xtn->ev.buf));
			nfds = xtn->ev.reg.len;
			MUTEX_UNLOCK (&xtn->ev.reg.pmtx);
			n = poll(xtn->ev.buf, nfds, 10000);
			if (n > 0) 
			{
				/* compact the return buffer as poll() doesn't */
				moo_oow_t i, j;
				for (i = 0, j = 0; i < nfds && j < n; i++)
				{
					if (xtn->ev.buf[i].revents)
					{
						if (j != i) xtn->ev.buf[j] = xtn->ev.buf[i];
						j++;
					}
				}
				n = j;
			}
		#elif defined(USE_SELECT)
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			MUTEX_LOCK (&xtn->ev.reg.smtx);
			maxfd = xtn->ev.reg.maxfd;
			MOO_MEMCPY (&rfds, &xtn->ev.reg.rfds, MOO_SIZEOF(rfds));
			MOO_MEMCPY (&wfds, &xtn->ev.reg.wfds, MOO_SIZEOF(wfds));
			MUTEX_UNLOCK (&xtn->ev.reg.smtx);
			n = select (maxfd + 1, &rfds, &wfds, NULL, &tv);
			if (n > 0)
			{
				int fd, count = 0;
				for (fd = 0;  fd <= maxfd; fd++)
				{
					int events = 0;
					if (FD_ISSET(fd, &rfds)) events |= XPOLLIN;
					if (FD_ISSET(fd, &wfds)) events |= XPOLLOUT;

					if (events)
					{
						MOO_ASSERT (moo, count < MOO_COUNTOF(xtn->ev.buf));
						xtn->ev.buf[count].fd = fd;
						xtn->ev.buf[count].events = events;
						count++;
					}
				}

				n = count;
				MOO_ASSERT (moo, n > 0);
			}
		#endif

			pthread_mutex_lock (&xtn->ev.mtx);
			if (n <= -1)
			{
				/* TODO: don't use MOO_DEBUG2. it's not thread safe... */
				/* the following call has a race-condition issue when called in this separate thread */
				/*MOO_DEBUG2 (moo, "Warning: multiplexer wait failure - %d, %hs\n", errno, strerror(errno));*/
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

		#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
			clock_gettime (CLOCK_REALTIME, &ts);
		#else
			{
				struct timeval tv;
				gettimeofday (&tv, MOO_NULL);
				ts.tv_sec = tv.tv_sec;
				ts.tv_nsec = MOO_USEC_TO_NSEC(tv.tv_usec);
			}
		#endif
			ts.tv_sec += 10;
			pthread_cond_timedwait (&xtn->ev.cnd, &xtn->ev.mtx, &ts);
			pthread_mutex_unlock (&xtn->ev.mtx);
		}

		/*sched_yield ();*/
	}

	return MOO_NULL;
}
#endif

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

	if (xtn->iothr_abort) return;

	if (xtn->ev.len <= 0) 
	{
		struct timespec ts;
		moo_ntime_t ns;

		if (!dur) return; /* immediate check is requested. and there is no event */

	#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
		clock_gettime (CLOCK_REALTIME, &ts);
		ns.sec = ts.tv_sec;
		ns.nsec = ts.tv_nsec;
	#else
		{
			struct timeval tv;
			gettimeofday (&tv, MOO_NULL);
			ns.sec = tv.tv_sec;
			ns.nsec = MOO_USEC_TO_NSEC(tv.tv_usec);
		}
	#endif
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
		#elif defined(USE_EPOLL)
			/*if (xtn->ev.buf[n].data.ptr == (void*)MOO_TYPE_MAX(moo_oow_t))*/
			if (xtn->ev.buf[n].data.fd == xtn->p[0])
		#elif defined(USE_POLL)
			if (xtn->ev.buf[n].fd == xtn->p[0])
		#elif defined(USE_SELECT)
			if (xtn->ev.buf[n].fd == xtn->p[0])
		#else
		#	error UNSUPPORTED
		#endif
			{
				moo_uint8_t u8;
				while (read(xtn->p[0], &u8, MOO_SIZEOF(u8)) > 0) 
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
			#elif defined(USE_EPOLL)
				revents = xtn->ev.buf[n].events;
			#elif defined(USE_POLL)
				revents = xtn->ev.buf[n].revents;
			#elif defined(USE_SELECT)
				revents = xtn->ev.buf[n].events;
			#endif

				mask = 0;
				if (revents & XPOLLIN) mask |= MOO_SEMAPHORE_IO_MASK_INPUT;
				if (revents & XPOLLOUT) mask |= MOO_SEMAPHORE_IO_MASK_OUTPUT;
				if (revents & XPOLLERR) mask |= MOO_SEMAPHORE_IO_MASK_ERROR;
				if (revents & XPOLLHUP) mask |= MOO_SEMAPHORE_IO_MASK_HANGUP;

			#if defined(USE_DEVPOLL)
				muxwcb (moo, xtn->ev.buf[n].fd, mask);
			#elif defined(USE_EPOLL)
				muxwcb (moo, xtn->ev.buf[n].data.fd, mask);
			#elif defined(USE_POLL)
				muxwcb (moo, xtn->ev.buf[n].fd, mask);
			#elif defined(USE_SELECT)
				muxwcb (moo, xtn->ev.buf[n].fd, mask);
			#else
			#	error UNSUPPORTED
			#endif
			}
		}
		while (n > 0);

		pthread_mutex_lock (&xtn->ev.mtx);
		xtn->ev.len = 0;
		pthread_cond_signal (&xtn->ev.cnd);
		pthread_mutex_unlock (&xtn->ev.mtx);
	}

#else /* USE_THREAD */
	int tmout = 0, n;
	#if defined(USE_DEVPOLL)
	struct dvpoll dvp;
	#elif defined(USE_SELECT)
	struct timeval tv;
	fd_set rfds, wfds;
	int maxfd;
	#endif

	if (dur) tmout = MOO_SECNSEC_TO_MSEC(dur->sec, dur->nsec);

	#if defined(USE_DEVPOLL)
	dvp.dp_timeout = tmout; /* milliseconds */
	dvp.dp_fds = xtn->ev.buf;
	dvp.dp_nfds = MOO_COUNTOF(xtn->ev.buf);
	n = ioctl (xtn->ep, DP_POLL, &dvp);
	#elif defined(USE_EPOLL)
	n = epoll_wait (xtn->ep, xtn->ev.buf, MOO_COUNTOF(xtn->ev.buf), tmout);
	#elif defined(USE_POLL)
	memcpy (xtn->ev.buf, xtn->ev.reg.ptr, xtn->ev.reg.len * MOO_SIZEOF(*xtn->ev.buf));
	n = poll(xtn->ev.buf, xtn->ev.reg.len, tmout);
	if (n > 0) 
	{
		/* compact the return buffer as poll() doesn't */
		moo_oow_t i, j;
		for (i = 0, j = 0; i < xtn->ev.reg.len && j < n; i++)
		{
			if (xtn->ev.buf[i].revents)
			{
				if (j != i) xtn->ev.buf[j] = xtn->ev.buf[i];
				j++;
			}
		}
		n = j;
	}
	#elif defined(USE_SELECT)
	if (dur)
	{
		tv.tv_sec = dur->sec;
		tv.tv_usec = MOO_NSEC_TO_USEC(dur->nsec); 
	}
	else
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
	maxfd = xtn->ev.reg.maxfd;
	MOO_MEMCPY (&rfds, &xtn->ev.reg.rfds, MOO_SIZEOF(rfds));
	MOO_MEMCPY (&wfds, &xtn->ev.reg.wfds, MOO_SIZEOF(wfds));
	n = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
	if (n > 0)
	{
		int fd, count = 0;
		for (fd = 0; fd <= maxfd; fd++)
		{
			int events = 0;
			if (FD_ISSET(fd, &rfds)) events |= XPOLLIN;
			if (FD_ISSET(fd, &wfds)) events |= XPOLLOUT;

			if (events)
			{
				MOO_ASSERT (moo, count < MOO_COUNTOF(xtn->ev.buf));
				xtn->ev.buf[count].fd = fd;
				xtn->ev.buf[count].events = events;
				count++;
			}
		}

		n = count;
		MOO_ASSERT (moo, n > 0);
	}
	#endif

	if (n <= -1)
	{
		moo_seterrwithsyserr (moo, errno);
		MOO_DEBUG2 (moo, "Warning: multiplexer wait failure - %d, %s\n", errno, moo_geterrmsg(moo));
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
	#elif defined(USE_EPOLL)
		revents = xtn->ev.buf[n].events;
	#elif defined(USE_POLL)
		revents = xtn->ev.buf[n].revents;
	#elif defined(USE_SELECT)
		revents = xtn->ev.buf[n].events;
	#else
		revents = 0; /* TODO: fake. unsupported but to compile on such an unsupported system.*/
	#endif

		mask = 0;
		if (revents & XPOLLIN) mask |= MOO_SEMAPHORE_IO_MASK_INPUT;
		if (revents & XPOLLOUT) mask |= MOO_SEMAPHORE_IO_MASK_OUTPUT;
		if (revents & XPOLLERR) mask |= MOO_SEMAPHORE_IO_MASK_ERROR;
		if (revents & XPOLLHUP) mask |= MOO_SEMAPHORE_IO_MASK_HANGUP;

	#if defined(USE_DEVPOLL)
		muxwcb (moo, xtn->ev.buf[n].fd, mask);
	#elif defined(USE_EPOLL)
		muxwcb (moo, xtn->ev.buf[n].data.fd, mask);
	#elif defined(USE_POLL)
		muxwcb (moo, xtn->ev.buf[n].fd, mask);
	#elif defined(USE_SELECT)
		muxwcb (moo, xtn->ev.buf[n].fd, mask);
	#endif
	}

	xtn->ev.len = 0;
#endif  /* USE_THREAD */
}

static void vm_sleep (moo_t* moo, const moo_ntime_t* dur)
{
#if defined(_WIN32)
	xtn_t* xtn = moo_getxtn(moo);
	if (xtn->waitable_timer)
	{
		LARGE_INTEGER li;
		li.QuadPart = -MOO_SECNSEC_TO_NSEC(dur->sec, dur->nsec);
		if(SetWaitableTimer(xtn->waitable_timer, &li, 0, MOO_NULL, MOO_NULL, FALSE) == FALSE) goto normal_sleep;
		WaitForSingleObject(xtn->waitable_timer, INFINITE);
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
	#elif defined(HAVE_NANOSLEEP)
		struct timespec ts;
		ts.tv_sec = dur->sec;
		ts.tv_nsec = dur->nsec;
		nanosleep (&ts, MOO_NULL);
	#elif defined(HAVE_USLEEP)
		usleep (MOO_SECNSEC_TO_USEC(dur->sec, dur->nsec));
	#else
	#	error UNSUPPORT SLEEP
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

#elif defined(_WIN32)
static HANDLE g_tick_timer = MOO_NULL; /*INVALID_HANDLE_VALUE;*/

static VOID CALLBACK arrange_process_switching (LPVOID arg, DWORD timeLow, DWORD timeHigh) 
{
	if (g_moo) moo_switchprocess (g_moo);
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
	/* TODO: */	

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

	/* TODO: */	
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

static void fini_moo (moo_t* moo)
{
	xtn_t* xtn = moo_getxtn(moo);
	if (xtn->logfd >= 0)
	{
		close (xtn->logfd);
		xtn->logfd = -1;
		xtn->logfd_istty = 0;
	}
}

static int handle_logopt (moo_t* moo, const moo_bch_t* str)
{
	xtn_t* xtn = moo_getxtn (moo);
	moo_bch_t* xstr = (moo_bch_t*)str;
	moo_bch_t* cm, * flt;
	moo_bitmask_t logmask;

	cm = moo_find_bchar_in_bcstr (xstr, ',');
	if (cm) 
	{
		/* i duplicate this string for open() below as open() doesn't 
		 * accept a length-bounded string */
		xstr = moo_dupbchars (moo, str, moo_count_bcstr(str));
		if (!xstr) 
		{
			fprintf (stderr, "ERROR: out of memory in duplicating %s\n", str);
			return -1;
		}

		cm = moo_find_bchar_in_bcstr(xstr, ',');
		*cm = '\0';

		logmask = xtn->logmask;
		do
		{
			flt = cm + 1;

			cm = moo_find_bchar_in_bcstr(flt, ',');
			if (cm) *cm = '\0';

			if (moo_comp_bcstr(flt, "app") == 0) logmask |= MOO_LOG_APP;
			else if (moo_comp_bcstr(flt, "compiler") == 0) logmask |= MOO_LOG_COMPILER;
			else if (moo_comp_bcstr(flt, "vm") == 0) logmask |= MOO_LOG_VM;
			else if (moo_comp_bcstr(flt, "mnemonic") == 0) logmask |= MOO_LOG_MNEMONIC;
			else if (moo_comp_bcstr(flt, "gc") == 0) logmask |= MOO_LOG_GC;
			else if (moo_comp_bcstr(flt, "ic") == 0) logmask |= MOO_LOG_IC;
			else if (moo_comp_bcstr(flt, "primitive") == 0) logmask |= MOO_LOG_PRIMITIVE;

			else if (moo_comp_bcstr(flt, "fatal") == 0) logmask |= MOO_LOG_FATAL;
			else if (moo_comp_bcstr(flt, "error") == 0) logmask |= MOO_LOG_ERROR;
			else if (moo_comp_bcstr(flt, "warn") == 0) logmask |= MOO_LOG_WARN;
			else if (moo_comp_bcstr(flt, "info") == 0) logmask |= MOO_LOG_INFO;
			else if (moo_comp_bcstr(flt, "debug") == 0) logmask |= MOO_LOG_DEBUG;

			else if (moo_comp_bcstr(flt, "fatal+") == 0) logmask |= MOO_LOG_FATAL;
			else if (moo_comp_bcstr(flt, "error+") == 0) logmask |= MOO_LOG_FATAL | MOO_LOG_ERROR;
			else if (moo_comp_bcstr(flt, "warn+") == 0) logmask |= MOO_LOG_FATAL | MOO_LOG_ERROR | MOO_LOG_WARN;
			else if (moo_comp_bcstr(flt, "info+") == 0) logmask |= MOO_LOG_FATAL | MOO_LOG_ERROR | MOO_LOG_WARN | MOO_LOG_INFO;
			else if (moo_comp_bcstr(flt, "debug+") == 0) logmask |= MOO_LOG_FATAL | MOO_LOG_ERROR | MOO_LOG_WARN | MOO_LOG_INFO | MOO_LOG_DEBUG;

			else
			{
				fprintf (stderr, "ERROR: unknown log option value - %s\n", flt);
				if (str != xstr) moo_freemem (moo, xstr);
				return -1;
			}
		}
		while (cm);


		if (!(logmask & MOO_LOG_ALL_TYPES)) logmask |= MOO_LOG_ALL_TYPES;  /* no types specified. force to all types */
		if (!(logmask & MOO_LOG_ALL_LEVELS)) logmask |= MOO_LOG_ALL_LEVELS;  /* no levels specified. force to all levels */
	}
	else
	{
		logmask = MOO_LOG_ALL_LEVELS | MOO_LOG_ALL_TYPES;
	}

	xtn->logfd = open(xstr, O_CREAT | O_WRONLY | O_APPEND , 0644);
	if (xtn->logfd == -1)
	{
		fprintf (stderr, "ERROR: cannot open a log file %s\n", xstr);
		if (str != xstr) moo_freemem (moo, xstr);
		return -1;
	}

	xtn->logmask = logmask;
#if defined(HAVE_ISATTY)
	xtn->logfd_istty = isatty(xtn->logfd);
#endif

	if (str != xstr) moo_freemem (moo, xstr);
	return 0;
}

#if defined(MOO_BUILD_DEBUG)
static int handle_dbgopt (moo_t* moo, const moo_bch_t* str)
{
	xtn_t* xtn = moo_getxtn (moo);
	const moo_bch_t* cm, * flt;
	moo_oow_t len;
	moo_bitmask_t trait, dbgopt = 0;

	cm = str - 1;
	do
	{
		flt = cm + 1;

		cm = moo_find_bchar_in_bcstr(flt, ',');
		len = cm? (cm - flt): moo_count_bcstr(flt);
		if (moo_comp_bchars_bcstr (flt, len, "gc") == 0)  dbgopt |= MOO_DEBUG_GC;
		else if (moo_comp_bchars_bcstr (flt, len, "bigint") == 0)  dbgopt |= MOO_DEBUG_BIGINT;
		else
		{
			fprintf (stderr, "ERROR: unknown debug option value - %.*s\n", (int)len, flt);
			return -1;
		}
	}
	while (cm);

	moo_getoption (moo, MOO_TRAIT, &trait);
	trait |= dbgopt;
	moo_setoption (moo, MOO_TRAIT, &trait);
	return 0;
}
#endif


#define MIN_MEMSIZE 2048000ul

int main (int argc, char* argv[])
{
	static moo_ooch_t str_my_object[] = { 'M', 'y', 'O', 'b','j','e','c','t' }; /*TODO: make this an argument */
	static moo_ooch_t str_main[] = { 'm', 'a', 'i', 'n' };

	moo_t* moo;
	xtn_t* xtn;
	moo_oocs_t objname;
	moo_oocs_t mthname;
	moo_vmprim_t vmprim;
	moo_cb_t moocb;
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

	const char* logopt = MOO_NULL;
	moo_oow_t memsize = MIN_MEMSIZE;
	int large_pages = 0;

#if defined(MOO_BUILD_DEBUG)
	const char* dbgopt = MOO_NULL;
#endif

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

	while ((c = moo_getbopt (argc, argv, &opt)) != MOO_BCI_EOF)
	{
		switch (c)
		{
			case 'l':
				logopt = opt.arg;
				break;

			case 'm':
				memsize = strtoul(opt.arg, MOO_NULL, 0);
				if (memsize <= MIN_MEMSIZE) memsize = MIN_MEMSIZE;
				break;

			case '\0':
				if (moo_comp_bcstr(opt.lngopt, "large-pages") == 0)
				{
					large_pages = 1;
					break;
				}
			#if defined(MOO_BUILD_DEBUG)
				else if (moo_comp_bcstr(opt.lngopt, "debug") == 0)
				{
					dbgopt = opt.arg;
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

	memset (&vmprim, 0, MOO_SIZEOF(vmprim));
	if (large_pages)
	{
		vmprim.alloc_heap = alloc_heap;
		vmprim.free_heap = free_heap;
	}
	vmprim.log_write = log_write;
	vmprim.syserrstrb = syserrstrb;
	vmprim.dl_open = dl_open;
	vmprim.dl_close = dl_close;
	vmprim.dl_getsym = dl_getsym;
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

	moo = moo_open (&sys_mmgr, MOO_SIZEOF(xtn_t), memsize, &vmprim, MOO_NULL);
	if (!moo)
	{
		fprintf (stderr, "ERROR: cannot open moo\n");
	#if defined(USE_LTDL)
		lt_dlexit ();
	#endif
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
		moo_bitmask_t trait = 0;

		/*trait |= MOO_NOGC;*/
		trait |= MOO_AWAIT_PROCS;
		moo_setoption (moo, MOO_TRAIT, &trait);

		/* disable GC logs */
		trait = ~MOO_LOG_GC;
		moo_setoption (moo, MOO_LOG_MASK, &trait);
	}

	xtn = moo_getxtn (moo);
	xtn->logfd = -1;
	xtn->logfd_istty = 0;

	memset (&moocb, 0, MOO_SIZEOF(moocb));
	moocb.fini = fini_moo;
	moo_regcb (moo, &moocb);

	if (logopt)
	{
		if (handle_logopt (moo, logopt) <= -1) 
		{
			moo_close (moo);
		#if defined(USE_LTDL)
			lt_dlexit ();
		#endif
			return -1;
		}
	}
	else
	{
		/* default logging mask when no logging option is set */
		xtn->logmask = MOO_LOG_ALL_TYPES | MOO_LOG_ERROR | MOO_LOG_FATAL;
	}

#if defined(MOO_BUILD_DEBUG)
	if (dbgopt)
	{
		if (handle_dbgopt (moo, dbgopt) <= -1)
		{
			moo_close (moo);
		#if defined(USE_LTDL)
			lt_dlexit ();
		#endif
			return -1;
		}
	}
#endif

	if (moo_ignite(moo) <= -1)
	{
		moo_logbfmt (moo, MOO_LOG_STDERR, "ERROR: cannot ignite moo - [%d] %js\n", moo_geterrnum(moo), moo_geterrstr(moo));
		moo_close (moo);
	#if defined(USE_LTDL)
		lt_dlexit ();
	#endif
		return -1;
	}

#if defined(macintosh)
	i = 20;
	xtn->source_path = "test.moo";
	goto compile;
#endif

	for (i = opt.ind; i < argc; i++)
	{
		xtn->source_path = argv[i];

	compile:
		if (moo_compile(moo, input_handler) <= -1)
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
					moo_logbfmt (moo, MOO_LOG_STDERR, "%s", xtn->source_path);
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

#if defined(USE_LTDL)
	lt_dlexit ();
#endif

	return xret;
}
