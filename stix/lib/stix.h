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

#ifndef _STIX_H_
#define _STIX_H_


/* TODO: move this macro out to the build files.... */
#define STIX_INCLUDE_COMPILER

/* =========================================================================
 * PRIMITIVE TYPE DEFINTIONS
 * ========================================================================= */
/* TODO: define these types and macros using autoconf */
typedef unsigned char      stix_uint8_t;
typedef signed char        stix_int8_t;

typedef unsigned short int stix_uint16_t;
typedef signed short int stix_int16_t;

#if defined(__MSDOS__)
	typedef unsigned long int stix_uint32_t;
	typedef signed long int stix_int32_t;
#else
	typedef unsigned int stix_uint32_t;
	typedef signed int stix_int32_t;
#endif

#if defined(_WIN64)
	typedef unsigned __int64  stix_uintptr_t;
	typedef signed __int64    stix_intptr_t;
	typedef unsigned __int64  stix_size_t;
	typedef signed __int64    stix_ssize_t;
#else
	typedef unsigned long int stix_uintptr_t;
	typedef signed long int   stix_intptr_t;
	typedef unsigned long int stix_size_t;
	typedef signed long int   stix_ssize_t;
#endif

typedef stix_uint8_t  stix_byte_t;
typedef stix_uint16_t stix_uch_t; /* TODO ... wchar_t??? */
typedef stix_int32_t  stix_uci_t;
typedef char          stix_bch_t;

struct stix_ucs_t
{
	stix_uch_t* ptr;
	stix_size_t len;
};
typedef struct stix_ucs_t stix_ucs_t;

/* =========================================================================
 * PRIMITIVE MACROS
 * ========================================================================= */
#define STIX_UCI_EOF ((stix_uci_t)-1)
#define STIX_UCI_NL  ((stix_uci_t)'\n')

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
#	define STIX_INCPTR(type,base,inc)  (((type*)base) + (inc))
#	define STIX_DECPTR(type,base,inc)  (((type*)base) - (inc))
#	define STIX_GTPTR(type,ptr1,ptr2)  (((type*)ptr1) > ((type*)ptr2))
#	define STIX_GEPTR(type,ptr1,ptr2)  (((type*)ptr1) >= ((type*)ptr2))
#	define STIX_LTPTR(type,ptr1,ptr2)  (((type*)ptr1) < ((type*)ptr2))
#	define STIX_LEPTR(type,ptr1,ptr2)  (((type*)ptr1) <= ((type*)ptr2))
#	define STIX_EQPTR(type,ptr1,ptr2)  (((type*)ptr1) == ((type*)ptr2))
#	define STIX_SUBPTR(type,ptr1,ptr2) (((type*)ptr1) - ((type*)ptr2))
#endif

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


/* ========================================================================== */

/**
 * The stix_errnum_t type defines the error codes.
 */
enum stix_errnum_t
{
	STIX_ENOERR,  /**< no error */
	STIX_EOTHER,  /**< other error */
	STIX_ENOIMPL, /**< not implemented */
	STIX_ESYSERR, /**< subsystem error */
	STIX_EINTERN, /**< internal error */
	STIX_ENOMEM,  /**< insufficient memory */
	STIX_EINVAL,  /**< invalid parameter or data */
	STIX_ERANGE,  /**< range error. overflow and underflow */
	STIX_ENOENT,  /**< no matching entry */
	STIX_EIOERR,  /**< I/O error */
	STIX_EECERR,  /**< encoding conversion error */

#if defined(STIX_INCLUDE_COMPILER)
	STIX_ESYNTAX /** < syntax error */
#endif
};
typedef enum stix_errnum_t stix_errnum_t;

enum stix_option_t
{
	STIX_TRAIT,
	STIX_DFL_SYMTAB_SIZE,
	STIX_DFL_SYSDIC_SIZE
};
typedef enum stix_option_t stix_option_t;

enum stix_trait_t
{
	/* perform no garbage collection when the heap is full. 
	 * you still can use stix_gc() explicitly. */
	STIX_NOGC = (1 << 0)
};
typedef enum stix_trait_t stix_trait_t;

/* NOTE: sizeof(stix_oop_t) must be equal to sizeof(stix_oow_t) */
typedef stix_uintptr_t           stix_oow_t;
typedef stix_intptr_t            stix_ooi_t;
typedef struct stix_obj_t        stix_obj_t;
typedef struct stix_obj_t*       stix_oop_t;

/* these are more specialized types for stix_obj_t */
typedef struct stix_obj_oop_t     stix_obj_oop_t;
typedef struct stix_obj_char_t    stix_obj_char_t;
typedef struct stix_obj_byte_t    stix_obj_byte_t;
typedef struct stix_obj_word_t    stix_obj_word_t;

/* these are more specialized types for stix_oop_t */
typedef struct stix_obj_oop_t*    stix_oop_oop_t;
typedef struct stix_obj_char_t*   stix_oop_char_t;
typedef struct stix_obj_byte_t*   stix_oop_byte_t;
typedef struct stix_obj_word_t*   stix_oop_word_t;

#define STIX_OOW_BITS (STIX_SIZEOF(stix_oow_t) * 8)
#define STIX_OOP_BITS (STIX_SIZEOF(stix_oop_t) * 8)


/* 
 * OOP encoding
 * An object pointer(OOP) is an ordinary pointer value to an object.
 * but some simple numeric values are also encoded into OOP using a simple
 * bit-shifting and masking.
 *
 * A real OOP is stored without any bit-shifting while a non-OOP value encoded
 * in an OOP is bit-shifted to the left by 2 and the 2 least-significant bits
 * are set to 1 or 2.
 * 
 * This scheme works because the object allocators aligns the object size to
 * a multiple of sizeof(stix_oop_t). This way, the 2 least-significant bits
 * of a real OOP are always 0s.
 */

#define STIX_OOP_TAG_BITS  2
#define STIX_OOP_TAG_SMINT 1
#define STIX_OOP_TAG_CHAR  2

#define STIX_OOP_IS_NUMERIC(oop) (((stix_oow_t)oop) & (STIX_OOP_TAG_SMINT | STIX_OOP_TAG_CHAR))
#define STIX_OOP_IS_POINTER(oop) (!STIX_OOP_IS_NUMERIC(oop))
#define STIX_OOP_GET_TAG(oop) (((stix_oow_t)oop) & STIX_LBMASK(stix_oow_t, STIX_OOP_TAG_BITS))

#define STIX_OOP_IS_SMINT(oop) (((stix_ooi_t)oop) & STIX_OOP_TAG_SMINT)
#define STIX_OOP_IS_CHAR(oop) (((stix_oow_t)oop) & STIX_OOP_TAG_CHAR)
#define STIX_OOP_FROM_SMINT(num) ((stix_oop_t)((((stix_ooi_t)(num)) << STIX_OOP_TAG_BITS) | STIX_OOP_TAG_SMINT))
#define STIX_OOP_TO_SMINT(oop) (((stix_ooi_t)oop) >> STIX_OOP_TAG_BITS)
#define STIX_OOP_FROM_CHAR(num) ((stix_oop_t)((((stix_oow_t)(num)) << STIX_OOP_TAG_BITS) | STIX_OOP_TAG_CHAR))
#define STIX_OOP_TO_CHAR(oop) (((stix_oow_t)oop) >> STIX_OOP_TAG_BITS)

#define STIX_SMINT_BITS (STIX_SIZEOF(stix_ooi_t) * 8 - STIX_OOP_TAG_BITS)
#define STIX_SMINT_MAX ((stix_ooi_t)(~((stix_oow_t)0) >> (STIX_OOP_TAG_BITS + 1)))
#define STIX_SMINT_MIN (-STIX_SMINT_MAX - 1)
#define STIX_OOI_IN_SMINT_RANGE(ooi)  ((ooi) >= STIX_SMINT_MIN && (ooi) <= STIX_SMINT_MAX)

/* TODO: There are untested code where smint is converted to stix_oow_t.
 *       for example, the spec making macro treats the number as stix_oow_t instead of stix_ooi_t.
 *       as of this writing, i skip testing that part with the spec value exceeding STIX_SMINT_MAX.
 *       later, please verify it works, probably by limiting the value ranges in such macros
 */

/*
 * Object structure
 */
enum stix_obj_type_t
{
	STIX_OBJ_TYPE_OOP,
	STIX_OBJ_TYPE_CHAR,
	STIX_OBJ_TYPE_BYTE,
	STIX_OBJ_TYPE_WORD

/*
	STIX_OBJ_TYPE_UINT8,
	STIX_OBJ_TYPE_UINT16,
	STIX_OBJ_TYPE_UINT32,
*/

/* NOTE: you can have STIX_OBJ_SHORT, STIX_OBJ_INT
 * STIX_OBJ_LONG, STIX_OBJ_FLOAT, STIX_OBJ_DOUBLE, etc 
 * type type field being 6 bits long, you can have up to 64 different types.

	STIX_OBJ_TYPE_SHORT,
	STIX_OBJ_TYPE_INT,
	STIX_OBJ_TYPE_LONG,
	STIX_OBJ_TYPE_FLOAT,
	STIX_OBJ_TYPE_DOUBLE */
};
typedef enum stix_obj_type_t stix_obj_type_t;

/* =========================================================================
 * Object header structure 
 * 
 * _flags:
 *   type: the type of a payload item. 
 *         one of STIX_OBJ_TYPE_OOP, STIX_OBJ_TYPE_CHAR, 
 *                STIX_OBJ_TYPE_BYTE, STIX_OBJ_TYPE_WORD
 *   unit: the size of a payload item in bytes. 
 *   extra: 0 or 1. 1 indicates that the payload contains 1 more
 *          item than the value of the size field. used for a 
 *          terminating null in a variable-char object. internel
 *          use only.
 *   kernel: 0 or 1. indicates that the object is a kernel object.
 *           VM disallows layout changes of a kernel object.
 *           internal use only.
 *   moved: 0 or 1. used by GC. internal use only.
 *   trailer: 0 or 1. indicates that there are trailing bytes
 *            after the object payload. internal use only.
 *
 * _size: the number of payload items in an object.
 *        it doesn't include the header size.
 * 
 * The total number of bytes occupied by an object can be calculated
 * with this fomula:
 *    sizeof(stix_obj_t) + ALIGN((size + extra) * unit), sizeof(stix_oop_t))
 * 
 * If the type is known to be not STIX_OBJ_TYPE_CHAR, you can assume that 
 * 'extra' is 0. So you can simplify the fomula in such a context.
 *    sizeof(stix_obj_t) + ALIGN(size * unit), sizeof(stix_oop_t))
 *
 * The ALIGN() macro is used above since allocation adjusts the payload
 * size to a multiple of sizeof(stix_oop_t). it assumes that sizeof(stix_obj_t)
 * is a multiple of sizeof(stix_oop_t). See the STIX_BYTESOF() macro.
 * 
 * Due to the header structure, an object can only contain items of
 * homogeneous data types in the payload. 
 *
 * It's actually possible to split the size field into 2. For example,
 * the upper half contains the number of oops and the lower half contains
 * the number of bytes/chars. This way, a variable-byte class or a variable-char
 * class can have normal instance variables. On the contrary, the actual byte
 * size calculation and the access to the payload fields become more complex. 
 * Therefore, i've dropped the idea.
 * ========================================================================= */
#define STIX_OBJ_FLAGS_TYPE_BITS    6
#define STIX_OBJ_FLAGS_UNIT_BITS    5
#define STIX_OBJ_FLAGS_EXTRA_BITS   1
#define STIX_OBJ_FLAGS_KERNEL_BITS  2
#define STIX_OBJ_FLAGS_MOVED_BITS   1
#define STIX_OBJ_FLAGS_TRAILER_BITS 1

#define STIX_OBJ_GET_FLAGS_TYPE(oop)       STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS), STIX_OBJ_FLAGS_TYPE_BITS)
#define STIX_OBJ_GET_FLAGS_UNIT(oop)       STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                            STIX_OBJ_FLAGS_UNIT_BITS)
#define STIX_OBJ_GET_FLAGS_EXTRA(oop)      STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                                                        STIX_OBJ_FLAGS_EXTRA_BITS)
#define STIX_OBJ_GET_FLAGS_KERNEL(oop)     STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                                                                                     STIX_OBJ_FLAGS_KERNEL_BITS)
#define STIX_OBJ_GET_FLAGS_MOVED(oop)      STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_TRAILER_BITS),                                                                                                                 STIX_OBJ_FLAGS_MOVED_BITS)
#define STIX_OBJ_GET_FLAGS_TRAILER(oop)    STIX_GETBITS(stix_oow_t, (oop)->_flags, 0,                                                                                                                                             STIX_OBJ_FLAGS_TRAILER_BITS)

#define STIX_OBJ_SET_FLAGS_TYPE(oop,v)     STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS), STIX_OBJ_FLAGS_TYPE_BITS,     v)
#define STIX_OBJ_SET_FLAGS_UNIT(oop,v)     STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                            STIX_OBJ_FLAGS_UNIT_BITS,     v)
#define STIX_OBJ_SET_FLAGS_EXTRA(oop,v)    STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                                                        STIX_OBJ_FLAGS_EXTRA_BITS,    v)
#define STIX_OBJ_SET_FLAGS_KERNEL(oop,v)   STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS),                                                                                     STIX_OBJ_FLAGS_KERNEL_BITS,   v)
#define STIX_OBJ_SET_FLAGS_MOVED(oop,v)    STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_TRAILER_BITS),                                                                                                                 STIX_OBJ_FLAGS_MOVED_BITS,    v)
#define STIX_OBJ_SET_FLAGS_TRAILER(oop,v)  STIX_SETBITS(stix_oow_t, (oop)->_flags, 0,                                                                                                                                             STIX_OBJ_FLAGS_TRAILER_BITS,  v)

#define STIX_OBJ_GET_SIZE(oop) ((oop)->_size)
#define STIX_OBJ_GET_CLASS(oop) ((oop)->_class)

#define STIX_OBJ_SET_SIZE(oop,v) ((oop)->_size = (v))
#define STIX_OBJ_SET_CLASS(oop,c) ((oop)->_class = (c))

/* [NOTE] this macro doesn't include the size of the trailer */
#define STIX_OBJ_BYTESOF(oop) ((STIX_OBJ_GET_SIZE(oop) + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop))

/* [NOTE] this macro doesn't check the range of the actual value.
 *        make sure that the value of each bit fields given falls within the 
 *        possible range of the defined bits */
#define STIX_OBJ_MAKE_FLAGS(t,u,e,k,m,r) ( \
	(((stix_oow_t)(t)) << (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS)) | \
	(((stix_oow_t)(u)) << (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS)) | \
	(((stix_oow_t)(e)) << (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS)) | \
	(((stix_oow_t)(k)) << (STIX_OBJ_FLAGS_MOVED_BITS + STIX_OBJ_FLAGS_TRAILER_BITS)) | \
	(((stix_oow_t)(m)) << (STIX_OBJ_FLAGS_TRAILER_BITS)) | \
	(((stix_oow_t)(r)) << 0) \
)

#define STIX_OBJ_HEADER \
	stix_oow_t _flags; \
	stix_oow_t _size; \
	stix_oop_t _class

struct stix_obj_t
{
	STIX_OBJ_HEADER;
};

struct stix_obj_oop_t
{
	STIX_OBJ_HEADER;
	stix_oop_t slot[1];
};

struct stix_obj_char_t
{
	STIX_OBJ_HEADER;
	stix_uch_t slot[1];
};

struct stix_obj_byte_t
{
	STIX_OBJ_HEADER;
	stix_byte_t slot[1];
};

struct stix_obj_word_t
{
	STIX_OBJ_HEADER;
	stix_oow_t slot[1];
};

typedef struct stix_trailer_t stix_trailer_t;
struct stix_trailer_t
{
	stix_oow_t size;
	stix_byte_t slot[1];
};

#define STIX_SET_NAMED_INSTVARS 2
typedef struct stix_set_t stix_set_t;
typedef struct stix_set_t* stix_oop_set_t;
struct stix_set_t
{
	STIX_OBJ_HEADER;
	stix_oop_t     tally;  /* SmallInteger */
	stix_oop_oop_t bucket; /* Array */
};

#define STIX_CLASS_NAMED_INSTVARS 11
typedef struct stix_class_t stix_class_t;
typedef struct stix_class_t* stix_oop_class_t;
struct stix_class_t
{
	STIX_OBJ_HEADER;

	stix_oop_t      spec;          /* SmallInteger. instance specification */
	stix_oop_t      selfspec;      /* SmallInteger. specification of the class object itself */

	stix_oop_t      superclass;    /* Another class */
	stix_oop_t      subclasses;    /* Array of subclasses */

	stix_oop_char_t name;          /* Symbol */

	/* == NEVER CHANGE THIS ORDER OF 3 ITEMS BELOW == */
	stix_oop_char_t instvars;      /* String */
	stix_oop_char_t classvars;     /* String */
	stix_oop_char_t classinstvars; /* String */
	/* == NEVER CHANGE THE ORDER OF 3 ITEMS ABOVE == */

	stix_oop_char_t pooldics;      /* String */

	/* [0] - instance methods, MethodDictionary
	 * [1] - class methods, MethodDictionary */
	stix_oop_set_t  mthdic[2];      

	/* indexed part afterwards */
	stix_oop_t      slot[1];   /* class instance variables and class variables. */
};
#define STIX_CLASS_MTHDIC_INSTANCE 0
#define STIX_CLASS_MTHDIC_CLASS    1

#define STIX_ASSOCIATION_NAMED_INSTVARS 2
typedef struct stix_association_t stix_association_t;
typedef struct stix_association_t* stix_oop_association_t;
struct stix_association_t
{
	STIX_OBJ_HEADER;
	stix_oop_t key;
	stix_oop_t value;
};

#if defined(STIX_USE_OBJECT_TRAILER)
#	define STIX_METHOD_NAMED_INSTVARS 5
#else
#	define STIX_METHOD_NAMED_INSTVARS 6
#endif
typedef struct stix_method_t stix_method_t;
typedef struct stix_method_t* stix_oop_method_t;
struct stix_method_t
{
	STIX_OBJ_HEADER;

	stix_oop_class_t owner; /* Class */

	/* primitive number */
	stix_oop_t       preamble; /* SmallInteger */

	/* number of temporaries including arguments */
	stix_oop_t       tmpr_count; /* SmallInteger */

	/* number of arguments in temporaries */
	stix_oop_t       tmpr_nargs; /* SmallInteger */

#if defined(STIX_USE_OBJECT_TRAILER)
	/* no code field is used */
#else
	stix_oop_byte_t  code; /* ByteArray */
#endif

	stix_oop_t       source; /* TODO: what should I put? */

	/* == variable indexed part == */
	stix_oop_t       slot[1]; /* it stores literals */
};

/* The preamble field is composed of a 8-bit code and a 16-bit
 * index. 
 *
 * The code can be one of the following values:
 *  0 - no special action
 *  1 - return self
 *  2 - return nil
 *  3 - return true
 *  4 - return false 
 *  5 - return index.
 *  6 - return -index.
 *  7 - return instvar[index]
 *  8 - do primitive[index]
 */
#define STIX_METHOD_MAKE_PREAMBLE(code,index)  ((((stix_ooi_t)index) << 8) | ((stix_ooi_t)code))
#define STIX_METHOD_GET_PREAMBLE_CODE(preamble) (((stix_ooi_t)preamble) & 0xFF)
#define STIX_METHOD_GET_PREAMBLE_INDEX(preamble) (((stix_ooi_t)preamble) >> 8)

#define STIX_METHOD_PREAMBLE_NONE            0
#define STIX_METHOD_PREAMBLE_RETURN_RECEIVER 1
#define STIX_METHOD_PREAMBLE_RETURN_NIL      2
#define STIX_METHOD_PREAMBLE_RETURN_TRUE     3
#define STIX_METHOD_PREAMBLE_RETURN_FALSE    4
#define STIX_METHOD_PREAMBLE_RETURN_INDEX    5
#define STIX_METHOD_PREAMBLE_RETURN_NEGINDEX 6
#define STIX_METHOD_PREAMBLE_RETURN_INSTVAR  7
#define STIX_METHOD_PREAMBLE_PRIMITIVE       8

/* the index is an 16-bit unsigned integer. */
#define STIX_OOI_IN_PREAMBLE_INDEX_RANGE(ooi) ((ooi) >= 0 && (ooi) <= 0xFFFF)

#define STIX_CONTEXT_NAMED_INSTVARS 8
typedef struct stix_context_t stix_context_t;
typedef struct stix_context_t* stix_oop_context_t;
struct stix_context_t
{
	STIX_OBJ_HEADER;

	/* it points to the active context at the moment when
	 * this context object has been activated. a new method context
	 * is activated as a result of normal message sending and a block
	 * context is activated when it is sent 'value'. it's set to
	 * nil if a block context created haven't received 'value'. */
	stix_oop_t         sender;

	/* SmallInteger, instruction pointer */
	stix_oop_t         ip;

	/* SmallInteger, stack pointer */
	stix_oop_t         sp;       /* stack pointer */

	/* SmallInteger. Number of temporaries.
	 * For a block context, it's inclusive of the temporaries
	 * defined its 'home'. */
	stix_oop_t         ntmprs;   /* SmallInteger. */

	/* CompiledMethod for a method context, 
	 * SmallInteger for a block context */
	stix_oop_t         method_or_nargs;

	/* it points to the receiver of the message for a method context.
	 * a block context created but not activated has nil in this field.
	 * if a block context is activated by 'value', it points to the
	 * block context object used as a base for shallow-copy. */
	stix_oop_t         receiver_or_source;

	/* it is set to nil for a method context.
	 * for a block context, it points to the active context at the 
	 * moment the block context was created. */
	stix_oop_t         home;

	/* when a method context is craeted, it is set to itself.
	 * no change is made when the method context is activated.
	 * when a block context is created, it is set to nil.
	 * when the block context is shallow-copied for activation,
	 * it is set to the origin of the active context at that moment */
	stix_oop_context_t origin; 

	/* variable indexed part */
	stix_oop_t         slot[1]; /* stack */
};

/**
 * The STIX_CLASSOF() macro return the class of an object including a numeric
 * object encoded into a pointer.
 */
#define STIX_CLASSOF(stix,oop) ( \
	STIX_OOP_IS_SMINT(oop)? (stix)->_small_integer: \
	STIX_OOP_IS_CHAR(oop)? (stix)->_character: STIX_OBJ_GET_CLASS(oop) \
)

/**
 * The STIX_BYTESOF() macro returns the size of the payload of
 * an object in bytes. If the pointer given encodes a numeric value, 
 * it returns the size of #stix_oow_t in bytes.
 */
#define STIX_BYTESOF(stix,oop) \
	(STIX_OOP_IS_NUMERIC(oop)? STIX_SIZEOF(stix_oow_t): STIX_OBJ_BYTESOF(oop))

/**
 * The STIX_ISTYPEOF() macro is a safe replacement for STIX_OBJ_GET_FLAGS_TYPE()
 */
#define STIX_ISTYPEOF(stix,oop,type) \
	(!STIX_OOP_IS_NUMERIC(oop) && STIX_OBJ_GET_FLAGS_TYPE(oop) == (type))

typedef struct stix_heap_t stix_heap_t;

struct stix_heap_t
{
	stix_uint8_t* base;  /* start of a heap */
	stix_uint8_t* limit; /* end of a heap */
	stix_uint8_t* ptr;   /* next allocation pointer */
};

typedef struct stix_t stix_t;

typedef void (*stix_cbimpl_t) (stix_t* stix);

typedef struct stix_cb_t stix_cb_t;
struct stix_cb_t
{
	stix_cbimpl_t gc;
	stix_cbimpl_t fini;

	/* private below */
	stix_cb_t*    prev;
	stix_cb_t*    next;
};

#if defined(STIX_INCLUDE_COMPILER)
typedef struct stix_compiler_t stix_compiler_t;
#endif

struct stix_t
{
	stix_mmgr_t*  mmgr;
	stix_errnum_t errnum;

	struct
	{
		int trait;
		stix_oow_t dfl_symtab_size;
		stix_oow_t dfl_sysdic_size;
	} option;

	stix_cb_t* cblist;

	/* ========================= */

	stix_heap_t* permheap; /* TODO: put kernel objects to here */
	stix_heap_t* curheap;
	stix_heap_t* newheap;

	/* ========================= */
	stix_oop_t _nil;  /* pointer to the nil object */
	stix_oop_t _true;
	stix_oop_t _false;

	/* == NEVER CHANGE THE ORDER OF FIELDS BELOW == */
	/* stix_ignite() assumes this order. make sure to update symnames in ignite_3() */
	stix_oop_t _apex; /* Apex */
	stix_oop_t _undefined_object; /* UndefinedObject */
	stix_oop_t _class; /* Class */
	stix_oop_t _object; /* Object */
	stix_oop_t _string; /* String */

	stix_oop_t _symbol; /* Symbol */
	stix_oop_t _array; /* Array */
	stix_oop_t _byte_array; /* ByteArray */
	stix_oop_t _symbol_set; /* SymbolSet */
	stix_oop_t _system_dictionary; /* SystemDictionary */

	stix_oop_t _namespace; /* Namespace */
	stix_oop_t _pool_dictionary; /* PoolDictionary */
	stix_oop_t _method_dictionary; /* MethodDictionary */
	stix_oop_t _method; /* CompiledMethod */
	stix_oop_t _association; /* Association */

	stix_oop_t _method_context; /* MethodContext */
	stix_oop_t _block_context; /* BlockContext */
	stix_oop_t _true_class; /* True */
	stix_oop_t _false_class; /* False */
	stix_oop_t _character; /* Character */

	stix_oop_t _small_integer; /* SmallInteger */
	/* == NEVER CHANGE THE ORDER OF FIELDS ABOVE == */

	stix_oop_set_t symtab; /* system-wide symbol table. instance of SymbolSet */
	stix_oop_set_t sysdic; /* system dictionary. instance of SystemDictionary */

	stix_oop_t* tmp_stack[256]; /* stack for temporaries */
	stix_oow_t tmp_count;

	/* == EXECUTION REGISTERS == */
	stix_oop_context_t active_context;
	stix_oop_method_t active_method;
	stix_byte_t* active_code;
	stix_ooi_t sp;
	stix_ooi_t ip;
	/* == END EXECUTION REGISTERS == */

#if defined(STIX_INCLUDE_COMPILER)
	stix_compiler_t* c;
#endif
};


#if defined(__cplusplus)
extern "C" {
#endif

STIX_EXPORT stix_t* stix_open (
	stix_mmgr_t*   mmgr,
	stix_size_t    xtnsize,
	stix_size_t    heapsize,
	stix_errnum_t* errnum
);

STIX_EXPORT void stix_close (
	stix_t* vm
);

STIX_EXPORT int stix_init (
	stix_t*      vm,
	stix_mmgr_t* mmgr,
	stix_size_t  heapsz
);

STIX_EXPORT void stix_fini (
	stix_t* vm
);


STIX_EXPORT stix_mmgr_t* stix_getmmgr (
	stix_t* stix
);

STIX_EXPORT void* stix_getxtn (
	stix_t* stix
);


STIX_EXPORT stix_errnum_t stix_geterrnum (
	stix_t* stix
);

STIX_EXPORT void stix_seterrnum (
	stix_t*       stix,
	stix_errnum_t errnum
);

/**
 * The stix_getoption() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
STIX_EXPORT int stix_getoption (
	stix_t*        stix,
	stix_option_t  id,
	void*          value
);

/**
 * The stix_setoption() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
STIX_EXPORT int stix_setoption (
	stix_t*       stix,
	stix_option_t id,
	const void*   value
);


STIX_EXPORT stix_cb_t* stix_regcb (
	stix_t*    stix,
	stix_cb_t* tmpl
);

STIX_EXPORT void stix_deregcb (
	stix_t*    stix,
	stix_cb_t* cb
);

/**
 * The stix_gc() function performs garbage collection.
 * It is not affected by #STIX_NOGC.
 */
STIX_EXPORT void stix_gc (
	stix_t* stix
);

/**
 * The stix_instantiate() function creates a new object of the class 
 * \a _class. The size of the fixed part is taken from the information
 * contained in the class defintion. The \a vlen parameter specifies 
 * the length of the variable part. The \a vptr parameter points to 
 * the memory area to copy into the variable part of the new object.
 * If \a vptr is #STIX_NULL, the variable part is initialized to 0 or
 * an equivalent value depending on the type.
 */
STIX_EXPORT stix_oop_t stix_instantiate (
	stix_t*          stix,
	stix_oop_t       _class,
	const void*      vptr,
	stix_oow_t       vlen
);

/**
 * The stix_ignite() function creates key initial objects.
 */
STIX_EXPORT int stix_ignite (
	stix_t* stix
);

/**
 * The stix_execute() function executes an activated context.
 */
STIX_EXPORT int stix_execute (
	stix_t* stix
);

/**
 * The stix_invoke() function sends a message named \a mthname to an object
 * named \a objname.
 */
STIX_EXPORT int stix_invoke (
	stix_t*           stix,
	const stix_ucs_t* objname,
	const stix_ucs_t* mthname
);

/* Temporary OOP management  */
STIX_EXPORT void stix_pushtmp (
	stix_t*     stix,
	stix_oop_t* oop_ptr
);

STIX_EXPORT void stix_poptmp (
	stix_t*     stix
);

STIX_EXPORT void stix_poptmps (
	stix_t*     stix,
	stix_oow_t  count
);


/* Memory allocation/deallocation functions using stix's MMGR */

STIX_EXPORT void* stix_allocmem (
	stix_t*     stix,
	stix_size_t size
);

STIX_EXPORT void* stix_callocmem (
	stix_t*     stix,
	stix_size_t size
);

STIX_EXPORT void* stix_reallocmem (
	stix_t*     stix,
	void*       ptr,
	stix_size_t size
);

STIX_EXPORT void stix_freemem (
	stix_t* stix,
	void*   ptr
);

#if defined(__cplusplus)
}
#endif


#endif
