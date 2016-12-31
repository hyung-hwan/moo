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
#	define stix_equaloochars(str1,str2,len) stix_equaluchars(str1,str2,len)
#	define stix_compoocbcstr(str1,str2) stix_compucbcstr(str1,str2)
#	define stix_compoocharsbcstr(str1,len1,str2) stix_compucharsbcstr(str1,len1,str2)
#	define stix_compoocstr(str1,str2) stix_compucstr(str1,str2)
#	define stix_copyoochars(dst,src,len) stix_copyuchars(dst,src,len)
#	define stix_copybctooochars(dst,src,len) stix_copybtouchars(dst,src,len)
#	define stix_copyoocstr(dst,len,src) stix_copyucstr(dst,len,src)
#	define stix_findoochar(ptr,len,c) stix_finduchar(ptr,len,c)
#	define stix_rfindoochar(ptr,len,c) stix_rfinduchar(ptr,len,c)
#	define stix_countoocstr(str) stix_countucstr(str)
#else
#	define stix_hashchars(ptr,len) stix_hashbchars(ptr,len)
#	define stix_equaloochars(str1,str2,len) stix_equalbchars(str1,str2,len)
#	define stix_compoocbcstr(str1,str2) stix_compbcstr(str1,str2)
#	define stix_compoocharsbcstr(str1,len1,str2) stix_compbcharsbcstr(str1,len1,str2)
#	define stix_compoocstr(str1,str2) stix_compbcstr(str1,str2)
#	define stix_copyoochars(dst,src,len) stix_copybchars(dst,src,len)
#	define stix_copybctooochars(dst,src,len) stix_copybchars(dst,src,len)
#	define stix_copyoocstr(dst,len,src) stix_copybcstr(dst,len,src)
#	define stix_findoochar(ptr,len,c) stix_findbchar(ptr,len,c)
#	define stix_rfindoochar(ptr,len,c) stix_rfindbchar(ptr,len,c)
#	define stix_countoocstr(str) stix_countbcstr(str)
#endif


/* ========================================================================= */
/* utl.c                                                                */
/* ========================================================================= */
STIX_EXPORT stix_oow_t stix_hashbytes (
	const stix_oob_t* ptr,
	stix_oow_t        len
);

STIX_EXPORT stix_oow_t stix_hashuchars (
	const stix_uch_t*  ptr,
	stix_oow_t        len
);

#define stix_hashbchars(ptr,len) stix_hashbytes(ptr,len)

/**
 * The stix_equaluchars() function determines equality of two strings
 * of the same length \a len.
 */
STIX_EXPORT int stix_equaluchars (
	const stix_uch_t* str1,
	const stix_uch_t* str2,
	stix_oow_t        len
);

STIX_EXPORT int stix_equalbchars (
	const stix_bch_t* str1,
	const stix_bch_t* str2,
	stix_oow_t        len
);

STIX_EXPORT int stix_compucstr (
	const stix_uch_t* str1,
	const stix_uch_t* str2
);

STIX_EXPORT int stix_compbcstr (
	const stix_bch_t* str1,
	const stix_bch_t* str2
);

STIX_EXPORT int stix_compucbcstr (
	const stix_uch_t* str1,
	const stix_bch_t* str2
);

STIX_EXPORT int stix_compucharsbcstr (
	const stix_uch_t* str1,
	stix_oow_t        len,
	const stix_bch_t* str2
);

STIX_EXPORT int stix_compbcharsbcstr (
	const stix_bch_t* str1,
	stix_oow_t        len,
	const stix_bch_t* str2
);

STIX_EXPORT void stix_copyuchars (
	stix_uch_t*       dst,
	const stix_uch_t* src,
	stix_oow_t        len
);

STIX_EXPORT void stix_copybchars (
	stix_bch_t*       dst,
	const stix_bch_t* src,
	stix_oow_t        len
);

STIX_EXPORT void stix_copybtouchars (
	stix_uch_t*       dst,
	const stix_bch_t* src,
	stix_oow_t        len
);

STIX_EXPORT stix_oow_t stix_copyucstr (
	stix_uch_t*       dst,
	stix_oow_t        len,
	const stix_uch_t* src
);

STIX_EXPORT stix_oow_t stix_copybcstr (
	stix_bch_t*       dst,
	stix_oow_t        len,
	const stix_bch_t* src
);

STIX_EXPORT stix_uch_t* stix_finduchar (
	const stix_uch_t* ptr,
	stix_oow_t        len,
	stix_uch_t        c
);

STIX_EXPORT stix_bch_t* stix_findbchar (
	const stix_bch_t* ptr,
	stix_oow_t        len,
	stix_bch_t        c
);

STIX_EXPORT stix_uch_t* stix_rfinduchar (
	const stix_uch_t* ptr,
	stix_oow_t        len,
	stix_uch_t        c
);

STIX_EXPORT stix_bch_t* stix_rfindbchar (
	const stix_bch_t* ptr,
	stix_oow_t        len,
	stix_bch_t        c
);


STIX_EXPORT stix_oow_t stix_countucstr (
	const stix_uch_t* str
);

STIX_EXPORT stix_oow_t stix_countbcstr (
	const stix_bch_t* str
);

STIX_EXPORT int stix_copyoocstrtosbuf (
	stix_t*            stix,
	const stix_ooch_t* str,
	int                id
);

STIX_EXPORT int stix_concatoocstrtosbuf (
	stix_t*            stix,
	const stix_ooch_t* str,
	int                id
);

STIX_EXPORT stix_cmgr_t* stix_getutf8cmgr (
	void
);

/**
 * The stix_convutoutf8chars() function converts a unicode character string \a ucs 
 * to a UTF8 string and writes it into the buffer pointed to by \a bcs, but
 * not more than \a bcslen bytes including the terminating null.
 *
 * Upon return, \a bcslen is modified to the actual number of bytes written to
 * \a bcs excluding the terminating null; \a ucslen is modified to the number of
 * wide characters converted.
 *
 * You may pass #STIX_NULL for \a bcs to dry-run conversion or to get the
 * required buffer size for conversion. -2 is never returned in this case.
 *
 * \return
 * - 0 on full conversion,
 * - -1 on no or partial conversion for an illegal character encountered,
 * - -2 on no or partial conversion for a small buffer.
 *
 * \code
 *   const stix_uch_t ucs[] = { 'H', 'e', 'l', 'l', 'o' };
 *   stix_bch_t bcs[10];
 *   stix_oow_t ucslen = 5;
 *   stix_oow_t bcslen = STIX_COUNTOF(bcs);
 *   n = stix_convutoutf8chars (ucs, &ucslen, bcs, &bcslen);
 *   if (n <= -1)
 *   {
 *      // conversion error
 *   }
 * \endcode
 */
STIX_EXPORT int stix_convutoutf8chars (
	const stix_uch_t*    ucs,
	stix_oow_t*          ucslen,
	stix_bch_t*          bcs,
	stix_oow_t*          bcslen
);

/**
 * The stix_convutf8touchars() function converts a UTF8 string to a uncide string.
 *
 * It never returns -2 if \a ucs is #STIX_NULL.
 *
 * \code
 *  const stix_bch_t* bcs = "test string";
 *  stix_uch_t ucs[100];
 *  stix_oow_t ucslen = STIX_COUNTOF(buf), n;
 *  stix_oow_t bcslen = 11;
 *  int n;
 *  n = stix_convutf8touchars (bcs, &bcslen, ucs, &ucslen);
 *  if (n <= -1) { invalid/incomplenete sequence or buffer to small }
 * \endcode
 * 
 * The resulting \a ucslen can still be greater than 0 even if the return
 * value is negative. The value indiates the number of characters converted
 * before the error has occurred.
 *
 * \return 0 on success.
 *         -1 if \a bcs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if \a bcs is not a complete sequence.
 */
STIX_EXPORT int stix_convutf8touchars (
	const stix_bch_t*   bcs,
	stix_oow_t*         bcslen,
	stix_uch_t*         ucs,
	stix_oow_t*         ucslen
);


STIX_EXPORT int stix_convutoutf8cstr (
	const stix_uch_t*    ucs,
	stix_oow_t*          ucslen,
	stix_bch_t*          bcs,
	stix_oow_t*          bcslen
);

STIX_EXPORT int stix_convutf8toucstr (
	const stix_bch_t*   bcs,
	stix_oow_t*         bcslen,
	stix_uch_t*         ucs,
	stix_oow_t*         ucslen
);

/* ========================================================================= */
/* utf8.c                                                                    */
/* ========================================================================= */
STIX_EXPORT stix_oow_t stix_uctoutf8 (
	stix_uch_t    uc,
	stix_bch_t*   utf8,
	stix_oow_t    size
);

STIX_EXPORT stix_oow_t stix_utf8touc (
	const stix_bch_t* utf8,
	stix_oow_t        size,
	stix_uch_t*       uc
);

#if defined(__cplusplus)
}
#endif


#endif
