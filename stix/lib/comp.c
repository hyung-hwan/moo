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

#define TOKEN_NAME_ALIGN 256
#define CLASS_BUFFER_ALIGN 8 /* 256 */

#if 0

enum stix_send_target_t
{
	STIX_SELF = 0,
	STIX_SUPER = 1
};
typedef enum stix_send_target_t stix_send_target_t;

enum stix_stack_operand_t
{
	STIX_RECEIVER_VARIABLE = 0,
	STIX_TEMPORARY_LOCATION = 1,
	STIX_LITERAL_CONSTANT = 2,
	STIX_LITERAL_VARIABLE = 3
};
typedef enum stix_stack_operand_t stix_stack_operand_t;

#endif


enum class_mod_t
{
	CLASS_INDEXED   = (1 << 0),
	CLASS_EXTENDED  = (1 << 1)
};

enum mth_mod_t
{
	MTH_CLASS    = (1 << 0)
};

enum var_type_t
{
	VAR_INSTANCE,
	VAR_CLASS,
	VAR_CLASSINST
};
typedef enum var_type_t var_type_t;

struct var_info_t
{
	var_type_t type;
	stix_size_t pos;
	stix_oop_class_t cls; /* useful if type is VAR_CLASS. note STIX_NULL indicates the self class. */
};
typedef struct var_info_t var_info_t;

static struct ksym_t
{
	stix_oow_t len;
	stix_uch_t str[11];
} ksyms[] = {
	{  4, { 'b','y','t','e'                                               } },
	{  9, { 'c','h','a','r','a','c','t','e','r'                           } },
	{  5, { 'c','l','a','s','s'                                           } },
	{  9, { 'c','l','a','s','s','i','n','s','t'                           } },
	{  3, { 'd','c','l'                                                   } },
	{  7, { 'd','e','c','l','a','r','e'                                   } },
	{  5, { 'f','a','l','s','e'                                           } },
	{  7, { 'i','n','c','l','u','d','e'                                   } },
	{  4, { 'm','a','i','n'                                               } },
	{  6, { 'm','e','t','h','o','d'                                       } },
	{  3, { 'm','t','h'                                                   } },
	{  3, { 'n','i','l'                                                   } },
	{  7, { 'p','o','i','n','t','e','r'                                   } },
	{ 10, { 'p','r','i','m','i','t','i','v','e',':'                       } },
	{  4, { 's','e','l','f'                                               } },
	{  5, { 's','u','p','e','r'                                           } },
	{ 11, { 't','h','i','s','C','o','n','t','e','x','t'                   } },
	{  4, { 't','r','u','e'                                               } },
	{  4, { 'w','o','r','d'                                               } },
	{  1, { '|'                                                           } },
	{  1, { '>'                                                           } },
	{  1, { '<'                                                           } }
};

enum ksym_id_t
{
	KSYM_BYTE,
	KSYM_CHARACTER,
	KSYM_CLASS,
	KSYM_CLASSINST,
	KSYM_DCL,
	KSYM_DECLARE,
	KSYM_FALSE,
	KSYM_INCLUDE,
	KSYM_MAIN,
	KSYM_METHOD,
	KSYM_MTH,
	KSYM_NIL,
	KSYM_POINTER,
	KSYM_PRIMITIVE_COLON,
	KSYM_SELF,
	KSYM_SUPER,
	KSYM_THIS_CONTEXT,
	KSYM_TRUE,
	KSYM_WORD,

	KSYM_VBAR,
	KSYM_GT,
	KSYM_LT
};
typedef enum ksym_id_t ksym_id_t;


static STIX_INLINE int is_spacechar (stix_uci_t c)
{
	/* TODO: handle other space unicode characters */
	switch (c)
	{
		case ' ':
		case '\f': /* formfeed */
		case '\n': /* linefeed */
		case '\r': /* carriage return */
		case '\t': /* horizon tab */
		case '\v': /* vertical tab */
			return 1;

		default:
			return 0;
	}
}

static STIX_INLINE int is_alphachar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static STIX_INLINE int is_digitchar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= '0' && c <= '9');
}

static STIX_INLINE int is_alnumchar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static STIX_INLINE int is_binselchar (stix_uci_t c)
{
	/*
	 * binary-selector-character :=
	 * 	'!' | '%' | '&' | '*' | '+' | ',' | 
	 * 	'/' | '<' | '>' | '=' | '?' | '@' | 
	 * 	'\' | '~' | '|' | '-'
	 */

	switch (c)
	{
		case '!':
		case '%':
		case '&':
		case '*':
		case '+':
		case ',':
		case '/': 
		case '<':
		case '>':
		case '=':
		case '?':
		case '@':
		case '\\':
		case '|':
		case '~':
		case '-':
			return 1;

		default:
			return 0;
	}
}

static STIX_INLINE int is_closing_char (stix_uci_t c)
{
	switch (c)
	{
		case '.':
		case ']':
		case ')':
		case ';':
		case '\"':
		case '\'':
			return 1;
	
		default:
			return 0;
	}
}

static STIX_INLINE int does_token_name_match (stix_t* stix, ksym_id_t id)
{
	return stix->c->tok.name.len == ksyms[id].len &&
	       stix_equalchars(stix->c->tok.name.ptr, ksyms[id].str, ksyms[id].len);
}

static STIX_INLINE int is_token_symbol (stix_t* stix, ksym_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_SYMLIT && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_ident (stix_t* stix, ksym_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_IDENT && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_binsel (stix_t* stix, ksym_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_BINSEL && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_keyword (stix_t* stix, ksym_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_KEYWORD && does_token_name_match(stix, id);
}

static int begin_include (stix_t* fsc);
static int end_include (stix_t* fsc);

static void set_syntax_error (stix_t* stix, stix_synerrnum_t num, const stix_ioloc_t* loc, const stix_ucs_t* tgt)
{
	stix->errnum = STIX_ESYNTAX;
	stix->c->synerr.num = num;
	stix->c->synerr.loc = loc? *loc: stix->c->tok.loc;
	if (tgt) stix->c->synerr.tgt = *tgt;
	else 
	{
		stix->c->synerr.tgt.ptr = STIX_NULL;
		stix->c->synerr.tgt.len = 0;
	}
}

static int copy_string_to (stix_t* stix, const stix_ucs_t* src, stix_ucs_t* dst, stix_size_t* dst_capa, int append, stix_uch_t append_delim)
{
	stix_size_t len, pos;

	if (append)
	{
		pos = dst->len;
		len = dst->len + src->len;
		if (append_delim != '\0') len++;
	}
	else
	{
		pos = 0;
		len = src->len;
	}

	if (len > *dst_capa)
	{
		stix_uch_t* tmp;
		stix_size_t capa;

		capa = STIX_ALIGN(len, CLASS_BUFFER_ALIGN);

		tmp = stix_reallocmem (stix, dst->ptr, STIX_SIZEOF(*tmp) * capa);
		if (!tmp)  return -1;

		dst->ptr = tmp;
		*dst_capa = capa;
	}

	if (append && append_delim) dst->ptr[pos++] = append_delim;
	stix_copychars (&dst->ptr[pos], src->ptr, src->len);
	dst->len = len;
	return 0;
}

static stix_ssize_t find_word_in_string (const stix_ucs_t* haystack, const stix_ucs_t* name)
{
	/* this function is inefficient. but considering the typical number
	 * of arguments and temporary variables, the inefficiency can be 
	 * ignored in my opinion. the overhead to maintain the reverse lookup
	 * table from a name to an index should be greater than this simple
	 * inefficient lookup */

	stix_uch_t* t, * e;
	stix_ssize_t index;
	stix_size_t i;

	t = haystack->ptr;
	e = t + haystack->len;
	index = 0;

	while (t < e)
	{
		while (t < e && is_spacechar(*t)) t++;

		for (i = 0; i < name->len; i++)
		{
			if (t >= e || name->ptr[i] != *t) goto unmatched;
			t++;
		}
		if (t >= e || is_spacechar(*t)) return index;

	unmatched:
		while (t < e)
		{
			if (is_spacechar(*t))
			{
				t++;
				break;
			}
			t++;
		}

		index++;
	}

	return -1;
}

/* ---------------------------------------------------------------------
 * Tokenizer 
 * --------------------------------------------------------------------- */

#define GET_CHAR(stix) \
	do { if (get_char(stix) <= -1) return -1; } while (0)

#define GET_CHAR_TO(stix,c) \
	do { \
		if (get_char(stix) <= -1) return -1; \
		c = (stix)->c->lxc.c; \
	} while(0)


#define GET_TOKEN(stix) \
	do { if (get_token(stix) <= -1) return -1; } while (0)

#define ADD_TOKEN_STR(stix,s,l) \
	do { if (add_token_str(stix, s, l) <= -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(stix,c) \
	do { if (add_token_char(stix, c) <= -1) return -1; } while (0)


static STIX_INLINE int add_token_str (stix_t* stix, const stix_uch_t* ptr, stix_size_t len)
{
	stix_ucs_t tmp;

	tmp.ptr = (stix_uch_t*)ptr;
	tmp.len = len;
	return copy_string_to (stix, &tmp, &stix->c->tok.name, &stix->c->tok.name_capa, 1, '\0');
}

static STIX_INLINE int add_token_char (stix_t* stix, stix_uch_t c)
{
	stix_ucs_t tmp;

	tmp.ptr = &c;
	tmp.len = 1;
	return copy_string_to (stix, &tmp, &stix->c->tok.name, &stix->c->tok.name_capa, 1, '\0');
}

static STIX_INLINE void unget_char (stix_t* stix, const stix_iolxc_t* c)
{
	/* Make sure that the unget buffer is large enough */
	STIX_ASSERT (stix->c->nungots < STIX_COUNTOF(stix->c->ungot));
	stix->c->ungot[stix->c->nungots++] = *c;
}

static int get_char (stix_t* stix)
{
	stix_ssize_t n;

	if (stix->c->nungots > 0)
	{
		/* something in the unget buffer */
		stix->c->lxc = stix->c->ungot[--stix->c->nungots];
		return 0;
	}

	if (stix->c->curinp->b.pos >= stix->c->curinp->b.len)
	{
		n = stix->c->impl (stix, STIX_IO_READ, stix->c->curinp);
		if (n <= -1) return -1;

		if (n == 0)
		{
			stix->c->curinp->lxc.c = STIX_UCI_EOF;
			stix->c->curinp->lxc.l.line = stix->c->curinp->line;
			stix->c->curinp->lxc.l.colm = stix->c->curinp->colm;
			stix->c->curinp->lxc.l.file = stix->c->curinp->name;
			stix->c->lxc = stix->c->curinp->lxc;

			/* indicate that EOF has been read. lxc.c is also set to EOF. */
			return 0; 
		}

		stix->c->curinp->b.pos = 0;
		stix->c->curinp->b.len = n;
	}

	if (stix->c->curinp->lxc.c == STIX_UCI_NL)
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->lxc causes the line number for
		 * TOK_EOF to be the same line as the lxc newline. */
		stix->c->curinp->line++;
		stix->c->curinp->colm = 1;
	}

	stix->c->curinp->lxc.c = stix->c->curinp->buf[stix->c->curinp->b.pos++];
	stix->c->curinp->lxc.l.line = stix->c->curinp->line;
	stix->c->curinp->lxc.l.colm = stix->c->curinp->colm++;
	stix->c->curinp->lxc.l.file = stix->c->curinp->name;
	stix->c->lxc = stix->c->curinp->lxc;

	return 1; /* indicate that a normal character has been read */
}

static int skip_spaces (stix_t* stix)
{
	while (is_spacechar(stix->c->curinp->lxc.c)) GET_CHAR (stix);
	return 0;
}

static int skip_comment (stix_t* stix)
{
	stix_uci_t c = stix->c->lxc.c;
	stix_iolxc_t lc;

	if (c == '"')
	{
		/* skip up to the closing " */
		do 
		{
			GET_CHAR_TO (stix, c); 
			if (c == STIX_UCI_EOF)
			{
				/* unterminated comment */
				set_syntax_error (stix, STIX_SYNERR_CMTNC, &stix->c->lxc.l, STIX_NULL);
				return -1;
			}
		}
		while (c != '"');

		if (c == '"') GET_CHAR (stix);
		return 1; /* double-quoted comment */
	}

	/* handle #! or ## */
	if (c != '#') return 0; /* not a comment */

	/* save the last character */
	lc = stix->c->lxc;
	/* read a new character */
	GET_CHAR_TO (stix, c);

	if (c == '!' || c == '#') 
	{
		do 
		{
			GET_CHAR_TO (stix, c);
			if (c == STIX_UCI_EOF)
			{
				break;
			}
			else if (c == STIX_UCI_NL)
			{
				GET_CHAR (stix);
				break;
			}
		} 
		while (1);

		return 1; /* single line comment led by ## or #! */
	}

	/* unget '#' */
	unget_char (stix, &stix->c->lxc);
	/* restore the previous state */
	stix->c->lxc = lc;

	return 0;
}

static int get_ident (stix_t* stix)
{
	/*
	 * identifier := alpha-char (alpha-char | digit-char)*
	 * keyword := identifier ":"
	 */

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_IDENT;

	do 
	{
		ADD_TOKEN_CHAR (stix, c);
		GET_CHAR (stix);
		c = stix->c->lxc.c;
	} 
	while (is_alnumchar(c));

	if (c == ':') 
	{
		ADD_TOKEN_CHAR (stix, c);
		stix->c->tok.type = STIX_IOTOK_KEYWORD;
		GET_CHAR (stix);
	}
	else
	{
		/* handle reserved words */
		if (is_token_ident(stix, KSYM_SELF))
		{
			stix->c->tok.type = STIX_IOTOK_SELF;
		}
		else if (is_token_ident(stix, KSYM_SUPER))
		{
			stix->c->tok.type = STIX_IOTOK_SUPER;
		}
		else if (is_token_ident(stix, KSYM_NIL))
		{
			stix->c->tok.type = STIX_IOTOK_NIL;
		}
		else if (is_token_ident(stix, KSYM_TRUE))
		{
			stix->c->tok.type = STIX_IOTOK_TRUE;
		}
		else if (is_token_ident(stix, KSYM_FALSE))
		{
			stix->c->tok.type = STIX_IOTOK_FALSE;
		}
		else if (is_token_ident(stix, KSYM_THIS_CONTEXT))
		{
			stix->c->tok.type = STIX_IOTOK_THIS_CONTEXT;
		}
	}

	return 0;
}

static int get_numlit (stix_t* stix, int negated)
{
	/* 
	 * number-literal := number | ("-" number)
	 * number := integer | float | scaledDecimal
	 * integer := decimal-integer  | radix-integer
	 * decimal-integer := digit-char+
	 * radix-integer := radix-specifier "r" radix-digit+
	 * radix-specifier := digit-char+
	 * radix-digit := digit-char | upper-alpha-char
	 *
	 * float :=  mantissa [exponentLetter exponent]
	 * mantissa := digit-char+ "." digit-char+
	 * exponent := ['-'] decimal-integer
	 * exponentLetter := 'e' | 'd' | 'q'
	 * scaledDecimal := scaledMantissa 's' [fractionalDigits]
	 * scaledMantissa := decimal-integer | mantissa
	 * fractionalDigits := decimal-integer
	 */

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_NUMLIT;

/*TODO: support a complex numeric literal */
	do 
	{
		ADD_TOKEN_CHAR(stix, c);
		GET_CHAR (stix);
		c = stix->c->lxc.c;
	} 
	while (is_digitchar(c));

	/* TODO; more */
	return 0;
}

static int get_charlit (stix_t* stix)
{
	/* 
	 * character-literal := "$" character
	 * character := normal-character | "'"
	 */

	stix_uci_t c = stix->c->lxc.c; /* even a new-line or white space would be taken */
	if (c == STIX_UCI_EOF) 
	{
		set_syntax_error (stix, STIX_SYNERR_CLTNT, &stix->c->lxc.l, STIX_NULL);
		return -1;
	}

	stix->c->tok.type = STIX_IOTOK_CHRLIT;
	ADD_TOKEN_CHAR(stix, c);
	GET_CHAR (stix);
	return 0;
}

static int get_strlit (stix_t* stix)
{
	/* 
	 * string-literal := single-quote string-character* single-quote
	 * string-character := normal-character | (single-quote single-quote)
	 * single-quote := "'"
	 * normal-character := character-except-single-quote
	 */

	/* TODO: C-like string */

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_STRLIT;

	do 
	{
		do 
		{
			ADD_TOKEN_CHAR (stix, c);
			GET_CHAR_TO (stix, c);

			if (c == STIX_UCI_EOF) 
			{
				/* string not closed */
				set_syntax_error (stix, STIX_SYNERR_STRNC, &stix->c->tok.loc /*&stix->c->lxc.l*/, STIX_NULL);
				return -1;
			}
		} 
		while (c != '\'');

		GET_CHAR_TO (stix, c);
	} 
	while (c == '\'');

	return 0;
}

static int get_binsel (stix_t* stix)
{
	/* 
	 * binary-selector := binary-selector-character+
	 */
	stix_uci_t oc;

	oc = stix->c->lxc.c;
	ADD_TOKEN_CHAR (stix, oc);

	GET_CHAR (stix);
	/* special case if a minus is followed by a digit immediately */
	if (oc == '-' && is_digitchar(stix->c->lxc.c)) return get_numlit (stix, 1);

	/* up to 2 characters only */
	if (is_binselchar (stix->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (stix, stix->c->lxc.c);
		GET_CHAR (stix);
	}

	/* or up to any occurrences */
	/*
	while (is_binselchar(stix->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (stix, c);
		GET_CHAR (stix);
	}
	*/

	stix->c->tok.type = STIX_IOTOK_BINSEL;
	return 0;
}

static int get_token (stix_t* stix)
{
	stix_uci_t c;
	int n;

retry:
	do 
	{
		if (skip_spaces(stix) <= -1) return -1;
		if ((n = skip_comment(stix)) <= -1) return -1;
	} 
	while (n >= 1);

	/* clear the token resetting its location */
	stix->c->tok.type = STIX_IOTOK_EOF;  /* is it correct? */
	stix->c->tok.name.len = 0;

	stix->c->tok.loc = stix->c->lxc.l;
	c = stix->c->lxc.c;

	switch (c)
	{
		case STIX_UCI_EOF:
		{
			static stix_uch_t _eof_str[] = { '<', 'E', 'O', 'F', '>' };
			int n;

			n = end_include (stix);
			if (n <= -1) return -1;
			if (n >= 1) goto retry;

			stix->c->tok.type = STIX_IOTOK_EOF;
			ADD_TOKEN_STR(stix, _eof_str, 5);
			break;
		}

		case '$': /* character literal */
			GET_CHAR (stix);
			if (get_charlit(stix) <= -1) return -1;
			break;

		case '\'': /* string literal */
			GET_CHAR (stix);
			if (get_strlit(stix) <= -1) return -1;
			break;

		case ':':
			stix->c->tok.type = STIX_IOTOK_COLON;
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR (stix);

			c = stix->c->lxc.c;
			if (c == '=') 
			{
				stix->c->tok.type = STIX_IOTOK_ASSIGN;
				ADD_TOKEN_CHAR(stix, c);
				GET_CHAR (stix);
			}
			break;

		case '^':
			stix->c->tok.type = STIX_IOTOK_RETURN;
			goto single_char_token;
		case '{': /* extension */
			stix->c->tok.type = STIX_IOTOK_LBRACE;
			goto single_char_token;
		case '}': /* extension */
			stix->c->tok.type = STIX_IOTOK_RBRACE;
			goto single_char_token;
		case '[':
			stix->c->tok.type = STIX_IOTOK_LBRACK;
			goto single_char_token;
		case ']': 
			stix->c->tok.type = STIX_IOTOK_RBRACK;
			goto single_char_token;
		case '(':
			stix->c->tok.type = STIX_IOTOK_LPAREN;
			goto single_char_token;
		case ')':
			stix->c->tok.type = STIX_IOTOK_RPAREN;
			goto single_char_token;
		case '.':
			stix->c->tok.type = STIX_IOTOK_PERIOD;
			goto single_char_token;
		case ';':
			stix->c->tok.type = STIX_IOTOK_SEMICOLON;
			goto single_char_token;

		case '#':  
			/*
			 * The hash sign is not the part of the token name.
			 * ADD_TOKEN_CHAR(stix, c); */
			GET_CHAR_TO (stix, c);
			switch (c)
			{
				case STIX_UCI_EOF:
					set_syntax_error (stix, STIX_SYNERR_HLTNT, &stix->c->lxc.l, STIX_NULL);
					return -1;
				
				case '(':
					/* #( */
					ADD_TOKEN_CHAR(stix, c);
					stix->c->tok.type = STIX_IOTOK_APAREN;
					GET_CHAR (stix);
					break;

				case '[':
					/* #[ - byte array literal */
					ADD_TOKEN_CHAR(stix, c);
					stix->c->tok.type = STIX_IOTOK_BPAREN;
					GET_CHAR (stix);
					break;

				case '\'':
					/* quoted symbol literal */
					GET_CHAR (stix);
					if (get_strlit(stix) <= -1) return -1;
					stix->c->tok.type = STIX_IOTOK_SYMLIT;
					break;

				default:
					/* symbol-literal := "#" symbol-body
					 * symbol-body := identifier | keyword+ | binary-selector | string-literal
					 */ 

					/* unquoted symbol literal */
					if (is_binselchar(c))
					{
						do 
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);
						} 
						while (is_binselchar(c));
					}
					else if (is_alphachar(c))
					{
						int colon_required = 0;

					nextword:
						do 
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);
						} 
						while (is_alnumchar(c));

						if (c == ':')
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);

							if (is_alphachar(c)) 
							{
								colon_required =1;
								goto nextword;
							}
						}
						else if (colon_required)
						{
							set_syntax_error (stix, STIX_SYNERR_CLNMS, &stix->c->lxc.l, STIX_NULL);
							return -1;
						}
					}
					else
					{
						set_syntax_error (stix, STIX_SYNERR_HLTNT, &stix->c->lxc.l, STIX_NULL);
						return -1;
					}

					stix->c->tok.type = STIX_IOTOK_SYMLIT;
					break;
			}
				
			break;

		default:
			if (is_alphachar(c)) 
			{
				if (get_ident (stix) <= -1) return -1;
			}
			else if (is_digitchar(c)) 
			{
				if (get_numlit (stix, 0) <= -1) return -1;
			}
			else if (is_binselchar(c)) 
			{
				/* binary selector */
				if (get_binsel (stix) <= -1) return -1;
			}
			else 
			{
				stix->c->ilchr = (stix_uch_t)c;
				set_syntax_error (stix, STIX_SYNERR_ILCHR, &stix->c->lxc.l, &stix->c->ilchr_ucs);
				return -1;
			}
			break;

		single_char_token:
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR (stix);
			break;
	}

	return 0;
}


static void clear_io_names (stix_t* stix)
{
	stix_iolink_t* cur;
	while (stix->c->io_names)
	{
		cur = stix->c->io_names;
		stix->c->io_names = cur->link;
		stix_freemem (stix, cur);
	}
}

static const stix_uch_t* add_io_name (stix_t* stix, const stix_ucs_t* name)
{
	stix_iolink_t* link;
	stix_uch_t* ptr;

	link = (stix_iolink_t*) stix_callocmem (stix, STIX_SIZEOF(*link) + STIX_SIZEOF(stix_uch_t) * (name->len + 1));
	if (!link) return STIX_NULL;

	ptr = (stix_uch_t*)(link + 1);

	stix_copychars (ptr, name->ptr, name->len);
	ptr[name->len] = '\0';

	link->link = stix->c->io_names;
	stix->c->io_names = link;

	return ptr;
}

static int begin_include (stix_t* stix)
{
	stix_ioarg_t* arg;
	const stix_uch_t* io_name;

	io_name = add_io_name (stix, &stix->c->tok.name);
	if (!io_name) return -1;

	arg = (stix_ioarg_t*) stix_callocmem (stix, STIX_SIZEOF(*arg));
	if (!arg) goto oops;

	arg->name = io_name;
	arg->line = 1;
	arg->colm = 1;
	arg->includer = stix->c->curinp;

	if (stix->c->impl (stix, STIX_IO_OPEN, arg) <= -1) goto oops;

	stix->c->curinp = arg;
	/* stix->c->depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_char(stix) <= -1 || get_token(stix) <= -1) 
	{
		end_include (stix); 
		/* i don't jump to oops since i've called 
		 * end_include() where stix->c->curinp/arg is freed. */
		return -1;
	}

	return 0;

oops:
	if (arg) stix_freemem (stix, arg);
	return -1;
}

static int end_include (stix_t* stix)
{
	int x;
	stix_ioarg_t* cur;

	if (stix->c->curinp == &stix->c->arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call STIX_IO_CLOSE on 
	 * stix->c->curinp again. */

	cur = stix->c->curinp;
	stix->c->curinp = stix->c->curinp->includer;

	STIX_ASSERT (cur->name != STIX_NULL);
	stix_freemem (stix, cur);
	/* stix->parse.depth.incl--; */

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		return -1;
	}

	stix->c->lxc = stix->c->curinp->lxc;
	return 1; /* ended the included file successfully */
}




#if 0
/* ---------------------------------------------------------------------
 * Parser and Code Generator 
 * --------------------------------------------------------------------- */
#if 0

#define EMIT_CODE_TEST(fsc,high,low) \
	do { if (emit_code_test(fsc,high,low) <= -1) return -1; } while (0)

#define EMIT_PUSH_RECEIVER_VARIABLE(fsc,pos) \
	do {  \
		if (emit_stack_positional ( \
			fsc, PUSH_RECEIVER_VARIABLE, pos) <= -1) return -1; \
	} while (0)

#define EMIT_PUSH_TEMPORARY_LOCATION(fsc,pos) \
	do {  \
		if (emit_stack_positional ( \
			fsc, PUSH_TEMPORARY_LOCATION, pos) <= -1) return -1; \
	} while (0)

#define EMIT_PUSH_LITERAL_CONSTANT(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, PUSH_LITERAL_CONSTANT, pos) <= -1) return -1; \
	} while (0)


#define EMIT_PUSH_LITERAL_VARIABLE(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, PUSH_LITERAL_VARIABLE, pos) <= -1) return -1; \
	} while (0)

#define EMIT_STORE_RECEIVER_VARIABLE(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, STORE_RECEIVER_VARIABLE, pos) <= -1) return -1; \
	} while (0)

#define EMIT_STORE_TEMPORARY_LOCATION(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, STORE_TEMPORARY_LOCATION, pos) <= -1) return -1; \
	} while (0)


#define EMIT_POP_STACK_TOP(fsc) EMIT_CODE(fsc, POP_STACK_TOP)
#define EMIT_DUPLICATE_STACK_TOP(fsc) EMIT_CODE(fsc, DUPLICATE_STACK_TOP)
#define EMIT_PUSH_ACTIVE_CONTEXT(fsc) EMIT_CODE(fsc, PUSH_ACTIVE_CONTEXT)
#define EMIT_RETURN_FROM_MESSAGE(fsc) EMIT_CODE(fsc, RETURN_FROM_MESSAGE)
#define EMIT_RETURN_FROM_BLOCK(fsc) EMIT_CODE(fsc, RETURN_FROM_BLOCK)

#define EMIT_SEND_TO_SELF(fsc,nargs,selector) \
	do { \
		if (emit_send_to_self(fsc,nargs,selector) <= -1) return -1; \
	} while (0)

#define EMIT_SEND_TO_SUPER(fsc,nargs,selector) \
	do { \
		if (emit_send_to_super(fsc,nargs,selector) <= -1) return -1; \
	} while (0)

#define EMIT_DO_PRIMITIVE(fsc,no) \
	do { if (emit_do_primitive(fsc,no) <= -1) return -1; } while(0)

#endif

static STIX_INLINE int emit_code_test (
	stix_t* fsc, const stix_uch_t* high, const stix_uch_t* low)
{
wprintf (L"CODE: %s %s\n", high, low);
	return 0;
}

static STIX_INLINE int emit_code (stix_t* fsc, const stix_uint8_t* code, int len)
{
	int i;

	if ((fsc->bcd.len + len) > STIX_COUNTOF(fsc->bcd.buf))
	{
		stix_seterrnum (fsc, STIX_FSC_EBCDTL, STIX_NULL);
		return -1;
	}

	for (i = 0; i < len; i++) fsc->bcd.buf[fsc->bcd.len++] = code[i];
	return 0;
}

static int emit_push_stack (stix_t* fsc, stix_stack_operand_t type, int pos)
{
	/*
	 * 0-15      0000iiii           Push Receiver Variable #iiii 
	 * 16-31     0001iiii           Push Temporary Location #iiii
	 * 32-63     001iiiii           Push Literal Constant #iiiii
	 * 64-95     010iiiii           Push Literal Variable #iiiii
	 * 128       10000000 jjkkkkkk  Push (Receiver Variable, Temporary Location, Literal Constant, Literal Variable) [jj] #kkkkkk
	 */

	static int bcds[] = 
	{
		STIX_PUSH_RECEIVER_VARIABLE,
		STIX_PUSH_TEMPORARY_LOCATION,
		STIX_PUSH_LITERAL_CONSTANT,
		STIX_PUSH_LITERAL_VARIABLE 
	};
	static int bounds[] = { 0x0F, 0x0F, 0x1F, 0x1F };

	stix_uint8_t code[2];
	int len = 0;

	STIX_ASSERT (pos >= 0x0 &&  pos <= 0x3F); /* 0 to 63 */
	STIX_ASSERT (type >= STIX_RECEIVER_VARIABLE && type <= STIX_LITERAL_VARIABLE);

	if (pos <= bounds[type])
	{
		code[len++] = bcds[type] | pos;
	}
	else
	{
		code[len++] = STIX_PUSH_EXTENDED;
		code[len++] = (type << 6)  | pos;
	}

	return emit_code (fsc, code, len);
}

static int emit_store_stack (stix_t* fsc, stix_stack_operand_t type, int pos)
{
	/*
	 * 129       10000001 jjkkkkkk  Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
	 */

	stix_uint8_t code[2];
	int len = 0;

	STIX_ASSERT (pos >= 0x0 &&  pos <= 0x3F); /* 0 to 63 */
	STIX_ASSERT (type >= STIX_RECEIVER_VARIABLE && type <= STIX_LITERAL_VARIABLE);

	code[len++] = STIX_STORE_EXTENDED;
	code[len++] = (type << 6) | pos;

	return emit_code (fsc, code, len);
}

static int emit_pop_store_stack (stix_t* fsc, stix_stack_operand_t type, int pos)
{
	/*
	 * 96-103    01100iii           Pop and Store Receiver Variable #iii
	 * 104-111   01101iii           Pop and Store Temporary Location #iii
	 * 129       10000001 jjkkkkkk  Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
	 * 130       10000010 jjkkkkkk  Pop and Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
	 */

	stix_uint8_t code[2];
	int len = 0;

	static int bcds[] = 
	{
		STIX_POP_STORE_RECEIVER_VARIABLE,
		STIX_POP_STORE_TEMPORARY_LOCATION
	};

	STIX_ASSERT (pos >= 0x0 &&  pos <= 0x3F); /* 0 to 63 */
	STIX_ASSERT (type >= STIX_RECEIVER_VARIABLE && type <= STIX_LITERAL_VARIABLE && type != STIX_LITERAL_CONSTANT);

	switch (type)
	{
		case STIX_RECEIVER_VARIABLE:
		case STIX_TEMPORARY_LOCATION:
			if (pos <= 0x07) 
			{
				code[len++] = bcds[type] | pos;
				break;
			}
			/* fall through */

		default:
			code[len++] = STIX_POP_STORE_EXTENDED;
			code[len++] = (type << 6)  | pos;
			break;
	}

	return emit_code (fsc, code, len);
}

static int emit_send_message (stix_t* fsc, stix_send_target_t target, int selector, int nargs)
{
	/*
	 * 131       10000011 jjjkkkkk          Send Literal Selector #kkkkk With jjj Arguments
	 * 132       10000100 jjjjjjjj kkkkkkkk Send Literal Selector #kkkkkkkk With jjjjjjjj Arguments
	 * 133       10000101 jjjkkkkk          Send Literal Selector #kkkkk To Superclass With jjj Arguments
	 * 134       10000110 jjjjjjjj kkkkkkkk Send Literal Selector #kkkkkkkk To Superclass With jjjjjjjj Arguments
	 */

	static struct { int basic; int extended; } bcds[] =
	{
		{ STIX_SEND_TO_SELF, STIX_SEND_TO_SELF_EXTENDED },
		{ STIX_SEND_TO_SUPER, STIX_SEND_TO_SUPER_EXTENDED }
	};

	stix_uint8_t code[3];
	int len = 0;

	STIX_ASSERT (selector >= 0 && selector <= 0xFF);
	STIX_ASSERT (nargs >= 0 && nargs <= 0xFF);

	if (nargs <= 0x7 && selector <= 0x1F)
	{
		code[len++] = bcds[target].basic;
		code[len++] = (nargs << 5) | selector;
	}
	else
	{
		code[len++] = bcds[target].extended;
		code[len++] = nargs;
		code[len++] = selector;
	}

	return emit_code (fsc, code, len);
}

static int emit_do_primitive (stix_t* stix, int no)
{
	stix_uint8_t code[2];
	int len = 0;

	STIX_ASSERT (no >= 0x0 && no <= 0xFF);

	code[len++] = STIX_DO_PRIMITIVE;
	code[len++] = no;

	return emit_code (stix, code, len);
}

#if 0
static int __add_literal (stix_t* fsc, stix_word_t literal)
{
	stix_word_t i;

	for (i = 0; i < fsc->literal_count; i++) {
		/* 
		 * it would remove redundancy of symbols and small integers. 
		 * more complex redundacy check may be done somewhere else 
		 * like in __add_string_literal.
		 */
		if (fsc->literals[i] == literal) return i;
	}

	if (fsc->literal_count >= STIX_COUNTOF(fsc->literals)) {
		fsc->errnum = STIX_FSC_ERROR_TOO_MANY_LITERALS;
		return -1;
	}

	fsc->literals[fsc->literal_count++] = literal;
	return fsc->literal_count - 1;
}

static int __add_character_literal (stix_t* fsc, stix_uch_t ch)
{
	stix_word_t i, c, literal;
	stix_vm_t* stx = fsc->stx;

	for (i = 0; i < fsc->literal_count; i++) {
		c = STIX_ISSMALLINT(fsc->literals[i])? 
			stx->class_smallinteger: STIX_CLASS (stx, fsc->literals[i]);
		if (c != stx->class_character) continue;

		if (ch == STIX_CHAR_AT(stx,fsc->literals[i],0)) return i;
	}

	literal = stix_instantiate (
		stx, stx->class_character, &ch, STIX_NULL, 0);
	return __add_literal (fsc, literal);
}

static int __add_string_literal (
	stix_t* fsc, const stix_uch_t* str, stix_word_t size)
{
	stix_word_t i, c, literal;
	stix_vm_t* stx = fsc->stx;

	for (i = 0; i < fsc->literal_count; i++) {
		c = STIX_ISSMALLINT(fsc->literals[i])? 
			stx->class_smallinteger: STIX_CLASS (stx, fsc->literals[i]);
		if (c != stx->class_string) continue;

		if (stix_strxncmp (str, size, 
			STIX_DATA(stx,fsc->literals[i]), 
			STIX_SIZE(stx,fsc->literals[i])) == 0) return i;
	}

	literal = stix_instantiate (
		stx, stx->class_string, STIX_NULL, str, size);
	return __add_literal (fsc, literal);
}

static int __add_symbol_literal (
	stix_t* fsc, const stix_uch_t* str, stix_word_t size)
{
	stix_vm_t* stx = fsc->stx;
	return __add_literal (fsc, stix_new_symbolx(stx, str, size));
}

static int finish_method (stix_t* fsc)
{
	stix_vm_t* stx = fsc->stx;
	stix_oop_class_t class_obj;
	stix_method_t* method_obj;
	stix_word_t method, selector;

	STIX_ASSERT (fsc->bcd.size != 0);

	class_obj = (stix_class_t*) STIX_OBJPTR(stx, fsc->method_class);

	if (class_obj->methods == stx->nil) 
	{
		/* TODO: reconfigure method dictionary size */
		class_obj->methods = stix_instantiate (
			stx, stx->class_system_dictionary, 
			STIX_NULL, STIX_NULL, 64);
	}
	STIX_ASSERT (class_obj->methods != stx->nil);

	selector = stix_new_symbolx (
		stx, fsc->met.name.buf, fsc->method_name.size);

	method = stix_instantiate(stx, stx->class_method, 
		STIX_NULL, fsc->literals, fsc->literal_count);
	method_obj = (stix_method_t*)STIX_OBJPTR(stx, method);

	/* TODO: text saving must be optional */
	/*method_obj->text = stix_instantiate (
		stx, stx->class_string, STIX_NULL, 
		fsc->text, stix_strlen(fsc->text));
	*/
	method_obj->selector = selector;
	method_obj->bytecodes = stix_instantiate (
		stx, stx->class_bytearray, STIX_NULL, 
		fsc->bcd.buf, fsc->bcd.size);

	/* TODO: better way to store argument count & temporary count */
	method_obj->tmpcount = STIX_TO_SMALLINT(fsc->met.tmpr.count - fsc->met.tmpr.nargs);
	method_obj->argcount = STIX_TO_SMALLINT(fsc->met.tmpr.nargs);

	stix_dict_put (stx, class_obj->methods, selector, method);
	return 0;
}
#endif




#if 0
static int parse_block_statements (stix_t* fsc)
{
	while (fsc->tok.type != STIX_IOTOK_RBRACK && 
	       fsc->tok.type != STIX_IOTOK_EOF) 
	{
		if (parse_statement(fsc) <= -1) return -1;
		if (fsc->tok.type != STIX_IOTOK_PERIOD) break;
		GET_TOKEN (fsc);
	}

	return 0;
}


static int parse_basic_expression (stix_t* fsc, const stix_uch_t* ident)
{
	/*
	 * <basic expression> := <primary> [<messages> <cascaded messages>]
	 */
	int is_super;

	if (parse_primary(fsc, ident, &is_super) == -1) return -1;
	if (fsc->tok.type != STIX_IOTOK_EOF && 
	    fsc->tok.type != STIX_IOTOK_RBRACE && 
	    fsc->tok.type != STIX_IOTOK_PERIOD) 
	{
		if (parse_message_continuation(fsc, is_super) == -1) return -1;
	}
	return 0;
}

static int parse_assignment (stix_t* fsc, const stix_uch_t* target)
{
	/*
	 * <assignment> := <assignment target> assignmentOperator <expression>
	 */

	stix_word_t i;
	stix_vm_t* stx = fsc->stx;

	for (i = fsc->met.tmpr.nargs; i < fsc->met.tmpr.count; i++) 
	{
		if (stix_strequal (target, fsc->met.tmpr.names[i])) 
		{
			if (parse_expression(fsc) == -1) return -1;
			EMIT_STORE_TEMPORARY_LOCATION (fsc, i);
			return 0;
		}
	}

	if (stix_get_instance_variable_index (stx, fsc->method_class, target, &i) == 0) 
	{
		if (parse_expression(fsc) == -1) return -1;
		EMIT_STORE_RECEIVER_VARIABLE (fsc, i);
		return 0;
	}

	if (stix_lookup_class_variable (stx, fsc->method_class, target) != stx->nil) 
	{
		if (parse_expression(fsc) == -1) return -1;

		/* TODO */
		EMIT_CODE_TEST (fsc, STIX_T("ASSIGN_CLASSVAR #"), target);
		//EMIT_STORE_CLASS_VARIABLE (fsc, target);
		return 0;
	}

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	fsc->errnum = STIX_FSC_ERROR_UNDECLARED_NAME;
	return -1;
}

static int parse_primary (stix_t* fsc, const stix_uch_t* ident, int* is_super)
{
	/*
	 * <primary> :=
	 * 	identifier | <literal> | 
	 * 	<block constructor> | ( '('<expression>')' )
	 */

	stix_vm_t* stx = fsc->stx;

	if (ident == STIX_NULL) 
	{
		int pos;
		stix_word_t literal;

		*is_super = stix_false;

		if (fsc->tok.type == STIX_IOTOK_IDENT) 
		{
			if (parse_primary_ident(fsc, 
				fsc->tok.name.buffer, is_super) == -1) return -1;
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_IOTOK_CHRLIT) {
			pos = __add_character_literal(
				fsc, fsc->tok.name.buffer[0]);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_IOTOK_STRLIT) {
			pos = __add_string_literal (fsc,
				fsc->tok.name.buffer, fsc->tok.name.size);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_IOTOK_NUMLIT) 
		{
			/* TODO: other types of numbers, negative numbers, etc */
			stix_word_t tmp;
			STIX_STRTOI (tmp, fsc->tok.name.buffer, STIX_NULL, 10);
			literal = STIX_TO_SMALLINT(tmp);
			pos = __add_literal(fsc, literal);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_IOTOK_SYMLIT) {
			pos = __add_symbol_literal (fsc,
				fsc->tok.name.buffer, fsc->tok.name.size);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_IOTOK_LBRACK) {
			GET_TOKEN (fsc);
			if (parse_block_constructor(fsc) == -1) return -1;
		}
		else if (fsc->tok.type == STIX_IOTOK_APAREN) {
			/* TODO: array literal */
		}
		else if (fsc->tok.type == STIX_IOTOK_LPAREN) {
			GET_TOKEN (fsc);
			if (parse_expression(fsc) == -1) return -1;
			if (fsc->tok.type != STIX_IOTOK_RPAREN) {
				fsc->errnum = STIX_FSC_ERROR_NO_RPAREN;
				return -1;
			}
			GET_TOKEN (fsc);
		}
		else {
			fsc->errnum = STIX_FSC_ERROR_PRIMARY;
			return -1;
		}
	}
	else {
		/*if (parse_primary_ident(fsc, fsc->tok.name.buffer) == -1) return -1;*/
		if (parse_primary_ident(fsc, ident, is_super) == -1) return -1;
	}

	return 0;
}

static int parse_primary_ident (stix_t* fsc, const stix_uch_t* ident, int* is_super)
{
	stix_word_t i;
	stix_vm_t* stx = fsc->stx;

	*is_super = stix_false;

	if (stix_strequal(ident, STIX_T("self"))) 
	{
		EMIT_CODE (fsc, PUSH_RECEIVER);
		return 0;
	}
	else if (stix_strequal(ident, STIX_T("super"))) 
	{
		*is_super = stix_true;
		EMIT_CODE (fsc, PUSH_RECEIVER);
		return 0;
	}
	else if (stix_strequal(ident, STIX_T("nil"))) 
	{
		EMIT_CODE (fsc, PUSH_NIL);
		return 0;
	}
	else if (stix_strequal(ident, STIX_T("true"))) 
	{
		EMIT_CODE (fsc, PUSH_TRUE);
		return 0;
	}
	else if (stix_strequal(ident, STIX_T("false"))) 
	{
		EMIT_CODE (fsc, PUSH_FALSE);
		return 0;
	}

	/* Refer to parse_assignment for identifier lookup */

	for (i = 0; i < fsc->met.tmpr.count; i++) 
	{
		if (stix_strequal(ident, fsc->met.tmpr.names[i])) 
		{
			EMIT_PUSH_TEMPORARY_LOCATION (fsc, i);
			return 0;
		}
	}

	if (get_instance_variable_index (
		stx, fsc->method_class, ident, &i) == 0) 
	{
		EMIT_PUSH_RECEIVER_VARIABLE (fsc, i);
		return 0;
	}

	/* TODO: what is the best way to look up a class variable? */
	/* 1. Use the class containing it and using its position */
	/* 2. Use a primitive method after pushing the name as a symbol */
	/* 3. Implement a vm instruction to do it */
/*
	if (stix_lookup_class_variable (
		stx, fsc->method_class, ident) != stx->nil) {
		//EMIT_LOOKUP_CLASS_VARIABLE (fsc, ident);
		return 0;
	}
*/

	/* TODO: IMPLEMENT POOL DICTIONARIES */

	/* TODO: IMPLEMENT GLOBLAS, but i don't like this idea */

	fsc->errnum = STIX_FSC_ERROR_UNDECLARED_NAME;
	return -1;
}

static int parse_block_constructor (stix_t* fsc)
{
	/*
	 * <block constructor> := '[' <block body> ']'
	 * <block body> := [<block argument>* '|']
	 * 	[<temporaries>] [<statements>]
	 * <block argument> := ':'  identifier
	 */

	if (fsc->tok.type == STIX_IOTOK_COLON) 
	{
		do 
		{
			GET_TOKEN (fsc);

			if (fsc->tok.type != STIX_IOTOK_IDENT) 
			{
				fsc->errnum = STIX_FSC_ERROR_BLOCK_ARGUMENT_NAME;
				return -1;
			}

			/* TODO : store block arguments */
			GET_TOKEN (fsc);
		} 
		while (fsc->tok.type == STIX_IOTOK_COLON);
			
		if (!is_vbar_tok(&fsc->tok)) 
		{
			fsc->errnum = STIX_FSC_ERROR_BLOCK_ARGUMENT_LIST;
			return -1;
		}

		GET_TOKEN (fsc);
	}

	/* TODO: create a block closure */
	if (parse_method_temporaries(fsc) == -1) return -1;
	if (parse_block_statements(fsc) == -1) return -1;

	if (fsc->tok.type != STIX_IOTOK_RBRACK) 
	{
		fsc->errnum = STIX_FSC_ERROR_BLOCK_NOT_CLOSED;
		return -1;
	}

	GET_TOKEN (fsc);

	/* TODO: do special treatment for block closures */

	return 0;
}

static int parse_message_continuation (
	stix_t* fsc, int is_super)
{
	/*
	 * <messages> :=
	 * 	(<unary message>+ <binary message>* [<keyword message>] ) |
	 * 	(<binary message>+ [<keyword message>] ) |
	 * 	<keyword message>
	 * <cascaded messages> := (';' <messages>)*
	 */
	if (parse_keyword_message(fsc, is_super) == -1) return -1;

	while (fsc->tok.type == STIX_IOTOK_SEMICOLON) 
	{
		EMIT_CODE_TEST (fsc, STIX_T("DoSpecial(DUP_RECEIVER(CASCADE))"), STIX_T(""));
		GET_TOKEN (fsc);

		if (parse_keyword_message(fsc, stix_false) == -1) return -1;
		EMIT_CODE_TEST (fsc, STIX_T("DoSpecial(POP_TOP)"), STIX_T(""));
	}

	return 0;
}

static int parse_keyword_message (stix_t* fsc, int is_super)
{
	/*
	 * <keyword message> := (keyword <keyword argument> )+
	 * <keyword argument> := <primary> <unary message>* <binary message>*
	 */

	stix_name_t name;
	stix_word_t pos;
	int is_super2;
	int nargs = 0, n;

	if (parse_binary_message (fsc, is_super) == -1) return -1;
	if (fsc->tok.type != STIX_IOTOK_KEYWORD) return 0;

	if (stix_name_open(&name, 0) == STIX_NULL) {
		fsc->errnum = STIX_FSC_ERROR_MEMORY;
		return -1;
	}
	
	do 
	{
		if (stix_name_adds(&name, fsc->tok.name.buffer) == -1) 
		{
			fsc->errnum = STIX_FSC_ERROR_MEMORY;
			stix_name_close (&name);
			return -1;
		}

		GET_TOKEN (fsc);
		if (parse_primary(fsc, STIX_NULL, &is_super2) == -1) 
		{
			stix_name_close (&name);
			return -1;
		}

		if (parse_binary_message(fsc, is_super2) == -1) 
		{
			stix_name_close (&name);
			return -1;
		}

		nargs++;
		/* TODO: check if it has too many arguments.. */
	} 
	while (fsc->tok.type == STIX_IOTOK_KEYWORD);

	pos = __add_symbol_literal (fsc, name.buffer, name.size);
	if (pos == -1) 
	{
		stix_name_close (&name);
		return -1;
	}

	n = (is_super)? emit_send_to_super(fsc,nargs,pos):
	                emit_send_to_self(fsc,nargs,pos);
	if (n == -1) {
		stix_name_close (&name);
		return -1;
	}

	stix_name_close (&name);
	return 0;
}

static int parse_binary_message (stix_t* fsc, int is_super)
{
	/*
	 * <binary message> := binary-selector <binary argument>
	 * <binary argument> := <primary> <unary message>*
	 */
	stix_word_t pos;
	int is_super2;
	int n;

	if (parse_unary_message (fsc, is_super) == -1) return -1;

	while (fsc->tok.type == STIX_IOTOK_BINSEL) 
	{
		stix_uch_t* op = stix_tok_yield (&fsc->tok, 0);
		if (op == STIX_NULL) {
			fsc->errnum = STIX_FSC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (fsc);
		if (parse_primary(fsc, STIX_NULL, &is_super2) == -1) {
			stix_free (op);
			return -1;
		}

		if (parse_unary_message(fsc, is_super2) == -1) {
			stix_free (op);
			return -1;
		}

		pos = __add_symbol_literal (fsc, op, stix_strlen(op));
		if (pos == -1) {
			stix_free (op);
			return -1;
		}

		n = (is_super)?
			emit_send_to_super(fsc,2,pos):
			emit_send_to_self(fsc,2,pos);
		if (n == -1) {
			stix_free (op);
			return -1;
		}

		stix_free (op);
	}

	return 0;
}

static int parse_unary_message (stix_t* fsc, int is_super)
{
	/* <unary message> := unarySelector */

	stix_word_t pos;
	int n;

	while (fsc->tok.type == STIX_IOTOK_IDENT) 
	{
		pos = __add_symbol_literal (fsc,
			fsc->tok.name.buffer, fsc->tok.name.size);
		if (pos == -1) return -1;

		n = (is_super)? emit_send_to_super (fsc, 0, pos):
		                emit_send_to_self (fsc, 0, pos);
		if (n == -1) return -1;

		GET_TOKEN (fsc);
	}

	return 0;
}

#endif


#endif


static STIX_INLINE int set_class_name (stix_t* stix, const stix_ucs_t* name)
{
	return copy_string_to (stix, name, &stix->c->cls.name, &stix->c->cls.name_capa, 0, '\0');
}

static STIX_INLINE int set_superclass_name (stix_t* stix, const stix_ucs_t* name)
{
	return copy_string_to (stix, name, &stix->c->cls.supername, &stix->c->cls.supername_capa, 0, '\0');
}

static STIX_INLINE int append_class_level_variable (stix_t* stix, var_type_t index, const stix_ucs_t* name)
{
	int n;

	n =  copy_string_to (stix, name, &stix->c->cls.vars[index], &stix->c->cls.vars_capa[index], 1, ' ');

	if (n >= 0) 
	{
		stix->c->cls.var_count[index]++;
/* TODO: check if it exceeds STIX_MAX_NAMED_INSTVARS, STIX_MAX_CLASSVARS, STIX_MAX_CLASSINSTVARS */
	}

	return n;
}

static stix_ssize_t find_class_level_variable (stix_t* stix, stix_oop_class_t self, const stix_ucs_t* name, var_info_t* var)
{
	stix_ssize_t pos;
	stix_oop_t super;
	stix_oop_char_t v;
	stix_oop_char_t* vv;
	stix_ucs_t hs;
	int index;

	if (self)
	{
		STIX_ASSERT (STIX_CLASSOF(stix, self) == stix->_class);

		/* NOTE the loop here assumes the right order of
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &self->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = STIX_OBJ_GET_SIZE(v);

			pos = find_word_in_string(&hs, name);
			if (pos >= 0) 
			{
				super = self->superclass;
				goto done;
			}
		}

		super = self->superclass;
	}
	else
	{
		/* the class definition is not available yet */
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			pos = find_word_in_string(&stix->c->cls.vars[index], name);

			if (pos >= 0) 
			{
				super = stix->c->cls.super_oop;
				goto done;
			}
		}
		super = stix->c->cls.super_oop;
	}

	while (super != stix->_nil)
	{
		STIX_ASSERT (STIX_CLASSOF(stix, super) == stix->_class);

		/* NOTE the loop here assumes the right order of
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &((stix_oop_class_t)super)->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = STIX_OBJ_GET_SIZE(v);

			pos = find_word_in_string(&hs, name);
			if (pos >= 0) 
			{
				super = ((stix_oop_class_t)super)->superclass;
				goto done;
			}
		}

		super = ((stix_oop_class_t)super)->superclass;
	}

	return -1;

done:
	/* 'self' may be STIX_NULL if STIX_NULL has been given for it.
	 * the caller must take good care when interpreting the meaning of 
	 * this field */
	var->cls = self; 

	if (super != stix->_nil)
	{
		stix_oow_t spec;

		STIX_ASSERT (STIX_CLASSOF(stix, super) == stix->_class);
		switch (index)
		{
			case VAR_INSTANCE:
				/* each class has the number of named instance variables 
				 * accumulated for inheritance. the position found in the
				 * local variable string can be adjusted by adding the
				 * number in the superclass */
				spec = STIX_OOP_TO_SMINT(((stix_oop_class_t)super)->spec);
				pos += STIX_CLASS_SPEC_NAMED_INSTVAR(spec);
				break;

			case VAR_CLASS:
				/* no adjustment is needed.
				 * a class object is composed of three parts.
				 *  fixed-part | classinst-variables | class-variabes 
				 * the position returned here doesn't consider 
				 * class instance variables that can be potentially
				 * placed before the class variables. */
				var->cls = (stix_oop_class_t)super;
				break;

			case VAR_CLASSINST:
				spec = STIX_OOP_TO_SMINT(((stix_oop_class_t)super)->selfspec);
				pos += STIX_CLASS_SELFSPEC_CLASSINSTVAR(spec);
				break;
		}
	}

	var->type = index;
	var->pos = pos;
	return pos;
}

static int append_method_name (stix_t* stix, const stix_ucs_t* name)
{
	/* method name segments are concatenated without any delimiters */
	return copy_string_to (stix, name, &stix->c->mth.name, &stix->c->mth.name_capa, 1, '\0');
}

static stix_ssize_t find_method_name (stix_t* stix, stix_oop_class_t self, const stix_ucs_t* name)
{
	/* TODO: .................... */
	return 0;
}

static int append_temporary (stix_t* stix, const stix_ucs_t* name)
{
	/* temporary variable names are added to the string with leading
	 * space if it's not the first variable */
	return copy_string_to (stix, name, &stix->c->mth.tmprs, &stix->c->mth.tmprs_capa, 1, ' ');
}

static STIX_INLINE stix_ssize_t find_temporary (stix_t* stix, const stix_ucs_t* name)
{
	return find_word_in_string(&stix->c->mth.tmprs, name);
}

static int compile_class_level_variables (stix_t* stix)
{
	var_type_t dcl_type = VAR_INSTANCE;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process variable modifiers */
		GET_TOKEN (stix);

		if (is_token_symbol(stix, KSYM_CLASS))
		{
			/* #dcl(#class) */
			dcl_type = VAR_CLASS;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, KSYM_CLASSINST))
		{
			/* #dcl(#classinst) */
			dcl_type = VAR_CLASSINST;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	do
	{
		if (stix->c->tok.type == STIX_IOTOK_IDENT)
		{
			var_info_t var;

			if (find_class_level_variable(stix, STIX_NULL, &stix->c->tok.name, &var) >= 0)
			{
printf ("duplicate variable name type %d pos %lu\n", var.type, var.pos);
				set_syntax_error (stix, STIX_SYNERR_VARNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}

			if (append_class_level_variable(stix, dcl_type, &stix->c->tok.name) <= -1) return -1;
		}
		else
		{
			break;
		}

		GET_TOKEN (stix);
	}
	while (1);

	if (stix->c->tok.type != STIX_IOTOK_PERIOD)
	{
		set_syntax_error (stix, STIX_SYNERR_PERIOD, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_unary_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	if (append_method_name (stix, &stix->c->tok.name) <= -1) return -1;
	GET_TOKEN (stix);
	return 0;
}

static int compile_binary_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	if (append_method_name (stix, &stix->c->tok.name) <= -1) return -1;
	GET_TOKEN (stix);

	/* collect the argument name */
	if (stix->c->tok.type != STIX_IOTOK_IDENT) 
	{
		/* wrong argument name. identifier expected */
		set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	/* no duplication check is performed against class-level variable names.
	 * a duplcate name will shade a previsouly defined variable. */
	if (append_temporary(stix, &stix->c->tok.name) <= -1) return -1;
	stix->c->mth.tmpr_nargs++;

	GET_TOKEN (stix);
	return 0;
}

static int compile_keyword_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	do 
	{
		if (append_method_name(stix, &stix->c->tok.name) <= -1) return -1;

		GET_TOKEN (stix);
		if (stix->c->tok.type != STIX_IOTOK_IDENT) 
		{
			/* wrong argument name. identifier is expected */
			set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (find_temporary(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_ARGNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (append_temporary(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_nargs++;

		GET_TOKEN (stix);
	} 
	while (stix->c->tok.type == STIX_IOTOK_KEYWORD);

	return 0;
}

static int compile_method_name (stix_t* stix)
{
	/* 
	 * method-name := unary-method-name | binary-method-name | keyword-method-name
	 * unary-method-name := unary-selector
	 * binary-method-name := binary-selector selector-argument
	 * keyword-method-name := (keyword selector-argument)+
	 * selector-argument := identifier
	 * unary-selector := identifier
	 */
	int n;

	STIX_ASSERT (stix->c->mth.tmpr_count == 0);

	stix->c->mth.name_loc = stix->c->tok.loc;
	switch (stix->c->tok.type)
	{
		case STIX_IOTOK_IDENT:
			n = compile_unary_method_name(stix);
			break;

		case STIX_IOTOK_BINSEL:
			n = compile_binary_method_name(stix);
			break;

		case STIX_IOTOK_KEYWORD:
			n = compile_keyword_method_name(stix);
			break;

		default:
			/* illegal method name  */
			set_syntax_error (stix, STIX_SYNERR_MTHNAME, &stix->c->tok.loc, &stix->c->tok.name);
			n = -1;
	}

	if (n >= 0)
	{
		if (find_method_name(stix, stix->c->cls.self_oop, &stix->c->mth.name) >= 0) 
 		{
			set_syntax_error (stix, STIX_SYNERR_MTHNAMEDUP, &stix->c->mth.name_loc, &stix->c->mth.name);
			return -1;
 		}
	}

	/* the total number of temporaries is equal to the number of 
	 * arguments after having processed the message pattern. it's because
	 * stix treats arguments the same as temporaries */
	stix->c->mth.tmpr_count = stix->c->mth.tmpr_nargs;
	return n;
}

static int compile_method_temporaries (stix_t* stix)
{
	/* 
	 * method-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */

	if (!is_token_binsel(stix, KSYM_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (stix);
	while (stix->c->tok.type == STIX_IOTOK_IDENT) 
	{
		if (find_temporary(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (append_temporary(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_count++;

		GET_TOKEN (stix);
	}

	if (!is_token_binsel(stix, KSYM_VBAR)) 
	{
		set_syntax_error (stix, STIX_SYNERR_VBAR, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_method_primitive (stix_t* stix)
{
	/* 
	 * method-primitive := "<"  "primitive:" integer ">"
	 */

	if (!is_token_binsel(stix, KSYM_LT)) 
	{
		/* return if < is not seen. it is not an error condition */
		return 0;
	}

	GET_TOKEN (stix);
	if (!is_token_keyword(stix, KSYM_PRIMITIVE_COLON))
	{
		set_syntax_error (stix, STIX_SYNERR_PRIMITIVE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}
/* TODO: other modifiers? 
 * <primitive: 10>
 * <some-other-modifier: xxxx>
 */

	GET_TOKEN (stix); /* TODO: only integer */
	if (stix->c->tok.type != STIX_IOTOK_NUMLIT) 
	{
		set_syntax_error (stix, STIX_SYNERR_INTEGER, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

/*TODO: more checks the validity of the primitive number */
#if 0
	if (!stix_stristype(stix->tok.name.buffer, stix_isdigit)) 
	{
		stix->errnum = STIX_FSC_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

	STIX_STRTOI (stix->c->mth.prim_no, stix->tok.name.buffer, STIX_NULL, 10);
	if (prim_no < 0 || prim_no > 0xFF) 
	{
		stix->errnum = STIX_FSC_ERROR_PRIMITIVE_NUMBER_RANGE;
		return -1;
	}

	EMIT_DO_PRIMITIVE (stix, stix-.c->fun.prim_no); 
#endif

	GET_TOKEN (stix);
	if (!is_token_binsel(stix, KSYM_GT)) 
	{
		set_syntax_error (stix, STIX_SYNERR_GT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_class_method_expression (stix_t* stix)
{
	/*
	 * method-expression := assignment-expression | basic-expression
	 * assignment-expression := identifier ":=" method-expression
	 * basic-expression := expression-primary (message cascaded-message)?
	 */

#if 0
	if (stix->tok.type == STIX_IOTOK_IDENT) 
	{
		stix_uch_t* ident = stix_tok_yield (&stix->tok, 0);
		if (ident == STIX_NULL) 
		{
			stix->errnum = STIX_FSC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (stix);
		if (stix->tok.type == STIX_IOTOK_ASSIGN) 
		{
			GET_TOKEN (stix);
			if (parse_assignment(stix, ident) <= -1) 
			{
				stix_free (ident);
				return -1;
			}
		}
		else 
		{
			if (parse_basic_expression(stix, ident) <= -1) 
			{
				stix_free (ident);
				return -1;
			}
		}

		stix_freemem (ident);
	}
	else 
	{
		if (parse_basic_expression(stix, STIX_NULL) <= -1) return -1;
	}

	return 0;
#endif

	return -1;
}

static int compile_class_method_statement (stix_t* stix)
{
	if (stix->c->tok.type == STIX_IOTOK_RETURN) 
	{
		GET_TOKEN (stix);
		if (compile_class_method_expression(stix) <= -1) return -1;
#if 0
		EMIT_RETURN_FROM_MESSAGE (stix);
#endif
	}
	else 
	{
		if (compile_class_method_expression(stix) <= -1) return -1;
	}

	return 0;
}


static int compile_method_statements (stix_t* stix)
{
	/*
	 * method-statements := method-statement ("." | ("." method-statements))*
	 * method-statement := method-return | method-expression
	 * method-return := "^" method-expression
	 * method-expression := ...
	 */

	if (stix->c->tok.type != STIX_IOTOK_EOF &&
	    stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		do
		{
			if (compile_class_method_statement(stix) <= -1) return -1;

			if (stix->c->tok.type == STIX_IOTOK_PERIOD) 
			{
				/* period after a statement */
				GET_TOKEN (stix);
				if (stix->c->tok.type == STIX_IOTOK_EOF && stix->c->tok.type == STIX_IOTOK_RBRACE) break;
			}
			else 
			{
				if (stix->c->tok.type == STIX_IOTOK_EOF && stix->c->tok.type == STIX_IOTOK_RBRACE) break;
				set_syntax_error (stix, STIX_SYNERR_PERIOD, &stix->c->tok.loc, &stix->c->tok.name);
			}
		}
		while (1);
	}

#if 0
	EMIT_CODE (stix, RETURN_RECEIVER);
#endif
	return 0;
}

static int compile_class_method (stix_t* stix)
{
	/* clear data required to compile a method */
	stix->c->mth.flags = 0;
	stix->c->mth.name.len = 0;
	STIX_MEMSET (&stix->c->mth.name_loc, 0, STIX_SIZEOF(stix->c->mth.name_loc));
	stix->c->mth.tmprs.len = 0;
	stix->c->mth.tmpr_count = 0;
	stix->c->mth.tmpr_nargs = 0;
	stix->c->mth.code.len = 0;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process method modifiers  */
		GET_TOKEN (stix);

		if (is_token_symbol(stix, KSYM_CLASS))
		{
			/* #method(#class) */
			stix->c->mth.flags |= MTH_CLASS;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			/* ) expected */
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (compile_method_name(stix) <= -1) return -1;

	if (stix->c->tok.type != STIX_IOTOK_LBRACE)
	{
		/* { expected */
		set_syntax_error (stix, STIX_SYNERR_LBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);

	if (compile_method_temporaries(stix) <= -1 ||
	    compile_method_primitive(stix) <= -1 ||
	    compile_method_statements(stix) <= -1 /*|| 
	    finish_method(stix) <= -1*/) return -1; 

	if (stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		/* } expected */
		set_syntax_error (stix, STIX_SYNERR_RBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}
	GET_TOKEN (stix);

	return 0;
}

static int make_defined_class (stix_t* stix)
{
	/* this function make a class object with no functions/methods */

	stix_oop_t tmp;
	stix_oow_t spec, self_spec;
	int just_made = 0;

	spec = STIX_CLASS_SPEC_MAKE (stix->c->cls.var_count[VAR_INSTANCE],  
	                             ((stix->c->cls.flags & CLASS_INDEXED)? 1: 0),
	                             stix->c->cls.indexed_type);
	self_spec = STIX_CLASS_SELFSPEC_MAKE(stix->c->cls.var_count[VAR_CLASS], stix->c->cls.var_count[VAR_CLASSINST]);

printf ("MAKING ... ");
print_ucs (&stix->c->cls.name);
printf (" instvars %d classvars %d classinstvars %d\n", (int)stix->c->cls.var_count[VAR_INSTANCE], (int)stix->c->cls.var_count[VAR_CLASS], (int)stix->c->cls.var_count[VAR_CLASSINST]);

	if (stix->c->cls.self_oop)
	{
		STIX_ASSERT (STIX_CLASSOF(stix, stix->c->cls.self_oop) == stix->_class);
		STIX_ASSERT (STIX_OBJ_GET_FLAGS_KERNEL (stix->c->cls.self_oop) == 1);

		if (spec != STIX_OOP_TO_SMINT(stix->c->cls.self_oop->spec) ||
		    self_spec != STIX_OOP_TO_SMINT(stix->c->cls.self_oop->selfspec))
		{
			/* it conflicts with internal definition */


printf (" CONFLICTING CLASS DEFINITION %lu %lu %lu %lu\n", 
		(unsigned long)spec, (unsigned long)self_spec,
		(unsigned long)STIX_OOP_TO_SMINT(stix->c->cls.self_oop->spec), (unsigned long)STIX_OOP_TO_SMINT(stix->c->cls.self_oop->selfspec)
);

			set_syntax_error (stix, STIX_SYNERR_CLASSCONTRA, &stix->c->cls.name_loc, &stix->c->cls.name);
			return -1;
		}
	}
	else
	{
		/* the class variables and class instance variables are placed
		 * inside the class object after the fixed part. */
		tmp = stix_instantiate (stix, stix->_class, STIX_NULL,
		                        stix->c->cls.var_count[VAR_CLASSINST] + stix->c->cls.var_count[VAR_CLASS]);
		if (!tmp) return -1;
	
		just_made = 1;
		stix->c->cls.self_oop = (stix_oop_class_t)tmp;

		STIX_ASSERT (STIX_CLASSOF(stix, stix->c->cls.self_oop) == stix->_class);

		stix->c->cls.self_oop->spec = STIX_OOP_FROM_SMINT(spec);
		stix->c->cls.self_oop->selfspec = STIX_OOP_FROM_SMINT(self_spec);
	}

	STIX_OBJ_SET_FLAGS_KERNEL (stix->c->cls.self_oop, 2);

	tmp = stix_makesymbol(stix, stix->c->cls.name.ptr, stix->c->cls.name.len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->name = (stix_oop_char_t)tmp;

	tmp = stix_makestring(stix, stix->c->cls.vars[0].ptr, stix->c->cls.vars[0].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->instvars = (stix_oop_char_t)tmp;

	tmp = stix_makestring(stix, stix->c->cls.vars[1].ptr, stix->c->cls.vars[1].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->classvars = (stix_oop_char_t)tmp;

	tmp = stix_makestring(stix, stix->c->cls.vars[2].ptr, stix->c->cls.vars[2].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->classinstvars = (stix_oop_char_t)tmp;

/* TODO: initialize more fields??? method_dictionary. */

	if (just_made)
	{
		/* register the class to the system dictionary */
		if (!stix_putatsysdic(stix, (stix_oop_t)stix->c->cls.self_oop->name, (stix_oop_t)stix->c->cls.self_oop)) return -1;
	}

	return 0;
}

static int __compile_class_definition (stix_t* stix)
{
	/* 
	 * class-definition := #class class-modifier? "{" class-body "}"
	 * class-modifier := "(" (#byte | #character | #word | #pointer)? ")"
	 * class-body := variable-definition* method-definition*
	 * 
	 * variable-definition := (#dcl | #declare) variable-modifier? variable-list "."
	 * variable-modifier := "(" (#class | #classinst)? ")"
	 * variable-list := identifier*
	 *
	 * method-definition := (#mth | #method) method-modifier? method-actual-definition
	 * method-modifier := "(" (#class | #instance)? ")"
	 * method-actual-definition := method-name "{" method-tempraries? method-primitive? method-statements* "}"
	 */
	stix_oop_association_t ass;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process class modifiers */

		GET_TOKEN (stix);

		if (is_token_symbol(stix, KSYM_BYTE))
		{
			/* #class(#byte) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_BYTE;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, KSYM_CHARACTER))
		{
			/* #class(#character) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_CHAR;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, KSYM_WORD))
		{
			/* #class(#word) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_WORD;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, KSYM_POINTER))
		{
			/* #class(#pointer) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_OOP;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (stix->c->tok.type != STIX_IOTOK_IDENT)
	{
		/* class name expected. */
		set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	/* copy the class name */
	if (set_class_name(stix, &stix->c->tok.name) <= -1) return -1;
	stix->c->cls.name_loc = stix->c->tok.loc;
	GET_TOKEN (stix); 

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
printf ("DEFININING..\n");
{
int i;
for (i = 0; i < stix->c->cls.name.len; i++)
{
printf ("%c", stix->c->cls.name.ptr[i]);
}
printf ("\n");
}
		int super_is_nil = 0;

		/* superclass is specified. new class defintion.
		 * for example, #class Class(Stix) 
		 */
		GET_TOKEN (stix); /* read superclass name */

		/* TODO: multiple inheritance */

		if (stix->c->tok.type == STIX_IOTOK_NIL)
		{
			super_is_nil = 1;
		}
		else if (stix->c->tok.type != STIX_IOTOK_IDENT)
		{
			/* superclass name expected */
			set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (set_superclass_name(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->cls.supername_loc = stix->c->tok.loc;

		GET_TOKEN (stix);
		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);

		ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.name);
		if (ass)
		{
			if (STIX_CLASSOF(stix, ass->value) != stix->_class  ||
			    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) > 1)
			{
				/* the object found with the name is not a class object 
				 * or the the class object found is a fully defined kernel 
				 * class object */
				set_syntax_error (stix, STIX_SYNERR_CLASSDUP, &stix->c->cls.name_loc, &stix->c->cls.name);
				return -1;
			}
			
			stix->c->cls.self_oop = (stix_oop_class_t)ass->value;
		}
		else
		{
			/* no class of such a name is found. it's a new definition,
			 * which is normal for most new classes. */
			STIX_ASSERT (stix->c->cls.self_oop == STIX_NULL);
		}

		if (super_is_nil)
		{
			stix->c->cls.super_oop = stix->_nil;
		}
		else
		{
			ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.supername);
			if (ass &&
			    STIX_CLASSOF(stix, ass->value) == stix->_class &&
			    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) != 1) 
			{
				/* the value found must be a class and it must not be 
				 * an incomplete internal class object */
				stix->c->cls.super_oop = ass->value;
			}
			else
			{
				/* there is no object with such a name. or,
				 * the object found with the name is not a class object. or,
				 * the class object found is a internally defined kernel
				 * class object. */
				set_syntax_error (stix, STIX_SYNERR_CLASSUNDEF, &stix->c->cls.supername_loc, &stix->c->cls.supername);
				return -1;
			}
		}
	}
	else
	{
		/* extending class */
		if (stix->c->cls.flags != 0)
		{
			/* the class definition specified with modifiers cannot extend 
			 * an existing class. the superclass must be specified enclosed
			 * in parentheses. an opening parenthesis is expected to specify
			 * a superclass here. */
			set_syntax_error (stix, STIX_SYNERR_LPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		stix->c->cls.flags |= CLASS_EXTENDED;

		ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.name);
		if (ass && 
		    STIX_CLASSOF(stix, ass->value) != stix->_class &&
		    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) != 1)
		{
			stix->c->cls.self_oop = (stix_oop_class_t)ass->value;
		}
		else
		{
			/* only an existing class can be extended. */
			set_syntax_error (stix, STIX_SYNERR_CLASSUNDEF, &stix->c->cls.name_loc, &stix->c->cls.name);
			return -1;
		}

		stix->c->cls.super_oop = stix->c->cls.self_oop->superclass;

		STIX_ASSERT ((stix_oop_t)stix->c->cls.super_oop == stix->_nil || 
		             STIX_CLASSOF(stix, stix->c->cls.super_oop) == stix->_class);
	}

	if (stix->c->tok.type != STIX_IOTOK_LBRACE)
	{
		set_syntax_error (stix, STIX_SYNERR_LBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	if (stix->c->cls.super_oop != stix->_nil)
	{
		/* adjust the instance variable count and the class instance variable
		 * count to include that of a superclass */
		stix_oop_class_t c;
		stix_oow_t spec, self_spec;

		c = (stix_oop_class_t)stix->c->cls.super_oop;
		spec = STIX_OOP_TO_SMINT(c->spec);
		self_spec = STIX_OOP_TO_SMINT(c->selfspec);
		stix->c->cls.var_count[VAR_INSTANCE] = STIX_CLASS_SPEC_NAMED_INSTVAR(spec);
		stix->c->cls.var_count[VAR_CLASSINST] = STIX_CLASS_SELFSPEC_CLASSINSTVAR(self_spec);
	}

	GET_TOKEN (stix);

	if (stix->c->cls.flags & CLASS_EXTENDED)
	{
		/* when a class is extended, a new variable cannot be added */
		if (is_token_symbol(stix, KSYM_DCL) || is_token_symbol(stix, KSYM_DECLARE))
		{
			set_syntax_error (stix, STIX_SYNERR_DCLBANNED, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}
	}
	else
	{
		/* a new class including an internally defined class object */
		while (is_token_symbol(stix, KSYM_DCL) || is_token_symbol(stix, KSYM_DECLARE))
		{
			/* variable definition. #dcl or #declare */
			GET_TOKEN (stix);
			if (compile_class_level_variables(stix) <= -1) return -1;
		}

		if (make_defined_class(stix) <= -1) return -1;
	}

	while (is_token_symbol(stix, KSYM_MTH) || is_token_symbol(stix, KSYM_METHOD))
	{
		/* method definition. #mth or #method */
		GET_TOKEN (stix);
		if (compile_class_method(stix) <= -1) return -1;
	}
	
	if (stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		set_syntax_error (stix, STIX_SYNERR_RBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_class_definition (stix_t* stix)
{
	int n;
	stix_size_t i;

	/* reset the structure to hold information about a class to be compiled */
	stix->c->cls.flags = 0;

	stix->c->cls.name.len = 0;
	stix->c->cls.supername.len = 0;
	STIX_MEMSET (&stix->c->cls.name_loc, 0, STIX_SIZEOF(stix->c->cls.name_loc));
	STIX_MEMSET (&stix->c->cls.supername_loc, 0, STIX_SIZEOF(stix->c->cls.supername_loc));

	for (i = 0; i < STIX_COUNTOF(stix->c->cls.var_count); i++) 
		stix->c->cls.var_count[i] = 0;

	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;

	/* do main compilation work */
	n = __compile_class_definition (stix);

	/* reset these oops not to confuse gc_compiler() */
	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;

	return n;
}

static int compile_stream (stix_t* stix)
{
	GET_CHAR (stix);
	GET_TOKEN (stix);

	while (stix->c->tok.type != STIX_IOTOK_EOF)
	{
		if (is_token_symbol(stix, KSYM_INCLUDE))
		{
			/* #include 'xxxx' */
			GET_TOKEN (stix);
			if (stix->c->tok.type != STIX_IOTOK_STRLIT)
			{
				set_syntax_error (stix, STIX_SYNERR_STRING, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}
			if (begin_include(stix) <= -1) return -1;
		}
		else if (is_token_symbol(stix, KSYM_CLASS))
		{
			/* #class Selfclass(Superclass) { } */
			GET_TOKEN (stix);
			if (compile_class_definition(stix) <= -1) return -1;
		}
#if 0
		else if (is_token_symbol(stix, KSYM_MAIN))
		{
			/* #main */
			/* TODO: implement this */
			
		}
#endif

		else
		{
			set_syntax_error(stix, STIX_SYNERR_DIRECTIVE, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}
	}

	return 0;
}

static void gc_compiler (stix_t* stix)
{
	/* called when garbage collection is performed */
	if (stix->c)
	{
		if (stix->c->cls.self_oop) 
			stix->c->cls.self_oop = (stix_oop_class_t)stix_moveoop (stix, (stix_oop_t)stix->c->cls.self_oop);

		if (stix->c->cls.super_oop)
			stix->c->cls.super_oop = stix_moveoop (stix, stix->c->cls.super_oop);
	}
}

static void fini_compiler (stix_t* stix)
{
	/* called before the stix object is closed */
	if (stix->c)
	{
		stix_size_t i;

		clear_io_names (stix);

		if (stix->c->tok.name.ptr) stix_freemem (stix, stix->c->tok.name.ptr);

		if (stix->c->cls.name.ptr) stix_freemem (stix, stix->c->cls.name.ptr);
		if (stix->c->cls.supername.ptr) stix_freemem (stix, stix->c->cls.supername.ptr);

		for (i = 0; i < STIX_COUNTOF(stix->c->cls.vars); i++)
		{
			if (stix->c->cls.vars[i].ptr) stix_freemem (stix, stix->c->cls.vars[i].ptr);
		}

		if (stix->c->mth.tmprs.ptr) stix_freemem (stix, stix->c->mth.tmprs.ptr);
		if (stix->c->mth.name.ptr) stix_freemem (stix, stix->c->mth.name.ptr);
		if (stix->c->mth.code.ptr) stix_freemem (stix, stix->c->mth.code.ptr);

		stix_freemem (stix, stix->c);
		stix->c = STIX_NULL;
	}
}

int stix_compile (stix_t* stix, stix_ioimpl_t io)
{
	int n;

	if (!io)
	{
		stix->errnum = STIX_EINVAL;
		return -1;
	}

	if (!stix->c)
	{
		stix_cb_t cb, * cbp;

		STIX_MEMSET (&cb, 0, STIX_SIZEOF(cb));
		cb.gc = gc_compiler;
		cb.fini = fini_compiler;
		cbp = stix_regcb (stix, &cb);
		if (!cbp) return -1;

		stix->c = stix_callocmem (stix, STIX_SIZEOF(*stix->c));
		if (!stix->c) 
		{
			stix_deregcb (stix, cbp);
			return -1;
		}

		stix->c->ilchr_ucs.ptr = &stix->c->ilchr;
		stix->c->ilchr_ucs.len = 1;
	}

	stix->c->impl = io;
	stix->c->arg.line = 1;
	stix->c->arg.colm = 1;
	stix->c->curinp = &stix->c->arg;
	clear_io_names (stix);

	/* open the top-level stream */
	n = stix->c->impl (stix, STIX_IO_OPEN, stix->c->curinp);
	if (n <= -1) return -1;

	if (compile_stream (stix) <= -1) goto oops;

	/* close the stream */
	STIX_ASSERT (stix->c->curinp == &stix->c->arg);
	stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (stix->c->curinp != &stix->c->arg)
	{
		stix_ioarg_t* prev;

		/* nothing much to do about a close error */
		stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

		prev = stix->c->curinp->includer;
		STIX_ASSERT (stix->c->curinp->name != STIX_NULL);
		STIX_MMGR_FREE (stix->mmgr, stix->c->curinp);
		stix->c->curinp = prev;
	}

	stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);
	return -1;
}

void stix_getsynerr (stix_t* stix, stix_synerr_t* synerr)
{
	STIX_ASSERT (stix->c != STIX_NULL);
	if (synerr) *synerr = stix->c->synerr;
}
