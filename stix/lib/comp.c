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

#if 0
enum class_type_t
{
	CLASS_NORMAL = 0,
	CLASS_VARIABLE,
	CLASS_VARIABLE_BYTE,
	CLASS_VARIABLE_CHAR
};
typedef enum class_type_t class_type_t;

enum vardef_type_t
{
	VARDEF_INSTANCE,
	VARDEF_CLASS_INSTANCE,
	VARDEF_CLASS
};
typedef enum vardef_type_t vardef_type_t;

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

static void clear_sio_names (stix_t* fsc);
static int begin_include (stix_t* fsc);
static int end_include (stix_t* fsc);

stix_t* stix_open (stix_mmgr_t* mmgr, stix_size_t xtnsize, stix_size_t vmheapsize)
{
	stix_t* fsc;

	fsc = STIX_MMGR_ALLOC (mmgr, STIX_SIZEOF(*fsc) + xtnsize);
	if (fsc)
	{
		if (stix_init (fsc, mmgr, vmheapsize) <= -1)
		{
			STIX_MMGR_FREE (mmgr, fsc);
			fsc = STIX_NULL;
		}
		else STIX_MEMSET (fsc + 1, 0, xtnsize);
	}

	return fsc;
}

void stix_close (stix_t* fsc)
{
	stix_fini (fsc);
	STIX_MMGR_FREE (fsc->mmgr, fsc);
}

int stix_init (stix_t* fsc, stix_mmgr_t* mmgr, stix_size_t vmheapsize)
{
	int trait;

	STIX_MEMSET (fsc, 0, STIX_SIZEOF(*fsc));
	fsc->mmgr = mmgr;
	fsc->errstr = stix_dflerrstr;

	fsc->vm = stix_vm_open (mmgr, 0, vmheapsize);
	if (fsc->vm == STIX_NULL) 
	{
		fsc->errnum = STIX_FSC_ENOMEM;
		return -1;
	}

	/* turn off garbage collection by force */
	stix_vm_getopt (fsc->vm, STIX_VM_TRAIT, &trait);
	trait |= STIX_VM_NOGC;
	stix_vm_setopt (fsc->vm, STIX_VM_TRAIT, &trait);

	if (stix_vm_boom (fsc->vm) <= -1)
	{
		/* TODO: translate vm error to fsc error */
fsc->errnum = STIX_FSC_ENOMEM;
		stix_vm_close (fsc->vm);
		return -1;
	}

#if 0
	if (stix_tok_open (&fsc->tok, 0) == STIX_NULL) 
	{
		if (fsc->__dynamic) stix_free (fsc);
		return STIX_NULL;
	}

	if (stix_arr_open (
		&fsc->bcd, 256, 
		stix_sizeof(stix_uint8_t), STIX_NULL) == STIX_NULL) 
	{
		stix_tok_close (&fsc->tok);
		if (fsc->__dynamic) stix_free (fsc);
		return STIX_NULL;
	}

	fsc->stx = stx;
	fsc->errnum = STIX_FSC_ERROR_NONE;

	fsc->met.tmpr.count = 0;
	fsc->met.tmpr.nargs = 0;
	fsc->literal_count = 0;

	fsc->sio.lxc.c = STIX_CHAR_EOF;
	fsc->ungotc_count = 0;

	fsc->input_owner = STIX_NULL;
	fsc->input_func = STIX_NULL;
	return fsc;
#endif

	return 0;
}

void stix_fini (stix_t* fsc)
{
	stix_vm_close (fsc->vm);

#if 0
	stix_arr_close (&fsc->bcd);
#endif

	clear_sio_names (fsc);
}

/* ---------------------------------------------------------------------
 * Tokenizer 
 * --------------------------------------------------------------------- */

static STIX_INLINE int is_binselchar (stix_cint_t c)
{
	/*
	 * binarySelectorCharacter ::=
	 * 	'!' | '%' | '&' | '*' | '+' | ',' | 
	 * 	'/' | '<' | '>' | '=' | '?' | '@' | 
	 * 	'\' | '~' | '|' | '-'
	 */

	switch (c)
	{
		case STIX_T('!'):
		case STIX_T('%'):
		case STIX_T('&'):
		case STIX_T('*'):
		case STIX_T('+'):
		case STIX_T(','):
		case STIX_T('/'): 
		case STIX_T('<'):
		case STIX_T('>'):
		case STIX_T('='):
		case STIX_T('?'):
		case STIX_T('@'):
		case STIX_T('\\'):
		case STIX_T('|'):
		case STIX_T('~'):
		case STIX_T('-'):
			return 1;
		
		default:
			return 0;
	}
}

static STIX_INLINE int is_closing_char (stix_cint_t c)
{
	return 
		c == STIX_T('.') || c == STIX_T(']') ||
		c == STIX_T(')') || c == STIX_T(';') ||
		c == STIX_T('\"') || c == STIX_T('\'');
}

#define GET_CHAR(fsc) \
	do { if (get_char (fsc) == -1) return -1; } while (0)

#define GET_TOKEN(fsc) \
	do { if (get_token (fsc) == -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(fsc,c) \
	do { if (add_token_char (fsc, c) == -1) return -1; } while (0)

#define ADD_TOKEN_STR(fsc,s) \
	do { if (add_token_str (fsc, s) == -1) return -1; } while (0)

static STIX_INLINE int add_token_char (stix_t* fsc, stix_char_t c)
{
	if (fsc->tok.name.len >= STIX_COUNTOF(fsc->tok.buf) - 1)
	{
		/* the tok buffer is full. cannot add more characters to it */
		fsc->errnum = STIX_FSC_ETOKTL;
		return -1;
	}
	fsc->tok.buf[fsc->tok.name.len++] = c;
	fsc->tok.buf[fsc->tok.name.len] = STIX_T('\0');
	return 0;
}

static STIX_INLINE int add_token_str (stix_t* fsc, const stix_char_t* str)
{
	stix_size_t len;

	len = stix_strlen (str);
	if (fsc->tok.name.len + len >= STIX_COUNTOF(fsc->tok.buf) - 1)
	{
		/* the tok buffer is full. cannot add more characters to it */
		fsc->errnum = STIX_FSC_ETOKTL;
		return -1;
	}

	fsc->tok.name.len += stix_strcpy (&fsc->tok.buf[fsc->tok.name.len], str);
	return 0;
}

static int get_char (stix_t* fsc)
{
	stix_ssize_t n;

	if (fsc->sio.inp->b.pos >= fsc->sio.inp->b.len)
	{
		n = fsc->sio.impl (fsc, STIX_FSC_IO_READ, fsc->sio.inp);
		if (n <= -1) return -1;

		if (n == 0)
		{
			fsc->sio.inp->lxc.c = STIX_CHAR_EOF;
			fsc->sio.inp->lxc.line = fsc->sio.inp->line;
			fsc->sio.inp->lxc.colm = fsc->sio.inp->colm;
			fsc->sio.inp->lxc.file = fsc->sio.inp->name;
			fsc->sio.lxc = fsc->sio.inp->lxc;
			return 0;
		}

		fsc->sio.inp->b.pos = 0;
		fsc->sio.inp->b.len = n;	
	}

	if (fsc->sio.inp->lxc.c == STIX_T('\n'))
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->lxc causes the line number for
		 * TOK_EOF to be the same line as the lxc newline. */
		fsc->sio.inp->line++;
		fsc->sio.inp->colm = 1;
	}
	
	fsc->sio.inp->lxc.c = fsc->sio.inp->buf[fsc->sio.inp->b.pos++];
	fsc->sio.inp->lxc.line = fsc->sio.inp->line;
	fsc->sio.inp->lxc.colm = fsc->sio.inp->colm++;
	fsc->sio.inp->lxc.file = fsc->sio.inp->name;
	fsc->sio.lxc = fsc->sio.inp->lxc;
	return 0;
}

static int skip_spaces (stix_t* fsc)
{
	while (STIX_ISSPACE(fsc->sio.lxc.c)) GET_CHAR (fsc);
	return 0;
}

static int skip_comment (stix_t* fsc)
{
	/* comment is a double quoted string */
	while (fsc->sio.lxc.c != STIX_T('"')) GET_CHAR (fsc);
	GET_CHAR (fsc); /* skip the closing quote */
	return 0;
}

static int get_ident (stix_t* fsc)
{
	/*
	 * identifier ::= letter (letter | digit)*
	 * keyword ::= identifier ':'
	 */

	stix_cint_t c = fsc->sio.lxc.c;
	fsc->tok.type = STIX_FSC_TOK_IDENT;

	do 
	{
		ADD_TOKEN_CHAR(fsc, c);
		GET_CHAR (fsc);
		c = fsc->sio.lxc.c;
	} 
	while (STIX_ISALPHA(c) || STIX_ISDIGIT(c));

	if (c == STIX_T(':')) 
	{
		ADD_TOKEN_CHAR (fsc, c);
		fsc->tok.type = STIX_FSC_TOK_KEYWORD;
		GET_CHAR (fsc);
	}

	return 0;
}

static int get_numlit (stix_t* fsc, int negated)
{
	/* 
	 * <number literal> ::= ['-'] <number>
	 * <number> ::= integer | float | scaledDecimal
	 * integer ::= decimalInteger  | radixInteger
	 * decimalInteger ::= digits
	 * digits ::= digit+
	 * radixInteger ::= radixSpecifier  'r' radixDigits
	 * radixSpecifier := digits
	 * radixDigits ::= (digit | uppercaseAlphabetic)+
	 * float ::=  mantissa [exponentLetter exponent]
	 * mantissa ::= digits'.' digits
	 * exponent ::= ['-']decimalInteger
	 * exponentLetter ::= 'e' | 'd' | 'q'
	 * scaledDecimal ::= scaledMantissa 's' [fractionalDigits]
	 * scaledMantissa ::= decimalInteger | mantissa
	 * fractionalDigits ::= decimalInteger
	 */

	stix_cint_t c = fsc->sio.lxc.c;
	fsc->tok.type = STIX_FSC_TOK_NUMLIT;

	do 
	{
		ADD_TOKEN_CHAR(fsc, c);
		GET_CHAR (fsc);
		c = fsc->sio.lxc.c;
	} 
	while (STIX_ISALPHA(c) || STIX_ISDIGIT(c));

	/* TODO; more */
	return 0;
}

static int get_charlit (stix_t* fsc)
{
	/* 
	 * character_literal ::= '$' character
	 * character ::= "Any character in the implementation-defined character set"
	 */

	stix_cint_t c = fsc->sio.lxc.c; /* even a new-line or white space would be taken */
	if (c == STIX_CHAR_EOF) 
	{
		stix_seterrnum (fsc, STIX_FSC_ECHRNT, STIX_NULL);
		return -1;
	}	

	fsc->tok.type = STIX_FSC_TOK_CHRLIT;
	ADD_TOKEN_CHAR(fsc, c);
	GET_CHAR (fsc);
	return 0;
}

static int get_strlit (stix_t* fsc)
{
	/* 
	 * string_literal ::= stringDelimiter stringBody stringDelimiter
	 * stringBody ::= (nonStringDelimiter | (stringDelimiter stringDelimiter)*)
	 * stringDelimiter ::= '''    "a single quote"
	 */

	/* TODO: C-like string */

	stix_cint_t c = fsc->sio.lxc.c;
	fsc->tok.type = STIX_FSC_TOK_STRLIT;

	do 
	{
		do 
		{
			ADD_TOKEN_CHAR (fsc, c);
			GET_CHAR (fsc);
			c = fsc->sio.lxc.c;

			if (c == STIX_CHAR_EOF) 
			{
				stix_seterrnum (fsc, STIX_FSC_ESTRNT, STIX_NULL);
				return -1;
			}
		} 
		while (c != STIX_T('\''));

		GET_CHAR (fsc);
		c = fsc->sio.lxc.c;
	} 
	while (c == STIX_T('\''));

	return 0;
}

static int get_binsel (stix_t* fsc)
{
	/* 
	 * binarySelector ::= binarySelectorCharacter+
	 */

	ADD_TOKEN_CHAR (fsc, fsc->sio.lxc.c);

	if (fsc->sio.lxc.c == STIX_T('-')) 
	{
		/* special case if a minus is followed by a digit immediately */
		GET_CHAR (fsc);
		if (STIX_ISDIGIT(fsc->sio.lxc.c)) return get_numlit (fsc, 1);
	}
	else GET_CHAR (fsc);

	/* up to 2 characters only */
	if (is_binselchar (fsc->sio.lxc.c)) 
	{
		ADD_TOKEN_CHAR (fsc, fsc->sio.lxc.c);
		GET_CHAR (fsc);
	}

	/* or up to any occurrences */
	/*
	while (is_binselchar(fsc->sio.lxc.c)) 
	{
		ADD_TOKEN_CHAR (fsc, c);
		GET_CHAR (fsc);
	}
	*/

	fsc->tok.type = STIX_FSC_TOK_BINSEL;
	return 0;
}

static int get_token (stix_t* fsc)
{
	stix_cint_t c;

retry:
	do 
	{
		if (skip_spaces(fsc) <= -1) return -1;
		if (fsc->sio.lxc.c != STIX_T('"')) break;

		GET_CHAR (fsc);
		if (skip_comment(fsc) <= -1) return -1;
	} 
	while (1);

	/* clear the token resetting its location */
	fsc->tok.type = 0; 
	fsc->tok.buf[0] = STIX_T('\0'); 
	fsc->tok.name.len = 0;
	fsc->tok.name.ptr = fsc->tok.buf;
	fsc->tok.loc.file = fsc->sio.lxc.file;
	fsc->tok.loc.line = fsc->sio.lxc.line;
	fsc->tok.loc.colm = fsc->sio.lxc.colm;
	c = fsc->sio.lxc.c;

	switch (c)
	{
		case STIX_CHAR_EOF:
		{
			int n;

			n = end_include (fsc);
			if (n <= -1) return -1;
			if (n >= 1) goto retry;

			fsc->tok.type = STIX_FSC_TOK_EOF;
			ADD_TOKEN_STR(fsc, STIX_T("<EOF>"));
			break;
		}

		case STIX_T('$'): /* character literal */
			GET_CHAR (fsc);
			if (get_charlit(fsc) == -1) return -1;
			break;

		case STIX_T('\''): /* string literal */
			GET_CHAR (fsc);
			if (get_strlit(fsc) == -1) return -1;
			break;

		case STIX_T(':'):
			fsc->tok.type = STIX_FSC_TOK_COLON;
			ADD_TOKEN_CHAR(fsc, c);
			GET_CHAR (fsc);

			c = fsc->sio.lxc.c;
			if (c == STIX_T('=')) 
			{
				fsc->tok.type = STIX_FSC_TOK_ASSIGN;
				ADD_TOKEN_CHAR(fsc, c);
				GET_CHAR (fsc);
			}
			break;

		case STIX_T('^'):
		case STIX_T('{'): /* extension */
		case STIX_T('}'): /* extension */
		case STIX_T('['):
		case STIX_T(']'): 
		case STIX_T('('):
		case STIX_T(')'):
		case STIX_T('.'):
		case STIX_T(';'):
		case STIX_T('#'):
			switch (c) 
			{
				case STIX_T('^'):
					fsc->tok.type = STIX_FSC_TOK_RETURN;
					break;

				case STIX_T('{'):
					fsc->tok.type = STIX_FSC_TOK_LBRACE;
					break;
		
				case STIX_T('}'): 
					fsc->tok.type = STIX_FSC_TOK_RBRACE;
					break;
		
				case STIX_T('['):
					fsc->tok.type = STIX_FSC_TOK_LBRACK;
					break;
		
				case STIX_T(']'): 
					fsc->tok.type = STIX_FSC_TOK_RBRACK;
					break;

				case STIX_T('('):
					fsc->tok.type = STIX_FSC_TOK_LPAREN;
					break;
		
				case STIX_T(')'):
					fsc->tok.type = STIX_FSC_TOK_RPAREN;
					break;

				case STIX_T('.'):
					fsc->tok.type = STIX_FSC_TOK_PERIOD;
					break;
		
				case STIX_T(';'):
					fsc->tok.type = STIX_FSC_TOK_SEMICOLON;
					break;

				case STIX_T('#'):
					fsc->tok.type = STIX_FSC_TOK_HASH;
					break;
			}

			ADD_TOKEN_CHAR(fsc, c);
			GET_CHAR (fsc);
			break;

#if 0
		case STIX_T('#'):  
			/*ADD_TOKEN_CHAR(fsc, c);*/
			GET_CHAR (fsc);

			c = fsc->sio.lxc.c;
			switch (c)
			{
				case STIX_CHAR_EOF:
					fsc->errnum = STIX_FSC_ELITNT;
					return -1;
				
				case STIX_T('('):
					/* #( */
					ADD_TOKEN_CHAR(fsc, c);
					fsc->tok.type = STIX_FSC_TOK_APAREN;
					GET_CHAR (fsc);
					break;

				case STIX_T('\''):
					/* #' - quoted symbol literal */
					GET_CHAR (fsc);
					if (get_strlit(fsc) <= -1) return -1;
					fsc->tok.type = STIX_FSC_TOK_SYMLIT;
					break;

				case STIX_T('['):
					/* #[ - byte array literal */
					/* TODO */
					break;

				default:
					/* unquoted symbol literal */
					if (is_closing_char(c) || STIX_ISSPACE(c))
					{
						fsc->errnum = STIX_FSC_ELITNT;
						return -1;
					}

					do 
					{
						ADD_TOKEN_CHAR(fsc, c);
						GET_CHAR (fsc);
						c = fsc->sio.lxc.c;
					} 
					while (!is_closing_char(c) && !STIX_ISSPACE(c));

					fsc->tok.type = STIX_FSC_TOK_SYMLIT;
					break;
			}
				
			break;
#endif

				
		default:
			if (STIX_ISALPHA (c)) 
			{
				if (get_ident (fsc) <= -1) return -1;
			}
			else if (STIX_ISDIGIT (c)) 
			{
				if (get_numlit (fsc, 0) <= -1) return -1;
			}
			else if (is_binselchar (c)) 
			{
				/* binary selector */
				if (get_binsel (fsc) <= -1) return -1;
			}
			else 
			{
				stix_cstr_t ea;
				stix_char_t cc;

				cc = (stix_char_t)c;
				ea.ptr = &cc;
				ea.len = 1;

				stix_seterrnum (fsc, STIX_FSC_EILCHR, &ea);
				return -1;
			}
			break;
	}

wprintf (L"TOK: %S\n", fsc->tok.name.ptr);
	return 0;
}

static void clear_sio_names (stix_t* fsc)
{
	stix_link_t* cur;
	while (fsc->sio_names)
	{
		cur = fsc->sio_names;
		fsc->sio_names = cur->link;
		stix_freemem (fsc, cur);
	}
}

static int begin_include (stix_t* fsc)
{
	stix_ioarg_t* arg;
	stix_link_t* link;

	link = (stix_link_t*) stix_callocmem (fsc, STIX_SIZEOF(*link) + STIX_SIZEOF(stix_char_t) * (fsc->tok.name.len + 1));
	if (link == STIX_NULL) goto oops;

	stix_strcpy ((stix_char_t*)(link + 1), fsc->tok.name.ptr); 
	link->link = fsc->sio_names;
	fsc->sio_names = link;

	arg = (stix_ioarg_t*) stix_callocmem (fsc, STIX_SIZEOF(*arg));
	if (arg == STIX_NULL) goto oops;

	arg->name = (const stix_char_t*)(link + 1);
	arg->line = 1;
	arg->colm = 1;
	arg->prev = fsc->sio.inp;

	if (fsc->sio.impl (fsc, STIX_FSC_IO_OPEN, arg) <= -1) goto oops;

	fsc->sio.inp = arg;
	/* fsc->parse.depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_char (fsc) <= -1 || get_token (fsc) <= -1) 
	{
		end_include (fsc); 
		/* i don't jump to oops since i've called 
		 * end_include() where fsc->sio.inp/arg is freed. */
		return -1;
	}

	return 0;

oops:
	/* i don't need to free 'link' since it's linked to
	 * fsc->sio_names that's freed at the beginning of stix_read()
	 * or by stix_fini() */
	if (arg) stix_freemem (fsc, arg);
	return -1;
}

static int end_include (stix_t* fsc)
{
	int x;
	stix_ioarg_t* cur;

	if (fsc->sio.inp == &fsc->sio.arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = fsc->sio.impl (fsc, STIX_FSC_IO_CLOSE, fsc->sio.inp);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call STIX_FSC_SIO_CLOSE on 
	 * fsc->sio.inp again. */

	cur = fsc->sio.inp;
	fsc->sio.inp = fsc->sio.inp->prev;

	STIX_ASSERT (cur->name != STIX_NULL);
	stix_freemem (fsc, cur);
	/* fsc->parse.depth.incl--; */

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		return -1;
	}

	fsc->sio.lxc = fsc->sio.inp->lxc;
	return 1; /* ended the included file successfully */
}


/* ---------------------------------------------------------------------
 * Parser and Code Generator 
 * --------------------------------------------------------------------- */

static STIX_INLINE int is_tok_pseudovar (stix_t* fsc)
{
	return fsc->tok.type == STIX_FSC_TOK_IDENT &&
	       (stix_strequal(fsc->tok.name.ptr, STIX_T("self")) ||
	        stix_strequal(fsc->tok.name.ptr, STIX_T("super")) ||
	        stix_strequal(fsc->tok.name.ptr, STIX_T("thisContext")) ||
	        stix_strequal(fsc->tok.name.ptr, STIX_T("nil")) ||
	        stix_strequal(fsc->tok.name.ptr, STIX_T("true")) ||
	        stix_strequal(fsc->tok.name.ptr, STIX_T("false")));
}

static STIX_INLINE int is_tok_binsel (stix_t* fsc, const stix_char_t* sel)
{
	return fsc->tok.type == STIX_FSC_TOK_BINSEL && 
	       stix_strequal (fsc->tok.name.ptr, sel);
}

#if 0

#define EMIT_CODE_TEST(fsc,high,low) \
	do { if (emit_code_test(fsc,high,low) == -1) return -1; } while (0)

#define EMIT_PUSH_RECEIVER_VARIABLE(fsc,pos) \
	do {  \
		if (emit_stack_positional ( \
			fsc, PUSH_RECEIVER_VARIABLE, pos) == -1) return -1; \
	} while (0)

#define EMIT_PUSH_TEMPORARY_LOCATION(fsc,pos) \
	do {  \
		if (emit_stack_positional ( \
			fsc, PUSH_TEMPORARY_LOCATION, pos) == -1) return -1; \
	} while (0)

#define EMIT_PUSH_LITERAL_CONSTANT(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, PUSH_LITERAL_CONSTANT, pos) == -1) return -1; \
	} while (0)


#define EMIT_PUSH_LITERAL_VARIABLE(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, PUSH_LITERAL_VARIABLE, pos) == -1) return -1; \
	} while (0)

#define EMIT_STORE_RECEIVER_VARIABLE(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, STORE_RECEIVER_VARIABLE, pos) == -1) return -1; \
	} while (0)	

#define EMIT_STORE_TEMPORARY_LOCATION(fsc,pos) \
	do { \
		if (emit_stack_positional ( \
			fsc, STORE_TEMPORARY_LOCATION, pos) == -1) return -1; \
	} while (0)


#define EMIT_POP_STACK_TOP(fsc) EMIT_CODE(fsc, POP_STACK_TOP)
#define EMIT_DUPLICATE_STACK_TOP(fsc) EMIT_CODE(fsc, DUPLICATE_STACK_TOP)
#define EMIT_PUSH_ACTIVE_CONTEXT(fsc) EMIT_CODE(fsc, PUSH_ACTIVE_CONTEXT)
#define EMIT_RETURN_FROM_MESSAGE(fsc) EMIT_CODE(fsc, RETURN_FROM_MESSAGE)
#define EMIT_RETURN_FROM_BLOCK(fsc) EMIT_CODE(fsc, RETURN_FROM_BLOCK)

#define EMIT_SEND_TO_SELF(fsc,nargs,selector) \
	do { \
		if (emit_send_to_self(fsc,nargs,selector) == -1) return -1; \
	} while (0)

#define EMIT_SEND_TO_SUPER(fsc,nargs,selector) \
	do { \
		if (emit_send_to_super(fsc,nargs,selector) == -1) return -1; \
	} while (0)

#define EMIT_DO_PRIMITIVE(fsc,no) \
	do { if (emit_do_primitive(fsc,no) == -1) return -1; } while(0)

#endif

static STIX_INLINE int emit_code_test (
	stix_t* fsc, const stix_char_t* high, const stix_char_t* low)
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

static int emit_do_primitive (stix_t* fsc, int no)
{
	stix_uint8_t code[2];
	int len = 0;

	STIX_ASSERT (no >= 0x0 && no <= 0xFF);

	code[len++] = STIX_DO_PRIMITIVE;
	code[len++] = no;

	return emit_code (fsc, code, len);
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

static int __add_character_literal (stix_t* fsc, stix_char_t ch)
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
	stix_t* fsc, const stix_char_t* str, stix_word_t size)
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
	stix_t* fsc, const stix_char_t* str, stix_word_t size)
{
	stix_vm_t* stx = fsc->stx;
	return __add_literal (fsc, stix_new_symbolx(stx, str, size));
}

static int finish_method (stix_t* fsc)
{
	stix_vm_t* stx = fsc->stx;
	stix_class_t* class_obj;
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

static int parse_statements (stix_t* fsc)
{
	/*
	 * <statements> ::= (ORIGINAL->maybe wrong)
	 * 	(<return statement> ['.'] ) |
	 * 	(<expression> ['.' [<statements>]])
	 * <statements> ::= (REVISED->correct?)
	 * 	<statement> ['. [<statements>]]
	 */

	while (fsc->tok.type != STIX_FSC_TOK_EOF) 
	{
		if (parse_statement (fsc) == -1) return -1;

		if (fsc->tok.type == STIX_FSC_TOK_PERIOD) 
		{
			GET_TOKEN (fsc);
			continue;
		}

		if (fsc->tok.type != STIX_FSC_TOK_EOF) 
		{
			fsc->errnum = STIX_FSC_ERROR_NO_PERIOD;
			return -1;
		}
	}

	EMIT_CODE (fsc, RETURN_RECEIVER);
	return 0;
}

static int parse_block_statements (stix_t* fsc)
{
	while (fsc->tok.type != STIX_FSC_TOK_RBRACK && 
	       fsc->tok.type != STIX_FSC_TOK_EOF) {

		if (parse_statement(fsc) == -1) return -1;
		if (fsc->tok.type != STIX_FSC_TOK_PERIOD) break;
		GET_TOKEN (fsc);
	}

	return 0;
}

static int parse_statement (stix_t* fsc)
{
	/* 
	 * <statement> ::= <return statement> | <expression>
	 * <return statement> ::= returnOperator <expression> 
	 * returnOperator ::= '^'
	 */

	if (fsc->tok.type == STIX_FSC_TOK_RETURN) {
		GET_TOKEN (fsc);
		if (parse_expression(fsc) == -1) return -1;
		EMIT_RETURN_FROM_MESSAGE (fsc);
	}
	else {
		if (parse_expression(fsc) == -1) return -1;
	}

	return 0;
}

static int parse_expression (stix_t* fsc)
{
	/*
	 * <expression> ::= <assignment> | <basic expression>
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 * <assignment target> ::= identifier
	 * assignmentOperator ::=  ':='
	 */
	stix_vm_t* stx = fsc->stx;

	if (fsc->tok.type == STIX_FSC_TOK_IDENT) {
		stix_char_t* ident = stix_tok_yield (&fsc->tok, 0);
		if (ident == STIX_NULL) {
			fsc->errnum = STIX_FSC_ERROR_MEMORY;
			return -1;
		}

		GET_TOKEN (fsc);
		if (fsc->tok.type == STIX_FSC_TOK_ASSIGN) {
			GET_TOKEN (fsc);
			if (parse_assignment(fsc, ident) == -1) {
				stix_free (ident);
				return -1;
			}
		}
		else {
			if (parse_basic_expression(fsc, ident) == -1) {
				stix_free (ident);
				return -1;
			}
		}

		stix_free (ident);
	}
	else 
	{
		if (parse_basic_expression(fsc, STIX_NULL) == -1) return -1;
	}

	return 0;
}

static int parse_basic_expression (
	stix_t* fsc, const stix_char_t* ident)
{
	/*
	 * <basic expression> ::= <primary> [<messages> <cascaded messages>]
	 */
	int is_super;

	if (parse_primary(fsc, ident, &is_super) == -1) return -1;
	if (fsc->tok.type != STIX_FSC_TOK_EOF &&
	    fsc->tok.type != STIX_FSC_TOK_PERIOD) 
	{
		if (parse_message_continuation(fsc, is_super) == -1) return -1;
	}
	return 0;
}

static int parse_assignment (
	stix_t* fsc, const stix_char_t* target)
{
	/*
	 * <assignment> ::= <assignment target> assignmentOperator <expression>
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

static int parse_primary (
	stix_t* fsc, const stix_char_t* ident, int* is_super)
{
	/*
	 * <primary> ::=
	 * 	identifier | <literal> | 
	 * 	<block constructor> | ( '('<expression>')' )
	 */

	stix_vm_t* stx = fsc->stx;

	if (ident == STIX_NULL) 
	{
		int pos;
		stix_word_t literal;

		*is_super = stix_false;

		if (fsc->tok.type == STIX_FSC_TOK_IDENT) 
		{
			if (parse_primary_ident(fsc, 
				fsc->tok.name.buffer, is_super) == -1) return -1;
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_FSC_TOK_CHRLIT) {
			pos = __add_character_literal(
				fsc, fsc->tok.name.buffer[0]);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_FSC_TOK_STRLIT) {
			pos = __add_string_literal (fsc,
				fsc->tok.name.buffer, fsc->tok.name.size);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_FSC_TOK_NUMLIT) 
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
		else if (fsc->tok.type == STIX_FSC_TOK_SYMLIT) {
			pos = __add_symbol_literal (fsc,
				fsc->tok.name.buffer, fsc->tok.name.size);
			if (pos == -1) return -1;
			EMIT_PUSH_LITERAL_CONSTANT (fsc, pos);
			GET_TOKEN (fsc);
		}
		else if (fsc->tok.type == STIX_FSC_TOK_LBRACK) {
			GET_TOKEN (fsc);
			if (parse_block_constructor(fsc) == -1) return -1;
		}
		else if (fsc->tok.type == STIX_FSC_TOK_APAREN) {
			/* TODO: array literal */
		}
		else if (fsc->tok.type == STIX_FSC_TOK_LPAREN) {
			GET_TOKEN (fsc);
			if (parse_expression(fsc) == -1) return -1;
			if (fsc->tok.type != STIX_FSC_TOK_RPAREN) {
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

static int parse_primary_ident (
	stix_t* fsc, const stix_char_t* ident, int* is_super)
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
	 * <block constructor> ::= '[' <block body> ']'
	 * <block body> ::= [<block argument>* '|']
	 * 	[<temporaries>] [<statements>]
	 * <block argument> ::= ':'  identifier
	 */

	if (fsc->tok.type == STIX_FSC_TOK_COLON) 
	{
		do 
		{
			GET_TOKEN (fsc);

			if (fsc->tok.type != STIX_FSC_TOK_IDENT) 
			{
				fsc->errnum = STIX_FSC_ERROR_BLOCK_ARGUMENT_NAME;
				return -1;
			}

			/* TODO : store block arguments */
			GET_TOKEN (fsc);
		} 
		while (fsc->tok.type == STIX_FSC_TOK_COLON);
			
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

	if (fsc->tok.type != STIX_FSC_TOK_RBRACK) 
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
	 * <messages> ::=
	 * 	(<unary message>+ <binary message>* [<keyword message>] ) |
	 * 	(<binary message>+ [<keyword message>] ) |
	 * 	<keyword message>
	 * <cascaded messages> ::= (';' <messages>)*
	 */
	if (parse_keyword_message(fsc, is_super) == -1) return -1;

	while (fsc->tok.type == STIX_FSC_TOK_SEMICOLON) 
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
	 * <keyword message> ::= (keyword <keyword argument> )+
	 * <keyword argument> ::= <primary> <unary message>* <binary message>*
	 */

	stix_name_t name;
	stix_word_t pos;
	int is_super2;
	int nargs = 0, n;

	if (parse_binary_message (fsc, is_super) == -1) return -1;
	if (fsc->tok.type != STIX_FSC_TOK_KEYWORD) return 0;

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
	while (fsc->tok.type == STIX_FSC_TOK_KEYWORD);

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
	 * <binary message> ::= binarySelector <binary argument>
	 * <binary argument> ::= <primary> <unary message>*
	 */
	stix_word_t pos;
	int is_super2;
	int n;

	if (parse_unary_message (fsc, is_super) == -1) return -1;

	while (fsc->tok.type == STIX_FSC_TOK_BINSEL) 
	{
		stix_char_t* op = stix_tok_yield (&fsc->tok, 0);
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
	/* <unary message> ::= unarySelector */

	stix_word_t pos;
	int n;

	while (fsc->tok.type == STIX_FSC_TOK_IDENT) 
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

static int parse_method (stix_t* fsc, stix_word_t method_class, void* input)
{
	/*
	 * <method definition> ::= 
	 * 	<message pattern> [<temporaries>] [<primitive>] [<statements>]
	 */

	GET_CHAR (fsc);
	GET_TOKEN (fsc);

	stix_name_clear (&fsc->method_name);
	stix_arr_clear (&fsc->bcd);

	while (fsc->met.tmpr.count > 0) 
	{
		stix_free (fsc->met.tmpr.names[--fsc->met.tmpr.count]);
	}
	fsc->met.tmpr.nargs = 0;
	fsc->literal_count = 0;

	if (parse_method_name_pattern(fsc) <= -1 ||
	    parse_method_temporaries(fsc) <= -1 ||
	    parse_method_primitive(fsc) <= -1 ||
	    parse_statements(fsc) <= -1 ||
	    finish_method (fsc) <= -1) return -1;

	return 0;
}

#endif

static int get_class_type (const stix_char_t* str, class_type_t* type)
{
	static struct 
	{
		stix_char_t* word;
		class_type_t type;
	} tab[] = 
	{
		{ STIX_T("class"),             CLASS_NORMAL },
		{ STIX_T("variableClass"),     CLASS_VARIABLE },
		{ STIX_T("variableByteClass"), CLASS_VARIABLE_BYTE },
		{ STIX_T("variableCharClass"), CLASS_VARIABLE_CHAR }
	};

	int i;

	for (i = 0; i < STIX_COUNTOF(tab); i++)
	{
		if (stix_strequal(str, tab[i].word)) 
		{
			*type = tab[i].type;
			return 0;
		}
	}

	return -1;
}

static int get_vardef_type (const stix_char_t* str, vardef_type_t* type)
{
	static struct 
	{
		stix_char_t* word;
		class_type_t type;
	} tab[] = 
	{
		{ STIX_T("|-"), VARDEF_INSTANCE },
		{ STIX_T("|+"), VARDEF_CLASS_INSTANCE },
		{ STIX_T("|*"), VARDEF_CLASS }
		/* TODO: shared pools */
	};

	int i;

	for (i = 0; i < STIX_COUNTOF(tab); i++)
	{
		if (stix_strequal(str, tab[i].word)) 
		{
			*type = tab[i].type;
			return 0;
		}
	}

	return -1;
}

static int compile_vardef (stix_t* fsc, vardef_type_t vardef_type)
{
	return -1;
}

static int parse_unary_pattern (stix_t* fsc)
{
	STIX_ASSERT (fsc->met.name.len == 0);
	STIX_ASSERT (fsc->met.tmpr.nargs == 0);

/* TODO: check if the method name exists */

	if (fsc->tok.name.len >= STIX_COUNTOF(fsc->met.name.buf))
	{
		stix_seterror (fsc, STIX_FSC_EMETNTL, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	/* collect the method name */
	fsc->met.name.len = stix_strcpy (fsc->met.name.buf, fsc->tok.name.ptr);
	GET_TOKEN (fsc);
	return 0;
}

static int parse_binary_pattern (stix_t* fsc)
{
	STIX_ASSERT (fsc->met.name.len == 0);
	STIX_ASSERT (fsc->met.tmpr.nargs == 0);

/* TODO: check if the method name exists */
	if (fsc->tok.name.len >= STIX_COUNTOF(fsc->met.name.buf))
	{
		stix_seterror (fsc, STIX_FSC_EMETNTL, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	/* collect the method name */
	fsc->met.name.len = stix_strcpy (fsc->met.name.buf, fsc->tok.name.ptr);
	GET_TOKEN (fsc);

	/* collect the argument name */
	if (fsc->tok.type != STIX_FSC_TOK_IDENT) 
	{
		stix_seterror (fsc, STIX_FSC_EILARGN, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	STIX_ASSERT (fsc->met.tmpr.nargs == 0);
	/*
	 * check if there are too many arguments defined.
	 * however, in this function, this condition will never be met.
	 * so let me just comment out the check.
	 *
	if (fsc->met.tmpr.nargs >= STIX_COUNTOF(fsc->met.tmpr.names)) 
	{
		stix_seterror (fsc, STIX_FSC_ETMARGS, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}
	*/

/* TODO: check for duplicate entries...in instvars */

	if (fsc->tok.name.len >= STIX_COUNTOF(fsc->met.tmpr.names[fsc->met.tmpr.count]))
	{
		/* argument name is too long */
		stix_seterror (fsc, STIX_FSC_EARGNTL, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}
	stix_strcpy (fsc->met.tmpr.names[fsc->met.tmpr.nargs], fsc->tok.name.ptr);
	fsc->met.tmpr.nargs++;

	GET_TOKEN (fsc);
	return 0;
}

static int parse_keyword_pattern (stix_t* fsc)
{
	STIX_ASSERT (fsc->met.name.len == 0);
	STIX_ASSERT (fsc->met.tmpr.nargs == 0);

	do 
	{
		if (fsc->tok.name.len + fsc->met.name.len >= STIX_COUNTOF(fsc->met.name.buf))
		{
			stix_seterror (fsc, STIX_FSC_EMETNTL, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}
		fsc->met.name.len += stix_strcpy (&fsc->met.name.buf[fsc->met.name.len], fsc->tok.name.ptr);

		GET_TOKEN (fsc);
		if (fsc->tok.type != STIX_FSC_TOK_IDENT || is_tok_pseudovar(fsc)) 
		{
			stix_seterror (fsc, STIX_FSC_EILARGN, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}

		if (fsc->met.tmpr.nargs >= STIX_COUNTOF(fsc->met.tmpr.names)) 
		{
			/* too many arguments */
			stix_seterror (fsc, STIX_FSC_ETMARGS, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments */
		if (fsc->tok.name.len >= STIX_COUNTOF(fsc->met.tmpr.names[fsc->met.tmpr.nargs]))
		{
			/* argument name is too long */
			stix_seterror (fsc, STIX_FSC_EARGNTL, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}
		stix_strcpy (fsc->met.tmpr.names[fsc->met.tmpr.nargs], fsc->tok.name.ptr);
		fsc->met.tmpr.nargs++;

		GET_TOKEN (fsc);
	} 
	while (fsc->tok.type == STIX_FSC_TOK_KEYWORD);

	/* TODO: check if the method name exists */
	/* if it exists, collapse arguments */

	return 0;
}

static int parse_method_name_pattern (stix_t* fsc)
{
	/* 
	 * <message pattern> ::= <unary pattern> | <binary pattern> | <keyword pattern>
	 * <unary pattern> ::= unarySelector
	 * <binary pattern> ::= binarySelector <method argument>
	 * <keyword pattern> ::= (keyword  <method argument>)+
	 */
	int n;

	STIX_ASSERT (fsc->met.tmpr.count == 0);

	switch (fsc->tok.type)
	{
		case STIX_FSC_TOK_IDENT:
			n = parse_unary_pattern (fsc);
			break;

		case STIX_FSC_TOK_BINSEL:
			n = parse_binary_pattern (fsc);
			break;

		case STIX_FSC_TOK_KEYWORD:
			n = parse_keyword_pattern (fsc);
			break;

		default:
			stix_seterror (fsc, STIX_FSC_EILMETN, &fsc->tok.name, &fsc->tok.loc);
			n = -1;
	}

	/* the total number of temporaries is equal to the number of arguments
	 * after having processed the message pattern */
	fsc->met.tmpr.count = fsc->met.tmpr.nargs;
	return n;
}

static int parse_method_temporaries (stix_t* fsc)
{
	/* 
	 * <temporaries> ::= '|' <temporary variable list> '|'
	 * <temporary variable list> ::= identifier*
	 */

	if (!is_tok_binsel (fsc, STIX_T("|"))) return 0;

	GET_TOKEN (fsc);
	while (fsc->tok.type == STIX_FSC_TOK_IDENT) 
	{
		if (fsc->met.tmpr.count >= STIX_COUNTOF(fsc->met.tmpr.names)) 
		{
			stix_seterror (fsc, STIX_FSC_ETMTMPS, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}

		if (is_tok_pseudovar(fsc)) 
		{
			stix_seterror (fsc, STIX_FSC_EILTMPN, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}

/* TODO: check for duplicate entries...in instvars/arguments/temporaries */

		if (fsc->tok.name.len >= STIX_COUNTOF(fsc->met.tmpr.names[fsc->met.tmpr.count]))
		{
			/* temporary variable name is too long */
			stix_seterror (fsc, STIX_FSC_ETMPNTL, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}
		stix_strcpy (fsc->met.tmpr.names[fsc->met.tmpr.count], fsc->tok.name.ptr);
		fsc->met.tmpr.count++;

		GET_TOKEN (fsc);
	}

	if (!is_tok_binsel (fsc, STIX_T("|"))) return 0;
	{
		stix_seterror (fsc, STIX_FSC_EVRTBAR, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	GET_TOKEN (fsc);
	return 0;
}

static int parse_method_primitive (stix_t* fsc)
{
	/* 
	 * <primitive> ::= '<' 'primitive:' number '>'
	 */

	int prim_no;

	if (!is_tok_binsel (fsc, STIX_T("<"))) return 0;

	GET_TOKEN (fsc);
	if (fsc->tok.type != STIX_FSC_TOK_KEYWORD ||
	    !stix_strequal (fsc->tok.name.ptr, STIX_T("primitive:"))) 
	{
		fsc->errnum = STIX_FSC_ERROR_PRIMITIVE_KEYWORD;
		return -1;
	}

	GET_TOKEN (fsc); /* TODO: only integer */
	if (fsc->tok.type != STIX_FSC_TOK_NUMLIT) 
	{
		fsc->errnum = STIX_FSC_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

/*TODO: more checks the validity of the primitive number */
	if (!stix_stristype(fsc->tok.name.buffer, stix_isdigit)) 
	{
		fsc->errnum = STIX_FSC_ERROR_PRIMITIVE_NUMBER;
		return -1;
	}

	STIX_STRTOI (prim_no, fsc->tok.name.buffer, STIX_NULL, 10);
	if (prim_no < 0 || prim_no > 0xFF) 
	{
		fsc->errnum = STIX_FSC_ERROR_PRIMITIVE_NUMBER_RANGE;
		return -1;
	}

	EMIT_DO_PRIMITIVE (fsc, prim_no);

	GET_TOKEN (fsc);
	if (!is_tok_binsel (fsc, STIX_T(">"))) return 0;
	{
		stix_seterror (fsc, STIX_FSC_ERABRCK, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	GET_TOKEN (fsc);
	return 0;
}


static int compile_method (stix_t* fsc, int instance)
{
	/* clear data required to compile a method */
	STIX_MEMSET (&fsc->met, 0, STIX_SIZEOF(fsc->met));

#if 0
	/* clear the byte-code buffer */
	fsc->bcd.len = 0;
#endif

	if (parse_method_name_pattern (fsc) <= -1) return -1;

	if (fsc->tok.type != STIX_FSC_TOK_LBRACE)
	{
		/* { expected */
		stix_seterror (fsc, STIX_FSC_ELBRACE, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}
	GET_TOKEN (fsc);

	if (parse_method_temporaries (fsc) <= -1 ||
	    parse_method_primitive (fsc) <= -1 /*||
	    parse_statements (fsc) <= -1 ||
	    finish_method (fsc) <= -1*/) return -1; 

	if (fsc->tok.type != STIX_FSC_TOK_RBRACE)
	{
		/* } expected */
		stix_seterror (fsc, STIX_FSC_ERBRACE, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}
	GET_TOKEN (fsc);

wprintf (L"METHOD NAME ==> [%S] temporaries => %d\n", fsc->met.name.buf, fsc->met.tmpr.count);
	return 0;
}

static int compile_classdef (stix_t* fsc, class_type_t class_type)
{
	stix_oop_t oop1, oop2;
	int extend;

	if (fsc->tok.type != STIX_FSC_TOK_IDENT)
	{
		/* class name expected. */
		stix_seterror (fsc, STIX_FSC_ECLSNAM, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	if (stix_vm_findclass (fsc->vm, fsc->tok.name.ptr, &oop1) <= -1)
	{
		/* this class is a new class. you can only extend an existing class */
		GET_TOKEN (fsc);
		if (fsc->tok.type == STIX_FSC_TOK_IDENT && 
		    stix_strequal (fsc->tok.name.ptr, STIX_T("extend")))
		{
			GET_TOKEN (fsc);
			if (fsc->tok.type != STIX_FSC_TOK_IDENT)
			{
				stix_seterror (fsc, STIX_FSC_ECLSNAM, &fsc->tok.name, &fsc->tok.loc);
				return -1;
			}

			if (stix_vm_findclass (fsc->vm, fsc->tok.name.ptr, &oop2) <= -1)
			{
				stix_seterror (fsc, STIX_FSC_EILBCLS, &fsc->tok.name, &fsc->tok.loc);
				return -1;
			}
		}
		else
		{
			stix_seterror (fsc, STIX_FSC_EEXTEND, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}

		extend = 1;
	}
	else
	{
/* TODO: check the existing class layout againt class_type (variableByteClass, variableCharClass, class, etc).
 * if no match, return -1 */
		extend = 0;
	}

	GET_TOKEN (fsc);
	if (fsc->tok.type != STIX_FSC_TOK_LBRACE)
	{
		/* { expected */
		stix_seterror (fsc, STIX_FSC_ELBRACE, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	GET_TOKEN (fsc);

	if (!extend)
	{
		while (1)
		{
			vardef_type_t vardef_type;

			/* TODO: compile other components including various pragma statements 
			 * <category: ...>
			 * <comment: ...>
			 */
			if (fsc->tok.type == STIX_FSC_TOK_BINSEL &&
			    get_vardef_type (fsc->tok.name.ptr, &vardef_type) >= 0)
			{
				if (compile_vardef (fsc, vardef_type) <= -1) return -1;
			}
			else
			{
				break;
			}
		}
	}

	while (1)
	{
		/* TODO: compile other components including various pragma statements 
		 * <category: ...>
		 * <comment: ...>
		 */
		if (is_tok_binsel (fsc, STIX_T("-"))) 
		{
			GET_TOKEN (fsc);
			if (compile_method (fsc, 1) <= -1) return -1;
		}
		else if (is_tok_binsel (fsc, STIX_T("+")))
		{
			GET_TOKEN (fsc);
			if (compile_method (fsc, 0) <= -1) return -1;
		}
		else
		{
			break;
		}	
	}

	if (fsc->tok.type != STIX_FSC_TOK_RBRACE)
	{
		/* TODO: } expected */
		stix_seterror (fsc, STIX_FSC_ERBRACE, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	GET_TOKEN (fsc);
	return 0;
}


static int compile_directive (stix_t* fsc)
{
	if (fsc->tok.type == STIX_FSC_TOK_IDENT)
	{
		class_type_t class_type;

		if (get_class_type (fsc->tok.name.ptr, &class_type) >= 0)
		{
			if (get_token (fsc) <= -1) return -1;
			return compile_classdef (fsc, class_type);
		}
		else if (stix_strequal (fsc->tok.name.ptr, STIX_T("include")))
		{
			if (get_token (fsc) <= -1) return -1;

			if (fsc->tok.type != STIX_FSC_TOK_STRLIT)
			{
				stix_seterror (fsc, STIX_FSC_ESTRLIT, &fsc->tok.name, &fsc->tok.loc);
				return -1;
			}

			if (begin_include (fsc) <= -1) return -1;
		}
		else
		{
			stix_seterror (fsc, STIX_FSC_EILDIR, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}
	}
	else
	{
		stix_seterror (fsc, STIX_FSC_EILDIR, &fsc->tok.name, &fsc->tok.loc);
		return -1;
	}

	return 0;
}

static int compile_stream (stix_t* fsc)
{
	GET_CHAR (fsc);
	GET_TOKEN (fsc);

	while (fsc->tok.type != STIX_FSC_TOK_EOF)
	{
		if (is_tok_binsel (fsc, STIX_T("@")))
		{
			GET_TOKEN (fsc);
			if (compile_directive (fsc) <= -1) return -1;
		}
		/* TODO: normal smalltalk message sending expressions */
		else 
		{
			stix_seterror (fsc, STIX_FSC_EILTTOK, &fsc->tok.name, &fsc->tok.loc);
			return -1;
		}
	}

	return 0;
}
#endif

int stix_compile (stix_t* stix, stix_ioimpl_t io)
{
	int n;

	if (!io)
	{
		stix->errnum = STIX_EINVAL;
		return -1;
	}

	STIX_ASSERT (stix->c == STIX_NULL);

	stix->c = stix_callocmem (stix, STIX_SIZEOF(*stix->c));
	if (!stix->c) return -1;
	stix->c->impl = io;
	stix->c->arg.line = 1;
	stix->c->arg.colm = 1;
	stix->c->curinp = &stix->c->arg;
	clear_sio_names (stix);

	/* open the top-level stream */
	n = stix->c->impl (stix, STIX_IO_OPEN, stix->c->curinp);
	if (n <= -1) return -1;

	if (compile_stream (stix) <= -1) goto oops;

	/* close the stream */
	STIX_ASSERT (stix->c->curinp == &stix->c->arg);
	stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);
	
	stix_freemem (stix, stix->c);
	stix->c = STIX_NULL;
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

	stix_freemem (stix, stix->c);
	stix->c = STIX_NULL;
	return -1;
}



