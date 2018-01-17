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

#ifndef _MOO_MOD_SCK_H_
#define _MOO_MOD_SCK_H_

#include <moo.h>

typedef struct sck_t* oop_sck_t;
struct sck_t
{
	MOO_OBJ_HEADER;

	moo_oop_t handle; /* SmallInteger */
	/* there are more fields in the actual object */
};

#if (MOO_SIZEOF_SOCKLEN_T > 0)
//#error x
#endif

#if (MOO_SIZEOF_INT8_T > 0)
#error x
#endif

#if (MOO_SIZEOF_SOCKLEN_T == 1)
	#if defined(MOO_SOCKLEN_T_IS_SIGNED)
		typedef moo_int8_t sck_len_t;
	#else
		typedef moo_uint8_t sck_len_t;
	#endif
#elif (MOO_SIZEOF_SOCKLEN_T == 2)
	#if defined(MOO_SOCKLEN_T_IS_SIGNED)
		typedef moo_int16_t sck_len_t;
	#else
		typedef moo_uint16_t sck_len_t;
	#endif
#elif (MOO_SIZEOF_SOCKLEN_T == 4)
	#if defined(MOO_SOCKLEN_T_IS_SIGNED)
		typedef moo_int32_t sck_len_t;
	#else
		typedef moo_uint32_t sck_len_t;
	#endif
#elif (MOO_SIZEOF_SOCKLEN_T == 8)
	#if defined(MOO_SOCKLEN_T_IS_SIGNED)
		typedef moo_int64_t sck_len_t;
	#else
		typedef moo_uint64_t sck_len_t;
	#endif
#else
#	define MOO_SIZEOF_SOCKLEN_T MOO_SIZEOF_INT
#	define MOO_SOCKLEN_T_IS_SIGNED
	typedef int sck_len_t;
#endif

#if (MOO_SIZEOF_SA_FAMILY_T == 1)
	#if defined(MOO_SA_FAMILY_T_IS_SIGNED)
		typedef moo_int8_t sck_addr_family_t;
	#else
		typedef moo_uint8_t sck_addr_family_t;
	#endif
#elif (MOO_SIZEOF_SA_FAMILY_T == 2)
	#if defined(MOO_SA_FAMILY_T_IS_SIGNED)
		typedef moo_int16_t sck_addr_family_t;
	#else
		typedef moo_uint16_t sck_addr_family_t;
	#endif
#elif (MOO_SIZEOF_SA_FAMILY_T == 4)
	#if defined(MOO_SA_FAMILY_T_IS_SIGNED)
		typedef moo_int32_t sck_addr_family_t;
	#else
		typedef moo_uint32_t sck_addr_family_t;
	#endif
#elif (MOO_SIZEOF_SA_FAMILY_T == 8)
	#if defined(MOO_SA_FAMILY_T_IS_SIGNED)
		typedef moo_int64_t sck_addr_family_t;
	#else
		typedef moo_uint64_t sck_addr_family_t;
	#endif
#else
#	define MOO_SIZEOF_SA_FAMILY_T MOO_SIZEOF_INT
#	define MOO_SA_FAMILY_T_IS_SIGNED
	typedef int sck_addr_family_t;
#endif

struct sck_addr_t
{
	sck_addr_family_t family;
	moo_uint8_t data[1];
};
typedef struct sck_addr_t sck_addr_t;

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT int moo_mod_sck (moo_t* moo, moo_mod_t* mod);
MOO_EXPORT int moo_mod_sck_addr (moo_t* moo, moo_mod_t* mod);


sck_len_t moo_sck_addr_len (sck_addr_t* addr);

#if defined(__cplusplus)
}
#endif


#endif
