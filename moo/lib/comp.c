/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
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

#define CLASS_BUFFER_ALIGN       64
#define LITERAL_BUFFER_ALIGN     64
#define CODE_BUFFER_ALIGN        512
#define BALIT_BUFFER_ALIGN       64
#define ARLIT_BUFFER_ALIGN       64
#define BLK_TMPRCNT_BUFFER_ALIGN 32
#define POOLDIC_OOP_BUFFER_ALIGN 32

/* initial method dictionary size */
#define INSTANCE_METHOD_DICTIONARY_SIZE 256 /* TODO: choose the right size */
#define CLASS_METHOD_DICTIONARY_SIZE 128 /* TODO: choose the right size */
#define NAMESPACE_SIZE 128 /* TODO: choose the right size */
#define POOL_DICTIONARY_SIZE_ALIGN 128

#define INVALID_IP MOO_TYPE_MAX(moo_oow_t)

enum class_mod_t
{
	CLASS_INDEXED   = (1 << 0)
};

enum var_type_t
{
	/* NEVER Change the order and the value of 3 items below.
	 * moo->c->cls.vars and moo->c->cls.var_count relies on them. */
	VAR_INSTANCE   = 0,
	VAR_CLASS      = 1,
	VAR_CLASSINST  = 2,
	/* NEVER Change the order and the value of 3 items above. */

	VAR_GLOBAL,
	VAR_ARGUMENT,
	VAR_TEMPORARY
};
typedef enum var_type_t var_type_t;

struct var_info_t
{
	var_type_t            type;
	moo_ooi_t             pos; /* not used for VAR_GLOBAL */
	moo_oop_class_t       cls; /* useful if type is VAR_CLASS. note MOO_NULL indicates the self class. */
	moo_oop_association_t gbl; /* used for VAR_GLOBAL only */
};
typedef struct var_info_t var_info_t;

static struct voca_t
{
	moo_oow_t len;
	moo_ooch_t str[11];
} vocas[] = {
	{  5, { 'b','r','e','a','k'                                           } },
	{  5, { '#','b','y','t','e'                                           } },
	{ 10, { '#','c','h','a','r','a','c','t','e','r'                       } },
	{  5, { 'c','l','a','s','s'                                           } },
	{  6, { '#','c','l','a','s','s'                                       } },
	{ 10, { '#','c','l','a','s','s','i','n','s','t'                       } },
	{  8, { 'c','o','n','t','i','n','u','e'                               } },
	{  3, { 'd','c','l'                                                   } },
	{  7, { 'd','e','c','l','a','r','e'                                   } },
	{  2, { 'd','o'                                                       } },
	{  4, { 'e','l','s','e'                                               } },
	{  5, { 'e','l','s','i','f'                                           } },
	{  6, { 'e','n','s','u','r','e',                                      } },
	{  5, { 'e','r','r','o','r'                                           } },
	{  9, { 'e','x','c','e','p','t','i','o','n'                           } },
	{  6, { 'e','x','t','e','n','d'                                       } },
	{  5, { 'f','a','l','s','e'                                           } },
	{  4, { 'f','r','o','m'                                               } },
	{  9, { '#','h','a','l','f','w','o','r','d'                           } },
	{  2, { 'i','f'                                                       } },
	{  8, { '#','i','n','c','l','u','d','e'                               } },
	{  7, { '#','l','i','w','o','r','d'                                   } },
	{  6, { 'm','e','t','h','o','d'                                       } },
	{  3, { 'n','i','l'                                                   } },
	{  8, { '#','p','o','i','n','t','e','r'                               } },
	{  7, { 'p','o','o','l','d','i','c'                                   } },
	{  8, { '#','p','o','o','l','d','i','c'                               } },
	{ 10, { 'p','r','i','m','i','t','i','v','e',':'                       } },
	{  4, { 's','e','l','f'                                               } },
	{  5, { 's','u','p','e','r'                                           } },
	{ 11, { 't','h','i','s','C','o','n','t','e','x','t'                   } },
	{ 11, { 't','h','i','s','P','r','o','c','e','s','s'                   } },
	{  4, { 't','r','u','e'                                               } },
	{  5, { 'w','h','i','l','e'                                           } },
	{  5, { '#','w','o','r','d'                                           } },

	{  1, { '|'                                                           } },
	{  1, { '>'                                                           } },
	{  1, { '<'                                                           } },

	{  5, { '<','E','O','F','>'                                           } }
};

enum voca_id_t
{
	VOCA_BREAK,
	VOCA_BYTE_S,
	VOCA_CHARACTER_S,
	VOCA_CLASS,
	VOCA_CLASS_S,
	VOCA_CLASSINST_S,
	VOCA_CONTINUE,
	VOCA_DCL,
	VOCA_DECLARE,
	VOCA_DO,
	VOCA_ELSE,
	VOCA_ELSIF,
	VOCA_ENSURE,
	VOCA_ERROR,
	VOCA_EXCEPTION,
	VOCA_EXTEND,
	VOCA_FALSE,
	VOCA_FROM,
	VOCA_HALFWORD_S,
	VOCA_IF,
	VOCA_INCLUDE_S,
	VOCA_LIWORD_S,
	VOCA_METHOD,
	VOCA_NIL,
	VOCA_POINTER_S,
	VOCA_POOLDIC,
	VOCA_POOLDIC_S,
	VOCA_PRIMITIVE_COLON,
	VOCA_SELF,
	VOCA_SUPER,
	VOCA_THIS_CONTEXT,
	VOCA_THIS_PROCESS,
	VOCA_TRUE,
	VOCA_WHILE,
	VOCA_WORD_S,

	VOCA_VBAR,
	VOCA_GT,
	VOCA_LT,

	VOCA_EOF
};
typedef enum voca_id_t voca_id_t;

static int compile_block_statement (moo_t* moo);
static int compile_method_statement (moo_t* moo);
static int compile_method_expression (moo_t* moo, int pop);
static int add_literal (moo_t* moo, moo_oop_t lit, moo_oow_t* index);

static MOO_INLINE int is_spacechar (moo_ooci_t c)
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


static MOO_INLINE int is_alphachar (moo_ooci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static MOO_INLINE int is_digitchar (moo_ooci_t c)
{
/* TODO: support full unicode */
	return (c >= '0' && c <= '9');
}

static MOO_INLINE int is_alnumchar (moo_ooci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static MOO_INLINE int is_binselchar (moo_ooci_t c)
{
	/*
	 * binary-selector-character :=
	 * 	'%' | '&' | '*' | '+' | '-' |
	 *   '/' | '<' | '>' | '=' | '?' |
	 *   '@' | '\' | '~' | '|'
	 */

	switch (c)
	{
		case '%':
		case '&':
		case '*':
		case '+':
		case '-':
		case '/': 
		case '<':
		case '>':
		case '=':
		case '?':
		case '@':
		case '\\':
		case '|':
		case '~':
			return 1;

		default:
			return 0;
	}
}

static MOO_INLINE int is_leadidentchar (moo_ooci_t c)
{
	return is_alphachar(c) || c == '_';
}

static MOO_INLINE int is_identchar (moo_ooci_t c)
{
	return is_alnumchar(c) || c == '_';
}

#if 0
static MOO_INLINE int is_closing_char (moo_ooci_t c)
{
	switch (c)
	{
		case '.':
		case '}':
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
#endif

static MOO_INLINE int is_word (const moo_oocs_t* oocs, voca_id_t id)
{
	return oocs->len == vocas[id].len && 
	       moo_equaloochars(oocs->ptr, vocas[id].str, vocas[id].len);
}

static int is_reserved_word (const moo_oocs_t* ucs)
{
	static int rw[] = 
	{
		VOCA_SELF,
		VOCA_SUPER,
		VOCA_NIL,
		VOCA_TRUE,
		VOCA_FALSE,
		VOCA_ERROR,
		VOCA_THIS_CONTEXT,
		VOCA_THIS_PROCESS,
		VOCA_IF
	};
	int i;

	for (i = 0; i < MOO_COUNTOF(rw); i++)
	{
		if (is_word(ucs, rw[i])) return 1;
	}

	return 0;
}

static int is_restricted_word (const moo_oocs_t* ucs)
{
	/* not fully reserved. but restricted in a certain context */

	static int rw[] = 
	{
		VOCA_CLASS,
		VOCA_DCL,
		VOCA_DECLARE,
		VOCA_EXTEND,
		VOCA_METHOD,
		VOCA_POOLDIC,
		VOCA_FROM
	};
	int i;

	for (i = 0; i < MOO_COUNTOF(rw); i++)
	{
		if (is_word(ucs, rw[i])) return 1;
	}

	return 0;
}

static int begin_include (moo_t* moo);
static int end_include (moo_t* moo);

static void set_syntax_error (moo_t* moo, moo_synerrnum_t num, const moo_ioloc_t* loc, const moo_oocs_t* tgt)
{
	moo->errnum = MOO_ESYNTAX;
	moo->c->synerr.num = num;

	/* The SCO compiler complains of this ternary operation saying:
	 *    error: operands have incompatible types: op ":" 
	 * it seems to complain of type mismatch between *loc and
	 * moo->c->tok.loc due to 'const' prefixed to loc. */
	/*moo->c->synerr.loc = loc? *loc: moo->c->tok.loc;*/
	if (loc)
		moo->c->synerr.loc = *loc;
	else
		moo->c->synerr.loc = moo->c->tok.loc;
	
	if (tgt) moo->c->synerr.tgt = *tgt;
	else 
	{
		moo->c->synerr.tgt.ptr = MOO_NULL;
		moo->c->synerr.tgt.len = 0;
	}
}

static int copy_string_to (moo_t* moo, const moo_oocs_t* src, moo_oocs_t* dst, moo_oow_t* dst_capa, int append, moo_ooch_t add_delim)
{
	moo_oow_t len, pos;

	if (append)
	{
		pos = dst->len;
		len = dst->len + src->len;
		if (add_delim != '\0') len++;
	}
	else
	{
		pos = 0;
		len = src->len;
	}

	if (len > *dst_capa)
	{
		moo_ooch_t* tmp;
		moo_oow_t capa;

		capa = MOO_ALIGN(len, CLASS_BUFFER_ALIGN);

		tmp = moo_reallocmem (moo, dst->ptr, MOO_SIZEOF(*tmp) * capa);
		if (!tmp)  return -1;

		dst->ptr = tmp;
		*dst_capa = capa;
	}

	if (append && add_delim) dst->ptr[pos++] = add_delim;
	moo_copyoochars (&dst->ptr[pos], src->ptr, src->len);
	dst->len = len;
	return 0;
}

static int find_word_in_string (const moo_oocs_t* haystack, const moo_oocs_t* name, moo_oow_t* xindex)
{
	/* this function is inefficient. but considering the typical number
	 * of arguments and temporary variables, the inefficiency can be 
	 * ignored in my opinion. the overhead to maintain the reverse lookup
	 * table from a name to an index should be greater than this simple
	 * inefficient lookup */

	moo_ooch_t* t, * e;
	moo_oow_t index, i;

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
		if (t >= e || is_spacechar(*t)) 
		{
			if (xindex) *xindex = index;
			return 0;
		}

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

#define CHAR_TO_NUM(c,base) \
	((c >= '0' && c <= '9')? ((c - '0' < base)? (c - '0'): base): \
	 (c >= 'A' && c <= 'Z')? ((c - 'A' + 10 < base)? (c - 'A' + 10): base): \
	 (c >= 'a' && c <= 'z')? ((c - 'a' + 10 < base)? (c - 'a' + 10): base): base)

static int string_to_smooi (moo_t* moo, moo_oocs_t* str, int radixed, moo_ooi_t* num)
{
	/* it is not a generic conversion function.
	 * it assumes a certain pre-sanity check on the string
	 * done by the lexical analyzer */

	int v, negsign, base;
	const moo_ooch_t* ptr, * end;
	moo_oow_t value, old_value;

	negsign = 0;
	ptr = str->ptr,
	end = str->ptr + str->len;

	MOO_ASSERT (moo, ptr < end);

	if (*ptr == '+' || *ptr == '-')
	{
		negsign = *ptr - '+';
		ptr++;
	}

	if (radixed)
	{
		MOO_ASSERT (moo, ptr < end);

		base = 0;
		do
		{
			base = base * 10 + CHAR_TO_NUM(*ptr, 10);
			ptr++;
		}
		while (*ptr != 'r');

		ptr++;
	}
	else base = 10;

	MOO_ASSERT (moo, ptr < end);

	value = old_value = 0;
	while (ptr < end && (v = CHAR_TO_NUM(*ptr, base)) < base)
	{
		value = value * base + v;
		if (value < old_value) 
		{
			/* overflow must have occurred */
			moo->errnum = MOO_ERANGE;
			return -1;
		}
		old_value = value;
		ptr++;
	}

	if (ptr < end)
	{
		/* trailing garbage? */
		moo->errnum = MOO_EINVAL;
		return -1;
	}

	MOO_ASSERT (moo, -MOO_SMOOI_MAX == MOO_SMOOI_MIN);
	if (value > MOO_SMOOI_MAX) 
	{
		moo->errnum = MOO_ERANGE;
		return -1;
	}

	*num = value;
	if (negsign) *num *= -1;

	return 0;
}

static moo_oop_t string_to_num (moo_t* moo, moo_oocs_t* str, int radixed)
{
	int negsign, base;
	const moo_ooch_t* ptr, * end;

	negsign = 0;
	ptr = str->ptr,
	end = str->ptr + str->len;

	MOO_ASSERT (moo, ptr < end);

	if (*ptr == '+' || *ptr == '-')
	{
		negsign = *ptr - '+';
		ptr++;
	}

	if (radixed)
	{
		MOO_ASSERT (moo, ptr < end);

		base = 0;
		do
		{
			base = base * 10 + CHAR_TO_NUM(*ptr, 10);
			ptr++;
		}
		while (*ptr != 'r');

		ptr++;
	}
	else base = 10;

/* TODO: handle floating point numbers ... etc */
	if (negsign) base = -base;
	return moo_strtoint (moo, ptr, end - ptr, base);
}

static moo_oop_t string_to_error (moo_t* moo, moo_oocs_t* str)
{
	moo_ooi_t num = 0;
	const moo_ooch_t* ptr, * end;

	ptr = str->ptr,
	end = str->ptr + str->len;

	/* i assume that the input is in the form of error(NNN)
	 * all other letters are non-digits except the NNN part.
	 * i just skip all non-digit letters for simplicity sake. */
	while (ptr < end)
	{
		if (is_digitchar(*ptr)) num = num * 10 + (*ptr - '0');
		ptr++;
	}

	return MOO_ERROR_TO_OOP(num);
}


/* ---------------------------------------------------------------------
 * SOME PRIVIATE UTILITILES
 * --------------------------------------------------------------------- */

static void init_oow_pool (moo_t* moo, moo_oow_pool_t* pool)
{
	pool->count = 0;
	pool->static_chunk.next = MOO_NULL;
	pool->head = &pool->static_chunk;
	pool->tail = &pool->static_chunk;
}

static void fini_oow_pool (moo_t* moo, moo_oow_pool_t* pool)
{
	moo_oow_pool_chunk_t* chunk, * next;

	/* dispose all chunks except the first static one */
	chunk = pool->head->next;
	while (chunk)
	{
		next = chunk->next;
		moo_freemem (moo, chunk);
		chunk = next;
	}

	/* this doesn't reinitialize the pool. call init_oow_pool() 
	 * to reuse it */
}

static int add_to_oow_pool (moo_t* moo, moo_oow_pool_t* pool, moo_oow_t v)
{
	moo_oow_t idx;

	idx = pool->count % MOO_COUNTOF(pool->static_chunk.buf);
	if (pool->count > 0 && idx == 0)
	{
		moo_oow_pool_chunk_t* chunk;

		chunk = moo_allocmem (moo, MOO_SIZEOF(pool->static_chunk));
		if (!chunk) return -1;

		chunk->next = MOO_NULL;
		pool->tail->next = chunk;
		pool->tail = chunk;
	}

	pool->tail->buf[idx] = v;
	pool->count++;

	return 0;
}

/* ---------------------------------------------------------------------
 * Tokenizer 
 * --------------------------------------------------------------------- */

#define GET_CHAR(moo) \
	do { if (get_char(moo) <= -1) return -1; } while (0)

#define GET_CHAR_TO(moo,c) \
	do { \
		if (get_char(moo) <= -1) return -1; \
		c = (moo)->c->lxc.c; \
	} while(0)


#define GET_TOKEN(moo) \
	do { if (get_token(moo) <= -1) return -1; } while (0)

#define GET_TOKEN_WITH_ERRRET(moo, v_ret) \
	do { if (get_token(moo) <= -1) return v_ret; } while (0)

#define ADD_TOKEN_STR(moo,s,l) \
	do { if (add_token_str(moo, s, l) <= -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(moo,c) \
	do { if (add_token_char(moo, c) <= -1) return -1; } while (0)

#define CLEAR_TOKEN_NAME(moo) ((moo)->c->tok.name.len = 0)
#define SET_TOKEN_TYPE(moo,tv) ((moo)->c->tok.type = (tv))

#define TOKEN_TYPE(moo)      ((moo)->c->tok.type)
#define TOKEN_NAME(moo)      (&(moo)->c->tok.name)
#define TOKEN_NAME_CAPA(moo) ((moo)->c->tok.name_capa)
#define TOKEN_NAME_PTR(moo)  ((moo)->c->tok.name.ptr)
#define TOKEN_NAME_LEN(moo)  ((moo)->c->tok.name.len)
#define TOKEN_LOC(moo)       (&(moo)->c->tok.loc)
#define LEXER_LOC(moo)       (&(moo)->c->lxc.l)

static MOO_INLINE int does_token_name_match (moo_t* moo, voca_id_t id)
{
	return TOKEN_NAME_LEN(moo) == vocas[id].len &&
	       moo_equaloochars(TOKEN_NAME_PTR(moo), vocas[id].str, vocas[id].len);
}

static MOO_INLINE int is_token_symbol (moo_t* moo, voca_id_t id)
{
	return TOKEN_TYPE(moo) == MOO_IOTOK_SYMLIT && does_token_name_match(moo, id);
}

static MOO_INLINE int is_token_word (moo_t* moo, voca_id_t id)
{
	return TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && does_token_name_match(moo, id);
}

static MOO_INLINE int is_token_binary_selector (moo_t* moo, voca_id_t id)
{
	return TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL && does_token_name_match(moo, id);
}

static MOO_INLINE int is_token_keyword (moo_t* moo, voca_id_t id)
{
	return TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD && does_token_name_match(moo, id);
}


static MOO_INLINE int add_token_str (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len)
{
	moo_oocs_t tmp;

	tmp.ptr = (moo_ooch_t*)ptr;
	tmp.len = len;
	return copy_string_to (moo, &tmp, TOKEN_NAME(moo), &TOKEN_NAME_CAPA(moo), 1, '\0');
}

static MOO_INLINE int add_token_char (moo_t* moo, moo_ooch_t c)
{
	moo_oocs_t tmp;

	tmp.ptr = &c;
	tmp.len = 1;
	return copy_string_to (moo, &tmp, TOKEN_NAME(moo), &TOKEN_NAME_CAPA(moo), 1, '\0');
}

static MOO_INLINE void unget_char (moo_t* moo, const moo_iolxc_t* c)
{
	/* Make sure that the unget buffer is large enough */
	MOO_ASSERT (moo, moo->c->nungots < MOO_COUNTOF(moo->c->ungot));
	moo->c->ungot[moo->c->nungots++] = *c;
}

static int get_char (moo_t* moo)
{
	moo_ooi_t n;
	moo_ooci_t lc, ec;

	if (moo->c->nungots > 0)
	{
		/* something in the unget buffer */
		moo->c->lxc = moo->c->ungot[--moo->c->nungots];
		return 0;
	}

	if (moo->c->curinp->b.state == -1) 
	{
		moo->c->curinp->b.state = 0;
		return -1;
	}
	else if (moo->c->curinp->b.state == 1) 
	{
		moo->c->curinp->b.state = 0;
		goto return_eof;
	}

	if (moo->c->curinp->b.pos >= moo->c->curinp->b.len)
	{
		n = moo->c->impl (moo, MOO_IO_READ, moo->c->curinp);
		if (n <= -1) return -1;
		
		if (n == 0)
		{
		return_eof:
			moo->c->curinp->lxc.c = MOO_UCI_EOF;
			moo->c->curinp->lxc.l.line = moo->c->curinp->line;
			moo->c->curinp->lxc.l.colm = moo->c->curinp->colm;
			moo->c->curinp->lxc.l.file = moo->c->curinp->name;
			moo->c->lxc = moo->c->curinp->lxc;

			/* indicate that EOF has been read. lxc.c is also set to EOF. */
			return 0; 
		}

		moo->c->curinp->b.pos = 0;
		moo->c->curinp->b.len = n;
	}

	if (moo->c->curinp->lxc.c == '\n' || moo->c->curinp->lxc.c == '\r')
	{
		/* moo->c->curinp->lxc.c is a previous character. the new character
		 * to be read is still in the buffer (moo->c->curinp->buf).
		 * moo->cu->curinp->colm has been incremented when the previous
		 * character has been read. */
		if (moo->c->curinp->line > 1 && 
		    moo->c->curinp->colm == 2 &&
		    moo->c->curinp->nl != moo->c->curinp->lxc.c) 
		{
			/* most likely, it's the second character in '\r\n' or '\n\r' 
			 * sequence. let's not update the line and column number. */
			/*moo->c->curinp->colm = 1;*/
		}
		else
		{
			/* if the previous charater was a newline,
			 * increment the line counter and reset column to 1.
			 * incrementing the line number here instead of
			 * updating inp->lxc causes the line number for
			 * TOK_EOF to be the same line as the lxc newline. */
			moo->c->curinp->line++;
			moo->c->curinp->colm = 1;
			moo->c->curinp->nl = moo->c->curinp->lxc.c;
		}
	}

	lc = moo->c->curinp->buf[moo->c->curinp->b.pos++];

	moo->c->curinp->lxc.c = lc;
	moo->c->curinp->lxc.l.line = moo->c->curinp->line;
	moo->c->curinp->lxc.l.colm = moo->c->curinp->colm++;
	moo->c->curinp->lxc.l.file = moo->c->curinp->name;
	moo->c->lxc = moo->c->curinp->lxc;

	return 1; /* indicate that a normal character has been read */
}

static MOO_INLINE int skip_spaces (moo_t* moo)
{
	while (is_spacechar(moo->c->lxc.c)) GET_CHAR (moo);
	return 0;
}

static int skip_comment (moo_t* moo)
{
	moo_ooci_t c = moo->c->lxc.c;
	moo_iolxc_t lc;

	if (c == '"')
	{
		/* skip up to the closing " */
		do 
		{
			GET_CHAR_TO (moo, c); 
			if (c == MOO_UCI_EOF) goto unterminated;
		}
		while (c != '"');

		if (c == '"') GET_CHAR (moo); /* keep the next character in lxc */
		return 1; /* double-quoted comment */
	}
	else if (c == '(')
	{
		/* handle (* ... *) */
		lc = moo->c->lxc;
		GET_CHAR_TO (moo, c);
		if (c != '*') goto not_comment;

		do 
		{
			GET_CHAR_TO (moo, c);
			if (c == MOO_UCI_EOF) goto unterminated;

			if (c == '*')
			{
				GET_CHAR_TO (moo, c);
				if (c == MOO_UCI_EOF) goto unterminated;

				if (c == ')')
				{
					GET_CHAR (moo); /* keep the first meaningful character in lxc */
					break;
				}
			}
		} 
		while (1);

		return 1; /* multi-line comment enclosed in (* and *) */
	}
	else if (c == '#')
	{
		/* handle #! or ## */

		/* save the last character */
		lc = moo->c->lxc;
		/* read a new character */
		GET_CHAR_TO (moo, c);

		if (c != '!' && c != '#') goto not_comment;
		do 
		{
			GET_CHAR_TO (moo, c);
			if (c == MOO_UCI_EOF)
			{
				/* EOF on the comment line is ok for a single-line comment */
				break;
			}
			else if (c == '\r' || c == '\n')
			{
				GET_CHAR (moo); /* keep the first meaningful character in lxc */
				break;
			}
		} 
		while (1);

		return 1; /* single line comment led by ## or #! */
	}
	else 
	{
		/* not comment. but no next character has been consumed.
		 * no need to unget a character */
		return 0;
	}

not_comment:
	/* unget the leading '#' */
	unget_char (moo, &moo->c->lxc);
	/* restore the previous state */
	moo->c->lxc = lc;

	return 0;


unterminated:
	set_syntax_error (moo, MOO_SYNERR_CMTNC, LEXER_LOC(moo), MOO_NULL);
	return -1;
}

static int get_ident (moo_t* moo, moo_ooci_t char_read_ahead)
{
	/*
	 * identifier := alpha-char (alpha-char | digit-char)*
	 * keyword := identifier ":"
	 */

	moo_ooci_t c;

	c = moo->c->lxc.c;
	SET_TOKEN_TYPE (moo, MOO_IOTOK_IDENT);

	if (char_read_ahead != MOO_UCI_EOF)
	{
		ADD_TOKEN_CHAR(moo, char_read_ahead);
	}

	do 
	{
		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR_TO (moo, c);
	} 
	while (is_identchar(c));

	if (c == '(' && is_token_word(moo, VOCA_ERROR))
	{
		/* error(NN) */
		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR_TO (moo, c);
		if (!is_digitchar(c))
		{
			ADD_TOKEN_CHAR (moo, c);
			set_syntax_error (moo, MOO_SYNERR_ERRLIT, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		do
		{
			ADD_TOKEN_CHAR (moo, c);
			GET_CHAR_TO (moo, c);
		}
		while (is_digitchar(c));

		if (c != ')')
		{
			set_syntax_error (moo, MOO_SYNERR_RPAREN, LEXER_LOC(moo), MOO_NULL);
			return -1;
		}

/* TODO: error number range check */

		ADD_TOKEN_CHAR (moo, c);
		SET_TOKEN_TYPE (moo, MOO_IOTOK_ERRLIT);
	}
	else if (c == ':') 
	{
	read_more_kwsym:
		ADD_TOKEN_CHAR (moo, c);
		SET_TOKEN_TYPE (moo, MOO_IOTOK_KEYWORD);
		GET_CHAR_TO (moo, c);

		if (moo->c->in_array && is_leadidentchar(c)) 
		{
			/* when reading an array literal, read as many characters as
			 * would compose a normal keyword symbol literal.
			 * for example, in #(a #b:c: x:y:) x:y: is not preceded
			 * by #. in an array literal, it should still be treated as 
			 * a symbol. */
			do
			{
				ADD_TOKEN_CHAR (moo, c);
				GET_CHAR_TO (moo, c);
			}
			while (is_identchar(c));

			if (c == ':') goto read_more_kwsym;
			else
			{
				/* the last character is not a colon */
				set_syntax_error (moo, MOO_SYNERR_COLON, LEXER_LOC(moo), MOO_NULL);
				return -1;
			}
		}
		else
		{
			unget_char (moo, &moo->c->lxc); 
		}
	}
	else
	{
		if (c == '.')
		{
			moo_iolxc_t period;

			period = moo->c->lxc;

		read_more_seg:
			GET_CHAR_TO (moo, c);

			if (is_leadidentchar(c))
			{
				SET_TOKEN_TYPE (moo, MOO_IOTOK_IDENT_DOTTED);

				ADD_TOKEN_CHAR (moo, '.');
				do
				{
					ADD_TOKEN_CHAR (moo, c);
					GET_CHAR_TO (moo, c);
				}
				while (is_identchar(c));

				if (c == '.') goto read_more_seg;
				else unget_char (moo, &moo->c->lxc); 
			}
			else
			{
				unget_char (moo, &moo->c->lxc); 

				/* unget the period itself */
				unget_char (moo, &period); 
			}
		}
		else
		{
			unget_char (moo, &moo->c->lxc); 
		}

		/* handle reserved words */
		if (is_token_word(moo, VOCA_SELF))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_SELF);
		}
		else if (is_token_word(moo, VOCA_SUPER))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_SUPER);
		}
		else if (is_token_word(moo, VOCA_NIL))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_NIL);
		}
		else if (is_token_word(moo, VOCA_TRUE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_TRUE);
		}
		else if (is_token_word(moo, VOCA_FALSE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_FALSE);
		}
		else if (is_token_word(moo, VOCA_ERROR))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ERROR);
		}
		else if (is_token_word(moo, VOCA_THIS_CONTEXT))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_THIS_CONTEXT);
		}
		else if (is_token_word(moo, VOCA_THIS_PROCESS))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_THIS_PROCESS);
		}
		else if (is_token_word(moo, VOCA_IF))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_IF);
		}
		else if (is_token_word(moo, VOCA_ELSE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ELSE);
		}
		else if (is_token_word(moo, VOCA_ELSIF))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ELSIF);
		}
		else if (is_token_word(moo, VOCA_WHILE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_WHILE);
		}
		else if (is_token_word(moo, VOCA_DO))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_DO);
		}
		else if (is_token_word(moo, VOCA_BREAK))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_BREAK);
		}
		else if (is_token_word(moo, VOCA_CONTINUE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_CONTINUE);
		}
	}

	return 0;
}

static int get_numlit (moo_t* moo, int negated)
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

	moo_ooci_t c;
	int radix = 0, r;
	

	c = moo->c->lxc.c;
	SET_TOKEN_TYPE (moo, MOO_IOTOK_NUMLIT);

/*TODO: support a complex numeric literal */
	do 
	{
		if (radix <= 36)
		{
			/* collect the potential radix specifier */
			r = CHAR_TO_NUM (c, 10);
			MOO_ASSERT (moo, r < 10);
			radix = radix * 10 + r;
		}

		ADD_TOKEN_CHAR(moo, c);
		GET_CHAR_TO (moo, c);
		if (c == '_')
		{
			moo_iolxc_t underscore;
			underscore = moo->c->lxc;
			GET_CHAR_TO(moo, c);
			if (!is_digitchar(c))
			{
				unget_char (moo, &moo->c->lxc);
				unget_char (moo, &underscore);
				break;
			}
			else continue;
		}
	} 
	while (is_digitchar(c));

	if (c == 'r')
	{
		/* radix specifier */

		if (radix < 2 || radix > 36)
		{
			/* no digit after the radix specifier */
			set_syntax_error (moo, MOO_SYNERR_RADIX, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		ADD_TOKEN_CHAR(moo, c);
		GET_CHAR_TO (moo, c);

		if (CHAR_TO_NUM(c, radix) >= radix)
		{
			/* no digit after the radix specifier */
			set_syntax_error (moo, MOO_SYNERR_RADNUMLIT, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		do
		{
			ADD_TOKEN_CHAR(moo, c);
			GET_CHAR_TO (moo, c);
			if (c == '_') 
			{
				moo_iolxc_t underscore;
				underscore = moo->c->lxc;
				GET_CHAR_TO(moo, c);
				if (CHAR_TO_NUM(c, radix) >= radix)
				{
					unget_char (moo, &moo->c->lxc);
					unget_char (moo, &underscore);
					break;
				}
				else continue;
			}
		}
		while (CHAR_TO_NUM(c, radix) < radix);

		SET_TOKEN_TYPE (moo, MOO_IOTOK_RADNUMLIT);
	}

	unget_char (moo, &moo->c->lxc);

/*
 * TODO: handle floating point number
 */
	return 0;
}

static int get_charlit (moo_t* moo)
{
	/* 
	 * character-literal := "$" character
	 * character := normal-character | "'"
	 */

	moo_ooci_t c = moo->c->lxc.c; /* even a new-line or white space would be taken */
	if (c == MOO_UCI_EOF) 
	{
		set_syntax_error (moo, MOO_SYNERR_CLTNT, LEXER_LOC(moo), MOO_NULL);
		return -1;
	}

	SET_TOKEN_TYPE (moo, MOO_IOTOK_CHARLIT);
	ADD_TOKEN_CHAR(moo, c);
	return 0;
}

static int get_strlit (moo_t* moo)
{
	/* 
	 * string-literal := single-quote string-character* single-quote
	 * string-character := normal-character | (single-quote single-quote)
	 * single-quote := "'"
	 * normal-character := character-except-single-quote
	 */

	moo_ooci_t c = moo->c->lxc.c;
	SET_TOKEN_TYPE (moo, MOO_IOTOK_STRLIT);

	do 
	{
		do 
		{
			ADD_TOKEN_CHAR (moo, c);
			GET_CHAR_TO (moo, c);

			if (c == MOO_UCI_EOF) 
			{
				/* string not closed */
				set_syntax_error (moo, MOO_SYNERR_STRNC, TOKEN_LOC(moo) /*&moo->c->lxc.l*/, MOO_NULL);
				return -1;
			}
		} 
		while (c != '\'');

		/* 'c' must be a single quote at this point*/
		GET_CHAR_TO (moo, c);
	} 
	while (c == '\''); /* if the next character is a single quote,
	                      it becomes a literal single quote character. */

	unget_char (moo, &moo->c->lxc);
	return 0;
}

static int get_string (moo_t* moo, moo_ooch_t end_char, moo_ooch_t esc_char, int regex, moo_oow_t preescaped)
{
	moo_ooci_t c;
	moo_oow_t escaped = preescaped;
	moo_oow_t digit_count = 0;
	moo_ooci_t c_acc = 0;

	SET_TOKEN_TYPE (moo, MOO_IOTOK_STRLIT);

	while (1)
	{
		GET_CHAR_TO (moo, c);

		if (c == MOO_UCI_EOF)
		{
			set_syntax_error (moo, MOO_SYNERR_STRNC, TOKEN_LOC(moo) /*&moo->c->lxc.l*/, MOO_NULL);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= '0' && c <= '7')
			{
				c_acc = c_acc * 8 + c - '0';
				digit_count++;
				if (digit_count >= escaped) 
				{
					/* should i limit the max to 0xFF/0377? 
					 * if (c_acc > 0377) c_acc = 0377;*/
					ADD_TOKEN_CHAR (moo, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ADD_TOKEN_CHAR (moo, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= '0' && c <= '9')
			{
				c_acc = c_acc * 16 + c - '0';
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (moo, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= 'A' && c <= 'F')
			{
				c_acc = c_acc * 16 + c - 'A' + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (moo, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= 'a' && c <= 'f')
			{
				c_acc = c_acc * 16 + c - 'a' + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (moo, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				moo_ooch_t rc;

				rc = (escaped == 2)? 'x':
				     (escaped == 4)? 'u': 'U';
				if (digit_count == 0) 
					ADD_TOKEN_CHAR (moo, rc);
				else ADD_TOKEN_CHAR (moo, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == end_char)
		{
			/* terminating quote */
			break;
		}

		if (escaped == 0 && c == esc_char)
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == 'n') c = '\n';
			else if (c == 'r') c = '\r';
			else if (c == 't') c = '\t';
			else if (c == 'f') c = '\f';
			else if (c == 'b') c = '\b';
			else if (c == 'v') c = '\v';
			else if (c == 'a') c = '\a';
			else if (c >= '0' && c <= '7' && !regex) 
			{
				/* i don't support the octal notation for a regular expression.
				 * it conflicts with the backreference notation between \1 and \7 inclusive. */
				escaped = 3;
				digit_count = 1;
				c_acc = c - '0';
				continue;
			}
			else if (c == 'x') 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == 'u' && MOO_SIZEOF(moo_ooch_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == 'U' && MOO_SIZEOF(moo_ooch_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (regex) 
			{
				/* if the following character doesn't compose a proper
				 * escape sequence, keep the escape character. 
				 * an unhandled escape sequence can be handled 
				 * outside this function since the escape character 
				 * is preserved.*/
				ADD_TOKEN_CHAR (moo, esc_char);
			}

			escaped = 0;
		}

		ADD_TOKEN_CHAR (moo, c);
	}

	return 0;
}

static int get_binsel (moo_t* moo)
{
	/* 
	 * binary-selector := binary-selector-character+
	 */
	moo_ooci_t oc;

	oc = moo->c->lxc.c;
	ADD_TOKEN_CHAR (moo, oc);

	GET_CHAR (moo);
	/* special case if a minus is followed by a digit immediately */
	if (oc == '-' && is_digitchar(moo->c->lxc.c)) return get_numlit (moo, 1);

#if 1
	/* up to 2 characters only */
	if (is_binselchar (moo->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (moo, moo->c->lxc.c);
	}
	else
	{
		unget_char (moo, &moo->c->lxc);
	}
#else
	/* or up to any occurrences */
	while (is_binselchar(moo->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR (moo);
	}
	unget_char (moo, &moo->c->lxc);
#endif

	SET_TOKEN_TYPE (moo, MOO_IOTOK_BINSEL);
	return 0;
}

static int get_token (moo_t* moo)
{
	moo_ooci_t c;
	int n;

retry:
	GET_CHAR (moo);

	do 
	{
		/* skip spaces */
		while (is_spacechar(moo->c->lxc.c)) GET_CHAR (moo);
		/* the first character after the last space is in moo->c->lxc */
		if ((n = skip_comment(moo)) <= -1) return -1;
	} 
	while (n >= 1);

	/* clear the token name, reset its location */
	SET_TOKEN_TYPE (moo, MOO_IOTOK_EOF); /* is it correct? */
	CLEAR_TOKEN_NAME (moo);
	moo->c->tok.loc = moo->c->lxc.l; /* remember token location */

	c = moo->c->lxc.c;

	switch (c)
	{
		case MOO_UCI_EOF:
		{
			int n;

			n = end_include (moo);
			if (n <= -1) return -1;
			if (n >= 1) goto retry;

			SET_TOKEN_TYPE (moo, MOO_IOTOK_EOF);
			ADD_TOKEN_STR(moo, vocas[VOCA_EOF].str, vocas[VOCA_EOF].len);
			break;
		}

		case '$': /* character literal */
			GET_CHAR (moo);
			if (get_charlit(moo) <= -1) return -1;
			break;

		case '\'': /* string literal */
			GET_CHAR (moo);
			if (get_strlit(moo) <= -1) return -1;
			break;

		case ':':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_COLON);
			ADD_TOKEN_CHAR (moo, c);
			GET_CHAR_TO (moo, c);
			if (c == '=') 
			{
				SET_TOKEN_TYPE (moo, MOO_IOTOK_ASSIGN);
				ADD_TOKEN_CHAR (moo, c);
			}
			else if (c == '{')
			{
				SET_TOKEN_TYPE (moo, MOO_IOTOK_DICBRACE);
				ADD_TOKEN_CHAR (moo, c);
			}
			else if (c == '(')
			{
				SET_TOKEN_TYPE (moo, MOO_IOTOK_ASSPAREN);
				ADD_TOKEN_CHAR (moo, c);
			}
			else
			{
				unget_char (moo, &moo->c->lxc);
			}
			break;

		case '^':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_RETURN);
			ADD_TOKEN_CHAR(moo, c);
			GET_CHAR_TO (moo, c);

			if (c == '^')
			{
				/* ^^ */
				TOKEN_TYPE(moo) = MOO_IOTOK_LOCAL_RETURN;
				ADD_TOKEN_CHAR (moo, c);
			}
			else
			{
				unget_char (moo, &moo->c->lxc);
			}
			break;

		case '{': /* extension */
			SET_TOKEN_TYPE (moo, MOO_IOTOK_LBRACE);
			goto single_char_token;
		case '}': /* extension */
			SET_TOKEN_TYPE (moo, MOO_IOTOK_RBRACE);
			goto single_char_token;
		case '[':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_LBRACK);
			goto single_char_token;
		case ']': 
			SET_TOKEN_TYPE (moo, MOO_IOTOK_RBRACK);
			goto single_char_token;
		case '(':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_LPAREN);
			goto single_char_token;
		case ')':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_RPAREN);
			goto single_char_token;
		case '.':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_PERIOD);
			goto single_char_token;
		case ',':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_COMMA);
			goto single_char_token;
		case ';':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_SEMICOLON);
			goto single_char_token;

		case '#':  
			ADD_TOKEN_CHAR(moo, c);
			GET_CHAR_TO (moo, c);
			switch (c)
			{
				case MOO_UCI_EOF:
					set_syntax_error (moo, MOO_SYNERR_HLTNT, LEXER_LOC(moo), MOO_NULL);
					return -1;

				case '(':
					/* #( - array literal */
					ADD_TOKEN_CHAR(moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_APAREN);
					break;

				case '[':
					/* #[ - byte array literal */
					ADD_TOKEN_CHAR(moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_BABRACK);
					break;

				case '{':
					/* #{ - array expression */
					ADD_TOKEN_CHAR(moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_ABRACE);
					break;

				case '\'':
					/* quoted symbol literal */
					GET_CHAR (moo);
					if (get_strlit(moo) <= -1) return -1;
					SET_TOKEN_TYPE (moo, MOO_IOTOK_SYMLIT);
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
							ADD_TOKEN_CHAR (moo, c);
							GET_CHAR_TO (moo, c);
						} 
						while (is_binselchar(c));

						unget_char (moo, &moo->c->lxc);
					}
					else if (is_leadidentchar(c)) 
					{
						do 
						{
							ADD_TOKEN_CHAR (moo, c);
							GET_CHAR_TO (moo, c);
						} 
						while (is_identchar(c));

						if (c == ':')
						{
							/* keyword symbol - e.g. #ifTrue:ifFalse: */
						read_more_word:
							ADD_TOKEN_CHAR (moo, c);
							GET_CHAR_TO (moo, c);

							if (is_leadidentchar(c))
							{
								do 
								{
									ADD_TOKEN_CHAR (moo, c);
									GET_CHAR_TO (moo, c);
								} 
								while (is_identchar(c));

								if (c == ':') goto read_more_word;
								else
								{
									/* if a colon is found in the middle of a symbol,
									 * the last charater is expected to be a colon as well.
									 * but, the last character is not a colon */
									set_syntax_error (moo, MOO_SYNERR_COLON, LEXER_LOC(moo), MOO_NULL);
									return -1;
								}
							}
							else
							{
								unget_char (moo, &moo->c->lxc);
							}
						}
						else if (c == '.')
						{
							/* dotted symbol e.g. #Planet.Earth.Object */

							moo_iolxc_t period;

							period = moo->c->lxc;

						read_more_seg:
							GET_CHAR_TO (moo, c);

							if (is_leadidentchar(c))
							{
								ADD_TOKEN_CHAR (moo, '.');
								do
								{
									ADD_TOKEN_CHAR (moo, c);
									GET_CHAR_TO (moo, c);
								}
								while (is_identchar(c));

								if (c == '.') goto read_more_seg;
								else unget_char (moo, &moo->c->lxc);
							}
							else
							{
								unget_char (moo, &moo->c->lxc); 
								unget_char (moo, &period); 
							}
						}
						else
						{
							unget_char (moo, &moo->c->lxc); 
						}
					}
					else
					{
						set_syntax_error (moo, MOO_SYNERR_HLTNT, LEXER_LOC(moo), MOO_NULL);
						return -1;
					}

					SET_TOKEN_TYPE (moo, MOO_IOTOK_SYMLIT);
					break;
			}

			break;

		case 'C': /* a character with a C-style escape sequence */
		case 'S': /* a string with a C-style escape sequences */
		case 'M': /* a symbol with a C-style escape sequences */
		{
			moo_ooci_t saved_c = c;

			GET_CHAR_TO (moo, c);
			if (c == '\'')
			{
				/*GET_CHAR (moo);*/
				if (get_string(moo, '\'', '\\', 0, 0) <= -1) return -1;

				if (saved_c == 'C')
				{
					if (moo->c->tok.name.len != 1)
					{
						set_syntax_error (moo, MOO_SYNERR_CHARLIT, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}
					SET_TOKEN_TYPE (moo, MOO_IOTOK_CHARLIT);
				}
				else if (saved_c == 'M')
				{
					SET_TOKEN_TYPE (moo, MOO_IOTOK_SYMLIT);
				}
			}
			else
			{
				if (get_ident(moo, saved_c) <= -1) return -1;
			}

			break;
		}

	/*	case 'B': TODO: byte string  with a c-style escape sequence? */

	/*  case 'R':
			TODO: regular expression?
			GET_CHAR_TO (moo, c);
			if (c == '\'')
			{
				GET_CHAR (moo);
				if (get_rexlit(moo) <= -1) return -1;
			}
			else
			{
				if (get_ident(moo, 'R') <= -1) return -1;
			}
			break;
	 */

		default:
			if (is_leadidentchar(c)) 
			{
				if (get_ident(moo, MOO_UCI_EOF) <= -1) return -1;
			}
			else if (is_digitchar(c)) 
			{
				if (get_numlit(moo, 0) <= -1) return -1;
			}
			else if (is_binselchar(c)) 
			{
				/* binary selector */
				if (get_binsel(moo) <= -1) return -1;
			}
			else 
			{
				moo->c->ilchr = (moo_ooch_t)c;
				set_syntax_error (moo, MOO_SYNERR_ILCHR, LEXER_LOC(moo), &moo->c->ilchr_ucs);
				return -1;
			}
			break;

		single_char_token:
			ADD_TOKEN_CHAR(moo, c);
			break;
	}

MOO_DEBUG2 (moo, "TOKEN: [%.*js]\n", (moo_ooi_t)moo->c->tok.name.len, moo->c->tok.name.ptr);

	return 0;
}

static void clear_io_names (moo_t* moo)
{
	moo_iolink_t* cur;

	MOO_ASSERT (moo, moo->c != MOO_NULL);

	while (moo->c->io_names)
	{
		cur = moo->c->io_names;
		moo->c->io_names = cur->link;
		moo_freemem (moo, cur);
	}
}

static const moo_ooch_t* add_io_name (moo_t* moo, const moo_oocs_t* name)
{
	moo_iolink_t* link;
	moo_ooch_t* ptr;

	link = (moo_iolink_t*) moo_callocmem (moo, MOO_SIZEOF(*link) + MOO_SIZEOF(moo_ooch_t) * (name->len + 1));
	if (!link) return MOO_NULL;

	ptr = (moo_ooch_t*)(link + 1);

	moo_copyoochars (ptr, name->ptr, name->len);
	ptr[name->len] = '\0';

	link->link = moo->c->io_names;
	moo->c->io_names = link;

	return ptr;
}

static int begin_include (moo_t* moo)
{
	moo_ioarg_t* arg;
	const moo_ooch_t* io_name;

	io_name = add_io_name (moo, TOKEN_NAME(moo));
	if (!io_name) return -1;

	arg = (moo_ioarg_t*) moo_callocmem (moo, MOO_SIZEOF(*arg));
	if (!arg) goto oops;

	arg->name = io_name;
	arg->line = 1;
	arg->colm = 1;
	/*arg->nl = '\0';*/
	arg->includer = moo->c->curinp;

	if (moo->c->impl (moo, MOO_IO_OPEN, arg) <= -1) 
	{
		set_syntax_error (moo, MOO_SYNERR_INCLUDE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

	GET_TOKEN (moo);
	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		/* check if a period is following the includee name */
		set_syntax_error (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

	/* switch to the includee's stream */
	moo->c->curinp = arg;
	/* moo->c->depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_token(moo) <= -1) 
	{
		end_include (moo); 
		/* i don't jump to oops since i've called 
		 * end_include() which frees moo->c->curinp/arg */
		return -1;
	}

	return 0;

oops:
	if (arg) moo_freemem (moo, arg);
	return -1;
}

static int end_include (moo_t* moo)
{
	int x;
	moo_ioarg_t* cur;

	if (moo->c->curinp == &moo->c->arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = moo->c->impl (moo, MOO_IO_CLOSE, moo->c->curinp);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call MOO_IO_CLOSE on 
	 * moo->c->curinp again. */

	cur = moo->c->curinp;
	moo->c->curinp = moo->c->curinp->includer;

	MOO_ASSERT (moo, cur->name != MOO_NULL);
	moo_freemem (moo, cur);
	/* moo->parse.depth.incl--; */

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		return -1;
	}

	moo->c->lxc = moo->c->curinp->lxc;
	return 1; /* ended the included file successfully */
}

/* ---------------------------------------------------------------------
 * Byte-Code Manipulation Functions
 * --------------------------------------------------------------------- */

static MOO_INLINE int emit_byte_instruction (moo_t* moo, moo_oob_t code)
{
	/* the context object has the ip field. it should be representable
	 * in a small integer. for simplicity, limit the total byte code length
	 * to fit in a small integer. because 'ip' points to the next instruction
	 * to execute, he upper bound should be (max - 1) so that i stays
	 * at the max when incremented */
	if (moo->c->mth.code.len == MOO_SMOOI_MAX - 1)
	{
		moo->errnum = MOO_EBCFULL; /* byte code too big */
		return -1;
	}

	if (moo->c->mth.code.len >= moo->c->mth.code_capa)
	{
		moo_oob_t* tmp;
		moo_oow_t newcapa;

		newcapa = MOO_ALIGN (moo->c->mth.code.len + 1, CODE_BUFFER_ALIGN);

		tmp = moo_reallocmem (moo, moo->c->mth.code.ptr, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->mth.code.ptr = tmp;
		moo->c->mth.code_capa = newcapa;
	}

	moo->c->mth.code.ptr[moo->c->mth.code.len++] = code;
	return 0;
}

static int emit_single_param_instruction (moo_t* moo, int cmd, moo_oow_t param_1)
{
	moo_oob_t bc;

	switch (cmd)
	{
		case BCODE_PUSH_INSTVAR_0:
		case BCODE_STORE_INTO_INSTVAR_0:
		case BCODE_POP_INTO_INSTVAR_0:
		case BCODE_PUSH_TEMPVAR_0:
		case BCODE_STORE_INTO_TEMPVAR_0:
		case BCODE_POP_INTO_TEMPVAR_0:
		case BCODE_PUSH_LITERAL_0:
			if (param_1 < 8)
			{
				/* low 3 bits to hold the parameter */
				bc = (moo_oob_t)(cmd & 0xF8) | (moo_oob_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the code to a long version */
				bc = cmd | 0x80;
				goto write_long;
			}

		case BCODE_PUSH_OBJECT_0:
		case BCODE_STORE_INTO_OBJECT_0:
		case BCODE_POP_INTO_OBJECT_0:
			if (param_1 < 4)
			{
				/* low 2 bits to hold the parameter */
				bc = (moo_oob_t)(cmd & 0xFC) | (moo_oob_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the instruction to a long version (_X) */
				bc = cmd | 0x80;
				goto write_long;
			}


		case BCODE_JUMP_FORWARD_0:
		case BCODE_JUMP_BACKWARD_0:
		case BCODE_JUMP_FORWARD_IF_FALSE_0:
		case BCODE_JUMP_BACKWARD_IF_FALSE_0:
		case BCODE_JUMP_BACKWARD_IF_TRUE_0:
			if (param_1 < 4)
			{
				/* low 2 bits to hold the parameter */
				bc = (moo_oob_t)(cmd & 0xFC) | (moo_oob_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the instruction to a long version (_X) */
				bc = cmd | 0x80;
				if (param_1 > MAX_CODE_JUMP)
				{
					cmd = cmd + 1; /* convert to a JUMP2 instruction */
					param_1 = param_1 - MAX_CODE_JUMP;
				}
				goto write_long;
			}

		case BCODE_JUMP2_FORWARD:
		case BCODE_JUMP2_BACKWARD:
		case BCODE_JUMP2_FORWARD_IF_FALSE:
		case BCODE_JUMP2_BACKWARD_IF_FALSE:
		case BCODE_JUMP2_BACKWARD_IF_TRUE:
		case BCODE_PUSH_INTLIT:
		case BCODE_PUSH_NEGINTLIT:
		case BCODE_PUSH_CHARLIT:
		case BCODE_MAKE_DICTIONARY:
		case BCODE_MAKE_ARRAY:
		case BCODE_POP_INTO_ARRAY:
			bc = cmd;
			goto write_long;
	}

	MOO_DEBUG1 (moo, "Invalid single param instruction opcode %d\n", (int)cmd);
	moo->errnum = MOO_EINVAL;
	return -1;

write_short:
	if (emit_byte_instruction(moo, bc) <= -1) return -1;
	return 0;

write_long:
	if (param_1 > MAX_CODE_PARAM) 
	{
		moo->errnum = MOO_ERANGE;
		return -1;
	}
#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, (param_1 >> 8) & 0xFF) <= -1 ||
	    emit_byte_instruction(moo, param_1 & 0xFF) <= -1) return -1;
#else
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, param_1) <= -1) return -1;
#endif
	return 0;
}

static int emit_double_param_instruction (moo_t* moo, int cmd, moo_oow_t param_1, moo_oow_t param_2)
{
	moo_oob_t bc;

	switch (cmd)
	{
		case BCODE_STORE_INTO_CTXTEMPVAR_0:
		case BCODE_POP_INTO_CTXTEMPVAR_0:
		case BCODE_PUSH_CTXTEMPVAR_0:
		case BCODE_PUSH_OBJVAR_0:
		case BCODE_STORE_INTO_OBJVAR_0:
		case BCODE_POP_INTO_OBJVAR_0:
		case BCODE_SEND_MESSAGE_0:
		case BCODE_SEND_MESSAGE_TO_SUPER_0:
			if (param_1 < 4 && param_2 < 0xFF)
			{
				/* low 2 bits of the instruction code is the first parameter */
				bc = (moo_oob_t)(cmd & 0xFC) | (moo_oob_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the code to a long version */
				bc = cmd | 0x80;
				goto write_long;
			}

		case BCODE_MAKE_BLOCK:
			bc = cmd;
			goto write_long;
	}

	MOO_DEBUG1 (moo, "Invalid double param instruction opcode %d\n", (int)cmd);
	moo->errnum = MOO_EINVAL;
	return -1;

write_short:
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, param_2) <= -1) return -1;
	return 0;

write_long:
	if (param_1 > MAX_CODE_PARAM || param_2 > MAX_CODE_PARAM) 
	{
		moo->errnum = MOO_ERANGE;
		return -1;
	}
#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, (param_1 >> 8) & 0xFF) <= -1 ||
	    emit_byte_instruction(moo, param_1 & 0xFF) <= -1 ||
	    emit_byte_instruction(moo, (param_2 >> 8) & 0xFF) <= -1 ||
	    emit_byte_instruction(moo, param_2 & 0xFF) <= -1) return -1;
#else
	
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, param_1) <= -1 ||
	    emit_byte_instruction(moo, param_2) <= -1) return -1;
#endif
	return 0;
}

static int emit_push_smooi_literal (moo_t* moo, moo_ooi_t i)
{
	moo_oow_t index;

	switch (i)
	{
		case -1:
			return emit_byte_instruction (moo, BCODE_PUSH_NEGONE);

		case 0:
			return emit_byte_instruction (moo, BCODE_PUSH_ZERO);

		case 1:
			return emit_byte_instruction (moo, BCODE_PUSH_ONE);

		case 2:
			return emit_byte_instruction (moo, BCODE_PUSH_TWO);
	}

	if (i >= 0 && i <= MAX_CODE_PARAM)
	{
		return emit_single_param_instruction(moo, BCODE_PUSH_INTLIT, i);
	}
	else if (i < 0 && i >= -(moo_ooi_t)MAX_CODE_PARAM)
	{
		return emit_single_param_instruction(moo, BCODE_PUSH_NEGINTLIT, -i);
	}

	if (add_literal(moo, MOO_SMOOI_TO_OOP(i), &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	return 0;
}

static int emit_push_character_literal (moo_t* moo, moo_ooch_t ch)
{
	moo_oow_t index;

	if (ch >= 0 && ch <= MAX_CODE_PARAM)
	{
		return emit_single_param_instruction (moo, BCODE_PUSH_CHARLIT, ch);
	}

	if (add_literal(moo, MOO_CHAR_TO_OOP(ch), &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	return 0;
}

static MOO_INLINE int emit_backward_jump_instruction (moo_t* moo, int cmd, moo_oow_t offset)
{
	moo_oow_t adj;

	MOO_ASSERT (moo, cmd == BCODE_JUMP_BACKWARD_0 ||
	                 cmd == BCODE_JUMP_BACKWARD_IF_FALSE_0 ||
	                 cmd == BCODE_JUMP_BACKWARD_IF_TRUE_0);
	
	/* the short BCODE_JUMP_BACKWARD instructions use low 2 bits to encode 
	 * the jump offset. so it can encode 0, 1, 2, 3. the instruction itself 
	 * is 1 byte long. the offset value of 0, 1, 2 can get encoded into the
	 * instruction, which result in 1, 2, 3 when combined with the length 1
	 * of the instruction itself */


	adj = (offset < 3)? 1: (MOO_BCODE_LONG_PARAM_SIZE + 1);
	return emit_single_param_instruction (moo, cmd, offset + adj);
}

static int patch_long_forward_jump_instruction (moo_t* moo, moo_oow_t jip, moo_oow_t jt, moo_oob_t jump2_inst, moo_ioloc_t* errloc)
{
	moo_oow_t code_size;
	moo_oow_t jump_offset;

	/* jip - jump instruction pointer, jt - jump target *
	 * 
	 * when this jump instruction is executed, the instruction pointer advances
	 * to the next instruction. so the actual jump size gets offset by the size
	 * of this jump instruction. MOO_BCODE_LONG_PARAM_SIZE + 1 is the size of
	 * the long JUMP_FORWARD instruction */
	code_size = jt - jip - (MOO_BCODE_LONG_PARAM_SIZE + 1);
	if (code_size > MAX_CODE_JUMP * 2)
	{
/* TODO: change error code or get it as a parameter */
		set_syntax_error (moo, MOO_SYNERR_BLKFLOOD, errloc, MOO_NULL); 
		return -1;
	}

	if (code_size > MAX_CODE_JUMP)
	{
		/* switch to JUMP2 instruction to allow a bigger jump offset.
		 * up to twice MAX_CODE_JUMP only */
		moo->c->mth.code.ptr[jip] = jump2_inst;
		jump_offset = code_size - MAX_CODE_JUMP;
	}
	else
	{
		jump_offset = code_size;
	}

#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
	moo->c->mth.code.ptr[jip + 1] = jump_offset >> 8;
	moo->c->mth.code.ptr[jip + 2] = jump_offset & 0xFF;
#else
	moo->c->mth.code.ptr[jip + 1] = jump_offset;
#endif

	return 0;
}

static int push_loop (moo_t* moo, moo_loop_type_t type, moo_oow_t startpos)
{
	moo_loop_t* loop;

	loop = moo_callocmem (moo, MOO_SIZEOF(*loop));
	if (!loop) return -1;

	init_oow_pool (moo, &loop->break_ip_pool);
	init_oow_pool (moo, &loop->continue_ip_pool);
	loop->type = type;
	loop->startpos = startpos;
	loop->next = moo->c->mth.loop;
	moo->c->mth.loop = loop;

	return 0;
}

static int update_loop_jumps (moo_t* moo, moo_oow_pool_t* pool, moo_oow_t jt)
{
	/* patch the jump instructions emitted for 'break' or 'continue' */
	moo_oow_pool_chunk_t* chunk;
	moo_oow_t i, j;

	for (chunk = pool->head, i = 0; chunk; chunk = chunk->next)
	{
		for (j = 0; j < MOO_COUNTOF(pool->static_chunk.buf) && i < pool->count; j++)
		{
			if (chunk->buf[j] != INVALID_IP &&
			    patch_long_forward_jump_instruction (moo, chunk->buf[j], jt, BCODE_JUMP2_FORWARD, MOO_NULL) <= -1) return -1;
			i++;
		}
	}

	return 0;
}

static void adjust_loop_jumps_for_elimination (moo_t* moo, moo_oow_pool_t* pool, moo_oow_t start, moo_oow_t end)
{
	/* update the jump instruction positions emitted for 'break' or 'continue' */
	moo_oow_pool_chunk_t* chunk;
	moo_oow_t i, j;

	for (chunk = pool->head, i = 0; chunk; chunk = chunk->next)
	{
		for (j = 0; j < MOO_COUNTOF(pool->static_chunk.buf) && i < pool->count; j++)
		{
			if (chunk->buf[j] != INVALID_IP)
			{
				if (chunk->buf[j] >= start && chunk->buf[j] <= end) 
				{
					/* invalidate the instruction position */
					chunk->buf[j] = INVALID_IP;
				}
				else if (chunk->buf[j] > end && chunk->buf[j] < moo->c->mth.code.len)
				{
					/* decrement the instruction position */
					chunk->buf[j] -= end - start + 1;
				}
			}
			i++;
		}
	}
}

static MOO_INLINE int update_loop_breaks (moo_t* moo, moo_oow_t jt)
{
	return update_loop_jumps (moo, &moo->c->mth.loop->break_ip_pool, jt);
}

static MOO_INLINE int update_loop_continues (moo_t* moo, moo_oow_t jt)
{
	return update_loop_jumps (moo, &moo->c->mth.loop->continue_ip_pool, jt);
}

static MOO_INLINE void adjust_all_loop_jumps_for_elimination (moo_t* moo, moo_oow_t start, moo_oow_t end)
{
	moo_loop_t* loop;

	loop = moo->c->mth.loop;
	while (loop)
	{
		adjust_loop_jumps_for_elimination (moo, &loop->break_ip_pool, start, end);
		adjust_loop_jumps_for_elimination (moo, &loop->continue_ip_pool, start, end);
		loop = loop->next;
	}
}

static MOO_INLINE moo_loop_t* unlink_loop (moo_t* moo)
{
	moo_loop_t* loop;

	MOO_ASSERT (moo, moo->c->mth.loop != MOO_NULL);
	loop = moo->c->mth.loop;
	moo->c->mth.loop = loop->next;

	return loop;
}

static MOO_INLINE void free_loop (moo_t* moo, moo_loop_t* loop)
{
	fini_oow_pool (moo, &loop->continue_ip_pool);
	fini_oow_pool (moo, &loop->break_ip_pool);
	moo_freemem (moo, loop);
}

static MOO_INLINE void pop_loop (moo_t* moo)
{
	free_loop (moo, unlink_loop (moo));
}

static MOO_INLINE int inject_break_to_loop (moo_t* moo)
{
	if (add_to_oow_pool (moo, &moo->c->mth.loop->break_ip_pool, moo->c->mth.code.len) <= -1 ||
	    emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) return -1;
	return 0;
}

static MOO_INLINE int inject_continue_to_loop (moo_t* moo)
{
	if (add_to_oow_pool (moo, &moo->c->mth.loop->continue_ip_pool, moo->c->mth.code.len) <= -1 ||
	    emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) return -1;
	return 0;
}


static void eliminate_instructions (moo_t* moo, moo_oow_t start, moo_oow_t end)
{
	moo_oow_t last;

	MOO_ASSERT (moo, moo->c->mth.code.len >= 1);

	last = moo->c->mth.code.len - 1;

	
	if (end >= last)
	{
		/* eliminate all instructions starting from the start index.
		 * setting the length to the start length will achieve this */
		adjust_all_loop_jumps_for_elimination (moo, start, last);
		moo->c->mth.code.len = start;
	}
	else
	{
		/* eliminate a chunk in the middle of the instruction buffer.
		 * some copying is required */
		adjust_all_loop_jumps_for_elimination (moo, start, end);
		MOO_MEMMOVE (&moo->c->mth.code.ptr[start], &moo->c->mth.code.ptr[end + 1], moo->c->mth.code.len - end - 1);
		moo->c->mth.code.len -= end - start + 1;
	}
}

/* ---------------------------------------------------------------------
 * Compiler
 * --------------------------------------------------------------------- */

static int add_literal (moo_t* moo, moo_oop_t lit, moo_oow_t* index)
{
	moo_oow_t i;

	for (i = 0; i < moo->c->mth.literal_count; i++) 
	{
		/* 
		 * this removes redundancy of symbols, characters, and small integers. 
		 * more complex redundacy check may be done somewhere else like 
		 * in add_string_literal().
		 */
		if (moo->c->mth.literals[i] == lit) 
		{
			*index = i;
			return i;
		}
	}

	if (moo->c->mth.literal_count >= moo->c->mth.literal_capa)
	{
		moo_oop_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (moo->c->mth.literal_count + 1, LITERAL_BUFFER_ALIGN);
		tmp = (moo_oop_t*)moo_reallocmem (moo, moo->c->mth.literals, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->mth.literal_capa = new_capa;
		moo->c->mth.literals = tmp;
	}

	*index = moo->c->mth.literal_count;
	moo->c->mth.literals[moo->c->mth.literal_count++] = lit;
	return 0;
}

static int add_string_literal (moo_t* moo, const moo_oocs_t* str, moo_oow_t* index)
{
	moo_oop_t lit;
	moo_oow_t i;

	for (i = 0; i < moo->c->mth.literal_count; i++) 
	{
		lit = moo->c->mth.literals[i];

		if (MOO_CLASSOF(moo, lit) == moo->_string && 
		    MOO_OBJ_GET_SIZE(lit) == str->len &&
		    moo_equaloochars(((moo_oop_char_t)lit)->slot, str->ptr, str->len)) 
		{
			*index = i;
			return 0;
		}
	}

	lit = moo_instantiate (moo, moo->_string, str->ptr, str->len);
	if (!lit) return -1;

	return add_literal (moo, lit, index);
}

static int add_symbol_literal (moo_t* moo, const moo_oocs_t* str, int offset, moo_oow_t* index)
{
	moo_oop_t tmp;

	tmp = moo_makesymbol (moo, str->ptr + offset, str->len - offset);
	if (!tmp) return -1;

	return add_literal (moo, tmp, index);
}

static MOO_INLINE int set_class_fqn (moo_t* moo, const moo_oocs_t* name)
{
	if (copy_string_to (moo, name, &moo->c->cls.fqn, &moo->c->cls.fqn_capa, 0, '\0') <= -1) return -1;
	moo->c->cls.name = moo->c->cls.fqn;
	return 0;
}

static MOO_INLINE int set_superclass_fqn (moo_t* moo, const moo_oocs_t* name)
{
	if (copy_string_to (moo, name, &moo->c->cls.superfqn, &moo->c->cls.superfqn_capa, 0, '\0') <= -1) return -1;
	moo->c->cls.supername = moo->c->cls.superfqn;
	return 0;
}

static MOO_INLINE int add_class_level_variable (moo_t* moo, var_type_t index, const moo_oocs_t* name)
{
	int n;

	n = copy_string_to (moo, name, &moo->c->cls.vars[index], &moo->c->cls.vars_capa[index], 1, ' ');
	if (n >= 0) 
	{
		moo->c->cls.var_count[index]++;
/* TODO: check if it exceeds MOO_MAX_NAMED_INSTVARS, MOO_MAX_CLASSVARS, MOO_MAX_CLASSINSTVARS */
	}

	return n;
}

static MOO_INLINE int add_pool_dictionary (moo_t* moo, const moo_oocs_t* name, moo_oop_set_t pooldic_oop)
{
	if (moo->c->cls.pooldic_count >= moo->c->cls.pooldic_imp_oops_capa)
	{
		moo_oow_t new_capa;
		moo_oop_set_t* tmp;

		new_capa = MOO_ALIGN(moo->c->cls.pooldic_imp_oops_capa + 1, POOLDIC_OOP_BUFFER_ALIGN);
		tmp = moo_reallocmem (moo, moo->c->cls.pooldic_imp_oops, new_capa * MOO_SIZEOF(moo_oop_set_t));
		if (!tmp) return -1;

		moo->c->cls.pooldic_imp_oops_capa = new_capa;
		moo->c->cls.pooldic_imp_oops = tmp;
	}

	moo->c->cls.pooldic_imp_oops[moo->c->cls.pooldic_count] = pooldic_oop;
	moo->c->cls.pooldic_count++;
/* TODO: check if pooldic_count overflows */

	return 0;
}

static moo_ooi_t find_class_level_variable (moo_t* moo, moo_oop_class_t self, const moo_oocs_t* name, var_info_t* var)
{
	moo_oow_t pos;
	moo_oop_t super;
	moo_oop_char_t v;
	moo_oop_char_t* vv;
	moo_oocs_t hs;
	int index;

	if (self)
	{
		MOO_ASSERT (moo, MOO_CLASSOF(moo, self) == moo->_class);

		/* [NOTE] 
		 *  the loop here assumes that the class has the following
		 *  fields in the order shown below:
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &self->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = MOO_OBJ_GET_SIZE(v);

			if (find_word_in_string(&hs, name, &pos) >= 0)
			{
				super = self->superclass;

				/* 'self' may be MOO_NULL if MOO_NULL has been given for it.
				 * the caller must take good care when interpreting the meaning of 
				 * this field */
				var->cls = self;
				goto done;
			}
		}

		super = self->superclass;
	}
	else
	{
		/* the class definition is not available yet.
		 * find the variable in the compiler's own list */
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			if (find_word_in_string(&moo->c->cls.vars[index], name, &pos) >= 0)
			{
				super = moo->c->cls.super_oop;
				var->cls = self;
				goto done;
			}
		}
		super = moo->c->cls.super_oop;
	}

	while (super != moo->_nil)
	{
		MOO_ASSERT (moo, MOO_CLASSOF(moo, super) == moo->_class);

		/* [NOTE] 
		 *  the loop here assumes that the class has the following
		 *  fields in the order shown below:
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &((moo_oop_class_t)super)->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = MOO_OBJ_GET_SIZE(v);

			if (find_word_in_string(&hs, name, &pos) >= 0)
			{
				/* class variables reside in the class where the definition is found.
				 * that means a class variable is found in the definition of
				 * a superclass, the superclass is the placeholder of the 
				 * class variable. on the other hand, instance variables and
				 * class instance variables live in the current class being 
				 * compiled as they are inherited. */
				var->cls = (index == VAR_CLASS)? (moo_oop_class_t)super: self;
				super = ((moo_oop_class_t)super)->superclass;
				goto done;
			}
		}

		super = ((moo_oop_class_t)super)->superclass;
	}

	moo->errnum = MOO_ENOENT;
	return -1;

done:
	if (super != moo->_nil)
	{
		moo_oow_t spec;

		/* the class being compiled has a superclass */

		MOO_ASSERT (moo, MOO_CLASSOF(moo, super) == moo->_class);
		switch (index)
		{
			case VAR_INSTANCE:
				/* each class has the number of named instance variables 
				 * accumulated for inheritance. the position found in the
				 * local variable string can be adjusted by adding the
				 * number in the superclass */
				spec = MOO_OOP_TO_SMOOI(((moo_oop_class_t)super)->spec);
				pos += MOO_CLASS_SPEC_NAMED_INSTVAR(spec);
				break;

			case VAR_CLASS:
				/* [NOTE]
				 *  no adjustment is needed.
				 *  a class object is composed of three parts.
				 *    fixed-part | classinst-variables | class-variabes 
				 *  the position returned here doesn't consider 
				 *  class instance variables that can be potentially
				 *  placed before the class variables. */
				break;

			case VAR_CLASSINST:
				spec = MOO_OOP_TO_SMOOI(((moo_oop_class_t)super)->selfspec);
				pos += MOO_CLASS_SELFSPEC_CLASSINSTVAR(spec);
				break;
		}
	}

	var->type = index;
	var->pos = pos;
	return pos;
}

static int clone_assignee (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	int n;
	moo_oow_t old_len;

	old_len = moo->c->mth.assignees.len;
	n = copy_string_to (moo, name, &moo->c->mth.assignees, &moo->c->mth.assignees_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = moo->c->mth.assignees.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int clone_binary_selector (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	int n;
	moo_oow_t old_len;

	old_len = moo->c->mth.binsels.len;
	n = copy_string_to (moo, name, &moo->c->mth.binsels, &moo->c->mth.binsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = moo->c->mth.binsels.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int clone_keyword (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	int n;
	moo_oow_t old_len;

	old_len = moo->c->mth.kwsels.len;
	n = copy_string_to (moo, name, &moo->c->mth.kwsels, &moo->c->mth.kwsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = moo->c->mth.kwsels.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int add_method_name_fragment (moo_t* moo, const moo_oocs_t* name)
{
	/* method name fragments are concatenated without any delimiters */
	return copy_string_to (moo, name, &moo->c->mth.name, &moo->c->mth.name_capa, 1, '\0');
}

static int method_exists (moo_t* moo, const moo_oocs_t* name)
{
	/* check if the current class contains a method of the given name */
#ifdef MTHDIC
	return moo_lookupdic (moo, moo->c->cls.mthdic_oop[moo->c->mth.type], name) != MOO_NULL;
#else
	return moo_lookupdic (moo, moo->c->cls.self_oop->mthdic[moo->c->mth.type], name) != MOO_NULL;
#endif
}

static int add_temporary_variable (moo_t* moo, const moo_oocs_t* name)
{
	/* temporary variable names are added to the string with leading
	 * space if it's not the first variable */
	return copy_string_to (moo, name, &moo->c->mth.tmprs, &moo->c->mth.tmprs_capa, 1, ' ');
}

static MOO_INLINE int find_temporary_variable (moo_t* moo, const moo_oocs_t* name, moo_oow_t* xindex)
{
	return find_word_in_string (&moo->c->mth.tmprs, name, xindex);
}

static moo_oop_set_t add_namespace (moo_t* moo, moo_oop_set_t dic, const moo_oocs_t* name)
{
	moo_oow_t tmp_count = 0;
	moo_oop_t sym;
	moo_oop_set_t ns;
	moo_oop_association_t ass;

	moo_pushtmp (moo, (moo_oop_t*)&dic); tmp_count++;

	sym = moo_makesymbol (moo, name->ptr, name->len);
	if (!sym) goto oops;

	moo_pushtmp (moo, &sym); tmp_count++;

	ns = moo_makedic (moo, moo->_namespace, NAMESPACE_SIZE);
	if (!ns) goto oops;

	/*moo_pushtmp (moo, &ns); tmp_count++;*/

	ass = moo_putatdic (moo, dic, sym, (moo_oop_t)ns);
	if (!ass) goto oops;

	moo_poptmps (moo, tmp_count);
	return (moo_oop_set_t)ass->value;

oops:
	moo_poptmps (moo, tmp_count);
	return MOO_NULL;
}

static int preprocess_dotted_name (moo_t* moo, int dont_add_ns, int accept_pooldic_as_ns, const moo_oocs_t* fqn, const moo_ioloc_t* fqn_loc, moo_oocs_t* name, moo_oop_set_t* ns_oop)
{
	const moo_ooch_t* ptr, * dot;
	moo_oow_t len;
	moo_oocs_t seg;
	moo_oop_set_t dic;
	moo_oop_association_t ass;
	int pooldic_gotten = 0;

	dic = moo->sysdic;
	ptr = fqn->ptr;
	len = fqn->len;

	while (1)
	{
		seg.ptr = (moo_ooch_t*)ptr;

		dot = moo_findoochar (ptr, len, '.');
		if (dot)
		{
			if (pooldic_gotten) goto wrong_name;

			seg.len = dot - ptr;

			if (is_reserved_word(&seg)) goto wrong_name;

			ass = moo_lookupdic (moo, dic, &seg);
			if (ass)
			{
				if (MOO_CLASSOF(moo, ass->value) == moo->_namespace || 
				    (seg.ptr == fqn->ptr && ass->value == (moo_oop_t)moo->sysdic))
				{
					/* ok */
					dic = (moo_oop_set_t)ass->value;
				}
				else
				{
					if (accept_pooldic_as_ns && MOO_CLASSOF(moo, ass->value) == moo->_pool_dictionary)
					{
						/* A pool dictionary is treated as if it's a name space.
						 * However, the pool dictionary can only act as a name space
						 * if it's the second last segment. */
						dic = (moo_oop_set_t)ass->value;
						pooldic_gotten = 1;
					}
					else
					{
						goto wrong_name;
					}
				}
			}
			else
			{
				moo_oop_set_t t;

				/* the segment does not exist. add it */
				if (dont_add_ns)
				{
					/* in '#extend Planet.Earth', it's an error
					 * if Planet doesn't exist */
					goto wrong_name;
				}
				
				/* When definining a new class, add a missing namespace */
				t = add_namespace (moo, dic, &seg);
				if (!t) return -1;

				dic = t;
			}
		}
		else
		{
			/* this is the last segment. it should be a class name */
			seg.len = len;

			if (is_reserved_word(&seg)) goto wrong_name;

			*name = seg;
			*ns_oop = dic;

			break;
		}

		ptr = dot + 1;
		len -= seg.len + 1;
	}

	return 0;

wrong_name:
	seg.len += seg.ptr - fqn->ptr;
	seg.ptr = fqn->ptr;
	set_syntax_error (moo, MOO_SYNERR_NAMESPACE, fqn_loc, &seg);
	return -1;
}

static int resolve_pooldic (moo_t* moo, int dotted, const moo_oocs_t* name)
{
	moo_oocs_t last; /* the last segment */
	moo_oop_set_t ns_oop; /* name space */
	moo_oop_association_t ass;
	moo_oow_t i;

	if (dotted)
	{
		if (preprocess_dotted_name(moo, 0, 0, name, TOKEN_LOC(moo), &last, &ns_oop) <= -1) return -1;
	}
	else
	{
		last = *name;
		/* it falls back to the name space of the class */
		ns_oop = moo->c->cls.ns_oop; 
	}

	/* check if the name refers to a pool dictionary */
	ass = moo_lookupdic (moo, ns_oop, &last);
	if (!ass || MOO_CLASSOF(moo, ass->value) != moo->_pool_dictionary)
	{
		set_syntax_error (moo, MOO_SYNERR_POOLDIC, TOKEN_LOC(moo), name);
		return -1;
	}

	/* check if the same dictionary pool has been declared for import */
	for (i = 0; i < moo->c->cls.pooldic_count; i++)
	{
		if ((moo_oop_set_t)ass->value == moo->c->cls.pooldic_imp_oops[i])
		{
			set_syntax_error (moo, MOO_SYNERR_POOLDICDUP, TOKEN_LOC(moo), name);
			return -1;
		}
	}

	return 0;
}

static int import_pool_dictionary (moo_t* moo, moo_oop_set_t ns_oop, const moo_oocs_t* tok_lastseg, const moo_oocs_t* tok_name, const moo_ioloc_t* tok_loc)
{
	moo_oop_association_t ass;
	moo_oow_t i;

	/* check if the name refers to a pool dictionary */
	ass = moo_lookupdic (moo, ns_oop, tok_lastseg);
	if (!ass || MOO_CLASSOF(moo, ass->value) != moo->_pool_dictionary)
	{
		set_syntax_error (moo, MOO_SYNERR_POOLDIC, tok_loc, tok_name);
		return -1;
	}

	/* check if the same dictionary pool has been declared for import */
	for (i = 0; i < moo->c->cls.pooldic_count; i++)
	{
		if ((moo_oop_set_t)ass->value == moo->c->cls.pooldic_imp_oops[i])
		{
			set_syntax_error (moo, MOO_SYNERR_POOLDICDUP, tok_loc, tok_name);
			return -1;
		}
	}

	if (add_pool_dictionary(moo, tok_name, (moo_oop_set_t)ass->value) <= -1) return -1;
	if (copy_string_to (moo, tok_name, &moo->c->cls.pooldic, &moo->c->cls.pooldic_capa, 1, ' ') <= -1)
	{
		moo->c->cls.pooldic_count--; /* roll back add_pool_dictionary() */
		return -1;
	}

	return 0;
}

static int compile_class_level_variables (moo_t* moo)
{
	var_type_t dcl_type = VAR_INSTANCE;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process variable modifiers */
		GET_TOKEN (moo);

		if (is_token_symbol(moo, VOCA_CLASS_S))
		{
			/* dcl(#class) */
			dcl_type = VAR_CLASS;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_CLASSINST_S))
		{
			/* dcl(#classinst) */
			dcl_type = VAR_CLASSINST;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_POOLDIC_S))
		{
			/* dcl(#pooldic) - import a pool dictionary */
			dcl_type = VAR_GLOBAL; /* this is not a real type. use for branching below */
			GET_TOKEN (moo);
		}

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (dcl_type == VAR_GLOBAL) 
	{
		/* pool dictionary import declaration
		 * #dcl(#pooldic) ... */
		moo_oocs_t last;
		moo_oop_set_t ns_oop;

		do
		{
			if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
			{
				if (preprocess_dotted_name(moo, 0, 0, TOKEN_NAME(moo), TOKEN_LOC(moo), &last, &ns_oop) <= -1) return -1;
			}
			else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
			{
				last = moo->c->tok.name;
				/* it falls back to the name space of the class */
				ns_oop = moo->c->cls.ns_oop; 
			}
			else break;

			if (import_pool_dictionary(moo, ns_oop, &last, TOKEN_NAME(moo), TOKEN_LOC(moo)) <= -1) return -1;
			GET_TOKEN (moo);
		}
		while (1);
	}
	else
	{
		/* variable declaration */
		do
		{
			if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
			{
				var_info_t var;

/*
TODO: more check on variability conflict.
if it's a indexed class, check if the superclass is fixed or index.
if super is fixed and self is fixed or variable-pointer, no restriction.
if super is fixed and self is variable-nonpointer, no instance varaible in the super side and in the self side.
if super is variable-pointer, self must be a variable-pointer. can't be fixed either
if super is variable-nonpointer, self must be a variable-nonpointer of the same type. can't be fixed either
if super is variable-nonpointer, no instance variable is allowed.
*/
				if (dcl_type == VAR_INSTANCE && (moo->c->cls.flags & CLASS_INDEXED) && (moo->c->cls.indexed_type != MOO_OBJ_TYPE_OOP))
				{
					set_syntax_error (moo, MOO_SYNERR_VARNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (find_class_level_variable(moo, MOO_NULL, TOKEN_NAME(moo), &var) >= 0 ||
				    moo_lookupdic (moo, moo->sysdic, TOKEN_NAME(moo)) ||  /* conflicts with a top global name */
				    moo_lookupdic (moo, moo->c->cls.ns_oop, TOKEN_NAME(moo))) /* conflicts with a global name in the class'es name space */
				{
					set_syntax_error (moo, MOO_SYNERR_VARNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (add_class_level_variable(moo, dcl_type, TOKEN_NAME(moo)) <= -1) return -1;
			}
			else
			{
				break;
			}

			GET_TOKEN (moo);
		}
		while (1);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		set_syntax_error (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_unary_method_name (moo_t* moo)
{
	MOO_ASSERT (moo, moo->c->mth.name.len == 0);
	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 0);

	if (add_method_name_fragment(moo, TOKEN_NAME(moo)) <= -1) return -1;
	GET_TOKEN (moo);

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* this is a procedural style method */
		MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 0);

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			do
			{
				if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
				{
					/* wrong argument name. identifier is expected */
					set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (find_temporary_variable(moo, TOKEN_NAME(moo), MOO_NULL) >= 0)
				{
					set_syntax_error (moo, MOO_SYNERR_ARGNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
				moo->c->mth.tmpr_nargs++;
				GET_TOKEN (moo); 

				if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN) break;

				if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
				{
					set_syntax_error (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				GET_TOKEN (moo);
			} 
			while (1);
		}

		/* indicate that the unary method name is followed by a parameter list */
		moo->c->mth.variadic = 1;
		GET_TOKEN (moo);
	}

	return 0;
}

static int compile_binary_method_name (moo_t* moo)
{
	MOO_ASSERT (moo, moo->c->mth.name.len == 0);
	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 0);

	if (add_method_name_fragment(moo, TOKEN_NAME(moo)) <= -1) return -1;
	GET_TOKEN (moo);

	/* collect the argument name */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
	{
		/* wrong argument name. identifier expected */
		set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 0);

	/* no duplication check is performed against class-level variable names.
	 * a duplcate name will shade a previsouly defined variable. */
	if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
	moo->c->mth.tmpr_nargs++;

	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 1);
	/* this check should not be not necessary
	if (moo->c->mth.tmpr_nargs > MAX_CODE_NARGS)
	{
		set_syntax_error (moo, MOO_SYNERR_ARGFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}
	*/

	GET_TOKEN (moo);
	return 0;
}

static int compile_keyword_method_name (moo_t* moo)
{
	MOO_ASSERT (moo, moo->c->mth.name.len == 0);
	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs == 0);

	do 
	{
		if (add_method_name_fragment(moo, TOKEN_NAME(moo)) <= -1) return -1;

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
		{
			/* wrong argument name. identifier is expected */
			set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (find_temporary_variable(moo, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			set_syntax_error (moo, MOO_SYNERR_ARGNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
		moo->c->mth.tmpr_nargs++;

		GET_TOKEN (moo);
	} 
	while (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD);

	return 0;
}

static int compile_method_name (moo_t* moo)
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

	MOO_ASSERT (moo, moo->c->mth.tmpr_count == 0);

	moo->c->mth.name_loc = moo->c->tok.loc;
	switch (TOKEN_TYPE(moo))
	{
		case MOO_IOTOK_IDENT:
			n = compile_unary_method_name(moo);
			break;

		case MOO_IOTOK_BINSEL:
			n = compile_binary_method_name(moo);
			break;

		case MOO_IOTOK_KEYWORD:
			n = compile_keyword_method_name(moo);
			break;

		default:
			/* illegal method name  */
			set_syntax_error (moo, MOO_SYNERR_MTHNAME, TOKEN_LOC(moo), TOKEN_NAME(moo));
			n = -1;
	}

	if (n >= 0)
	{
		if (method_exists(moo, &moo->c->mth.name)) 
 		{
			set_syntax_error (moo, MOO_SYNERR_MTHNAMEDUP, &moo->c->mth.name_loc, &moo->c->mth.name);
			return -1;
 		}
	}

	MOO_ASSERT (moo, moo->c->mth.tmpr_nargs < MAX_CODE_NARGS);
	/* the total number of temporaries is equal to the number of 
	 * arguments after having processed the message pattern. it's because
	 * moo treats arguments the same as temporaries */
	moo->c->mth.tmpr_count = moo->c->mth.tmpr_nargs;
	return n;
}

static int compile_method_temporaries (moo_t* moo)
{
	/* 
	 * method-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (moo);
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT) 
	{
		if (find_temporary_variable(moo, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			set_syntax_error (moo, MOO_SYNERR_TMPRNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
		moo->c->mth.tmpr_count++;

		if (moo->c->mth.tmpr_count > MAX_CODE_NARGS)
		{
			set_syntax_error (moo, MOO_SYNERR_TMPRFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		set_syntax_error (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_method_primitive (moo_t* moo)
{
	/* 
	 * method-primitive := "<"  "primitive:" integer ">" |
	 *                     "<"  "primitive:" symbol ">" |
	 *                     "<"  "exception" ">" |
	 *                     "<"  "ensure" ">"
	 */
	moo_ooi_t pfnum;
	const moo_ooch_t* ptr, * end;

	if (!is_token_binary_selector(moo, VOCA_LT)) 
	{
		/* return if < is not seen. it is not an error condition */
		return 0;
	}

	GET_TOKEN (moo);
	if (is_token_keyword(moo, VOCA_PRIMITIVE_COLON))
	{
		GET_TOKEN (moo); 
		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_NUMLIT: /* TODO: allow only an integer */
	/*TODO: more checks the validity of the primitive number. support number with radix and so on support more extensive syntax. support primitive name, not number*/
				ptr = TOKEN_NAME_PTR(moo);
				end = ptr + TOKEN_NAME_LEN(moo);
				pfnum = 0;
				while (ptr < end && is_digitchar(*ptr)) 
				{
					pfnum = pfnum * 10 + (*ptr - '0');
					if (!MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(pfnum))
					{
						set_syntax_error (moo, MOO_SYNERR_PFNUM, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					ptr++;
				}

				moo->c->mth.pfnum = pfnum;
				break;

			case MOO_IOTOK_SYMLIT:
			{
				const moo_ooch_t* tptr;
				moo_oow_t tlen;

				tptr = TOKEN_NAME_PTR(moo) + 1;
				tlen = TOKEN_NAME_LEN(moo) - 1;

				/* attempt get a primitive function number by name */
				pfnum = moo_getpfnum (moo, tptr, tlen);
				if (pfnum <= -1)
				{
					/* a built-in primitive function is not found 
					 * check if it is a primitive function identifier */
					moo_oow_t lit_idx;

					if (moo_findoochar (tptr, tlen, '.') && 
					    add_symbol_literal(moo, TOKEN_NAME(moo), 1, &lit_idx) >= 0 &&
					    MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(lit_idx))
					{
						/* external named primitive containing a period. */
						moo->c->mth.pftype = 2; 
						moo->c->mth.pfnum = lit_idx;
						break;
					}

					/* wrong primitive number */
					set_syntax_error (moo, MOO_SYNERR_PFID, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				else if (!MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(pfnum))
				{
					set_syntax_error (moo, MOO_SYNERR_PFID, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				moo->c->mth.pftype = 1; 
				moo->c->mth.pfnum = pfnum;
				break;
			}

			default:
				set_syntax_error (moo, MOO_SYNERR_INTEGER, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
		}

	}
	else if (is_token_word(moo, VOCA_EXCEPTION))
	{
/* TODO: exception handler is supposed to be used by BlockContext on:do:. 
 *       it needs to check the number of arguments at least */
		moo->c->mth.pftype = 3;
	}
	else if (is_token_word(moo, VOCA_ENSURE))
	{
		moo->c->mth.pftype = 4;
	}
	else
	{
		set_syntax_error (moo, MOO_SYNERR_PRIMITIVE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	if (!is_token_binary_selector(moo, VOCA_GT)) 
	{
		set_syntax_error (moo, MOO_SYNERR_GT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int get_variable_info (moo_t* moo, const moo_oocs_t* name, const moo_ioloc_t* name_loc, int name_dotted, var_info_t* var)
{
	moo_oow_t index;

	MOO_MEMSET (var, 0, MOO_SIZEOF(*var));

	if (name_dotted)
	{
		/* if a name is dotted,
		 * 
		 *   self.XXX - instance variable
		 *   A.B.C    - namespace or pool dictionary related reference.
		 */

		moo_oocs_t last;
		moo_oop_set_t ns_oop;
		moo_oop_association_t ass;
		const moo_ooch_t* dot;

		dot = moo_findoochar (name->ptr, name->len, '.');
		MOO_ASSERT (moo, dot != MOO_NULL);
		if (dot - (const moo_ooch_t*)name->ptr == 4 && 
		    moo_equaloochars(name->ptr, vocas[VOCA_SELF].str, 4))
		{
			/* the dotted name begins with self. */
			dot = moo_findoochar (dot + 1, name->len - 5, '.');
			if (!dot)
			{
				/* the dotted name is composed of 2 segments only */
				last.ptr = name->ptr + 5;
				last.len = name->len - 5;
				if (!is_reserved_word(&last))
				{
					if (find_class_level_variable(moo, moo->c->cls.self_oop, &last, var) >= 0)
					{
						goto class_level_variable;
					}
					else
					{
						/* undeclared identifier */
						set_syntax_error (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
						return -1;
					}
				}
			}
		}

		if (preprocess_dotted_name (moo, 1, 1, name, name_loc, &last, &ns_oop) <= -1) return -1;

		ass = moo_lookupdic (moo, ns_oop, &last);
		if (ass)
		{
			var->type = VAR_GLOBAL;
			var->gbl = ass;
		}
		else
		{
			/* undeclared identifier */
			set_syntax_error (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
			return -1;
		}

		return 0;
	}

	if (find_temporary_variable (moo, name, &index) >= 0)
	{
		var->type = (index < moo->c->mth.tmpr_nargs)? VAR_ARGUMENT: VAR_TEMPORARY;
		var->pos = index;
	}
	else 
	{
		if (find_class_level_variable(moo, moo->c->cls.self_oop, name, var) >= 0)
		{
		class_level_variable:
			switch (var->type)
			{
				case VAR_INSTANCE:
					if (moo->c->mth.type == MOO_METHOD_CLASS)
					{
						/* a class method cannot access an instance variable */
						set_syntax_error (moo, MOO_SYNERR_VARINACC, name_loc, name);
						return -1;
					}
					break;

				case VAR_CLASS:
					/* a class variable can be access by both instance methods and class methods */
					MOO_ASSERT (moo, var->cls != MOO_NULL);
					MOO_ASSERT (moo, MOO_CLASSOF(moo, var->cls) == moo->_class);

					/* increment the position by the number of class instance variables
					 * as the class variables are placed after the class instance variables */
					var->pos += MOO_CLASS_NAMED_INSTVARS + 
					            MOO_CLASS_SELFSPEC_CLASSINSTVAR(MOO_OOP_TO_SMOOI(var->cls->selfspec));
					break;

				case VAR_CLASSINST:
					/* class instance variable can be accessed by only class methods */
					if (moo->c->mth.type == MOO_METHOD_INSTANCE)
					{
						/* an instance method cannot access a class-instance variable */
						set_syntax_error (moo, MOO_SYNERR_VARINACC, name_loc, name);
						return -1;
					}

					/* to a class object itself, a class-instance variable is
					 * just an instance variriable. but these are located
					 * after the named instance variables. */
					var->pos += MOO_CLASS_NAMED_INSTVARS;
					break;

				default:
					/* internal error - it must not happen */
					moo->errnum = MOO_EINTERN;
					return -1;
			}
		}
		else 
		{
			moo_oop_association_t ass;
			/*ass = moo_lookupsysdic (moo, name);*/
			ass = moo_lookupdic (moo, moo->c->cls.ns_oop, name);
			if (!ass && moo->c->cls.ns_oop != moo->sysdic) 
				ass = moo_lookupdic (moo, moo->sysdic, name);

			if (ass)
			{
				var->type = VAR_GLOBAL;
				var->gbl = ass;
			}
			else
			{
				moo_oow_t i;
				moo_oop_association_t ass2 = MOO_NULL;

				/* attempt to find the variable in pool dictionaries */
				for (i = 0; i < moo->c->cls.pooldic_count; i++)
				{
					ass = moo_lookupdic (moo, moo->c->cls.pooldic_imp_oops[i], name);
					if (ass)
					{
						if (ass2)
						{
							/* the variable name has been found at least in 2 dictionaries */
							set_syntax_error (moo, MOO_SYNERR_VARAMBIG, name_loc, name);
							return -1;
						}
						ass2 = ass;
					}
				}

				if (ass2)
				{
					var->type = VAR_GLOBAL;
					var->gbl = ass2;
				}
				else
				{
					/* undeclared identifier */
					set_syntax_error (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
					return -1;
				}
			}
		}
	}

	if (var->pos > MAX_CODE_INDEX)
	{
		/* the assignee is not usable because its index is too large 
		 * to be expressed in byte-codes. */
		set_syntax_error (moo, MOO_SYNERR_VARUNUSE, name_loc, name);
		return -1;
	}

	return 0;
}

static int compile_block_temporaries (moo_t* moo)
{
	/* 
	 * block-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (moo);
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT) 
	{
		if (find_temporary_variable(moo, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			set_syntax_error (moo, MOO_SYNERR_TMPRNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
		moo->c->mth.tmpr_count++;
		if (moo->c->mth.tmpr_count > MAX_CODE_NTMPRS)
		{
			set_syntax_error (moo, MOO_SYNERR_TMPRFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo)); 
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		set_syntax_error (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int store_tmpr_count_for_block (moo_t* moo, moo_oow_t tmpr_count)
{
	if (moo->c->mth.blk_depth >= moo->c->mth.blk_tmprcnt_capa)
	{
		moo_oow_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (moo->c->mth.blk_depth + 1, BLK_TMPRCNT_BUFFER_ALIGN);
		tmp = (moo_oow_t*)moo_reallocmem (moo, moo->c->mth.blk_tmprcnt, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->mth.blk_tmprcnt_capa = new_capa;
		moo->c->mth.blk_tmprcnt = tmp;
	}

	/* [NOTE] i don't increment blk_depth here. it's updated
	 *        by the caller after this function has been called for
	 *        a new block entered. */
	moo->c->mth.blk_tmprcnt[moo->c->mth.blk_depth] = tmpr_count;
	return 0;
}

static int compile_block_expression (moo_t* moo)
{
	moo_oow_t jump_inst_pos;
	moo_oow_t saved_tmpr_count, saved_tmprs_len;
	moo_oow_t block_arg_count, block_tmpr_count;
	moo_ioloc_t block_loc, colon_loc, tmpr_loc;

	/*
	 * block-expression := "[" block-body "]"
	 * block-body := (block-argument* "|")? block-temporaries? method-statement*
	 * block-argument := ":" identifier
	 */

	/* this function expects [ not to be consumed away */
	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_LBRACK);

	if (moo->c->mth.loop) 
	{
		/* this block is placed inside the {} loop */
		moo->c->mth.loop->blkcount++; 
	}
	block_loc = *TOKEN_LOC(moo);
	GET_TOKEN (moo);

	saved_tmprs_len = moo->c->mth.tmprs.len;
	saved_tmpr_count = moo->c->mth.tmpr_count;
	MOO_ASSERT (moo, moo->c->mth.blk_depth > 0);
	MOO_ASSERT (moo, moo->c->mth.blk_tmprcnt[moo->c->mth.blk_depth - 1] == saved_tmpr_count);

	if (TOKEN_TYPE(moo) == MOO_IOTOK_COLON) 
	{
		colon_loc = moo->c->tok.loc;

		/* block temporary variables */
		do 
		{
			GET_TOKEN (moo);

			if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
			{
				/* wrong argument name. identifier expected */
				set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

/* TODO: check conflicting names as well */
			if (find_temporary_variable(moo, TOKEN_NAME(moo), MOO_NULL) >= 0)
			{
				set_syntax_error (moo, MOO_SYNERR_BLKARGNAMEDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (add_temporary_variable(moo, TOKEN_NAME(moo)) <= -1) return -1;
			moo->c->mth.tmpr_count++;
			if (moo->c->mth.tmpr_count > MAX_CODE_NARGS)
			{
				set_syntax_error (moo, MOO_SYNERR_BLKARGFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo)); 
				return -1;
			}

			GET_TOKEN (moo);
		} 
		while (TOKEN_TYPE(moo) == MOO_IOTOK_COLON);

		if (!is_token_binary_selector(moo, VOCA_VBAR))
		{
			set_syntax_error (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	block_arg_count = moo->c->mth.tmpr_count - saved_tmpr_count;
	if (block_arg_count > MAX_CODE_NBLKARGS)
	{
		/* while an integer object is pused to indicate the number of
		 * block arguments, evaluation which is done by message passing
		 * limits the number of arguments that can be passed. so the
		 * check is implemented */
		set_syntax_error (moo, MOO_SYNERR_BLKARGFLOOD, &colon_loc, MOO_NULL); 
		return -1;
	}

	tmpr_loc = moo->c->tok.loc;
	if (compile_block_temporaries(moo) <= -1) return -1;

	/* this is a block-local temporary count including arguments */
	block_tmpr_count = moo->c->mth.tmpr_count - saved_tmpr_count;
	if (block_tmpr_count > MAX_CODE_NBLKTMPRS)
	{
		set_syntax_error (moo, MOO_SYNERR_BLKTMPRFLOOD, &tmpr_loc, MOO_NULL); 
		return -1;
	}

	/* store the accumulated number of temporaries for the current block.
	 * block depth is not raised as it's not entering a new block but
	 * updating the temporaries count for the current block. */
	if (store_tmpr_count_for_block (moo, moo->c->mth.tmpr_count) <= -1) return -1;

#if defined(MOO_USE_MAKE_BLOCK)
	if (emit_double_param_instruction(moo, BCODE_MAKE_BLOCK, block_arg_count, moo->c->mth.tmpr_count/*block_tmpr_count*/) <= -1) return -1;
#else
	if (emit_byte_instruction(moo, BCODE_PUSH_CONTEXT) <= -1 ||
	    emit_push_smooi_literal(moo, block_arg_count) <= -1 ||
	    emit_push_smooi_literal(moo, moo->c->mth.tmpr_count/*block_tmpr_count*/) <= -1 ||
	    emit_byte_instruction(moo, BCODE_SEND_BLOCK_COPY) <= -1) return -1;
#endif

	/* insert dummy instructions before replacing them with a jump instruction */
	jump_inst_pos = moo->c->mth.code.len;
	/* specifying MAX_CODE_JUMP causes emit_single_param_instruction() to 
	 * produce the long jump instruction (BCODE_JUMP_FORWARD_X) */
	if (emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) return -1;

	/* compile statements inside a block */
	if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACK)
	{
		/* the block is empty */
		if (emit_byte_instruction (moo, BCODE_PUSH_NIL) <= -1) return -1;
	}
	else 
	{
		while (TOKEN_TYPE(moo) != MOO_IOTOK_EOF)
		{
			if (compile_block_statement(moo) <= -1) return -1;

			if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACK) break;
			else if (TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD)
			{
				GET_TOKEN (moo);
				if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACK) break;
				if (emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) return -1;
			}
			else
			{
				set_syntax_error (moo, MOO_SYNERR_RBRACK, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
	}

	if (emit_byte_instruction(moo, BCODE_RETURN_FROM_BLOCK) <= -1) return -1;

	if (patch_long_forward_jump_instruction (moo, jump_inst_pos, moo->c->mth.code.len, BCODE_JUMP2_FORWARD, &block_loc) <= -1) return -1;

	/* restore the temporary count */
	moo->c->mth.tmprs.len = saved_tmprs_len;
	moo->c->mth.tmpr_count = saved_tmpr_count;

	if (moo->c->mth.loop) 
	{
		MOO_ASSERT (moo, moo->c->mth.loop->blkcount > 0);
		moo->c->mth.loop->blkcount--;
	}

	GET_TOKEN (moo); /* read the next token after ] */

	return 0;
}

static int add_to_byte_array_literal_buffer (moo_t* moo, moo_oob_t b)
{
	if (moo->c->mth.balit_count >= moo->c->mth.balit_capa)
	{
		moo_oob_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (moo->c->mth.balit_count + 1, BALIT_BUFFER_ALIGN);
		tmp = (moo_oob_t*)moo_reallocmem (moo, moo->c->mth.balit, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->mth.balit_capa = new_capa;
		moo->c->mth.balit = tmp;
	}

/* TODO: overflow check of moo->c->mth.balit_count itself */
	moo->c->mth.balit[moo->c->mth.balit_count++] = b;
	return 0;
}

static int add_to_array_literal_buffer (moo_t* moo, moo_oop_t item)
{
	if (moo->c->mth.arlit_count >= moo->c->mth.arlit_capa)
	{
		moo_oop_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (moo->c->mth.arlit_count + 1, ARLIT_BUFFER_ALIGN);
		tmp = (moo_oop_t*)moo_reallocmem (moo, moo->c->mth.arlit, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->mth.arlit_capa = new_capa;
		moo->c->mth.arlit = tmp;
	}

/* TODO: overflow check of moo->c->mth.arlit_count itself */
	moo->c->mth.arlit[moo->c->mth.arlit_count++] = item;
	return 0;
}

static int __read_byte_array_literal (moo_t* moo, moo_oop_t* xlit)
{
	moo_ooi_t tmp;
	moo_oop_t ba;

	moo->c->mth.balit_count = 0;

	while (TOKEN_TYPE(moo) == MOO_IOTOK_NUMLIT || TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT)
	{
		/* TODO: check if the number is an integer */

		if (string_to_smooi(moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT, &tmp) <= -1)
		{
			/* the token reader reads a valid token. no other errors
			 * than the range error must not occur */
			MOO_ASSERT (moo, moo->errnum == MOO_ERANGE);

			/* if the token is out of the SMOOI range, it's too big or 
			 * to small to be a byte */
			set_syntax_error (moo, MOO_SYNERR_BYTERANGE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		else if (tmp < 0 || tmp > 255)
		{
			set_syntax_error (moo, MOO_SYNERR_BYTERANGE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_to_byte_array_literal_buffer(moo, tmp) <= -1) return -1;
		GET_TOKEN (moo);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACK)
	{
		set_syntax_error (moo, MOO_SYNERR_RBRACK, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	ba = moo_instantiate (moo, moo->_byte_array, moo->c->mth.balit, moo->c->mth.balit_count);
	if (!ba) return -1;

	*xlit = ba;
	return 0;
}

struct arlit_info_t
{
	moo_oow_t pos;
	moo_oow_t len;
};

typedef struct arlit_info_t arlit_info_t;

static int __read_array_literal (moo_t* moo, moo_oop_t* xlit)
{
	moo_oop_t lit, a;
	moo_oow_t i, saved_arlit_count;
	arlit_info_t info;

	info.pos = moo->c->mth.arlit_count;
	info.len = 0;

	do
	{
		switch (TOKEN_TYPE(moo))
		{
/* TODO: floating pointer number */

			case MOO_IOTOK_NUMLIT:
			case MOO_IOTOK_RADNUMLIT:
				lit = string_to_num (moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT);
				break;

			case MOO_IOTOK_CHARLIT:
				MOO_ASSERT (moo, TOKEN_NAME_LEN(moo) == 1);
				lit = MOO_CHAR_TO_OOP(TOKEN_NAME_PTR(moo)[0]);
				break;

			case MOO_IOTOK_STRLIT:
				lit = moo_instantiate (moo, moo->_string, TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo));
				break;

			case MOO_IOTOK_SYMLIT:
				lit = moo_makesymbol (moo, TOKEN_NAME_PTR(moo) + 1, TOKEN_NAME_LEN(moo) - 1);
				break;

/*
			case MOO_IOTOK_IDENT:
			case MOO_IOTOK_IDENT_DOTTED:
			case MOO_IOTOK_BINSEL:
			case MOO_IOTOK_KEYWORD:
			case MOO_IOTOK_SELF:
			case MOO_IOTOK_SUPER:
			case MOO_IOTOK_THIS_CONTEXT:
			case MOO_IOTOK_THIS_PROCESS:
			case MOO_IOTOK_DO:
			case MOO_IOTOK_WHILE:
			case MOO_IOTOK_BREAK:
			case MOO_IOTOK_CONTINUE:
			case MOO_IOTOK_IF:
			case MOO_IOTOK_ELSE:
			case MOO_IOTOK_ELSIF:
				lit = moo_makesymbol (moo, TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo));
				break;
*/
/*		
a := #(1 2 3 self 5)
a := #(1 2 3 #(1 2 3) self)

a := #(1, 2, 3, self, 5) <---- is array constant? is array non constant?
if array constant contains a comma, produce MAKE_ARRAY
if array literal contains no comma, it's just a literal array.
what to do with a single element array? no problem if the element is still a literal.
should i allow expression or something like that here???

#(( abc ))

#(1, 2)
* 

make array 10.
push 10
put array at 1.
push 20
put array at 2
if index is too large, switch to at:put? (or don't care as it's too large???).
*/

			case MOO_IOTOK_NIL:
				lit = moo->_nil;
				break;

			case MOO_IOTOK_TRUE:
				lit = moo->_true;
				break;

			case MOO_IOTOK_FALSE:
				lit = moo->_false;
				break;

			case MOO_IOTOK_ERROR: /* error */
				lit = MOO_ERROR_TO_OOP(MOO_EGENERIC);
				break;

			case MOO_IOTOK_ERRLIT: /* error(X) */
				lit = string_to_error (moo, TOKEN_NAME(moo));
				break;

			case MOO_IOTOK_APAREN: /* #( */
			/*case MOO_IOTOK_LPAREN:*/ /* ( */
				saved_arlit_count = moo->c->mth.arlit_count;
/* TODO: get rid of recursion?? */
				GET_TOKEN (moo);
				if (__read_array_literal (moo, &lit) <= -1) return -1;
				moo->c->mth.arlit_count = saved_arlit_count;
				break;

			case MOO_IOTOK_BABRACK: /* #[ */
			/*case MOO_IOTOK_LBRACK:*/ /* [ */
				GET_TOKEN (moo);
				if (__read_byte_array_literal (moo, &lit) <= -1) return -1;
				break;

			default:
				goto done;
		}

		if (!lit || add_to_array_literal_buffer(moo, lit) <= -1) return -1;
		info.len++;

		GET_TOKEN (moo);
	}
	while (1);

done:
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	a = moo_instantiate (moo, moo->_array, MOO_NULL, info.len);
	if (!a) return -1;

	for (i = 0; i < info.len; i++)
	{
		((moo_oop_oop_t)a)->slot[i] = moo->c->mth.arlit[info.pos + i];
	}

	*xlit = a;
	return 0;
}

static MOO_INLINE int read_byte_array_literal (moo_t* moo, moo_oop_t* xlit)
{
	GET_TOKEN (moo); /* skip #[ and read the next token */
	return __read_byte_array_literal(moo, xlit);
}

static int compile_byte_array_literal (moo_t* moo)
{
	moo_oop_t lit;
	moo_oow_t index;

	if (read_byte_array_literal(moo, &lit) <= -1 ||
	    add_literal(moo, lit, &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (moo);
	return 0;
}

static int read_array_literal (moo_t* moo, moo_oop_t* xlit)
{
	int x;
	moo_oow_t saved_arlit_count;

	moo->c->in_array = 1;
	if (get_token(moo) <= -1)
	{
		/* skip #( and read the next token */
		moo->c->in_array = 0;
		return -1;
	}
	saved_arlit_count = moo->c->mth.arlit_count;
	x = __read_array_literal (moo, xlit);
	moo->c->mth.arlit_count = saved_arlit_count;
	moo->c->in_array = 0;

	return x;
}

static int compile_array_literal (moo_t* moo)
{
	moo_oop_t lit;
	moo_oow_t index;

	MOO_ASSERT (moo, moo->c->mth.arlit_count == 0);

	if (read_array_literal(moo, &lit) <= -1 ||
	    add_literal(moo, lit, &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (moo);
	return 0;
}

static int compile_array_expression (moo_t* moo)
{
	moo_oow_t maip;
	moo_ioloc_t aeloc;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_ABRACE);

	maip = moo->c->mth.code.len;
	if (emit_single_param_instruction(moo, BCODE_MAKE_ARRAY, 0) <= -1) return -1;

	aeloc = *TOKEN_LOC(moo);
	GET_TOKEN (moo); /* read a token after #{ */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		moo_oow_t index;

		index = 0;
		do
		{
			if (compile_method_expression (moo, 0) <= -1) return -1;
			if (emit_single_param_instruction (moo, BCODE_POP_INTO_ARRAY, index) <= -1) return -1;
			index++;

			if (index > MAX_CODE_PARAM)
			{
				set_syntax_error (moo, MOO_SYNERR_ARREXPFLOOD, &aeloc, MOO_NULL);
				return -1;
			}

			if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;

			if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
			{
				set_syntax_error (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			GET_TOKEN (moo);
		} 
		while (1);

	/* TODO: devise a double_param MAKE_ARRAY to increase the number of elementes supported... */
		/* patch the MAKE_ARRAY instruction */
	#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
		moo->c->mth.code.ptr[maip + 1] = index >> 8;
		moo->c->mth.code.ptr[maip + 2] = index & 0xFF;
	#else
		moo->c->mth.code.ptr[maip + 1] = index;
	#endif
	}

	GET_TOKEN (moo); /* read a token after } */
	return 0;
}

static int compile_association_expression (moo_t* moo)
{
	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_ASSPAREN);

	GET_TOKEN (moo); /* read a token after #{ */
	if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
	{
		/* key is required */
		set_syntax_error (moo, MOO_SYNERR_NOASSKEY, TOKEN_LOC(moo), MOO_NULL);
		return -1;
	}

	if (emit_byte_instruction (moo, BCODE_MAKE_ASSOCIATION) <= -1) return -1;

	if (compile_method_expression (moo, 0) <= -1) return -1;
	if (emit_byte_instruction (moo, BCODE_POP_INTO_ASSOCIATION_KEY) <= -1) return -1;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
	{
		/* no comma, no value is specified.  */
		goto done;
	}
	else if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
	{
		set_syntax_error (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo); /* read a token after the comma */
	if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
	{
		set_syntax_error (moo, MOO_SYNERR_NOASSVALUE, TOKEN_LOC(moo), MOO_NULL);
		return -1;
	}

	if (compile_method_expression (moo, 0) <= -1) return -1;
	if (emit_byte_instruction (moo, BCODE_POP_INTO_ASSOCIATION_VALUE) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

done:
	GET_TOKEN (moo);
	return 0;
}

static int compile_dictionary_expression (moo_t* moo)
{
	moo_oow_t mdip;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_DICBRACE);

	GET_TOKEN (moo); /* read a token after :{ */

	mdip = moo->c->mth.code.len;
	if (emit_single_param_instruction (moo, BCODE_MAKE_DICTIONARY, 0) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_oow_t count;

		count = 0;
		do
		{
			if (TOKEN_TYPE(moo) == MOO_IOTOK_ASSPAREN)
			{
				moo_oow_t si;
				static moo_ooch_t msg[] = { '_', '_','p','u','t','_','a','s','s','o','c',':' }; /* don't put '\0' at the back */
				moo_oocs_t x;

				x.ptr = msg;
				x.len = MOO_COUNTOF(msg);
				/* [ATTENTION] 
				 *  if the method returns self, i don't need DUP_STACKTOP and POP_STACKTOP.
				 *  if the method retruns something else, DUP_STACKTOP and POP_STACKTOP is needed
				 *  to emulate message cascading.
				if (emit_byte_instruction (moo, BCODE_DUP_STACKTOP) <= -1 ||
				    compile_association_expression(moo) <= -1 ||
				    add_symbol_literal(moo, &x, 0, &si) <= -1 ||
				    emit_double_param_instruction (moo, BCODE_SEND_MESSAGE_0, 1, si) <= -1 ||
				    emit_byte_instruction (moo, BCODE_POP_STACKTOP) <= -1) return -1; */
				if (compile_association_expression(moo) <= -1 ||
				    add_symbol_literal(moo, &x, 0, &si) <= -1 ||
				    emit_double_param_instruction (moo, BCODE_SEND_MESSAGE_0, 1, si) <= -1) return -1;
				count++;
			}
			else
			{
				set_syntax_error (moo, MOO_SYNERR_NOASSOC, TOKEN_LOC(moo), MOO_NULL);
				return -1;
			}

			if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;
			if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA)
			{
				GET_TOKEN(moo);
			}
		}
		while (1);

		/* count is just a hint unlike in array */
		if (count > MAX_CODE_PARAM) count = MAX_CODE_PARAM;

		/* patch the MAKE_DICTIONARY instruction */
	#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
		moo->c->mth.code.ptr[mdip + 1] = count >> 8;
		moo->c->mth.code.ptr[mdip + 2] = count & 0xFF;
	#else
		moo->c->mth.code.ptr[mdip + 1] = count;
	#endif
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_expression_primary (moo_t* moo, const moo_oocs_t* ident, const moo_ioloc_t* ident_loc, int ident_dotted, int* to_super)
{
	/*
	 * expression-primary := identifier | literal | block-constructor | ( "(" method-expression ")" )
	 */

	var_info_t var;
	int read_next_token = 0;
	moo_oow_t index;

	*to_super = 0;

	if (ident) 
	{
		/* the caller has read the identifier and the next word */
	handle_ident:
		if (get_variable_info(moo, ident, ident_loc, ident_dotted, &var) <= -1) return -1;

		switch (var.type)
		{
			case VAR_ARGUMENT:
			case VAR_TEMPORARY:
			{
			#if defined(MOO_USE_CTXTEMPVAR)
				if (moo->c->mth.blk_depth > 0)
				{
					moo_oow_t i;

					/* if a temporary variable is accessed inside a block,
					 * use a special instruction to indicate it */
					MOO_ASSERT (moo, var.pos < moo->c->mth.blk_tmprcnt[moo->c->mth.blk_depth]);
					for (i = moo->c->mth.blk_depth; i > 0; i--)
					{
						if (var.pos >= moo->c->mth.blk_tmprcnt[i - 1])
						{
							if (emit_double_param_instruction(moo, BCODE_PUSH_CTXTEMPVAR_0, moo->c->mth.blk_depth - i, var.pos - moo->c->mth.blk_tmprcnt[i - 1]) <= -1) return -1;
							goto temporary_done;
						}
					}
				}
			#endif

				if (emit_single_param_instruction(moo, BCODE_PUSH_TEMPVAR_0, var.pos) <= -1) return -1;
			temporary_done:
				break;
			}

			case VAR_INSTANCE:
			case VAR_CLASSINST:
				if (emit_single_param_instruction(moo, BCODE_PUSH_INSTVAR_0, var.pos) <= -1) return -1;
				break;

			case VAR_CLASS:
				if (add_literal(moo, (moo_oop_t)var.cls, &index) <= -1 ||
				    emit_double_param_instruction(moo, BCODE_PUSH_OBJVAR_0, var.pos, index) <= -1) return -1;
				break;

			case VAR_GLOBAL:
				/* [NOTE]
				 * the association object pointed to by a system dictionary
				 * is stored into the literal frame. so the system dictionary
				 * must not migrate the value of the association to a new
				 * association when it rehashes the entire dictionary. 
				 * If the association entry is deleted from the dictionary,
				 * the code compiled before the deletion will still access
				 * the deleted association
				 */
				if (add_literal(moo, (moo_oop_t)var.gbl, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_OBJECT_0, index) <= -1) return -1;
				break;

			default:
				moo->errnum = MOO_EINTERN;
				return -1;
		}

		if (read_next_token) GET_TOKEN (moo);
	}
	else
	{
		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_IDENT_DOTTED:
				ident_dotted = 1;
			case MOO_IOTOK_IDENT:
				ident = TOKEN_NAME(moo);
				ident_loc = TOKEN_LOC(moo);
				read_next_token = 1;
				goto handle_ident;

			case MOO_IOTOK_SELF:
				if (emit_byte_instruction(moo, BCODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_SUPER:
				if (emit_byte_instruction(moo, BCODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (moo);
				*to_super = 1;
				break;

			case MOO_IOTOK_NIL:
				if (emit_byte_instruction(moo, BCODE_PUSH_NIL) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_TRUE:
				if (emit_byte_instruction(moo, BCODE_PUSH_TRUE) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_FALSE:
				if (emit_byte_instruction(moo, BCODE_PUSH_FALSE) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_ERROR:
				if (add_literal(moo, MOO_ERROR_TO_OOP(MOO_EGENERIC), &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_ERRLIT:
			{
				moo_oop_t tmp;

				tmp = string_to_error (moo, TOKEN_NAME(moo));
				if (!tmp) return -1;

				if (add_literal(moo, tmp, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

				GET_TOKEN (moo);
				break;
			}

			case MOO_IOTOK_THIS_CONTEXT:
				if (emit_byte_instruction(moo, BCODE_PUSH_CONTEXT) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_THIS_PROCESS:
				if (emit_byte_instruction(moo, BCODE_PUSH_PROCESS) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_CHARLIT:
				MOO_ASSERT (moo, TOKEN_NAME_LEN(moo) == 1);
				if (emit_push_character_literal(moo, TOKEN_NAME_PTR(moo)[0]) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_STRLIT:
				if (add_string_literal(moo, TOKEN_NAME(moo), &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_SYMLIT:
				if (add_symbol_literal(moo, TOKEN_NAME(moo), 1, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_NUMLIT:
			case MOO_IOTOK_RADNUMLIT:
			{
/* TODO: floating pointer number */
				/* TODO: other types of numbers, etc */
				moo_oop_t tmp;

				tmp = string_to_num (moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT);
				if (!tmp) return -1;

				if (MOO_OOP_IS_SMOOI(tmp))
				{
					if (emit_push_smooi_literal(moo, MOO_OOP_TO_SMOOI(tmp)) <= -1) return -1;
				}
				else
				{
					if (add_literal(moo, tmp, &index) <= -1 ||
					    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				}

				GET_TOKEN (moo);
				break;
			}

			case MOO_IOTOK_BABRACK: /* #[ */
				/*GET_TOKEN (moo);*/
				if (compile_byte_array_literal(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_APAREN: /* #( */
				/*GET_TOKEN (moo);*/
				if (compile_array_literal(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_ABRACE: /* #{ */
				if (compile_array_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_ASSPAREN: /* :( */
				if (compile_association_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_DICBRACE: /* :{ */
				if (compile_dictionary_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_LBRACK: /* [ */
			{
				int n;

				/*GET_TOKEN (moo);*/
				if (store_tmpr_count_for_block (moo, moo->c->mth.tmpr_count) <= -1) return -1;
				moo->c->mth.blk_depth++;
				/*
				 * moo->c->mth.tmpr_count[0] contains the number of temporaries for a method.
				 * moo->c->mth.tmpr_count[1] contains the number of temporaries for the block plus the containing method.
				 * ...
				 * moo->c->mth.tmpr_count[n] contains the number of temporaries for the block plus all containing method and blocks.
				 */
				n = compile_block_expression(moo);
				moo->c->mth.blk_depth--;
				if (n <= -1) return -1;
				break;
			}

			case MOO_IOTOK_LPAREN:
				GET_TOKEN (moo);
				if (compile_method_expression(moo, 0) <= -1) return -1;
				if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
				{
					set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				GET_TOKEN (moo);
				break;

			default:
				set_syntax_error (moo, MOO_SYNERR_PRIMARY, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
		}
	}

	return 0;
}

static moo_oob_t send_message_cmd[] = 
{
	BCODE_SEND_MESSAGE_0,
	BCODE_SEND_MESSAGE_TO_SUPER_0
};

static int compile_unary_message (moo_t* moo, int to_super)
{
	moo_oow_t index;
	moo_oow_t nargs;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_IDENT);

	do
	{
		nargs = 0;
		if (add_symbol_literal(moo, TOKEN_NAME(moo), 0, &index) <= -1) return -1;

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
		{
			/* parameterized procedure call */
			GET_TOKEN(moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
			{
				do
				{
					if (compile_method_expression (moo, 0) <= -1) return -1;
					nargs++;

					if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN) break;

					if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
					{
						set_syntax_error (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					GET_TOKEN(moo);
				}
				while (1);
			}

			GET_TOKEN(moo);

			/* NOTE: since the actual method may not be known at the compile time,
			 *       i can't check if nargs will match the number of arguments
			 *       expected by the method */
		}

		if (emit_double_param_instruction(moo, send_message_cmd[to_super], nargs, index) <= -1) return -1;
	}
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT);

	return 0;
}

static int compile_binary_message (moo_t* moo, int to_super)
{
	/*
	 * binary-message := binary-selector binary-argument
	 * binary-argument := expression-primary unary-message*
	 */
	moo_oow_t index;
	int to_super2;
	moo_oocs_t binsel;
	moo_oow_t saved_binsels_len, binsel_offset;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL);

	do
	{
		binsel = moo->c->tok.name;
		saved_binsels_len = moo->c->mth.binsels.len;

		if (clone_binary_selector(moo, &binsel, &binsel_offset) <= -1) goto oops;

		GET_TOKEN (moo);

		if (compile_expression_primary(moo, MOO_NULL, MOO_NULL, 0, &to_super2) <= -1) goto oops;

		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && compile_unary_message(moo, to_super2) <= -1) goto oops;

		/* update the pointer to the cloned selector now 
		 * to be free from reallocation risk for the recursive call
		 * to compile_expression_primary(). */
		binsel.ptr = &moo->c->mth.binsels.ptr[binsel_offset];
		if (add_symbol_literal(moo, &binsel, 0, &index) <= -1 ||
		    emit_double_param_instruction(moo, send_message_cmd[to_super], 1, index) <= -1) goto oops;

		moo->c->mth.binsels.len = saved_binsels_len;
	}
	while (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL);

	return 0;

oops:
	moo->c->mth.binsels.len = saved_binsels_len;
	return -1;
}

static int compile_keyword_message (moo_t* moo, int to_super)
{
	/*
	 * keyword-message := (keyword keyword-argument)+
	 * keyword-argument := expression-primary unary-message* binary-message*
	 */

	moo_oow_t index;
	int to_super2;
	moo_oocs_t kw, kwsel;
	moo_ioloc_t saved_kwsel_loc;
	moo_oow_t saved_kwsel_len;
	moo_oow_t kw_offset;
	moo_oow_t nargs = 0;

	saved_kwsel_loc = moo->c->tok.loc;
	saved_kwsel_len = moo->c->mth.kwsels.len;

/* TODO: optimization for ifTrue: ifFalse: whileTrue: whileFalse .. */
	do 
	{
		kw = moo->c->tok.name;
		if (clone_keyword(moo, &kw, &kw_offset) <= -1) goto oops;

		GET_TOKEN (moo);

		if (compile_expression_primary(moo, MOO_NULL, MOO_NULL, 0, &to_super2) <= -1) goto oops;
		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && compile_unary_message(moo, to_super2) <= -1) goto oops;
		if (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL && compile_binary_message(moo, to_super2) <= -1) goto oops;

		kw.ptr = &moo->c->mth.kwsels.ptr[kw_offset];
		if (nargs >= MAX_CODE_NARGS)
		{
			/* 'kw' points to only one segment of the full keyword message. 
			 * if it parses an expression like 'aBlock value: 10 with: 20',
			 * 'kw' may point to 'value:' or 'with:'.
			 */
			set_syntax_error (moo, MOO_SYNERR_ARGFLOOD, &saved_kwsel_loc, &kw); 
			goto oops;
		}

		nargs++;
	} 
	while (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD);

	kwsel.ptr = &moo->c->mth.kwsels.ptr[saved_kwsel_len];
	kwsel.len = moo->c->mth.kwsels.len - saved_kwsel_len;

	if (add_symbol_literal(moo, &kwsel, 0, &index) <= -1 ||
	    emit_double_param_instruction(moo, send_message_cmd[to_super], nargs, index) <= -1) goto oops;

	moo->c->mth.kwsels.len = saved_kwsel_len;
	return 0;

oops:
	moo->c->mth.kwsels.len = saved_kwsel_len;
	return -1;
}

static int compile_message_expression (moo_t* moo, int to_super)
{
	/*
	 * message-expression := single-message cascaded-message
	 * single-message := 
	 *     (unary-message+ binary-message* keyword-message?) |
	 *     (binary-message+ keyword-message?) |
	 *     keyword-message
	 * 
	 * keyword-message := (keyword keyword-argument)+
	 * keyword-argument := expression-primary unary-message* binary-message*
	 * binary-message := binary-selector binary-argument
	 * binary-argument := expression-primary unary-message*
	 * unary-message := unary-selector
	 * cascaded-message := (";" single-message)*
	 */
	moo_oow_t noop_pos;

	do
	{
		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_IDENT:
				/* insert NOOP to change to DUP_STACKTOP if there is a 
				 * cascaded message */
				noop_pos = moo->c->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_unary_message(moo, to_super) <= -1) return -1;

				if (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL)
				{
					MOO_ASSERT (moo, moo->c->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&moo->c->mth.code.ptr[noop_pos], &moo->c->mth.code.ptr[noop_pos + 1], moo->c->mth.code.len - noop_pos - 1);
					moo->c->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = moo->c->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;
					if (compile_binary_message(moo, to_super) <= -1) return -1;
				}

				if (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
				{
					MOO_ASSERT (moo, moo->c->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&moo->c->mth.code.ptr[noop_pos], &moo->c->mth.code.ptr[noop_pos + 1], moo->c->mth.code.len - noop_pos - 1);
					moo->c->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = moo->c->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;
					if (compile_keyword_message(moo, to_super) <= -1) return -1;
				}
				break;

			case MOO_IOTOK_BINSEL:
				noop_pos = moo->c->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_binary_message(moo, to_super) <= -1) return -1;
				if (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
				{
					MOO_ASSERT (moo, moo->c->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&moo->c->mth.code.ptr[noop_pos], &moo->c->mth.code.ptr[noop_pos + 1], moo->c->mth.code.len - noop_pos - 1);
					moo->c->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = moo->c->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;
					if (compile_keyword_message(moo, to_super) <= -1) return -1;
				}
				break;

			case MOO_IOTOK_KEYWORD:
				noop_pos = moo->c->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_keyword_message(moo, to_super) <= -1) return -1;
				break;

			default:
				goto done;

		}

		if (TOKEN_TYPE(moo) == MOO_IOTOK_SEMICOLON)
		{
			moo->c->mth.code.ptr[noop_pos] = BCODE_DUP_STACKTOP;
			if (emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) return -1;
			GET_TOKEN(moo);
		}
		else 
		{
			MOO_ASSERT (moo, moo->c->mth.code.len > noop_pos);
			/*MOO_MEMMOVE (&moo->c->mth.code.ptr[noop_pos], &moo->c->mth.code.ptr[noop_pos + 1], moo->c->mth.code.len - noop_pos - 1);
			moo->c->mth.code.len--;*/
			/* eliminate the NOOP instruction */
			eliminate_instructions (moo, noop_pos, noop_pos);
			goto done;
		}
	}
	while (1);

done:
	return 0;
}

static int compile_basic_expression (moo_t* moo, const moo_oocs_t* ident, const moo_ioloc_t* ident_loc, int ident_dotted)
{
	/*
	 * basic-expression := expression-primary message-expression?
	 */
	int to_super;

	if (compile_expression_primary(moo, ident, ident_loc, ident_dotted, &to_super) <= -1) return -1;

#if 0
	if (TOKEN_TYPE(moo) != MOO_IOTOK_EOF && 
	    TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE && 
	    TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD &&
	    TOKEN_TYPE(moo) != MOO_IOTOK_SEMICOLON)
	{
		if (compile_message_expression(moo, to_super) <= -1) return -1;
	}
#else
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT ||
	    TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL ||
	    TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
	{
		if (compile_message_expression(moo, to_super) <= -1) return -1;
	}
#endif

	return 0;
}

static int compile_braced_block (moo_t* moo)
{
	/* handle a code block enclosed in { } */

/*TODO: support local variable declaration inside {} */
	moo_oow_t code_start;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	code_start = moo->c->mth.code.len;
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		while (TOKEN_TYPE(moo) != MOO_IOTOK_EOF)
		{
			if (compile_block_statement(moo) <= -1) return -1;

			if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;
			else if (TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD)
			{
				GET_TOKEN (moo);
				if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;
				if (emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) return -1;
			}
			else
			{
				set_syntax_error (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
	}

	if (moo->c->mth.code.len == code_start)
	{
		/* the block doesn't contain an instruction at all */
		if (emit_byte_instruction(moo, BCODE_PUSH_NIL) <= -1) return -1;
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	return 0;
}

static int compile_conditional (moo_t* moo)
{
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LPAREN)
	{
		set_syntax_error (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	/* a weird expression like this is also allowed for the call to  compile_method_expression() 
	 *   if (if (a == 10) { ^20 }) { ^40 }. */
	if (compile_method_expression(moo, 0) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		set_syntax_error (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	return 0;
}

static int compile_if_expression (moo_t* moo)
{
	moo_oow_pool_t jumptoend;
	moo_oow_pool_chunk_t* jumptoend_chunk;
	moo_oow_t i, j;
	moo_oow_t jumptonext, precondpos, postcondpos, endoftrueblock;
	moo_ioloc_t if_loc, brace_loc;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_IF);
	if_loc = *TOKEN_LOC(moo);

	init_oow_pool (moo, &jumptoend);
	jumptonext = INVALID_IP;
	endoftrueblock = INVALID_IP;

	do
	{
		int falseblock = 0;

		GET_TOKEN (moo); /* get ( */
		precondpos = moo->c->mth.code.len;

		if (jumptonext != INVALID_IP &&
		    patch_long_forward_jump_instruction (moo, jumptonext, precondpos, BCODE_JUMP2_FORWARD_IF_FALSE, &brace_loc) <= -1) goto oops;

		if (compile_conditional(moo) <= -1) goto oops;
		postcondpos = moo->c->mth.code.len;

		if (precondpos + 1 == postcondpos && moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_TRUE)
		{
			/* do not generate jump */
			jumptonext = INVALID_IP;
			falseblock = 0;

			/* eliminate PUSH_TRUE as well */
			eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
			postcondpos = precondpos;
		}
		else if (precondpos + 1 == postcondpos && moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_FALSE)
		{
			jumptonext = INVALID_IP;
			/* mark that the conditional is false. instructions will get eliminated below */
			falseblock = 1; 
		}
		else
		{
			/* remember position of the jump_forward_if_false instruction to be generated */
			jumptonext = moo->c->mth.code.len; 
			/* specifying MAX_CODE_JUMP causes emit_single_param_instruction() to 
			 * produce the long jump instruction (BCODE_JUMP_FORWARD_X) */
			if (emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_IF_FALSE_0, MAX_CODE_JUMP) <= -1) goto oops;
		}

		GET_TOKEN (moo); /* get { */
		brace_loc = *TOKEN_LOC(moo);
		if (compile_braced_block(moo) <= -1) goto oops;

		if (jumptonext == INVALID_IP)
		{
			if (falseblock) 
			{
				/* the conditional was false. elimiate instructions emitted
				 * for the block attached to the conditional */
				eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
				postcondpos = precondpos;
			}
			else if (endoftrueblock == INVALID_IP) 
			{
				/* update the end position of the first true block */
				endoftrueblock = moo->c->mth.code.len;
			}
		}
		else
		{
			if (endoftrueblock == INVALID_IP)
			{
				/* emit an instruction to jump to the end */
				if (add_to_oow_pool(moo, &jumptoend, moo->c->mth.code.len) <= -1 ||
				    emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) goto oops;
			}
		}

		GET_TOKEN (moo); /* get the next token after } */
	} while (TOKEN_TYPE(moo) == MOO_IOTOK_ELSIF);

	if (jumptonext != INVALID_IP &&
	    patch_long_forward_jump_instruction (moo, jumptonext, moo->c->mth.code.len, BCODE_JUMP2_FORWARD_IF_FALSE, &brace_loc) <= -1) goto oops;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_ELSE)
	{
		GET_TOKEN (moo); /* get { */
		if (compile_braced_block (moo) <= -1) goto oops;
		GET_TOKEN (moo); /* get the next token after } */
	}
	else
	{
		/* emit an instruction to push nil if no 'else' part exists */
		if (emit_byte_instruction (moo, BCODE_PUSH_NIL) <= -1) goto oops;
	}

	if (endoftrueblock != INVALID_IP)
	{
		/* eliminate all instructions after the end of the first true block found */
		eliminate_instructions (moo, endoftrueblock, moo->c->mth.code.len - 1);
	}

	/* patch instructions that jumps to the end of if expression */
	for (jumptoend_chunk = jumptoend.head, i = 0; jumptoend_chunk; jumptoend_chunk = jumptoend_chunk->next)
	{
		/* pass if_loc to every call to patch_long_forward_jump_instruction().
		 * it's harmless because if the first call doesn't flood, the subseqent 
		 * call will never flood either. */
		for (j = 0; j < MOO_COUNTOF(jumptoend.static_chunk.buf) && i < jumptoend.count; j++)
		{
			if (patch_long_forward_jump_instruction (moo, jumptoend_chunk->buf[j], moo->c->mth.code.len, BCODE_JUMP2_FORWARD_IF_FALSE, &if_loc) <= -1) goto oops;
			i++;
		}
	}

	fini_oow_pool (moo, &jumptoend);
	return 0;

oops:
	fini_oow_pool (moo, &jumptoend);
	return -1;
}

static int compile_while_expression (moo_t* moo)
{
	moo_ioloc_t while_loc, brace_loc;
	moo_oow_t precondpos, postcondpos, prebbpos, postbbpos;
	int cond_style = 0, loop_pushed = 0;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_WHILE);
	while_loc = *TOKEN_LOC(moo);

	GET_TOKEN (moo); /* get (, verification is done inside compile_conditional() */
	precondpos = moo->c->mth.code.len;
	if (compile_conditional (moo) <= -1) goto oops;

	postcondpos = moo->c->mth.code.len;
#if 0
	if (precondpos + 1 == postcondpos)
	{
		/* simple optimization - 
		 *   if the conditional is known to be true, emit the absolute jump instruction.
		 *   if it is known to be false, kill all generated instructions. */
		if (moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_TRUE)
		{
			/* the conditional is always true */
			cond_style = 1;
			eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
			postcondpos = precondpos;
		}
		else if (moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_FALSE)
		{
			/* the conditional is always false */
			cond_style = -1;
		}
	}
#endif

	if (cond_style != 1)
	{
		/* specifying MAX_CODE_JUMP causes emit_single_param_instruction() to 
		 * produce the long jump instruction (BCODE_JUMP_FORWARD_X) */
		if (emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_IF_FALSE_0, MAX_CODE_JUMP) <= -1) goto oops;
	}

	/* remember information about this while loop. */
	if (push_loop (moo, MOO_LOOP_WHILE, precondpos) <= -1) goto oops;
	loop_pushed = 1;

	GET_TOKEN (moo); /* get { */
	brace_loc = *TOKEN_LOC(moo);
	prebbpos = moo->c->mth.code.len;
	if (compile_braced_block (moo) <= -1) goto oops;
	GET_TOKEN (moo); /* get the next token after } */
	postbbpos = moo->c->mth.code.len;

	if (prebbpos + 1 == postbbpos && moo->c->mth.code.ptr[prebbpos] == BCODE_PUSH_NIL)
	{
		/* optimization -
		 *  the braced block is kind of empty as it only pushes nil.
		 *  get rid of this push instruction and don't generate the POP_STACKTOP */
		eliminate_instructions (moo, prebbpos, moo->c->mth.code.len - 1);
	}
	else if (prebbpos < postbbpos)
	{
		/* emit code to pop the value pushed by the braced block */
		if (emit_byte_instruction (moo, BCODE_POP_STACKTOP) <= -1) goto oops;
	}

	/* emit code to jump back to the condition */
	if (emit_backward_jump_instruction (moo, BCODE_JUMP_BACKWARD_0, moo->c->mth.code.len - precondpos) <= -1) 
	{
		if (moo->errnum == MOO_ERANGE) 
		{
			/* the jump offset is out of the representable range by the offset
			 * portion of the jump instruction */
			set_syntax_error (moo, MOO_SYNERR_BLKFLOOD, &while_loc, MOO_NULL);
		}
		goto oops;
	}

	if (cond_style != 1)
	{
		/* patch the jump instruction */
		if (patch_long_forward_jump_instruction (moo, postcondpos, moo->c->mth.code.len, BCODE_JUMP2_FORWARD_IF_FALSE, &brace_loc) <= -1) goto oops;
	}

	if (cond_style == -1) 
	{
		/* optimization - get rid of instructions generated for the while
		 * loop including the conditional as the condition was false */
		eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
	}

	/* patch the jump instructions for break */
	if (update_loop_breaks (moo, moo->c->mth.code.len) <= -1) goto oops;

	/* destroy the loop information stored earlier in this function  */
	pop_loop (moo);
	loop_pushed = 0;

	/* push nil as a result of the while expression. TODO: is it the best value? anything else? */
	if (emit_byte_instruction (moo, BCODE_PUSH_NIL) <= -1) goto oops;

	return 0;

oops:
	if (loop_pushed) pop_loop (moo);
	return -1;
}

static int compile_do_while_expression (moo_t* moo)
{
	moo_ioloc_t do_loc;
	moo_oow_t precondpos, postcondpos, prebbpos, postbbpos;
	int jbinst = 0, loop_pushed = 0;
	moo_loop_t* loop = MOO_NULL;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_DO);
	do_loc = *TOKEN_LOC(moo);

	GET_TOKEN (moo); /* get { */
	prebbpos = moo->c->mth.code.len;

	/* remember information about this loop. 
	 * position of the conditional is not known yet.*/
	if (push_loop (moo, MOO_LOOP_DO_WHILE, prebbpos) <= -1) goto oops;
	loop_pushed = 1;

	if (compile_braced_block(moo) <= -1) goto oops;

	GET_TOKEN (moo); /* get the next token after } */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_WHILE)
	{
		set_syntax_error (moo, MOO_SYNERR_WHILE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}
	GET_TOKEN (moo); /* get ( */
	postbbpos = moo->c->mth.code.len;

	if (prebbpos + 1 == postbbpos && moo->c->mth.code.ptr[prebbpos] == BCODE_PUSH_NIL)
	{
		/* optimization -
		 *  the braced block is kind of empty as it only pushes nil.
		 *  get rid of this push instruction and don't generate the POP_STACKTOP */
		eliminate_instructions (moo, prebbpos, moo->c->mth.code.len - 1);
		precondpos = prebbpos;
	}
	else if (prebbpos < postbbpos)
	{
		/* emit code to pop the value pushed by the braced block */
		if (emit_byte_instruction (moo, BCODE_POP_STACKTOP) <= -1) goto oops;
	}

	precondpos = moo->c->mth.code.len;

	/* update jump instructions emitted for continue */
	if (update_loop_continues (moo, precondpos) <= -1) goto oops;
	/* cannnot destroy the loop information because of pending jump updates
	 * for break. but need to unlink it as the conditional is not really
	 * part of the loop body */
	loop = unlink_loop (moo); 

	if (compile_conditional (moo) <= -1) goto oops;
	postcondpos = moo->c->mth.code.len;
	jbinst = BCODE_JUMP_BACKWARD_IF_TRUE_0;
	if (precondpos + 1 == postcondpos)
	{
		/* simple optimization - 
		 *   if the conditional is known to be true, emit the absolute jump instruction.
		 *   if it is known to be false, kill all generated instructions. */
		if (moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_TRUE)
		{
			/* the conditional is always true. eliminate PUSH_TRUE and emit an absolute jump */
			eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
			postcondpos = precondpos;
			jbinst = BCODE_JUMP_BACKWARD_0;
		}
		else if (moo->c->mth.code.ptr[precondpos] == BCODE_PUSH_FALSE)
		{
			/* the conditional is always false. eliminate PUSH_FALSE and don't emit jump */
			eliminate_instructions (moo, precondpos, moo->c->mth.code.len - 1);
			postcondpos = precondpos;
			goto skip_emitting_jump_backward;
		}
	}

	if (emit_backward_jump_instruction (moo, jbinst, moo->c->mth.code.len - prebbpos) <= -1) 
	{
		if (moo->errnum == MOO_ERANGE) 
		{
			/* the jump offset is out of the representable range by the offset
			 * portion of the jump instruction */
			set_syntax_error (moo, MOO_SYNERR_BLKFLOOD, &do_loc, MOO_NULL);
		}
		goto oops;
	}

skip_emitting_jump_backward:
	GET_TOKEN (moo); /* get the next token after ) */

	/* update jump instructions emitted for break */
	if (update_loop_jumps (moo, &loop->break_ip_pool, moo->c->mth.code.len) <= -1) return -1;
	free_loop (moo, loop); /* destroy the unlinked loop information */
	loop = MOO_NULL;
	loop_pushed = 0;

	/* push nil as a result of the while expression. TODO: is it the best value? anything else? */
	if (emit_byte_instruction (moo, BCODE_PUSH_NIL) <= -1) goto oops;

	return 0;

oops:
	if (loop_pushed) 
	{
		if (loop) free_loop (moo, loop);
		else pop_loop (moo);
	}
	return -1;
}

static int compile_method_expression (moo_t* moo, int pop)
{
	/*
	 * method-expression := method-assignment-expression | basic-expression | if-expression | while-expression | do-while-expression
	 * method-assignment-expression := identifier ":=" method-expression
	 * if-expression := if ( ) {  } elsif { } else { }
	 * while-expression := while () {}
	 * do-while-expression := do { } while ()
	 */

	moo_oocs_t assignee;
	moo_oow_t index;
	int ret = 0;

	MOO_ASSERT (moo, pop == 0 || pop == 1);
	MOO_MEMSET (&assignee, 0, MOO_SIZEOF(assignee));

	if (TOKEN_TYPE(moo) == MOO_IOTOK_IF)
	{
		if (compile_if_expression (moo) <= -1) return -1;
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_WHILE)
	{
		if (compile_while_expression (moo) <= -1) return -1;
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_DO)
	{
		if (compile_do_while_expression (moo) <= -1) return -1;
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT ||
	         TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		moo_ioloc_t assignee_loc;
		moo_oow_t assignee_offset;
		int assignee_dotted;

		/* store the assignee name to the internal buffer
		 * to make it valid after the token buffer has been overwritten */
		assignee = moo->c->tok.name;

		if (clone_assignee(moo, &assignee, &assignee_offset) <= -1) return -1;

		assignee_loc = moo->c->tok.loc;
		assignee_dotted = (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED);

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) == MOO_IOTOK_ASSIGN) 
		{
			/* assignment expression */
			var_info_t var;

			GET_TOKEN (moo);

			if (compile_method_expression(moo, 0) <= -1) goto oops;

			/* compile_method_expression() is called after clone_assignee().
			 * clone_assignee() may reallocate a single buffer that holds 
			 * a series of assigness names. this pointer based operation is
			 * fragile as it can change. use the offset of the cloned
			 * assignee to update the actual pointer after the recursive
			 * compile_method_expression() call */
			assignee.ptr = &moo->c->mth.assignees.ptr[assignee_offset];
			if (get_variable_info(moo, &assignee, &assignee_loc, assignee_dotted, &var) <= -1) goto oops;

			switch (var.type)
			{
				case VAR_ARGUMENT: /* TODO: consider if assigning to an argument should be disallowed */
				case VAR_TEMPORARY:
				{
				#if defined(MOO_USE_CTXTEMPVAR)
					if (moo->c->mth.blk_depth > 0)
					{
						moo_oow_t i;

						/* if a temporary variable is accessed inside a block,
						 * use a special instruction to indicate it */
						MOO_ASSERT (moo, var.pos < moo->c->mth.blk_tmprcnt[moo->c->mth.blk_depth]);
						for (i = moo->c->mth.blk_depth; i > 0; i--)
						{
							if (var.pos >= moo->c->mth.blk_tmprcnt[i - 1])
							{
								if (emit_double_param_instruction(moo, (pop? BCODE_POP_INTO_CTXTEMPVAR_0: BCODE_STORE_INTO_CTXTEMPVAR_0), moo->c->mth.blk_depth - i, var.pos - moo->c->mth.blk_tmprcnt[i - 1]) <= -1) return -1;
								goto temporary_done;
							}
						}
					}
				#endif

					if (emit_single_param_instruction (moo, (pop? BCODE_POP_INTO_TEMPVAR_0: BCODE_STORE_INTO_TEMPVAR_0), var.pos) <= -1) goto oops;

				temporary_done:
					ret = pop;
					break;
				}

				case VAR_INSTANCE:
				case VAR_CLASSINST:
					if (emit_single_param_instruction (moo, (pop? BCODE_POP_INTO_INSTVAR_0: BCODE_STORE_INTO_INSTVAR_0), var.pos) <= -1) goto oops;
					ret = pop;
					break;

				case VAR_CLASS:
					if (add_literal (moo, (moo_oop_t)var.cls, &index) <= -1 ||
					    emit_double_param_instruction (moo, (pop? BCODE_POP_INTO_OBJVAR_0: BCODE_STORE_INTO_OBJVAR_0), var.pos, index) <= -1) goto oops;
					ret = pop;
					break;

				case VAR_GLOBAL:
					if (add_literal(moo, (moo_oop_t)var.gbl, &index) <= -1 ||
					    emit_single_param_instruction(moo, (pop? BCODE_POP_INTO_OBJECT_0: BCODE_STORE_INTO_OBJECT_0), index) <= -1) return -1;
					ret = pop;
					break;

				default:
					moo->errnum = MOO_EINTERN;
					goto oops;
			}
		}
		else 
		{
			/* what is held in assignee is not an assignee any more.
			 * potentially it is a variable or object reference
			 * to be pused on to the stack */
			assignee.ptr = &moo->c->mth.assignees.ptr[assignee_offset];
			if (compile_basic_expression(moo, &assignee, &assignee_loc, assignee_dotted) <= -1) goto oops;
		}
	}
	else 
	{
		assignee.len = 0;
		if (compile_basic_expression(moo, MOO_NULL, MOO_NULL, 0) <= -1) goto oops;
	}

	moo->c->mth.assignees.len -= assignee.len;
	return ret;

oops:
	moo->c->mth.assignees.len -= assignee.len;
	return -1;
}

static int compile_special_statement (moo_t* moo)
{
	if (TOKEN_TYPE(moo) == MOO_IOTOK_RETURN) 
	{
		/* ^ - return - return to the sender of the origin */
		GET_TOKEN (moo);
		if (compile_method_expression(moo, 0) <= -1) return -1;
		return emit_byte_instruction (moo, BCODE_RETURN_STACKTOP);
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_LOCAL_RETURN)
	{
		/* ^^ - local return - return to the origin */
		GET_TOKEN (moo);
		if (compile_method_expression(moo, 0) <= -1) return -1;
		return emit_byte_instruction (moo, BCODE_LOCAL_RETURN);
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_BREAK)
	{
		if (!moo->c->mth.loop)
		{
			/* break outside a loop */
			set_syntax_error (moo, MOO_SYNERR_NOTINLOOP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		if (moo->c->mth.loop->blkcount > 0)
		{
			/* break cannot cross boundary of a block */
			set_syntax_error (moo, MOO_SYNERR_INBLOCK, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo); /* read the next token to break */
		return inject_break_to_loop (moo);
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_CONTINUE)
	{
		if (!moo->c->mth.loop)
		{
			set_syntax_error (moo, MOO_SYNERR_NOTINLOOP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		if (moo->c->mth.loop->blkcount > 0)
		{
			/* continue cannot cross boundary of a block */
			set_syntax_error (moo, MOO_SYNERR_INBLOCK, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo); /* read the next token to continue */

		return (moo->c->mth.loop->type == MOO_LOOP_DO_WHILE)?
			inject_continue_to_loop (moo): /* in a do-while loop, the position to the conditional is not known yet */
			emit_backward_jump_instruction (moo, BCODE_JUMP_BACKWARD_0, moo->c->mth.code.len - moo->c->mth.loop->startpos);
	}

	return 9999; /* to indicate that no special statement has been seen and processed */
}

static int compile_block_statement (moo_t* moo)
{
	/* compile_block_statement() is a simpler version of
	 * of compile_method_statement(). it doesn't cater for
	 * popping the stack top */
	int n;
	n = compile_special_statement(moo);
	if (n <= -1) return -1;
	if (n == 9999) n = compile_method_expression(moo, 0);
	return n;
}

static int compile_method_statement (moo_t* moo)
{
	/*
	 * method-statement := method-return-statement | break | continue | method-expression
	 * method-return-statement := "^" method-expression
	 */
	int n;

	n = compile_special_statement(moo);
	if (n <= -1) return -1;

	if (n == 9999)
	{
		/* the second parameter to compile_method_expression() indicates 
		 * that the stack top will eventually be popped off. the compiler
		 * can optimize some instruction sequencese. for example, two 
		 * consecutive store and pop intructions can be transformed to 
		 * a more specialized single pop-and-store instruction. 
		 * the compile_method_expression() function emits POP_INTO_XXX 
		 * instructions if the second parameter is 1 whenever possible and 
		 * STORE_INTO_XXX if it's 0.*/
		moo_oow_t preexprpos;

		preexprpos = moo->c->mth.code.len;
		n = compile_method_expression(moo, 1);
		if (n <= -1) return -1;

		/* if n is 1, no stack popping is required as POP_INTO_XXX has been
		 * emitted in place of STORE_INTO_XXX. */
		if (n == 0)
		{
			if (preexprpos + 1 == moo->c->mth.code.len)
			{
/* TODO: MORE optimization. if expresssion is a literal, no push and pop are required. check for multie-byte instructions as well */
				switch (moo->c->mth.code.ptr[preexprpos])
				{
					case BCODE_PUSH_NIL:
					case BCODE_PUSH_TRUE:
					case BCODE_PUSH_FALSE:
					case BCODE_PUSH_CONTEXT:
					case BCODE_PUSH_PROCESS:
					case BCODE_PUSH_NEGONE:
					case BCODE_PUSH_ZERO:
					case BCODE_PUSH_ONE:
					case BCODE_PUSH_TWO:
						/* eliminate the unneeded push instruction */
						n = 0;
						eliminate_instructions (moo, preexprpos, moo->c->mth.code.len - 1);
						break;
					default:
						goto pop_stacktop;
				}
			}
			else
			{
			pop_stacktop:
				return emit_byte_instruction (moo, BCODE_POP_STACKTOP);
			}
		}
	}

	return n;
}

static int compile_method_statements (moo_t* moo)
{
	/*
	 * method-statements := method-statement ("." | ("." method-statements))*
	 */

	if (TOKEN_TYPE(moo) != MOO_IOTOK_EOF &&
	    TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		do
		{
			if (compile_method_statement(moo) <= -1) return -1;

			if (TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD) 
			{
				/* period after a statement */
				GET_TOKEN (moo);

				if (TOKEN_TYPE(moo) == MOO_IOTOK_EOF ||
				    TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;
			}
			else
			{
				if (TOKEN_TYPE(moo) == MOO_IOTOK_EOF ||
				    TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) break;

				/* not a period, EOF, nor } */
				set_syntax_error (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
		while (1);
	}

	/* arrange to return the receiver if execution reached 
	 * the end of the method without explicit return */
	return emit_byte_instruction (moo, BCODE_RETURN_RECEIVER);
}

static int add_compiled_method (moo_t* moo)
{
	moo_oop_char_t name; /* selector */
	moo_oop_method_t mth; /* method */
#if defined(MOO_USE_OBJECT_TRAILER)
	/* nothing extra */
#else
	moo_oop_byte_t code;
#endif
	moo_oow_t tmp_count = 0;
	moo_oow_t i;
	moo_ooi_t preamble_code, preamble_index, preamble_flags;

	name = (moo_oop_char_t)moo_makesymbol (moo, moo->c->mth.name.ptr, moo->c->mth.name.len);
	if (!name) return -1;
	moo_pushtmp (moo, (moo_oop_t*)&name); tmp_count++;

	/* The variadic data part passed to moo_instantiate() is not GC-safe */
#if defined(MOO_USE_OBJECT_TRAILER)
	mth = (moo_oop_method_t)moo_instantiatewithtrailer (moo, moo->_method, moo->c->mth.literal_count, moo->c->mth.code.ptr, moo->c->mth.code.len);
#else
	mth = (moo_oop_method_t)moo_instantiate (moo, moo->_method, MOO_NULL, moo->c->mth.literal_count);
#endif
	if (!mth) goto oops;

	for (i = 0; i < moo->c->mth.literal_count; i++)
	{
		/* let's do the variadic data initialization here */
		mth->slot[i] = moo->c->mth.literals[i];
	}
	moo_pushtmp (moo, (moo_oop_t*)&mth); tmp_count++;

#if defined(MOO_USE_OBJECT_TRAILER)
	/* do nothing */
#else
	code = (moo_oop_byte_t)moo_instantiate (moo, moo->_byte_array, moo->c->mth.code.ptr, moo->c->mth.code.len);
	if (!code) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&code); tmp_count++;
#endif

	preamble_code = MOO_METHOD_PREAMBLE_NONE;
	preamble_index = 0;
	preamble_flags = 0;

	if (moo->c->mth.pftype <= 0)
	{
		/* no primitive is set */
		if (moo->c->mth.code.len <= 0)
		{
			preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
		}
		else
		{
			if (moo->c->mth.code.ptr[0] == BCODE_RETURN_RECEIVER)
			{
				preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
			}
			else if (moo->c->mth.code.len > 1 && moo->c->mth.code.ptr[1] == BCODE_RETURN_STACKTOP)
			{
				switch (moo->c->mth.code.ptr[0])
				{
					case BCODE_PUSH_RECEIVER:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
						break;

					case BCODE_PUSH_NIL:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_NIL;
						break;

					case BCODE_PUSH_TRUE:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_TRUE;
						break;

					case BCODE_PUSH_FALSE:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_FALSE;
						break;

					case BCODE_PUSH_NEGONE:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_NEGINDEX;
						preamble_index = 1;
						break;

					case BCODE_PUSH_ZERO:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INDEX;
						preamble_index = 0;
						break;

					case BCODE_PUSH_ONE:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INDEX;
						preamble_index = 1;
						break;

					case BCODE_PUSH_TWO:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INDEX;
						preamble_index = 2;
						break;

					case BCODE_PUSH_INSTVAR_0:
					case BCODE_PUSH_INSTVAR_1:
					case BCODE_PUSH_INSTVAR_2:
					case BCODE_PUSH_INSTVAR_3:
					case BCODE_PUSH_INSTVAR_4:
					case BCODE_PUSH_INSTVAR_5:
					case BCODE_PUSH_INSTVAR_6:
					case BCODE_PUSH_INSTVAR_7:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INSTVAR;
						preamble_index = moo->c->mth.code.ptr[0] & 0x7; /* low 3 bits */
						break;
				}
			}
			else if (moo->c->mth.code.len > MOO_BCODE_LONG_PARAM_SIZE + 1 &&
			         moo->c->mth.code.ptr[MOO_BCODE_LONG_PARAM_SIZE + 1] == BCODE_RETURN_STACKTOP)
			{
				int i;
				switch (moo->c->mth.code.ptr[0])
				{
					case BCODE_PUSH_INSTVAR_X:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INSTVAR;
						goto set_preamble_index;

					case BCODE_PUSH_INTLIT:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_INDEX;
						goto set_preamble_index;

					case BCODE_PUSH_NEGINTLIT:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_NEGINDEX;
						goto set_preamble_index;

					set_preamble_index:
						preamble_index = 0;
						for (i = 1; i <= MOO_BCODE_LONG_PARAM_SIZE; i++)
						{
							preamble_index = (preamble_index << 8) | moo->c->mth.code.ptr[i];
						}

						if (!MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(preamble_index))
						{
							/* the index got out of the range */
							preamble_code = MOO_METHOD_PREAMBLE_NONE;
							preamble_index = 0;
						}
				}
			}
		}
	}
	else if (moo->c->mth.pftype == 1)
	{
		preamble_code = MOO_METHOD_PREAMBLE_PRIMITIVE;
		preamble_index = moo->c->mth.pfnum;
	}
	else if (moo->c->mth.pftype == 2)
	{
		preamble_code = MOO_METHOD_PREAMBLE_NAMED_PRIMITIVE;
		preamble_index = moo->c->mth.pfnum; /* index to literal frame */
	}
	else if (moo->c->mth.pftype == 3)
	{
		preamble_code = MOO_METHOD_PREAMBLE_EXCEPTION;
		preamble_index = 0;
	}
	else 
	{
		MOO_ASSERT (moo, moo->c->mth.pftype == 4);
		preamble_code = MOO_METHOD_PREAMBLE_ENSURE;
		preamble_index = 0;
	}

	if (moo->c->mth.variadic /*&& moo->c->mth.tmpr_nargs > 0*/)
		preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_VARIADIC;

	MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(preamble_index));

	mth->owner = moo->c->cls.self_oop;
	mth->name = name;
	mth->preamble = MOO_SMOOI_TO_OOP(MOO_METHOD_MAKE_PREAMBLE(preamble_code, preamble_index, preamble_flags));
	mth->preamble_data[0] = MOO_SMOOI_TO_OOP(0);
	mth->preamble_data[1] = MOO_SMOOI_TO_OOP(0);
	mth->tmpr_count = MOO_SMOOI_TO_OOP(moo->c->mth.tmpr_count);
	mth->tmpr_nargs = MOO_SMOOI_TO_OOP(moo->c->mth.tmpr_nargs);

#if defined(MOO_USE_OBJECT_TRAILER)
	/* do nothing */
#else
	mth->code = code;
#endif

	/*TODO: preserve source??? mth->text = moo->c->mth.text
the compiler must collect all source method string collected so far.
need to write code to collect string.
*/

#if defined(MOO_DEBUG_COMPILER)
	moo_decode (moo, mth, &moo->c->cls.fqn);
#endif

	moo_poptmps (moo, tmp_count); tmp_count = 0;

#ifdef MTHDIC
	if (!moo_putatdic(moo, moo->c->cls.mthdic_oop[moo->c->mth.type], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
#else
	if (!moo_putatdic(moo, moo->c->cls.self_oop->mthdic[moo->c->mth.type], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
#endif

	return 0;

oops:
	moo_poptmps (moo, tmp_count);
	return -1;
}

static int compile_method_definition (moo_t* moo)
{
	/* clear data required to compile a method */
	moo->c->mth.type = MOO_METHOD_INSTANCE;
	moo->c->mth.text.len = 0;
	moo->c->mth.assignees.len = 0;
	moo->c->mth.binsels.len = 0;
	moo->c->mth.kwsels.len = 0;
	moo->c->mth.name.len = 0;
	MOO_MEMSET (&moo->c->mth.name_loc, 0, MOO_SIZEOF(moo->c->mth.name_loc));
	moo->c->mth.variadic = 0;
	moo->c->mth.tmprs.len = 0;
	moo->c->mth.tmpr_count = 0;
	moo->c->mth.tmpr_nargs = 0;
	moo->c->mth.literal_count = 0;
	moo->c->mth.balit_count = 0;
	moo->c->mth.arlit_count = 0;
	moo->c->mth.pftype = 0;
	moo->c->mth.pfnum = 0;
	moo->c->mth.blk_depth = 0;
	moo->c->mth.code.len = 0;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process method modifiers  */
		GET_TOKEN (moo);

		if (is_token_symbol(moo, VOCA_CLASS_S))
		{
			/* method(#class) */
			moo->c->mth.type = MOO_METHOD_CLASS;
			GET_TOKEN (moo);
		}

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			/* ) expected */
			set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (compile_method_name(moo) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		/* { expected */
		set_syntax_error (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	if (compile_method_temporaries(moo) <= -1 ||
	    compile_method_primitive(moo) <= -1 ||
	    compile_method_statements(moo) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		/* } expected */
		set_syntax_error (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}
	GET_TOKEN (moo);

	/* add a compiled method to the method dictionary */
	if (add_compiled_method(moo) <= -1) return -1;

	return 0;
}

static int make_defined_class (moo_t* moo)
{
	/* this function make a class object with no functions/methods */

	moo_oop_t tmp;
	moo_oow_t spec, self_spec;
	int just_made = 0;

	spec = MOO_CLASS_SPEC_MAKE (moo->c->cls.var_count[VAR_INSTANCE],  
	                             ((moo->c->cls.flags & CLASS_INDEXED)? 1: 0),
	                             moo->c->cls.indexed_type);

	self_spec = MOO_CLASS_SELFSPEC_MAKE (moo->c->cls.var_count[VAR_CLASS],
	                                      moo->c->cls.var_count[VAR_CLASSINST]);

	if (moo->c->cls.self_oop)
	{
		/* this is an internally created class object being defined. */

		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->c->cls.self_oop) == moo->_class);
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_KERNEL (moo->c->cls.self_oop) == 1);

		if (spec != MOO_OOP_TO_SMOOI(moo->c->cls.self_oop->spec) ||
		    self_spec != MOO_OOP_TO_SMOOI(moo->c->cls.self_oop->selfspec))
		{
			/* it conflicts with internal definition */
			set_syntax_error (moo, MOO_SYNERR_CLASSCONTRA, &moo->c->cls.fqn_loc, &moo->c->cls.name);
			return -1;
		}
	}
	else
	{
		/* the class variables and class instance variables are placed
		 * inside the class object after the fixed part. */
		tmp = moo_instantiate (moo, moo->_class, MOO_NULL,
		                        moo->c->cls.var_count[VAR_CLASSINST] + moo->c->cls.var_count[VAR_CLASS]);
		if (!tmp) return -1;

		just_made = 1;
		moo->c->cls.self_oop = (moo_oop_class_t)tmp;

		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->c->cls.self_oop) == moo->_class);

		moo->c->cls.self_oop->spec = MOO_SMOOI_TO_OOP(spec);
		moo->c->cls.self_oop->selfspec = MOO_SMOOI_TO_OOP(self_spec);
	}

/* TODO: check if the current class definition conflicts with the superclass.
 * if superclass is byte variable, the current class cannot be word variable or something else.
*  TODO: TODO: TODO:
 */
	MOO_OBJ_SET_FLAGS_KERNEL (moo->c->cls.self_oop, 2);

	moo->c->cls.self_oop->superclass = moo->c->cls.super_oop;

	tmp = moo_makesymbol (moo, moo->c->cls.name.ptr, moo->c->cls.name.len);
	if (!tmp) return -1;
	moo->c->cls.self_oop->name = (moo_oop_char_t)tmp;

	tmp = moo_makestring (moo, moo->c->cls.vars[VAR_INSTANCE].ptr, moo->c->cls.vars[VAR_INSTANCE].len);
	if (!tmp) return -1;
	moo->c->cls.self_oop->instvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring (moo, moo->c->cls.vars[VAR_CLASS].ptr, moo->c->cls.vars[VAR_CLASS].len);
	if (!tmp) return -1;
	moo->c->cls.self_oop->classvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring (moo, moo->c->cls.vars[VAR_CLASSINST].ptr, moo->c->cls.vars[VAR_CLASSINST].len);
	if (!tmp) return -1;
	moo->c->cls.self_oop->classinstvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring (moo, moo->c->cls.pooldic.ptr, moo->c->cls.pooldic.len);
	if (!tmp) return -1;
	moo->c->cls.self_oop->pooldics = (moo_oop_char_t)tmp;

/* TOOD: good dictionary size */
	tmp = (moo_oop_t)moo_makedic (moo, moo->_method_dictionary, INSTANCE_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
#ifdef MTHDIC
	moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE] = (moo_oop_set_t)tmp;
#else
	moo->c->cls.self_oop->mthdic[MOO_METHOD_INSTANCE] = (moo_oop_set_t)tmp;
#endif

/* TOOD: good dictionary size */
	tmp = (moo_oop_t)moo_makedic (moo, moo->_method_dictionary, CLASS_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
#ifdef MTHDIC
	moo->c->cls.mthdic_oop[MOO_METHOD_CLASS] = (moo_oop_set_t)tmp;
#else
	moo->c->cls.self_oop->mthdic[MOO_METHOD_CLASS] = (moo_oop_set_t)tmp;
#endif

/* TODO: initialize more fields??? whatelse. */

/* TODO: update the subclasses field of the superclass if it's not nil */

	if (just_made)
	{
		/* register the class to the system dictionary */
		/*if (!moo_putatsysdic(moo, (moo_oop_t)moo->c->cls.self_oop->name, (moo_oop_t)moo->c->cls.self_oop)) return -1;*/
		if (!moo_putatdic(moo, moo->c->cls.ns_oop, (moo_oop_t)moo->c->cls.self_oop->name, (moo_oop_t)moo->c->cls.self_oop)) return -1;
	}

	return 0;
}

static int __compile_class_definition (moo_t* moo, int extend)
{
	/* 
	 * class-definition := #class class-modifier? class-name  (class-body | class-module-import)
	 *
	 * class-modifier := "(" (#byte | #character | #word | #pointer)? ")"
	 * class-body := "{" variable-definition* method-definition* "}"
	 * class-module-import := from "module-name-string"
	 * 
	 * variable-definition := (#dcl | #declare) variable-modifier? variable-list "."
	 * variable-modifier := "(" (#class | #classinst)? ")"
	 * variable-list := identifier*
	 *
	 * method-definition := method method-modifier? method-actual-definition
	 * method-modifier := "(" (#class | #instance)? ")"
	 * method-actual-definition := method-name "{" method-tempraries? method-primitive? method-statements* "}"
	 *
	 * NOTE: when extending a class, class-module-import and variable-definition are not allowed.
	 */
	moo_oop_association_t ass;
	moo_ooch_t modname[MOO_MOD_NAME_LEN_MAX + 1];
	moo_oow_t modnamelen = 0;

	if (!extend && TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process class modifiers */

		GET_TOKEN (moo);

		if (is_token_symbol(moo, VOCA_BYTE_S))
		{
			/* class(#byte) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_BYTE;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_CHARACTER_S))
		{
			/* class(#character) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_CHAR;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_HALFWORD_S))
		{
			/* class(#halfword) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_HALFWORD;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_WORD_S))
		{
			/* class(#word) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_WORD;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_POINTER_S))
		{
			/* class(#pointer) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_OOP;
			GET_TOKEN (moo);
		}
		else if (is_token_symbol(moo, VOCA_LIWORD_S))
		{
			/* class(#liword) */
			moo->c->cls.flags |= CLASS_INDEXED;
			moo->c->cls.indexed_type = MOO_OBJ_TYPE_LIWORD;
			GET_TOKEN (moo);
		}

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT &&
	    TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		/* class name expected. */
		set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

#if 0
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && is_restricted_word (TOKEN_NAME(moo)))
	{
		/* wrong class name */
		set_syntax_error (moo, MOO_SYNERR_CLASSNAME, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}
#endif

	/* copy the class name */
	if (set_class_fqn(moo, TOKEN_NAME(moo)) <= -1) return -1;
	moo->c->cls.fqn_loc = moo->c->tok.loc;
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		if (preprocess_dotted_name(moo, extend, 0, &moo->c->cls.fqn, &moo->c->cls.fqn_loc, &moo->c->cls.name, &moo->c->cls.ns_oop) <= -1) return -1;
	}
	else
	{
		moo->c->cls.ns_oop = moo->sysdic;
	}
	GET_TOKEN (moo); 

	if (extend)
	{
		/* extending class */
		MOO_ASSERT (moo, moo->c->cls.flags == 0);

		/*ass = moo_lookupsysdic(moo, &moo->c->cls.name);*/
		ass = moo_lookupdic(moo, moo->c->cls.ns_oop, &moo->c->cls.name);
		if (ass && 
		    MOO_CLASSOF(moo, ass->value) == moo->_class &&
		    MOO_OBJ_GET_FLAGS_KERNEL(ass->value) != 1)
		{
			/* the value must be a class object.
			 * and it must be either a user-defined(0) or 
			 * completed kernel built-in(2). 
			 * an incomplete kernel built-in class object(1) can not be
			 * extended */
			moo->c->cls.self_oop = (moo_oop_class_t)ass->value;
		}
		else
		{
			/* only an existing class can be extended. */
			set_syntax_error (moo, MOO_SYNERR_CLASSUNDEF, &moo->c->cls.fqn_loc, &moo->c->cls.name);
			return -1;
		}

		moo->c->cls.super_oop = moo->c->cls.self_oop->superclass;

		MOO_ASSERT (moo, (moo_oop_t)moo->c->cls.super_oop == moo->_nil || 
		             MOO_CLASSOF(moo, moo->c->cls.super_oop) == moo->_class);
	}
	else
	{
		int super_is_nil = 0;

		MOO_INFO2 (moo, "Defining a class %.*js\n", moo->c->cls.fqn.len, moo->c->cls.fqn.ptr);

		if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
		{
			/* superclass is specified. new class defintion.
			 * for example, #class Class(Stix) 
			 */
			GET_TOKEN (moo); /* read superclass name */

			/* TODO: multiple inheritance */

			if (TOKEN_TYPE(moo) == MOO_IOTOK_NIL)
			{
				super_is_nil = 1;
			}
			else if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT &&
			         TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
			{
				/* superclass name expected */
				set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (set_superclass_fqn(moo, TOKEN_NAME(moo)) <= -1) return -1;
			moo->c->cls.superfqn_loc = moo->c->tok.loc;

			if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
			{
				if (preprocess_dotted_name(moo, 1, 0, &moo->c->cls.superfqn, &moo->c->cls.superfqn_loc, &moo->c->cls.supername, &moo->c->cls.superns_oop) <= -1) return -1;
			}
			else
			{
				/* if no fully qualified name is specified for the super class name,
				 * the name is searched in the name space that the class being defined
				 * belongs to first and in the 'moo->sysdic'. */
				moo->c->cls.superns_oop = moo->c->cls.ns_oop;
			}

			GET_TOKEN (moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
			{
				set_syntax_error (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			GET_TOKEN (moo);
		}
		else 
		{
			super_is_nil = 1;
		}

		/*ass = moo_lookupsysdic(moo, &moo->c->cls.name);*/
		ass = moo_lookupdic (moo, moo->c->cls.ns_oop, &moo->c->cls.name);
		if (ass)
		{
			if (MOO_CLASSOF(moo, ass->value) != moo->_class  ||
			    MOO_OBJ_GET_FLAGS_KERNEL(ass->value) > 1)
			{
				/* the object found with the name is not a class object 
				 * or the the class object found is a fully defined kernel 
				 * class object */
				set_syntax_error (moo, MOO_SYNERR_CLASSDUP, &moo->c->cls.fqn_loc, &moo->c->cls.name);
				return -1;
			}

			moo->c->cls.self_oop = (moo_oop_class_t)ass->value;
		}
		else
		{
			/* no class of such a name is found. it's a new definition,
			 * which is normal for most new classes. */
			MOO_ASSERT (moo, moo->c->cls.self_oop == MOO_NULL);
		}

		if (super_is_nil)
		{
			moo->c->cls.super_oop = moo->_nil;
		}
		else
		{
			/* ass = moo_lookupsysdic(moo, &moo->c->cls.supername); */
			ass = moo_lookupdic (moo, moo->c->cls.superns_oop, &moo->c->cls.supername);
			if (!ass && moo->c->cls.superns_oop != moo->sysdic)
				ass = moo_lookupdic (moo, moo->sysdic, &moo->c->cls.supername);
			if (ass &&
			    MOO_CLASSOF(moo, ass->value) == moo->_class &&
			    MOO_OBJ_GET_FLAGS_KERNEL(ass->value) != 1) 
			{
				/* the value found must be a class and it must not be 
				 * an incomplete internal class object */
				moo->c->cls.super_oop = ass->value;
			}
			else
			{
				/* there is no object with such a name. or,
				 * the object found with the name is not a class object. or,
				 * the class object found is a internally defined kernel
				 * class object. */
				set_syntax_error (moo, MOO_SYNERR_CLASSUNDEF, &moo->c->cls.superfqn_loc, &moo->c->cls.superfqn);
				return -1;
			}
		}

		if (is_token_word (moo, VOCA_FROM))
		{
			GET_TOKEN (moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_STRLIT)
			{
				set_syntax_error (moo, MOO_SYNERR_STRING, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (TOKEN_NAME_LEN(moo) < 1 || 
			    TOKEN_NAME_LEN(moo) > MOO_MOD_NAME_LEN_MAX ||
			    moo_findoochar(TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo), '_'))
			{
				set_syntax_error (moo, MOO_SYNERR_MODNAME, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			modnamelen = TOKEN_NAME_LEN(moo);
			moo_copyoochars (modname, TOKEN_NAME_PTR(moo), modnamelen);

			GET_TOKEN (moo);
		}
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	if (moo->c->cls.super_oop != moo->_nil)
	{
		/* adjust the instance variable count and the class instance variable
		 * count to include that of a superclass */
		moo_oop_class_t c;
		moo_oow_t spec, self_spec;

		c = (moo_oop_class_t)moo->c->cls.super_oop;
		spec = MOO_OOP_TO_SMOOI(c->spec);
		self_spec = MOO_OOP_TO_SMOOI(c->selfspec);
		moo->c->cls.var_count[VAR_INSTANCE] = MOO_CLASS_SPEC_NAMED_INSTVAR(spec);
		moo->c->cls.var_count[VAR_CLASSINST] = MOO_CLASS_SELFSPEC_CLASSINSTVAR(self_spec);
	}

	GET_TOKEN (moo);

	if (extend)
	{
		moo_oop_char_t pds;

		/* when a class is extended, a new variable cannot be added */
		if (is_token_word(moo, VOCA_DCL) || is_token_word(moo, VOCA_DECLARE))
		{
			set_syntax_error (moo, MOO_SYNERR_DCLBANNED, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

#ifdef MTHDIC
		/* use the method dictionary of an existing class object */
		moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE] = moo->c->cls.self_oop->mthdic[MOO_METHOD_INSTANCE];
		moo->c->cls.mthdic_oop[MOO_METHOD_CLASS] = moo->c->cls.self_oop->mthdic[MOO_METHOD_CLASS];
#endif

		/* load the pooldic definition from the existing class object */
		pds = moo->c->cls.self_oop->pooldics;
		if ((moo_oop_t)pds != moo->_nil)
		{
			moo_ooch_t* ptr, * end;

			MOO_ASSERT (moo, MOO_CLASSOF(moo, pds) == moo->_string);

			ptr = pds->slot;
			end = pds->slot + MOO_OBJ_GET_SIZE(pds);

			/* this loop handles the pooldics string as if it's a pooldic import.
			 * see compile_class_level_variables() for mostly identical code except token handling */
			do
			{
				moo_oocs_t last, tok;
				moo_ioloc_t loc;
				int dotted = 0;
				moo_oop_set_t ns_oop;

				while (ptr < end && is_spacechar(*ptr)) ptr++;
				if (ptr >= end) break;

				MOO_MEMSET (&loc, 0, MOO_SIZEOF(loc)); /* fake location */

				tok.ptr = ptr;
				while (ptr < end && !is_spacechar(*ptr))
				{
					if (*ptr == '.') dotted = 1;
					ptr++;
				}
				tok.len = ptr - tok.ptr;
				MOO_ASSERT (moo, tok.len > 0);

				if (dotted)
				{
					if (preprocess_dotted_name(moo, 0, 0, &tok, &loc, &last, &ns_oop) <= -1) return -1;
				}
				else
				{
					last = tok;
					/* it falls back to the name space of the class */
					ns_oop = moo->c->cls.ns_oop; 
				}

				if (import_pool_dictionary(moo, ns_oop, &last, &tok, &loc) <= -1) return -1;
			}
			while (1);
		}
	}
	else
	{
		/* a new class including an internally defined class object */

		while (is_token_word(moo, VOCA_DCL) || is_token_word(moo, VOCA_DECLARE))
		{
			/* variable definition. dcl or declare */
			GET_TOKEN (moo);
			if (compile_class_level_variables(moo) <= -1) return -1;
		}

		if (make_defined_class(moo) <= -1) return -1;

		if (modnamelen > 0)
		{
			if (moo_importmod (moo, (moo_oop_t)moo->c->cls.self_oop, modname, modnamelen) <= -1) return -1;
		}
	}

	while (is_token_word(moo, VOCA_METHOD))
	{
		/* method definition. method */
		GET_TOKEN (moo);
		if (compile_method_definition(moo) <= -1) return -1;
	}
	
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

#ifdef MTHDIC
	if (!extend)
	{
		/* link the method dictionaries created to the actual class object */
/* TODO: anything else to set? */
		moo->c->cls.self_oop->mthdic[MOO_CLASS_MTHDIC_INSTANCE] = moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE];
		moo->c->cls.self_oop->mthdic[MOO_CLASS_MTHDIC_CLASS] = moo->c->cls.mthdic_oop[MOO_METHOD_CLASS];
	}
#endif

	GET_TOKEN (moo);
	return 0;
}

static int compile_class_definition (moo_t* moo, int extend)
{
	int n;
	moo_oow_t i;

	/* reset the structure to hold information about a class to be compiled */
	moo->c->cls.flags = 0;
	moo->c->cls.indexed_type = MOO_OBJ_TYPE_OOP;

	moo->c->cls.name.len = 0;
	moo->c->cls.supername.len = 0;
	MOO_MEMSET (&moo->c->cls.fqn_loc, 0, MOO_SIZEOF(moo->c->cls.fqn_loc));
	MOO_MEMSET (&moo->c->cls.superfqn_loc, 0, MOO_SIZEOF(moo->c->cls.superfqn_loc));

	MOO_ASSERT (moo, MOO_COUNTOF(moo->c->cls.var_count) == MOO_COUNTOF(moo->c->cls.vars));
	for (i = 0; i < MOO_COUNTOF(moo->c->cls.var_count); i++) 
	{
		moo->c->cls.var_count[i] = 0;
		moo->c->cls.vars[i].len = 0;
	}

	moo->c->cls.pooldic_count = 0;
	moo->c->cls.pooldic.len = 0;

	moo->c->cls.self_oop = MOO_NULL;
	moo->c->cls.super_oop = MOO_NULL;
#ifdef MTHDIC
	moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE] = MOO_NULL;
	moo->c->cls.mthdic_oop[MOO_METHOD_CLASS] = MOO_NULL;
#endif
	moo->c->cls.ns_oop = MOO_NULL;
	moo->c->cls.superns_oop = MOO_NULL;
	moo->c->mth.literal_count = 0;
	moo->c->mth.balit_count = 0;
	moo->c->mth.arlit_count = 0;

	/* do main compilation work */
	n = __compile_class_definition (moo, extend);

	/* reset these oops plus literal pointers not to confuse gc_compiler() */
	moo->c->cls.self_oop = MOO_NULL;
	moo->c->cls.super_oop = MOO_NULL;
#ifdef MTHDIC
	moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE] = MOO_NULL;
	moo->c->cls.mthdic_oop[MOO_METHOD_CLASS] = MOO_NULL;
#endif
	moo->c->cls.ns_oop = MOO_NULL;
	moo->c->cls.superns_oop = MOO_NULL;
	moo->c->mth.literal_count = 0;
	moo->c->mth.balit_count = 0;
	moo->c->mth.arlit_count = 0;

	moo->c->cls.pooldic_count = 0;

	return n;
}

static int __compile_pooldic_definition (moo_t* moo)
{
	moo_oop_t lit;
	moo_ooi_t tally;
	moo_oow_t i;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && 
	    TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		set_syntax_error (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	/* [NOTE] 
	 * reuse moo->c->cls.fqn and related fields are reused 
	 * to store the pool dictionary name */
	if (set_class_fqn(moo, TOKEN_NAME(moo)) <= -1) return -1;
	moo->c->cls.fqn_loc = moo->c->tok.loc;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		if (preprocess_dotted_name(moo, 0, 0, &moo->c->cls.fqn, &moo->c->cls.fqn_loc, &moo->c->cls.name, &moo->c->cls.ns_oop) <= -1) return -1;
	}
	else
	{
		moo->c->cls.ns_oop = moo->sysdic;
	}

	if (moo_lookupdic (moo, moo->c->cls.ns_oop, &moo->c->cls.name))
	{
		/* a conflicting entry has been found */
		set_syntax_error (moo, MOO_SYNERR_POOLDICDUP, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	MOO_INFO2 (moo, "Defining a pool dictionary %.*js\n", moo->c->cls.fqn.len, moo->c->cls.fqn.ptr);

	GET_TOKEN (moo);

	while (TOKEN_TYPE(moo) == MOO_IOTOK_SYMLIT)
	{
		lit = moo_makesymbol (moo, TOKEN_NAME_PTR(moo) + 1, TOKEN_NAME_LEN(moo) - 1);
		if (!lit || add_to_array_literal_buffer (moo, lit) <= -1) return -1;

		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) != MOO_IOTOK_ASSIGN)
		{
			set_syntax_error (moo, MOO_SYNERR_ASSIGN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);

		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_NIL:
				lit = moo->_nil;
				goto add_literal;

			case MOO_IOTOK_TRUE:
				lit = moo->_true;
				goto add_literal;

			case MOO_IOTOK_FALSE:
				lit = moo->_false;
				goto add_literal;

			case MOO_IOTOK_ERROR:
				lit = MOO_ERROR_TO_OOP(MOO_EGENERIC);
				goto add_literal;

			case MOO_IOTOK_ERRLIT:
				lit = string_to_error (moo, TOKEN_NAME(moo));
				goto add_literal;

			case MOO_IOTOK_CHARLIT:
				MOO_ASSERT (moo, TOKEN_NAME_LEN(moo) == 1);
				lit = MOO_CHAR_TO_OOP(TOKEN_NAME_PTR(moo)[0]);
				goto add_literal;

			case MOO_IOTOK_STRLIT:
				lit = moo_instantiate (moo, moo->_string, TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo));
				if (!lit) return -1;
				goto add_literal;

			case MOO_IOTOK_SYMLIT:
				lit = moo_makesymbol (moo, TOKEN_NAME_PTR(moo) + 1, TOKEN_NAME_LEN(moo) - 1);
				if (!lit) return -1;
				goto add_literal;

			case MOO_IOTOK_NUMLIT:
			case MOO_IOTOK_RADNUMLIT:
				lit = string_to_num (moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT);
				if (!lit) return -1;
				goto add_literal;

			case MOO_IOTOK_BABRACK: /* #[ - byte array literal parenthesis */
				if (read_byte_array_literal(moo, &lit) <= -1) return -1;
				goto add_literal;

			case MOO_IOTOK_APAREN: /* #( - array literal parenthesis */
				if (read_array_literal(moo, &lit) <= -1) return -1;
				goto add_literal;

			add_literal:
				/*
				 * for this definition, #pooldic MyPoolDic { #a := 10. #b := 20 },
				 * arlit_buffer contains (#a 10 #b 20) when the 'while' loop is over. */
				if (add_to_array_literal_buffer(moo, lit) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			default:
				set_syntax_error (moo, MOO_SYNERR_LITERAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
		}

		/*if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) goto done;
		else*/ if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
		{
			set_syntax_error (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}


	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		set_syntax_error (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

/*done:*/
	GET_TOKEN (moo);

	tally = moo->c->mth.arlit_count / 2;
/*TODO: tally and arlit_count range check */
	/*if (!MOO_IN_SMOOI_RANGE(tally)) ERROR??*/

	/* i use mthdic_oop[0] when compling #pooldic. it's not a real method dictionary.
	 * i just use it to avoid declaring another field into the compiler */
	moo->c->cls.pooldic_oop = moo_makedic (moo, moo->_pool_dictionary, MOO_ALIGN(tally + 10, POOL_DICTIONARY_SIZE_ALIGN));
	if (!moo->c->cls.pooldic_oop) return -1;

	for (i = 0; i < moo->c->mth.arlit_count; i += 2)
	{
		/* TODO: handle duplicate keys? */
		if (!moo_putatdic(moo, moo->c->cls.pooldic_oop, moo->c->mth.arlit[i], moo->c->mth.arlit[i + 1])) return -1;
	}

	/* eveything seems ok. register the pool dictionary to the main
	 * system dictionary or to the name space it belongs to */
	lit = moo_makesymbol (moo, moo->c->cls.name.ptr, moo->c->cls.name.len);
	if (!lit || !moo_putatdic (moo, moo->c->cls.ns_oop, lit, (moo_oop_t)moo->c->cls.pooldic_oop)) return -1;
	return 0;
}

static int compile_pooldic_definition (moo_t* moo)
{
	int n;

	/* reset the structure to hold information about a pool dictionary to be compiled.
	 * i'll be reusing some fields reserved for compling a class */
	moo->c->cls.name.len = 0;
	MOO_MEMSET (&moo->c->cls.fqn_loc, 0, MOO_SIZEOF(moo->c->cls.fqn_loc));
	moo->c->cls.pooldic_oop = MOO_NULL;
	moo->c->cls.ns_oop = MOO_NULL;
	moo->c->mth.balit_count = 0;
	moo->c->mth.arlit_count = 0;

	n = __compile_pooldic_definition (moo);

	/* reset these oops plus literal pointers not to confuse gc_compiler() */
	moo->c->cls.pooldic_oop = MOO_NULL;
	moo->c->cls.ns_oop = MOO_NULL;
	moo->c->mth.balit_count = 0;
	moo->c->mth.arlit_count = 0;

	return n;
}

static int compile_stream (moo_t* moo)
{
	GET_TOKEN (moo);

	while (TOKEN_TYPE(moo) != MOO_IOTOK_EOF)
	{
		if (is_token_symbol(moo, VOCA_INCLUDE_S))
		{
			/* #include 'xxxx' */
			GET_TOKEN (moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_STRLIT)
			{
				set_syntax_error (moo, MOO_SYNERR_STRING, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
			if (begin_include(moo) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_CLASS))
		{
			/* class Selfclass(Superclass) { } */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, 0) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_EXTEND))
		{
			/* extend Selfclass {} */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, 1) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_POOLDIC))
		{
			/* pooldic SharedPoolDic { #ABC := 20. #DEFG := 'ayz' } */
			GET_TOKEN (moo);
			if (compile_pooldic_definition(moo) <= -1) return -1;
		}
#if 0
		else if (is_token_symbol(moo, VOCA_MAIN))
		{
			/* #main */
			/* TODO: implement this */
			
		}
#endif
		else
		{
			set_syntax_error(moo, MOO_SYNERR_DIRECTIVE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
	}

	return 0;
}

static void gc_compiler (moo_t* moo)
{
	/* called when garbage collection is performed */
	if (moo->c)
	{
		moo_oow_t i;

		if (moo->c->cls.self_oop) 
			moo->c->cls.self_oop = (moo_oop_class_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.self_oop);

		if (moo->c->cls.super_oop)
			moo->c->cls.super_oop = moo_moveoop (moo, moo->c->cls.super_oop);

#ifdef MTHDIC
		if (moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE])
			moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE] = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.mthdic_oop[MOO_METHOD_INSTANCE]);

		if (moo->c->cls.mthdic_oop[MOO_METHOD_CLASS])
			moo->c->cls.mthdic_oop[MOO_METHOD_CLASS] = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.mthdic_oop[MOO_METHOD_CLASS]);
#endif

		if (moo->c->cls.pooldic_oop)
			moo->c->cls.pooldic_oop = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.pooldic_oop);

		if (moo->c->cls.ns_oop)
			moo->c->cls.ns_oop = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.ns_oop);

		if (moo->c->cls.superns_oop)
			moo->c->cls.superns_oop = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.superns_oop);

		for (i = 0; i < moo->c->cls.pooldic_count; i++)
		{
			moo->c->cls.pooldic_imp_oops[i] = (moo_oop_set_t)moo_moveoop (moo, (moo_oop_t)moo->c->cls.pooldic_imp_oops[i]);
		}

		for (i = 0; i < moo->c->mth.literal_count; i++)
		{
			moo->c->mth.literals[i] = moo_moveoop (moo, moo->c->mth.literals[i]);
		}

		for (i = 0; i < moo->c->mth.arlit_count; i++)
		{
			moo->c->mth.arlit[i] = moo_moveoop (moo, moo->c->mth.arlit[i]);
		}
	}
}

static void fini_compiler (moo_t* moo)
{
	/* called before the moo object is closed */
	if (moo->c)
	{
		moo_oow_t i;

		clear_io_names (moo);

		if (moo->c->tok.name.ptr) moo_freemem (moo, moo->c->tok.name.ptr);
		if (moo->c->cls.fqn.ptr) moo_freemem (moo, moo->c->cls.fqn.ptr);
		if (moo->c->cls.superfqn.ptr) moo_freemem (moo, moo->c->cls.superfqn.ptr);

		for (i = 0; i < MOO_COUNTOF(moo->c->cls.vars); i++)
		{
			if (moo->c->cls.vars[i].ptr) moo_freemem (moo, moo->c->cls.vars[i].ptr);
		}

		if (moo->c->cls.pooldic.ptr) moo_freemem (moo, moo->c->cls.pooldic.ptr);
		if (moo->c->cls.pooldic_imp_oops) moo_freemem (moo, moo->c->cls.pooldic_imp_oops);

		if (moo->c->mth.text.ptr) moo_freemem (moo, moo->c->mth.text.ptr);
		if (moo->c->mth.assignees.ptr) moo_freemem (moo, moo->c->mth.assignees.ptr);
		if (moo->c->mth.binsels.ptr) moo_freemem (moo, moo->c->mth.binsels.ptr);
		if (moo->c->mth.kwsels.ptr) moo_freemem (moo, moo->c->mth.kwsels.ptr);
		if (moo->c->mth.name.ptr) moo_freemem (moo, moo->c->mth.name.ptr);
		if (moo->c->mth.tmprs.ptr) moo_freemem (moo, moo->c->mth.tmprs.ptr);
		if (moo->c->mth.code.ptr) moo_freemem (moo, moo->c->mth.code.ptr);
		if (moo->c->mth.literals) moo_freemem (moo, moo->c->mth.literals);
		if (moo->c->mth.balit) moo_freemem (moo, moo->c->mth.balit);
		if (moo->c->mth.arlit) moo_freemem (moo, moo->c->mth.arlit);
		if (moo->c->mth.blk_tmprcnt) moo_freemem (moo, moo->c->mth.blk_tmprcnt);

		moo_freemem (moo, moo->c);
		moo->c = MOO_NULL;
	}
}

int moo_compile (moo_t* moo, moo_ioimpl_t io)
{
	int n;

	if (!io)
	{
		moo->errnum = MOO_EINVAL;
		return -1;
	}

	if (!moo->c)
	{
		moo_cb_t cb, * cbp;

		MOO_MEMSET (&cb, 0, MOO_SIZEOF(cb));
		cb.gc = gc_compiler;
		cb.fini = fini_compiler;
		cbp = moo_regcb (moo, &cb);
		if (!cbp) return -1;

		moo->c = moo_callocmem (moo, MOO_SIZEOF(*moo->c));
		if (!moo->c) 
		{
			moo_deregcb (moo, cbp);
			return -1;
		}

		moo->c->ilchr_ucs.ptr = &moo->c->ilchr;
		moo->c->ilchr_ucs.len = 1;
	}

	/* Some IO names could have been stored in earlier calls to this function.
	 * I clear such names before i begin this function. i don't clear it
	 * at the end of this function because i may be referenced as an error
	 * location */
	clear_io_names (moo);

	/* initialize some key fields */
	moo->c->impl = io;
	moo->c->nungots = 0;

	/* The name field and the includer field are MOO_NULL 
	 * for the main stream */
	MOO_MEMSET (&moo->c->arg, 0, MOO_SIZEOF(moo->c->arg));
	moo->c->arg.line = 1;
	moo->c->arg.colm = 1;

	/* open the top-level stream */
	n = moo->c->impl (moo, MOO_IO_OPEN, &moo->c->arg);
	if (n <= -1) return -1;

	/* the stream is open. set it as the current input stream */
	moo->c->curinp = &moo->c->arg;

	/* compile the contents of the stream */
	if (compile_stream (moo) <= -1) goto oops;

	/* close the stream */
	MOO_ASSERT (moo, moo->c->curinp == &moo->c->arg);
	moo->c->impl (moo, MOO_IO_CLOSE, moo->c->curinp);

	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (moo->c->curinp != &moo->c->arg)
	{
		moo_ioarg_t* prev;

		/* nothing much to do about a close error */
		moo->c->impl (moo, MOO_IO_CLOSE, moo->c->curinp);

		prev = moo->c->curinp->includer;
		MOO_ASSERT (moo, moo->c->curinp->name != MOO_NULL);
		MOO_MMGR_FREE (moo->mmgr, moo->c->curinp);
		moo->c->curinp = prev;
	}

	moo->c->impl (moo, MOO_IO_CLOSE, moo->c->curinp);
	return -1;
}

void moo_getsynerr (moo_t* moo, moo_synerr_t* synerr)
{
	MOO_ASSERT (moo, moo->c != MOO_NULL);
	if (synerr) *synerr = moo->c->synerr;
}
