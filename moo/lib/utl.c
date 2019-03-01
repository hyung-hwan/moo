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

#include "moo-prv.h"

/* ----------------------------------------------------------------------- */

#if defined(MOO_HAVE_UINT16_T)

moo_uint16_t moo_ntoh16 (moo_uint16_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint16_t)(
		((moo_uint16_t)c[0] << 8) |
		((moo_uint16_t)c[1] << 0));
#else
#	error Unknown endian
#endif
}

moo_uint16_t moo_hton16 (moo_uint16_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint16_t)(
		((moo_uint16_t)c[0] << 8) |
		((moo_uint16_t)c[1] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* --------------------------------------------------------------- */

#if defined(MOO_HAVE_UINT32_T)

moo_uint32_t moo_ntoh32 (moo_uint32_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint32_t)(
		((moo_uint32_t)c[0] << 24) |
		((moo_uint32_t)c[1] << 16) |
		((moo_uint32_t)c[2] << 8) | 
		((moo_uint32_t)c[3] << 0));
#else
#	error Unknown endian
#endif
}

moo_uint32_t moo_hton32 (moo_uint32_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint32_t)(
		((moo_uint32_t)c[0] << 24) |
		((moo_uint32_t)c[1] << 16) |
		((moo_uint32_t)c[2] << 8) | 
		((moo_uint32_t)c[3] << 0));
#else
#	error Unknown endian
#endif
}
#endif


#if defined(MOO_HAVE_UINT64_T)

moo_uint64_t moo_ntoh64 (moo_uint64_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint64_t)(
		((moo_uint64_t)c[0] << 56) | 
		((moo_uint64_t)c[1] << 48) | 
		((moo_uint64_t)c[2] << 40) |
		((moo_uint64_t)c[3] << 32) |
		((moo_uint64_t)c[4] << 24) |
		((moo_uint64_t)c[5] << 16) |
		((moo_uint64_t)c[6] << 8)  |
		((moo_uint64_t)c[7] << 0));
#else
#	error Unknown endian
#endif
}

moo_uint64_t moo_hton64 (moo_uint64_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint64_t)(
		((moo_uint64_t)c[0] << 56) | 
		((moo_uint64_t)c[1] << 48) | 
		((moo_uint64_t)c[2] << 40) |
		((moo_uint64_t)c[3] << 32) |
		((moo_uint64_t)c[4] << 24) |
		((moo_uint64_t)c[5] << 16) |
		((moo_uint64_t)c[6] << 8)  |
		((moo_uint64_t)c[7] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* --------------------------------------------------------------- */

#if defined(MOO_HAVE_UINT128_T)

moo_uint128_t moo_ntoh128 (moo_uint128_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint128_t)(
		((moo_uint128_t)c[0]  << 120) | 
		((moo_uint128_t)c[1]  << 112) | 
		((moo_uint128_t)c[2]  << 104) |
		((moo_uint128_t)c[3]  << 96) |
		((moo_uint128_t)c[4]  << 88) |
		((moo_uint128_t)c[5]  << 80) |
		((moo_uint128_t)c[6]  << 72) |
		((moo_uint128_t)c[7]  << 64) |
		((moo_uint128_t)c[8]  << 56) | 
		((moo_uint128_t)c[9]  << 48) | 
		((moo_uint128_t)c[10] << 40) |
		((moo_uint128_t)c[11] << 32) |
		((moo_uint128_t)c[12] << 24) |
		((moo_uint128_t)c[13] << 16) |
		((moo_uint128_t)c[14] << 8)  |
		((moo_uint128_t)c[15] << 0));
#else
#	error Unknown endian
#endif
}

moo_uint128_t moo_hton128 (moo_uint128_t x)
{
#if defined(MOO_ENDIAN_BIG)
	return x;
#elif defined(MOO_ENDIAN_LITTLE)
	moo_uint8_t* c = (moo_uint8_t*)&x;
	return (moo_uint128_t)(
		((moo_uint128_t)c[0]  << 120) | 
		((moo_uint128_t)c[1]  << 112) | 
		((moo_uint128_t)c[2]  << 104) |
		((moo_uint128_t)c[3]  << 96) |
		((moo_uint128_t)c[4]  << 88) |
		((moo_uint128_t)c[5]  << 80) |
		((moo_uint128_t)c[6]  << 72) |
		((moo_uint128_t)c[7]  << 64) |
		((moo_uint128_t)c[8]  << 56) | 
		((moo_uint128_t)c[9]  << 48) | 
		((moo_uint128_t)c[10] << 40) |
		((moo_uint128_t)c[11] << 32) |
		((moo_uint128_t)c[12] << 24) |
		((moo_uint128_t)c[13] << 16) |
		((moo_uint128_t)c[14] << 8)  |
		((moo_uint128_t)c[15] << 0));
#else
#	error Unknown endian
#endif
}

#endif

/* ----------------------------------------------------------------------- */


/* some naming conventions
 *  bchars, uchars -> pointer and length
 *  bcstr, ucstr -> null-terminated string pointer
 *  btouchars -> bchars to uchars
 *  utobchars -> uchars to bchars
 *  btoucstr -> bcstr to ucstr
 *  utobcstr -> ucstr to bcstr
 */


#if MOO_SIZEOF_OOW_T == 4
#	define FNV_MAGIC_INIT (0x811c9dc5)
#	define FNV_MAGIC_PRIME (0x01000193)
#elif MOO_SIZEOF_OOW_T == 8
#	define FNV_MAGIC_INIT (0xCBF29CE484222325)
#	define FNV_MAGIC_PRIME (0x100000001B3)
#elif MOO_SIZEOF_OOW_T == 16
#	define FNV_MAGIC_INIT (0x6C62272E07BB014262B821756295C58D)
#	define FNV_MAGIC_PRIME (0x1000000000000000000013B)
#endif

moo_oow_t moo_hash_bytes (const moo_oob_t* ptr, moo_oow_t len)
{
	moo_oow_t h;
	const moo_uint8_t* bp, * be;

	bp = ptr; be = bp + len;

	/* this hash doesn't produce good distribution */
	/* 
	h = 0
	while (bp < be) h = h * 31 + *bp++;
	*/

#if defined(FNV_MAGIC_INIT)
	/* FNV-1 hash */
	h = FNV_MAGIC_INIT;
	while (bp < be)
	{
		 h ^= (moo_oow_t)(*bp++);
		 h *= FNV_MAGIC_PRIME;
	}
#else
	/* SDBM hash is known to produce good overall distribution
	 * for many different data sets */
	h = 0;
	while (bp < be) h = (h << 6) + (h << 16) - h + *bp++;
#endif

	/* constrain the hash value to be representable in a small integer
	 * for convenience sake */
	return h % ((moo_oow_t)MOO_SMOOI_MAX + 1);
}

int moo_equal_uchars (const moo_uch_t* str1, const moo_uch_t* str2, moo_oow_t len)
{
	moo_oow_t i;

	/* NOTE: you should call this function after having ensured that
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

	/* NOTE: you should call this function after having ensured that
	 *       str1 and str2 are in the same length */

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

moo_oow_t moo_count_bcstr (const moo_bch_t* str)
{
	const moo_bch_t* ptr = str;
	while (*ptr != '\0') ptr++;
	return ptr - str;
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

MOO_INLINE int moo_conv_bcs_to_ucs_with_cmgr (
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

MOO_INLINE int moo_conv_ucs_to_bcs_with_cmgr (
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

static moo_cmgr_t utf8_cmgr =
{
	moo_utf8_to_uc,
	moo_uc_to_utf8
};

moo_cmgr_t* moo_get_utf8_cmgr (void)
{
	return &utf8_cmgr;
}

int moo_conv_utf8_to_uchars (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* the source is length bound */
	return moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, &utf8_cmgr, 0);
}

int moo_conv_uchars_to_utf8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	return moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, &utf8_cmgr);
}

int moo_conv_utf8_to_ucstr (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	return moo_conv_bcs_to_ucs_with_cmgr(bcs, bcslen, ucs, ucslen, &utf8_cmgr, 0);
}

int moo_conv_ucstr_to_utf8 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	return moo_conv_ucs_to_bcs_with_cmgr(ucs, ucslen, bcs, bcslen, &utf8_cmgr);
}

/* ----------------------------------------------------------------------- */

static moo_cmgr_t utf16_cmgr =
{
	moo_utf16_to_uc,
	moo_uc_to_utf16
};

moo_cmgr_t* moo_get_utf16_cmgr (void)
{
	return &utf16_cmgr;
}

int moo_conv_utf16_to_uchars (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* the source is length bound */
	return moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, &utf16_cmgr, 0);
}

int moo_conv_uchars_to_utf16 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	return moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, &utf16_cmgr);
}

int moo_conv_utf16_to_ucstr (const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	return moo_conv_bcs_to_ucs_with_cmgr(bcs, bcslen, ucs, ucslen, &utf16_cmgr, 0);
}

int moo_conv_ucstr_to_utf16 (const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	return moo_conv_ucs_to_bcs_with_cmgr(ucs, ucslen, bcs, bcslen, &utf16_cmgr);
}

/* ----------------------------------------------------------------------- */

int moo_convbtouchars (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* length bound */
	int n;

	n = moo_conv_bchars_to_uchars_with_cmgr(bcs, bcslen, ucs, ucslen, moo->cmgr, 0);

	if (n <= -1)
	{
		/* -1: illegal character, -2: buffer too small, -3: incomplete sequence */
		moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	}

	return n;
}

int moo_convutobchars (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* length bound */
	int n;

	n = moo_conv_uchars_to_bchars_with_cmgr(ucs, ucslen, bcs, bcslen, moo->cmgr);

	if (n <= -1)
	{
		moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	}

	return n;
}

int moo_convbtoucstr (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* bcslen, moo_uch_t* ucs, moo_oow_t* ucslen)
{
	/* null-terminated. */
	int n;

	n = moo_conv_bcs_to_ucs_with_cmgr(bcs, bcslen, ucs, ucslen, moo->cmgr, 0);

	if (n <= -1)
	{
		moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	}

	return n;
}

int moo_convutobcstr (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* ucslen, moo_bch_t* bcs, moo_oow_t* bcslen)
{
	/* null-terminated */
	int n;

	n = moo_conv_ucs_to_bcs_with_cmgr(ucs, ucslen, bcs, bcslen, moo->cmgr);

	if (n <= -1)
	{
		moo_seterrnum (moo, (n == -2)? MOO_EBUFFULL: MOO_EECERR);
	}

	return n;
}

/* ----------------------------------------------------------------------- */

MOO_INLINE moo_uch_t* moo_dupbtoucharswithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_bch_t* bcs, moo_oow_t bcslen, moo_oow_t* ucslen)
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

moo_uch_t* moo_dupbtouchars (moo_t* moo, const moo_bch_t* bcs, moo_oow_t bcslen, moo_oow_t* ucslen)
{
	return moo_dupbtoucharswithheadroom (moo, 0, bcs, bcslen, ucslen);
}

MOO_INLINE moo_bch_t* moo_duputobcharswithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_uch_t* ucs, moo_oow_t ucslen, moo_oow_t* bcslen)
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

moo_bch_t* moo_duputobchars (moo_t* moo, const moo_uch_t* ucs, moo_oow_t ucslen, moo_oow_t* bcslen)
{
	return moo_duputobcharswithheadroom (moo, 0, ucs, ucslen, bcslen);
}


/* ----------------------------------------------------------------------- */

MOO_INLINE moo_uch_t* moo_dupbtoucstrwithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_bch_t* bcs, moo_oow_t* ucslen)
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

moo_uch_t* moo_dupbtoucstr (moo_t* moo, const moo_bch_t* bcs, moo_oow_t* ucslen)
{
	return moo_dupbtoucstrwithheadroom (moo, 0, bcs, ucslen);
}

MOO_INLINE moo_bch_t* moo_duputobcstrwithheadroom (moo_t* moo, moo_oow_t headroom_bytes, const moo_uch_t* ucs, moo_oow_t* bcslen)
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

moo_bch_t* moo_duputobcstr (moo_t* moo, const moo_uch_t* ucs, moo_oow_t* bcslen)
{
	return moo_duputobcstrwithheadroom (moo, 0, ucs, bcslen);
}
/* ----------------------------------------------------------------------- */

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
