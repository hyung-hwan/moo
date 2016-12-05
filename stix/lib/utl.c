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

#include "stix-prv.h"

#define STIX_BCLEN_MAX 6

/* some naming conventions
 *  bchars, uchars -> pointer and length
 *  bcstr, ucstr -> null-terminated string pointer
 *  bctouchars -> bchars to uchars
 */

stix_oow_t stix_hashbytes (const stix_oob_t* ptr, stix_oow_t len)
{
	stix_oow_t h = 0;
	const stix_uint8_t* bp, * be;

	bp = ptr; be = bp + len;
	while (bp < be) h = h * 31 + *bp++;

	return h;
}

stix_oow_t stix_hashuchars (const stix_uch_t* ptr, stix_oow_t len)
{
	return stix_hashbytes ((const stix_oob_t *)ptr, len * STIX_SIZEOF(*ptr));
}

int stix_equaluchars (const stix_uch_t* str1, const stix_uch_t* str2, stix_oow_t len)
{
	stix_oow_t i;

	for (i = 0; i < len; i++)
	{
		if (str1[i] != str2[i]) return 0;
	}

	return 1;
}

int stix_equalbchars (const stix_bch_t* str1, const stix_bch_t* str2, stix_oow_t len)
{
	stix_oow_t i;

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

int stix_compucharsbcstr (const stix_uch_t* str1, stix_oow_t len, const stix_bch_t* str2)
{
	const stix_uch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0' && *str1 == *str2) str1++, str2++;
	if (str1 == end && *str2 == '\0') return 0;
	if (*str1 == *str2) return (str1 < end)? 1: -1;
	return (*str1 > *str2)? 1: -1;
}

void stix_copyuchars (stix_uch_t* dst, const stix_uch_t* src, stix_oow_t len)
{
	stix_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

void stix_copybchars (stix_bch_t* dst, const stix_bch_t* src, stix_oow_t len)
{
	stix_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

void stix_copybctouchars (stix_uch_t* dst, const stix_bch_t* src, stix_oow_t len)
{
	stix_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

stix_oow_t stix_copyucstr (stix_uch_t* dst, stix_oow_t len, const stix_uch_t* src)
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

stix_oow_t stix_copybcstr (stix_bch_t* dst, stix_oow_t len, const stix_bch_t* src)
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

stix_oow_t stix_countucstr (const stix_uch_t* str)
{
	const stix_uch_t* ptr = str;
	while (*ptr != '\0') ptr++;
	return ptr - str;
}

stix_oow_t stix_countbcstr (const stix_bch_t* str)
{
	const stix_bch_t* ptr = str;
	while (*ptr != '\0') ptr++;
	return ptr - str;
}

stix_uch_t* stix_finduchar (const stix_uch_t* ptr, stix_oow_t len, stix_uch_t c)
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

stix_bch_t* stix_findbchar (const stix_bch_t* ptr, stix_oow_t len, stix_bch_t c)
{
	const stix_bch_t* end;

	end = ptr + len;
	while (ptr < end)
	{
		if (*ptr == c) return (stix_bch_t*)ptr;
		ptr++;
	}

	return STIX_NULL;
}

stix_uch_t* stix_rfinduchar (const stix_uch_t* ptr, stix_oow_t len, stix_uch_t c)
{
	const stix_uch_t* cur;

	cur = ptr + len;
	while (cur > ptr)
	{
		--cur;
		if (*cur == c) return (stix_uch_t*)cur;
	}

	return STIX_NULL;
}

stix_bch_t* stix_rfindbchar (const stix_bch_t* ptr, stix_oow_t len, stix_bch_t c)
{
	const stix_bch_t* cur;

	cur = ptr + len;
	while (cur > ptr)
	{
		--cur;
		if (*cur == c) return (stix_bch_t*)cur;
	}

	return STIX_NULL;
}

/* ----------------------------------------------------------------------- */

int stix_concatoocstrtosbuf (stix_t* stix, const stix_ooch_t* str, int id)
{
	stix_sbuf_t* p;
	stix_oow_t len;

	p = &stix->sbuf[id];
	len = stix_countoocstr (str);

	if (len > p->capa - p->len)
	{
		stix_oow_t newcapa;
		stix_ooch_t* tmp;

		newcapa = STIX_ALIGN(p->len + len, 512); /* TODO: adjust this capacity */

		/* +1 to handle line ending injection more easily */
		tmp = stix_reallocmem (stix, p->ptr, (newcapa + 1) * STIX_SIZEOF(*tmp)); 
		if (!tmp) return -1;

		p->ptr = tmp;
		p->capa = newcapa;
	}

	stix_copyoochars (&p->ptr[p->len], str, len);
	p->len += len;
	p->ptr[p->len] = '\0';

	return 0;
}

int stix_copyoocstrtosbuf (stix_t* stix, const stix_ooch_t* str, int id)
{
	stix->sbuf[id].len = 0;;
	return stix_concatoocstrtosbuf (stix, str, id);
}



/* ----------------------------------------------------------------------- */

static STIX_INLINE int bcsn_to_ucsn_with_cmgr (
	const stix_bch_t* bcs, stix_oow_t* bcslen,
	stix_uch_t* ucs, stix_oow_t* ucslen, stix_cmgr_t* cmgr, int all)
{
	const stix_bch_t* p;
	int ret = 0;
	stix_oow_t mlen;

	if (ucs)
	{
		/* destination buffer is specified. 
		 * copy the conversion result to the buffer */

		stix_uch_t* q, * qend;

		p = bcs;
		q = ucs;
		qend = ucs + *ucslen;
		mlen = *bcslen;

		while (mlen > 0)
		{
			stix_oow_t n;

			if (q >= qend)
			{
				/* buffer too small */
				ret = -2;
				break;
			}

			n = cmgr->bctouc (p, mlen, q);
			if (n == 0)
			{
				/* invalid sequence */
				if (all)
				{
					n = 1;
					*q = '?';
				}
				else
				{
					ret = -1;
					break;
				}
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				if (all)
				{
					n = 1;
					*q = '?';
				}
				else
				{
					ret = -3;
					break;
				}
			}

			q++;
			p += n;
			mlen -= n;
		}

		*ucslen = q - ucs;
		*bcslen = p - bcs;
	}
	else
	{
		/* no destination buffer is specified. perform conversion
		 * but don't copy the result. the caller can call this function
		 * without a buffer to find the required buffer size, allocate
		 * a buffer with the size and call this function again with 
		 * the buffer. */

		stix_uch_t w;
		stix_oow_t wlen = 0;

		p = bcs;
		mlen = *bcslen;

		while (mlen > 0)
		{
			stix_oow_t n;

			n = cmgr->bctouc (p, mlen, &w);
			if (n == 0)
			{
				/* invalid sequence */
				if (all) n = 1;
				else
				{
					ret = -1;
					break;
				}
			}
			if (n > mlen)
			{
				/* incomplete sequence */
				if (all) n = 1;
				else
				{
					ret = -3;
					break;
				}
			}

			p += n;
			mlen -= n;
			wlen += 1;
		}

		*ucslen = wlen;
		*bcslen = p - bcs;
	}

	return ret;
}

static STIX_INLINE int bcs_to_ucs_with_cmgr (
	const stix_bch_t* bcs, stix_oow_t* bcslen,
	stix_uch_t* ucs, stix_oow_t* ucslen, stix_cmgr_t* cmgr, int all)
{
	const stix_bch_t* bp;
	stix_oow_t mlen, wlen;
	int n;

	for (bp = bcs; *bp != '\0'; bp++) /* nothing */ ;

	mlen = bp - bcs; wlen = *ucslen;
	n = bcsn_to_ucsn_with_cmgr (bcs, &mlen, ucs, &wlen, cmgr, all);
	if (ucs)
	{
		/* null-terminate the target buffer if it has room for it. */
		if (wlen < *ucslen) ucs[wlen] = '\0';
		else n = -2; /* buffer too small */
	}
	*bcslen = mlen; *ucslen = wlen;

	return n;
}

static STIX_INLINE int ucsn_to_bcsn_with_cmgr (
	const stix_uch_t* ucs, stix_oow_t* ucslen,
	stix_bch_t* bcs, stix_oow_t* bcslen, stix_cmgr_t* cmgr)
{
	const stix_uch_t* p = ucs;
	const stix_uch_t* end = ucs + *ucslen;
	int ret = 0; 

	if (bcs)
	{
		stix_oow_t rem = *bcslen;

		while (p < end) 
		{
			stix_oow_t n;

			if (rem <= 0)
			{
				ret = -2; /* buffer too small */
				break;
			}

			n = cmgr->uctobc (*p, bcs, rem);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}
			if (n > rem) 
			{
				ret = -2; /* buffer too small */
				break;
			}
			bcs += n; rem -= n; p++;
		}

		*bcslen -= rem; 
	}
	else
	{
		stix_bch_t bcsbuf[STIX_BCLEN_MAX];
		stix_oow_t mlen = 0;

		while (p < end)
		{
			stix_oow_t n;

			n = cmgr->uctobc (*p, bcsbuf, STIX_COUNTOF(bcsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that bcsbuf is large enough to hold a character */
			STIX_ASSERT (n <= STIX_COUNTOF(bcsbuf));

			p++; mlen += n;
		}

		/* this length excludes the terminating null character. 
		 * this function doesn't even null-terminate the result. */
		*bcslen = mlen;
	}

	*ucslen = p - ucs;
	return ret;
}


static int ucs_to_bcs_with_cmgr (
	const stix_uch_t* ucs, stix_oow_t* ucslen,
	stix_bch_t* bcs, stix_oow_t* bcslen, stix_cmgr_t* cmgr)
{
	const stix_uch_t* p = ucs;
	int ret = 0;

	if (bcs)
	{
		stix_oow_t rem = *bcslen;

		while (*p != '\0')
		{
			stix_oow_t n;

			if (rem <= 0)
			{
				ret = -2;
				break;
			}
			
			n = cmgr->uctobc (*p, bcs, rem);
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}
			if (n > rem) 
			{
				ret = -2;
				break; /* buffer too small */
			}

			bcs += n; rem -= n; p++;
		}

		/* update bcslen to the length of the bcs string converted excluding
		 * terminating null */
		*bcslen -= rem; 

		/* null-terminate the multibyte sequence if it has sufficient space */
		if (rem > 0) *bcs = '\0';
		else 
		{
			/* if ret is -2 and cs[cslen] == '\0', 
			 * this means that the bcs buffer was lacking one
			 * slot for the terminating null */
			ret = -2; /* buffer too small */
		}
	}
	else
	{
		stix_bch_t bcsbuf[STIX_BCLEN_MAX];
		stix_oow_t mlen = 0;

		while (*p != '\0')
		{
			stix_oow_t n;

			n = cmgr->uctobc (*p, bcsbuf, STIX_COUNTOF(bcsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that bcs is large enough to hold a character */
			STIX_ASSERT (n <= STIX_COUNTOF(bcs));

			p++; mlen += n;
		}

		/* this length holds the number of resulting multi-byte characters 
		 * excluding the terminating null character */
		*bcslen = mlen;
	}

	*ucslen = p - ucs;  /* the number of wide characters handled. */
	return ret;
}

static stix_cmgr_t utf8_cmgr =
{
	stix_utf8touc,
	stix_uctoutf8
};

stix_cmgr_t* stix_getutf8cmgr (void)
{
	return &utf8_cmgr;
}

int stix_utf8toucs (const stix_bch_t* bcs, stix_oow_t* bcslen, stix_uch_t* ucs, stix_oow_t* ucslen)
{
	if (*bcslen == ~(stix_oow_t)0)
	{
		/* the source is null-terminated. */
		return bcs_to_ucs_with_cmgr (bcs, bcslen, ucs, ucslen, &utf8_cmgr, 0);
	}
	else
	{
		/* the source is length bound */
		return bcsn_to_ucsn_with_cmgr (bcs, bcslen, ucs, ucslen, &utf8_cmgr, 0);
	}
}

int stix_ucstoutf8 (const stix_uch_t* ucs, stix_oow_t* ucslen, stix_bch_t* bcs, stix_oow_t* bcslen)
{
	if (*ucslen == ~(stix_oow_t)0)
	{
		/* null-terminated */
		return ucs_to_bcs_with_cmgr (ucs, ucslen, bcs, bcslen, &utf8_cmgr);
	}
	else
	{
		/* length bound */
		return ucsn_to_bcsn_with_cmgr (ucs, ucslen, bcs, bcslen, &utf8_cmgr);
	}
}

int stix_bcstoucs (stix_t* stix, const stix_bch_t* bcs, stix_oow_t* bcslen, stix_uch_t* ucs, stix_oow_t* ucslen)
{
	if (*bcslen == ~(stix_oow_t)0)
	{
		/* the source is null-terminated. */
		return bcs_to_ucs_with_cmgr (bcs, bcslen, ucs, ucslen, stix->cmgr, 0);
	}
	else
	{
		/* the source is length bound */
		return bcsn_to_ucsn_with_cmgr (bcs, bcslen, ucs, ucslen, stix->cmgr, 0);
	}
}

int stix_ucstobcs (stix_t* stix, const stix_uch_t* ucs, stix_oow_t* ucslen, stix_bch_t* bcs, stix_oow_t* bcslen)
{
	if (*ucslen == ~(stix_oow_t)0)
	{
		/* null-terminated */
		return ucs_to_bcs_with_cmgr (ucs, ucslen, bcs, bcslen, stix->cmgr);
	}
	else
	{
		/* length bound */
		return ucsn_to_bcsn_with_cmgr (ucs, ucslen, bcs, bcslen, stix->cmgr);
	}
}

