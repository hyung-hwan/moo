/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _MOO_FMT_H_
#define _MOO_FMT_H_

#include <moo-cmn.h>

/** \file
 * This file defines various formatting functions.
 */

/** 
 * The moo_fmt_intmax_flag_t type defines enumerators to change the
 * behavior of moo_fmt_intmax() and moo_fmt_uintmax().
 */
enum moo_fmt_intmax_flag_t
{
	/* Use lower 6 bits to represent base between 2 and 36 inclusive.
	 * Upper bits are used for these flag options */

	/** Don't truncate if the buffer is not large enough */
	MOO_FMT_INTMAX_NOTRUNC = (0x40 << 0),
#define MOO_FMT_INTMAX_NOTRUNC             MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_UINTMAX_NOTRUNC            MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_INTMAX_TO_BCSTR_NOTRUNC    MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_UINTMAX_TO_BCSTR_NOTRUNC   MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_INTMAX_TO_UCSTR_NOTRUNC    MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_UINTMAX_TO_UCSTR_NOTRUNC   MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_INTMAX_TO_OOCSTR_NOTRUNC   MOO_FMT_INTMAX_NOTRUNC
#define MOO_FMT_UINTMAX_TO_OOCSTR_NOTRUNC  MOO_FMT_INTMAX_NOTRUNC

	/** Don't append a terminating null */
	MOO_FMT_INTMAX_NONULL = (0x40 << 1),
#define MOO_FMT_INTMAX_NONULL             MOO_FMT_INTMAX_NONULL
#define MOO_FMT_UINTMAX_NONULL            MOO_FMT_INTMAX_NONULL
#define MOO_FMT_INTMAX_TO_BCSTR_NONULL    MOO_FMT_INTMAX_NONULL
#define MOO_FMT_UINTMAX_TO_BCSTR_NONULL   MOO_FMT_INTMAX_NONULL
#define MOO_FMT_INTMAX_TO_UCSTR_NONULL    MOO_FMT_INTMAX_NONULL
#define MOO_FMT_UINTMAX_TO_UCSTR_NONULL   MOO_FMT_INTMAX_NONULL
#define MOO_FMT_INTMAX_TO_OOCSTR_NONULL   MOO_FMT_INTMAX_NONULL
#define MOO_FMT_UINTMAX_TO_OOCSTR_NONULL  MOO_FMT_INTMAX_NONULL

	/** Produce no digit for a value of zero  */
	MOO_FMT_INTMAX_NOZERO = (0x40 << 2),
#define MOO_FMT_INTMAX_NOZERO             MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_UINTMAX_NOZERO            MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_INTMAX_TO_BCSTR_NOZERO    MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_UINTMAX_TO_BCSTR_NOZERO   MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_INTMAX_TO_UCSTR_NOZERO    MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_UINTMAX_TO_UCSTR_NOZERO   MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_INTMAX_TO_OOCSTR_NOZERO   MOO_FMT_INTMAX_NOZERO
#define MOO_FMT_UINTMAX_TO_OOCSTR_NOZERO  MOO_FMT_INTMAX_NOZERO

	/** Produce a leading zero for a non-zero value */
	MOO_FMT_INTMAX_ZEROLEAD = (0x40 << 3),
#define MOO_FMT_INTMAX_ZEROLEAD             MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_UINTMAX_ZEROLEAD            MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_INTMAX_TO_BCSTR_ZEROLEAD    MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_UINTMAX_TO_BCSTR_ZEROLEAD   MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_INTMAX_TO_UCSTR_ZEROLEAD    MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_UINTMAX_TO_UCSTR_ZEROLEAD   MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_INTMAX_TO_OOCSTR_ZEROLEAD   MOO_FMT_INTMAX_ZEROLEAD
#define MOO_FMT_UINTMAX_TO_OOCSTR_ZEROLEAD  MOO_FMT_INTMAX_ZEROLEAD

	/** Use uppercase letters for alphabetic digits */
	MOO_FMT_INTMAX_UPPERCASE = (0x40 << 4),
#define MOO_FMT_INTMAX_UPPERCASE             MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_UINTMAX_UPPERCASE            MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_INTMAX_TO_BCSTR_UPPERCASE    MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_UINTMAX_TO_BCSTR_UPPERCASE   MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_INTMAX_TO_UCSTR_UPPERCASE    MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_UINTMAX_TO_UCSTR_UPPERCASE   MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_INTMAX_TO_OOCSTR_UPPERCASE   MOO_FMT_INTMAX_UPPERCASE
#define MOO_FMT_UINTMAX_TO_OOCSTR_UPPERCASE  MOO_FMT_INTMAX_UPPERCASE

	/** Insert a plus sign for a positive integer including 0 */
	MOO_FMT_INTMAX_PLUSSIGN = (0x40 << 5),
#define MOO_FMT_INTMAX_PLUSSIGN             MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_UINTMAX_PLUSSIGN            MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_INTMAX_TO_BCSTR_PLUSSIGN    MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_UINTMAX_TO_BCSTR_PLUSSIGN   MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_INTMAX_TO_UCSTR_PLUSSIGN    MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_UINTMAX_TO_UCSTR_PLUSSIGN   MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_INTMAX_TO_OOCSTR_PLUSSIGN   MOO_FMT_INTMAX_PLUSSIGN
#define MOO_FMT_UINTMAX_TO_OOCSTR_PLUSSIGN  MOO_FMT_INTMAX_PLUSSIGN

	/** Insert a space for a positive integer including 0 */
	MOO_FMT_INTMAX_EMPTYSIGN = (0x40 << 6),
#define MOO_FMT_INTMAX_EMPTYSIGN             MOO_FMT_INTMAX_EMPTYSIGN
#define MOO_FMT_UINTMAX_EMPTYSIGN            MOO_FMT_INTMAX_EMPTYSIGN
#define MOO_FMT_INTMAX_TO_BCSTR_EMPTYSIGN    MOO_FMT_INTMAX_EMPTYSIGN
#define MOO_FMT_UINTMAX_TO_BCSTR_EMPTYSIGN   MOO_FMT_INTMAX_EMPTYSIGN
#define MOO_FMT_INTMAX_TO_UCSTR_EMPTYSIGN    MOO_FMT_INTMAX_EMPTYSIGN
#define MOO_FMT_UINTMAX_TO_UCSTR_EMPTYSIGN   MOO_FMT_INTMAX_EMPTYSIGN

	/** Fill the right part of the string */
	MOO_FMT_INTMAX_FILLRIGHT = (0x40 << 7),
#define MOO_FMT_INTMAX_FILLRIGHT             MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_UINTMAX_FILLRIGHT            MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_INTMAX_TO_BCSTR_FILLRIGHT    MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_UINTMAX_TO_BCSTR_FILLRIGHT   MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_INTMAX_TO_UCSTR_FILLRIGHT    MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_UINTMAX_TO_UCSTR_FILLRIGHT   MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_INTMAX_TO_OOCSTR_FILLRIGHT   MOO_FMT_INTMAX_FILLRIGHT
#define MOO_FMT_UINTMAX_TO_OOCSTR_FILLRIGHT  MOO_FMT_INTMAX_FILLRIGHT

	/** Fill between the sign chacter and the digit part */
	MOO_FMT_INTMAX_FILLCENTER = (0x40 << 8)
#define MOO_FMT_INTMAX_FILLCENTER             MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_UINTMAX_FILLCENTER            MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_INTMAX_TO_BCSTR_FILLCENTER    MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_UINTMAX_TO_BCSTR_FILLCENTER   MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_INTMAX_TO_UCSTR_FILLCENTER    MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_UINTMAX_TO_UCSTR_FILLCENTER   MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_INTMAX_TO_OOCSTR_FILLCENTER   MOO_FMT_INTMAX_FILLCENTER
#define MOO_FMT_UINTMAX_TO_OOCSTR_FILLCENTER  MOO_FMT_INTMAX_FILLCENTER
};

/* =========================================================================
 * FORMATTED OUTPUT
 * ========================================================================= */
typedef struct moo_fmtout_t moo_fmtout_t;

typedef int (*moo_fmtout_putbchars_t) (
	moo_fmtout_t*     fmtout,
	const moo_bch_t*  ptr,
	moo_oow_t         len
);

typedef int (*moo_fmtout_putuchars_t) (
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
	moo_oow_t              count; /* out */

	moo_fmtout_putbchars_t putbchars; /* in */
	moo_fmtout_putuchars_t putuchars; /* in */
	moo_fmtout_putobj_t    putobj; /* in - %O is not handled if it's not set. */
	moo_bitmask_t          mask;   /* in */
	void*                  ctx;    /* in */

	moo_fmtout_fmt_type_t  fmt_type;
	const void*            fmt_str;
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The moo_fmt_intmax_to_bcstr() function formats an integer \a value to a 
 * multibyte string according to the given base and writes it to a buffer 
 * pointed to by \a buf. It writes to the buffer at most \a size characters 
 * including the terminating null. The base must be between 2 and 36 inclusive 
 * and can be ORed with zero or more #moo_fmt_intmax_to_bcstr_flag_t enumerators. 
 * This ORed value is passed to the function via the \a base_and_flags 
 * parameter. If the formatted string is shorter than \a bufsize, the redundant
 * slots are filled with the fill character \a fillchar if it is not a null 
 * character. The filling behavior is determined by the flags shown below:
 *
 * - If #MOO_FMT_INTMAX_TO_BCSTR_FILLRIGHT is set in \a base_and_flags, slots 
 *   after the formatting string are filled.
 * - If #MOO_FMT_INTMAX_TO_BCSTR_FILLCENTER is set in \a base_and_flags, slots 
 *   before the formatting string are filled. However, if it contains the
 *   sign character, the slots between the sign character and the digit part
 *   are filled.  
 * - If neither #MOO_FMT_INTMAX_TO_BCSTR_FILLRIGHT nor #MOO_FMT_INTMAX_TO_BCSTR_FILLCENTER
 *   , slots before the formatting string are filled.
 *
 * The \a precision parameter specified the minimum number of digits to
 * produce from the \a value. If \a value produces fewer digits than
 * \a precision, the actual digits are padded with '0' to meet the precision
 * requirement. You can pass a negative number if you don't wish to specify
 * precision.
 *
 * The terminating null is not added if #MOO_FMT_INTMAX_TO_BCSTR_NONULL is set;
 * The #MOO_FMT_INTMAX_TO_BCSTR_UPPERCASE flag indicates that the function should
 * use the uppercase letter for a alphabetic digit; 
 * You can set #MOO_FMT_INTMAX_TO_BCSTR_NOTRUNC if you require lossless formatting.
 * The #MOO_FMT_INTMAX_TO_BCSTR_PLUSSIGN flag and #MOO_FMT_INTMAX_TO_BCSTR_EMPTYSIGN 
 * ensures that the plus sign and a space is added for a positive integer 
 * including 0 respectively.
 * The #MOO_FMT_INTMAX_TO_BCSTR_ZEROLEAD flag ensures that the numeric string
 * begins with '0' before applying the prefix.
 * You can set the #MOO_FMT_INTMAX_TO_BCSTR_NOZERO flag if you want the value of
 * 0 to produce nothing. If both #MOO_FMT_INTMAX_TO_BCSTR_NOZERO and 
 * #MOO_FMT_INTMAX_TO_BCSTR_ZEROLEAD are specified, '0' is still produced.
 * 
 * If \a prefix is not #MOO_NULL, it is inserted before the digits.
 * 
 * \return
 *  - -1 if the base is not between 2 and 36 inclusive. 
 *  - negated number of characters required for lossless formatting 
 *   - if \a bufsize is 0.
 *   - if #MOO_FMT_INTMAX_TO_BCSTR_NOTRUNC is set and \a bufsize is less than
 *     the minimum required for lossless formatting.
 *  - number of characters written to the buffer excluding a terminating 
 *    null in all other cases.
 */
MOO_EXPORT int moo_fmt_intmax_to_bcstr (
	moo_bch_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	moo_intmax_t       value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	moo_bch_t        fillchar,        /**< fill character */
	const moo_bch_t* prefix           /**< prefix */
);

/**
 * The moo_fmt_intmax_to_ucstr() function formats an integer \a value to a 
 * wide-character string according to the given base and writes it to a buffer 
 * pointed to by \a buf. It writes to the buffer at most \a size characters 
 * including the terminating null. The base must be between 2 and 36 inclusive 
 * and can be ORed with zero or more #moo_fmt_intmax_to_ucstr_flag_t enumerators. 
 * This ORed value is passed to the function via the \a base_and_flags 
 * parameter. If the formatted string is shorter than \a bufsize, the redundant
 * slots are filled with the fill character \a fillchar if it is not a null 
 * character. The filling behavior is determined by the flags shown below:
 *
 * - If #MOO_FMT_INTMAX_TO_UCSTR_FILLRIGHT is set in \a base_and_flags, slots 
 *   after the formatting string are filled.
 * - If #MOO_FMT_INTMAX_TO_UCSTR_FILLCENTER is set in \a base_and_flags, slots 
 *   before the formatting string are filled. However, if it contains the
 *   sign character, the slots between the sign character and the digit part
 *   are filled.  
 * - If neither #MOO_FMT_INTMAX_TO_UCSTR_FILLRIGHT nor #MOO_FMT_INTMAX_TO_UCSTR_FILLCENTER
 *   , slots before the formatting string are filled.
 * 
 * The \a precision parameter specified the minimum number of digits to
 * produce from the \ value. If \a value produces fewer digits than
 * \a precision, the actual digits are padded with '0' to meet the precision
 * requirement. You can pass a negative number if don't wish to specify
 * precision.
 *
 * The terminating null is not added if #MOO_FMT_INTMAX_TO_UCSTR_NONULL is set;
 * The #MOO_FMT_INTMAX_TO_UCSTR_UPPERCASE flag indicates that the function should
 * use the uppercase letter for a alphabetic digit; 
 * You can set #MOO_FMT_INTMAX_TO_UCSTR_NOTRUNC if you require lossless formatting.
 * The #MOO_FMT_INTMAX_TO_UCSTR_PLUSSIGN flag and #MOO_FMT_INTMAX_TO_UCSTR_EMPTYSIGN 
 * ensures that the plus sign and a space is added for a positive integer 
 * including 0 respectively.
 * The #MOO_FMT_INTMAX_TO_UCSTR_ZEROLEAD flag ensures that the numeric string
 * begins with 0 before applying the prefix.
 * You can set the #MOO_FMT_INTMAX_TO_UCSTR_NOZERO flag if you want the value of
 * 0 to produce nothing. If both #MOO_FMT_INTMAX_TO_UCSTR_NOZERO and 
 * #MOO_FMT_INTMAX_TO_UCSTR_ZEROLEAD are specified, '0' is still produced.
 *
 * If \a prefix is not #MOO_NULL, it is inserted before the digits.
 * 
 * \return
 *  - -1 if the base is not between 2 and 36 inclusive. 
 *  - negated number of characters required for lossless formatting 
 *   - if \a bufsize is 0.
 *   - if #MOO_FMT_INTMAX_TO_UCSTR_NOTRUNC is set and \a bufsize is less than
 *     the minimum required for lossless formatting.
 *  - number of characters written to the buffer excluding a terminating 
 *    null in all other cases.
 */
MOO_EXPORT int moo_fmt_intmax_to_ucstr (
	moo_uch_t*       buf,             /**< buffer pointer */
	int                bufsize,         /**< buffer size */
	moo_intmax_t       value,           /**< integer to format */
	int                base_and_flags,  /**< base ORed with flags */
	int                precision,       /**< precision */
	moo_uch_t        fillchar,        /**< fill character */
	const moo_uch_t* prefix           /**< prefix */
);

/**
 * The moo_fmt_uintmax_to_bcstr() function formats an unsigned integer \a value 
 * to a multibyte string buffer. It behaves the same as moo_fmt_uintmax_to_bcstr() 
 * except that it handles an unsigned integer.
 */
MOO_EXPORT int moo_fmt_uintmax_to_bcstr (
	moo_bch_t*       buf,             /**< buffer pointer */
	int              bufsize,         /**< buffer size */
	moo_uintmax_t    value,           /**< integer to format */
	int              base_and_flags,  /**< base ORed with flags */
	int              precision,       /**< precision */
	moo_bch_t        fillchar,        /**< fill character */
	const moo_bch_t* prefix           /**< prefix */
);

MOO_EXPORT int moo_fmt_uintmax_to_ucstr (
	moo_uch_t*       buf,             /**< buffer pointer */
	int              bufsize,         /**< buffer size */
	moo_uintmax_t    value,           /**< integer to format */
	int              base_and_flags,  /**< base ORed with flags */
	int              precision,       /**< precision */
	moo_uch_t        fillchar,        /**< fill character */
	const moo_uch_t* prefix           /**< prefix */
);

#if defined(MOO_OOCH_IS_BCH)
#	define moo_fmt_intmax_to_oocstr moo_fmt_intmax_to_bcstr
#	define moo_fmt_uintmax_to_oocstr moo_fmt_uintmax_to_bcstr
#else
#	define moo_fmt_intmax_to_oocstr moo_fmt_intmax_to_ucstr
#	define moo_fmt_uintmax_to_oocstr moo_fmt_uintmax_to_ucstr
#endif


/* TODO: moo_fmt_fltmax_to_bcstr()... moo_fmt_fltmax_to_ucstr() */


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

#if defined(__cplusplus)
}
#endif

#endif
