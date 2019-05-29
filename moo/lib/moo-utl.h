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
#include <stdarg.h>

/* =========================================================================
 * DOUBLY LINKED LIST
 * ========================================================================= */
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

/* =========================================================================
 * ENDIAN CHANGE OF A CONSTANT
 * ========================================================================= */
#define MOO_CONST_BSWAP16(x) \
	((moo_uint16_t)((((moo_uint16_t)(x) & ((moo_uint16_t)0xff << 0)) << 8) | \
	                (((moo_uint16_t)(x) & ((moo_uint16_t)0xff << 8)) >> 8)))

#define MOO_CONST_BSWAP32(x) \
	((moo_uint32_t)((((moo_uint32_t)(x) & ((moo_uint32_t)0xff <<  0)) << 24) | \
	                (((moo_uint32_t)(x) & ((moo_uint32_t)0xff <<  8)) <<  8) | \
	                (((moo_uint32_t)(x) & ((moo_uint32_t)0xff << 16)) >>  8) | \
	                (((moo_uint32_t)(x) & ((moo_uint32_t)0xff << 24)) >> 24)))

#if defined(MOO_HAVE_UINT64_T)
#define MOO_CONST_BSWAP64(x) \
	((moo_uint64_t)((((moo_uint64_t)(x) & ((moo_uint64_t)0xff <<  0)) << 56) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff <<  8)) << 40) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 16)) << 24) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 24)) <<  8) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 32)) >>  8) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 40)) >> 24) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 48)) >> 40) | \
	                (((moo_uint64_t)(x) & ((moo_uint64_t)0xff << 56)) >> 56)))
#endif

#if defined(MOO_HAVE_UINT128_T)
#define MOO_CONST_BSWAP128(x) \
	((moo_uint128_t)((((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 0)) << 120) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 8)) << 104) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 16)) << 88) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 24)) << 72) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 32)) << 56) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 40)) << 40) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 48)) << 24) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 56)) << 8) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 64)) >> 8) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 72)) >> 24) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 80)) >> 40) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 88)) >> 56) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 96)) >> 72) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 104)) >> 88) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 112)) >> 104) | \
	                 (((moo_uint128_t)(x) & ((moo_uint128_t)0xff << 120)) >> 120)))
#endif

#if defined(MOO_ENDIAN_LITTLE)

#	if defined(MOO_HAVE_UINT16_T)
#	define MOO_CONST_NTOH16(x) MOO_CONST_BSWAP16(x)
#	define MOO_CONST_HTON16(x) MOO_CONST_BSWAP16(x)
#	define MOO_CONST_HTOBE16(x) MOO_CONST_BSWAP16(x)
#	define MOO_CONST_HTOLE16(x) (x)
#	define MOO_CONST_BE16TOH(x) MOO_CONST_BSWAP16(x)
#	define MOO_CONST_LE16TOH(x) (x)
#	endif

#	if defined(MOO_HAVE_UINT32_T)
#	define MOO_CONST_NTOH32(x) MOO_CONST_BSWAP32(x)
#	define MOO_CONST_HTON32(x) MOO_CONST_BSWAP32(x)
#	define MOO_CONST_HTOBE32(x) MOO_CONST_BSWAP32(x)
#	define MOO_CONST_HTOLE32(x) (x)
#	define MOO_CONST_BE32TOH(x) MOO_CONST_BSWAP32(x)
#	define MOO_CONST_LE32TOH(x) (x)
#	endif

#	if defined(MOO_HAVE_UINT64_T)
#	define MOO_CONST_NTOH64(x) MOO_CONST_BSWAP64(x)
#	define MOO_CONST_HTON64(x) MOO_CONST_BSWAP64(x)
#	define MOO_CONST_HTOBE64(x) MOO_CONST_BSWAP64(x)
#	define MOO_CONST_HTOLE64(x) (x)
#	define MOO_CONST_BE64TOH(x) MOO_CONST_BSWAP64(x)
#	define MOO_CONST_LE64TOH(x) (x)
#	endif

#	if defined(MOO_HAVE_UINT128_T)
#	define MOO_CONST_NTOH128(x) MOO_CONST_BSWAP128(x)
#	define MOO_CONST_HTON128(x) MOO_CONST_BSWAP128(x)
#	define MOO_CONST_HTOBE128(x) MOO_CONST_BSWAP128(x)
#	define MOO_CONST_HTOLE128(x) (x)
#	define MOO_CONST_BE128TOH(x) MOO_CONST_BSWAP128(x)
#	define MOO_CONST_LE128TOH(x) (x)
#endif

#elif defined(MOO_ENDIAN_BIG)

#	if defined(MOO_HAVE_UINT16_T)
#	define MOO_CONST_NTOH16(x) (x)
#	define MOO_CONST_HTON16(x) (x)
#	define MOO_CONST_HTOBE16(x) (x)
#	define MOO_CONST_HTOLE16(x) MOO_CONST_BSWAP16(x)
#	define MOO_CONST_BE16TOH(x) (x)
#	define MOO_CONST_LE16TOH(x) MOO_CONST_BSWAP16(x)
#	endif

#	if defined(MOO_HAVE_UINT32_T)
#	define MOO_CONST_NTOH32(x) (x)
#	define MOO_CONST_HTON32(x) (x)
#	define MOO_CONST_HTOBE32(x) (x)
#	define MOO_CONST_HTOLE32(x) MOO_CONST_BSWAP32(x)
#	define MOO_CONST_BE32TOH(x) (x)
#	define MOO_CONST_LE32TOH(x) MOO_CONST_BSWAP32(x)
#	endif

#	if defined(MOO_HAVE_UINT64_T)
#	define MOO_CONST_NTOH64(x) (x)
#	define MOO_CONST_HTON64(x) (x)
#	define MOO_CONST_HTOBE64(x) (x)
#	define MOO_CONST_HTOLE64(x) MOO_CONST_BSWAP64(x)
#	define MOO_CONST_BE64TOH(x) (x)
#	define MOO_CONST_LE64TOH(x) MOO_CONST_BSWAP64(x)
#	endif

#	if defined(MOO_HAVE_UINT128_T)
#	define MOO_CONST_NTOH128(x) (x)
#	define MOO_CONST_HTON128(x) (x)
#	define MOO_CONST_HTOBE128(x) (x)
#	define MOO_CONST_HTOLE128(x) MOO_CONST_BSWAP128(x)
#	define MOO_CONST_BE128TOH(x) (x)
#	define MOO_CONST_LE128TOH(x) MOO_CONST_BSWAP128(x)
#	endif

#else
#	error UNKNOWN ENDIAN
#endif


/* =========================================================================
 * HASH
 * ========================================================================= */

#if (MOO_SIZEOF_OOW_T == 4)
#	define MOO_HASH_FNV_MAGIC_INIT (0x811c9dc5)
#	define MOO_HASH_FNV_MAGIC_PRIME (0x01000193)
#elif (MOO_SIZEOF_OOW_T == 8)
#	define MOO_HASH_FNV_MAGIC_INIT (0xCBF29CE484222325)
#	define MOO_HASH_FNV_MAGIC_PRIME (0x100000001B3l)
#elif (MOO_SIZEOF_OOW_T == 16)
#	define MOO_HASH_FNV_MAGIC_INIT (0x6C62272E07BB014262B821756295C58D)
#	define MOO_HASH_FNV_MAGIC_PRIME (0x1000000000000000000013B)
#endif

#if defined(MOO_HASH_FNV_MAGIC_INIT)
	/* FNV-1 hash */
#	define MOO_HASH_INIT MOO_HASH_FNV_MAGIC_INIT
#	define MOO_HASH_VALUE(hv,v) (((hv) ^ (v)) * MOO_HASH_FNV_MAGIC_PRIME)

#else
	/* SDBM hash */
#	define MOO_HASH_INIT 0
#	define MOO_HASH_VALUE(hv,v) (((hv) << 6) + ((hv) << 16) - (hv) + (v))
#endif

#define MOO_HASH_VPTL(hv, ptr, len, type) do { \
	hv = MOO_HASH_INIT; \
	MOO_HASH_MORE_VPTL (hv, ptr, len, type); \
} while(0)

#define MOO_HASH_MORE_VPTL(hv, ptr, len, type) do { \
	type* __moo_hash_more_vptl_p = (type*)(ptr); \
	type* __moo_hash_more_vptl_q = (type*)__moo_hash_more_vptl_p + (len); \
	while (__moo_hash_more_vptl_p < __moo_hash_more_vptl_q) \
	{ \
		hv = MOO_HASH_VALUE(hv, *__moo_hash_more_vptl_p); \
		__moo_hash_more_vptl_p++; \
	} \
} while(0)

#define MOO_HASH_VPTR(hv, ptr, type) do { \
	hv = MOO_HASH_INIT; \
	MOO_HASH_MORE_VPTR (hv, ptr, type); \
} while(0)

#define MOO_HASH_MORE_VPTR(hv, ptr, type) do { \
	type* __moo_hash_more_vptr_p = (type*)(ptr); \
	while (*__moo_hash_more_vptr_p) \
	{ \
		hv = MOO_HASH_VALUE(hv, *__moo_hash_more_vptr_p); \
		__moo_hash_more_vptr_p++; \
	} \
} while(0)

#define MOO_HASH_BYTES(hv, ptr, len) MOO_HASH_VPTL(hv, ptr, len, const moo_uint8_t)
#define MOO_HASH_MORE_BYTES(hv, ptr, len) MOO_HASH_MORE_VPTL(hv, ptr, len, const moo_uint8_t)

#define MOO_HASH_BCHARS(hv, ptr, len) MOO_HASH_VPTL(hv, ptr, len, const moo_bch_t)
#define MOO_HASH_MORE_BCHARS(hv, ptr, len) MOO_HASH_MORE_VPTL(hv, ptr, len, const moo_bch_t)

#define MOO_HASH_UCHARS(hv, ptr, len) MOO_HASH_VPTL(hv, ptr, len, const moo_uch_t)
#define MOO_HASH_MORE_UCHARS(hv, ptr, len) MOO_HASH_MORE_VPTL(hv, ptr, len, const moo_uch_t)

#define MOO_HASH_BCSTR(hv, ptr) MOO_HASH_VPTR(hv, ptr, const moo_bch_t)
#define MOO_HASH_MORE_BCSTR(hv, ptr) MOO_HASH_MORE_VPTR(hv, ptr, const moo_bch_t)

#define MOO_HASH_UCSTR(hv, ptr) MOO_HASH_VPTR(hv, ptr, const moo_uch_t)
#define MOO_HASH_MORE_UCSTR(hv, ptr) MOO_HASH_MORE_VPTR(hv, ptr, const moo_uch_t)

/* =========================================================================
 * CMGR
 * ========================================================================= */
enum moo_cmgr_id_t
{
	MOO_CMGR_UTF8,
	MOO_CMGR_UTF16,
	MOO_CMGR_MB8,
};
typedef enum moo_cmgr_id_t moo_cmgr_id_t;


/* =========================================================================
 * FORMATTED OUTPUT
 * ========================================================================= */
typedef struct moo_fmtout_t moo_fmtout_t;

typedef int (*moo_fmtout_putbcs_t) (
	moo_fmtout_t*     fmtout,
	const moo_bch_t*  ptr,
	moo_oow_t         len
);

typedef int (*moo_fmtout_putucs_t) (
	moo_fmtout_t*     fmtout,
	const moo_uch_t*  ptr,
	moo_oow_t         len
);

typedef int (*moo_fmtout_putobj_t) (
	moo_fmtout_t*     fmtout,
	moo_oop_t         obj
);

enum moo_fmtout_fmt_type_t 
{
	MOO_FMTOUT_FMT_TYPE_BCH = 0,
	MOO_FMTOUT_FMT_TYPE_UCH
};
typedef enum moo_fmtout_fmt_type_t moo_fmtout_fmt_type_t;


struct moo_fmtout_t
{
	moo_oow_t             count; /* out */

	moo_fmtout_putbcs_t   putbcs; /* in */
	moo_fmtout_putucs_t   putucs; /* in */
	moo_fmtout_putobj_t   putobj; /* in - %O is not handled if it's not set. */
	moo_bitmask_t         mask;   /* in */
	void*                 ctx;    /* in */

	moo_fmtout_fmt_type_t fmt_type;
	const void*           fmt_str;
};


#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT moo_oow_t moo_hash_bytes_ (
	const moo_oob_t* ptr,
	moo_oow_t        len
);

#if defined(MOO_HAVE_INLINE)
	static MOO_INLINE moo_oow_t moo_hash_bytes (const moo_oob_t* ptr, moo_oow_t len)
	{
		moo_oow_t hv;
		MOO_HASH_BYTES (hv, ptr, len);
		/* constrain the hash value to be representable in a small integer
		 * for convenience sake */
		return hv % ((moo_oow_t)MOO_SMOOI_MAX + 1);
	}

	static MOO_INLINE moo_oow_t moo_hash_bchars (const moo_bch_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_bch_t));
	}

	static MOO_INLINE moo_oow_t moo_hash_uchars (const moo_uch_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_uch_t));
	}

	static MOO_INLINE moo_oow_t moo_hash_words (const moo_oow_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oow_t));
	}

	static MOO_INLINE moo_oow_t moo_hash_halfwords (const moo_oohw_t* ptr, moo_oow_t len)
	{
		return moo_hash_bytes((const moo_oob_t*)ptr, len * MOO_SIZEOF(moo_oohw_t));
	}
#else
#	define moo_hash_bytes(ptr,len)     moo_hash_bytes_(ptr, len)
#	define moo_hash_bchars(ptr,len)    moo_hash_bytes_((const moo_oob_t*)(ptr), (len) * MOO_SIZEOF(moo_bch_t))
#	define moo_hash_uchars(ptr,len)    moo_hash_bytes_((const moo_oob_t*)(ptr), (len) * MOO_SIZEOF(moo_uch_t))
#	define moo_hash_words(ptr,len)     moo_hash_bytes_((const moo_oob_t*)(ptr), (len) * MOO_SIZEOF(moo_oow_t))
#	define moo_hash_halfwords(ptr,len) moo_hash_bytes_((const moo_oob_t*)(ptr), (len) * MOO_SIZEOF(moo_oohw_t))
#endif

#if defined(MOO_OOCH_IS_UCH)
#	define moo_hash_oochars(ptr,len) moo_hash_uchars(ptr,len)
#else
#	define moo_hash_oochars(ptr,len) moo_hash_bchars(ptr,len)
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

#define MOO_BYTE_TO_BCSTR_RADIXMASK (0xFF)
#define MOO_BYTE_TO_BCSTR_LOWERCASE (1 << 8)

moo_oow_t moo_byte_to_bcstr (
	moo_uint8_t   byte,  
	moo_bch_t*    buf,
	moo_oow_t     size,
	int           flagged_radix,
	moo_bch_t     fill
);

/* ------------------------------------------------------------------------- */

MOO_EXPORT int moo_get_ucwidth (
	moo_uch_t uc
);

/* ------------------------------------------------------------------------- */

#if defined(MOO_OOCH_IS_UCH)
#	define moo_conv_oocstr_to_bcstr_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr) moo_conv_ucstr_to_bcstr_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr)
#	define moo_conv_oochars_to_bchars_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr) moo_conv_uchars_to_bchars_with_cmgr(oocs,oocslen,bcs,bcslen,cmgr)
#else
#	define moo_conv_oocstr_to_ucstr_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr) moo_conv_bcstr_to_ucstr_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr,0)
#	define moo_conv_oochars_to_uchars_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr) moo_conv_bchars_to_uchars_with_cmgr(oocs,oocslen,ucs,ucslen,cmgr,0)
#endif


MOO_EXPORT int moo_conv_bcstr_to_ucstr_with_cmgr (
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

MOO_EXPORT int moo_conv_ucstr_to_bcstr_with_cmgr (
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

MOO_EXPORT moo_cmgr_t* moo_get_cmgr_by_id (
	moo_cmgr_id_t id
);

MOO_EXPORT moo_cmgr_t* moo_get_cmgr_by_bcstr (
	const moo_bch_t* name
);

MOO_EXPORT moo_cmgr_t* moo_get_cmgr_by_ucstr (
	const moo_uch_t* name
);

#if defined(MOO_OOCH_IS_UCH)
#	define moo_get_cmgr_by_name(name) moo_get_cmgr_by_ucstr(name)
#else
#	define moo_get_cmgr_by_name(name) moo_get_cmgr_by_bcstr(name)
#endif

#define moo_get_utf8_cmgr() moo_get_cmgr_by_id(MOO_CMGR_UTF8)
#define moo_get_utf16_cmgr() moo_get_cmgr_by_id(MOO_CMGR_UTF16)
#define moo_get_mb8_cmgr() moo_get_cmgr_by_id(MOO_CMGR_MB8)

/* ------------------------------------------------------------------------- */

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

MOO_EXPORT int moo_conv_uchars_to_mb8 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_conv_mb8_to_uchars (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);


MOO_EXPORT int moo_conv_ucstr_to_mb8 (
	const moo_uch_t*    ucs,
	moo_oow_t*          ucslen,
	moo_bch_t*          bcs,
	moo_oow_t*          bcslen
);

MOO_EXPORT int moo_conv_mb8_to_ucstr (
	const moo_bch_t*   bcs,
	moo_oow_t*         bcslen,
	moo_uch_t*         ucs,
	moo_oow_t*         ucslen
);

MOO_EXPORT moo_oow_t moo_uc_to_mb8 (
	moo_uch_t    uc,
	moo_bch_t*   mb8,
	moo_oow_t    size
);

MOO_EXPORT moo_oow_t moo_mb8_to_uc (
	const moo_bch_t* mb8,
	moo_oow_t        size,
	moo_uch_t*       uc
);

/* =========================================================================
 * FORMATTED OUTPUT
 * ========================================================================= */
MOO_EXPORT int moo_bfmt_outv (
	moo_fmtout_t*    fmtout,
	const moo_bch_t* fmt,
	va_list          ap
);

MOO_EXPORT int moo_ufmt_outv (
	moo_fmtout_t*    fmtout,
	const moo_uch_t* fmt,
	va_list          ap
);


MOO_EXPORT int moo_bfmt_out (
	moo_fmtout_t*    fmtout,
	const moo_bch_t* fmt,
	...
);

MOO_EXPORT int moo_ufmt_out (
	moo_fmtout_t*    fmtout,
	const moo_uch_t* fmt,
	...
);

/* =========================================================================
 * BIT SWAP
 * ========================================================================= */
#if defined(MOO_HAVE_INLINE)

#if defined(MOO_HAVE_UINT16_T)
static MOO_INLINE moo_uint16_t moo_bswap16 (moo_uint16_t x)
{
#if defined(MOO_HAVE_BUILTIN_BSWAP16)
	return __builtin_bswap16(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	__asm__ /*volatile*/ ("xchgb %b0, %h0" : "=Q"(x): "0"(x));
	return x;
#elif defined(__GNUC__) && defined(__arm__) && (defined(__ARM_ARCH) && (__ARM_ARCH >= 6))
	__asm__ /*volatile*/ ("rev16 %0, %0" : "+r"(x));
	return x;
#else
	return (x << 8) | (x >> 8);
#endif
}
#endif

#if defined(MOO_HAVE_UINT32_T)
static MOO_INLINE moo_uint32_t moo_bswap32 (moo_uint32_t x)
{
#if defined(MOO_HAVE_BUILTIN_BSWAP32)
	return __builtin_bswap32(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	__asm__ /*volatile*/ ("bswapl %0" : "=r"(x) : "0"(x));
	return x;
#elif defined(__GNUC__) && defined(__aarch64__)
	__asm__ /*volatile*/ ("rev32 %0, %0" : "+r"(x));
	return x;
#elif defined(__GNUC__) && defined(__arm__) && (defined(__ARM_ARCH) && (__ARM_ARCH >= 6))
	__asm__ /*volatile*/ ("rev %0, %0" : "+r"(x));
	return x;
#elif defined(__GNUC__) && defined(__ARM_ARCH)
	moo_uint32_t tmp;
	__asm__ /*volatile*/ (
		"eor %1, %0, %0, ror #16\n\t"
		"bic %1, %1, #0x00ff0000\n\t"
		"mov %0, %0, ror #8\n\t"
		"eor %0, %0, %1, lsr #8\n\t"
		:"+r"(x), "=&r"(tmp)
	);
	return x;
#else
	return ((x >> 24)) | 
	       ((x >>  8) & ((moo_uint32_t)0xff << 8)) | 
	       ((x <<  8) & ((moo_uint32_t)0xff << 16)) | 
	       ((x << 24));
#endif
}
#endif

#if defined(MOO_HAVE_UINT64_T)
static MOO_INLINE moo_uint64_t moo_bswap64 (moo_uint64_t x)
{
#if defined(MOO_HAVE_BUILTIN_BSWAP64)
	return __builtin_bswap64(x);
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64))
	__asm__ /*volatile*/ ("bswapq %0" : "=r"(x) : "0"(x));
	return x;
#elif defined(__GNUC__) && defined(__aarch64__)
	__asm__ /*volatile*/ ("rev %0, %0" : "+r"(x));
	return x;
#else
	return ((x >> 56)) | 
	       ((x >> 40) & ((moo_uint64_t)0xff << 8)) | 
	       ((x >> 24) & ((moo_uint64_t)0xff << 16)) | 
	       ((x >>  8) & ((moo_uint64_t)0xff << 24)) | 
	       ((x <<  8) & ((moo_uint64_t)0xff << 32)) | 
	       ((x << 24) & ((moo_uint64_t)0xff << 40)) | 
	       ((x << 40) & ((moo_uint64_t)0xff << 48)) | 
	       ((x << 56));
#endif
}
#endif

#if defined(MOO_HAVE_UINT128_T)
static MOO_INLINE moo_uint128_t moo_bswap128 (moo_uint128_t x)
{
#if defined(MOO_HAVE_BUILTIN_BSWAP128)
	return __builtin_bswap128(x);
#else
	return ((x >> 120)) | 
	       ((x >> 104) & ((moo_uint128_t)0xff << 8)) |
	       ((x >>  88) & ((moo_uint128_t)0xff << 16)) |
	       ((x >>  72) & ((moo_uint128_t)0xff << 24)) |
	       ((x >>  56) & ((moo_uint128_t)0xff << 32)) |
	       ((x >>  40) & ((moo_uint128_t)0xff << 40)) |
	       ((x >>  24) & ((moo_uint128_t)0xff << 48)) |
	       ((x >>   8) & ((moo_uint128_t)0xff << 56)) |
	       ((x <<   8) & ((moo_uint128_t)0xff << 64)) |
	       ((x <<  24) & ((moo_uint128_t)0xff << 72)) |
	       ((x <<  40) & ((moo_uint128_t)0xff << 80)) |
	       ((x <<  56) & ((moo_uint128_t)0xff << 88)) |
	       ((x <<  72) & ((moo_uint128_t)0xff << 96)) |
	       ((x <<  88) & ((moo_uint128_t)0xff << 104)) |
	       ((x << 104) & ((moo_uint128_t)0xff << 112)) |
	       ((x << 120));
#endif
}
#endif

#else

#if defined(MOO_HAVE_UINT16_T)
#	if defined(MOO_HAVE_BUILTIN_BSWAP16)
#	define moo_bswap16(x) ((moo_uint16_t)__builtin_bswap16((moo_uint16_t)(x)))
#	else 
#	define moo_bswap16(x) ((moo_uint16_t)(((moo_uint16_t)(x)) << 8) | (((moo_uint16_t)(x)) >> 8))
#	endif
#endif

#if defined(MOO_HAVE_UINT32_T)
#	if defined(MOO_HAVE_BUILTIN_BSWAP32)
#	define moo_bswap32(x) ((moo_uint32_t)__builtin_bswap32((moo_uint32_t)(x)))
#	else 
#	define moo_bswap32(x) ((moo_uint32_t)(((((moo_uint32_t)(x)) >> 24)) | \
	                                      ((((moo_uint32_t)(x)) >>  8) & ((moo_uint32_t)0xff << 8)) | \
	                                      ((((moo_uint32_t)(x)) <<  8) & ((moo_uint32_t)0xff << 16)) | \
	                                      ((((moo_uint32_t)(x)) << 24))))
#	endif
#endif

#if defined(MOO_HAVE_UINT64_T)
#	if defined(MOO_HAVE_BUILTIN_BSWAP64)
#	define moo_bswap64(x) ((moo_uint64_t)__builtin_bswap64((moo_uint64_t)(x)))
#	else 
#	define moo_bswap64(x) ((moo_uint64_t)(((((moo_uint64_t)(x)) >> 56)) | \
	                                      ((((moo_uint64_t)(x)) >> 40) & ((moo_uint64_t)0xff << 8)) | \
	                                      ((((moo_uint64_t)(x)) >> 24) & ((moo_uint64_t)0xff << 16)) | \
	                                      ((((moo_uint64_t)(x)) >>  8) & ((moo_uint64_t)0xff << 24)) | \
	                                      ((((moo_uint64_t)(x)) <<  8) & ((moo_uint64_t)0xff << 32)) | \
	                                      ((((moo_uint64_t)(x)) << 24) & ((moo_uint64_t)0xff << 40)) | \
	                                      ((((moo_uint64_t)(x)) << 40) & ((moo_uint64_t)0xff << 48)) | \
	                                      ((((moo_uint64_t)(x)) << 56))))
#	endif
#endif

#if defined(MOO_HAVE_UINT128_T)
#	if defined(MOO_HAVE_BUILTIN_BSWAP128)
#	define moo_bswap128(x) ((moo_uint128_t)__builtin_bswap128((moo_uint128_t)(x)))
#	else 
#	define moo_bswap128(x) ((moo_uint128_t)(((((moo_uint128_t)(x)) >> 120)) |  \
	                                        ((((moo_uint128_t)(x)) >> 104) & ((moo_uint128_t)0xff << 8)) | \
	                                        ((((moo_uint128_t)(x)) >>  88) & ((moo_uint128_t)0xff << 16)) | \
	                                        ((((moo_uint128_t)(x)) >>  72) & ((moo_uint128_t)0xff << 24)) | \
	                                        ((((moo_uint128_t)(x)) >>  56) & ((moo_uint128_t)0xff << 32)) | \
	                                        ((((moo_uint128_t)(x)) >>  40) & ((moo_uint128_t)0xff << 40)) | \
	                                        ((((moo_uint128_t)(x)) >>  24) & ((moo_uint128_t)0xff << 48)) | \
	                                        ((((moo_uint128_t)(x)) >>   8) & ((moo_uint128_t)0xff << 56)) | \
	                                        ((((moo_uint128_t)(x)) <<   8) & ((moo_uint128_t)0xff << 64)) | \
	                                        ((((moo_uint128_t)(x)) <<  24) & ((moo_uint128_t)0xff << 72)) | \
	                                        ((((moo_uint128_t)(x)) <<  40) & ((moo_uint128_t)0xff << 80)) | \
	                                        ((((moo_uint128_t)(x)) <<  56) & ((moo_uint128_t)0xff << 88)) | \
	                                        ((((moo_uint128_t)(x)) <<  72) & ((moo_uint128_t)0xff << 96)) | \
	                                        ((((moo_uint128_t)(x)) <<  88) & ((moo_uint128_t)0xff << 104)) | \
	                                        ((((moo_uint128_t)(x)) << 104) & ((moo_uint128_t)0xff << 112)) | \
	                                        ((((moo_uint128_t)(x)) << 120))))
#	endif
#endif

#endif /* MOO_HAVE_INLINE */


#if defined(MOO_ENDIAN_LITTLE)

#	if defined(MOO_HAVE_UINT16_T)
#	define moo_hton16(x) moo_bswap16(x)
#	define moo_ntoh16(x) moo_bswap16(x)
#	define moo_htobe16(x) moo_bswap16(x)
#	define moo_be16toh(x) moo_bswap16(x)
#	define moo_htole16(x) ((moo_uint16_t)(x))
#	define moo_le16toh(x) ((moo_uint16_t)(x))
#	endif

#	if defined(MOO_HAVE_UINT32_T)
#	define moo_hton32(x) moo_bswap32(x)
#	define moo_ntoh32(x) moo_bswap32(x)
#	define moo_htobe32(x) moo_bswap32(x)
#	define moo_be32toh(x) moo_bswap32(x)
#	define moo_htole32(x) ((moo_uint32_t)(x))
#	define moo_le32toh(x) ((moo_uint32_t)(x))
#	endif

#	if defined(MOO_HAVE_UINT64_T)
#	define moo_hton64(x) moo_bswap64(x)
#	define moo_ntoh64(x) moo_bswap64(x)
#	define moo_htobe64(x) moo_bswap64(x)
#	define moo_be64toh(x) moo_bswap64(x)
#	define moo_htole64(x) ((moo_uint64_t)(x))
#	define moo_le64toh(x) ((moo_uint64_t)(x))
#	endif

#	if defined(MOO_HAVE_UINT128_T)

#	define moo_hton128(x) moo_bswap128(x)
#	define moo_ntoh128(x) moo_bswap128(x)
#	define moo_htobe128(x) moo_bswap128(x)
#	define moo_be128toh(x) moo_bswap128(x)
#	define moo_htole128(x) ((moo_uint128_t)(x))
#	define moo_le128toh(x) ((moo_uint128_t)(x))
#	endif

#elif defined(MOO_ENDIAN_BIG)

#	if defined(MOO_HAVE_UINT16_T)
#	define moo_hton16(x) ((moo_uint16_t)(x))
#	define moo_ntoh16(x) ((moo_uint16_t)(x))
#	define moo_htobe16(x) ((moo_uint16_t)(x))
#	define moo_be16toh(x) ((moo_uint16_t)(x))
#	define moo_htole16(x) moo_bswap16(x)
#	define moo_le16toh(x) moo_bswap16(x)
#	endif

#	if defined(MOO_HAVE_UINT32_T)
#	define moo_hton32(x) ((moo_uint32_t)(x))
#	define moo_ntoh32(x) ((moo_uint32_t)(x))
#	define moo_htobe32(x) ((moo_uint32_t)(x))
#	define moo_be32toh(x) ((moo_uint32_t)(x))
#	define moo_htole32(x) moo_bswap32(x)
#	define moo_le32toh(x) moo_bswap32(x)
#	endif

#	if defined(MOO_HAVE_UINT64_T)
#	define moo_hton64(x) ((moo_uint64_t)(x))
#	define moo_ntoh64(x) ((moo_uint64_t)(x))
#	define moo_htobe64(x) ((moo_uint64_t)(x))
#	define moo_be64toh(x) ((moo_uint64_t)(x))
#	define moo_htole64(x) moo_bswap64(x)
#	define moo_le64toh(x) moo_bswap64(x)
#	endif

#	if defined(MOO_HAVE_UINT128_T)
#	define moo_hton128(x) ((moo_uint128_t)(x))
#	define moo_ntoh128(x) ((moo_uint128_t)(x))
#	define moo_htobe128(x) ((moo_uint128_t)(x))
#	define moo_be128toh(x) ((moo_uint128_t)(x))
#	define moo_htole128(x) moo_bswap128(x)
#	define moo_le128toh(x) moo_bswap128(x)
#	endif

#else
#	error UNKNOWN ENDIAN
#endif

/* =========================================================================
 * BIT POSITION
 * ========================================================================= */
static MOO_INLINE int moo_get_pos_of_msb_set_pow2 (moo_oow_t x)
{
	/* the caller must ensure that x is power of 2. if x happens to be zero,
	 * the return value is undefined as each method used may give different result. */
#if defined(MOO_HAVE_BUILTIN_CTZLL) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG_LONG)
	return __builtin_ctzll(x); /* count the number of trailing zeros */
#elif defined(MOO_HAVE_BUILTIN_CTZL) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG)
	return __builtin_ctzl(x); /* count the number of trailing zeros */
#elif defined(MOO_HAVE_BUILTIN_CTZ) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_INT)
	return __builtin_ctz(x); /* count the number of trailing zeros */
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	moo_oow_t pos;
	/* use the Bit Scan Forward instruction */
#if 1
	__asm__ volatile (
		"bsf %1,%0\n\t"
		: "=r"(pos) /* output */
		: "r"(x) /* input */
	);
#else
	__asm__ volatile (
		"bsf %[X],%[EXP]\n\t"
		: [EXP]"=r"(pos) /* output */
		: [X]"r"(x) /* input */
	);
#endif
	return (int)pos;
#elif defined(__GNUC__) && defined(__aarch64__) || (defined(__arm__) && (defined(__ARM_ARCH) && (__ARM_ARCH >= 5)))
	moo_oow_t n;
	/* CLZ is available in ARMv5T and above. there is no instruction to
	 * count trailing zeros or something similar. using RBIT with CLZ
	 * would be good in ARMv6T2 and above to avoid further calculation
	 * afte CLZ */
	__asm__ volatile (
		"clz %0,%1\n\t"
		: "=r"(n) /* output */
		: "r"(x) /* input */
	);
	return (int)(MOO_OOW_BITS - n - 1); 
	/* TODO: PPC - use cntlz, cntlzw, cntlzd, SPARC - use lzcnt, MIPS clz */
#else
	int pos = 0;
	while (x >>= 1) pos++;
	return pos;
#endif
}

static MOO_INLINE int moo_get_pos_of_msb_set (moo_oow_t x)
{
	/* x doesn't have to be power of 2. if x is zero, the result is undefined */
#if defined(MOO_HAVE_BUILTIN_CLZLL) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG_LONG)
	return MOO_OOW_BITS - __builtin_clzll(x) - 1; /* count the number of leading zeros */
#elif defined(MOO_HAVE_BUILTIN_CLZL) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_LONG)
	return MOO_OOW_BITS - __builtin_clzl(x) - 1; /* count the number of leading zeros */
#elif defined(MOO_HAVE_BUILTIN_CLZ) && (MOO_SIZEOF_OOW_T == MOO_SIZEOF_INT)
	return MOO_OOW_BITS - __builtin_clz(x) - 1; /* count the number of leading zeros */
#elif defined(__GNUC__) && (defined(__x86_64) || defined(__amd64) || defined(__i386) || defined(i386))
	/* bit scan reverse. not all x86 CPUs have LZCNT. */
	moo_oow_t pos;
	__asm__ volatile (
		"bsr %1,%0\n\t"
		: "=r"(pos) /* output */
		: "r"(x) /* input */
	);
	return (int)pos;
#elif defined(__GNUC__) && defined(__aarch64__) || (defined(__arm__) && (defined(__ARM_ARCH) && (__ARM_ARCH >= 5)))
	moo_oow_t n;
	__asm__ volatile (
		"clz %0,%1\n\t"
		: "=r"(n) /* output */
		: "r"(x) /* input */
	);
	return (int)(MOO_OOW_BITS - n - 1); 
	/* TODO: PPC - use cntlz, cntlzw, cntlzd, SPARC - use lzcnt, MIPS clz */
#else
	int pos = 0;
	while (x >>= 1) pos++;
	return pos;
#endif
}

#if defined(__cplusplus)
}
#endif

#endif
