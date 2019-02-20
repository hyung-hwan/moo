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

#ifndef _MOO_UTL_H_
#define _MOO_UTL_H_

#include "moo-cmn.h"


/* ----------------------------------------------------------------------- 
 * DOUBLY LINKED LIST MACROS
 * ----------------------------------------------------------------------- */
#define MOO_APPEND_TO_LIST(list, node) do { \
	(node)->next = MOO_NULL; \
	(node)->prev = (list)->last; \
	if ((list)->first) (list)->last->next = (node); \
	else (list)->first = (node); \
	(list)->last = (node); \
} while(0)

#define MOO_PREPPEND_TO_LIST(list, node) do { \
	(node)->prev = MOO_NULL; \
	(node)->next = (list)->first; \
	if ((list)->last) (list)->first->prev = (node); \
	else (list)->last = (node); \
	(list)->first = (node); \
} while(0)

#define MOO_DELETE_FROM_LIST(list, node) do { \
	if ((node)->prev) (node)->prev->next = (node)->next; \
	else (list)->first = (node)->next; \
	if ((node)->next) (node)->next->prev = (node)->prev; \
	else (list)->last = (node)->prev; \
} while(0)



#define MOO_APPEND_TO_OOP_LIST(moo, list, node_type, node, _link) do { \
	(node)->_link.next = (node_type)(moo)->_nil; \
	(node)->_link.prev = (list)->last; \
	if ((moo_oop_t)(list)->last != (moo)->_nil) (list)->last->_link.next = (node); \
	else (list)->first = (node); \
	(list)->last = (node); \
} while(0)

#define MOO_PREPPEND_TO_OOP_LIST(moo, list, node_type, node, _link) do { \
	(node)->_link.prev = (node_type)(moo)->_nil; \
	(node)->_link.next = (list)->first; \
	if ((moo_oop_t)(list)->first != (moo)->_nil) (list)->first->_link.prev = (node); \
	else (list)->last = (node); \
	(list)->first = (node); \
} while(0)

#define MOO_DELETE_FROM_OOP_LIST(moo, list, node, _link) do { \
	if ((moo_oop_t)(node)->_link.prev != (moo)->_nil) (node)->_link.prev->_link.next = (node)->_link.next; \
	else (list)->first = (node)->_link.next; \
	if ((moo_oop_t)(node)->_link.next != (moo)->_nil) (node)->_link.next->_link.prev = (node)->_link.prev; \
	else (list)->last = (node)->_link.prev; \
} while(0)

/*
#define MOO_CLEANUP_FROM_OOP_LIST(moo, list, node, _link) do { \
	MOO_DELETE_FROM_OOP_LIST (moo, list, node, _link); \
	(node)->_link.prev = (node_type)(moo)->_nil; \
	(node)->_link.next = (node_type)(moo)->_nil; \
} while(0);
*/



#define MOO_CONST_SWAP16(x) \
	((qse_uint16_t)((((qse_uint16_t)(x) & (qse_uint16_t)0x00ffU) << 8) | \
	                (((qse_uint16_t)(x) & (qse_uint16_t)0xff00U) >> 8) ))

#define MOO_CONST_SWAP32(x) \
	((qse_uint32_t)((((qse_uint32_t)(x) & (qse_uint32_t)0x000000ffUL) << 24) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0x0000ff00UL) <<  8) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0x00ff0000UL) >>  8) | \
	                (((qse_uint32_t)(x) & (qse_uint32_t)0xff000000UL) >> 24) ))

#if defined(MOO_ENDIAN_LITTLE)
#       define MOO_CONST_NTOH16(x) MOO_CONST_SWAP16(x)
#       define MOO_CONST_HTON16(x) MOO_CONST_SWAP16(x)
#       define MOO_CONST_NTOH32(x) MOO_CONST_SWAP32(x)
#       define MOO_CONST_HTON32(x) MOO_CONST_SWAP32(x)
#elif defined(MOO_ENDIAN_BIG)
#       define MOO_CONST_NTOH16(x) (x)
#       define MOO_CONST_HTON16(x) (x)
#       define MOO_CONST_NTOH32(x) (x)
#       define MOO_CONST_HTON32(x) (x)
#else
#       error UNKNOWN ENDIAN
#endif


#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT moo_oow_t moo_hash_bytes (
	const moo_oob_t* ptr,
	moo_oow_t        len
);

#if defined(MOO_HAVE_INLINE)
	static MOO_INLINE moo_oow_t moo_hashbchars (const moo_bch_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_bch_t));
	}

	static MOO_INLINE moo_oow_t moo_hashuchars (const moo_uch_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_uch_t));
	}

	static MOO_INLINE moo_oow_t moo_hashwords (const moo_oow_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oow_t));
	}

	static MOO_INLINE moo_oow_t moo_hashhalfwords (const moo_oohw_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oohw_t));
	}
#else
#	define moo_hashbchars(ptr,len)    moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_bch_t))
#	define moo_hashuchars(ptr,len)    moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_uch_t))
#	define moo_hashwords(ptr,len)     moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oow_t))
#	define moo_hashhalfwords(ptr,len) moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oohw_t))
#endif

#if defined(MOO_OOCH_IS_UCH)
#	define moo_hashoochars(ptr,len) moo_hashuchars(ptr,len)
#else
#	define moo_hashoochars(ptr,len) moo_hashbchars(ptr,len)
#endif

/**
 * The moo_equal_uchars() function determines equality of two strings
 * of the same length \a len.
 */
MOO_EXPORT int moo_equal_uchars (
	const moo_uch_t* str1,
	const moo_uch_t* str2,
	moo_oow_t        len
);

MOO_EXPORT int moo_equal_bchars (
	const moo_bch_t* str1,
	const moo_bch_t* str2,
	moo_oow_t        len
);

MOO_EXPORT int moo_comp_uchars (
	const moo_uch_t* str1,
	moo_oow_t        len1,
	const moo_uch_t* str2,
	moo_oow_t        len2
);

MOO_EXPORT int moo_comp_bchars (
	const moo_bch_t* str1,
	moo_oow_t        len1,
	const moo_bch_t* str2,
	moo_oow_t        len2
);

MOO_EXPORT int moo_comp_ucstr (
	const moo_uch_t* str1,
	const moo_uch_t* str2
);

MOO_EXPORT int moo_comp_bcstr (
	const moo_bch_t* str1,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_comp_ucstr_bcstr (
	const moo_uch_t* str1,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_comp_uchars_ucstr (
	const moo_uch_t* str1,
	moo_oow_t        len,
	const moo_uch_t* str2
);

MOO_EXPORT int moo_comp_uchars_bcstr (
	const moo_uch_t* str1,
	moo_oow_t        len,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_comp_bchars_bcstr (
	const moo_bch_t* str1,
	moo_oow_t        len,
	const moo_bch_t* str2
);

MOO_EXPORT int moo_comp_bchars_ucstr (
	const moo_bch_t* str1,
	moo_oow_t        len,
	const moo_uch_t* str2
);

MOO_EXPORT void moo_copy_uchars (
	moo_uch_t*       dst,
	const moo_uch_t* src,
	moo_oow_t        len
);

MOO_EXPORT void moo_copy_bchars (
	moo_bch_t*       dst,
	const moo_bch_t* src,
	moo_oow_t        len
);

MOO_EXPORT void moo_copy_bchars_to_uchars (
	moo_uch_t*       dst,
	const moo_bch_t* src,
	moo_oow_t        len
);

MOO_EXPORT moo_oow_t moo_copy_ucstr (
	moo_uch_t*       dst,
	moo_oow_t        len,
	const moo_uch_t* src
);

MOO_EXPORT moo_oow_t moo_copy_bcstr (
	moo_bch_t*       dst,
	moo_oow_t        len,
	const moo_bch_t* src
);

MOO_EXPORT void moo_fill_uchars (
	moo_uch_t*       dst,
	const moo_uch_t  ch,
	moo_oow_t        len
);

MOO_EXPORT void moo_fill_bchars (
	moo_bch_t*       dst,
	const moo_bch_t  ch,
	moo_oow_t        len
);


MOO_EXPORT moo_uch_t* moo_find_uchar (
	const moo_uch_t* ptr,
	moo_oow_t        len,
	moo_uch_t        c
);

MOO_EXPORT moo_bch_t* moo_find_bchar (
	const moo_bch_t* ptr,
	moo_oow_t        len,
	moo_bch_t        c
);

MOO_EXPORT moo_uch_t* moo_rfind_uchar (
	const moo_uch_t* ptr,
	moo_oow_t        len,
	moo_uch_t        c
);

MOO_EXPORT moo_bch_t* moo_rfind_bchar (
	const moo_bch_t* ptr,
	moo_oow_t        len,
	moo_bch_t        c
);

MOO_EXPORT moo_uch_t* moo_find_uchar_in_ucstr (
	const moo_uch_t* ptr,
	moo_uch_t        c
);

MOO_EXPORT moo_bch_t* moo_find_bchar_in_bcstr (
	const moo_bch_t* ptr,
	moo_bch_t        c
);

MOO_EXPORT moo_oow_t moo_count_ucstr (
	const moo_uch_t* str
);

MOO_EXPORT moo_oow_t moo_count_bcstr (
	const moo_bch_t* str
);

#if defined(MOO_OOCH_IS_UCH)
#	define moo_equal_oochars(str1,str2,len) moo_equal_uchars(str1,str2,len)
#	define moo_comp_oochars(str1,len1,str2,len2) moo_comp_uchars(str1,len1,str2,len2)
#	define moo_comp_oocstr_bcstr(str1,str2) moo_comp_ucstr_bcstr(str1,str2)
#	define moo_comp_oochars_bcstr(str1,len1,str2) moo_comp_uchars_bcstr(str1,len1,str2)
#	define moo_comp_oochars_ucstr(str1,len1,str2) moo_comp_uchars_ucstr(str1,len1,str2)
#	define moo_comp_oochars_oocstr(str1,len1,str2) moo_comp_uchars_ucstr(str1,len1,str2)
#	define moo_comp_oocstr(str1,str2) moo_comp_ucstr(str1,str2)
#	define moo_copy_oochars(dst,src,len) moo_copy_uchars(dst,src,len)
#	define moo_copy_bchars_to_oochars(dst,src,len) moo_copy_bchars_to_uchars(dst,src,len)
#	define moo_copy_oocstr(dst,len,src) moo_copy_ucstr(dst,len,src)
#	define moo_fill_oochars(dst,ch,len) moo_fill_uchars(dst,ch,len)
#	define moo_find_oochar(ptr,len,c) moo_find_uchar(ptr,len,c)
#	define moo_rfind_oochar(ptr,len,c) moo_rfind_uchar(ptr,len,c)
#	define moo_find_oochar_in_oocstr(ptr,c) moo_find_uchar_in_ucstr(ptr,c)
#	define moo_count_oocstr(str) moo_count_ucstr(str)
#else
#	define moo_equal_oochars(str1,str2,len) moo_equal_bchars(str1,str2,len)
#	define moo_comp_oochars(str1,len1,str2,len2) moo_comp_bchars(str1,len1,str2,len2)
#	define moo_comp_oocstr_bcstr(str1,str2) moo_comp_bcstr(str1,str2)
#	define moo_comp_oochars_bcstr(str1,len1,str2) moo_comp_bchars_bcstr(str1,len1,str2)
#	define moo_comp_oochars_ucstr(str1,len1,str2) moo_comp_bchars_ucstr(str1,len1,str2)
#	define moo_comp_oochars_oocstr(str1,len1,str2) moo_comp_bchars_bcstr(str1,len1,str2)
#	define moo_comp_oocstr(str1,str2) moo_comp_bcstr(str1,str2)
#	define moo_copy_oochars(dst,src,len) moo_copy_bchars(dst,src,len)
#	define moo_copy_bchars_to_oochars(dst,src,len) moo_copy_bchars(dst,src,len)
#	define moo_copy_oocstr(dst,len,src) moo_copy_bcstr(dst,len,src)
#	define moo_fill_oochars(dst,ch,len) moo_fill_bchars(dst,ch,len)
#	define moo_find_oochar(ptr,len,c) moo_find_bchar(ptr,len,c)
#	define moo_rfind_oochar(ptr,len,c) moo_rfind_bchar(ptr,len,c)
#	define moo_find_oochar_in_oocstr(ptr,c) moo_find_bchar_in_bcstr(ptr,c)
#	define moo_count_oocstr(str) moo_count_bcstr(str)
#endif

/* ------------------------------------------------------------------------- */

MOO_EXPORT int moo_ucwidth (
	moo_uch_t uc
);

/* ------------------------------------------------------------------------- */

#if defined(MOO_OOCH_IS_UCH)
#	define moo_conv_oocs_to_bcs_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr) moo_conv_ucs_to_bcs_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr)
#	define moo_conv_oochars_to_bchars_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr) moo_conv_uchars_to_bchars_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr)
#else
#	define moo_conv_oocs_to_ucs_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr) moo_conv_bcs_to_ucs_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr,0)
#	define moo_conv_oochars_to_uchars_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr) moo_conv_bchars_to_uchars_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr,0)
#endif


MOO_EXPORT int moo_conv_bcs_to_ucs_with_cmgr (
	const moo_bch_t* bcs,
	moo_oow_t*       bcslen,
	moo_uch_t*       ucs,
	moo_oow_t*       ucslen,
	moo_cmgr_t*      cmgr,
	int              all
);

MOO_EXPORT int moo_conv_bchars_to_uchars_with_cmgr (
	const moo_bch_t* bcs,
	moo_oow_t*       bcslen,
	moo_uch_t*       ucs,
	moo_oow_t*       ucslen,
	moo_cmgr_t*      cmgr,
	int              all
);

MOO_EXPORT int moo_conv_ucs_to_bcs_with_cmgr (
	const moo_uch_t* ucs,
	moo_oow_t*       ucslen,
	moo_bch_t*       bcs,
	moo_oow_t*       bcslen,
	moo_cmgr_t*      cmgr
);

MOO_EXPORT int moo_conv_uchars_to_bchars_with_cmgr (
	const moo_uch_t* ucs,
	moo_oow_t*       ucslen,
	moo_bch_t*       bcs,
	moo_oow_t*       bcslen,
	moo_cmgr_t*      cmgr
);

/* ------------------------------------------------------------------------- */

MOO_EXPORT moo_cmgr_t* moo_get_utf8_cmgr (
	void
);

/**
 * The moo_conv_uchars_to_utf8() function converts a unicode character string \a ucs 
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
 *   n = moo_conv_uchars_to_utf8 (ucs, &ucslen, bcs, &bcslen);
 *   if (n <= -1)
 *   {
 *      // conversion error
 *   }
 * \endcode
 */
MOO_EXPORT int moo_conv_uchars_to_utf8 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

/**
 * The moo_conv_utf8_to_uchars() function converts a UTF8 string to a uncide string.
 *
 * It never returns -2 if \a ucs is #MOO_NULL.
 *
 * \code
 *  const moo_bch_t* bcs = "test string";
 *  moo_uch_t ucs[100];
 *  moo_oow_t ucslen = MOO_COUNTOF(buf), n;
 *  moo_oow_t bcslen = 11;
 *  int n;
 *  n = moo_conv_utf8_to_uchars (bcs, &bcslen, ucs, &ucslen);
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
MOO_EXPORT int moo_conv_utf8_to_uchars (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);


MOO_EXPORT int moo_conv_ucstr_to_utf8 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_conv_utf8_to_ucstr (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);


MOO_EXPORT moo_oow_t moo_uc_to_utf8 (
	moo_uch_t    uc,
	moo_bch_t*   utf8,
	moo_oow_t    size
);

MOO_EXPORT moo_oow_t moo_utf8_to_uc (
	const moo_bch_t* utf8,
	moo_oow_t        size,
	moo_uch_t*       uc
);

/* ------------------------------------------------------------------------- */

MOO_EXPORT moo_cmgr_t* moo_get_utf16_cmgr (
	void
);

MOO_EXPORT int moo_conv_uchars_to_utf16 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_conv_utf16_to_uchars (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);


MOO_EXPORT int moo_conv_ucstr_to_utf16 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_conv_utf16_to_ucstr (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);

MOO_EXPORT moo_oow_t moo_uc_to_utf16 (
	moo_uch_t    uc,
	moo_bch_t*   utf16,
	moo_oow_t    size
);

MOO_EXPORT moo_oow_t moo_utf16_to_uc (
	const moo_bch_t* utf16,
	moo_oow_t        size,
	moo_uch_t*       uc
);

/* ------------------------------------------------------------------------- */

#if defined(MOO_HAVE_UINT16_T)
MOO_EXPORT moo_uint16_t moo_ntoh16 (
	moo_uint16_t x
);

MOO_EXPORT moo_uint16_t moo_hton16 (
	moo_uint16_t x
);
#endif

#if defined(MOO_HAVE_UINT32_T)
MOO_EXPORT moo_uint32_t moo_ntoh32 (
	moo_uint32_t x
);

MOO_EXPORT moo_uint32_t moo_hton32 (
	moo_uint32_t x
);
#endif

#if defined(MOO_HAVE_UINT64_T)
MOO_EXPORT moo_uint64_t moo_ntoh64 (
	moo_uint64_t x
);

MOO_EXPORT moo_uint64_t moo_hton64 (
	moo_uint64_t x
);
#endif

#if defined(MOO_HAVE_UINT128_T)
MOO_EXPORT moo_uint128_t moo_ntoh128 (
	moo_uint128_t x
);

MOO_EXPORT moo_uint128_t moo_hton128 (
	moo_uint128_t x
);
#endif

#if defined(__cplusplus)
}
#endif


#endif
