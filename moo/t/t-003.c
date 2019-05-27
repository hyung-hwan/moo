#include <moo.h>
#include <moo-utl.h>
#include <stdarg.h>


/* Max number conversion buffer length: 
 * moo_intmax_t in base 2, plus NUL byte. */
#define MAXNBUF (MOO_SIZEOF(moo_intmax_t) * MOO_BITS_PER_BYTE + 1)

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




typedef struct moo_fmtout_t moo_fmtout_t;

typedef int (*moo_fmtout_putbch_t) (
	
	moo_fmtout_t*   fmtout,
	moo_bch_t       c,
	moo_oow_t       len
);

typedef int (*moo_fmtout_putbcs_t) (
	moo_fmtout_t*     fmtout,
	const moo_bch_t*  ptr,
	moo_oow_t         len
);

typedef int (*moo_fmtout_putuch_t) (
	moo_fmtout_t*     fmtout,
	moo_uch_t         c,
	moo_oow_t         len
);

typedef int (*moo_fmtout_putucs_t) (
	moo_fmtout_t*     fmtout,
	const moo_uch_t*  ptr,
	moo_oow_t         len
);

typedef moo_ooi_t (*moo_outbfmt_t) (
	moo_t*             moo,
	moo_bitmask_t      mask,
	const moo_bch_t*   fmt,
	...
);

enum moo_fmtout_fmt_type_t 
{
	MOO_FMTOUT_FMT_TYPE_BCH = 0,
	MOO_FMTOUT_FMT_TYPE_UCH
};
typedef enum moo_fmtout_fmt_type_t moo_fmtout_fmt_type_t;


struct moo_fmtout_t
{
	moo_oow_t             count; /* out */

	moo_fmtout_fmt_type_t fmttype;
	const void*           fmtstr;
	moo_bitmask_t         mask;  /* in */
	void*                 ctx;

	moo_fmtout_putbch_t   putbch; /* in */
	moo_fmtout_putbcs_t   putbcs; /* in */
	moo_fmtout_putuch_t   putuch; /* in */
	moo_fmtout_putucs_t   putucs; /* in */
	/*moo_outbfmt_t outbfmt;*/
};


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

#define PUT_BCH(fmtout,c,n) do { \
	if (n > 0) { \
		int xx; \
		if ((xx = fmtout->putbch(fmtout, c, n)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		fmtout->count += n; \
	} \
} while (0)

#define PUT_BCS(fmtout,ptr,len) do { \
	if (len > 0) { \
		int xx; \
		if ((xx = fmtout->putbcs(fmtout, ptr, len)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		fmtout->count += len; \
	} \
} while (0)

#define PUT_UCH(fmtout,c,n) do { \
	if (n > 0) { \
		int xx; \
		if ((xx = fmtout->putuch(fmtout, c, n)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		fmtout->count += n; \
	} \
} while (0)

#define PUT_UCS(fmtout,ptr,len) do { \
	if (len > 0) { \
		int xx; \
		if ((xx = fmtout->putucs(fmtout, ptr, len)) <= -1) goto oops; \
		if (xx == 0) goto done; \
		fmtout->count += len; \
	} \
} while (0)


#if defined(MOO_OOCH_IS_BCH)
#	define PUT_OOCH(fmtout,c,n) PUT_BCH(fmtout,c,n)
#	define PUT_OOCS(fmtout,ptr,len) PUT_BCS(fmtout,ptr,len)
#else
#	define PUT_OOCH(fmtout,c,n) PUT_UCH(fmtout,c,n)
#	define PUT_OOCS(fmtout,ptr,len) PUT_UCS(fmtout,ptr,len)
#endif

#define BYTE_PRINTABLE(x) ((x >= 'a' && x <= 'z') || (x >= 'A' &&  x <= 'Z') || (x >= '0' && x <= '9') || (x == ' '))


#define PUT_BYTE_IN_HEX(fmtout,byte,extra_flags) do { \
	moo_bch_t __xbuf[3]; \
	moo_byte_to_bcstr ((byte), __xbuf, MOO_COUNTOF(__xbuf), (16 | (extra_flags)), '0'); \
	PUT_BCH(fmtout, __xbuf[0], 1); \
	PUT_BCH(fmtout, __xbuf[1], 1); \
} while (0)

/* ------------------------------------------------------------------------- */

static int moo_do_fmtoutv (moo_fmtout_t* fmtout, va_list ap)
{
	const moo_uint8_t* fmtptr, * percent;
	int fmtchsz;

	moo_uch_t uch;
	moo_bch_t bch;
	moo_ooch_t padc;
	
	int n, base, neg, sign;
	moo_ooi_t tmp, width, precision;
	int lm_flag, lm_dflag, flagc, numlen;

	moo_uintmax_t num = 0;
	moo_bch_t nbuf[MAXNBUF];
	const moo_bch_t* nbufp;
	int stop = 0;
#if 0
	moo_bchbuf_t* fltfmt;
	moo_oochbuf_t* fltout;
#endif
	moo_bch_t* (*sprintn) (moo_bch_t* nbuf, moo_uintmax_t num, int base, moo_ooi_t* lenp);


	fmtptr = (const moo_uint8_t*)fmtout->fmtstr;
	switch (fmtout->fmttype)
	{
		case MOO_FMTOUT_FMT_TYPE_BCH: 
			fmtchsz = MOO_SIZEOF_BCH_T;
			break;
		case MOO_FMTOUT_FMT_TYPE_UCH: 
			fmtchsz = MOO_SIZEOF_UCH_T;
			break;
	}
	fmtout->count = 0;

#if 0
	fltfmt = &moo->d->fltfmt;
	fltout = &moo->d->fltout;

	fltfmt->ptr  = fltfmt->buf;
	fltfmt->capa = MOO_COUNTOF(fltfmt->buf) - 1;

	fltout->ptr  = fltout->buf;
	fltout->capa = MOO_COUNTOF(fltout->buf) - 1;
#endif

	while (1)
	{
	#if defined(HAVE_LABELS_AS_VALUES)
		static void* before_percent_tab[] = { &&before_percent_bch, &&before_percent_uch };
		goto *before_percent_tab[fmtout->fmttype];
	#else
		switch (fmtout->fmttype)
		{
			case MOO_FMOUT_FMT_BCH:
				goto before_percent_bch;
			case MOO_FMTOUT_FMT_TYPE_UCH:
				goto before_percent_uch;
		}
	#endif

	before_percent_bch:
		{
			const moo_bch_t* start, * end;
			start = end = (const moo_bch_t*)fmtptr;
			while ((bch = *end++) != '%' || stop) 
			{
				if (bch == '\0') 
				{
					PUT_BCS (fmtout, start, end - start - 1);
					goto done;
				}
			}
			PUT_BCS (fmtout, start, end - start - 1);
			fmtptr = (const moo_uint8_t*)end;
			percent = (const moo_uint8_t*)(end - 1);
		}
		goto handle_percent;

	before_percent_uch:
		{
			const moo_uch_t* start, * end;
			start = end = (const moo_uch_t*)fmtptr;
			while ((uch = *end++) != '%' || stop) 
			{
				if (uch == '\0') 
				{
					PUT_UCS (fmtout, start, end - start - 1);
					goto done;
				}
			}
			PUT_UCS (fmtout, start, end - start - 1);
			fmtptr = (const moo_uint8_t*)end;
			percent = (const moo_uint8_t*)(end - 1);
		}
		goto handle_percent;

	handle_percent:
		padc = ' '; 
		width = 0; precision = 0; neg = 0; sign = 0;
		lm_flag = 0; lm_dflag = 0; flagc = 0; 
		sprintn = sprintn_lower;

	reswitch:
		switch (fmtout->fmttype)
		{
			case MOO_FMTOUT_FMT_TYPE_BCH: 
				uch = *(const moo_bch_t*)fmtptr;
				break;
			case MOO_FMTOUT_FMT_TYPE_UCH:
				uch = *(const moo_uch_t*)fmtptr;
				break;
		}
		fmtptr += fmtchsz;

		switch (uch) 
		{
		case '%': /* %% */
			bch = uch;
			goto print_lowercase_c;

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

				precision = va_arg(ap, moo_ooi_t); /* this deviates from the standard printf that accepts 'int' */
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

				width = va_arg(ap, moo_ooi_t); /* it deviates from the standard printf that accepts 'int' */
				if (width < 0) 
				{
					/*
					if (flagc & FLAGC_LEFTADJ) 
						flagc  &= ~FLAGC_LEFTADJ;
					else
					*/
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
		{
			if (flagc & FLAGC_LENMOD) goto invalid_format;
			for (n = 0;; fmtptr += fmtchsz) 
			{
				n = n * 10 + uch - '0';
				switch (fmtout->fmttype)
				{
					case MOO_FMTOUT_FMT_TYPE_BCH: 
						uch = *(const moo_bch_t*)fmtptr;
						break;
					case MOO_FMTOUT_FMT_TYPE_UCH:
						uch = *(const moo_uch_t*)fmtptr;
						break;
				}
				if (uch < '0' || uch > '9') break;
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
		}

		/* length modifiers */
		case 'h': /* short int */
		case 'l': /* long int */
		case 'q': /* long long int */
		case 'j': /* moo_intmax_t/moo_uintmax_t */
		case 'z': /* moo_ooi_t/moo_oow_t */
		case 't': /* ptrdiff_t */
			if (lm_flag & (LF_LD | LF_QD)) goto invalid_format;

			flagc |= FLAGC_LENMOD;
			if (lm_dflag)
			{
				/* error */
				goto invalid_format;
			}
			else if (lm_flag)
			{
				if (lm_tab[uch - 'a'].dflag && lm_flag == lm_tab[uch - 'a'].flag)
				{
					lm_flag &= ~lm_tab[uch - 'a'].flag;
					lm_flag |= lm_tab[uch - 'a'].dflag;
					lm_dflag |= lm_flag;
					goto reswitch;
				}
				else
				{
					/* error */
					goto invalid_format;
				}
			}
			else 
			{
				lm_flag |= lm_tab[uch - 'a'].flag;
				goto reswitch;
			}
			break;

		case 'L': /* long double */
			if (flagc & FLAGC_LENMOD) 
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_LD;
			goto reswitch;

		case 'Q': /* __float128 */
			if (flagc & FLAGC_LENMOD)
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_QD;
			goto reswitch;
		/* end of length modifiers */

		case 'n': /* number of characters printed so far */
			if (lm_flag & LF_J) /* j */
				*(va_arg(ap, moo_intmax_t*)) = fmtout->count;
			else if (lm_flag & LF_Z) /* z */
				*(va_arg(ap, moo_ooi_t*)) = fmtout->count;
		#if (MOO_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q) /* ll */
				*(va_arg(ap, long long int*)) = fmtout->count;
		#endif
			else if (lm_flag & LF_L) /* l */
				*(va_arg(ap, long int*)) = fmtout->count;
			else if (lm_flag & LF_H) /* h */
				*(va_arg(ap, short int*)) = fmtout->count;
			else if (lm_flag & LF_C) /* hh */
				*(va_arg(ap, char*)) = fmtout->count;
			else if (flagc & FLAGC_LENMOD) 
				goto invalid_format;
			else
				*(va_arg(ap, int*)) = fmtout->count;
			break;
 
		/* signed integer conversions */
		case 'd':
		case 'i': /* signed conversion */
			base = 10;
			sign = 1;
			goto handle_sign;
		/* end of signed integer conversions */

		/* unsigned integer conversions */
		case 'o': 
			base = 8;
			goto handle_nosign;
		case 'u':
			base = 10;
			goto handle_nosign;
		case 'X':
			sprintn = sprintn_upper;
		case 'x':
			base = 16;
			goto handle_nosign;
		case 'b':
			base = 2;
			goto handle_nosign;
		/* end of unsigned integer conversions */

		case 'p': /* pointer */
			base = 16;

			if (width == 0) flagc |= FLAGC_SHARP;
			else flagc &= ~FLAGC_SHARP;

			num = (moo_uintptr_t)va_arg(ap, void*);
			goto number;

		case 'c':
		{
			/* zeropad must not take effect for 'c' */
			if (flagc & FLAGC_ZEROPAD) padc = ' '; 
			if (lm_flag & LF_L) goto uppercase_c;
		#if defined(MOO_OOCH_IS_UCH)
			if (lm_flag & LF_J) goto uppercase_c;
		#endif
		lowercase_c:
			bch = MOO_SIZEOF(moo_bch_t) < MOO_SIZEOF(int)? va_arg(ap, int): va_arg(ap, moo_bch_t);

		print_lowercase_c:
			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_BCH (fmtout, padc, width);
			PUT_BCH (fmtout, bch, 1);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_BCH (fmtout, padc, width);
			break;
		}

		case 'C':
		{
			/* zeropad must not take effect for 'C' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_H) goto lowercase_c;
		#if defined(MOO_OOCH_IS_BCH)
			if (lm_flag & LF_J) goto lowercase_c;
		#endif
		uppercase_c:
			uch = MOO_SIZEOF(moo_uch_t) < MOO_SIZEOF(int)? va_arg(ap, int): va_arg(ap, moo_uch_t);

			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_UCH (fmtout, padc, width);
			PUT_UCH (fmtout, uch, 1);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_UCH (fmtout, padc, width);
			break;
		}

		case 's':
		{
			const moo_bch_t* bsp;

			/* zeropad must not take effect for 'S' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_L) goto uppercase_s;
		#if defined(MOO_OOCH_IS_UCH)
			if (lm_flag & LF_J) goto uppercase_s;
		#endif
		lowercase_s:
			bsp = va_arg(ap, moo_bch_t*);
			if (!bsp) bsp = bch_nullstr;

			if (flagc & FLAGC_DOT)
			{
				for (n = 0; n < precision && bsp[n]; n++);
			}
			else
			{
				for (n = 0; bsp[n]; n++);
			}

			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_BCH (fmtout, padc, width);
			PUT_BCS (fmtout, bsp, n);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_BCH (fmtout, padc, width);
			break;
		}

		case 'S':
		{
			const moo_uch_t* usp;

			/* zeropad must not take effect for 's' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_H) goto lowercase_s;
		#if defined(MOO_OOCH_IS_UCH)
			if (lm_flag & LF_J) goto lowercase_s;
		#endif
		uppercase_s:
			usp = va_arg(ap, moo_uch_t*);
			if (!usp) usp = uch_nullstr;

			if (flagc & FLAGC_DOT)
			{
				for (n = 0; n < precision && usp[n]; n++);
			}
			else
			{
				for (n = 0; usp[n]; n++);
			}

			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_UCH (fmtout, padc, width);
			PUT_UCS (fmtout, usp, n);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_UCH (fmtout, padc, width);

			break;
		}

		case 'k':
		case 'K':
		{
			/* byte or multibyte character string in escape sequence */

			const moo_uint8_t* bsp;
			moo_oow_t k_hex_width;

			/* zeropad must not take effect for 'k' and 'K' 
			 * 
 			 * 'h' & 'l' is not used to differentiate moo_bch_t and moo_uch_t
			 * because 'k' means moo_byte_t. 
			 * 'l', results in uppercase hexadecimal letters. 
			 * 'h' drops the leading \x in the output 
			 * --------------------------------------------------------
			 * hk -> \x + non-printable in lowercase hex
			 * k -> all in lowercase hex
			 * lk -> \x +  all in lowercase hex
			 * --------------------------------------------------------
			 * hK -> \x + non-printable in uppercase hex
			 * K -> all in uppercase hex
			 * lK -> \x +  all in uppercase hex
			 * --------------------------------------------------------
			 * with 'k' or 'K', i don't substitute "(null)" for the NULL pointer
			 */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';

			bsp = va_arg(ap, moo_uint8_t*);
			k_hex_width = (lm_flag & (LF_H | LF_L))? 4: 2;

			if (lm_flag& LF_H)
			{
				if (flagc & FLAGC_DOT)
				{
					/* if precision is specifed, it doesn't stop at the value of zero unlike 's' or 'S' */
					for (n = 0; n < precision; n++) width -= BYTE_PRINTABLE(bsp[n])? 1: k_hex_width;
				}
				else
				{
					for (n = 0; bsp[n]; n++) width -= BYTE_PRINTABLE(bsp[n])? 1: k_hex_width;
				}
			}
			else
			{
				if (flagc & FLAGC_DOT)
				{
					/* if precision is specifed, it doesn't stop at the value of zero unlike 's' or 'S' */
					for (n = 0; n < precision; n++) /* nothing */;
				}
				else
				{
					for (n = 0; bsp[n]; n++) /* nothing */;
				}
				width -= (n * k_hex_width);
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (fmtout, padc, width);

			while (n--) 
			{
				if ((lm_flag & LF_H) && BYTE_PRINTABLE(*bsp)) 
				{
					PUT_BCH (fmtout, *bsp, 1);
				}
				else
				{
					moo_bch_t xbuf[3];
					moo_byte_to_bcstr (*bsp, xbuf, MOO_COUNTOF(xbuf), (16 | (uch == 'k'? MOO_BYTE_TO_BCSTR_LOWERCASE: 0)), '0');
					if (lm_flag & (LF_H | LF_L))
					{
						PUT_BCH(fmtout, '\\', 1);
						PUT_BCH(fmtout, 'x', 1);
					}
					PUT_BCH(fmtout, xbuf[0], 1);
					PUT_BCH(fmtout, xbuf[1], 1);
				}
				bsp++;
			}

			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (fmtout, padc, width);
			break;
		}

		case 'w':
		case 'W':
		{
			/* unicode string in unicode escape sequence.
			 * 
			 * hw -> \uXXXX, \UXXXXXXXX, printable-byte(only in ascii range)
			 * w -> \uXXXX, \UXXXXXXXX
			 * lw -> all in \UXXXXXXXX
			 */
			const moo_uch_t* usp;
			moo_oow_t uwid;

			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			usp = va_arg(ap, moo_uch_t*);

			if (flagc & FLAGC_DOT)
			{
				/* if precision is specifed, it doesn't stop at the value of zero unlike 's' or 'S' */
				for (n = 0; n < precision; n++) 
				{
					if ((lm_flag & LF_H) && BYTE_PRINTABLE(usp[n])) uwid = 1;
					else if (!(lm_flag & LF_L) && usp[n] <= 0xFFFF) uwid = 6;
					else uwid = 10;
					width -= uwid;
				}
			}
			else
			{
				for (n = 0; usp[n]; n++)
				{
					if ((lm_flag & LF_H) && BYTE_PRINTABLE(usp[n])) uwid = 1;
					else if (!(lm_flag & LF_L) && usp[n] <= 0xFFFF) uwid = 6;
					else uwid = 10;
					width -= uwid;
				}
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (fmtout, padc, width);

			while (n--) 
			{
				if ((lm_flag & LF_H) && BYTE_PRINTABLE(*usp)) 
				{
					PUT_OOCH(fmtout, *usp, 1);
				}
				else if (!(lm_flag & LF_L) && *usp <= 0xFFFF) 
				{
					moo_uint16_t u16 = *usp;
					int extra_flags = ((uch) == 'w'? MOO_BYTE_TO_BCSTR_LOWERCASE: 0);
					PUT_BCH(fmtout, '\\', 1);
					PUT_BCH(fmtout, 'u', 1);
					PUT_BYTE_IN_HEX(fmtout, (u16 >> 8) & 0xFF, extra_flags);
					PUT_BYTE_IN_HEX(fmtout, u16 & 0xFF, extra_flags);
				}
				else
				{
					moo_uint32_t u32 = *usp;
					int extra_flags = ((uch) == 'w'? MOO_BYTE_TO_BCSTR_LOWERCASE: 0);
					PUT_BCH(fmtout, '\\', 1);
					PUT_BCH(fmtout, 'U', 1);
					PUT_BYTE_IN_HEX(fmtout, (u32 >> 24) & 0xFF, extra_flags);
					PUT_BYTE_IN_HEX(fmtout, (u32 >> 16) & 0xFF, extra_flags);
					PUT_BYTE_IN_HEX(fmtout, (u32 >> 8) & 0xFF, extra_flags);
					PUT_BYTE_IN_HEX(fmtout, u32 & 0xFF, extra_flags);
				}
				usp++;
			}

			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (fmtout, padc, width);
			break;
		}

#if 0
//////////////////////////
		case 'O': /* object - ignore precision, width, adjustment */
			if (print_object(moo, fmtout->mask, va_arg(ap, moo_oop_t), outbfmt) <= -1) goto oops;
			break;
#endif

#if 0
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		/*
		case 'a':
		case 'A':
		*/
		{
			/* let me rely on snprintf until i implement float-point to string conversion */
			int q;
			moo_oow_t fmtlen;
		#if (MOO_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			__float128 v_qd;
		#endif
			long double v_ld;
			double v_d;
			int dtype = 0;
			moo_oow_t newcapa;

			if (lm_flag & LF_J)
			{
			#if (MOO_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF) && (MOO_SIZEOF_FLTMAX_T == MOO_SIZEOF___FLOAT128)
				v_qd = va_arg (ap, moo_fltmax_t);
				dtype = LF_QD;
			#elif MOO_SIZEOF_FLTMAX_T == MOO_SIZEOF_DOUBLE
				v_d = va_arg (ap, moo_fltmax_t);
			#elif MOO_SIZEOF_FLTMAX_T == MOO_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, moo_fltmax_t);
				dtype = LF_LD;
			#else
				#error Unsupported moo_flt_t
			#endif
			}
			else if (lm_flag & LF_Z)
			{
				/* moo_flt_t is limited to double or long double */

				/* precedence goes to double if sizeof(double) == sizeof(long double) 
				 * for example, %Lf didn't work on some old platforms.
				 * so i prefer the format specifier with no modifier.
				 */
			#if MOO_SIZEOF_FLT_T == MOO_SIZEOF_DOUBLE
				v_d = va_arg (ap, moo_flt_t);
			#elif MOO_SIZEOF_FLT_T == MOO_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, moo_flt_t);
				dtype = LF_LD;
			#else
				#error Unsupported moo_flt_t
			#endif
			}
			else if (lm_flag & (LF_LD | LF_L))
			{
				v_ld = va_arg (ap, long double);
				dtype = LF_LD;
			}
		#if (MOO_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			else if (lm_flag & (LF_QD | LF_Q))
			{
				v_qd = va_arg (ap, __float128);
				dtype = LF_QD;
			}
		#endif
			else if (flagc & FLAGC_LENMOD)
			{
				goto invalid_format;
			}
			else
			{
				v_d = va_arg (ap, double);
			}

			fmtlen = fmt - percent;
			if (fmtlen > fltfmt->capa)
			{
				if (fltfmt->ptr == fltfmt->buf)
				{
					fltfmt->ptr = MOO_MMGR_ALLOC(MOO_MMGR_GETDFL(), MOO_SIZEOF(*fltfmt->ptr) * (fmtlen + 1));
					if (!fltfmt->ptr) goto oops;
				}
				else
				{
					moo_bch_t* tmpptr;

					tmpptr = MOO_MMGR_REALLOC(MOO_MMGR_GETDFL(), fltfmt->ptr, MOO_SIZEOF(*fltfmt->ptr) * (fmtlen + 1));
					if (!tmpptr) goto oops;
					fltfmt->ptr = tmpptr;
				}

				fltfmt->capa = fmtlen;
			}

			/* compose back the format specifier */
			fmtlen = 0;
			fltfmt->ptr[fmtlen++] = '%';
			if (flagc & FLAGC_SPACE) fltfmt->ptr[fmtlen++] = ' ';
			if (flagc & FLAGC_SHARP) fltfmt->ptr[fmtlen++] = '#';
			if (flagc & FLAGC_SIGN) fltfmt->ptr[fmtlen++] = '+';
			if (flagc & FLAGC_LEFTADJ) fltfmt->ptr[fmtlen++] = '-';
			if (flagc & FLAGC_ZEROPAD) fltfmt->ptr[fmtlen++] = '0';

			if (flagc & FLAGC_STAR1) fltfmt->ptr[fmtlen++] = '*';
			else if (flagc & FLAGC_WIDTH) 
			{
				fmtlen += moo_fmtuintmaxtombs (
					&fltfmt->ptr[fmtlen], fltfmt->capa - fmtlen, 
					width, 10, -1, '\0', MOO_NULL);
			}
			if (flagc & FLAGC_DOT) fltfmt->ptr[fmtlen++] = '.';
			if (flagc & FLAGC_STAR2) fltfmt->ptr[fmtlen++] = '*';
			else if (flagc & FLAGC_PRECISION) 
			{
				fmtlen += moo_fmtuintmaxtombs (
					&fltfmt->ptr[fmtlen], fltfmt->capa - fmtlen, 
					precision, 10, -1, '\0', MOO_NULL);
			}

			if (dtype == LF_LD)
				fltfmt->ptr[fmtlen++] = 'L';
		#if (MOO_SIZEOF___FLOAT128 > 0)
			else if (dtype == LF_QD)
				fltfmt->ptr[fmtlen++] = 'Q';
		#endif

			fltfmt->ptr[fmtlen++] = ch;
			fltfmt->ptr[fmtlen] = '\0';

		#if defined(HAVE_SNPRINTF)
			/* nothing special here */
		#else
			/* best effort to avoid buffer overflow when no snprintf is available. 
			 * i really can't do much if it happens. */
			newcapa = precision + width + 32;
			if (fltout->capa < newcapa)
			{
				MOO_ASSERT (moo, fltout->ptr == fltout->buf);

				fltout->ptr = MOO_MMGR_ALLOC(MOO_MMGR_GETDFL(), MOO_SIZEOF(char_t) * (newcapa + 1));
				if (!fltout->ptr) goto oops;
				fltout->capa = newcapa;
			}
		#endif

			while (1)
			{

				if (dtype == LF_LD)
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((moo_bch_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_ld);
				#else
					q = sprintf ((moo_bch_t*)fltout->ptr, fltfmt->ptr, v_ld);
				#endif
				}
			#if (MOO_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
				else if (dtype == LF_QD)
				{
					q = quadmath_snprintf((moo_bch_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_qd);
				}
			#endif
				else
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((moo_bch_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_d);
				#else
					q = sprintf ((moo_bch_t*)fltout->ptr, fltfmt->ptr, v_d);
				#endif
				}
				if (q <= -1) goto oops;
				if (q <= fltout->capa) break;

				newcapa = fltout->capa * 2;
				if (newcapa < q) newcapa = q;

				if (fltout->ptr == fltout->sbuf)
				{
					fltout->ptr = MOO_MMGR_ALLOC(MOO_MMGR_GETDFL(), MOO_SIZEOF(char_t) * (newcapa + 1));
					if (!fltout->ptr) goto oops;
				}
				else
				{
					char_t* tmpptr;

					tmpptr = MOO_MMGR_REALLOC(MOO_MMGR_GETDFL(), fltout->ptr, MOO_SIZEOF(char_t) * (newcapa + 1));
					if (!tmpptr) goto oops;
					fltout->ptr = tmpptr;
				}
				fltout->capa = newcapa;
			}

			if (MOO_SIZEOF(char_t) != MOO_SIZEOF(moo_bch_t))
			{
				fltout->ptr[q] = '\0';
				while (q > 0)
				{
					q--;
					fltout->ptr[q] = ((moo_bch_t*)fltout->ptr)[q];
				}
			}

			sp = fltout->ptr;
			flagc &= ~FLAGC_DOT;
			width = 0;
			precision = 0;
			goto print_lowercase_s;
		}
#endif


		handle_nosign:
			sign = 0;
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (MOO_SIZEOF_UINTMAX_T > MOO_SIZEOF_OOW_T) && \
			    (MOO_SIZEOF_UINTMAX_T != MOO_SIZEOF_LONG_LONG) && \
			    (MOO_SIZEOF_UINTMAX_T != MOO_SIZEOF_LONG)
				/* GCC-compiled binaries crashed when getting moo_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < MOO_SIZEOF(moo_uintmax_t) / MOO_SIZEOF(moo_oow_t); i++)
				{
				#if defined(MOO_ENDIAN_BIG)
					num = num << (8 * MOO_SIZEOF(moo_oow_t)) | (va_arg (ap, moo_oow_t));
				#else
					register int shift = i * MOO_SIZEOF(moo_oow_t);
					moo_oow_t x = va_arg (ap, moo_oow_t);
					num |= (moo_uintmax_t)x << (shift * MOO_BITS_PER_BYTE);
				#endif
				}
			#else
				num = va_arg (ap, moo_uintmax_t);
			#endif
			}
#if 0
			else if (lm_flag & LF_T)
				num = va_arg(ap, moo_ptrdiff_t);
#endif
			else if (lm_flag & LF_Z)
				num = va_arg(ap, moo_oow_t);
			#if (MOO_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg(ap, unsigned long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg(ap, unsigned long int);
			else if (lm_flag & LF_H)
				num = (unsigned short int)va_arg(ap, int);
			else if (lm_flag & LF_C)
				num = (unsigned char)va_arg(ap, int);
			else
				num = va_arg(ap, unsigned int);
			goto number;

		handle_sign:
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (MOO_SIZEOF_INTMAX_T > MOO_SIZEOF_OOI_T) && \
			    (MOO_SIZEOF_UINTMAX_T != MOO_SIZEOF_LONG_LONG) && \
			    (MOO_SIZEOF_UINTMAX_T != MOO_SIZEOF_LONG)
				/* GCC-compiled binraries crashed when getting moo_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < MOO_SIZEOF(moo_intmax_t) / MOO_SIZEOF(moo_oow_t); i++)
				{
				#if defined(MOO_ENDIAN_BIG)
					num = num << (8 * MOO_SIZEOF(moo_oow_t)) | (va_arg (ap, moo_oow_t));
				#else
					register int shift = i * MOO_SIZEOF(moo_oow_t);
					moo_oow_t x = va_arg (ap, moo_oow_t);
					num |= (moo_uintmax_t)x << (shift * MOO_BITS_PER_BYTE);
				#endif
				}
			#else
				num = va_arg (ap, moo_intmax_t);
			#endif
			}

#if 0
			else if (lm_flag & LF_T)
				num = va_arg(ap, moo_ptrdiff_t);
#endif
			else if (lm_flag & LF_Z)
				num = va_arg (ap, moo_ooi_t);
			#if (MOO_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg (ap, long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg (ap, long int);
			else if (lm_flag & LF_H)
				num = (short int)va_arg (ap, int);
			else if (lm_flag & LF_C)
				num = (char)va_arg (ap, int);
			else
				num = va_arg (ap, int);

		number:
			if (sign && (moo_intmax_t)num < 0) 
			{
				neg = 1;
				num = -(moo_intmax_t)num;
			}

			nbufp = sprintn(nbuf, num, base, &tmp);
			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 2 || base == 8) tmp += 2;
				else if (base == 16) tmp += 3;
			}
			if (neg) tmp++;
			else if (flagc & FLAGC_SIGN) tmp++;
			else if (flagc & FLAGC_SPACE) tmp++;

			numlen = (int)((const moo_bch_t*)nbufp - (const moo_bch_t*)nbuf);
			if ((flagc & FLAGC_DOT) && precision > numlen) 
			{
				/* extra zeros for precision specified */
				tmp += (precision - numlen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && !(flagc & FLAGC_ZEROPAD) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (fmtout, padc, width);
				width = 0;
			}

			if (neg) PUT_OOCH (fmtout, '-', 1);
			else if (flagc & FLAGC_SIGN) PUT_OOCH (fmtout, '+', 1);
			else if (flagc & FLAGC_SPACE) PUT_OOCH (fmtout, ' ', 1);

			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 2) 
				{
					PUT_OOCH (fmtout, '2', 1);
					PUT_OOCH (fmtout, 'r', 1);
				}
				if (base == 8) 
				{
					PUT_OOCH (fmtout, '8', 1);
					PUT_OOCH (fmtout, 'r', 1);
				} 
				else if (base == 16) 
				{
					PUT_OOCH (fmtout, '1', 1);
					PUT_OOCH (fmtout, '6', 1);
					PUT_OOCH (fmtout, 'r', 1);
				}
			}

			if ((flagc & FLAGC_DOT) && precision > numlen)
			{
				/* extra zeros for precision specified */
				PUT_OOCH (fmtout, '0', precision - numlen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (fmtout, padc, width);
			}

			while (*nbufp) PUT_OOCH (fmtout, *nbufp--, 1); /* output actual digits */

			if ((flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (fmtout, padc, width);
			}
			break;

		invalid_format:
			switch (fmtout->fmttype)
			{
				case MOO_FMTOUT_FMT_TYPE_BCH:
					PUT_BCS (fmtout, (const moo_bch_t*)percent, (fmtptr - percent) / fmtchsz);
					break;
				case MOO_FMTOUT_FMT_TYPE_UCH:
					PUT_UCS (fmtout, (const moo_uch_t*)percent, (fmtptr - percent) / fmtchsz);
					break;
			}
			break;

		default:
			switch (fmtout->fmttype)
			{
				case MOO_FMTOUT_FMT_TYPE_BCH:
					PUT_BCS (fmtout, (const moo_bch_t*)percent, (fmtptr - percent) / fmtchsz);
					break;
				case MOO_FMTOUT_FMT_TYPE_UCH:
					PUT_UCS (fmtout, (const moo_uch_t*)percent, (fmtptr - percent) / fmtchsz);
					break;
			}
			/*
			 * Since we ignore an formatting argument it is no
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			stop = 1;
			break;
		}
	}

done:
	return 0;

oops:
	return -1;
}


/* ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>

static int put_bch (moo_fmtout_t* fmtout, moo_bch_t c, moo_oow_t len)
{
	while (len > 0)
	{
		putchar (c);
		len--;
	}
	return 1;	
}

static int put_bcs (moo_fmtout_t* fmtout, const moo_bch_t* c, moo_oow_t len)
{
	while (len > 0)
	{
		putchar (*c);
		c++;
		len--;
	}
	return 1;
}

static int put_uch (moo_fmtout_t* fmtout, moo_uch_t c, moo_oow_t len)
{
	while (len > 0)
	{
		putchar (c);
		len--;
	}
	return 1;
}

static int put_ucs (moo_fmtout_t* fmtout, const moo_uch_t* c, moo_oow_t len)
{
	while (len > 0)
	{
		putchar (*c);
		c++;
		len--;
	}
	return 1;
}

static void moo_bfmtout (const moo_bch_t* fmt, ...)
{
	moo_fmtout_t fmtout;
	va_list ap;

	memset (&fmtout, 0, MOO_SIZEOF(fmtout));
	fmtout.fmttype = MOO_FMTOUT_FMT_TYPE_BCH;
	fmtout.fmtstr = fmt;
	fmtout.putbch = put_bch;
	fmtout.putbcs = put_bcs;
	fmtout.putuch = put_uch;
	fmtout.putucs = put_ucs;

	va_start (ap, fmt);
	moo_do_fmtoutv (&fmtout, ap);
	va_end (ap);
}

/*
static int moo_ufmtout (const moo_bch_t* fmt, ...)
{

}*/

int main ()
{
	moo_uch_t x[] = { 'A', 'B', 'C' };
	moo_bfmtout ("[%s %d %020X %ls %s %w %.*lk]\n", "test", 10, 0x1232, x, "code", x, MOO_SIZEOF_UCH_T * 3, x);
	return 0;
}
