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

#include "stix-prv.h"

#define STIX_BCLEN_MAX 16

/*
 * from RFC 2279 UTF-8, a transformation format of ISO 10646
 *
 *     UCS-4 range (hex.)  UTF-8 octet sequence (binary)
 * 1:2 00000000-0000007F  0xxxxxxx
 * 2:2 00000080-000007FF  110xxxxx 10xxxxxx
 * 3:2 00000800-0000FFFF  1110xxxx 10xxxxxx 10xxxxxx
 * 4:4 00010000-001FFFFF  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * inv 00200000-03FFFFFF  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * inv 04000000-7FFFFFFF  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

struct __utf8_t
{
	stix_uint32_t  lower;
	stix_uint32_t  upper;
	stix_uint8_t   fbyte;  /* mask to the first utf8 byte */
	stix_uint8_t   mask;
	stix_uint8_t   fmask;
	int            length; /* number of bytes */
};

typedef struct __utf8_t __utf8_t;

static __utf8_t utf8_table[] = 
{
	{0x00000000ul, 0x0000007Ful, 0x00, 0x80, 0x7F, 1},
	{0x00000080ul, 0x000007FFul, 0xC0, 0xE0, 0x1F, 2},
	{0x00000800ul, 0x0000FFFFul, 0xE0, 0xF0, 0x0F, 3},
	{0x00010000ul, 0x001FFFFFul, 0xF0, 0xF8, 0x07, 4},
	{0x00200000ul, 0x03FFFFFFul, 0xF8, 0xFC, 0x03, 5},
	{0x04000000ul, 0x7FFFFFFFul, 0xFC, 0xFE, 0x01, 6}
};

static STIX_INLINE __utf8_t* get_utf8_slot (stix_char_t uc)
{
	__utf8_t* cur, * end;

	STIX_ASSERT (STIX_SIZEOF(stix_bchar_t) == 1);
	STIX_ASSERT (STIX_SIZEOF(stix_char_t) >= 2);

	end = utf8_table + STIX_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if (uc >= cur->lower && uc <= cur->upper) return cur;
		cur++;
	}

	return STIX_NULL; /* invalid character */
}

stix_size_t stix_uctoutf8 (stix_char_t uc, stix_bchar_t* utf8, stix_size_t size)
{
	__utf8_t* cur = get_utf8_slot (uc);

	if (cur == STIX_NULL) return 0; /* illegal character */

	if (utf8 && cur->length <= size)
	{
		int index = cur->length;
		while (index > 1) 
		{
			/*
			 * 0x3F: 00111111
			 * 0x80: 10000000
			 */
			utf8[--index] = (uc & 0x3F) | 0x80;
			uc >>= 6;
		}

		utf8[0] = uc | cur->fbyte;
	}

	/* small buffer is also indicated by this return value
	 * greater than 'size'. */
	return (stix_size_t)cur->length;
}

stix_size_t stix_utf8touc (const stix_bchar_t* utf8, stix_size_t size, stix_char_t* uc)
{
	__utf8_t* cur, * end;

	STIX_ASSERT (utf8 != STIX_NULL);
	STIX_ASSERT (size > 0);
	STIX_ASSERT (STIX_SIZEOF(stix_bchar_t) == 1);
	STIX_ASSERT (STIX_SIZEOF(stix_char_t) >= 2);

	end = utf8_table + STIX_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if ((utf8[0] & cur->mask) == cur->fbyte) 
		{

			/* if size is less that cur->length, the incomplete-seqeunce 
			 * error is naturally indicated. so validate the string
			 * only if size is as large as cur->length. */

			if (size >= cur->length) 
			{
				int i;

				if (uc)
				{
					stix_char_t w;

					w = utf8[0] & cur->fmask;
					for (i = 1; i < cur->length; i++)
					{
						/* in utf8, trailing bytes are all
						 * set with 0x80. 
						 *
						 *   10XXXXXX & 11000000 => 10000000
						 *
						 * if not, invalid. */
						if ((utf8[i] & 0xC0) != 0x80) return 0; 
						w = (w << 6) | (utf8[i] & 0x3F);
					}
					*uc = w;
				}
				else
				{
					for (i = 1; i < cur->length; i++)
					{
						/* in utf8, trailing bytes are all
						 * set with 0x80. 
						 *
						 *   10XXXXXX & 11000000 => 10000000
						 *
						 * if not, invalid. */
						if ((utf8[i] & 0xC0) != 0x80) return 0; 
					}
				}
			}

			/* this return value can indicate both 
			 *    the correct length (len >= cur->length) 
			 * and 
			 *    the incomplete seqeunce error (len < cur->length).
			 */
			return (stix_size_t)cur->length;
		}
		cur++;
	}

	return 0; /* error - invalid sequence */
}

stix_size_t stix_utf8len (const stix_bchar_t* utf8, stix_size_t size)
{
	return stix_utf8touc (utf8, size, STIX_NULL);
}

/* ----------------------------------------------------------------------- */

static int bcsn_to_csn_with_cmgr (
	const stix_bchar_t* bcs, stix_size_t* bcslen,
	stix_char_t* cs, stix_size_t* cslen, stix_cmgr_t* cmgr, int all)
{
	const stix_bchar_t* p;
	int ret = 0;
	stix_size_t mlen;

	if (cs)
	{
		stix_char_t* q, * qend;

		p = bcs;
		q = cs;
		qend = cs + *cslen;
		mlen = *bcslen;

		while (mlen > 0)
		{
			stix_size_t n;

			if (q >= qend)
			{
				/* buffer too small */
				ret = -2;
				break;
			}

			n = cmgr->bctoc (p, mlen, q);
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

		*cslen = q - cs;
		*bcslen = p - bcs;
	}
	else
	{
		stix_char_t w;
		stix_size_t wlen = 0;

		p = bcs;
		mlen = *bcslen;

		while (mlen > 0)
		{
			stix_size_t n;

			n = cmgr->bctoc (p, mlen, &w);
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

		*cslen = wlen;
		*bcslen = p - bcs;
	}

	return ret;
}

static int csn_to_bcsn_with_cmgr (
	const stix_char_t* cs, stix_size_t* cslen,
	stix_bchar_t* bcs, stix_size_t* bcslen, stix_cmgr_t* cmgr)
{
	const stix_char_t* p = cs;
	const stix_char_t* end = cs + *cslen;
	int ret = 0; 

	if (bcs)
	{
		stix_size_t rem = *bcslen;

		while (p < end) 
		{
			stix_size_t n;

			if (rem <= 0)
			{
				ret = -2; /* buffer too small */
				break;
			}

			n = cmgr->ctobc (*p, bcs, rem);
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
		stix_bchar_t bcsbuf[STIX_BCLEN_MAX];
		stix_size_t mlen = 0;

		while (p < end)
		{
			stix_size_t n;

			n = cmgr->ctobc (*p, bcsbuf, STIX_COUNTOF(bcsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that bcs is large enough to hold a character */
			STIX_ASSERT (n <= STIX_COUNTOF(bcsbuf));

			p++; mlen += n;
		}

		/* this length excludes the terminating null character. 
		 * this function doesn't event null-terminate the result. */
		*bcslen = mlen;
	}

	*cslen = p - cs;

	return ret;
}


static stix_cmgr_t utf8_cmgr =
{
	stix_utf8touc,
	stix_uctoutf8
};

int stix_utf8toucs (
	const stix_bchar_t* bcs, stix_size_t* bcslen,
	stix_char_t* ucs, stix_size_t* ucslen)
{
	return bcsn_to_csn_with_cmgr (bcs, bcslen, ucs, ucslen, &utf8_cmgr, 0);
}

int stix_ucstoutf8 (
	const stix_char_t* ucs, stix_size_t *ucslen,
	stix_bchar_t* bcs, stix_size_t* bcslen)
{
	return csn_to_bcsn_with_cmgr (ucs, ucslen, bcs, bcslen, &utf8_cmgr);
}
