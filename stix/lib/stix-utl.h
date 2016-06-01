/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _STIX_UTL_H_
#define _STIX_UTL_H_

#include "stix-cmn.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(STIX_OOCH_IS_UCH)
#	define stix_hashchars(ptr,len) stix_hashuchars(ptr,len)
#	define stix_compoocbcstr(str1,str2) stix_compucbcstr(str1,str2)
#	define stix_compoocstr(str1,str2) stix_compucstr(str1,str2)
#	define stix_copyoochars(dst,src,len) stix_copyuchars(dst,src,len)
#	define stix_copybchtooochars(dst,src,len) stix_copybchtouchars(dst,src,len)
#	define stix_copyoocstr(dst,len,src) stix_copyucstr(dst,len,src)
#	define stix_findoochar(ptr,len,c) stix_finduchar(ptr,len,c)
#	define stix_countoocstr(str) stix_countucstr(str)
#else
#	define stix_hashchars(ptr,len) stix_hashbchars(ptr,len)
#	define stix_compoocbcstr(str1,str2) stix_compbcstr(str1,str2)
#	define stix_compoocstr(str1,str2) stix_compbcstr(str1,str2)
#	define stix_copyoochars(dst,src,len) stix_copybchars(dst,src,len)
#	define stix_copybchtooochars(dst,src,len) stix_copybchars(dst,src,len)
#	define stix_copyoocstr(dst,len,src) stix_copybcstr(dst,len,src)
#	define stix_findoochar(ptr,len,c) stix_findbchar(ptr,len,c)
#	define stix_countoocstr(str) stix_countbcstr(str)
#endif


/* ========================================================================= */
/* utl.c                                                                */
/* ========================================================================= */
stix_oow_t stix_hashbytes (
	const stix_oob_t* ptr,
	stix_oow_t        len
);

stix_oow_t stix_hashuchars (
	const stix_uch_t*  ptr,
	stix_oow_t        len
);

#define stix_hashbchars(ptr,len) stix_hashbytes(ptr,len)


int stix_equalchars (
	const stix_uch_t* str1,
	const stix_uch_t* str2,
	stix_oow_t        len
);

int stix_compucstr (
	const stix_uch_t* str1,
	const stix_uch_t* str2
);

int stix_compbcstr (
	const stix_bch_t* str1,
	const stix_bch_t* str2
);

int stix_compucbcstr (
	const stix_uch_t* str1,
	const stix_bch_t* str2
);

int stix_compucxbcstr (
	const stix_uch_t* str1,
	stix_oow_t        len,
	const stix_bch_t* str2
);

void stix_copyuchars (
	stix_uch_t*       dst,
	const stix_uch_t* src,
	stix_oow_t        len
);

void stix_copybchars (
	stix_bch_t*       dst,
	const stix_bch_t* src,
	stix_oow_t        len
);

void stix_copybchtouchars (
	stix_uch_t*       dst,
	const stix_bch_t* src,
	stix_oow_t        len
);

stix_oow_t stix_copyucstr (
	stix_uch_t*       dst,
	stix_oow_t        len,
	const stix_uch_t* src
);

stix_oow_t stix_copybcstr (
	stix_bch_t*       dst,
	stix_oow_t        len,
	const stix_bch_t* src
);

stix_uch_t* stix_finduchar (
	const stix_uch_t* ptr,
	stix_oow_t        len,
	stix_uch_t        c
);

stix_bch_t* stix_findbchar (
	const stix_bch_t* ptr,
	stix_oow_t        len,
	stix_bch_t        c
);

stix_oow_t stix_countucstr (
	const stix_uch_t* str
);

stix_oow_t stix_countbcstr (
	const stix_bch_t* str
);


#if defined(__cplusplus)
}
#endif


#endif
