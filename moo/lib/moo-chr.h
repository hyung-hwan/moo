/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _MOO_CHR_H_
#define _MOO_CHR_H_

#include <moo-cmn.h>

enum moo_ooch_prop_t
{
	MOO_OOCH_PROP_UPPER  = (1 << 0),
#define MOO_UCH_PROP_UPPER MOO_OOCH_PROP_UPPER
#define MOO_BCH_PROP_UPPER MOO_OOCH_PROP_UPPER

	MOO_OOCH_PROP_LOWER  = (1 << 1),
#define MOO_UCH_PROP_LOWER MOO_OOCH_PROP_LOWER
#define MOO_BCH_PROP_LOWER MOO_OOCH_PROP_LOWER

	MOO_OOCH_PROP_ALPHA  = (1 << 2),
#define MOO_UCH_PROP_ALPHA MOO_OOCH_PROP_ALPHA
#define MOO_BCH_PROP_ALPHA MOO_OOCH_PROP_ALPHA

	MOO_OOCH_PROP_DIGIT  = (1 << 3),
#define MOO_UCH_PROP_DIGIT MOO_OOCH_PROP_DIGIT
#define MOO_BCH_PROP_DIGIT MOO_OOCH_PROP_DIGIT

	MOO_OOCH_PROP_XDIGIT = (1 << 4),
#define MOO_UCH_PROP_XDIGIT MOO_OOCH_PROP_XDIGIT
#define MOO_BCH_PROP_XDIGIT MOO_OOCH_PROP_XDIGIT

	MOO_OOCH_PROP_ALNUM  = (1 << 5),
#define MOO_UCH_PROP_ALNUM MOO_OOCH_PROP_XDIGIT
#define MOO_BCH_PROP_ALNUM MOO_OOCH_PROP_XDIGIT

	MOO_OOCH_PROP_SPACE  = (1 << 6),
#define MOO_UCH_PROP_SPACE MOO_OOCH_PROP_SPACE
#define MOO_BCH_PROP_SPACE MOO_OOCH_PROP_SPACE

	MOO_OOCH_PROP_PRINT  = (1 << 8),
#define MOO_UCH_PROP_PRINT MOO_OOCH_PROP_PRINT
#define MOO_BCH_PROP_PRINT MOO_OOCH_PROP_PRINT

	MOO_OOCH_PROP_GRAPH  = (1 << 9),
#define MOO_UCH_PROP_GRAPH MOO_OOCH_PROP_GRAPH
#define MOO_BCH_PROP_GRAPH MOO_OOCH_PROP_GRAPH

	MOO_OOCH_PROP_CNTRL  = (1 << 10),
#define MOO_UCH_PROP_CNTRL MOO_OOCH_PROP_CNTRL
#define MOO_BCH_PROP_CNTRL MOO_OOCH_PROP_CNTRL

	MOO_OOCH_PROP_PUNCT  = (1 << 11),
#define MOO_UCH_PROP_PUNCT MOO_OOCH_PROP_PUNCT
#define MOO_BCH_PROP_PUNCT MOO_OOCH_PROP_PUNCT

	MOO_OOCH_PROP_BLANK  = (1 << 12)
#define MOO_UCH_PROP_BLANK MOO_OOCH_PROP_BLANK
#define MOO_BCH_PROP_BLANK MOO_OOCH_PROP_BLANK
};

typedef enum moo_ooch_prop_t moo_ooch_prop_t;
typedef enum moo_ooch_prop_t moo_uch_prop_t;
typedef enum moo_ooch_prop_t moo_bch_prop_t;

#if defined(__cplusplus)
extern "C" {
#endif

MOO_EXPORT int moo_is_uch_type (moo_uch_t c, moo_uch_prop_t type);
MOO_EXPORT int moo_is_uch_upper (moo_uch_t c);
MOO_EXPORT int moo_is_uch_lower (moo_uch_t c);
MOO_EXPORT int moo_is_uch_alpha (moo_uch_t c);
MOO_EXPORT int moo_is_uch_digit (moo_uch_t c);
MOO_EXPORT int moo_is_uch_xdigit (moo_uch_t c);
MOO_EXPORT int moo_is_uch_alnum (moo_uch_t c);
MOO_EXPORT int moo_is_uch_space (moo_uch_t c);
MOO_EXPORT int moo_is_uch_print (moo_uch_t c);
MOO_EXPORT int moo_is_uch_graph (moo_uch_t c);
MOO_EXPORT int moo_is_uch_cntrl (moo_uch_t c);
MOO_EXPORT int moo_is_uch_punct (moo_uch_t c);
MOO_EXPORT int moo_is_uch_blank (moo_uch_t c);
MOO_EXPORT moo_uch_t moo_to_uch_upper (moo_uch_t c);
MOO_EXPORT moo_uch_t moo_to_uch_lower (moo_uch_t c);


/* ------------------------------------------------------------------------- */

MOO_EXPORT int moo_is_bch_type (moo_bch_t c, moo_bch_prop_t type);

#if defined(__has_builtin)
#	if __has_builtin(__builtin_isupper)
#		define moo_is_bch_upper __builtin_isupper
#	endif
#	if __has_builtin(__builtin_islower)
#		define moo_is_bch_lower __builtin_islower
#	endif
#	if __has_builtin(__builtin_isalpha)
#		define moo_is_bch_alpha __builtin_isalpha
#	endif
#	if __has_builtin(__builtin_isdigit)
#		define moo_is_bch_digit __builtin_isdigit
#	endif
#	if __has_builtin(__builtin_isxdigit)
#		define moo_is_bch_xdigit __builtin_isxdigit
#	endif
#	if __has_builtin(__builtin_isalnum)
#		define moo_is_bch_alnum __builtin_isalnum
#	endif
#	if __has_builtin(__builtin_isspace)
#		define moo_is_bch_space __builtin_isspace
#	endif
#	if __has_builtin(__builtin_isprint)
#		define moo_is_bch_print __builtin_isprint
#	endif
#	if __has_builtin(__builtin_isgraph)
#		define moo_is_bch_graph __builtin_isgraph
#	endif
#	if __has_builtin(__builtin_iscntrl)
#		define moo_is_bch_cntrl __builtin_iscntrl
#	endif
#	if __has_builtin(__builtin_ispunct)
#		define moo_is_bch_punct __builtin_ispunct
#	endif
#	if __has_builtin(__builtin_isblank)
#		define moo_is_bch_blank __builtin_isblank
#	endif
#	if __has_builtin(__builtin_toupper)
#		define moo_to_bch_upper __builtin_toupper
#	endif
#	if __has_builtin(__builtin_tolower)
#		define moo_to_bch_lower __builtin_tolower
#	endif
#elif (__GNUC__ >= 4) 
#	define moo_is_bch_upper __builtin_isupper
#	define moo_is_bch_lower __builtin_islower
#	define moo_is_bch_alpha __builtin_isalpha
#	define moo_is_bch_digit __builtin_isdigit
#	define moo_is_bch_xdigit __builtin_isxdigit
#	define moo_is_bch_alnum __builtin_isalnum
#	define moo_is_bch_space __builtin_isspace
#	define moo_is_bch_print __builtin_isprint
#	define moo_is_bch_graph __builtin_isgraph
#	define moo_is_bch_cntrl __builtin_iscntrl
#	define moo_is_bch_punct __builtin_ispunct
#	define moo_is_bch_blank __builtin_isblank
#	define moo_to_bch_upper __builtin_toupper
#	define moo_to_bch_lower __builtin_tolower
#endif

/* the bch class functions support no locale.
 * these implemenent latin-1 only */

#if !defined(moo_is_bch_upper) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_upper (moo_bch_t c) { return (moo_bchu_t)c - 'A' < 26; }
#elif !defined(moo_is_bch_upper)
#	define moo_is_bch_upper(c) ((moo_bchu_t)(c) - 'A' < 26)
#endif

#if !defined(moo_is_bch_lower) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_lower (moo_bch_t c) { return (moo_bchu_t)c - 'a' < 26; }
#elif !defined(moo_is_bch_lower)
#	define moo_is_bch_lower(c) ((moo_bchu_t)(c) - 'a' < 26)
#endif

#if !defined(moo_is_bch_alpha) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_alpha (moo_bch_t c) { return ((moo_bchu_t)c | 32) - 'a' < 26; }
#elif !defined(moo_is_bch_alpha)
#	define moo_is_bch_alpha(c) (((moo_bchu_t)(c) | 32) - 'a' < 26)
#endif

#if !defined(moo_is_bch_digit) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_digit (moo_bch_t c) { return (moo_bchu_t)c - '0' < 10; }
#elif !defined(moo_is_bch_digit)
#	define moo_is_bch_digit(c) ((moo_bchu_t)(c) - '0' < 10)
#endif

#if !defined(moo_is_bch_xdigit) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_xdigit (moo_bch_t c) { return moo_is_bch_digit(c) || ((moo_bchu_t)c | 32) - 'a' < 6; }
#elif !defined(moo_is_bch_xdigit)
#	define moo_is_bch_xdigit(c) (moo_is_bch_digit(c) || ((moo_bchu_t)(c) | 32) - 'a' < 6)
#endif

#if !defined(moo_is_bch_alnum) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_alnum (moo_bch_t c) { return moo_is_bch_alpha(c) || moo_is_bch_digit(c); }
#elif !defined(moo_is_bch_alnum)
#	define moo_is_bch_alnum(c) (moo_is_bch_alpha(c) || moo_is_bch_digit(c))
#endif

#if !defined(moo_is_bch_space) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_space (moo_bch_t c) { return c == ' ' || (moo_bchu_t)c - '\t' < 5; }
#elif !defined(moo_is_bch_space)
#	define moo_is_bch_space(c) ((c) == ' ' || (moo_bchu_t)(c) - '\t' < 5)
#endif

#if !defined(moo_is_bch_print) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_print (moo_bch_t c) { return (moo_bchu_t)c - ' ' < 95; }
#elif !defined(moo_is_bch_print)
#	define moo_is_bch_print(c) ((moo_bchu_t)(c) - ' ' < 95)
#endif

#if !defined(moo_is_bch_graph) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_graph (moo_bch_t c) { return (moo_bchu_t)c - '!' < 94; }
#elif !defined(moo_is_bch_graph)
#	define moo_is_bch_graph(c) ((moo_bchu_t)(c) - '!' < 94)
#endif

#if !defined(moo_is_bch_cntrl) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_cntrl (moo_bch_t c) { return (moo_bchu_t)c < ' ' || (moo_bchu_t)c == 127; }
#elif !defined(moo_is_bch_cntrl)
#	define moo_is_bch_cntrl(c) ((moo_bchu_t)(c) < ' ' || (moo_bchu_t)(c) == 127)
#endif

#if !defined(moo_is_bch_punct) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_punct (moo_bch_t c) { return moo_is_bch_graph(c) && !moo_is_bch_alnum(c); }
#elif !defined(moo_is_bch_punct)
#	define moo_is_bch_punct(c) (moo_is_bch_graph(c) && !moo_is_bch_alnum(c))
#endif

#if !defined(moo_is_bch_blank) && defined(MOO_HAVE_INLINE)
static MOO_INLINE int moo_is_bch_blank (moo_bch_t c) { return c == ' ' || c == '\t'; }
#elif !defined(moo_is_bch_blank)
#	define moo_is_bch_blank(c) ((c) == ' ' || (c) == '\t')
#endif

#if !defined(moo_to_bch_upper)
MOO_EXPORT moo_bch_t moo_to_bch_upper (moo_bch_t c);
#endif
#if !defined(moo_to_bch_lower)
MOO_EXPORT moo_bch_t moo_to_bch_lower (moo_bch_t c);
#endif

#if defined(MOO_OOCH_IS_UCH)
#	define moo_is_ooch_type moo_is_uch_type
#	define moo_is_ooch_upper moo_is_uch_upper
#	define moo_is_ooch_lower moo_is_uch_lower
#	define moo_is_ooch_alpha moo_is_uch_alpha
#	define moo_is_ooch_digit moo_is_uch_digit
#	define moo_is_ooch_xdigit moo_is_uch_xdigit
#	define moo_is_ooch_alnum moo_is_uch_alnum
#	define moo_is_ooch_space moo_is_uch_space
#	define moo_is_ooch_print moo_is_uch_print
#	define moo_is_ooch_graph moo_is_uch_graph
#	define moo_is_ooch_cntrl moo_is_uch_cntrl
#	define moo_is_ooch_punct moo_is_uch_punct
#	define moo_is_ooch_blank moo_is_uch_blank
#	define moo_to_ooch_upper moo_to_uch_upper
#	define moo_to_ooch_lower moo_to_uch_lower
#else
#	define moo_is_ooch_type moo_is_bch_type
#	define moo_is_ooch_upper moo_is_bch_upper
#	define moo_is_ooch_lower moo_is_bch_lower
#	define moo_is_ooch_alpha moo_is_bch_alpha
#	define moo_is_ooch_digit moo_is_bch_digit
#	define moo_is_ooch_xdigit moo_is_bch_xdigit
#	define moo_is_ooch_alnum moo_is_bch_alnum
#	define moo_is_ooch_space moo_is_bch_space
#	define moo_is_ooch_print moo_is_bch_print
#	define moo_is_ooch_graph moo_is_bch_graph
#	define moo_is_ooch_cntrl moo_is_bch_cntrl
#	define moo_is_ooch_punct moo_is_bch_punct
#	define moo_is_ooch_blank moo_is_bch_blank
#	define moo_to_ooch_upper moo_to_bch_upper
#	define moo_to_ooch_lower moo_to_bch_lower
#endif

#if defined(__cplusplus)
}
#endif

#endif
