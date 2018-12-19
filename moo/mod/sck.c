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

#include "_sck.h"

#if defined(HAVE_ACCEPT4)
#	define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

typedef struct sck_modctx_t sck_modctx_t;
struct sck_modctx_t
{
	moo_oop_class_t sck_class;
};

static moo_pfrc_t pf_open_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_t dom, type, proto;
	int fd = -1, typev;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t))
	);

	if (nargs == 1)
	{
		/* special form of opening. the socket handle is given */
		sck->handle = MOO_STACK_GETARG(moo, nargs, 0);
		MOO_STACK_SETRETTORCV (moo, nargs);
		return MOO_PF_SUCCESS;
	}
	
	dom = MOO_STACK_GETARG(moo, nargs, 0);
	type = MOO_STACK_GETARG(moo, nargs, 1);
	proto = (nargs < 3)? 0: MOO_STACK_GETARG(moo, nargs, 2);
	

	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_SMOOI(dom) && MOO_OOP_IS_SMOOI(type) && MOO_OOP_IS_SMOOI(proto));

	typev = MOO_OOP_TO_SMOOI(type);
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	typev |= SOCK_NONBLOCK | SOCK_CLOEXEC;
create_socket:
#endif

	fd = socket (MOO_OOP_TO_SMOOI(dom), typev, MOO_OOP_TO_SMOOI(proto));
	if (fd == -1) 
	{
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (typev & (SOCK_NONBLOCK | SOCK_CLOEXEC))) 
		{
			typev &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			goto create_socket;
		}
#endif
		moo_seterrwithsyserr (moo, 0, errno);
		goto oops;
	}

	if (!MOO_IN_SMOOI_RANGE(fd))
	{
		/* the file descriptor is too big to be represented as a small integer */
		moo_seterrbfmt (moo, MOO_ERANGE, "socket handle %d not in the permitted range", fd);
		goto oops;
	}
	
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	if (!(typev & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
#endif
	{
		int fl;

		fl = fcntl(fd, F_GETFL, 0);
		if (fl == -1)
		{
		fcntl_failure:
			moo_seterrwithsyserr (moo, 0, errno);
			goto oops;
		}

		fl |= O_NONBLOCK;
	#if defined(O_CLOEXEC)
		fl |= O_CLOEXEC;
	#endif

		if (fcntl(fd, F_SETFL, fl) == -1) goto fcntl_failure;
	}

	sck->handle = MOO_SMOOI_TO_OOP(fd);
	MOO_STACK_SETRETTORCV (moo, nargs);

	return MOO_PF_SUCCESS;

oops:
	if (fd >= 0) close (fd);
	return MOO_PF_FAILURE;
}

static moo_pfrc_t pf_close_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
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
		moo_releaseiohandle (moo, MOO_OOP_TO_SMOOI(sck->handle));
		if (close(MOO_OOP_TO_SMOOI(sck->handle)) == -1)
		{
			moo_seterrwithsyserr (moo, 0, errno);
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

static moo_pfrc_t pf_bind_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_t arg;
	int fd, enable;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_BYTE_POINTER(arg));

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	enable = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, MOO_SIZEOF(int)) == -1 ||
	    bind(fd, (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(arg), moo_sck_addr_len((sck_addr_t*)MOO_OBJ_GET_BYTE_SLOT(arg))) == -1)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_accept_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck, newsck;
	moo_oop_t arg;
	sck_len_t addrlen;
	int fd, newfd, fl;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_BYTE_POINTER(arg));

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	addrlen = MOO_OBJ_GET_SIZE(arg);
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC) && defined(HAVE_ACCEPT4)
	newfd = accept4(fd, (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(arg), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (newfd == -1)
	{
		if (errno == ENOSYS) goto normal_accept;
		
		if (errno != EWOULDBLOCK && errno != EAGAIN) 
		{
			moo_seterrwithsyserr (moo, 0, errno);
			return MOO_PF_FAILURE;
		}

		/* return nil if accept() is not ready to accept a socket */
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
		return MOO_PF_SUCCESS;
	}
	else
	{
		goto accept_done;
	}
normal_accept:
#endif

	newfd = accept(fd, (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(arg), &addrlen);
	if (newfd == -1)
	{
		if (errno != EWOULDBLOCK && errno != EAGAIN)
		{
			moo_seterrwithsyserr (moo, 0, errno);
			return MOO_PF_FAILURE;
		}


		/* return nil if accept() is not ready to accept a socket */
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
		return MOO_PF_SUCCESS;
	}
	fl = fcntl(newfd, F_GETFL, 0);
	if (fl == -1)
	{
	fcntl_failure:
		moo_seterrwithsyserr (moo, 0, errno);
		close (newfd);
		return MOO_PF_FAILURE;
	}

	fl |= O_NONBLOCK;
#if defined(O_CLOEXEC)
	fl |= O_CLOEXEC;
#endif
	if (fcntl(newfd, F_SETFL, fl) == -1) goto fcntl_failure;
	
#if 0
accept_done:
	/*newsck = (oop_sck_t)moo_instantiate(moo, MOO_OBJ_GET_CLASS(sck), MOO_NULL, 0);*/
	newsck = (oop_sck_t)moo_instantiate(moo, ((sck_modctx_t*)mod->ctx)->sck_class, MOO_NULL, 0);
	if (!newsck) 
	{
		close (newfd);
		return MOO_PF_FAILURE;
	}

	if (!MOO_IN_SMOOI_RANGE(newfd)) 
	{
		/* the file descriptor is too big to be represented as a small integer */
		moo_seterrbfmt (moo, MOO_ERANGE, "socket handle %d not in the permitted range", newfd);
		close (newfd);
		return MOO_PF_FAILURE;
	}
	newsck->handle = MOO_SMOOI_TO_OOP(newfd);

	/* return the partially initialized socket object. the handle field is set to the new file
	 * descriptor. however all other fields are just set to nil. so the user of this primitive
	 * method should call application-level initializer. */
	MOO_STACK_SETRET (moo, nargs, (moo_oop_t)newsck);
	return MOO_PF_SUCCESS;
#else
accept_done:
	if (!MOO_IN_SMOOI_RANGE(newfd)) 
	{
		/* the file descriptor is too big to be represented as a small integer */
		moo_seterrbfmt (moo, MOO_ERANGE, "socket handle %d not in the permitted range", newfd);
		close (newfd);
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(newfd));
	return MOO_PF_SUCCESS;
#endif
}

static moo_pfrc_t pf_listen_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_t arg;
	int fd, n;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));/*newsck = (oop_sck_t)moo_instantiate(moo, MOO_OBJ_GET_CLASS(sck), MOO_NULL, 0);*/
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_SMOOI(arg));

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	n = listen(fd, MOO_OOP_TO_SMOOI(arg));
	if (n == -1)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_connect_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd, n;
	moo_oop_t arg;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo,
		MOO_OOP_IS_POINTER(sck) &&
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_BYTE_POINTER(arg));

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	n = connect(fd, (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(arg), moo_sck_addr_len((sck_addr_t*)MOO_OBJ_GET_BYTE_SLOT(arg))); 
	if (n == -1)
	{
		if (errno == EINPROGRESS)
		{
			/* have the primitive function to return -1 */
			MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(-1));
			return MOO_PF_SUCCESS;
		}
		else
		{
			moo_seterrwithsyserr (moo, 0, errno);
			return MOO_PF_FAILURE;
		}
	}

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(0));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_socket_error (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	int fd, ret;
	sck_len_t len;

	sck = (oop_sck_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, 
		MOO_OOP_IS_POINTER(sck) && 
		MOO_OBJ_BYTESOF(sck) >= (MOO_SIZEOF(*sck) - MOO_SIZEOF(moo_obj_t)) &&
		MOO_OOP_IS_SMOOI(sck->handle)
	);

	fd = MOO_OOP_TO_SMOOI(sck->handle);
	if (fd <= -1)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "bad socket handle - %d", fd);
		return MOO_PF_FAILURE;
	}

	len = MOO_SIZEOF(ret);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&ret, &len) == -1)
	{
		moo_seterrwithsyserr (moo, 0, errno);
		return MOO_PF_FAILURE;
	}

	if (ret == EINPROGRESS) ret = -1; /* map EINPROGRESS to -1. all others are returned without change */

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(ret));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_read_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_byte_t buf;
	moo_oow_t offset, length, maxlen;
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

	/* NOTE: on EWOULDBLOCK or EGAIN, -1 is returned  */

	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(n));

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_write_socket (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	oop_sck_t sck;
	moo_oop_byte_t buf;
	moo_oow_t offset, length, maxlen;
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

	/* NOTE: on EWOULDBLOCK or EGAIN, -1 is returned  */
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
	{ I, { 'a','c','c','e','p','t',':','\0' },                    0, { pf_accept_socket,    1, 1  }  },
	{ I, { 'b','i','n','d',':','\0' },                            0, { pf_bind_socket,      1, 1  }  },
	{ I, { 'c','l','o','s','e','\0' },                            0, { pf_close_socket,     0, 0  }  },
	{ I, { 'c','o','n','n','e','c','t',':','\0' },                0, { pf_connect_socket,   1, 1  }  },
	{ I, { 'l','i','s','t','e','n',':','\0' },                    0, { pf_listen_socket,    1, 1  }  },
	{ I, { 'o','p','e','n','\0' },                                0, { pf_open_socket,      1, 3  }  },
	{ I, { 'r','e','a','d','B','y','t','e','s','I','n','t','o',':','\0' },        0, { pf_read_socket,      1, 1  }  },
	{ I, { 'r','e','a','d','B','y','t','e','s','I','n','t','o',':','o','f','f','s','e','t',':','l','e','n','g','t','h',':','\0' },    0, { pf_read_socket,     3, 3  }  },
	{ I, { 's','o','c','k','e','t','E','r','r','o','r','\0' },    0, { pf_get_socket_error, 0, 0  }  },
	{ I, { 'w','r','i','t','e','B','y','t','e','s','F','r','o','m',':','\0' },    0, { pf_write_socket,     1, 1  }  },
	{ I, { 'w','r','i','t','e','B','y','t','e','s','F','r','o','m',':','o','f','f','s','e','t',':','l','e','n','g','t','h',':','\0' },    0, { pf_write_socket,     3, 3  }  }
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	/*if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(sck_t), MOO_NULL) <= -1) return -1;*/
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

static void gc_mod_sck (moo_t* moo, moo_mod_t* mod)
{
	sck_modctx_t* ctx = mod->ctx;

	MOO_ASSERT (moo, ctx != MOO_NULL);
	if (ctx->sck_class)	
	{
		ctx->sck_class = (moo_oop_class_t)moo_moveoop(moo, (moo_oop_t)ctx->sck_class);
	}
}

int moo_mod_sck (moo_t* moo, moo_mod_t* mod)
{
/*
	if (mod->hints & MOO_MOD_LOAD_FOR_IMPORT)
	{
		mod->gc = MOO_NULL;
		mod->ctx = MOO_NULL;
	}
	else
	{
		sck_modctx_t* ctx;

		static moo_ooch_t name_sck[] = { 'S','o','c','k','e','t','\0' };

		ctx = moo_callocmem(moo, MOO_SIZEOF(*ctx));
		if (!ctx) return -1;

		ctx->sck_class = (moo_oop_class_t)moo_findclass(moo, moo->sysdic, name_sck);
		if (!ctx->sck_class)
		{
			MOO_DEBUG0 (moo, "Socket class not found\n");
			moo_freemem (moo, ctx);
			return -1;
		}

		mod->gc = gc_mod_sck;
		mod->ctx = ctx;
	}
*/

	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	return 0;
}
