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

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef struct io_modctx_t io_modctx_t;
struct io_modctx_t
{
	moo_oop_class_t io_class;
};

static moo_pfrc_t pf_close_io (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_io_t io;
	int fd;

	io = (oop_io_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(io) && 
		MOO_OBJ_BYTESOF(io) >= (MOO_SIZEOF(*io) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(io->handle)
	);

	fd = MOO_OOP_TO_SMOOI(io->handle);
	if (fd >= 0)
	{
		moo_releaseiohandle (moo, MOO_OOP_TO_SMOOI(io->handle));
		if (close(MOO_OOP_TO_SMOOI(io->handle)) == -1)
		{
			moo_seterrwithsyserr (moo, 0, errno);
			return MOO_PF_FAILURE;
		}
		else
		{
			io->handle = MOO_SMOOI_TO_OOP(-1);
			MOO_STACK_SETRETTORCV (moo, nargs);
			return MOO_PF_SUCCESS;
		}
	}

	moo_seterrbfmt (moo, MOO_EBADHND, "bad IO handle - %O", io->handle);
	return MOO_PF_FAILURE;
}

#if 0
static moo_pfrc_t pf_read_io (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_io_t io;
	moo_oop_byte_t buf;
	moo_oow_t offset, length, maxlen;
	int fd;
	ssize_t n;

	io = (oop_io_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(io) && 
		MOO_OBJ_BYTESOF(io) >= (MOO_SIZEOF(*io) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(io->handle)
	);

	fd = MOO_OOP_TO_SMOOI(io->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	buf = (moo_oop_byte_t)MOO_STACK_GETARG (moo, nargs, 0);
	if (!MOO_OBJ_IS_BYTE_POINTER(buf))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "buffer not a byte array - %O", buf);
		return MOO_PF_FAILURE;
	}

	offset = 0;
	maxlen = MOO_OBJ_GET_SIZE(buf);
	length = maxlen;

	if (nargs >= 2)
	{
		moo_oop_t tmp;

		tmp = MOO_STACK_GETARG(moo, nargs, 1);
		if (moo_inttooow (moo, tmp, &offset) <= 0)
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset - %O", tmp);
			return MOO_PF_FAILURE;
		}

		if (nargs >= 3)
		{
			tmp = MOO_STACK_GETARG(moo, nargs, 2);
			if (moo_inttooow(moo, tmp, &length) <= 0)
			{
				moo_seterrbfmt (moo, MOO_EINVAL, "invalid length - %O", tmp);
				return MOO_PF_FAILURE;
			}
		}

		if (offset >= maxlen) offset = maxlen - 1;
		if (length > maxlen - offset) length = maxlen - offset;
	}

	n = recv(fd, MOO_OBJ_GET_BYTE_PTR(buf, offset), length, 0);
	if (n <= -1 && errno != EWOULDBLOCK && errno != EAGAIN)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	/* [NOTE] on EWOULDBLOCK or EGAIN, -1 is returned  */

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(n));

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_write_io (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_io_t io;
	moo_oop_byte_t buf;
	moo_oow_t offset, length, maxlen;
	int fd;
	ssize_t n;

	io = (oop_io_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(io) && 
		MOO_OBJ_BYTESOF(io) >= (MOO_SIZEOF(*io) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(io->handle)
	);

	fd = MOO_OOP_TO_SMOOI(io->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	buf = (moo_oop_byte_t)MOO_STACK_GETARG (moo, nargs, 0);
	if (!MOO_OBJ_IS_BYTE_POINTER(buf))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "buffer not a byte array - %O", buf);
		return MOO_PF_FAILURE;
	}

	offset = 0;
	maxlen = MOO_OBJ_GET_SIZE(buf);
	length = maxlen;

	if (nargs >= 2)
	{
		moo_oop_t tmp;

		tmp = MOO_STACK_GETARG(moo, nargs, 1);
		if (moo_inttooow (moo, tmp, &offset) <= 0)
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "invalid offset - %O", tmp);
			return MOO_PF_FAILURE;
		}

		if (nargs >= 3)
		{
			tmp = MOO_STACK_GETARG(moo, nargs, 2);
			if (moo_inttooow(moo, tmp, &length) <= 0)
			{
				moo_seterrbfmt (moo, MOO_EINVAL, "invalid length - %O", tmp);
				return MOO_PF_FAILURE;
			}
		}

		if (offset >= maxlen) offset = maxlen - 1;
		if (length > maxlen - offset) length = maxlen - offset;
	}

	n = send(fd, MOO_OBJ_GET_BYTE_PTR(buf, offset), length, 0);
	if (n <= -1 && errno != EWOULDBLOCK && errno != EAGAIN)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	/* [NOTE] on EWOULDBLOCK or EGAIN, -1 is returned  */
	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(n));

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}
#endif

/* ------------------------------------------------------------------------ */
 
typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	moo_method_type_t type;
	moo_ooch_t mthname[15];
	int variadic;
	moo_pfimpl_t handler;
};

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

#define MA MOO_TYPE_MAX(moo_oow_t)

static moo_pfinfo_t pfinfos[] =
{
	{ I, { 'c','l','o','s','e','\0' },                            0, { pf_close_io,     0, 0  }  },
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	/*if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(io_t), MOO_NULL) <= -1) return -1;*/
	return 0;
}

static moo_pfbase_t* query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase(moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
	if (mod->ctx) moo_freemem (moo, mod->ctx);
}

int moo_mod_io (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	return 0;
}
