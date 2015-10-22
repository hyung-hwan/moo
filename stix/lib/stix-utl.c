
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

#include "stix-utl.h"

int stix_equalchars (const stix_uch_t* str1, const stix_uch_t* str2, stix_size_t len)
{
	stix_size_t i;

	for (i = 0; i < len; i++)
	{
		if (str1[i] != str2[i]) return 0;
	}

	return 1;
}

int stix_compucstr (const stix_uch_t* str1, const stix_uch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++, str2++;
	}

	return (*str1 > *str2)? 1: -1;
}

int stix_compbcstr (const stix_bch_t* str1, const stix_bch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++, str2++;
	}

	return (*str1 > *str2)? 1: -1;
}

int stix_compucbcstr (const stix_uch_t* str1, const stix_bch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++, str2++;
	}

	return (*str1 > *str2)? 1: -1;
}

int stix_compucxbcstr (const stix_uch_t* str1, stix_size_t len, const stix_bch_t* str2)
{
	const stix_uch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0' && *str1 == *str2) str1++, str2++;
	if (str1 == end && *str2 == '\0') return 0;
	if (*str1 == *str2) return (str1 < end)? 1: -1;
	return (*str1 > *str2)? 1: -1;
}

void stix_copyuchars (stix_uch_t* dst, const stix_uch_t* src, stix_size_t len)
{
	stix_size_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

void stix_copybchtouchars (stix_uch_t* dst, const stix_bch_t* src, stix_size_t len)
{
	stix_size_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

stix_size_t stix_copyucstr (stix_uch_t* dst, stix_size_t len, const stix_uch_t* src)
{
	stix_uch_t* p, * p2;

	p = dst; p2 = dst + len - 1;

	while (p < p2)
	{
		 if (*src == '\0') break;
		 *p++ = *src++;
	}

	if (len > 0) *p = '\0';
	return p - dst;
}

stix_size_t stix_copybcstr (stix_bch_t* dst, stix_size_t len, const stix_bch_t* src)
{
	stix_bch_t* p, * p2;

	p = dst; p2 = dst + len - 1;

	while (p < p2)
	{
		 if (*src == '\0') break;
		 *p++ = *src++;
	}

	if (len > 0) *p = '\0';
	return p - dst;
}

stix_uch_t* stix_findchar (const stix_uch_t* ptr, stix_size_t len, stix_uch_t c)
{
	const stix_uch_t* end;

	end = ptr + len;
	while (ptr < end)
	{
		if (*ptr == c) return (stix_uch_t*)ptr;
		ptr++;
	}

	return STIX_NULL;
}
