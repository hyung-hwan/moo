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

static moo_pfrc_t pf_connect (moo_t* moo, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd, oldfl, n;
	moo_errnum_t errnum;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));

	fd = MOO_OOP_TO_SMOOI(sck->handle);

	oldfl = fcntl(fd, F_GETFL, 0);
	if (oldfl == -1 || fcntl(fd, F_SETFL, oldfl | O_NONBLOCK) == -1) goto oops_syserr;

{

struct sockaddr_in sin;
memset (&sin, 0, sizeof(sin));
sin.sin_family = AF_INET;
sin.sin_addr.s_addr = inet_addr ("1.234.53.142");
sin.sin_port = htons(12345);
	do
	{
		n = connect(fd, (struct sockaddr*)&sin, sizeof(sin));
	}
	while (n == -1 && errno == EINTR);
}

	if (n == -1 && errno != EINPROGRESS)
	{
		fcntl (fd, F_SETFL, oldfl);
		goto oops_syserr;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

oops_syserr:
	errnum = moo_syserr_to_errnum(errno);

oops:
	moo_seterrnum (moo, errnum);
	return MOO_PF_FAILURE;
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
	{ I, { 'c','l','o','s','e','\0' },          0, { pf_close_socket,    0, 0  }  },
	{ I, { 'c','o','n','n','e','c','t','\0' },  0, { pf_connect,         3, 3  }  },
	{ I, { 'o','p','e','n','\0' },              0, { pf_open_socket,     3, 3  }  },
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
