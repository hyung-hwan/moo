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

#ifndef _MOO_UTL_H_
#define _MOO_UTL_H_

#include "moo-cmn.h"

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT moo_oow_t moo_hashbytes (
	const moo_oob_t* ptr,
	moo_oow_t        len
);

#if defined(MOO_HAVE_INLINE)
	static MOO_INLINE moo_oow_t moo_hashbchars (const moo_bch_t* ptr, moo_oow_t len)
	{
		return moo_hashbytes((const moo_oob_t*)ptr,len * MOO_SIZEOF(moo_bch_t));
	}

	static MOO_INLINE moo_oow_t moo_hashuchars (const moo_uch_t* ptr, moo_oow_t len)
	{
		return moo_hashbytes((const moo_oob_t*)ptr,len * MOO_SIZEOF(moo_uch_t));
	}

	static MOO_INLINE moo_oow_t moo_hashwords (const moo_oow_t* ptr, moo_oow_t len)
	{
		return moo_hashbytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oow_t));
	}

	static MOO_INLINE moo_oow_t moo_hashhalfwords (const moo_oohw_t* ptr, moo_oow_t len)
	{
		return moo_hashbytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oohw_t));
	}
#else
#	define moo_hashbchars(ptr,len) moo_hashbytes((const moo_oob_t*)ptr,len * MOO_SIZEOF(moo_bch_t))
#	define moo_hashuchars(ptr,len) moo_hashbytes((const moo_oob_t*)ptr,len * MOO_SIZEOF(moo_uch_t))
#	define moo_hashwords(ptr,len) moo_hashbytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oow_t))
#	define moo_hashhalfwords(ptr,len) moo_hashbytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oohw_t))
#endif

#if defined(MOO_OOCH_IS_UCH)
#	define moo_hashoochars(ptr,len) moo_hashuchars(ptr,len)
#else
#	define moo_hashoochars(ptr,len) moo_hashbchars(ptr,len)
#endif


MOO_EXPORT moo_oow_t moo_hashwords (
	const moo_oow_t* ptr,
	moo_oow_t        len
);

/**
 * The moo_equaluchars() function determines equality of two strings
 * of the same length \a len.
 */
MOO_EXPORT int moo_equaluchars (
	const moo_uch_t* str1,
	const moo_uch_t* str2,
	moo_oow_t        len
);

MOO_EXPORT int moo_equalbchars (
	const moo_bch_t* str1,
	const moo_bch_t* str2,
	moo_oow_t        len
);

MOO_EXPORT int moo_compucstr (
	const moo_uch_t* str1,
	const moo_uch_t* str2
);

MOO_EXPORT int moo_compbcstr (
	const moo_bch_t* str1,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_compucbcstr (
	const moo_uch_t* str1,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_compucharsbcstr (
	const moo_uch_t* str1,
	moo_oow_t        len,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_compbcharsbcstr (
	const moo_bch_t* str1,
	moo_oow_t        len,
	const moo_bch_t* str2
);

MOO_EXPORT void moo_copyuchars (
	moo_uch_t*       dst,
	const moo_uch_t* src,
	moo_oow_t        len
);

MOO_EXPORT void moo_copybchars (
	moo_bch_t*       dst,
	const moo_bch_t* src,
	moo_oow_t        len
);

MOO_EXPORT void moo_copybtouchars (
	moo_uch_t*       dst,
	const moo_bch_t* src,
	moo_oow_t        len
);

MOO_EXPORT moo_oow_t moo_copyucstr (
	moo_uch_t*       dst,
	moo_oow_t        len,
	const moo_uch_t* src
);

MOO_EXPORT moo_oow_t moo_copybcstr (
	moo_bch_t*       dst,
	moo_oow_t        len,
	const moo_bch_t* src
);

MOO_EXPORT moo_uch_t* moo_finduchar (
	const moo_uch_t* ptr,
	moo_oow_t        len,
	moo_uch_t        c
);

MOO_EXPORT moo_bch_t* moo_findbchar (
	const moo_bch_t* ptr,
	moo_oow_t        len,
	moo_bch_t        c
);

MOO_EXPORT moo_uch_t* moo_rfinduchar (
	const moo_uch_t* ptr,
	moo_oow_t        len,
	moo_uch_t        c
);

MOO_EXPORT moo_bch_t* moo_rfindbchar (
	const moo_bch_t* ptr,
	moo_oow_t        len,
	moo_bch_t        c
);


MOO_EXPORT moo_oow_t moo_countucstr (
	const moo_uch_t* str
);

MOO_EXPORT moo_oow_t moo_countbcstr (
	const moo_bch_t* str
);

#if defined(MOO_OOCH_IS_UCH)
#	define moo_equaloochars(str1,str2,len) moo_equaluchars(str1,str2,len)
#	define moo_compoocbcstr(str1,str2) moo_compucbcstr(str1,str2)
#	define moo_compoocharsbcstr(str1,len1,str2) moo_compucharsbcstr(str1,len1,str2)
#	define moo_compoocstr(str1,str2) moo_compucstr(str1,str2)
#	define moo_copyoochars(dst,src,len) moo_copyuchars(dst,src,len)
#	define moo_copybctooochars(dst,src,len) moo_copybtouchars(dst,src,len)
#	define moo_copyoocstr(dst,len,src) moo_copyucstr(dst,len,src)
#	define moo_findoochar(ptr,len,c) moo_finduchar(ptr,len,c)
#	define moo_rfindoochar(ptr,len,c) moo_rfinduchar(ptr,len,c)
#	define moo_countoocstr(str) moo_countucstr(str)
#else

#	define moo_equaloochars(str1,str2,len) moo_equalbchars(str1,str2,len)
#	define moo_compoocbcstr(str1,str2) moo_compbcstr(str1,str2)
#	define moo_compoocharsbcstr(str1,len1,str2) moo_compbcharsbcstr(str1,len1,str2)
#	define moo_compoocstr(str1,str2) moo_compbcstr(str1,str2)
#	define moo_copyoochars(dst,src,len) moo_copybchars(dst,src,len)
#	define moo_copybctooochars(dst,src,len) moo_copybchars(dst,src,len)
#	define moo_copyoocstr(dst,len,src) moo_copybcstr(dst,len,src)
#	define moo_findoochar(ptr,len,c) moo_findbchar(ptr,len,c)
#	define moo_rfindoochar(ptr,len,c) moo_rfindbchar(ptr,len,c)
#	define moo_countoocstr(str) moo_countbcstr(str)
#endif



MOO_EXPORT int moo_copyoocstrtosbuf (
	moo_t*            moo,
	const moo_ooch_t* str,
	int                id
);

MOO_EXPORT int moo_concatoocstrtosbuf (
	moo_t*            moo,
	const moo_ooch_t* str,
	int                id
);

MOO_EXPORT moo_cmgr_t* moo_getutf8cmgr (
	void
);

/**
 * The moo_convutoutf8chars() function converts a unicode character string \a ucs 
 * to a UTF8 string and writes it into the buffer pointed to by \a bcs, but
 * not more than \a bcslen bytes including the terminating null.
 *
 * Upon return, \a bcslen is modified to the actual number of bytes written to
 * \a bcs excluding the terminating null; \a ucslen is modified to the number of
 * wide characters converted.
 *
 * You may pass #MOO_NULL for \a bcs to dry-run conversion or to get the
 * required buffer size for conversion. -2 is never returned in this case.
 *
 * \return
 * - 0 on full conversion,
 * - -1 on no or partial conversion for an illegal character encountered,
 * - -2 on no or partial conversion for a small buffer.
 *
 * \code
 *   const moo_uch_t ucs[] = { 'H', 'e', 'l', 'l', 'o' };
 *   moo_bch_t bcs[10];
 *   moo_oow_t ucslen = 5;
 *   moo_oow_t bcslen = MOO_COUNTOF(bcs);
 *   n = moo_convutoutf8chars (ucs, &ucslen, bcs, &bcslen);
 *   if (n <= -1)
 *   {
 *      // conversion error
 *   }
 * \endcode
 */
MOO_EXPORT int moo_convutoutf8chars (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

/**
 * The moo_convutf8touchars() function converts a UTF8 string to a uncide string.
 *
 * It never returns -2 if \a ucs is #MOO_NULL.
 *
 * \code
 *  const moo_bch_t* bcs = "test string";
 *  moo_uch_t ucs[100];
 *  moo_oow_t ucslen = MOO_COUNTOF(buf), n;
 *  moo_oow_t bcslen = 11;
 *  int n;
 *  n = moo_convutf8touchars (bcs, &bcslen, ucs, &ucslen);
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
MOO_EXPORT int moo_convutf8touchars (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);


MOO_EXPORT int moo_convutoutf8cstr (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_convutf8toucstr (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);



MOO_EXPORT moo_oow_t moo_uctoutf8 (
	moo_uch_t    uc,
	moo_bch_t*   utf8,
	moo_oow_t    size
);

MOO_EXPORT moo_oow_t moo_utf8touc (
	const moo_bch_t* utf8,
	moo_oow_t        size,
	moo_uch_t*       uc
);


#if defined(__cplusplus)
}
#endif


#endif
