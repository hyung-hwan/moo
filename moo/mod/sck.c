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

#include "_sck.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

static moo_pfrc_t pf_open_socket (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_t dom, type, proto;
	int fd = -1;
	moo_errnum_t errnum;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t))
	);

	dom = MOO_STACK_GETARG(moo, nargs, 0);
	type = MOO_STACK_GETARG(moo, nargs, 1);
	proto = MOO_STACK_GETARG(moo, nargs, 2);

	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_SMOOI(dom) && MOO_OOP_IS_SMOOI(type) && MOO_OOP_IS_SMOOI(proto));

	fd = socket (MOO_OOP_TO_SMOOI(dom), MOO_OOP_TO_SMOOI(type), MOO_OOP_TO_SMOOI(proto));
	if (fd == -1) 
	{
		moo_seterrwithsyserr (moo, errno);
		goto oops;
	}

	if (!MOO_IN_SMOOI_RANGE(fd))
	{
		/* the file descriptor is too big to be represented as a small integer */
		moo_seterrbfmt (moo, MOO_ERANGE, "socket handle %d not in the permitted range", fd);
		goto oops;
	}

	sck->handle = MOO_SMOOI_TO_OOP(fd);
	MOO_STACK_SETRETTORCV (moo, nargs);

	return MOO_PF_SUCCESS;

oops:
	if (fd >= 0) close (fd);
	return MOO_PF_FAILURE;
}

static moo_pfrc_t pf_close_socket (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) && 
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle)
	);

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd >= 0)
	{
		if (close(MOO_OOP_TO_SMOOI(sck->handle)) == -1)
		{
			moo_seterrwithsyserr (moo, errno);
			return MOO_PF_FAILURE;
		}
		else
		{
			sck->handle = MOO_SMOOI_TO_OOP(-1);
			MOO_STACK_SETRETTORCV (moo, nargs);
			return MOO_PF_SUCCESS;
		}
	}

	moo_seterrbfmt (moo, MOO_EBADHND, "bad socket handle - %O", sck->handle);
	return MOO_PF_FAILURE;
}

static moo_pfrc_t pf_bind_socket (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd, n;


	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));

	fd = MOO_OOP_TO_SMOOI(sck->handle);

#if 0
	n = bind(fd, &sin, MOO_SIZEOF(sin));
	if (n == -1)
	{
		moo_seterrwithsyserr (moo, errno);
		return MOO_PF_FAILURE;
	}
#endif

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_connect (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd, oldfl, n;
	moo_errnum_t errnum;
	moo_oop_t arg;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_BYTE_POINTER(arg));

	fd = MOO_OOP_TO_SMOOI(sck->handle);

	oldfl = fcntl(fd, F_GETFL, 0);
	if (oldfl == -1 || fcntl(fd, F_SETFL, oldfl | O_NONBLOCK) == -1) goto oops_syserr;

	do
	{
		n = connect(fd, (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(arg), moo_sck_addr_len((sck_addr_t*)MOO_OBJ_GET_BYTE_SLOT(arg)));
	}
	while (n == -1 && errno == EINTR);


	if (n == -1 && errno != EINPROGRESS)
	{
		fcntl (fd, F_SETFL, oldfl);
		goto oops_syserr;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

oops_syserr:
	moo_seterrwithsyserr (moo, errno);
	return MOO_PF_FAILURE;

oops:
	moo_seterrnum (moo, errnum);
	return MOO_PF_FAILURE;
}

static moo_pfrc_t pf_end_connect (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) && 
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle)
	);

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd >= 0)
	{
		socklen_t len;
		int ret;

		len = MOO_SIZEOF(ret);
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == -1)
		{
			moo_seterrwithsyserr (moo, errno);
			return MOO_PF_FAILURE;
		}

		if (ret == EINPROGRESS)
		{
			return MOO_PF_FAILURE;
		}
		else if (ret != 0)
		{
			moo_seterrwithsyserr (moo, ret);
			return MOO_PF_FAILURE;
		}
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_read_socket (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_byte_t buf;
	int fd;
	ssize_t n;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) && 
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle)
	);

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d\n", fd);
		return MOO_PF_FAILURE;
	}

	buf = (moo_oop_byte_t)MOO_STACK_GETARG (moo, nargs, 0);
	if (!MOO_OBJ_IS_BYTE_POINTER(buf))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "buffer not a byte array - %O\n", buf);
		return MOO_PF_FAILURE;
	}

	n = recv (fd, MOO_OBJ_GET_BYTE_SLOT(buf), MOO_OBJ_GET_SIZE(buf), 0);
	if (n <= -1 && errno != EWOULDBLOCK)
	{
		moo_seterrwithsyserr (moo, errno);
		return MOO_PF_FAILURE;
	}

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(n));

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_write_socket (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_byte_t buf;
	int fd;
	ssize_t n;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) && 
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle)
	);

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d\n", fd);
		return MOO_PF_FAILURE;
	}

	buf = (moo_oop_byte_t)MOO_STACK_GETARG (moo, nargs, 0);
	if (!MOO_OBJ_IS_BYTE_POINTER(buf))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "buffer not a byte array - %O\n", buf);
		return MOO_PF_FAILURE;
	}

	n = send (fd, MOO_OBJ_GET_BYTE_SLOT(buf), MOO_OBJ_GET_SIZE(buf), 0);
	if (n <= -1 && errno != EWOULDBLOCK)
	{
		moo_seterrwithsyserr (moo, errno);
		return MOO_PF_FAILURE;
	}

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(n));

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}

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
	{ I, { 'b','i','n','d','\0' },                              0, { pf_bind_socket,     1, 1  }  },
	{ I, { 'c','l','o','s','e','\0' },                          0, { pf_close_socket,    0, 0  }  },
	{ I, { 'c','o','n','n','e','c','t','\0' },                  0, { pf_connect,         1, 1  }  },
	{ I, { 'e','n','d','C','o','n','n','e','c','t','\0' },      0, { pf_end_connect,     0, 0  }  },
	{ I, { 'o','p','e','n','\0' },                              0, { pf_open_socket,     3, 3  }  },
	{ I, { 'r','e','a','d','B','y','t','e','s',':','\0' },      0, { pf_read_socket,     1, 1  }  },
	{ I, { 'w','r','i','t','e','B','y','t','e','s',':','\0' },  0, { pf_write_socket,    1, 1  }  },
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	/*if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(sck_t), MOO_NULL) <= -1) return -1;*/
	return 0;
}

static moo_pfbase_t* query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase (moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
}

int moo_mod_sck (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}


