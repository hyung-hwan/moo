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
#include "../lib/moo-prv.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#if defined(HAVE_NETINET_IN_H)
#	include <netinet/in.h>
#endif
#if defined(HAVE_SYS_UN_H)
#	include <sys/un.h>
#endif
#if defined(HAVE_NETPACKET_PACKET_H)
#	include <netpacket/packet.h>
#endif
#if defined(HAVE_NET_IF_DL_H)
#	include <net/if_dl.h>
#endif

#include <arpa/inet.h>
#include <string.h>


#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

#define MA MOO_TYPE_MAX(moo_oow_t)

union sockaddr_t
{
	struct sockaddr    sa;
#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN > 0)
	struct sockaddr_in in4;
#endif
#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
	struct sockaddr_in6 in6;
#endif
#if (MOO_SIZEOF_STRUCT_SOCKADDR_LL > 0)
	struct sockaddr_ll ll;
#endif
#if (MOO_SIZEOF_STRUCT_SOCKADDR_UN > 0)
	struct sockaddr_un un;
#endif
};
typedef union sockaddr_t sockaddr_t;

static int str_to_ipv4 (const moo_ooch_t* str, moo_oow_t len, struct in_addr* inaddr)
{
	const moo_ooch_t* end;
	int dots = 0, digits = 0;
	moo_uint32_t acc = 0, addr = 0;
	moo_ooch_t c;

	end = str + len;

	do
	{
		if (str >= end)
		{
			if (dots < 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			break;
		}

		c = *str++;

		if (c >= '0' && c <= '9') 
		{
			if (digits > 0 && acc == 0) return -1;
			acc = acc * 10 + (c - '0');
			if (acc > 255) return -1;
			digits++;
		}
		else if (c == '.') 
		{
			if (dots >= 3 || digits == 0) return -1;
			addr = (addr << 8) | acc;
			dots++; acc = 0; digits = 0;
		}
		else return -1;
	}
	while (1);

	inaddr->s_addr = moo_hton32(addr);
	return 0;

}

#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
static int str_to_ipv6 (const moo_ooch_t* src, moo_oow_t len, struct in6_addr* inaddr)
{
	moo_uint8_t* tp, * endp, * colonp;
	const moo_ooch_t* curtok;
	moo_ooch_t ch;
	int saw_xdigit;
	unsigned int val;
	const moo_ooch_t* src_end;

	src_end = src + len;

	MOO_MEMSET (inaddr, 0, MOO_SIZEOF(*inaddr));
	tp = &inaddr->s6_addr[0];
	endp = &inaddr->s6_addr[MOO_COUNTOF(inaddr->s6_addr)];
	colonp = MOO_NULL;

	/* Leading :: requires some special handling. */
	if (src < src_end && *src == ':')
	{
		src++;
		if (src >= src_end || *src != ':') return -1;
	}

	curtok = src;
	saw_xdigit = 0;
	val = 0;

	while (src < src_end)
	{
		int v1;

		ch = *src++;

		if (ch >= '0' && ch <= '9')
			v1 = ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			v1 = ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'f')
			v1 = ch - 'a' + 10;
		else v1 = -1;
		if (v1 >= 0)
		{
			val <<= 4;
			val |= v1;
			if (val > 0xffff) return -1;
			saw_xdigit = 1;
			continue;
		}

		if (ch == ':') 
		{
			curtok = src;
			if (!saw_xdigit) 
			{
				if (colonp) return -1;
				colonp = tp;
				continue;
			}
			else if (src >= src_end)
			{
				/* a colon can't be the last character */
				return -1;
			}

			*tp++ = (moo_uint8_t)(val >> 8) & 0xff;
			*tp++ = (moo_uint8_t)val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}

		if (ch == '.' && ((tp + MOO_SIZEOF(struct in_addr)) <= endp) &&
		    str_to_ipv4(curtok, src_end - curtok, (struct in_addr*)tp) == 0) 
		{
			tp += MOO_SIZEOF(struct in_addr*);
			saw_xdigit = 0;
			break; 
		}

		return -1;
	}

	if (saw_xdigit) 
	{
		if (tp + MOO_SIZEOF(moo_uint16_t) > endp) return -1;
		*tp++ = (moo_uint8_t)(val >> 8) & 0xff;
		*tp++ = (moo_uint8_t)val & 0xff;
	}
	if (colonp != MOO_NULL) 
	{
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		moo_oow_t n = tp - colonp;
		moo_oow_t i;
 
		for (i = 1; i <= n; i++) 
		{
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp) return -1;

	return 0;
}
#endif


static int str_to_sockaddr (moo_t* moo, const moo_ooch_t* str, moo_oow_t len, sockaddr_t* nwad)
{
	const moo_ooch_t* p;
	const moo_ooch_t* end;
	moo_oocs_t tmp;

	p = str;
	end = str + len;

	if (p >= end) 
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "blank address");
		return -1;
	}

	MOO_MEMSET (nwad, 0, MOO_SIZEOF(*nwad));

#if defined(AF_UNIX)
	if (*p == '/' && len >= 2)
	{
	#if defined(MOO_OOCH_IS_BCH)
		moo_copy_bcstr (nwad->un.sun_path, MOO_COUNTOF(nwad->un.sun_path), str);
	#else
		moo_oow_t dstlen;

		dstlen = MOO_COUNTOF(nwad->un.sun_path) - 1;
		if (moo_convutobchars (moo, p, &len, nwad->un.sun_path, &dstlen) <= -1) 
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "unable to convert encoding");
			return -1;
		}
		nwad->un.sun_path[dstlen] = '\0';
	#endif
		nwad->un.sun_family = AF_UNIX;
		return 0;
	}
#endif

#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
	if (*p == '[')
	{
		/* IPv6 address */
		tmp.ptr = (moo_ooch_t*)++p; /* skip [ and remember the position */
		while (p < end && *p != '%' && *p != ']') p++;

		if (p >= end) goto no_rbrack;

		tmp.len = p - tmp.ptr;
		if (*p == '%')
		{
			/* handle scope id */
			moo_uint32_t x;

			p++; /* skip % */

			if (p >= end)
			{
				/* premature end */
				moo_seterrbfmt (moo, MOO_EINVAL, "scope id blank");
				return -1;
			}

			if (*p >= '0' && *p <= '9') 
			{
				/* numeric scope id */
				nwad->in6.sin6_scope_id = 0;
				do
				{
					x = nwad->in6.sin6_scope_id * 10 + (*p - '0');
					if (x < nwad->in6.sin6_scope_id) 
					{
						moo_seterrbfmt (moo, MOO_EINVAL, "scope id too large");
						return -1; /* overflow */
					}
					nwad->in6.sin6_scope_id = x;
					p++;
				}
				while (p < end && *p >= '0' && *p <= '9');
			}
			else
			{
#if 0
TODO:
				/* interface name as a scope id? */
				const moo_ooch_t* stmp = p;
				unsigned int index;
				do p++; while (p < end && *p != ']');
				if (moo_nwifwcsntoindex (stmp, p - stmp, &index) <= -1) return -1;
				tmpad.u.in6.scope = index;
#endif
			}

			if (p >= end || *p != ']') goto no_rbrack;
		}
		p++; /* skip ] */

		if (str_to_ipv6(tmp.ptr, tmp.len, &nwad->in6.sin6_addr) <= -1) goto unrecog;
		nwad->in6.sin6_family = AF_INET6;
	}
	else
	{
#endif
		/* IPv4 address */
		tmp.ptr = (moo_ooch_t*)p;
		while (p < end && *p != ':') p++;
		tmp.len = p - tmp.ptr;

		if (str_to_ipv4(tmp.ptr, tmp.len, &nwad->in4.sin_addr) <= -1)
		{
		#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
			/* check if it is an IPv6 address not enclosed in []. 
			 * the port number can't be specified in this format. */
			if (p >= end || *p != ':') 
			{
				/* without :, it can't be an ipv6 address */
				goto unrecog;
			}


			while (p < end && *p != '%') p++;
			tmp.len = p - tmp.ptr;

			if (str_to_ipv6(tmp.ptr, tmp.len, &nwad->in6.sin6_addr) <= -1) goto unrecog;

			if (p < end && *p == '%')
			{
				/* handle scope id */
				moo_uint32_t x;

				p++; /* skip % */

				if (p >= end)
				{
					/* premature end */
					moo_seterrbfmt (moo, MOO_EINVAL, "scope id blank");
					return -1;
				}

				if (*p >= '0' && *p <= '9') 
				{
					/* numeric scope id */
					nwad->in6.sin6_scope_id = 0;
					do
					{
						x = nwad->in6.sin6_scope_id * 10 + (*p - '0');
						if (x < nwad->in6.sin6_scope_id) 
						{
							moo_seterrbfmt (moo, MOO_EINVAL, "scope id too large");
							return -1; /* overflow */
						}
						nwad->in6.sin6_scope_id = x;
						p++;
					}
					while (p < end && *p >= '0' && *p <= '9');
				}
				else
				{
#if 0
TODO
					/* interface name as a scope id? */
					const moo_ooch_t* stmp = p;
					unsigned int index;
					do p++; while (p < end);
					if (moo_nwifwcsntoindex (stmp, p - stmp, &index) <= -1) return -1;
					nwad->in6.sin6_scope_id = index;
#endif
				}
			}

			if (p < end) goto unrecog; /* some gargage after the end? */

			nwad->in6.sin6_family = AF_INET6;
			return 0;
		#else
			goto unrecog;
		#endif	
		}

		nwad->in4.sin_family = AF_INET;
#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
	}
#endif

	if (p < end && *p == ':') 
	{
		/* port number */
		moo_uint32_t port = 0;

		p++; /* skip : */

		tmp.ptr = (moo_ooch_t*)p;
		while (p < end && *p >= '0' && *p <= '9')
		{
			port = port * 10 + (*p - '0');
			p++;
		}

		tmp.len = p - tmp.ptr;
		if (tmp.len <= 0 || tmp.len >= 6 || 
		    port > MOO_TYPE_MAX(moo_uint16_t)) 
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "port number blank or too large");
			return -1;
		}

	#if (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
		if (nwad->in4.sin_family == AF_INET)
			nwad->in4.sin_port = moo_hton16(port);
		else
			nwad->in6.sin6_port = moo_hton16(port);
	#else
		nwad->in4.sin_port = moo_hton16(port);
	#endif
	}

	return 0;
	
	
unrecog:
	moo_seterrbfmt (moo, MOO_EINVAL, "unrecognized address");
	return -1;
	
no_rbrack:
	moo_seterrbfmt (moo, MOO_EINVAL, "missing right bracket");
	return -1;
}

static moo_pfrc_t pf_get_family (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_oop_t v;
	struct sockaddr* sa;

	rcv = (moo_oop_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OBJ_IS_BYTE_POINTER(rcv) && MOO_OBJ_GET_SIZE(rcv) >= MOO_SIZEOF(sockaddr_t));

	sa = (struct sockaddr*)MOO_OBJ_GET_BYTE_SLOT(rcv);
	v = MOO_SMOOI_TO_OOP(sa->sa_family);

	MOO_STACK_SETRET (moo, nargs, v);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_from_string (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_oop_t str;

	rcv = (moo_oop_t)MOO_STACK_GETRCV(moo, nargs);
	str = (moo_oop_t)MOO_STACK_GETARG(moo, nargs, 0);
	
	MOO_PF_CHECK_RCV (moo, MOO_OBJ_IS_BYTE_POINTER(rcv) && MOO_OBJ_GET_SIZE(rcv) >= MOO_SIZEOF(sockaddr_t));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OBJ_IS_CHAR_POINTER(str));

	if (str_to_sockaddr(moo, MOO_OBJ_GET_CHAR_SLOT(str), MOO_OBJ_GET_SIZE(str), (sockaddr_t*)MOO_OBJ_GET_BYTE_SLOT(rcv)) <= -1)
	{
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}


static moo_pfinfo_t pfinfos[] =
{
	{ I, "family",       { pf_get_family,     0, 0  }  },
	{ I, "fromString:",  { pf_from_string,    1, 1  }  }
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	moo_ooi_t spec;

	spec = MOO_OOP_TO_SMOOI(_class->spec);
	if (!MOO_CLASS_SPEC_IS_INDEXED(spec) || MOO_CLASS_SPEC_INDEXED_TYPE(spec) != MOO_OBJ_TYPE_BYTE || MOO_CLASS_SPEC_NAMED_INSTVARS(spec) != 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "%O not a plain #byte class", _class);
		return -1;
	}

	/* change the number of the fixed fields forcibly.
	 * the check against the superclass is done by the main compiler 
	 * after this import. so i perform no check about the superclass. */
	spec = MOO_CLASS_SPEC_MAKE(MOO_SIZEOF(sockaddr_t), MOO_CLASS_SPEC_FLAGS(spec), MOO_CLASS_SPEC_INDEXED_TYPE(spec));
	_class->spec = MOO_SMOOI_TO_OOP(spec);
	return 0;
}

static moo_pfbase_t* querypf (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase (moo, pfinfos, MOO_COUNTOF(pfinfos), name, namelen);
}

static void unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything? close open open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
}

int moo_mod_sck_addr (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->querypf = querypf;
	mod->querypv = MOO_NULL;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}


/* -------------------------------------------------------------------------- */
sck_len_t moo_sck_addr_len (sck_addr_t* addr)
{
	switch (addr->family)
	{
	#if defined(AF_INET) && (MOO__SIZEOF_STRUCT_SOCKADDR_IN > 0)
		case AF_INET:
			return MOO_SIZEOF(struct sockaddr_in);
	#endif
	#if defined(AF_INET6) && (MOO_SIZEOF_STRUCT_SOCKADDR_IN6 > 0)
		case AF_INET6:
			return MOO_SIZEOF(struct sockaddr_in6);
	#endif
	#if defined(AF_PACKET) && (MOO_SIZEOF_STRUCT_SOCKADDR_LL > 0)
		case AF_PACKET:
			return MOO_SIZEOF(struct sockaddr_ll);
	#endif
	#if defined(AF_UNIX) && (MOO_SIZEOF_STRUCT_SOCKADDR_UN > 0)
		case AF_UNIX:
			return MOO_SIZEOF(struct sockaddr_un);
	#endif
		default:
			return 0;
	}
}

