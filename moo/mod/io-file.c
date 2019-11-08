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
 
#include "_io.h"
#include "../lib/moo-prv.h"
#include "../lib/moo-utl.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>


#if !defined(O_CLOEXEC)
#	define O_CLOEXEC 0 /* since it's not defined, 0 results in no effect when bitwise-ORed. */
#endif

static moo_pfrc_t pf_open_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_io_t io;
	moo_oop_t path, flags, mode;
	int fd = -1, fl;
	moo_bch_t* tmp;

	io = (oop_io_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(io) &&
		MOO_OBJ_BYTESOF(io) >= (MOO_SIZEOF(*io) - MOO_SIZEOF(moo_obj_t))
	);

	if (nargs == 1)
	{
		/* special form of opening. the socket handle is given */
		io->handle = MOO_STACK_GETARG(moo, nargs, 0);
		MOO_STACK_SETRETTORCV (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	path = MOO_STACK_GETARG(moo, nargs, 0);
	flags = MOO_STACK_GETARG(moo, nargs, 1);
	mode = (nargs < 3)? MOO_SMOOI_TO_OOP(0644): MOO_STACK_GETARG(moo, nargs, 2);

/* TODO: always set O_LARGEFILE on flags if necessary */

	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_CHAR_POINTER(path) && MOO_OOP_IS_SMOOI(flags) && MOO_OOP_IS_SMOOI(mode));
	MOO_PF_CHECK_ARGS (moo, nargs, moo_count_oocstr(MOO_OBJ_GET_CHAR_SLOT(path)) == MOO_OBJ_GET_SIZE(path));

#if defined(MOO_OOCH_IS_UCH)
	tmp = moo_dupootobcstr(moo, MOO_OBJ_GET_CHAR_SLOT(path), MOO_NULL);
	if (!tmp) goto oops;
	fd = open(tmp, MOO_OOP_TO_SMOOI(flags), MOO_OOP_TO_SMOOI(mode));
	moo_freemem (moo, tmp);
#else
	fd = open(MOO_OBJ_GET_CHAR_SLOT(path), MOO_OOP_TO_SMOOI(flags), MOO_OOP_TO_SMOOI(mode));
#endif
	if (fd == -1)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		goto oops;
	}

	if (!MOO_IN_SMOOI_RANGE(fd))
	{
		/* the io descriptor is too big to be represented as a small integer */
		moo_seterrbfmt (moo, MOO_ERANGE, "IO handle %d not in the permitted range", fd);
		goto oops;
	}

#if defined(O_NONBLOCK) && defined(O_CLOEXEC)
	fl = fcntl(fd, F_GETFL, 0);
	if (fl == -1)
	{
	fcntl_failure:
		moo_seterrwithsyserr (moo, 0, errno);
		goto oops;
	}

	fl |= O_NONBLOCK | O_CLOEXEC;

	if (fcntl(fd, F_SETFL, fl) == -1) goto fcntl_failure;
#endif

	io->handle = MOO_SMOOI_TO_OOP(fd);
	MOO_STACK_SETRETTORCV (moo, nargs);

	return MOO_PF_SUCCESS;

oops:
	if (fd >= 0) close (fd);
	return MOO_PF_FAILURE;
}

#define RCV_TO_FD(moo, nargs, fd_) \
	do \
	{ \
		oop_io_t rcv_; \
		rcv_ = (oop_io_t)MOO_STACK_GETRCV(moo, nargs); \
		MOO_PF_CHECK_RCV (moo, \
			MOO_OOP_IS_POINTER(rcv_) &&  \
			MOO_OBJ_BYTESOF(rcv_) >= (MOO_SIZEOF(*rcv_) - MOO_SIZEOF(moo_obj_t)) && \
			MOO_OOP_IS_SMOOI(rcv_->handle) \
		); \
		fd_ = MOO_OOP_TO_SMOOI(rcv_->handle); \
		if (fd_ <= -1) \
		{ \
			moo_seterrbfmt (moo, MOO_EINVAL, "bad IO handle - %d", fd_); \
			return MOO_PF_FAILURE; \
		} \
	} \
	while(0)

#define SETRET_WITH_SYSCALL(moo, nargs, n) \
	do { \
		int x_ = (n); \
		if (x_ <= -1) \
		{ \
			moo_seterrwithsyserr (moo, 0, errno); \
			return MOO_PF_FAILURE; \
		} \
		MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(x_)); \
	} \
	while(0)

static moo_pfrc_t pf_chmod_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#else
	moo_oop_t tmp;
	int fd;
	moo_oow_t mode;

	RCV_TO_FD (moo, nargs, fd);

	MOO_STATIC_ASSERT (MOO_TYPE_IS_UNSIGNED(mode_t));

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow_noseterr(moo, tmp, &mode) == 0 || mode > MOO_TYPE_MAX(mode_t))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid mode - %O", tmp);
		return MOO_PF_FAILURE;
	}

	SETRET_WITH_SYSCALL (moo, nargs, fchmod(fd, mode));
	return MOO_PF_SUCCESS;
#endif
}


static moo_pfrc_t pf_chown_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#else
	moo_oop_t tmp;
	int fd;
	moo_ooi_t uid, gid;

	RCV_TO_FD (moo, nargs, fd);

	MOO_STATIC_ASSERT (MOO_TYPE_IS_UNSIGNED(uid_t));
	MOO_STATIC_ASSERT (MOO_TYPE_IS_UNSIGNED(gid_t));

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttoooi_noseterr(moo, tmp, &uid) == 0 || uid > MOO_TYPE_MAX(uid_t))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid uid - %O", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttoooi_noseterr(moo, tmp, &gid) == 0 || gid > MOO_TYPE_MAX(gid_t))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid gid - %O", tmp);
		return MOO_PF_FAILURE;
	}

	if (uid <= -1) uid = (uid_t)-1;
	if (gid <= -1) gid = (gid_t)-1;

	SETRET_WITH_SYSCALL (moo, nargs, fchown(fd, uid, gid));
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_lock_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
#if defined(_WIN32)
	moo_seterrnum (moo, MOO_ENOIMPL);
	return MOO_PF_FAILURE;
#else
	moo_oop_t tmp;
	int fd;

	RCV_TO_FD (moo, nargs, fd);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (!MOO_OOP_IS_SMOOI(tmp))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid operation code - %O", tmp);
		return MOO_PF_FAILURE;
	}

	SETRET_WITH_SYSCALL (moo, nargs, flock(fd, MOO_OOP_TO_SMOOI(tmp)));
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_seek_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;
	moo_intmax_t offset; 
	moo_ooi_t whence;
	int fd;
	off_t n;

	RCV_TO_FD (moo, nargs, fd);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttointmax_noseterr(moo, tmp, &offset) == 0 || offset < MOO_TYPE_MIN(off_t) || offset > MOO_TYPE_MAX(off_t))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset - %O", tmp);
		return MOO_PF_FAILURE;
	}

	tmp = MOO_STACK_GETARG(moo, nargs, 1);
	if (moo_inttoooi_noseterr(moo, tmp, &whence) == 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid whence - %O", tmp);
		return MOO_PF_FAILURE;
	}

	n = lseek(fd, offset, whence);
	if (n == (off_t)-1)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	tmp = moo_uintmaxtoint(moo, (moo_uintmax_t)n);
	if (!tmp) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, tmp);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_truncate_file (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t tmp;
	moo_intmax_t size; 
	int fd, n;

	RCV_TO_FD (moo, nargs, fd);

	tmp = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttointmax_noseterr(moo, tmp, &size) == 0 || size < MOO_TYPE_MIN(off_t) || size > MOO_TYPE_MAX(off_t))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid size - %O", tmp);
		return MOO_PF_FAILURE;
	}

	SETRET_WITH_SYSCALL (moo, nargs, ftruncate(fd, size));
	return MOO_PF_SUCCESS;
}

/* TODO: posix_fallocate(), posix_fadvise(), fallocate() */

/* ------------------------------------------------------------------------ */

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

#define MA MOO_TYPE_MAX(moo_oow_t)

static moo_pfinfo_t pfinfos[] =
{
	{ I, "chmod:",           { pf_chmod_file,    1, 1  }  },
	{ I, "chown:group:",     { pf_chown_file,    2, 2  }  },
	{ I, "lock:",            { pf_lock_file,     1, 1  }  },
	{ I, "open:flags:",      { pf_open_file,     2, 2  }  },
	{ I, "open:flags:mode:", { pf_open_file,     3, 3  }  },
	{ I, "seek:whence:",     { pf_seek_file,     2, 2  }  },
	{ I, "truncate:",        { pf_truncate_file, 1, 1  }  }
};

#if defined(_WIN32)
#	define LOCK_EX 0
#	define LOCK_NB 0
#	define LOCK_SH 0
#	define LOCK_UN 0
#	define O_NOFOLLOW 0
#	define O_NONBLOCK 0
#endif
static moo_pvinfo_t pvinfos[] = 
{
/*
	{ "F_RDLCK",    { MOO_PV_OOI, (const void*)F_RDLCK } },
	{ "F_UNLCK",    { MOO_PV_OOI, (const void*)F_UNLCK } },
	{ "F_WRLCK",    { MOO_PV_OOI, (const void*)F_WRLCK } },
*/
	{ "LOCK_EX",    { MOO_PV_OOI, (const void*)LOCK_EX } },
	{ "LOCK_NB",    { MOO_PV_OOI, (const void*)LOCK_NB } },
	{ "LOCK_SH",    { MOO_PV_OOI, (const void*)LOCK_SH } },
	{ "LOCK_UN",    { MOO_PV_OOI, (const void*)LOCK_UN } },

	{ "O_APPEND",   { MOO_PV_OOI, (const void*)O_APPEND } },
	{ "O_CLOEXEC",  { MOO_PV_OOI, (const void*)O_CLOEXEC } },
	{ "O_CREAT",    { MOO_PV_OOI, (const void*)O_CREAT } },
	{ "O_EXCL",     { MOO_PV_OOI, (const void*)O_EXCL } },
	{ "O_NOFOLLOW", { MOO_PV_OOI, (const void*)O_NOFOLLOW } },
	{ "O_NONBLOCK", { MOO_PV_OOI, (const void*)O_NONBLOCK } },
	{ "O_RDONLY",   { MOO_PV_OOI, (const void*)O_RDONLY } },
	{ "O_RDWR",     { MOO_PV_OOI, (const void*)O_RDWR } },
	{ "O_TRUNC",    { MOO_PV_OOI, (const void*)O_TRUNC } },
	{ "O_WRONLY",   { MOO_PV_OOI, (const void*)O_WRONLY } },

	{ "SEEK_CUR",   { MOO_PV_OOI, (const void*)SEEK_CUR } },
	{ "SEEK_END",   { MOO_PV_OOI, (const void*)SEEK_SET } },
	{ "SEEK_SET",   { MOO_PV_OOI, (const void*)SEEK_SET } }
};
/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	return 0;
}

static moo_pfbase_t* querypf (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase(moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}

static moo_pvbase_t* querypv (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpvbase(moo, pvinfos, MOO_COUNTOF(pvinfos), name, namelen);
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
}

int moo_mod_io_file (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->querypf = querypf;
	mod->querypv = querypv;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}
