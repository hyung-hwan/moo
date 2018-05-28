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
	moo_uint32_t  lower;
	moo_uint32_t  upper;
	moo_uint8_t   fbyte;  /* mask to the first utf8 byte */
	moo_uint8_t   mask;
	moo_uint8_t   fmask;
	int           length; /* number of bytes */
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

static MOO_INLINE __utf8_t* get_utf8_slot (moo_uch_t uc)
{
	__utf8_t* cur, * end;

	/*MOO_ASSERT (moo, MOO_SIZEOF(moo_bch_t) == 1);
	MOO_ASSERT (moo, MOO_SIZEOF(moo_uch_t) >= 2);*/

	end = utf8_table + MOO_COUNTOF(utf8_table);
	cur = utf8_table;

	while (cur < end) 
	{
		if (uc >= cur->lower && uc <= cur->upper) return cur;
		cur++;
	}

	return MOO_NULL; /* invalid character */
}

moo_oow_t moo_uc_to_utf8 (moo_uch_t uc, moo_bch_t* utf8, moo_oow_t size)
{
	__utf8_t* cur = get_utf8_slot (uc);

	if (cur == MOO_NULL) return 0; /* illegal character */

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
	return (moo_oow_t)cur->length;
}

moo_oow_t moo_utf8_to_uc (const moo_bch_t* utf8, moo_oow_t size, moo_uch_t* uc)
{
	__utf8_t* cur, * end;

	/*MOO_ASSERT (moo, utf8 != MOO_NULL);
	MOO_ASSERT (moo, size > 0);
	MOO_ASSERT (moo, MOO_SIZEOF(moo_bch_t) == 1);
	MOO_ASSERT (moo, MOO_SIZEOF(moo_uch_t) >= 2);*/

	end = utf8_table + MOO_COUNTOF(utf8_table);
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
					moo_uch_t w;

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
			 *    the correct length (size >= cur->length) 
			 * and 
			 *    the incomplete seqeunce error (size < cur->length).
			 */
			return (moo_oow_t)cur->length;
		}
		cur++;
	}

	return 0; /* error - invalid sequence */
}


/*
 * See http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c 
 */
struct interval 
{
	int first;
	int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(moo_uch_t ucs, const struct interval *table, int max) 
{
	int min = 0;
	int mid;

	if (ucs < table[0].first || ucs > table[max].last) return 0;
	while (max >= min)
	{
		mid = (min + max) / 2;
		if (ucs > table[mid].last) min = mid + 1;
		else if (ucs < table[mid].first) max = mid - 1;
		else return 1;
	}

	return 0;
}

/* The following two functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - SOFT HYPHEN (U+00AD) has a column width of 1.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      Full-width (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

int moo_ucwidth (moo_uch_t uc)
{
	/* sorted list of non-overlapping intervals of non-spacing characters */
	/* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
	static const struct interval combining[] = {
		{ 0x0300, 0x036F }, { 0x0483, 0x0486 }, { 0x0488, 0x0489 },
		{ 0x0591, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
		{ 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 }, { 0x0600, 0x0603 },
		{ 0x0610, 0x0615 }, { 0x064B, 0x065E }, { 0x0670, 0x0670 },
		{ 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
		{ 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
		{ 0x07A6, 0x07B0 }, { 0x07EB, 0x07F3 }, { 0x0901, 0x0902 },
		{ 0x093C, 0x093C }, { 0x0941, 0x0948 }, { 0x094D, 0x094D },
		{ 0x0951, 0x0954 }, { 0x0962, 0x0963 }, { 0x0981, 0x0981 },
		{ 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD },
		{ 0x09E2, 0x09E3 }, { 0x0A01, 0x0A02 }, { 0x0A3C, 0x0A3C },
		{ 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D },
		{ 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC },
		{ 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD },
		{ 0x0AE2, 0x0AE3 }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
		{ 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
		{ 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
		{ 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
		{ 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBC, 0x0CBC },
		{ 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
		{ 0x0CE2, 0x0CE3 }, { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D },
		{ 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 },
		{ 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E },
		{ 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC },
		{ 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 },
		{ 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E },
		{ 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 },
		{ 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 },
		{ 0x1032, 0x1032 }, { 0x1036, 0x1037 }, { 0x1039, 0x1039 },
		{ 0x1058, 0x1059 }, { 0x1160, 0x11FF }, { 0x135F, 0x135F },
		{ 0x1712, 0x1714 }, { 0x1732, 0x1734 }, { 0x1752, 0x1753 },
		{ 0x1772, 0x1773 }, { 0x17B4, 0x17B5 }, { 0x17B7, 0x17BD },
		{ 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x17DD, 0x17DD },
		{ 0x180B, 0x180D }, { 0x18A9, 0x18A9 }, { 0x1920, 0x1922 },
		{ 0x1927, 0x1928 }, { 0x1932, 0x1932 }, { 0x1939, 0x193B },
		{ 0x1A17, 0x1A18 }, { 0x1B00, 0x1B03 }, { 0x1B34, 0x1B34 },
		{ 0x1B36, 0x1B3A }, { 0x1B3C, 0x1B3C }, { 0x1B42, 0x1B42 },
		{ 0x1B6B, 0x1B73 }, { 0x1DC0, 0x1DCA }, { 0x1DFE, 0x1DFF },
		{ 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x2063 },
		{ 0x206A, 0x206F }, { 0x20D0, 0x20EF }, { 0x302A, 0x302F },
		{ 0x3099, 0x309A }, { 0xA806, 0xA806 }, { 0xA80B, 0xA80B },
		{ 0xA825, 0xA826 }, { 0xFB1E, 0xFB1E }, { 0xFE00, 0xFE0F },
		{ 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF }, { 0xFFF9, 0xFFFB },
		{ 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
		{ 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
		{ 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
		{ 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
		{ 0xE0100, 0xE01EF }
	};

	/* test for 8-bit control characters */
	if (uc == 0) return 0;
	if (uc < 32 || (uc >= 0x7f && uc < 0xa0)) return -1;

	/* binary search in table of non-spacing characters */
	if (bisearch(uc, combining, sizeof(combining) / sizeof(struct interval) - 1)) return 0;

	/* if we arrive here, uc is not a combining or C0/C1 control character */

	if (uc >= 0x1100)
	{
		if (uc <= 0x115f || /* Hangul Jamo init. consonants */
		    uc == 0x2329 || uc == 0x232a ||
		    (uc >= 0x2e80 && uc <= 0xa4cf && uc != 0x303f) || /* CJK ... Yi */
		    (uc >= 0xac00 && uc <= 0xd7a3) || /* Hangul Syllables */
		    (uc >= 0xf900 && uc <= 0xfaff) || /* CJK Compatibility Ideographs */
		    (uc >= 0xfe10 && uc <= 0xfe19) || /* Vertical forms */
		    (uc >= 0xfe30 && uc <= 0xfe6f) || /* CJK Compatibility Forms */
		    (uc >= 0xff00 && uc <= 0xff60) || /* Fullwidth Forms */
		    (uc >= 0xffe0 && uc <= 0xffe6)
		#if (MOO_SIZEOF_UCH_T  > 2)
		    || 
		    (uc >= 0x20000 && uc <= 0x2fffd) ||
		    (uc >= 0x30000 && uc <= 0x3fffd)
		#endif
		   )
		{
			return 2;
		}
	}

	return 1; 
}
