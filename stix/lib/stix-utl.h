/*
 * $Id$
 *
    Copyright (c) 2014-2015 Chung, Hyung-Hwan. All rights reserved.

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


/* ========================================================================= */
/* stix-utl.c                                                                */
/* ========================================================================= */
int stix_equalchars (
	const stix_uch_t*  str1,
	const stix_uch_t*  str2,
	stix_size_t        len
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
	stix_size_t       len,
	const stix_bch_t* str2
);

void stix_copyuchars (
	stix_uch_t*       dst,
	const stix_uch_t* src,
	stix_size_t       len
);

void stix_copybchtouchars (
	stix_uch_t*       dst,
	const stix_bch_t* src,
	stix_size_t       len
);

stix_uch_t* stix_findchar (
	const stix_uch_t* ptr,
	stix_size_t       len,
	stix_uch_t        c
);

stix_size_t stix_copyucstr (
	stix_uch_t*       dst,
	stix_size_t       len,
	const stix_uch_t* src
);

stix_size_t stix_copybcstr (
	stix_bch_t*       dst,
	stix_size_t       len,
	const stix_bch_t* src
);


#if defined(__cplusplus)
}
#endif


#endif
