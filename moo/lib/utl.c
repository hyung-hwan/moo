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

#include "moo-prv.h"

/* ----------------------------------------------------------------------- */
int moo_log2_for_pow2 (moo_oow_t pow2v)
{
#if defined(MOO_HAVE_UINT32_T) && (MOO_SIZEOF_OOW_T == 4)

	static const int debruijn[32] = 
	{
		0, 1, 28, 2, 29, 14, 24, 3,
		30, 22, 20, 15, 25, 17, 4, 8, 
		31, 27, 13, 23, 21, 19, 16, 7,
		26, 12, 18, 6, 11, 5, 10, 9
	};

	return debruijn[(moo_uint32_t)(pow2v * 0x077CB531u) >> 27];

#elif defined(MOO_HAVE_UINT64_T) && (MOO_SIZEOF_OOW_T == 8)

	static const int debruijn[64] = 
	{
		0, 1,  2, 53,  3,  7, 54, 27,
		4, 38, 41,  8, 34, 55, 48, 28,
		62,  5, 39, 46, 44, 42, 22,  9,
		24, 35, 59, 56, 49, 18, 29, 11,
		63, 52,  6, 26, 37, 40, 33, 47,
		61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16,
		50, 31, 19, 15, 30, 14, 13, 12
	};

	return debruijn[(moo_uint64_t)(pow2v * 0x022fdd63cc95386dllu) >> 58];

#else
	int i;
	for (i = -1; pow2v; i++) pow2v >>= 1;
	return (i == -1) ? 0 : i;

#endif
}

/* ----------------------------------------------------------------------- */

moo_oow_t moo_hash_bytes_ (const moo_oob_t* ptr, moo_oow_t len)
{
	moo_oow_t hv;
	MOO_HASH_BYTES (hv, ptr, len);
	/* constrain the hash value to be representable in a small integer
	 * for convenience sake */
	return hv % ((moo_oow_t)MOO_SMOOI_MAX + 1);
}

/* ----------------------------------------------------------------------- */

/* some naming conventions
 *  bchars, uchars -> pointer and length
 *  bcstr, ucstr -> null-terminated string pointer
 *  btouchars -> bchars to uchars
 *  utobchars -> uchars to bchars
 *  btoucstr -> bcstr to ucstr
 *  utobcstr -> ucstr to bcstr
 */

int moo_equal_uchars (const moo_uch_t* str1, const moo_uch_t* str2, moo_oow_t len)
{
	moo_oow_t i;

	/* [NOTE] you should call this function after having ensured that
	 *       str1 and str2 are in the same length */

	for (i = 0; i < len; i++)
	{
		if (str1[i] != str2[i]) return 0;
	}

	return 1;
}

int moo_equal_bchars (const moo_bch_t* str1, const moo_bch_t* str2, moo_oow_t len)
{
	moo_oow_t i;

	/* [NOTE] you should call this function after having ensured that
	 *        str1 and str2 are in the same length */

	for (i = 0; i < len; i++)
	{
		if (str1[i] != str2[i]) return 0;
	}

	return 1;
}

int moo_comp_uchars (const moo_uch_t* str1, moo_oow_t len1, const moo_uch_t* str2, moo_oow_t len2)
{
	moo_uchu_t c1, c2;
	const moo_uch_t* end1 = str1 + len1;
	const moo_uch_t* end2 = str2 + len2;

	while (str1 < end1)
	{
		c1 = *str1;
		if (str2 < end2) 
		{
			c2 = *str2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		str1++; str2++;
	}

	return (str2 < end2)? -1: 0;
}

int moo_comp_bchars (const moo_bch_t* str1, moo_oow_t len1, const moo_bch_t* str2, moo_oow_t len2)
{
	moo_bchu_t c1, c2;
	const moo_bch_t* end1 = str1 + len1;
	const moo_bch_t* end2 = str2 + len2;

	while (str1 < end1)
	{
		c1 = *str1;
		if (str2 < end2) 
		{
			c2 = *str2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		str1++; str2++;
	}

	return (str2 < end2)? -1: 0;
}

int moo_comp_ucstr (const moo_uch_t* str1, const moo_uch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++; str2++;
	}

	return ((moo_uchu_t)*str1 > (moo_uchu_t)*str2)? 1: -1;
}

int moo_comp_bcstr (const moo_bch_t* str1, const moo_bch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++; str2++;
	}

	return ((moo_bchu_t)*str1 > (moo_bchu_t)*str2)? 1: -1;
}

int moo_comp_ucstr_bcstr (const moo_uch_t* str1, const moo_bch_t* str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == '\0') return 0;
		str1++; str2++;
	}

	return ((moo_uchu_t)*str1 > (moo_bchu_t)*str2)? 1: -1;
}

int moo_comp_uchars_ucstr (const moo_uch_t* str1, moo_oow_t len, const moo_uch_t* str2)
{
	/* for "abc\0" of length 4 vs "abc", the fourth character
	 * of the first string is equal to the terminating null of
	 * the second string. the first string is still considered 
	 * bigger */
	const moo_uch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0') 
	{
		if (*str1 != *str2) return ((moo_uchu_t)*str1 > (moo_uchu_t)*str2)? 1: -1;
		str1++; str2++;
	}
	return (str1 < end)? 1: (*str2 == '\0'? 0: -1);
}

int moo_comp_uchars_bcstr (const moo_uch_t* str1, moo_oow_t len, const moo_bch_t* str2)
{
	const moo_uch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0') 
	{
		if (*str1 != *str2) return ((moo_uchu_t)*str1 > (moo_bchu_t)*str2)? 1: -1;
		str1++; str2++;
	}
	return (str1 < end)? 1: (*str2 == '\0'? 0: -1);
}

int moo_comp_bchars_bcstr (const moo_bch_t* str1, moo_oow_t len, const moo_bch_t* str2)
{
	const moo_bch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0') 
	{
		if (*str1 != *str2) return ((moo_bchu_t)*str1 > (moo_bchu_t)*str2)? 1: -1;
		str1++; str2++;
	}
	return (str1 < end)? 1: (*str2 == '\0'? 0: -1);
}

int moo_comp_bchars_ucstr (const moo_bch_t* str1, moo_oow_t len, const moo_uch_t* str2)
{
	const moo_bch_t* end = str1 + len;
	while (str1 < end && *str2 != '\0') 
	{
		if (*str1 != *str2) return ((moo_bchu_t)*str1 > (moo_uchu_t)*str2)? 1: -1;
		str1++; str2++;
	}
	return (str1 < end)? 1: (*str2 == '\0'? 0: -1);
}

void moo_copy_uchars (moo_uch_t* dst, const moo_uch_t* src, moo_oow_t len)
{
	/* take note of no forced null termination */
	moo_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

void moo_copy_bchars (moo_bch_t* dst, const moo_bch_t* src, moo_oow_t len)
{
	/* take note of no forced null termination */
	moo_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

void moo_copy_bchars_to_uchars (moo_uch_t* dst, const moo_bch_t* src, moo_oow_t len)
{
	/* copy without conversions.
	 * use moo_bctouchars() for conversion encoding */
	moo_oow_t i;
	for (i = 0; i < len; i++) dst[i] = src[i];
}

moo_oow_t moo_copy_ucstr (moo_uch_t* dst, moo_oow_t len, const moo_uch_t* src)
{
	moo_uch_t* p, * p2;

	p = dst; p2 = dst + len - 1;

	while (p < p2)
	{
		 if (*src == '\0') break;
		 *p++ = *src++;
	}

	if (len > 0) *p = '\0';
	return p - dst;
}

moo_oow_t moo_copy_bcstr (moo_bch_t* dst, moo_oow_t len, const moo_bch_t* src)
{
	moo_bch_t* p, * p2;

	p = dst; p2 = dst + len - 1;

	while (p < p2)
	{
		 if (*src == '\0') break;
		 *p++ = *src++;
	}

	if (len > 0) *p = '\0';
	return p - dst;
}

void moo_fill_uchars (moo_uch_t* dst, moo_uch_t ch, moo_oow_t len)
{
	moo_oow_t i;
	for (i = 0; i < len; i++) dst[i] = ch;
}

void moo_fill_bchars (moo_bch_t* dst, moo_bch_t ch, moo_oow_t len)
{
	moo_oow_t i;
	for (i = 0; i < len; i++) dst[i] = ch;
}

moo_oow_t moo_count_ucstr (const moo_uch_t* str)
{
	const moo_uch_t* ptr = str;
	while (*ptr != '\0') ptr++;
	return ptr - str;
}

moo_oow_t moo_count_ucstr_limited (const moo_uch_t* str, moo_oow_t maxlen)
{
	moo_oow_t i;
	for (i = 0; i < maxlen; i++)
	{
		if (str[i] == '\0') break;
	}
	return i;
}

moo_oow_t moo_count_bcstr (const moo_bch_t* str)
{
	const moo_bch_t* ptr = str;
	while (*ptr != '\0') ptr++;
	return ptr - str;
}

moo_oow_t moo_count_bcstr_limited (const moo_bch_t* str, moo_oow_t maxlen)
{
	moo_oow_t i;
	for (i = 0; i < maxlen; i++)
	{
		if (str[i] == '\0') break;
	}
	return i;
}

moo_uch_t* moo_find_uchar (const moo_uch_t* ptr, moo_oow_t len, moo_uch_t c)
{
	const moo_uch_t* end;

	end = ptr + len;
	while (ptr < end)
	{
		if (*ptr == c) return (moo_uch_t*)ptr;
		ptr++;
	}

	return MOO_NULL;
}

moo_bch_t* moo_find_bchar (const moo_bch_t* ptr, moo_oow_t len, moo_bch_t c)
{
	const moo_bch_t* end;

	end = ptr + len;
	while (ptr < end)
	{
		if (*ptr == c) return (moo_bch_t*)ptr;
		ptr++;
	}

	return MOO_NULL;
}

moo_uch_t* moo_rfind_uchar (const moo_uch_t* ptr, moo_oow_t len, moo_uch_t c)
{
	const moo_uch_t* cur;

	cur = ptr + len;
	while (cur > ptr)
	{
		--cur;
		if (*cur == c) return (moo_uch_t*)cur;
	}

	return MOO_NULL;
}

moo_bch_t* moo_rfind_bchar (const moo_bch_t* ptr, moo_oow_t len, moo_bch_t c)
{
	const moo_bch_t* cur;

	cur = ptr + len;
	while (cur > ptr)
	{
		--cur;
		if (*cur == c) return (moo_bch_t*)cur;
	}

	return MOO_NULL;
}

moo_uch_t* moo_find_uchar_in_ucstr (const moo_uch_t* ptr, moo_uch_t c)
{
	while (*ptr != '\0')
	{
		if (*ptr == c) return (moo_uch_t*)ptr;
		ptr++;
	}

	return MOO_NULL;
}

moo_bch_t* moo_find_bchar_in_bcstr (const moo_bch_t* ptr, moo_bch_t c)
{
	while (*ptr != '\0')
	{
		if (*ptr == c) return (moo_bch_t*)ptr;
		ptr++;
	}

	return MOO_NULL;
}

moo_oow_t moo_rotate_bchars (moo_bch_t* str, moo_oow_t len, int dir, moo_oow_t n)
{
	moo_oow_t first, last, count, index, nk;
	moo_bch_t c;

	if (dir == 0 || len == 0) return len;
	if ((n %= len) == 0) return len;

	if (dir > 0) n = len - n;
	first = 0; nk = len - n; count = 0;

	while (count < n)
	{
		last = first + nk;
		index = first;
		c = str[first];
		do
		{
			count++;
			while (index < nk)
			{
				str[index] = str[index + n];
				index += n;
			}
			if (index == last) break;
			str[index] = str[index - nk];
			index -= nk;
		}
		while (1);
		str[last] = c; first++;
	}
	return len;
}

moo_oow_t moo_rotate_uchars (moo_uch_t* str, moo_oow_t len, int dir, moo_oow_t n)
{
	moo_oow_t first, last, count, index, nk;
	moo_uch_t c;

	if (dir == 0 || len == 0) return len;
	if ((n %= len) == 0) return len;

	if (dir > 0) n = len - n;
	first = 0; nk = len - n; count = 0;

	while (count < n)
	{
		last = first + nk;
		index = first;
		c = str[first];
		do
		{
			count++;
			while (index < nk)
			{
				str[index] = str[index + n];
				index += n;
			}
			if (index == last) break;
			str[index] = str[index - nk];
			index -= nk;
		}
		while (1);
		str[last] = c; first++;
	}
	return len;
}

/* ----------------------------------------------------------------------- */

moo_oow_t moo_byte_to_ucstr (moo_oob_t byte, moo_uch_t* buf, moo_oow_t size, int flagged_radix, moo_uch_t fill)
{
	moo_uch_t tmp[(MOO_SIZEOF(moo_oob_t) * MOO_BITS_PER_BYTE)];
	moo_uch_t* p = tmp, * bp = buf, * be = buf + size - 1;
	int radix;
	moo_uch_t radix_char;

	radix = (flagged_radix & MOO_BYTE_TO_UCSTR_RADIXMASK);
	radix_char = (flagged_radix & MOO_BYTE_TO_UCSTR_LOWERCASE)? 'a': 'A';
	if (radix < 2 || radix > 36 || size <= 0) return 0;

	do 
	{
		moo_oob_t digit = byte % radix;	
		if (digit < 10) *p++ = digit + '0';
		else *p++ = digit + radix_char - 10;
		byte /= radix;
	}
	while (byte > 0);

	if (fill != '\0') 
	{
		while (size - 1 > p - tmp) 
		{
			*bp++ = fill;
			size--;
		}
	}

	while (p > tmp && bp < be) *bp++ = *--p;
	*bp = '\0';
	return bp - buf;
}

moo_oow_t moo_byte_to_bcstr (moo_oob_t byte, moo_bch_t* buf, moo_oow_t size, int flagged_radix, moo_bch_t fill)
{
	moo_bch_t tmp[(MOO_SIZEOF(moo_oob_t) * MOO_BITS_PER_BYTE)];
	moo_bch_t* p = tmp, * bp = buf, * be = buf + size - 1;
	int radix;
	moo_bch_t radix_char;

	radix = (flagged_radix & MOO_BYTE_TO_BCSTR_RADIXMASK);
	radix_char = (flagged_radix & MOO_BYTE_TO_BCSTR_LOWERCASE)? 'a': 'A';
	if (radix < 2 || radix > 36 || size <= 0) return 0;

	do 
	{
		moo_oob_t digit = byte % radix;	
		if (digit < 10) *p++ = digit + '0';
		else *p++ = digit + radix_char - 10;
		byte /= radix;
	}
	while (byte > 0);

	if (fill != '\0') 
	{
		while (size - 1 > p - tmp) 
		{
			*bp++ = fill;
			size--;
		}
	}

	while (p > tmp && bp < be) *bp++ = *--p;
	*bp = '\0';
	return bp - buf;
}

/* ----------------------------------------------------------------------- */

MOO_INLINE int moo_conv_bchars_to_uchars_with_cmgr (
	const moo_bch_t* bcs, moo_oow_t* bcslen,
	moo_uch_t* ucs, moo_oow_t* ucslen, moo_cmgr_t* cmgr, int all)
{
	const moo_bch_t* p;
	int ret = 0;
	moo_oow_t mlen;

	if (ucs)
	{
		/* destination buffer is specified. 
		 * copy the conversion result to the buffer */

		moo_uch_t* q, * qend;

		p = bcs;
		q = ucs;
		qend = ucs + *ucslen;
		mlen = *bcslen;

		while (mlen > 0)
		{
			moo_oow_t n;

			if (q >= qend)
			{
				/* buffer too small */
				ret = -2;
				break;
			}

			n = cmgr->bctouc(p, mlen, q);
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

		moo_uch_t w;
		moo_oow_t wlen = 0;

		p = bcs;
		mlen = *bcslen;

		while (mlen > 0)
		{
			moo_oow_t n;

			n = cmgr->bctouc(p, mlen, &w);
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

MOO_INLINE int moo_conv_bcstr_to_ucstr_with_cmgr (
	const moo_bch_t* bcs, moo_oow_t* bcslen,
	moo_uch_t* ucs, moo_oow_t* ucslen, moo_cmgr_t* cmgr, int all)
{
	const moo_bch_t* bp;
	moo_oow_t mlen, wlen;
	int n;

	for (bp = bcs; *bp != '\0'; bp++) /* nothing */ ;

	mlen = bp - bcs; wlen = *ucslen;
	n = moo_conv_bchars_to_uchars_with_cmgr (bcs, &mlen, ucs, &wlen, cmgr, all);
	if (ucs)
	{
		/* null-terminate the target buffer if it has room for it. */
		if (wlen < *ucslen) ucs[wlen] = '\0';
		else n = -2; /* buffer too small */
	}
	*bcslen = mlen; *ucslen = wlen;

	return n;
}

MOO_INLINE int moo_conv_uchars_to_bchars_with_cmgr (
	const moo_uch_t* ucs, moo_oow_t* ucslen,
	moo_bch_t* bcs, moo_oow_t* bcslen, moo_cmgr_t* cmgr)
{
	const moo_uch_t* p = ucs;
	const moo_uch_t* end = ucs + *ucslen;
	int ret = 0; 

	if (bcs)
	{
		moo_oow_t rem = *bcslen;

		while (p < end) 
		{
			moo_oow_t n;

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
		moo_bch_t bcsbuf[MOO_BCSIZE_MAX];
		moo_oow_t mlen = 0;

		while (p < end)
		{
			moo_oow_t n;

			n = cmgr->uctobc (*p, bcsbuf, MOO_COUNTOF(bcsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that bcsbuf is large enough to hold a character */
			/*MOO_ASSERT (moo, n <= MOO_COUNTOF(bcsbuf));*/

			p++; mlen += n;
		}

		/* this length excludes the terminating null character. 
		 * this function doesn't even null-terminate the result. */
		*bcslen = mlen;
	}

	*ucslen = p - ucs;
	return ret;
}

MOO_INLINE int moo_conv_ucstr_to_bcstr_with_cmgr (
	const moo_uch_t* ucs, moo_oow_t* ucslen,
	moo_bch_t* bcs, moo_oow_t* bcslen, moo_cmgr_t* cmgr)
{
	const moo_uch_t* p = ucs;
	int ret = 0;

	if (bcs)
	{
		moo_oow_t rem = *bcslen;

		while (*p != '\0')
		{
			moo_oow_t n;

			if (rem <= 0)
			{
				ret = -2;
				break;
			}
			
			n = cmgr->uctobc(*p, bcs, rem);
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
		moo_bch_t bcsbuf[MOO_BCSIZE_MAX];
		moo_oow_t mlen = 0;

		while (*p != '\0')
		{
			moo_oow_t n;

			n = cmgr->uctobc(*p, bcsbuf, MOO_COUNTOF(bcsbuf));
			if (n == 0) 
			{
				ret = -1;
				break; /* illegal character */
			}

			/* it assumes that bcs is large enough to hold a character */
			/*MOO_ASSERT (moo, n <= MOO_COUNTOF(bcs));*/

			p++; mlen += n;
		}

		/* this length holds the number of resulting multi-byte characters 
		 * excluding the terminating null character */
		*bcslen = mlen;
	}

	*ucslen = p - ucs;  /* the number of wide characters handled. */
	return ret;
}

/* ----------------------------------------------------------------------- */

static moo_cmgr_t builtin_cmgr[] =
{
	/* keep the order aligned with moo_cmgr_id_t values in <moo-utl.h> */
	{ moo_utf8_to_uc,  moo_uc_to_utf8 },
	{ moo_utf16_to_uc, moo_uc_to_utf16 },
	{ moo_mb8_to_uc,   moo_uc_to_mb8 }
};

moo_cmgr_t* moo_get_cmgr_by_id (moo_cmgr_id_t id)
{
	return &builtin_cmgr[id];
}


static struct
{
	const moo_bch_t* name;
	moo_cmgr_id_t     id;
} builtin_cmgr_tab[] =
{
	{ "utf8",    MOO_CMGR_UTF8 },
	{ "utf16",   MOO_CMGR_UTF16 },
	{ "mb8",     MOO_CMGR_MB8 }
};

moo_cmgr_t* moo_get_cmgr_by_bcstr (const moo_bch_t* name)
{
	if (name)
	{
		moo_oow_t i;

		for (i = 0; i < MOO_COUNTOF(builtin_cmgr_tab); i++)
		{
			if (moo_comp_bcstr(name, builtin_cmgr_tab[i].name) == 0)
			{
				return &builtin_cmgr[builtin_cmgr_tab[i].id];
			}
		 }
	}

	return MOO_NULL;
}

moo_cmgr_t* moo_get_cmgr_by_ucstr (const moo_uch_t* name)
{
	if (name)
	{
		moo_oow_t i;

		for (i = 0; i < MOO_COUNTOF(builtin_cmgr_tab); i++)
		{
			if (moo_comp_ucstr_bcstr(name, builtin_cmgr_tab[i].name) == 0)
			{
				return &builtin_cmgr[builtin_cmgr_tab[i].id];
			}
		 }
	}

	return MOO_NULL;
}

/* ----------------------------------------------------------------------- */

int moo_conv_utf8_to_uchars (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* the source is length bound */
	return moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_UTF8], 0);
}

int moo_conv_uchars_to_utf8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	return moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_UTF8]);
}

int moo_conv_utf8_to_ucstr (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	return moo_conv_bcstr_to_ucstr_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_UTF8], 0);
}

int moo_conv_ucstr_to_utf8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	return moo_conv_ucstr_to_bcstr_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_UTF8]);
}

/* ----------------------------------------------------------------------- */

int moo_conv_utf16_to_uchars (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* the source is length bound */
	return moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_UTF16], 0);
}

int moo_conv_uchars_to_utf16 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	return moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_UTF16]);
}

int moo_conv_utf16_to_ucstr (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	return moo_conv_bcstr_to_ucstr_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_UTF16], 0);
}

int moo_conv_ucstr_to_utf16 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	return moo_conv_ucstr_to_bcstr_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_UTF16]);
}

/* ----------------------------------------------------------------------- */

int moo_conv_mb8_to_uchars (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* the source is length bound */
	return moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_MB8], 0);
}

int moo_conv_uchars_to_mb8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	return moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_MB8]);
}

int moo_conv_mb8_to_ucstr (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	return moo_conv_bcstr_to_ucstr_with_cmgr(bcs, bcslen, ucs, ucslen, &builtin_cmgr[MOO_CMGR_MB8], 0);
}

int moo_conv_ucstr_to_mb8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	return moo_conv_ucstr_to_bcstr_with_cmgr(ucs, ucslen, bcs, bcslen, &builtin_cmgr[MOO_CMGR_MB8]);
}

/* ----------------------------------------------------------------------- */

int moo_convbtouchars (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* length bound */
	int n;
	n = moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, moo_getcmgr(moo), 0);
	/* -1: illegal character, -2: buffer too small, -3: incomplete sequence */
	if (n <= -1) moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	return n;
}

int moo_convutobchars (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	int n;
	n = moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, moo_getcmgr(moo));
	if (n <= -1) moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	return n;
}

int moo_convbtoucstr (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	int n;
	n = moo_conv_bcstr_to_ucstr_with_cmgr(bcs, bcslen, ucs, ucslen, moo_getcmgr(moo), 0);
	if (n <= -1) moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	return n;
}

int moo_convutobcstr (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	int n;
	n = moo_conv_ucstr_to_bcstr_with_cmgr(ucs, ucslen, bcs, bcslen, moo_getcmgr(moo));
	if (n <= -1) moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	return n;
}

/* ----------------------------------------------------------------------- */

moo_uch_t* moo_dupbtoucharswithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_bch_t* bcs, moo_oow_t bcslen, moo_oow_t* ucslen)
{
	moo_oow_t inlen, outlen;
	moo_uch_t* ptr;

	inlen = bcslen;
	if (moo_convbtouchars(moo, bcs, &inlen, MOO_NULL, &outlen) <= -1) 
	{
		/* note it's also an error if no full conversion is made in this function */
		return MOO_NULL;
	}

	ptr = (moo_uch_t*)moo_allocmem(moo, headroom_bytes + ((outlen + 1) * MOO_SIZEOF(moo_uch_t)));
	if (!ptr) return MOO_NULL;

	inlen = bcslen;

	ptr = (moo_uch_t*)((moo_oob_t*)ptr + headroom_bytes);
	moo_convbtouchars (moo, bcs, &inlen, ptr, &outlen);

	/* moo_convbtouchars() doesn't null-terminate the target. 
	 * but in moo_dupbtouchars(), i allocate space. so i don't mind
	 * null-terminating it with 1 extra character overhead */
	ptr[outlen] = '\0'; 
	if (ucslen) *ucslen = outlen;
	return ptr;
}

moo_bch_t* moo_duputobcharswithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_uch_t* ucs, moo_oow_t ucslen, moo_oow_t* bcslen)
{
	moo_oow_t inlen, outlen;
	moo_bch_t* ptr;

	inlen = ucslen;
	if (moo_convutobchars(moo, ucs, &inlen, MOO_NULL, &outlen) <= -1) 
	{
		/* note it's also an error if no full conversion is made in this function */
		return MOO_NULL;
	}

	ptr = (moo_bch_t*)moo_allocmem(moo, headroom_bytes + ((outlen + 1) * MOO_SIZEOF(moo_bch_t)));
	if (!ptr) return MOO_NULL;

	inlen = ucslen;
	ptr = (moo_bch_t*)((moo_oob_t*)ptr + headroom_bytes);
	moo_convutobchars (moo, ucs, &inlen, ptr, &outlen);

	ptr[outlen] = '\0';
	if (bcslen) *bcslen = outlen;
	return ptr;
}

/* ----------------------------------------------------------------------- */

moo_uch_t* moo_dupbtoucstrwithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_bch_t* bcs, moo_oow_t* ucslen)
{
	moo_oow_t inlen, outlen;
	moo_uch_t* ptr;

	if (moo_convbtoucstr(moo, bcs, &inlen, MOO_NULL, &outlen) <= -1) 
	{
		/* note it's also an error if no full conversion is made in this function */
		return MOO_NULL;
	}

	outlen++;
	ptr = (moo_uch_t*)moo_allocmem(moo, headroom_bytes + (outlen * MOO_SIZEOF(moo_uch_t)));
	if (!ptr) return MOO_NULL;

	moo_convbtoucstr (moo, bcs, &inlen, ptr, &outlen);
	if (ucslen) *ucslen = outlen;
	return ptr;
}

moo_bch_t* moo_duputobcstrwithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_uch_t* ucs, moo_oow_t* bcslen)
{
	moo_oow_t inlen, outlen;
	moo_bch_t* ptr;

	if (moo_convutobcstr(moo, ucs, &inlen, MOO_NULL, &outlen) <= -1) 
	{
		/* note it's also an error if no full conversion is made in this function */
		return MOO_NULL;
	}

	outlen++;
	ptr = (moo_bch_t*)moo_allocmem(moo, headroom_bytes + (outlen * MOO_SIZEOF(moo_bch_t)));
	if (!ptr) return MOO_NULL;

	ptr = (moo_bch_t*)((moo_oob_t*)ptr + headroom_bytes);

	moo_convutobcstr (moo, ucs, &inlen, ptr, &outlen);
	if (bcslen) *bcslen = outlen;
	return ptr;
}

/* ----------------------------------------------------------------------- */
moo_uch_t* moo_dupucstr (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* ucslen)
{
	moo_uch_t* ptr;
	moo_oow_t len;

	len = moo_count_ucstr(ucs);

	ptr = (moo_uch_t*)moo_allocmem(moo, (len + 1) * MOO_SIZEOF(moo_uch_t));
	if (!ptr) return MOO_NULL;

	moo_copy_uchars (ptr, ucs, len);
	ptr[len] = '\0';

	if (ucslen) *ucslen = len;
	return ptr;
}

moo_bch_t* moo_dupbcstr (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* bcslen)
{
	moo_bch_t* ptr;
	moo_oow_t len;

	len = moo_count_bcstr(bcs);

	ptr = (moo_bch_t*)moo_allocmem(moo, (len + 1) * MOO_SIZEOF(moo_bch_t));
	if (!ptr) return MOO_NULL;

	moo_copy_bchars (ptr, bcs, len);
	ptr[len] = '\0';

	if (bcslen) *bcslen = len;
	return ptr;
}

moo_uch_t* moo_dupuchars (moo_t* moo, const moo_uch_t* ucs, moo_oow_t ucslen)
{
	moo_uch_t* ptr;

	ptr = (moo_uch_t*)moo_allocmem(moo, (ucslen + 1) * MOO_SIZEOF(moo_uch_t));
	if (!ptr) return MOO_NULL;

	moo_copy_uchars (ptr, ucs, ucslen);
	ptr[ucslen] = '\0';
	return ptr;
}

moo_bch_t* moo_dupbchars (moo_t* moo, const moo_bch_t* bcs, moo_oow_t bcslen)
{
	moo_bch_t* ptr;

	ptr = (moo_bch_t*)moo_allocmem(moo, (bcslen + 1) * MOO_SIZEOF(moo_bch_t));
	if (!ptr) return MOO_NULL;

	moo_copy_bchars (ptr, bcs, bcslen);
	ptr[bcslen] = '\0';
	return ptr;
}

/* ----------------------------------------------------------------------- */

static MOO_INLINE int secure_space_in_sbuf (moo_t* moo, moo_oow_t req, moo_sbuf_t* p)
{
	if (req > p->capa - p->len)
	{
		moo_oow_t newcapa;
		moo_ooch_t* tmp;

		newcapa = p->len + req + 1;
		newcapa = MOO_ALIGN_POW2(newcapa, 512); /* TODO: adjust this capacity */

		tmp = (moo_ooch_t*)moo_reallocmem(moo, p->ptr, newcapa * MOO_SIZEOF(*tmp)); 
		if (!tmp) return -1;

		p->ptr = tmp;
		p->capa = newcapa - 1;
	}

	return 0;
}

int moo_concatoocstrtosbuf (moo_t* moo, const moo_ooch_t* str, moo_sbuf_id_t id)
{
	return moo_concatoocharstosbuf(moo, str, moo_count_oocstr(str), id);
}

int moo_concatoocharstosbuf (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len, moo_sbuf_id_t id)
{
	moo_sbuf_t* p;

	p = &moo->sbuf[id];
	if (secure_space_in_sbuf(moo, len, p) <= -1) return -1;
	moo_copy_oochars (&p->ptr[p->len], ptr, len);
	p->len += len;
	p->ptr[p->len] = '\0';

	return 0;
}

int moo_concatoochartosbuf (moo_t* moo, moo_ooch_t ch, moo_oow_t count, moo_sbuf_id_t id)
{
	moo_sbuf_t* p;

	p = &moo->sbuf[id];
	if (secure_space_in_sbuf(moo, count, p) <= -1) return -1;
	moo_fill_oochars (&p->ptr[p->len], ch, count);
	p->len += count;
	p->ptr[p->len] = '\0';

	return 0;
}

int moo_copyoocstrtosbuf (moo_t* moo, const moo_ooch_t* str, moo_sbuf_id_t id)
{
	moo->sbuf[id].len = 0;
	return moo_concatoocstrtosbuf(moo, str, id);
}

int moo_copyoocharstosbuf (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len, moo_sbuf_id_t id)
{
	moo->sbuf[id].len = 0;
	return moo_concatoocharstosbuf(moo, ptr, len, id);
}
