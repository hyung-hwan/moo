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

/*#include <stdio.h>*/ /* for snrintf(). used for floating-point number formatting */

#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf 
#	if !defined(HAVE_SNPRINTF)
#		define HAVE_SNPRINTF
#	endif
#endif
#if defined(HAVE_QUADMATH_H)
#	include <quadmath.h> /* for quadmath_snprintf() */
#endif
/* TODO: remove stdio.h and quadmath.h once snprintf gets replaced by own 
floting-point conversion implementation*/

/* Max number conversion buffer length: 
 * moo_intmax_t in base 2, plus NUL byte. */
#define MAXNBUF (MOO_SIZEOF(moo_intmax_t) * 8 + 1)

enum
{
	/* integer */
	LF_C = (1 << 0),
	LF_H = (1 << 1),
	LF_J = (1 << 2),
	LF_L = (1 << 3),
	LF_Q = (1 << 4),
	LF_T = (1 << 5),
	LF_Z = (1 << 6),

	/* long double */
	LF_LD = (1 << 7),
	/* __float128 */
	LF_QD = (1 << 8)
};

static struct
{
	moo_uint8_t flag; /* for single occurrence */
	moo_uint8_t dflag; /* for double occurrence */
} lm_tab[26] = 
{
	{ 0,    0 }, /* a */
	{ 0,    0 }, /* b */
	{ 0,    0 }, /* c */
	{ 0,    0 }, /* d */
	{ 0,    0 }, /* e */
	{ 0,    0 }, /* f */
	{ 0,    0 }, /* g */
	{ LF_H, LF_C }, /* h */
	{ 0,    0 }, /* i */
	{ LF_J, 0 }, /* j */
	{ 0,    0 }, /* k */
	{ LF_L, LF_Q }, /* l */
	{ 0,    0 }, /* m */
	{ 0,    0 }, /* n */
	{ 0,    0 }, /* o */
	{ 0,    0 }, /* p */
	{ LF_Q, 0 }, /* q */
	{ 0,    0 }, /* r */
	{ 0,    0 }, /* s */
	{ LF_T, 0 }, /* t */
	{ 0,    0 }, /* u */
	{ 0,    0 }, /* v */
	{ 0,    0 }, /* w */
	{ 0,    0 }, /* z */
	{ 0,    0 }, /* y */
	{ LF_Z, 0 }, /* z */
};


enum 
{
	FLAGC_DOT       = (1 << 0),
	FLAGC_SPACE     = (1 << 1),
	FLAGC_SHARP     = (1 << 2),
	FLAGC_SIGN      = (1 << 3),
	FLAGC_LEFTADJ   = (1 << 4),
	FLAGC_ZEROPAD   = (1 << 5),
	FLAGC_WIDTH     = (1 << 6),
	FLAGC_PRECISION = (1 << 7),
	FLAGC_STAR1     = (1 << 8),
	FLAGC_STAR2     = (1 << 9),
	FLAGC_LENMOD    = (1 << 10) /* length modifier */
};

static const moo_bch_t hex2ascii_lower[] = 
{
	'0','1','2','3','4','5','6','7','8','9',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z'
};

static const moo_bch_t hex2ascii_upper[] = 
{
	'0','1','2','3','4','5','6','7','8','9',
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','H','Z'
};

static moo_uch_t uch_nullstr[] = { '(','n','u','l','l', ')','\0' };
static moo_bch_t bch_nullstr[] = { '(','n','u','l','l', ')','\0' };

typedef int (*moo_fmtout_putch_t) (
	moo_t*          moo,
	moo_bitmask_t   mask,
	moo_ooch_t      c,
	moo_oow_t       len
);

typedef int (*moo_fmtout_putcs_t) (
	moo_t*            moo,
	moo_bitmask_t     mask,
	const moo_ooch_t* ptr,
	moo_oow_t         len
);

typedef struct moo_fmtout_data_t moo_fmtout_data_t;
struct moo_fmtout_data_t
{
	moo_oow_t            count; /* out */
	moo_bitmask_t        mask;  /* in */
	moo_fmtout_putch_t   putch; /* in */
	moo_fmtout_putcs_t   putcs; /* in */
};


typedef moo_ooi_t (*moo_outbfmt_t) (
	moo_t*             moo,
	moo_bitmask_t      mask,
	const moo_bch_t*   fmt,
	...
);
/* ------------------------------------------------------------------------- */
/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */

static moo_bch_t* sprintn_lower (moo_bch_t* nbuf, moo_uintmax_t num, int base, moo_ooi_t* lenp)
{
	moo_bch_t* p;

	p = nbuf;
	*p = '\0';
	do { *++p = hex2ascii_lower[num % base]; } while (num /= base);

	if (lenp) *lenp = p - nbuf;
	return p; /* returns the end */
}

static moo_bch_t* sprintn_upper (moo_bch_t* nbuf, moo_uintmax_t num, int base, moo_ooi_t* lenp)
{
	moo_bch_t* p;

	p = nbuf;
	*p = '\0';
	do { *++p = hex2ascii_upper[num % base]; } while (num /= base);

	if (lenp) *lenp = p - nbuf;
	return p; /* returns the end */
}

/* ------------------------------------------------------------------------- */
static int put_ooch (moo_t* moo, moo_bitmask_t mask, moo_ooch_t ch, moo_oow_t len)
{
	/* this is not equivalent to put_oocs(moo,mask,&ch, 1);
	 * this function is to emit a single character multiple times */
	moo_oow_t rem;

	if (len <= 0) return 1;

	if (moo->log.len > 0 && moo->log.last_mask != mask)
	{
		/* the mask has changed. commit the buffered text */

/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
		if (moo->log.ptr[moo->log.len - 1] != '\n')
		{
			/* no line ending - append a line terminator */
			moo->log.ptr[moo->log.len++] = '\n';
		}

		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

redo:
	rem = 0;
	if (len > moo->log.capa - moo->log.len)
	{
		moo_oow_t newcapa, max;
		moo_ooch_t* tmp;

		max = MOO_TYPE_MAX(moo_oow_t) - moo->log.len;
		if (len > max)
		{
			/* data too big. */
			rem += len - max;
			len = max;
		}

		newcapa = MOO_ALIGN_POW2(moo->log.len + len, MOO_LOG_CAPA_ALIGN); /* TODO: adjust this capacity */
		if (newcapa > moo->option.log_maxcapa) 
		{
			/* [NOTE]
			 * it doesn't adjust newcapa to moo->option.log_maxcapa.
			 * nor does it cut the input to fit it into the adjusted capacity.
			 * if maxcapa set is not aligned to MOO_LOG_CAPA_ALIGN,
			 * the largest buffer capacity may be suboptimal */
			goto make_do;
		}

		/* +1 to handle line ending injection more easily */
		tmp = moo_reallocmem(moo, moo->log.ptr, (newcapa + 1) * MOO_SIZEOF(*tmp)); 
		if (!tmp) 
		{
		make_do:
			if (moo->log.len > 0)
			{
				/* can't expand the buffer. just flush the existing contents */
				/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
				if (moo->log.ptr[moo->log.len - 1] != '\n')
				{
					/* no line ending - append a line terminator */
					moo->log.ptr[moo->log.len++] = '\n';
				}

				vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
				moo->log.len = 0;
			}

			if (len > moo->log.capa)
			{
				rem += len - moo->log.capa;
				len = moo->log.capa;
			}

		}
		else
		{
			moo->log.ptr = tmp;
			moo->log.capa = newcapa; 
		}
	}

	while (len > 0)
	{
		moo->log.ptr[moo->log.len++] = ch;
		len--;
	}
	moo->log.last_mask = mask;

	if (rem > 0)
	{
		len = rem;
		goto redo;
	}

	return 1; /* success */
}

static int put_oocs (moo_t* moo, moo_bitmask_t mask, const moo_ooch_t* ptr, moo_oow_t len)
{
	moo_oow_t rem;

	if (len <= 0) return 1;

	if (moo->log.len > 0 && moo->log.last_mask != mask)
	{
		/* the mask has changed. commit the buffered text */
/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
		if (moo->log.ptr[moo->log.len - 1] != '\n')
		{
			/* no line ending - append a line terminator */
			moo->log.ptr[moo->log.len++] = '\n';
		}

		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

redo:
	rem = 0;
	if (len > moo->log.capa - moo->log.len)
	{
		moo_oow_t newcapa, max;
		moo_ooch_t* tmp;

		max = MOO_TYPE_MAX(moo_oow_t) - moo->log.len;
		if (len > max) 
		{
			/* data too big. */
			rem += len - max;
			len = max;
		}

		newcapa = MOO_ALIGN_POW2(moo->log.len + len, 512); /* TODO: adjust this capacity */
		if (newcapa > moo->option.log_maxcapa)
		{
			/* [NOTE]
			 * it doesn't adjust newcapa to moo->option.log_maxcapa.
			 * nor does it cut the input to fit it into the adjusted capacity.
			 * if maxcapa set is not aligned to MOO_LOG_CAPA_ALIGN,
			 * the largest buffer capacity may be suboptimal */
			goto make_do;
		}

		/* +1 to handle line ending injection more easily */
		tmp = moo_reallocmem(moo, moo->log.ptr, (newcapa + 1) * MOO_SIZEOF(*tmp));
		if (!tmp) 
		{
		make_do:
			if (moo->log.len > 0)
			{
				/* can't expand the buffer. just flush the existing contents */
				/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
				if (moo->log.ptr[moo->log.len - 1] != '\n')
				{
					/* no line ending - append a line terminator */
					moo->log.ptr[moo->log.len++] = '\n';
				}
				vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
				moo->log.len = 0;
			}

			if (len > moo->log.capa)
			{
				rem += len - moo->log.capa;
				len = moo->log.capa;
			}
		}
		else
		{
			moo->log.ptr = tmp;
			moo->log.capa = newcapa;
		}
	}

	MOO_MEMCPY (&moo->log.ptr[moo->log.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->log.len += len;
	moo->log.last_mask = mask;

	if (rem > 0)
	{
		ptr += len;
		len = rem;
		goto redo;
	}

	return 1; /* success */
}

/* ------------------------------------------------------------------------- */

static int print_object (moo_t* moo, moo_bitmask_t mask, moo_oop_t oop, moo_outbfmt_t outbfmt)
{
	if (oop == moo->_nil)
	{
		if (outbfmt(moo, mask, "nil") <= -1) return -1;
	}
	else if (oop == moo->_true)
	{
		if (outbfmt(moo, mask, "true") <= -1) return -1;
	}
	else if (oop == moo->_false)
	{
		if (outbfmt(moo, mask, "false") <= -1) return -1;
	}
	else if (MOO_OOP_IS_SMOOI(oop))
	{
		if (outbfmt(moo, mask, "%zd", MOO_OOP_TO_SMOOI(oop)) <= -1) return -1;
	}
	else if (MOO_OOP_IS_SMPTR(oop))
	{
		if (outbfmt(moo, mask, "#\\p%X", MOO_OOP_TO_SMPTR(oop)) <= -1) return -1;
	}
	else if (MOO_OOP_IS_CHAR(oop))
	{
		if (outbfmt(moo, mask, "$%.1jc", MOO_OOP_TO_CHAR(oop)) <= -1) return -1;
	}
	else if (MOO_OOP_IS_ERROR(oop))
	{
		if (outbfmt(moo, mask, "#\\e%zd", MOO_OOP_TO_ERROR(oop)) <= -1) return -1;
	}
	else
	{
		moo_oop_class_t c;
		moo_oow_t i;

		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
		c = (moo_oop_class_t)MOO_OBJ_GET_CLASS(oop); /*MOO_CLASSOF(moo, oop);*/

		if (c == moo->_large_negative_integer || c == moo->_large_positive_integer)
		{
			if (!moo_inttostr(moo, oop, 10 | MOO_INTTOSTR_NONEWOBJ)) return -1;
			if (outbfmt(moo, mask, "%.*js", moo->inttostr.xbuf.len, moo->inttostr.xbuf.ptr) <= -1) return -1;
		}
		else if (c == moo->_fixed_point_decimal)
		{
#if 1
			if (!moo_numtostr(moo, oop, 10 | MOO_NUMTOSTR_NONEWOBJ)) return -1;
			if (outbfmt(moo, mask, "%.*js", moo->inttostr.xbuf.len, moo->inttostr.xbuf.ptr) <= -1) return -1;
#else
			moo_ooch_t* ptr;
			moo_oow_t len;
			moo_ooi_t scale;

			if (!moo_inttostr(moo, ((moo_oop_fpdec_t)oop)->value, 10 | MOO_INTTOSTR_NONEWOBJ)) return -1;

			ptr = moo->inttostr.xbuf.ptr;
			len = moo->inttostr.xbuf.len;
			if (ptr[0] == '-')
			{
				if (outbfmt(moo, mask, "-") <= -1) return -1; 
				ptr++;
				len--;
			}

			scale = MOO_OOP_TO_SMOOI(((moo_oop_fpdec_t)oop)->scale);
			if (scale >= len)
			{
				moo_oow_t i;

				if (outbfmt(moo, mask, "0.") <= -1) return -1; 
				for (i = len; i < scale; i++)
				{
					if (outbfmt(moo, mask, "0") <= -1) return -1; 
				}

				if (outbfmt(moo, mask, "%.*js", len, ptr) <= -1) return -1; 
			}
			else
			{
				if (outbfmt(moo, mask, "%.*js.%.*js", len - scale, &ptr[0], scale, &ptr[len - scale]) <= -1) return -1;
			}
#endif
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_CHAR)
		{
			if (c == moo->_symbol) 
			{
				if (outbfmt(moo, mask, "#%.*js", MOO_OBJ_GET_SIZE(oop), MOO_OBJ_GET_CHAR_SLOT(oop)) <= -1) return -1;
			}
			else /*if ((moo_oop_t)c == moo->_string)*/
			{
				moo_ooch_t ch;
				int escape = 0;

				for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
				{
					ch = MOO_OBJ_GET_CHAR_SLOT(oop)[i];
					if (ch < ' ') 
					{
						escape = 1;
						break;
					}
				}

				if (escape)
				{
					moo_ooch_t escaped;

					if (outbfmt(moo, mask, "S'") <= -1) return -1;
					for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
					{
						ch = MOO_OBJ_GET_CHAR_SLOT(oop)[i];
						if (ch < ' ') 
						{
							switch (ch)
							{
								case '\0':
									escaped = '0';
									break;
								case '\n':
									escaped = 'n';
									break;
								case '\r':
									escaped = 'r';
									break;
								case '\t':
									escaped = 't';
									break;
								case '\f':
									escaped = 'f';
									break;
								case '\b':
									escaped = 'b';
									break;
								case '\v':
									escaped = 'v';
									break;
								case '\a':
									escaped = 'a';
									break;
								default:
									escaped = ch;
									break;
							}

							if (escaped == ch)
							{
								if (outbfmt(moo, mask, "\\x%X", ch) <= -1) return -1;
							}
							else
							{
								if (outbfmt(moo, mask, "\\%jc", escaped) <= -1) return -1;
							}
						}
						else
						{
							if (outbfmt(moo, mask, "%jc", ch) <= -1) return -1;
						}
					}

					if (outbfmt(moo, mask, "'") <= -1) return -1;
				}
				else
				{
					if (outbfmt(moo, mask, "'%.*js'", MOO_OBJ_GET_SIZE(oop), MOO_OBJ_GET_CHAR_SLOT(oop)) <= -1) return -1;
				}
			}
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_BYTE)
		{
			if (outbfmt(moo, mask, "#[") <= -1) return -1;
			i = 0;
			if (i < MOO_OBJ_GET_SIZE(oop))
			{
				if (outbfmt(moo, mask, "%d", MOO_OBJ_GET_BYTE_SLOT(oop)[i]) <= -1) return -1;
				for (++i; i < MOO_OBJ_GET_SIZE(oop); i++)
				{
					if (outbfmt(moo, mask, " %d", MOO_OBJ_GET_BYTE_SLOT(oop)[i]) <= -1) return -1;
				}
			}
			if (outbfmt(moo, mask, "]") <= -1) return -1;
		}
		
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_HALFWORD)
		{
			if (outbfmt(moo, mask, "#[[") <= -1) return -1;; /* TODO: fix this symbol/notation */
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				if (outbfmt(moo, mask, " %zX", (moo_oow_t)(MOO_OBJ_GET_HALFWORD_SLOT(oop)[i])) <= -1) return -1;
			}
			outbfmt (moo, mask, "]]");
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_WORD)
		{
			if (outbfmt(moo, mask, "#[[[") <= -1) return -1;; /* TODO: fix this symbol/notation */
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				if (outbfmt(moo, mask, " %zX", MOO_OBJ_GET_WORD_SLOT(oop)[i]) <= -1) return -1;
			}
			if (outbfmt(moo, mask, "]]]") <= -1) return -1;
		}
		else if (c == moo->_array)
		{
			if (outbfmt(moo, mask, "#(") <= -1) return -1;
			i = 0;
			if (i < MOO_OBJ_GET_SIZE(oop))
			{
				if (print_object(moo, mask, MOO_OBJ_GET_OOP_VAL(oop, i), outbfmt) <= -1) return -1;
				for (++i; i < MOO_OBJ_GET_SIZE(oop); i++)
				{
					if (outbfmt(moo, mask, " ") <= -1) return -1;
					if (print_object(moo, mask, MOO_OBJ_GET_OOP_VAL(oop, i), outbfmt) <= -1) return -1;
				}
			}
			if (outbfmt(moo, mask, ")") <= -1) return -1;
		}
		else if (c == moo->_class)
		{
			/* print the class name */
			if (outbfmt(moo, mask, "%.*js", MOO_OBJ_GET_SIZE(((moo_oop_class_t)oop)->name), MOO_OBJ_GET_CHAR_SLOT(((moo_oop_class_t)oop)->name)) <= -1) return -1;
		}
		else if (c == moo->_association)
		{
			if (outbfmt(moo, mask, "%O -> %O", ((moo_oop_association_t)oop)->key, ((moo_oop_association_t)oop)->value) <= -1) return -1;
		}
		else
		{
			if (outbfmt(moo, mask, "<<%.*js:%p>>", MOO_OBJ_GET_SIZE(c->name), MOO_OBJ_GET_CHAR_SLOT(c->name), oop) <= -1) return -1;
		}
	}

	return 0;
}

/* ------------------------------------------------------------------------- */

#undef FMTCHAR_IS_BCH
#undef FMTCHAR_IS_UCH
#undef FMTCHAR_IS_OOCH
#undef fmtchar_t
#undef fmtoutv
#define fmtchar_t moo_bch_t
#define fmtoutv __logbfmtv
#define FMTCHAR_IS_BCH
#if defined(MOO_OOCH_IS_BCH)
#	define FMTCHAR_IS_OOCH
#endif
#include "fmtoutv.h"

#undef FMTCHAR_IS_BCH
#undef FMTCHAR_IS_UCH
#undef FMTCHAR_IS_OOCH
#undef fmtchar_t
#undef fmtoutv
#define fmtchar_t moo_uch_t
#define fmtoutv __logufmtv
#define FMTCHAR_IS_UCH
#if defined(MOO_OOCH_IS_UCH)
#	define FMTCHAR_IS_OOCH
#endif
#include "fmtoutv.h" 


static int _logbfmtv (moo_t* moo, const moo_bch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logbfmtv(moo, fmt, data, ap, moo_logbfmt);
}

static int _logufmtv (moo_t* moo, const moo_uch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logufmtv(moo, fmt, data, ap, moo_logbfmt);
}

moo_ooi_t moo_logbfmt (moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_data_t fo;

	if (moo->log.default_type_mask & MOO_LOG_ALL_TYPES) 
	{
		/* if a type is given, it's not untyped any more.
		 * mask off the UNTYPED bit */
		mask &= ~MOO_LOG_UNTYPED; 

		/* if the default_type_mask has the UNTYPED bit on,
		 * it'll get turned back on */
		mask |= (moo->log.default_type_mask & MOO_LOG_ALL_TYPES);
	}
	else if (!(mask & MOO_LOG_ALL_TYPES)) 
	{
		/* no type is set in the given mask and no default type is set.
		 * make it UNTYPED. */
		mask |= MOO_LOG_UNTYPED;
	}

	fo.mask = mask;
	fo.putch = put_ooch;
	fo.putcs = put_oocs;

	va_start (ap, fmt);
	x = _logbfmtv(moo, fmt, &fo, ap);
	va_end (ap);

	if (moo->log.len > 0 && moo->log.ptr[moo->log.len - 1] == '\n')
	{
		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}
	return (x <= -1)? -1: fo.count;
}

moo_ooi_t moo_logufmt (moo_t* moo, moo_bitmask_t mask, const moo_uch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_data_t fo;

	if (moo->log.default_type_mask & MOO_LOG_ALL_TYPES) 
	{
		mask &= ~MOO_LOG_UNTYPED;
		mask |= (moo->log.default_type_mask & MOO_LOG_ALL_TYPES);
	}
	else if (!(mask & MOO_LOG_ALL_TYPES)) 
	{
		mask |= MOO_LOG_UNTYPED;
	}

	fo.mask = mask;
	fo.putch = put_ooch;
	fo.putcs = put_oocs;

	va_start (ap, fmt);
	x = _logufmtv(moo, fmt, &fo, ap);
	va_end (ap);

	if (moo->log.len > 0 && moo->log.ptr[moo->log.len - 1] == '\n')
	{
		vmprim_log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

	return (x <= -1)? -1: fo.count;
}


/* -------------------------------------------------------------------------- 
 * ERROR MESSAGE FORMATTING
 * -------------------------------------------------------------------------- */

static int put_errch (moo_t* moo, moo_bitmask_t mask, moo_ooch_t ch, moo_oow_t len)
{
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;
	if (len > max) len = max;

	if (len <= 0) return 1;

	while (len > 0)
	{
		moo->errmsg.buf[moo->errmsg.len++] = ch;
		len--;
	}
	moo->errmsg.buf[moo->errmsg.len] = '\0';

	return 1; /* success */
}

static int put_errcs (moo_t* moo, moo_bitmask_t mask, const moo_ooch_t* ptr, moo_oow_t len)
{
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;
	if (len > max) len = max;

	if (len <= 0) return 1;

	MOO_MEMCPY (&moo->errmsg.buf[moo->errmsg.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->errmsg.len += len;
	moo->errmsg.buf[moo->errmsg.len] = '\0';

	return 1; /* success */
}


static moo_ooi_t __errbfmtv (moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...);

static int _errbfmtv (moo_t* moo, const moo_bch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logbfmtv (moo, fmt, data, ap, __errbfmtv);
}

static int _errufmtv (moo_t* moo, const moo_uch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logufmtv (moo, fmt, data, ap, __errbfmtv);
}

static moo_ooi_t __errbfmtv (moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_data_t fo;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errbfmtv (moo, fmt, &fo, ap);
	va_end (ap);

	return fo.count;
}

void moo_seterrbfmt (moo_t* moo, moo_errnum_t errnum, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_data_t fo;

	if (moo->shuterr) return;
	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errbfmtv (moo, fmt, &fo, ap);
	va_end (ap);

	moo->errnum = errnum;
}

void moo_seterrufmt (moo_t* moo, moo_errnum_t errnum, const moo_uch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_data_t fo;

	if (moo->shuterr) return;
	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errufmtv (moo, fmt, &fo, ap);
	va_end (ap);

	moo->errnum = errnum;
}


void moo_seterrbfmtv (moo_t* moo, moo_errnum_t errnum, const moo_bch_t* fmt, va_list ap)
{
	moo_fmtout_data_t fo;

	if (moo->shuterr) return;

	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	_errbfmtv (moo, fmt, &fo, ap);
	moo->errnum = errnum;
}

void moo_seterrufmtv (moo_t* moo, moo_errnum_t errnum, const moo_uch_t* fmt, va_list ap)
{
	moo_fmtout_data_t fo;

	if (moo->shuterr) return;

	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	_errufmtv (moo, fmt, &fo, ap);
	moo->errnum = errnum;
}


/* -------------------------------------------------------------------------- 
 * SUPPORT FOR FORMATTED OUTPUT TO BE USED BY BUILTIN PRIMITIVE FUNCTIONS
 * -------------------------------------------------------------------------- */

#define PRINT_OOCH(c,n) do { \
	if (n > 0) { \
		int xx; \
		if ((xx = data->putch(moo, data->mask, c, n)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		data->count += n; \
	} \
} while (0)

#define PRINT_OOCS(ptr,len) do { \
	if (len > 0) { \
		int xx; \
		if ((xx = data->putcs(moo, data->mask, ptr, len)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		data->count += len; \
	} \
} while(0)

#define GET_NEXT_ARG_TO(moo,nargs,arg_state,arg) do { \
	if ((arg_state)->idx >= nargs) { (arg_state)->stop = 1;  goto invalid_format; } \
	arg = MOO_STACK_GETARG(moo, nargs, (arg_state)->idx); \
	(arg_state)->idx++; \
} while(0)

#define GET_NEXT_CHAR_TO(moo,fmt,fmtend,ch) do { \
	if (fmt >= fmtend) ch = MOO_OOCI_EOF; \
	else { ch = *(fmt); (fmt)++; }\
} while(0)

static MOO_INLINE int print_formatted (moo_t* moo, moo_ooi_t nargs, moo_fmtout_data_t* data, moo_outbfmt_t outbfmt, int ignore_rcv)
{
	const moo_ooch_t* fmt, * fmtend;
	const moo_ooch_t* checkpoint, * percent;

	int n, radix, neg, sign, radix_flags;
	moo_ooi_t extra, width, precision;
	moo_ooch_t padc, ooch;
	moo_ooci_t ch;
	int flagc;

	struct 
	{
		moo_ooi_t idx;
		int stop;
	} arg_state;
	moo_oop_t arg;

	if (ignore_rcv)
	{
		arg = MOO_STACK_GETARG(moo, nargs, 0);
		arg_state.idx = 1;
	}
	else
	{
		arg = MOO_STACK_GETRCV(moo, nargs);
		arg_state.idx = 0;
	}

	if (!MOO_OOP_IS_POINTER(arg) || MOO_OBJ_GET_FLAGS_TYPE(arg) != MOO_OBJ_TYPE_CHAR)
	{
		moo_ooi_t i;
		/* if the first argument is not a valid formatting string, 
		 * print all arguments as objects */
		if (print_object(moo, data->mask, arg, outbfmt) <= -1) goto oops;
		for (i = arg_state.idx; i < nargs; i++)
		{
			arg = MOO_STACK_GETARG(moo, nargs, i);
			if (print_object(moo, data->mask, arg, outbfmt) <= -1) goto oops;
		}
		return 0;
	}

	arg_state.stop = 0;

	fmt = MOO_OBJ_GET_CHAR_SLOT(arg);
	fmtend = fmt + MOO_OBJ_GET_SIZE(arg);

	data->count = 0;

	while (1)
	{
		checkpoint = fmt;

		while (1)
		{
			GET_NEXT_CHAR_TO (moo, fmt, fmtend, ch);
			if (ch == '%' && !arg_state.stop) break;

			if (ch == MOO_OOCI_EOF) 
			{
				/* fmt is not advanced when it is length-bounded.
				 * so not fmt - checkpoint - 1 */
				PRINT_OOCS (checkpoint, fmt - checkpoint); 
				goto done;
			}
		}
		PRINT_OOCS (checkpoint, fmt - checkpoint - 1);

		percent = fmt - 1;

		padc = ' '; 
		width = 0; precision = 0;
		neg = 0; sign = 0;

		flagc = 0; 
		radix_flags = MOO_INTTOSTR_NONEWOBJ;

	reswitch:
		GET_NEXT_CHAR_TO (moo, fmt, fmtend, ch);
		switch (ch) 
		{
		case '%': /* %% */
			ooch = ch;
			goto print_char;

		/* flag characters */
		case '.':
			if (flagc & FLAGC_DOT) goto invalid_format;
			flagc |= FLAGC_DOT;
			goto reswitch;

		case '#': 
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SHARP;
			goto reswitch;

		case ' ':
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SPACE;
			goto reswitch;

		case '+': /* place sign for signed conversion */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SIGN;
			goto reswitch;

		case '-': /* left adjusted */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			if (flagc & FLAGC_DOT)
			{
				goto invalid_format;
			}
			else
			{
				flagc |= FLAGC_LEFTADJ;
				if (flagc & FLAGC_ZEROPAD)
				{
					padc = ' ';
					flagc &= ~FLAGC_ZEROPAD;
				}
			}
			
			goto reswitch;

		case '*': /* take the length from the parameter */
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & (FLAGC_STAR2 | FLAGC_PRECISION)) goto invalid_format;
				flagc |= FLAGC_STAR2;

				GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
				if (moo_inttoooi(moo, arg, &precision) <= -1) goto invalid_format;
				if (precision < 0) 
				{
					/* if precision is less than 0, 
					 * treat it as if no .precision is specified */
					flagc &= ~FLAGC_DOT;
					precision = 0;
				}
			} 
			else 
			{
				if (flagc & (FLAGC_STAR1 | FLAGC_WIDTH)) goto invalid_format;
				flagc |= FLAGC_STAR1;

				GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
				if (moo_inttoooi(moo, arg, &width) <= -1) goto invalid_format;
				if (width < 0) 
				{
					flagc |= FLAGC_LEFTADJ;
					width = -width;
				}
			}
			goto reswitch;

		case '0': /* zero pad */
			if (flagc & FLAGC_LENMOD) goto invalid_format;
			if (!(flagc & (FLAGC_DOT | FLAGC_LEFTADJ)))
			{
				padc = '0';
				flagc |= FLAGC_ZEROPAD;
				goto reswitch;
			}
		/* end of flags characters */

		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (flagc & FLAGC_LENMOD) goto invalid_format;
			for (n = 0;; ++fmt) 
			{
				n = n * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9') break;
			}
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & FLAGC_STAR2) goto invalid_format;
				precision = n;
				flagc |= FLAGC_PRECISION;
			}
			else 
			{
				if (flagc & FLAGC_STAR1) goto invalid_format;
				width = n;
				flagc |= FLAGC_WIDTH;
			}
			goto reswitch;
 
		/* integer conversions */
		case 'd':
		case 'i': /* signed conversion */
			radix = 10;
			sign = 1;
			goto print_integer;
		case 'o': 
			radix = 8;
			goto print_integer;
		case 'u':
			radix = 10;
			goto print_integer;
		case 'x':
			radix_flags |= MOO_INTTOSTR_LOWERCASE;
		case 'X':
			radix = 16;
			goto print_integer;
		case 'b':
			radix = 2;
			goto print_integer;
		/* end of integer conversions */

		case 'f':
		{
			const moo_ooch_t* nsptr;
			moo_oow_t nslen;
			moo_oow_t scale = 0;

			GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
			if (MOO_OOP_IS_CHAR(arg)) 
			{
				arg = MOO_SMOOI_TO_OOP(MOO_OOP_TO_CHAR(arg));
			}
			else if (MOO_OOP_IS_FPDEC(moo, arg))
			{
				moo_oop_fpdec_t fa = (moo_oop_fpdec_t)arg;
				scale = MOO_OOP_TO_SMOOI(fa->scale);
				arg = fa->value;
			}

			if (!moo_inttostr(moo, arg, 10 | MOO_INTTOSTR_NONEWOBJ)) 
			{
				MOO_LOG1 (moo, MOO_LOG_WARN | MOO_LOG_UNTYPED, "unable to convert %O to string \n", arg);
				goto invalid_format;
			}

			nsptr = moo->inttostr.xbuf.ptr;
			nslen = moo->inttostr.xbuf.len;
			MOO_ASSERT (moo, nslen > 0);

			if (nsptr[0] == '-')
			{
				MOO_ASSERT (moo, (MOO_OOP_IS_SMOOI(arg) && MOO_OOP_TO_SMOOI(arg) < 0) || MOO_OOP_IS_NBIGINT(moo,arg));
				nsptr++;
				nslen--;
				neg = 1;
			}

			if (!(flagc & FLAGC_DOT)) 
			{
				precision = scale;
				if (precision <= 0) precision = 1;
			}

			if ((flagc & FLAGC_DOT) && precision < scale)  
			{
				moo_oow_t diff  = scale - precision;
				scale = precision;
				nslen = (nslen < diff)? 0: (nslen - diff);
			}

			if (nslen < scale + 1)
			{
				extra = 1;
				if (precision > 0) extra += 1 + scale;
			}
			else
			{
				extra = 0;
				if (nslen > 0) extra += nslen - scale;
				if (precision > 0)
				{
					extra += 1;
					if (nslen > 0) extra += scale;
				}
			}

			if (neg) extra++;
			else if (flagc & FLAGC_SIGN) extra++;
			else if (flagc & FLAGC_SPACE) extra++;

			if ((flagc & FLAGC_DOT) && precision > scale) 
			{
				/* trailing zeros in the fractional part */
				extra += precision - scale;
			}

			if (!(flagc & FLAGC_LEFTADJ) && !(flagc & FLAGC_ZEROPAD) && width > extra)
			{
				width -= extra;
				PRINT_OOCH (padc, width);
				width = 0;
			}
			if (neg) PRINT_OOCH ('-', 1);
			else if (flagc & FLAGC_SIGN) PRINT_OOCH ('+', 1);
			else if (flagc & FLAGC_SPACE) PRINT_OOCH (' ', 1);

			if (!(flagc & FLAGC_LEFTADJ) && width > extra)
			{
				width -= extra;
				PRINT_OOCH (padc, width);
			}

			if (nslen < scale + 1)
			{
				PRINT_OOCH ('0', 1);
				if (precision > 0)
				{
					PRINT_OOCH ('.', 1);
					PRINT_OOCH ('0', scale - nslen);
					PRINT_OOCS (nsptr, nslen);
				}
			}
			else
			{
				if (nslen > 0) PRINT_OOCS (nsptr, nslen - scale);
				if (precision > 0) 
				{
					PRINT_OOCH ('.', 1);
					if (nslen > 0) PRINT_OOCS (&nsptr[nslen - scale], scale);
				}
			}
			if (precision > scale) 
			{
				/* trailing zeros in the fractional part */
				PRINT_OOCH ('0', precision - scale);
			}

			if ((flagc & FLAGC_LEFTADJ) && width > extra)
			{
				width -= extra;
				PRINT_OOCH (padc, width);
			}
			break;
		}

		case 'c':
		case 'C':
			GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
			if (MOO_OOP_IS_SMOOI(arg)) arg = MOO_CHAR_TO_OOP(MOO_OOP_TO_SMOOI(arg));
			if (!MOO_OOP_IS_CHAR(arg)) goto invalid_format;
			ooch = MOO_OOP_TO_CHAR(arg);

		print_char:
			/* zeropad must not take effect for 'c' */
			if (flagc & FLAGC_ZEROPAD) padc = ' '; 

			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PRINT_OOCH (padc, width);
			PRINT_OOCH (ooch, 1);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PRINT_OOCH (padc, width);
			break;

		case 's':
		case 'S':
		{
			const moo_ooch_t* oosp;
			moo_oow_t oosl;

			/* zeropad must not take effect for 'S' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';

			GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
			if (!MOO_OOP_IS_POINTER(arg) || MOO_OBJ_GET_FLAGS_TYPE(arg) != MOO_OBJ_TYPE_CHAR) goto invalid_format;

			oosp = MOO_OBJ_GET_CHAR_SLOT(arg);
			oosl = MOO_OBJ_GET_SIZE(arg);

			if (flagc & FLAGC_DOT)
			{
				if (oosl > precision) oosl = precision;
			}
			width -= oosl;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PRINT_OOCH (padc, width);
			PRINT_OOCS (oosp, oosl);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PRINT_OOCH (padc, width);
			break;
		}

		case 'O': /* object - ignore precision, width, adjustment */
			GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
			if (print_object(moo, data->mask, arg, outbfmt) <= -1) goto oops;
			break;

		print_integer:
		{
			const moo_ooch_t* nsptr;
			moo_oow_t nslen;

			GET_NEXT_ARG_TO (moo, nargs, &arg_state, arg);
			if (MOO_OOP_IS_CHAR(arg)) 
			{
				arg = MOO_SMOOI_TO_OOP(MOO_OOP_TO_CHAR(arg));
			}
			else if (MOO_OOP_IS_FPDEC(moo, arg))
			{
				moo_oop_t nv;
				moo_oop_fpdec_t fa = (moo_oop_fpdec_t)arg;

				/* the given number for integer output is a fixed-point decimal.
				 * i will drop all digits after the fixed point */
				moo_pushvolat (moo, &arg);
				nv = moo_truncfpdecval(moo, fa->value, MOO_OOP_TO_SMOOI(fa->scale), 0);
				moo_popvolat (moo);
				if (!nv)
				{
					MOO_LOG1 (moo, MOO_LOG_WARN | MOO_LOG_UNTYPED, "unable to truncate a fixed-point number %O to an integer for output\n", arg);
					goto invalid_format;
				}

				arg = nv;
			}

			if (!moo_inttostr(moo, arg, radix | radix_flags)) 
			{
				/*moo_seterrbfmt (moo, MOO_EINVAL, "not a valid number - %O", arg);
				goto oops;*/
				MOO_LOG1 (moo, MOO_LOG_WARN | MOO_LOG_UNTYPED, "unable to convert integer %O to string \n", arg);
				goto invalid_format;
			}

			nsptr = moo->inttostr.xbuf.ptr;
			nslen = moo->inttostr.xbuf.len;

			MOO_ASSERT (moo, nslen > 0);
			if (nsptr[0] == '-') 
			{
				/* a negative number was given. i must skip the minus sign 
				 * added by moo_inttostr() for a negative number. */
				MOO_ASSERT (moo, (MOO_OOP_IS_SMOOI(arg) && MOO_OOP_TO_SMOOI(arg) < 0) || MOO_OOP_IS_NBIGINT(moo,arg));
				nsptr++;
				nslen--; 
			} 

			extra = nslen;
			if (sign && ((MOO_OOP_IS_SMOOI(arg) && MOO_OOP_TO_SMOOI(arg) < 0) || MOO_OOP_IS_NBIGINT(moo,arg))) neg = 1;

			if ((flagc & FLAGC_SHARP) && arg != MOO_SMOOI_TO_OOP(0)) 
			{
				if (radix == 2 || radix == 8 || radix == 16) extra += 2;
			}
			if (neg) extra++;
			else if (flagc & FLAGC_SIGN) extra++;
			else if (flagc & FLAGC_SPACE) extra++;

			if ((flagc & FLAGC_DOT) && precision > nslen) 
			{
				/* extra zeros for precision specified */
				extra += (precision - nslen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && !(flagc & FLAGC_ZEROPAD) && width > 0 && (width -= extra) > 0)
			{
				PRINT_OOCH (padc, width);
				width = 0;
			}

			if (neg) PRINT_OOCH ('-', 1);
			else if (flagc & FLAGC_SIGN) PRINT_OOCH ('+', 1);
			else if (flagc & FLAGC_SPACE) PRINT_OOCH (' ', 1);

			if ((flagc & FLAGC_SHARP) && arg != MOO_SMOOI_TO_OOP(0)) 
			{
				if (radix == 2) 
				{
					PRINT_OOCH ('2', 1);
					PRINT_OOCH ('r', 1);
				}
				if (radix == 8) 
				{
					PRINT_OOCH ('8', 1);
					PRINT_OOCH ('r', 1);
				} 
				else if (radix == 16) 
				{
					PRINT_OOCH ('1', 1);
					PRINT_OOCH ('6', 1);
					PRINT_OOCH ('r', 1);
				}
			}

			if ((flagc & FLAGC_DOT) && precision > nslen)
			{
				/* extra zeros for precision specified */
				PRINT_OOCH ('0', precision - nslen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0 && (width -= extra) > 0)
			{
				PRINT_OOCH (padc, width);
			}

			PRINT_OOCS (nsptr, nslen);

			if ((flagc & FLAGC_LEFTADJ) && width > 0 && (width -= extra) > 0)
			{
				PRINT_OOCH (padc, width);
			}
			break;
		}

		invalid_format:
			PRINT_OOCS (percent, fmt - percent);
			break;

		default:
			PRINT_OOCS (percent, fmt - percent);
			/*
			 * Since we ignore an formatting argument it is no
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			arg_state.stop = 1;
			break;
		}
	}

done:
	return 0;

oops:
	return -1;
}

#if 0
/* -------------------------------------------------------------------------- 
 * SUPPORT FOR THE BUILTIN PRINTF PRIMITIVE FUNCTION
 * -------------------------------------------------------------------------- */

int moo_printfmtst (moo_t* moo, moo_ooi_t nargs)
{
	moo_fmtout_data_t fo;
	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.putch = put_prch;
	fo.putcs = put_prcs;
	return print_formatted(moo, nargs, &fo, moo_proutbfmt, 0);
}

/* -------------------------------------------------------------------------- 
 * SUPPORT FOR THE BUILTIN LOGF PRIMITIVE FUNCTION
 * -------------------------------------------------------------------------- */

int moo_logfmtst (moo_t* moo, moo_ooi_t nargs)
{
	moo_fmtout_data_t fo;

	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.mask = MOO_LOG_FATAL | MOO_LOG_APP;

	if (moo->log.default_type_mask & MOO_LOG_ALL_TYPES) 
	{
		/* if a type is given, it's not untyped any more.
		 * mask off the UNTYPED bit */
		fo.mask &= ~MOO_LOG_UNTYPED; 

		/* if the default_type_mask has the UNTYPED bit on,
		 * it'll get turned back on */
		fo.mask |= (moo->log.default_type_mask & MOO_LOG_ALL_TYPES);
	}

	fo.putch = put_logch;
	fo.putcs = put_logcs;
	return print_formatted(moo, nargs, &fo, moo_logbfmt, 0);
}
#endif

/* -------------------------------------------------------------------------- 
 * SUPPORT FOR THE BUILTIN SPRINTF PRIMITIVE FUNCTION
 * -------------------------------------------------------------------------- */
static int put_sprcs (moo_t* moo, moo_bitmask_t mask, const moo_ooch_t* ptr, moo_oow_t len)
{
	if (len > moo->sprintf.xbuf.capa - moo->sprintf.xbuf.len)
	{
		moo_ooch_t* tmp;
		moo_oow_t newcapa;

		newcapa = moo->sprintf.xbuf.len + len + 1;
		newcapa = MOO_ALIGN_POW2(newcapa, 256);

		tmp = (moo_ooch_t*)moo_reallocmem(moo, moo->sprintf.xbuf.ptr, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->sprintf.xbuf.ptr = tmp;
		moo->sprintf.xbuf.capa = newcapa;
	}

	MOO_MEMCPY (&moo->sprintf.xbuf.ptr[moo->sprintf.xbuf.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->sprintf.xbuf.len += len;
	return 1; /* success */
}

static int put_sprch (moo_t* moo, moo_bitmask_t mask, moo_ooch_t ch, moo_oow_t len)
{
	if (len > moo->sprintf.xbuf.capa - moo->sprintf.xbuf.len)
	{
		moo_ooch_t* tmp;
		moo_oow_t newcapa;

		newcapa = moo->sprintf.xbuf.len + len + 1;
		newcapa = MOO_ALIGN_POW2(newcapa, 256);

		tmp = (moo_ooch_t*)moo_reallocmem(moo, moo->sprintf.xbuf.ptr, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->sprintf.xbuf.ptr = tmp;
		moo->sprintf.xbuf.capa = newcapa;
	}

	while (len > 0)
	{
		--len;
		moo->sprintf.xbuf.ptr[moo->sprintf.xbuf.len++] = ch;
	}

	return 1; /* success */
}

static moo_ooi_t __sprbfmtv (moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...);

static int _sprbfmtv (moo_t* moo, const moo_bch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logbfmtv(moo, fmt, data, ap, __sprbfmtv);
}

/*
static int _sprufmtv (moo_t* moo, const moo_uch_t* fmt, moo_fmtout_data_t* data, va_list ap)
{
	return __logufmtv (moo, fmt, data, ap, __sprbfmtv);
}*/

static moo_ooi_t __sprbfmtv(moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_data_t fo;

	fo.mask = mask; /* not used */
	fo.putch = put_sprch;
	fo.putcs = put_sprcs;

	va_start (ap, fmt);
	_sprbfmtv (moo, fmt, &fo, ap);
	va_end (ap);

	return fo.count;
}

moo_ooi_t moo_sproutbfmt (moo_t* moo, moo_bitmask_t mask, const moo_bch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_data_t fo;

	fo.mask = mask;
	fo.putch = put_sprch;
	fo.putcs = put_sprcs;

	va_start (ap, fmt);
	x = _sprbfmtv(moo, fmt, &fo, ap);
	va_end (ap);

	return (x <= -1)? -1: fo.count;
}

/*
moo_ooi_t moo_sproutufmt (moo_t* moo, moo_bitmask_t mask, const moo_uch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_data_t fo;

	fo.mask = mask;
	fo.putch = put_sprch;
	fo.putcs = put_sprcs;

	va_start (ap, fmt);
	x = _sprufmtv(moo, fmt, &fo, ap);
	va_end (ap);

	return (x <= -1)? -1: fo.count;
}*/


int moo_sprintfmtst (moo_t* moo, moo_ooi_t nargs)
{
	/* format a string using the receiver and arguments on the stack */
	moo_fmtout_data_t fo;
	MOO_MEMSET (&fo, 0, MOO_SIZEOF(fo));
	fo.putch = put_sprch;
	fo.putcs = put_sprcs;
	moo->sprintf.xbuf.len = 0;
	return print_formatted(moo, nargs, &fo, moo_sproutbfmt, 0);
}
