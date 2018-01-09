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
#include "../lib/moo-prv.h"

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
	moo_ooi_t spec;
	/*if (moo_setclasstrsize (moo, _class, MOO_SIZEOF(sck_addr_trailer_t), MOO_NULL) <= -1) return -1;*/

	spec = MOO_OOP_TO_SMOOI(_class->spec);
	if (!MOO_CLASS_SPEC_IS_INDEXED(spec) || MOO_CLASS_SPEC_INDEXED_TYPE(spec) != MOO_OBJ_TYPE_BYTE || MOO_CLASS_SPEC_NAMED_INSTVARS(spec) != 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "%O not a plain byte class", _class);
		return -1;
	}

	/* change the number of the fixed fields forcibly */
/* TODO: check if the super class has what kind of size ... */
	spec = MOO_CLASS_SPEC_MAKE (10, MOO_CLASS_SPEC_FLAGS(spec), MOO_CLASS_SPEC_INDEXED_TYPE(spec));
	_class->spec = MOO_SMOOI_TO_OOP(spec);
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

