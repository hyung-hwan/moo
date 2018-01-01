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


#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE

#define MA MOO_TYPE_MAX(moo_oow_t)

typedef struct sck_addr_trailer_t sck_addr_trailer_t;

struct sck_addr_trailer_t
{
	int family;
	union
	{
		struct
		{
			moo_uint16_t port;
			moo_uint32_t addr;
		} in4;
		struct
		{
			moo_uint16_t port;
			moo_uint8_t addr[16];
			moo_uint32_t scope;
		} in6;

		/*
		struct
		{
			moo_ooch_t path[100];
		} local; */
	};
};



static moo_pfrc_t pf_from_string (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	sck_addr_trailer_t* tr;

	rcv = (moo_oop_t)MOO_STACK_GETRCV(moo, nargs);
	
	tr = moo_getobjtrailer (moo, MOO_STACK_GETRCV(moo,nargs), MOO_NULL);
	//if (tr->event)
	
	
	return MOO_PF_SUCCESS;
}

static moo_pfinfo_t pfinfos[] =
{
	{ I, { 'f','r','o','m','S','t','r','i','n','g',':','\0' },  0, { pf_from_string,    1, 1  }  },
};

/* ------------------------------------------------------------------------ */

static int import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(sck_addr_trailer_t), MOO_NULL) <= -1) return -1;
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

int moo_mod_sck_addr (moo_t* moo, moo_mod_t* mod)
{
	mod->import = import;
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = MOO_NULL;
	return 0;
}

