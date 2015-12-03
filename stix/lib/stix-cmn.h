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

#ifndef _STIX_CMN_H_
#define _STIX_CMN_H_

/* WARNING: NEVER CHANGE/DELETE THE FOLLOWING STIX_HAVE_CFG_H DEFINITION. 
 *          IT IS USED FOR DEPLOYMENT BY MAKEFILE.AM */
/*#define STIX_HAVE_CFG_H*/

#if defined(STIX_HAVE_CFG_H)
#	include "stix-cfg.h"
#elif defined(_WIN32)
#	include "stix-msw.h"
#elif defined(__MSDOS__)
#	include "stix-dos.h"
#elif defined(macintosh)
#	include "stix-mac.h" /* class mac os */
#else
#	error UNSUPPORTED SYSTEM
#endif

#if defined(EMSCRIPTEN)
#	if defined(STIX_SIZEOF___INT128)
#		undef STIX_SIZEOF___INT128 
#		define STIX_SIZEOF___INT128 0
#	endif
#	if defined(STIX_SIZEOF_LONG) && defined(STIX_SIZEOF_INT) && (STIX_SIZEOF_LONG > STIX_SIZEOF_INT)
		/* autoconf doesn't seem to match actual emscripten */
#		undef STIX_SIZEOF_LONG
#		define STIX_SIZEOF_LONG STIX_SIZEOF_INT
#	endif
#endif

/* =========================================================================
 * PRIMITIVE TYPE DEFINTIONS
 * ========================================================================= */

/* stix_int8_t */
#if defined(STIX_SIZEOF_CHAR) && (STIX_SIZEOF_CHAR == 1)
#	define STIX_HAVE_UINT8_T
#	define STIX_HAVE_INT8_T
	typedef unsigned char      stix_uint8_t;
	typedef signed char        stix_int8_t;
#elif defined(STIX_SIZEOF___INT8) && (STIX_SIZEOF___INT8 == 1)
#	define STIX_HAVE_UINT8_T
#	define STIX_HAVE_INT8_T
	typedef unsigned __int8    stix_uint8_t;
	typedef signed __int8      stix_int8_t;
#elif defined(STIX_SIZEOF___INT8_T) && (STIX_SIZEOF___INT8_T == 1)
#	define STIX_HAVE_UINT8_T
#	define STIX_HAVE_INT8_T
	typedef unsigned __int8_t  stix_uint8_t;
	typedef signed __int8_t    stix_int8_t;
#else
#	define STIX_HAVE_UINT8_T
#	define STIX_HAVE_INT8_T
	typedef unsigned char      stix_uint8_t;
	typedef signed char        stix_int8_t;
#endif


/* stix_int16_t */
#if defined(STIX_SIZEOF_SHORT) && (STIX_SIZEOF_SHORT == 2)
#	define STIX_HAVE_UINT16_T
#	define STIX_HAVE_INT16_T
	typedef unsigned short int  stix_uint16_t;
	typedef signed short int    stix_int16_t;
#elif defined(STIX_SIZEOF___INT16) && (STIX_SIZEOF___INT16 == 2)
#	define STIX_HAVE_UINT16_T
#	define STIX_HAVE_INT16_T
	typedef unsigned __int16    stix_uint16_t;
	typedef signed __int16      stix_int16_t;
#elif defined(STIX_SIZEOF___INT16_T) && (STIX_SIZEOF___INT16_T == 2)
#	define STIX_HAVE_UINT16_T
#	define STIX_HAVE_INT16_T
	typedef unsigned __int16_t  stix_uint16_t;
	typedef signed __int16_t    stix_int16_t;
#else
#	define STIX_HAVE_UINT16_T
#	define STIX_HAVE_INT16_T
	typedef unsigned short int  stix_uint16_t;
	typedef signed short int    stix_int16_t;
#endif


/* stix_int32_t */
#if defined(STIX_SIZEOF_INT) && (STIX_SIZEOF_INT == 4)
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned int        stix_uint32_t;
	typedef signed int          stix_int32_t;
#elif defined(STIX_SIZEOF_LONG) && (STIX_SIZEOF_LONG == 4)
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned long       stix_uint32_t;
	typedef signed long         stix_int32_t;
#elif defined(STIX_SIZEOF___INT32) && (STIX_SIZEOF___INT32 == 4)
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned __int32    stix_uint32_t;
	typedef signed __int32      stix_int32_t;
#elif defined(STIX_SIZEOF___INT32_T) && (STIX_SIZEOF___INT32_T == 4)
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned __int32_t  stix_uint32_t;
	typedef signed __int32_t    stix_int32_t;
#elif defined(__MSDOS__)
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned long int   stix_uint32_t;
	typedef signed long int     stix_int32_t;
#else
#	define STIX_HAVE_UINT32_T
#	define STIX_HAVE_INT32_T
	typedef unsigned int        stix_uint32_t;
	typedef signed int          stix_int32_t;
#endif

/* stix_int64_t */
#if defined(STIX_SIZEOF_INT) && (STIX_SIZEOF_INT == 8)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned int        stix_uint64_t;
	typedef signed int          stix_int64_t;
#elif defined(STIX_SIZEOF_LONG) && (STIX_SIZEOF_LONG == 8)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned long       stix_uint64_t;
	typedef signed long         stix_int64_t;
#elif defined(STIX_SIZEOF_LONG_LONG) && (STIX_SIZEOF_LONG_LONG == 8)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned long long  stix_uint64_t;
	typedef signed long long    stix_int64_t;
#elif defined(STIX_SIZEOF___INT64) && (STIX_SIZEOF___INT64 == 8)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned __int64    stix_uint64_t;
	typedef signed __int64      stix_int64_t;
#elif defined(STIX_SIZEOF___INT64_T) && (STIX_SIZEOF___INT64_T == 8)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned __int64_t  stix_uint64_t;
	typedef signed __int64_t    stix_int64_t;
#elif defined(_WIN64) || defined(_WIN32)
#	define STIX_HAVE_UINT64_T
#	define STIX_HAVE_INT64_T
	typedef unsigned __int64  stix_uint64_t;
	typedef signed __int64    stix_int64_t;
#else
	/* no 64-bit integer */
#endif

/* stix_int128_t */
#if defined(STIX_SIZEOF_INT) && (STIX_SIZEOF_INT == 16)
#	define STIX_HAVE_UINT128_T
#	define STIX_HAVE_INT128_T
	typedef unsigned int        stix_uint128_t;
	typedef signed int          stix_int128_t;
#elif defined(STIX_SIZEOF_LONG) && (STIX_SIZEOF_LONG == 16)
#	define STIX_HAVE_UINT128_T
#	define STIX_HAVE_INT128_T
	typedef unsigned long       stix_uint128_t;
	typedef signed long         stix_int128_t;
#elif defined(STIX_SIZEOF_LONG_LONG) && (STIX_SIZEOF_LONG_LONG == 16)
#	define STIX_HAVE_UINT128_T
#	define STIX_HAVE_INT128_T
	typedef unsigned long long  stix_uint128_t;
	typedef signed long long    stix_int128_t;
#elif defined(STIX_SIZEOF___INT128) && (STIX_SIZEOF___INT128 == 16)
#	define STIX_HAVE_UINT128_T
#	define STIX_HAVE_INT128_T
	typedef unsigned __int128    stix_uint128_t;
	typedef signed __int128      stix_int128_t;
#elif defined(STIX_SIZEOF___INT128_T) && (STIX_SIZEOF___INT128_T == 16)
#	define STIX_HAVE_UINT128_T
#	define STIX_HAVE_INT128_T
	#if defined(__clang__)
	typedef __uint128_t  stix_uint128_t;
	typedef __int128_t   stix_int128_t;
	#else
	typedef unsigned __int128_t  stix_uint128_t;
	typedef signed __int128_t    stix_int128_t;
	#endif
#else
	/* no 128-bit integer */
#endif

#if defined(STIX_HAVE_UINT8_T) && (STIX_SIZEOF_VOID_P == 1)
#	error UNSUPPORTED POINTER SIZE
#elif defined(STIX_HAVE_UINT16_T) && (STIX_SIZEOF_VOID_P == 2)
	typedef stix_uint16_t stix_uintptr_t;
	typedef stix_int16_t stix_intptr_t;
	typedef stix_uint8_t stix_ushortptr_t;
	typedef stix_int8_t stix_shortptr_t;
#elif defined(STIX_HAVE_UINT32_T) && (STIX_SIZEOF_VOID_P == 4)
	typedef stix_uint32_t stix_uintptr_t;
	typedef stix_int32_t stix_intptr_t;
	typedef stix_uint16_t stix_ushortptr_t;
	typedef stix_int16_t stix_shortptr_t;
#elif defined(STIX_HAVE_UINT64_T) && (STIX_SIZEOF_VOID_P == 8)
	typedef stix_uint64_t stix_uintptr_t;
	typedef stix_int64_t stix_intptr_t;
	typedef stix_uint32_t stix_ushortptr_t;
	typedef stix_int32_t stix_shortptr_t;
#elif defined(STIX_HAVE_UINT128_T) && (STIX_SIZEOF_VOID_P == 16) 
	typedef stix_uint128_t stix_uintptr_t;
	typedef stix_int128_t stix_intptr_t;
	typedef stix_uint64_t stix_ushortptr_t;
	typedef stix_int64_t stix_shortptr_t;
#else
#	error UNKNOWN POINTER SIZE
#endif

#define STIX_SIZEOF_INTPTR_T STIX_SIZEOF_VOID_P
#define STIX_SIZEOF_UINTPTR_T STIX_SIZEOF_VOID_P
#define STIX_SIZEOF_SHORTPTR_T (STIX_SIZEOF_VOID_P / 2)
#define STIX_SIZEOF_USHORTPTR_T (STIX_SIZEOF_VOID_P / 2)

#if defined(STIX_HAVE_INT128_T)
#	define STIX_SIZEOF_INTMAX_T 16
#	define STIX_SIZEOF_UINTMAX_T 16
	typedef stix_int128_t stix_intmax_t;
	typedef stix_uint128_t stix_uintmax_t;
#elif defined(STIX_HAVE_INT64_T)
#	define STIX_SIZEOF_INTMAX_T 8
#	define STIX_SIZEOF_UINTMAX_T 8
	typedef stix_int64_t stix_intmax_t;
	typedef stix_uint64_t stix_uintmax_t;
#elif defined(STIX_HAVE_INT32_T)
#	define STIX_SIZEOF_INTMAX_T 4
#	define STIX_SIZEOF_UINTMAX_T 4
	typedef stix_int32_t stix_intmax_t;
	typedef stix_uint32_t stix_uintmax_t;
#elif defined(STIX_HAVE_INT16_T)
#	define STIX_SIZEOF_INTMAX_T 2
#	define STIX_SIZEOF_UINTMAX_T 2
	typedef stix_int16_t stix_intmax_t;
	typedef stix_uint16_t stix_uintmax_t;
#elif defined(STIX_HAVE_INT8_T)
#	define STIX_SIZEOF_INTMAX_T 1
#	define STIX_SIZEOF_UINTMAX_T 1
	typedef stix_int8_t stix_intmax_t;
	typedef stix_uint8_t stix_uintmax_t;
#else
#	error UNKNOWN INTMAX SIZE
#endif


typedef stix_uintptr_t stix_size_t;
typedef stix_intptr_t stix_ssize_t;

typedef char          stix_bch_t;
typedef stix_uint16_t stix_uch_t; /* TODO ... wchar_t??? */
typedef stix_int32_t  stix_uci_t;

struct stix_ucs_t
{
	stix_uch_t* ptr;
	stix_size_t len;
};
typedef struct stix_ucs_t stix_ucs_t;


/* =========================================================================
 * PRIMITIVE MACROS
 * ========================================================================= */
#define STIX_UCI_EOF ((stix_ooci_t)-1)
#define STIX_UCI_NL  ((stix_ooci_t)'\n')

#define STIX_SIZEOF(x) (sizeof(x))
#define STIX_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

/**
 * The STIX_OFFSETOF() macro returns the offset of a field from the beginning
 * of a structure.
 */
#define STIX_OFFSETOF(type,member) ((stix_uintptr_t)&((type*)0)->member)

/**
 * The STIX_ALIGNOF() macro returns the alignment size of a structure.
 * Note that this macro may not work reliably depending on the type given.
 */
#define STIX_ALIGNOF(type) STIX_OFFSETOF(struct { stix_uint8_t d1; type d2; }, d2)
        /*(sizeof(struct { stix_uint8_t d1; type d2; }) - sizeof(type))*/

#if defined(__cplusplus)
#	if (__cplusplus >= 201103L) /* C++11 */
#		define STIX_NULL nullptr
#	else
#		define STIX_NULL (0)
#	endif
#else
#	define STIX_NULL ((void*)0)
#endif

/* make a low bit mask that can mask off low n bits*/
#define STIX_LBMASK(type,n) (~(~((type)0) << (n))) 

/* get 'length' bits starting from the bit at the 'offset' */
#define STIX_GETBITS(type,value,offset,length) \
	((((type)(value)) >> (offset)) & STIX_LBMASK(type,length))

#define STIX_SETBITS(type,value,offset,length,bits) \
	(value = (((type)(value)) | (((bits) & STIX_LBMASK(type,length)) << (offset))))


/** 
 * The STIX_BITS_MAX() macros calculates the maximum value that the 'nbits'
 * bits of an unsigned integer of the given 'type' can hold.
 * \code
 * printf ("%u", STIX_BITS_MAX(unsigned int, 5));
 * \endcode
 */
/*#define STIX_BITS_MAX(type,nbits) ((((type)1) << (nbits)) - 1)*/
#define STIX_BITS_MAX(type,nbits) ((~(type)0) >> (STIX_SIZEOF(type) * 8 - (nbits)))

/* =========================================================================
 * POINTER MANIPULATION MACROS
 * ========================================================================= */

/*
#if defined(__MSDOS__)
#	define STIX_INCPTR(type,base,inc)  (((type __huge*)base) + (inc))
#	define STIX_DECPTR(type,base,inc)  (((type __huge*)base) - (inc))
#	define STIX_GTPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) > ((type __huge*)ptr2))
#	define STIX_GEPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) >= ((type __huge*)ptr2))
#	define STIX_LTPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) < ((type __huge*)ptr2))
#	define STIX_LEPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) <= ((type __huge*)ptr2))
#	define STIX_EQPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) == ((type __huge*)ptr2))
#	define STIX_SUBPTR(type,ptr1,ptr2) (((type __huge*)ptr1) - ((type __huge*)ptr2))
#else 
*/
#	define STIX_INCPTR(type,base,inc)  (((type*)base) + (inc))
#	define STIX_DECPTR(type,base,inc)  (((type*)base) - (inc))
#	define STIX_GTPTR(type,ptr1,ptr2)  (((type*)ptr1) > ((type*)ptr2))
#	define STIX_GEPTR(type,ptr1,ptr2)  (((type*)ptr1) >= ((type*)ptr2))
#	define STIX_LTPTR(type,ptr1,ptr2)  (((type*)ptr1) < ((type*)ptr2))
#	define STIX_LEPTR(type,ptr1,ptr2)  (((type*)ptr1) <= ((type*)ptr2))
#	define STIX_EQPTR(type,ptr1,ptr2)  (((type*)ptr1) == ((type*)ptr2))
#	define STIX_SUBPTR(type,ptr1,ptr2) (((type*)ptr1) - ((type*)ptr2))
/*
#endif
*/

/* =========================================================================
 * MMGR
 * ========================================================================= */
typedef struct stix_mmgr_t stix_mmgr_t;

/** 
 * allocate a memory chunk of the size \a n.
 * \return pointer to a memory chunk on success, #STIX_NULL on failure.
 */
typedef void* (*stix_mmgr_alloc_t)   (stix_mmgr_t* mmgr, stix_size_t n);
/** 
 * resize a memory chunk pointed to by \a ptr to the size \a n.
 * \return pointer to a memory chunk on success, #STIX_NULL on failure.
 */
typedef void* (*stix_mmgr_realloc_t) (stix_mmgr_t* mmgr, void* ptr, stix_size_t n);
/**
 * free a memory chunk pointed to by \a ptr.
 */
typedef void  (*stix_mmgr_free_t)    (stix_mmgr_t* mmgr, void* ptr);

/**
 * The stix_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer \a ctx is passed to each memory management 
 * function whenever it is called. You can allocate, reallocate, and free 
 * a memory chunk.
 *
 * For example, a stix_xxx_open() function accepts a pointer of the stix_mmgr_t
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct stix_mmgr_t
{
	stix_mmgr_alloc_t   alloc;   /**< allocation function */
	stix_mmgr_realloc_t realloc; /**< resizing function */
	stix_mmgr_free_t    free;    /**< disposal function */
	void*               ctx;     /**< user-defined data pointer */
};

/**
 * The STIX_MMGR_ALLOC() macro allocates a memory block of the \a size bytes
 * using the \a mmgr memory manager.
 */
#define STIX_MMGR_ALLOC(mmgr,size) ((mmgr)->alloc(mmgr,size))

/**
 * The STIX_MMGR_REALLOC() macro resizes a memory block pointed to by \a ptr 
 * to the \a size bytes using the \a mmgr memory manager.
 */
#define STIX_MMGR_REALLOC(mmgr,ptr,size) ((mmgr)->realloc(mmgr,ptr,size))

/** 
 * The STIX_MMGR_FREE() macro deallocates the memory block pointed to by \a ptr.
 */
#define STIX_MMGR_FREE(mmgr,ptr) ((mmgr)->free(mmgr,ptr))


/* =========================================================================
 * CMGR
 * =========================================================================*/

typedef struct stix_cmgr_t stix_cmgr_t;

typedef stix_size_t (*stix_cmgr_bctouc_t) (
	const stix_bch_t*   mb, 
	stix_size_t         size,
	stix_uch_t*         wc
);

typedef stix_size_t (*stix_cmgr_uctobc_t) (
	stix_uch_t    wc,
	stix_bch_t*   mb,
	stix_size_t   size
);

/**
 * The stix_cmgr_t type defines the character-level interface to 
 * multibyte/wide-character conversion. This interface doesn't 
 * provide any facility to store conversion state in a context
 * independent manner. This leads to the limitation that it can
 * handle a stateless multibyte encoding only.
 */
struct stix_cmgr_t
{
	stix_cmgr_bctouc_t bctouc;
	stix_cmgr_uctobc_t uctobc;
};

/* =========================================================================
 * MACROS THAT CHANGES THE BEHAVIORS OF THE C COMPILER/LINKER
 * =========================================================================*/

#if defined(_WIN32) || (defined(__WATCOMC__) && !defined(__WINDOWS_386__))
#	define STIX_IMPORT __declspec(dllimport)
#	define STIX_EXPORT __declspec(dllexport)
#	define STIX_PRIVATE 
#elif defined(__GNUC__) && (__GNUC__>=4)
#	define STIX_IMPORT __attribute__((visibility("default")))
#	define STIX_EXPORT __attribute__((visibility("default")))
#	define STIX_PRIVATE __attribute__((visibility("hidden")))
/*#	define STIX_PRIVATE __attribute__((visibility("internal")))*/
#else
#	define STIX_IMPORT
#	define STIX_EXPORT
#	define STIX_PRIVATE
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L)
#	define STIX_INLINE inline
#	define STIX_HAVE_INLINE
#elif defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
		/* gcc disables inline when -std=c89 or -ansi is used. 
		 * so use __inline__ supported by gcc regardless of the options */
#	define STIX_INLINE /*extern*/ __inline__
#	define STIX_HAVE_INLINE
#else
#	define STIX_INLINE 
#	undef STIX_HAVE_INLINE
#endif




/**
 * The STIX_TYPE_IS_SIGNED() macro determines if a type is signed.
 * \code
 * printf ("%d\n", (int)STIX_TYPE_IS_SIGNED(int));
 * printf ("%d\n", (int)STIX_TYPE_IS_SIGNED(unsigned int));
 * \endcode
 */
#define STIX_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))

/**
 * The STIX_TYPE_IS_SIGNED() macro determines if a type is unsigned.
 * \code
 * printf ("%d\n", STIX_TYPE_IS_UNSIGNED(int));
 * printf ("%d\n", STIX_TYPE_IS_UNSIGNED(unsigned int));
 * \endcode
 */
#define STIX_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))

#define STIX_TYPE_SIGNED_MAX(type) \
	((type)~((type)1 << (STIX_SIZEOF(type) * 8 - 1)))
#define STIX_TYPE_UNSIGNED_MAX(type) ((type)(~(type)0))

#define STIX_TYPE_SIGNED_MIN(type) \
	((type)((type)1 << (STIX_SIZEOF(type) * 8 - 1)))
#define STIX_TYPE_UNSIGNED_MIN(type) ((type)0)

#define STIX_TYPE_MAX(type) \
	((STIX_TYPE_IS_SIGNED(type)? STIX_TYPE_SIGNED_MAX(type): STIX_TYPE_UNSIGNED_MAX(type)))
#define STIX_TYPE_MIN(type) \
	((STIX_TYPE_IS_SIGNED(type)? STIX_TYPE_SIGNED_MIN(type): STIX_TYPE_UNSIGNED_MIN(type)))


/* =========================================================================
 * BASIC STIX TYPES
 * =========================================================================*/

typedef stix_uint8_t             stix_oob_t;

/* NOTE: sizeof(stix_oop_t) must be equal to sizeof(stix_oow_t) */
typedef stix_uintptr_t           stix_oow_t;
typedef stix_intptr_t            stix_ooi_t;
#define STIX_SIZEOF_OOW_T STIX_SIZEOF_UINTPTR_T
#define STIX_SIZEOF_OOI_T STIX_SIZEOF_INTPTR_T

typedef stix_ushortptr_t         stix_oohw_t; /* half word - half word */
typedef stix_shortptr_t          stix_oohi_t; /* signed half word */
#define STIX_SIZEOF_OOHW_T STIX_SIZEOF_USHORTPTR_T
#define STIX_SIZEOF_OOHI_T STIX_SIZEOF_SHORTPTR_T

typedef stix_uch_t               stix_ooch_t;
typedef stix_uci_t               stix_ooci_t;
typedef stix_ucs_t               stix_oocs_t;
#define STIX_OOCH_IS_UCH


/* =========================================================================
 * COMPILER FEATURE TEST MACROS
 * =========================================================================*/
#if defined(__has_builtin)
	#if __has_builtin(__builtin_ctz)
		#define STIX_HAVE_BUILTIN_CTZ
	#endif

	#if __has_builtin(__builtin_uadd_overflow)
		#define STIX_HAVE_BUILTIN_UADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddl_overflow)
		#define STIX_HAVE_BUILTIN_UADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddll_overflow)
		#define STIX_HAVE_BUILTIN_UADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umul_overflow)
		#define STIX_HAVE_BUILTIN_UMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umull_overflow)
		#define STIX_HAVE_BUILTIN_UMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umulll_overflow)
		#define STIX_HAVE_BUILTIN_UMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_sadd_overflow)
		#define STIX_HAVE_BUILTIN_SADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddl_overflow)
		#define STIX_HAVE_BUILTIN_SADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddll_overflow)
		#define STIX_HAVE_BUILTIN_SADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smul_overflow)
		#define STIX_HAVE_BUILTIN_SMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smull_overflow)
		#define STIX_HAVE_BUILTIN_SMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smulll_overflow)
		#define STIX_HAVE_BUILTIN_SMULLL_OVERFLOW 
	#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)

	#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
		#define STIX_HAVE_BUILTIN_CTZ
	#endif

	#if (__GNUC__ >= 5)
		#define STIX_HAVE_BUILTIN_UADD_OVERFLOW
		#define STIX_HAVE_BUILTIN_UADDL_OVERFLOW
		#define STIX_HAVE_BUILTIN_UADDLL_OVERFLOW
		#define STIX_HAVE_BUILTIN_UMUL_OVERFLOW
		#define STIX_HAVE_BUILTIN_UMULL_OVERFLOW
		#define STIX_HAVE_BUILTIN_UMULLL_OVERFLOW

		#define STIX_HAVE_BUILTIN_SADD_OVERFLOW
		#define STIX_HAVE_BUILTIN_SADDL_OVERFLOW
		#define STIX_HAVE_BUILTIN_SADDLL_OVERFLOW
		#define STIX_HAVE_BUILTIN_SMUL_OVERFLOW
		#define STIX_HAVE_BUILTIN_SMULL_OVERFLOW
		#define STIX_HAVE_BUILTIN_SMULLL_OVERFLOW
	#endif

#endif

/*
#if !defined(__has_builtin)
	#define __has_builtin(x) 0
#endif


#if !defined(__is_identifier)
	#define __is_identifier(x) 0
#endif

#if !defined(__has_attribute)
	#define __has_attribute(x) 0
#endif
*/



#endif
