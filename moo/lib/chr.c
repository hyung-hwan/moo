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

#include <moo-chr.h>

/* ---------------------------------------------------------- */
#include "uch-prop.h"
#include "uch-case.h"
/* ---------------------------------------------------------- */

#define UCH_PROP_MAP_INDEX(c) ((c) / MOO_COUNTOF(uch_prop_page_0000))
#define UCH_PROP_PAGE_INDEX(c) ((c) % MOO_COUNTOF(uch_prop_page_0000))

#define UCH_CASE_MAP_INDEX(c) ((c) / MOO_COUNTOF(uch_case_page_0000))
#define UCH_CASE_PAGE_INDEX(c) ((c) % MOO_COUNTOF(uch_case_page_0000))

#define UCH_IS_TYPE(c,type) \
	((c) >= 0 && (c) <= UCH_PROP_MAX && \
	 (uch_prop_map[UCH_PROP_MAP_INDEX(c)][UCH_PROP_PAGE_INDEX(c)] & (type)) != 0)

int moo_is_uch_type (moo_uch_t c, moo_uch_prop_t type)
{
	return UCH_IS_TYPE((moo_uchu_t)c, type);
}

int moo_is_uch_upper (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_UPPER);
}

int moo_is_uch_lower (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_LOWER);
}

int moo_is_uch_alpha (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_ALPHA);
}

int moo_is_uch_digit (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_DIGIT);
}

int moo_is_uch_xdigit (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_XDIGIT);
}

int moo_is_uch_alnum (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_ALNUM);
}

int moo_is_uch_space (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_SPACE);
}

int moo_is_uch_print (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_PRINT);
}

int moo_is_uch_graph (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_GRAPH);
}

int moo_is_uch_cntrl (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_CNTRL);
}

int moo_is_uch_punct (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_PUNCT);
}

int moo_is_uch_blank (moo_uch_t c)
{
	return UCH_IS_TYPE((moo_uchu_t)c, MOO_UCH_PROP_BLANK);
}

moo_uch_t moo_to_uch_upper (moo_uch_t c)
{
	moo_uchu_t uc = (moo_uchu_t)c;
	if (uc >= 0 && uc <= UCH_CASE_MAX) 
	{
	 	uch_case_page_t* page;
		page = uch_case_map[UCH_CASE_MAP_INDEX(uc)];
		return uc - page[UCH_CASE_PAGE_INDEX(uc)].upper;
	}
	return c;
}

moo_uch_t moo_to_uch_lower (moo_uch_t c)
{
	moo_uchu_t uc = (moo_uchu_t)c;
	if (uc >= 0 && uc <= UCH_CASE_MAX) 
	{
	 	uch_case_page_t* page;
		page = uch_case_map[UCH_CASE_MAP_INDEX(uc)];
		return uc + page[UCH_CASE_PAGE_INDEX(uc)].lower;
	}
	return c;
}

/* ---------------------------------------------------------- */

int moo_is_bch_type (moo_bch_t c, moo_bch_prop_t type)
{
	switch (type)
	{
		case MOO_OOCH_PROP_UPPER:
			return moo_is_bch_upper(c);
		case MOO_OOCH_PROP_LOWER:
			return moo_is_bch_lower(c);
		case MOO_OOCH_PROP_ALPHA:
			return moo_is_bch_alpha(c);
		case MOO_OOCH_PROP_DIGIT:
			return moo_is_bch_digit(c);
		case MOO_OOCH_PROP_XDIGIT:
			return moo_is_bch_xdigit(c);
		case MOO_OOCH_PROP_ALNUM:
			return moo_is_bch_alnum(c);
		case MOO_OOCH_PROP_SPACE:
			return moo_is_bch_space(c);
		case MOO_OOCH_PROP_PRINT:
			return moo_is_bch_print(c);
		case MOO_OOCH_PROP_GRAPH:
			return moo_is_bch_graph(c);
		case MOO_OOCH_PROP_CNTRL:
			return moo_is_bch_cntrl(c);
		case MOO_OOCH_PROP_PUNCT:
			return moo_is_bch_punct(c);
		case MOO_OOCH_PROP_BLANK:
			return moo_is_bch_blank(c);
	}

	/* must not reach here */
	return 0;
}

#if !defined(moo_to_bch_upper)
moo_bch_t moo_to_bch_upper (moo_bch_t c)
{
	if(moo_is_bch_lower(c)) return c & 95;
	return c;
}
#endif

#if !defined(moo_to_bch_lower)
moo_bch_t moo_to_bch_lower (moo_bch_t c)
{
	if(moo_is_bch_upper(c)) return c | 32;
	return c;
}
#endif

