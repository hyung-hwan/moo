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
#define NAMESPACE_SIZE 128 /* TODO: choose the right size - moo->option.dfl_sysdic_size may be too big for non-toplevel namespaces */
#define POOL_DICTIONARY_SIZE_ALIGN 128

#define INVALID_IP MOO_TYPE_MAX(moo_oow_t)

#define PFTYPE_NONE       0
#define PFTYPE_NUMBERED   1
#define PFTYPE_NAMED      2
#define PFTYPE_EXCEPTION  3
#define PFTYPE_ENSURE     4

enum class_type_t
{
	CLASS_TYPE_NORMAL = 0,
	CLASS_TYPE_EXTEND
};

enum class_mod_t
{
	CLASS_FINAL      = (1 << 0),
	CLASS_LIMITED    = (1 << 1),
	CLASS_INDEXED    = (1 << 2),
	CLASS_IMMUTABLE  = (1 << 3)
};

enum var_type_t
{
	/* == NEVER CHANGE THIS ORDER OF 3 ITEMS BELOW ==
	 * ((moo_cunit_class_t*)moo->c->cunit)->var and some iterations rely on them. */
	VAR_INSTANCE   = 0,
	VAR_CLASSINST  = 1,
	VAR_CLASS      = 2,
	/* == NEVER CHANGE THIS ORDER OF 3 ITEMS ABOVE == */

	VAR_GLOBAL,
	VAR_ARGUMENT,
	VAR_TEMPORARY,

	VAR_LITERAL /* used when compiling pooldic elements only */
};
typedef enum var_type_t var_type_t;

enum varacc_type_t
{
	VARACC_GETTER = (1 << 0),
	VARACC_SETTER = (1 << 1)
};
typedef enum varacc_type_t varacc_type_t;

struct var_info_t
{
	var_type_t            type;

	/* not used for VAR_GLOBAL */
	moo_ooi_t             pos; 

	/* useful if type is VAR_CLASS(class variable). 
	 * note it may be set to MOO_NULL to indicate the self class when
	 * the current class being compiled has not been instantiated. */
	moo_oop_class_t       cls; 

	union 
	{
		moo_oop_association_t gbl; /* used for VAR_GLOBAL only */
		moo_oop_t             lit; /* used for VAR_LITERAL only */
	} u;
};
typedef struct var_info_t var_info_t;

static struct voca_t
{
	moo_oow_t len;
	moo_ooch_t str[11];
} vocas[] = {
	{  3, { 'a','n','d'                                                   } },
	{  5, { 'b','r','e','a','k'                                           } },
	{  5, { '#','b','y','t','e'                                           } },
	{ 10, { '#','c','h','a','r','a','c','t','e','r'                       } },
	{  5, { 'c','l','a','s','s'                                           } },
	{  6, { '#','c','l','a','s','s'                                       } },
	{ 10, { '#','c','l','a','s','s','i','n','s','t'                       } },
	{  8, { 'c','o','n','t','i','n','u','e'                               } },
	{  2, { 'd','o'                                                       } },
	{  5, { '#','d','u','a','l'                                           } },
	{  4, { 'e','l','s','e'                                               } },
	{  5, { 'e','l','s','i','f'                                           } },
	{  8, { 'e','l','s','i','f','n','o','t'                               } },
	{  6, { 'e','n','s','u','r','e',                                      } },
	{  9, { 'e','x','c','e','p','t','i','o','n'                           } },
	{  6, { 'e','x','t','e','n','d'                                       } },
	{  5, { 'f','a','l','s','e'                                           } },
	{  6, { '#','f','i','n','a','l'                                       } },
	{  4, { 'f','r','o','m'                                               } },
	{  4, { '#','g','e','t'                                               } },
	{  9, { '#','h','a','l','f','w','o','r','d'                           } },
	{  2, { 'i','f'                                                       } },
	{  5, { 'i','f','n','o','t'                                           } },
	{ 10, { '#','i','m','m','u','t','a','b','l','e'                       } },
	{  6, { 'i','m','p','o','r','t'                                       } },
	{  8, { '#','i','n','c','l','u','d','e'                               } },
	{  9, { 'i','n','t','e','r','f','a','c','e'                           } },
	{  8, { '#','l','e','n','i','e','n','t'                               } },
	{  8, { '#','l','i','b','e','r','a','l'                               } },
	{  8, { '#','l','i','m','i','t','e','d'                               } },
	{  7, { '#','l','i','w','o','r','d'                                   } },
	{  6, { 'm','e','t','h','o','d'                                       } },
	{  3, { 'n','i','l'                                                   } },
	{  3, { 'o','f','f'                                                   } },
	{  2, { 'o','n'                                                       } },
	{  2, { 'o','r'                                                       } },
	{  8, { '#','p','o','i','n','t','e','r'                               } },
	{  7, { 'p','o','o','l','d','i','c'                                   } },
	{  8, { '#','p','o','o','l','d','i','c'                               } },
	{  7, { '#','p','r','a','g','m','a'                                   } },
	{ 10, { 'p','r','i','m','i','t','i','v','e',':'                       } },
	{ 10, { '#','p','r','i','m','i','t','i','v','e'                       } },
	{  2, { 'q','c'                                                       } },
	{  4, { 's','e','l','f'                                               } },
	{  6, { 's','e','l','f','n','s'                                       } },
	{  4, { '#','s','e','t'                                               } },
	{  5, { 's','u','p','e','r'                                           } },
	{ 11, { 't','h','i','s','C','o','n','t','e','x','t'                   } },
	{ 11, { 't','h','i','s','P','r','o','c','e','s','s'                   } },
	{  4, { 't','r','u','e'                                               } },
	{  5, { 'u','n','t','i','l'                                           } },
	{  3, { 'v','a','r'                                                   } },
	{  8, { 'v','a','r','i','a','b','l','e'                               } },
	{  9, { '#','v','a','r','i','a','d','i','c'                           } },
	{  5, { 'w','h','i','l','e'                                           } },
	{  5, { '#','w','o','r','d'                                           } },

	{  1, { '|'                                                           } },
	{  2, { '|','+'                                                       } },
	{  2, { '|','*'                                                       } },
	{  1, { '>'                                                           } },
	{  1, { '<'                                                           } },

	{  5, { '<','E','O','F','>'                                           } }
};

enum voca_id_t
{
	VOCA_AND,
	VOCA_BREAK,
	VOCA_BYTE_S,
	VOCA_CHARACTER_S,
	VOCA_CLASS,
	VOCA_CLASS_S,
	VOCA_CLASSINST_S,
	VOCA_CONTINUE,
	VOCA_DO,
	VOCA_DUAL_S,
	VOCA_ELSE,
	VOCA_ELSIF,
	VOCA_ELSIFNOT,
	VOCA_ENSURE,
	VOCA_EXCEPTION,
	VOCA_EXTEND,
	VOCA_FALSE,
	VOCA_FINAL_S,
	VOCA_FROM,
	VOCA_GET_S,
	VOCA_HALFWORD_S,
	VOCA_IF,
	VOCA_IFNOT,
	VOCA_IMMUTABLE_S,
	VOCA_IMPORT,
	VOCA_INCLUDE_S,
	VOCA_INTERFACE,
	VOCA_LENIENT_S,
	VOCA_LIBERAL_S,
	VOCA_LIMITED_S,
	VOCA_LIWORD_S,
	VOCA_METHOD,
	VOCA_NIL,
	VOCA_OFF,
	VOCA_ON,
	VOCA_OR,
	VOCA_POINTER_S,
	VOCA_POOLDIC,
	VOCA_POOLDIC_S,
	VOCA_PRAGMA_S,
	VOCA_PRIMITIVE_COLON,
	VOCA_PRIMITIVE_S,
	VOCA_QC,
	VOCA_SELF,
	VOCA_SELFNS,
	VOCA_SET_S,
	VOCA_SUPER,
	VOCA_THIS_CONTEXT,
	VOCA_THIS_PROCESS,
	VOCA_TRUE,
	VOCA_UNTIL,
	VOCA_VAR,
	VOCA_VARIABLE,
	VOCA_VARIADIC_S,
	VOCA_WHILE,
	VOCA_WORD_S,

	VOCA_VBAR,
	VOCA_VBAR_PLUS,
	VOCA_VBAR_ASTER,
	VOCA_GT,
	VOCA_LT,

	VOCA_EOF
};
typedef enum voca_id_t voca_id_t;

static int compile_pooldic_definition (moo_t* moo);
static int compile_interface_definition (moo_t* moo);
static int compile_class_definition (moo_t* moo, int class_type);

static int compile_block_statement (moo_t* moo);
static int compile_method_statement (moo_t* moo);
static int compile_method_expression (moo_t* moo, int pop);
static MOO_INLINE int add_literal (moo_t* moo, moo_oop_t lit, moo_oow_t* index);
static moo_oop_t token_to_literal (moo_t* moo, int rdonly);
static moo_oop_t find_element_in_compiling_pooldic (moo_t* moo, const moo_oocs_t* name);

static void gc_cunit_chain (moo_t* moo);
static moo_cunit_t* push_cunit (moo_t* moo, moo_cunit_type_t type);
static void pop_cunit (moo_t* moo);

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

static MOO_INLINE int is_xdigitchar (moo_ooci_t c)
{
/* TODO: support full unicode */
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
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
	 *   '&' | '*' | '+' | '-' | '/' |
	 *   '<' | '>' | '=' | '@' | '~' | '|'
	 *
	 * - a comma is special in moo and doesn't form a binary selector.
	 * - a percent sign is special in moo and doesn't form a binary selector.
	 * - an exclamation mark is excluded intentioinally because i can't tell
	 *   the method symbol #! from the comment introducer #!.
	 * - a question mark forms a normal identifier.
	 * - a backslash is used to form an error literal combined with a hash sign. (e.g. #\E10)
	 */

	switch (c)
	{
		case '&':
		case '*':
		case '+':
		case '-':
		case '/': 
		case '<':
		case '>':
		case '=':
		case '@':
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
	return is_alnumchar(c) || c == '_' || c == '?';
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
}VOCA_ERROR,
#endif

static MOO_INLINE int is_word (const moo_oocs_t* oocs, voca_id_t id)
{
	return oocs->len == vocas[id].len && 
	       moo_equal_oochars(oocs->ptr, vocas[id].str, vocas[id].len);
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
		VOCA_THIS_CONTEXT,
		VOCA_THIS_PROCESS,
		VOCA_SELFNS,
		VOCA_IF,
		VOCA_ELSE,
		VOCA_ELSIF,
		VOCA_ELSIFNOT,
		VOCA_WHILE,
		VOCA_UNTIL,
		VOCA_DO,
		VOCA_BREAK,
		VOCA_CONTINUE
	};
	int i;

	for (i = 0; i < MOO_COUNTOF(rw); i++)
	{
		if (is_word(ucs, rw[i])) return 1;
	}

	return 0;
}

#if 0
static int is_restricted_word (const moo_oocs_t* ucs)
{
	/* not fully reserved. but restricted in a certain context */

	static int rw[] = 
	{
		VOCA_CLASS,
		VOCA_EXTEND,
		VOCA_FROM,
		VOCA_IMPORT,
		VOCA_METHOD,
		VOCA_POOLDIC,
		VOCA_VAR,
		VOCA_VARIABLE
	};
	int i;

	for (i = 0; i < MOO_COUNTOF(rw); i++)
	{
		if (is_word(ucs, rw[i])) return 1;
	}

	return 0;
}
#endif

static int begin_include (moo_t* moo);
static int end_include (moo_t* moo);

static int copy_string_to (moo_t* moo, const moo_oocs_t* src, moo_oocs_t* dst, moo_oow_t* dst_capa, int append, moo_ooch_t delim_char)
{
	moo_oow_t len, pos;

	if (append)
	{
		pos = dst->len;
		len = dst->len + src->len;
		if (delim_char != '\0') len++;
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

		tmp = moo_reallocmem(moo, dst->ptr, MOO_SIZEOF(*tmp) * capa);
		if (!tmp)  return -1;

		dst->ptr = tmp;
		*dst_capa = capa;
	}

	if (append && delim_char != '\0') dst->ptr[pos++] = delim_char;
	moo_copy_oochars (&dst->ptr[pos], src->ptr, src->len);
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

static int fetch_word_from_string (const moo_oocs_t* haystack, moo_oow_t xindex, moo_oocs_t* str)
{
	moo_ooch_t* t, * e, * ss;
	moo_oow_t index;

	t = haystack->ptr;
	e = t + haystack->len;
	index = 0;

	while (t < e)
	{
		while (t < e && is_spacechar(*t)) t++;

		ss = t;
		while (t < e && !is_spacechar(*t)) t++;

		if (xindex == index)
		{
			str->ptr = ss;
			str->len = t - ss;
			return 0;
		}

		index++;
	}

	return -1;
}


static int add_oop_to_oopbuf (moo_t* moo, moo_oopbuf_t* oopbuf, moo_oop_t item)
{
	if (oopbuf->count >= oopbuf->capa)
	{
		moo_oop_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (oopbuf->count + 1, ARLIT_BUFFER_ALIGN);
		tmp = (moo_oop_t*)moo_reallocmem(moo, oopbuf->ptr, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		oopbuf->capa = new_capa;
		oopbuf->ptr = tmp;
	}

/* TODO: overflow check of oopbuf->count itself */
	oopbuf->ptr[oopbuf->count++] = item;
	return 0;
}

static int add_oop_to_oopbuf_nodup (moo_t* moo, moo_oopbuf_t* oopbuf, moo_oop_t item, moo_oow_t* index)
{
	moo_oow_t i;

	for (i = 0; i < oopbuf->count; i++) 
	{
		if (oopbuf->ptr[i] == item) 
		{
			*index = i;
			return 0;
		}
	}

	if (add_oop_to_oopbuf(moo, oopbuf, item) <= -1) return -1;
	*index = oopbuf->count - 1;
	return 0;
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
			moo_seterrnum (moo, MOO_ERANGE);
			return -1;
		}
		old_value = value;
		ptr++;
	}

	if (ptr < end)
	{
		/* trailing garbage? */
		moo_seterrnum (moo, MOO_EINVAL);
		return -1;
	}

	MOO_ASSERT (moo, -MOO_SMOOI_MAX == MOO_SMOOI_MIN);
	if (value > MOO_SMOOI_MAX) 
	{
		moo_seterrnum (moo, MOO_ERANGE);
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

static moo_oop_t string_to_error (moo_t* moo, moo_oocs_t* str, moo_ioloc_t* loc)
{
	moo_ooi_t num = 0;
	const moo_ooch_t* ptr, * end;

	ptr = str->ptr,
	end = str->ptr + str->len;

	/* i assume that the input is in the form of \#ENNN
	 * all other letters are non-digits except the NNN part.
	 * i just skip all non-digit letters for simplicity sake. */
	while (ptr < end)
	{
		if (is_digitchar(*ptr)) 
		{
			moo_oow_t xnum;

			xnum = num * 10 + (*ptr - '0');
			if (xnum < num || xnum > MOO_ERROR_MAX) 
			{
				/* overflowed */
				moo_setsynerr (moo, MOO_SYNERR_ERRLITINVAL, loc, str);
				return MOO_NULL;
			}
			num = xnum;
		}
		ptr++;
	}

	return MOO_ERROR_TO_OOP(num);
}

static moo_oop_t string_to_smptr (moo_t* moo, moo_oocs_t* str, moo_ioloc_t* loc)
{
	moo_oow_t num = 0;
	const moo_ooch_t* ptr, * end;

	ptr = str->ptr,
	end = str->ptr + str->len;

	/* i assume that the input is in the form of \#PNNN
	 * all other letters are non-xdigits except the NNN part.
	 * i just skip all non-digit letters for simplicity sake. */
	while (ptr < end)
	{
		if (is_xdigitchar(*ptr)) 
		{
			moo_oow_t xnum;

			xnum = num * 16;
			if (*ptr >= 'a' && *ptr <= 'f') xnum += (*ptr - 'a' + 10);
			else if (*ptr >= 'A' && *ptr <= 'F') xnum += (*ptr - 'A' + 10);
			else xnum += (*ptr - '0');

			if (xnum < num)
			{
				/* overflowed */
				moo_setsynerr (moo, MOO_SYNERR_SMPTRLITINVAL, loc, str);
				return MOO_NULL;
			}
			num = xnum;
		}
		ptr++;
	}

	if (!MOO_IN_SMPTR_RANGE(num))
	{
		moo_setsynerr (moo, MOO_SYNERR_SMPTRLITINVAL, loc, str);
		return MOO_NULL;
	}

	return MOO_SMPTR_TO_OOP(num);
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

#define GET_TOKEN_RETURN(moo,fail_ret) \
	do { if (get_token(moo) <= -1) return fail_ret; } while (0)

#define GET_TOKEN_GOTO(moo,fail_label) \
	do { if (get_token(moo) <= -1) goto fail_label; } while (0)

#define ADD_TOKEN_STR(moo,s,l) \
	do { if (add_token_str(moo, s, l) <= -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(moo,c) \
	do { if (add_token_char(moo, (moo_ooch_t)c) <= -1) return -1; } while (0)

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
	       moo_equal_oochars(TOKEN_NAME_PTR(moo), vocas[id].str, vocas[id].len);
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
	moo_ooci_t lc;

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
		n = moo->c->impl(moo, MOO_IO_READ, moo->c->curinp);
		if (n <= -1) return -1;
		
		if (n == 0)
		{
		return_eof:
			moo->c->curinp->lxc.c = MOO_OOCI_EOF;
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

	if ((moo->c->pragma_flags & MOO_PRAGMA_QC) && c == '"')
	{
		/* skip up to the closing " */
		do 
		{
			GET_CHAR_TO (moo, c); 
			if (c == MOO_OOCI_EOF) goto unterminated;
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
			if (c == MOO_OOCI_EOF) goto unterminated;

			if (c == '*')
			{
			check_rparen:
				GET_CHAR_TO (moo, c);
				if (c == MOO_OOCI_EOF) goto unterminated;

				if (c == '*') goto check_rparen; /* got another * after * */
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
			if (c == MOO_OOCI_EOF)
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
	moo_setsynerr (moo, MOO_SYNERR_CMTNC, LEXER_LOC(moo), MOO_NULL);
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

	if (char_read_ahead != MOO_OOCI_EOF)
	{
		ADD_TOKEN_CHAR (moo, char_read_ahead);
	}

	/* while() instead of do..while() because when char_read_ahead is not EOF
	 * c may not be a identifier character */
	while (is_identchar(c))
	{
		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR_TO (moo, c);
	} 

	if (c == ':') 
	{
#if 0
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
				moo_setsynerr (moo, MOO_SYNERR_COLON, LEXER_LOC(moo), MOO_NULL);
				return -1;
			}
		}
		else
		{
			unget_char (moo, &moo->c->lxc); 
		}
#else
		moo_iolxc_t lc = moo->c->lxc;

		GET_CHAR_TO (moo, c);

		if (c == '=' || c == '{')
		{
			/* := or :{ appeared after an identifier */
			unget_char (moo, &moo->c->lxc); 
			unget_char (moo, &lc);
		}
		else
		{
			ADD_TOKEN_CHAR (moo, lc.c);
			SET_TOKEN_TYPE (moo, MOO_IOTOK_KEYWORD);
			unget_char (moo, &moo->c->lxc); 
		}
#endif
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
				unget_char (moo, &moo->c->lxc); 
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
		else if (is_token_word(moo, VOCA_THIS_CONTEXT))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_THIS_CONTEXT);
		}
		else if (is_token_word(moo, VOCA_THIS_PROCESS))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_THIS_PROCESS);
		}
		else if (is_token_word(moo, VOCA_SELFNS))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_SELFNS);
		}
		else if (is_token_word(moo, VOCA_IF))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_IF);
		}
		else if (is_token_word(moo, VOCA_IFNOT))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_IFNOT);
		}
		else if (is_token_word(moo, VOCA_ELSE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ELSE);
		}
		else if (is_token_word(moo, VOCA_ELSIF))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ELSIF);
		}
		else if (is_token_word(moo, VOCA_ELSIFNOT))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_ELSIFNOT);
		}
		else if (is_token_word(moo, VOCA_WHILE))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_WHILE);
		}
		else if (is_token_word(moo, VOCA_UNTIL))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_UNTIL);
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
		else if (is_token_word(moo, VOCA_AND))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_AND);
		}
		else if (is_token_word(moo, VOCA_OR))
		{
			SET_TOKEN_TYPE (moo, MOO_IOTOK_OR);
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
	moo_oow_t radix = 0;
	int radix_overflowed = 0;

	c = moo->c->lxc.c;
	SET_TOKEN_TYPE (moo, MOO_IOTOK_NUMLIT);

/*TODO: support a complex numeric literal */
	do 
	{
		/* collect the potential radix specifier */
		if (!radix_overflowed)
		{
			int r;
			moo_oow_t rv;

			r = CHAR_TO_NUM(c, 10);
			MOO_ASSERT (moo, r < 10);
			rv = radix * 10 + r;
			if (rv < radix) radix_overflowed = 1;
			radix = rv;
		}

		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR_TO (moo, c);
		if (c == '_')
		{
			/* i allow digit separation with _  as in 123_456. */
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

		if (radix_overflowed || radix < 2 || radix > 36)
		{
			/* radix too big */
			moo_setsynerr (moo, MOO_SYNERR_RADIXINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		ADD_TOKEN_CHAR (moo, c);
		GET_CHAR_TO (moo, c);

		if (CHAR_TO_NUM(c, radix) >= radix)
		{
			/* no digit after the radix specifier */
			moo_setsynerr (moo, MOO_SYNERR_RADNUMLITINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		do
		{
			ADD_TOKEN_CHAR (moo, c);
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
		unget_char (moo, &moo->c->lxc);
	}
	else
	{
		unget_char (moo, &moo->c->lxc);
	}

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
	if (c == MOO_OOCI_EOF) 
	{
		moo_setsynerr (moo, MOO_SYNERR_CLTNT, LEXER_LOC(moo), MOO_NULL);
		return -1;
	}

	SET_TOKEN_TYPE (moo, MOO_IOTOK_CHARLIT);
	ADD_TOKEN_CHAR (moo, c);
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

	moo_ooci_t oc, c;

	oc = moo->c->lxc.c; /* opening quote */

	SET_TOKEN_TYPE (moo, MOO_IOTOK_STRLIT);
	GET_CHAR_TO (moo, c);

	if (c != oc)
	{
		do 
		{
			do 
			{
			in_strlit:
				ADD_TOKEN_CHAR (moo, c);
				GET_CHAR_TO (moo, c);

				if (c == MOO_OOCI_EOF) 
				{
					/* string not closed */
					moo_setsynerr (moo, MOO_SYNERR_STRNC, TOKEN_LOC(moo) /*&moo->c->lxc.l*/, MOO_NULL);
					return -1;
				}
			} 
			while (c != oc);

			/* 'c' must be a single quote at this point*/
			GET_CHAR_TO (moo, c);
		} 
		while (c == oc); /* if the next character is a single quote, it becomes a literal single quote character. */
	}
	else
	{
		GET_CHAR_TO (moo, c);
		if (c == oc) goto in_strlit;
	}

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

		if (c == MOO_OOCI_EOF)
		{
			moo_setsynerr (moo, MOO_SYNERR_STRNC, TOKEN_LOC(moo) /*&moo->c->lxc.l*/, MOO_NULL);
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
		case MOO_OOCI_EOF:
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
			else
			{
				unget_char (moo, &moo->c->lxc);
			}
			break;

		case '^':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_RETURN);
			ADD_TOKEN_CHAR (moo, c);
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

		case '%':
			SET_TOKEN_TYPE (moo, MOO_IOTOK_PERCENT);
			ADD_TOKEN_CHAR (moo, c);
			GET_CHAR_TO (moo, c);

			if (c == '(')
			{
				/* %( - array expression */
				ADD_TOKEN_CHAR (moo, c);
				SET_TOKEN_TYPE (moo, MOO_IOTOK_PERCPAREN);
			}
			else if (c == '[')
			{
				/* %[ - byte-array expression */
				ADD_TOKEN_CHAR (moo, c);
				SET_TOKEN_TYPE (moo, MOO_IOTOK_PERCBRACK);
			}
			else if (c == '{')
			{
				/* %{ - dictionary expression */
				ADD_TOKEN_CHAR (moo, c);
				SET_TOKEN_TYPE (moo, MOO_IOTOK_PERCBRACE);
			}
			else
			{
				/* NOTE the percent sign not followed by  ( or } is 
				 *      meaningless at this moment. however, i return
				 *      it as a token so that the compiler anyway 
				 *      will fail eventually */
				unget_char (moo, &moo->c->lxc);
			}
			break;

		case '#':  
			ADD_TOKEN_CHAR (moo, c);
			GET_CHAR_TO (moo, c);
			switch (c)
			{
				case MOO_OOCI_EOF:
					moo_setsynerr (moo, MOO_SYNERR_HLTNT, LEXER_LOC(moo), MOO_NULL);
					return -1;

				case '(':
					/* #( - array literal */
					ADD_TOKEN_CHAR (moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_HASHPAREN);
					break;

				case '[':
					/* #[ - byte array literal */
					ADD_TOKEN_CHAR (moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_HASHBRACK);
					break;

				case '{':
					/* #[ - dictionary literal */
					ADD_TOKEN_CHAR (moo, c);
					SET_TOKEN_TYPE (moo, MOO_IOTOK_HASHBRACE);
					break;

				case '\'':
					/* quoted symbol literal */
					if (get_strlit(moo) <= -1) return -1; /* reuse the string literal tokenizer */
					SET_TOKEN_TYPE (moo, MOO_IOTOK_SYMLIT); /* change the symbol type to symbol */
					break;

				case '\\':
					ADD_TOKEN_CHAR (moo, c);
					GET_CHAR_TO (moo, c);
					if (c == 'E' || c == 'e')
					{
						/* #\eNNN - error literal - #\e0, #\e1234, etc */
						SET_TOKEN_TYPE(moo, MOO_IOTOK_ERRLIT);
						ADD_TOKEN_CHAR (moo, c);
						GET_CHAR_TO (moo, c);
						if (!is_digitchar(c))
						{
							ADD_TOKEN_CHAR (moo, c); /* to include it to the error messsage */
							moo_setsynerr (moo, MOO_SYNERR_ERRLITINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
							return -1;
						}
						do
						{
							ADD_TOKEN_CHAR (moo, c);
							GET_CHAR_TO (moo, c);
						}
						while (is_digitchar(c) || c == '_');
					}
					else if (c == 'P' || c == 'p')
					{
						/* #\pXXX - smptr literal - #\p0, #\p100, etc */
						SET_TOKEN_TYPE(moo, MOO_IOTOK_SMPTRLIT);
						ADD_TOKEN_CHAR (moo, c);
						GET_CHAR_TO (moo, c);

						if (!is_xdigitchar(c))
						{
							ADD_TOKEN_CHAR (moo, c); /* to include it to the error messsage */
							moo_setsynerr (moo, MOO_SYNERR_SMPTRLITINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
							return -1;
						}
						do
						{
							ADD_TOKEN_CHAR (moo, c);
							GET_CHAR_TO (moo, c);
						}
						while (is_xdigitchar(c) || c == '_');
					}
					else
					{
						ADD_TOKEN_CHAR (moo, c); /* to include it to the error messsage */
						moo_setsynerr (moo, MOO_SYNERR_HBSLPINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					unget_char (moo, &moo->c->lxc);
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
									moo_setsynerr (moo, MOO_SYNERR_COLON, LEXER_LOC(moo), MOO_NULL);
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
						moo_setsynerr (moo, MOO_SYNERR_HLTNT, LEXER_LOC(moo), MOO_NULL);
						return -1;
					}

					SET_TOKEN_TYPE (moo, MOO_IOTOK_SYMLIT);
					break;
			}

			break;

		case '"':
			/* if MOO_PRAGMA_QC is set, this part should never be reached */
			MOO_ASSERT (moo, !(moo->c->pragma_flags & MOO_PRAGMA_QC));
			if (get_string(moo, '"', '\\', 0, 0) <= -1) return -1;
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
						moo_setsynerr (moo, MOO_SYNERR_CHARLITINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
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
				if (get_ident(moo, MOO_OOCI_EOF) <= -1) return -1;
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
				moo_setsynerr (moo, MOO_SYNERR_ILCHR, LEXER_LOC(moo), &moo->c->ilchr_ucs);
				return -1;
			}
			break;

		single_char_token:
			ADD_TOKEN_CHAR (moo, c);
			break;
	}

#if defined(MOO_DEBUG_LEXER)
	MOO_DEBUG3 (moo, "TOKEN: [%.*js] %d\n", (moo_ooi_t)moo->c->tok.name.len, moo->c->tok.name.ptr, (int)moo->c->tok.type);
#endif

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

	moo_copy_oochars (ptr, name->ptr, name->len);
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
		moo_setsynerr (moo, MOO_SYNERR_INCLUDE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

	GET_TOKEN (moo);
	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		/* check if a period is following the includee name */
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	/* the context object has the ip field. it should be representable
	 * in a small integer. for simplicity, limit the total byte code length
	 * to fit in a small integer. because 'ip' points to the next instruction
	 * to execute, he upper bound should be (max - 1) so that i stays
	 * at the max when incremented */
	if (cc->mth.code.len == MOO_SMOOI_MAX - 1)
	{
		moo_seterrnum (moo, MOO_EBCFULL); /* byte code too big */
		return -1;
	}

	if (cc->mth.code.len >= cc->mth.code_capa)
	{
		moo_oob_t* tmp;
		moo_oow_t newcapa;

		newcapa = MOO_ALIGN (cc->mth.code.len + 1, CODE_BUFFER_ALIGN);

		tmp = moo_reallocmem (moo, cc->mth.code.ptr, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		cc->mth.code.ptr = tmp;
		cc->mth.code_capa = newcapa;
	}

	cc->mth.code.ptr[cc->mth.code.len++] = code;
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
		case BCODE_JUMPOP_BACKWARD_IF_FALSE_0:
		case BCODE_JUMPOP_BACKWARD_IF_TRUE_0:
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

		case BCODE_JUMP_BACKWARD_X:
		case BCODE_JUMPOP_BACKWARD_IF_FALSE_X:
		case BCODE_JUMPOP_BACKWARD_IF_TRUE_X:
		case BCODE_JUMP_FORWARD_X:
		case BCODE_JUMP_FORWARD_IF_TRUE:
		case BCODE_JUMP_FORWARD_IF_FALSE:
		case BCODE_JUMPOP_FORWARD_IF_FALSE:
		case BCODE_JUMPOP_FORWARD_IF_TRUE:
			if (param_1 > MAX_CODE_JUMP)
			{
				cmd = cmd + 1; /* convert to a JUMP2 instruction */
				param_1 = param_1 - MAX_CODE_JUMP;
			}
			/* fall thru */
		case BCODE_JUMP2_FORWARD:
		case BCODE_JUMP2_FORWARD_IF_TRUE:
		case BCODE_JUMP2_FORWARD_IF_FALSE:
		case BCODE_JUMP2_BACKWARD:
		case BCODE_JUMPOP2_BACKWARD_IF_FALSE:
		case BCODE_JUMPOP2_BACKWARD_IF_TRUE:
		case BCODE_JUMPOP2_FORWARD_IF_FALSE:
		case BCODE_JUMPOP2_FORWARD_IF_TRUE:
		case BCODE_PUSH_INTLIT:
		case BCODE_PUSH_NEGINTLIT:
		case BCODE_PUSH_CHARLIT:
		case BCODE_MAKE_DICTIONARY:
		case BCODE_MAKE_ARRAY:
		case BCODE_POP_INTO_ARRAY:
		case BCODE_MAKE_BYTEARRAY:
		case BCODE_POP_INTO_BYTEARRAY:
			bc = cmd;
			goto write_long;
	}

	MOO_DEBUG1 (moo, "Invalid single param instruction opcode %d\n", (int)cmd);
	moo_seterrnum (moo, MOO_EINVAL);
	return -1;

write_short:
	if (emit_byte_instruction(moo, bc) <= -1) return -1;
	return 0;

write_long:
	if (param_1 > MAX_CODE_PARAM) 
	{
		moo_seterrnum (moo, MOO_ERANGE);
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
	moo_seterrnum (moo, MOO_EINVAL);
	return -1;

write_short:
	if (emit_byte_instruction(moo, bc) <= -1 ||
	    emit_byte_instruction(moo, param_2) <= -1) return -1;
	return 0;

write_long:
	if (param_1 > MAX_CODE_PARAM || param_2 > MAX_CODE_PARAM) 
	{
		moo_seterrnum (moo, MOO_ERANGE);
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
	                 cmd == BCODE_JUMPOP_BACKWARD_IF_FALSE_0 ||
	                 cmd == BCODE_JUMPOP_BACKWARD_IF_TRUE_0);
	
	/* the short BCODE_JUMP_BACKWARD instructions use low 2 bits to encode 
	 * the jump offset. so it can encode 0, 1, 2, 3. the instruction itself 
	 * is 1 byte long. the offset value of 0, 1, 2 can get encoded into the
	 * instruction, which result in 1, 2, 3 when combined with the length 1
	 * of the instruction itself */

	adj = (offset < 3)? 1: (MOO_BCODE_LONG_PARAM_SIZE + 1);
	return emit_single_param_instruction (moo, cmd, offset + adj);
}

static int patch_long_forward_jump_instruction (moo_t* moo, moo_oow_t jip, moo_oow_t jt, moo_ioloc_t* errloc)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
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
		moo_setsynerr (moo, MOO_SYNERR_BLKFLOOD, errloc, MOO_NULL); 
		return -1;
	}

	MOO_ASSERT (moo, cc->mth.code.ptr[jip] == BCODE_JUMP_FORWARD_X ||
	                 cc->mth.code.ptr[jip] == BCODE_JUMP_FORWARD_IF_FALSE ||
	                 cc->mth.code.ptr[jip] == BCODE_JUMP_FORWARD_IF_TRUE ||
	                 cc->mth.code.ptr[jip] == BCODE_JUMPOP_FORWARD_IF_FALSE ||
	                 cc->mth.code.ptr[jip] == BCODE_JUMPOP_FORWARD_IF_TRUE);

	if (code_size > MAX_CODE_JUMP)
	{
		/* switch to JUMP2 instruction to allow a bigger jump offset.
		 * up to twice MAX_CODE_JUMP only */
		cc->mth.code.ptr[jip]++; /* switch to the JUMP2 instruction */
		jump_offset = code_size - MAX_CODE_JUMP;
	}
	else
	{
		jump_offset = code_size;
	}

#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
	cc->mth.code.ptr[jip + 1] = jump_offset >> 8;
	cc->mth.code.ptr[jip + 2] = jump_offset & 0xFF;
#else
	cc->mth.code.ptr[jip + 1] = jump_offset;
#endif

	return 0;
}

static int push_loop (moo_t* moo, moo_loop_type_t type, moo_oow_t startpos)
{
	moo_loop_t* loop;

	loop = moo_callocmem(moo, MOO_SIZEOF(*loop));
	if (!loop) return -1;

	init_oow_pool (moo, &loop->break_ip_pool);
	init_oow_pool (moo, &loop->continue_ip_pool);
	loop->type = type;
	loop->startpos = startpos;

	/* link the new loop to the loop chain of the current method being compiled */
	MOO_ASSERT (moo, moo->c->cunit && moo->c->cunit->cunit_type == MOO_CUNIT_CLASS);
	loop->next = ((moo_cunit_class_t*)moo->c->cunit)->mth.loop;
	((moo_cunit_class_t*)moo->c->cunit)->mth.loop = loop;

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
			    patch_long_forward_jump_instruction(moo, chunk->buf[j], jt, MOO_NULL) <= -1) return -1;
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
				else if (chunk->buf[j] > end && chunk->buf[j] < ((moo_cunit_class_t*)moo->c->cunit)->mth.code.len)
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
	return update_loop_jumps(moo, &((moo_cunit_class_t*)moo->c->cunit)->mth.loop->break_ip_pool, jt);
}

static MOO_INLINE int update_loop_continues (moo_t* moo, moo_oow_t jt)
{
	return update_loop_jumps(moo, &((moo_cunit_class_t*)moo->c->cunit)->mth.loop->continue_ip_pool, jt);
}

static MOO_INLINE void adjust_all_loop_jumps_for_elimination (moo_t* moo, moo_oow_t start, moo_oow_t end)
{
	moo_loop_t* loop;

	loop = ((moo_cunit_class_t*)moo->c->cunit)->mth.loop;
	while (loop)
	{
		adjust_loop_jumps_for_elimination (moo, &loop->break_ip_pool, start, end);
		adjust_loop_jumps_for_elimination (moo, &loop->continue_ip_pool, start, end);
		loop = loop->next;
	}
}

static MOO_INLINE moo_loop_t* unlink_loop (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_loop_t* loop;

	MOO_ASSERT (moo, cc->mth.loop != MOO_NULL);
	loop = cc->mth.loop;
	cc->mth.loop = loop->next;

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
	free_loop (moo, unlink_loop(moo));
}

static MOO_INLINE int inject_break_to_loop (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	if (add_to_oow_pool(moo, &cc->mth.loop->break_ip_pool, cc->mth.code.len) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) return -1;
	return 0;
}

static MOO_INLINE int inject_continue_to_loop (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	/* used for a do-while loop. jump forward because the conditional 
	 * is at the end of the do-while loop */
	if (add_to_oow_pool(moo, &cc->mth.loop->continue_ip_pool, cc->mth.code.len) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) return -1;
	return 0;
}

static void eliminate_instructions (moo_t* moo, moo_oow_t start, moo_oow_t end)
{
	moo_oow_t last;
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	MOO_ASSERT (moo, cc->mth.code.len >= 1);

	last = cc->mth.code.len - 1;

	if (end >= last)
	{
		/* eliminate all instructions starting from the start index.
		 * setting the length to the start length will achieve this */
		adjust_all_loop_jumps_for_elimination (moo, start, last);
		cc->mth.code.len = start;
	}
	else
	{
		/* eliminate a chunk in the middle of the instruction buffer.
		 * some copying is required */
		adjust_all_loop_jumps_for_elimination (moo, start, end);
		MOO_MEMMOVE (&cc->mth.code.ptr[start], &cc->mth.code.ptr[end + 1], cc->mth.code.len - end - 1);
		cc->mth.code.len -= end - start + 1;
	}
}

/* ---------------------------------------------------------------------
 * Compiler
 * --------------------------------------------------------------------- */

static MOO_INLINE int add_literal (moo_t* moo, moo_oop_t lit, moo_oow_t* index)
{
#if 0
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t i;

	for (i = 0; i < cc->mth.literals.count; i++) 
	{
		/* 
		 * this removes redundancy of symbols, characters, and small integers. 
		 * more complex redundacy check may be done somewhere else like 
		 * in add_string_literal().
		 */
		if (cc->mth.literals.ptr[i] == lit) 
		{
			*index = i;
			return 0;
		}
	}

	if (add_oop_to_oopbuf(moo, &cc->mth.literals, lit) <= -1) return -1;
	*index = cc->mth.literals.count - 1;
	return 0;
#else
	/* 
	 * this removes redundancy of symbols, characters, and small integers. 
	 * more complex redundacy check may be done somewhere else like 
	 * in add_string_literal().
	 */
	return add_oop_to_oopbuf_nodup(moo, &((moo_cunit_class_t*)moo->c->cunit)->mth.literals, lit, index);
#endif
}

static int add_string_literal (moo_t* moo, const moo_oocs_t* str, moo_oow_t* index)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oop_t lit;
	moo_oow_t i;

	for (i = 0; i < cc->mth.literals.count; i++) 
	{
		lit = cc->mth.literals.ptr[i];

		if (MOO_CLASSOF(moo, lit) == moo->_string && 
		    MOO_OBJ_GET_SIZE(lit) == str->len &&
		    moo_equal_oochars(((moo_oop_char_t)lit)->slot, str->ptr, str->len)) 
		{
			*index = i;
			return 0;
		}
	}

	lit = moo_instantiate(moo, moo->_string, str->ptr, str->len);
	if (!lit) return -1;

	return add_literal(moo, lit, index);
}

static int add_symbol_literal (moo_t* moo, const moo_oocs_t* str, moo_oow_t offset, moo_oow_t* index)
{
	moo_oop_t tmp;

	tmp = moo_makesymbol(moo, str->ptr + offset, str->len - offset);
	if (!tmp) return -1;

	return add_literal(moo, tmp, index);
}

static MOO_INLINE int set_class_fqn (moo_t* moo, moo_cunit_class_t* cc, const moo_oocs_t* name)
{
	if (copy_string_to(moo, name, &cc->fqn, &cc->fqn_capa, 0, '\0') <= -1) return -1;
	cc->name = cc->fqn;
	return 0;
}

static MOO_INLINE int set_superclass_fqn (moo_t* moo, moo_cunit_class_t* cc, const moo_oocs_t* name)
{
	if (copy_string_to(moo, name, &cc->superfqn, &cc->superfqn_capa, 0, '\0') <= -1) return -1;
	cc->supername = cc->superfqn;
	return 0;
}

static MOO_INLINE int set_class_modname (moo_t* moo, moo_cunit_class_t* cc, const moo_oocs_t* name)
{
	if (copy_string_to(moo, name, &cc->modname, &cc->modname_capa, 0, '\0') <= -1) return -1;
	return 0;
}

static MOO_INLINE int set_pooldic_fqn (moo_t* moo, moo_cunit_pooldic_t* pd, const moo_oocs_t* name)
{
	if (copy_string_to(moo, name, &pd->fqn, &pd->fqn_capa, 0, '\0') <= -1) return -1;
	pd->name = pd->fqn;
	return 0;
}

static MOO_INLINE int set_interface_fqn (moo_t* moo, moo_cunit_interface_t* ifce, const moo_oocs_t* name)
{
	if (copy_string_to(moo, name, &ifce->fqn, &ifce->fqn_capa, 0, '\0') <= -1) return -1;
	ifce->name = ifce->fqn;
	return 0;
}

static MOO_INLINE int add_class_level_variable (moo_t* moo, var_type_t var_type, const moo_oocs_t* name, const moo_ioloc_t* loc)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int n;

	n = copy_string_to(moo, name, &cc->var[var_type].str, &cc->var[var_type].str_capa, 1, ' ');
	if (n >= 0) 
	{
		static moo_oow_t varlim[] =
		{
			MOO_MAX_NAMED_INSTVARS, /* VAR_INSTANCE */
			MOO_MAX_CLASSINSTVARS,  /* VAR_CLASSINST */
			MOO_MAX_CLASSVARS,      /* VAR_CLASS */
		};

		MOO_ASSERT (moo, VAR_INSTANCE == 0);
		MOO_ASSERT (moo, VAR_CLASSINST == 1);
		MOO_ASSERT (moo, VAR_CLASS == 2);
		MOO_ASSERT (moo, var_type >= VAR_INSTANCE && var_type <= VAR_CLASS);

		if (cc->var[var_type].total_count >= varlim[var_type])
		{
			moo_setsynerrbfmt (moo, MOO_SYNERR_VARFLOOD, loc, name, "too many class-level variables");
			return -1;
		}

		cc->var[var_type].count++;
		cc->var[var_type].total_count++;
	}

	return n;
}

static int set_class_level_variable_initv (moo_t* moo, var_type_t var_type, moo_oow_t var_index, moo_oop_t initv, int flags)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	if (var_index >= cc->var[var_type].initv_capa)
	{
		moo_oow_t newcapa, oldcapa;
		/*moo_oow_t i;*/
		moo_initv_t* tmp;

		oldcapa = cc->var[var_type].initv_capa;
		newcapa = MOO_ALIGN_POW2 ((var_index + 1), 32);
		tmp = moo_reallocmem (moo, cc->var[var_type].initv, newcapa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		/*for (i = cc->var[var_type].initv_capa; i < newcapa; i++) tmp[i] = MOO_NULL;*/
		MOO_MEMSET (&tmp[oldcapa], 0, (newcapa - oldcapa) * MOO_SIZEOF(moo_oop_t));

		cc->var[var_type].initv = tmp;
		cc->var[var_type].initv_capa = newcapa;
	}

	if (var_index >= cc->var[var_type].initv_count) 
	{
		moo_oow_t i;

		for (i = cc->var[var_type].initv_count; i < var_index; i++) 
		{
			cc->var[var_type].initv[i].v = MOO_NULL;
			cc->var[var_type].initv[i].flags = 0;
		}

		cc->var[var_type].initv_count = var_index + 1;
	}

	cc->var[var_type].initv[var_index].v = initv;
	cc->var[var_type].initv[var_index].flags = flags;
	return 0;
}

static MOO_INLINE int add_pooldic_import (moo_t* moo, const moo_oocs_t* name, moo_oop_dic_t pooldic_oop)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (cc->pdimp.dcl_count >= cc->pdimp.oops_capa)
	{
		moo_oow_t new_capa;
		moo_oop_dic_t* tmp;

		new_capa = MOO_ALIGN(cc->pdimp.oops_capa + 1, POOLDIC_OOP_BUFFER_ALIGN);
		tmp = moo_reallocmem(moo, cc->pdimp.oops, new_capa * MOO_SIZEOF(moo_oop_dic_t));
		if (!tmp) return -1;

		cc->pdimp.oops_capa = new_capa;
		cc->pdimp.oops = tmp;
	}

	cc->pdimp.oops[cc->pdimp.dcl_count] = pooldic_oop;
	cc->pdimp.dcl_count++;
/* TODO: check if pooldic_count overflows */

	return 0;
}

static moo_ooi_t find_class_level_variable (moo_t* moo, moo_oop_class_t self, const moo_oocs_t* name, var_info_t* var)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
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
		 *    classinstvars
		 *    classvars
		 */
		vv = &self->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASS; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = MOO_OBJ_GET_SIZE(v);

			if (find_word_in_string(&hs, name, &pos) >= 0)
			{
				super = self->superclass;
				MOO_ASSERT (moo, super == cc->super_oop);

				/* 'self' may be MOO_NULL if MOO_NULL has been given for it.
				 * the caller must take good care when interpreting the meaning of 
				 * this field */
				var->cls = self;
				goto done;
			}
		}

		super = self->superclass;
		MOO_ASSERT (moo, super == cc->super_oop);
	}
	else
	{
		/* the class definition is not available yet.
		 * find the variable in the compiler's own list */
		for (index = VAR_INSTANCE; index <= VAR_CLASS; index++)
		{
			if (find_word_in_string(&cc->var[index].str, name, &pos) >= 0)
			{
				super = cc->super_oop;
				var->cls = MOO_NULL; /* the current class being compiled */
				goto done;
			}
		}
		super = cc->super_oop;
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
		for (index = VAR_INSTANCE; index <= VAR_CLASS; index++)
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

	moo_seterrnum (moo, MOO_ENOENT);
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
				pos += MOO_CLASS_SPEC_NAMED_INSTVARS(spec);
				break;

			case VAR_CLASSINST:
				spec = MOO_OOP_TO_SMOOI(((moo_oop_class_t)super)->selfspec);
				pos += MOO_CLASS_SELFSPEC_CLASSINSTVARS(spec);
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
		}
	}

	var->type = index;
	var->pos = pos;
	return pos;
}

static int clone_assignee (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int n;
	moo_oow_t old_len;

	old_len = cc->mth.assignees.len;
	n = copy_string_to (moo, name, &cc->mth.assignees, &cc->mth.assignees_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = cc->mth.assignees.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int clone_binary_selector (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int n;
	moo_oow_t old_len;

	old_len = cc->mth.binsels.len;
	n = copy_string_to (moo, name, &cc->mth.binsels, &cc->mth.binsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = cc->mth.binsels.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int clone_keyword (moo_t* moo, const moo_oocs_t* name, moo_oow_t* offset)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int n;
	moo_oow_t old_len;

	old_len = cc->mth.kwsels.len;
	n = copy_string_to (moo, name, &cc->mth.kwsels, &cc->mth.kwsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	/*name->ptr = cc->mth.kwsels.ptr + old_len;*/
	*offset = old_len;
	return 0;
}

static int add_method_name_fragment (moo_t* moo, moo_method_data_t* mth, const moo_oocs_t* name)
{
	/* method name fragments are concatenated without any delimiters */
	return copy_string_to (moo, name, &mth->name, &mth->name_capa, 1, '\0');
}

static int method_exists (moo_t* moo, const moo_oocs_t* name)
{
	if (moo->c->cunit->cunit_type == MOO_CUNIT_INTERFACE)
	{
/* TODO: remove duplicate code between interface and class */
		moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;

		if (ifce->mth.type == MOO_METHOD_DUAL)
		{
			return moo_lookupdic(moo, ifce->self_oop->mthdic[MOO_METHOD_INSTANCE], name) != MOO_NULL ||
			       moo_lookupdic(moo, ifce->self_oop->mthdic[MOO_METHOD_CLASS], name) != MOO_NULL;
		}
		else
		{
			MOO_ASSERT (moo, ifce->mth.type < MOO_COUNTOF(ifce->self_oop->mthdic));
			return moo_lookupdic(moo, ifce->self_oop->mthdic[ifce->mth.type], name) != MOO_NULL;
		}
	}
	else
	{
		/* this function must be called from the inteface or the class context only */
		moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

		MOO_ASSERT (moo, moo->c->cunit->cunit_type == MOO_CUNIT_CLASS);

		/* check if the current class contains a method of the given name */
		if (cc->mth.type == MOO_METHOD_DUAL)
		{
			return moo_lookupdic(moo, cc->self_oop->mthdic[MOO_METHOD_INSTANCE], name) != MOO_NULL ||
			       moo_lookupdic(moo, cc->self_oop->mthdic[MOO_METHOD_CLASS], name) != MOO_NULL;
		}
		else
		{
			MOO_ASSERT (moo, cc->mth.type < MOO_COUNTOF(cc->self_oop->mthdic));
			return moo_lookupdic(moo, cc->self_oop->mthdic[cc->mth.type], name) != MOO_NULL;
		}
	}
}

static int add_temporary_variable (moo_t* moo, moo_method_data_t* mth, const moo_oocs_t* name)
{
	/* temporary variable names are added to the string with leading
	 * space if it's not the first variable */
	return copy_string_to(moo, name, &mth->tmprs, &mth->tmprs_capa, 1, ' ');
}

static MOO_INLINE int find_temporary_variable (moo_t* moo, moo_method_data_t* mth, const moo_oocs_t* name, moo_oow_t* xindex)
{
	return find_word_in_string(&mth->tmprs, name, xindex);
}

static moo_oop_nsdic_t add_namespace (moo_t* moo, moo_oop_nsdic_t dic, const moo_oocs_t* name)
{
	/* add a stand-alone namespace that doesn't belong to a class */

	moo_oow_t tmp_count = 0;
	moo_oop_char_t sym;
	moo_oop_nsdic_t nsdic;
	moo_oop_association_t ass;

	moo_pushtmp (moo, (moo_oop_t*)&dic); tmp_count++;

	sym = (moo_oop_char_t)moo_makesymbol(moo, name->ptr, name->len);
	if (!sym) goto oops;

	moo_pushtmp (moo, (moo_oop_t*)&sym); tmp_count++;

	nsdic = moo_makensdic(moo, moo->_namespace, NAMESPACE_SIZE);
	if (!nsdic) goto oops;

	/*moo_pushtmp (moo, &ns); tmp_count++;*/
	ass = moo_putatdic(moo, (moo_oop_dic_t)dic, (moo_oop_t)sym, (moo_oop_t)nsdic);
	if (!ass) goto oops;

	nsdic = (moo_oop_nsdic_t)ass->value; 
	nsdic->nsup = (moo_oop_t)dic;
	nsdic->name = sym;

	moo_poptmps (moo, tmp_count);
	return nsdic;

oops:
	moo_poptmps (moo, tmp_count);
	return MOO_NULL;
}

static moo_oop_nsdic_t attach_nsdic_to_class (moo_t* moo, moo_oop_class_t c)
{
	moo_oow_t tmp_count = 0;
	moo_oop_nsdic_t nsdic;

	moo_pushtmp (moo, (moo_oop_t*)&c); tmp_count++;

	nsdic = moo_makensdic(moo, moo->_namespace, NAMESPACE_SIZE);
	if (!nsdic) goto oops;

	nsdic->nsup = (moo_oop_t)c; /* it points to the owning class as it belongs to a class */
	nsdic->name = c->name; /* for convenience only */
	c->nsdic = nsdic;

	moo_poptmps (moo, tmp_count);
	return nsdic;

oops:
	moo_poptmps (moo, tmp_count);
	return MOO_NULL;
}

#define PDN_DONT_ADD_NS (1 << 0)
#define PDN_ACCEPT_POOLDIC_AS_NS (1 << 1)

static int preprocess_dotted_name (moo_t* moo, int flags, moo_oop_nsdic_t topdic, const moo_oocs_t* fqn, const moo_ioloc_t* fqn_loc, moo_oocs_t* name, moo_oop_nsdic_t* ns_oop)
{
	const moo_ooch_t* ptr, * dot;
	moo_oow_t len;
	moo_oocs_t seg;
	moo_oop_nsdic_t dic;
	moo_oop_association_t ass;
	int pooldic_gotten = 0;

	dic = topdic? topdic: moo->sysdic;
	ptr = fqn->ptr;
	len = fqn->len;

	while (1)
	{
		seg.ptr = (moo_ooch_t*)ptr;

		dot = moo_find_oochar (ptr, len, '.');
		if (dot)
		{
			if (pooldic_gotten) goto wrong_name;

			seg.len = dot - ptr;

			if (is_reserved_word(&seg)) goto wrong_name;

			ass = moo_lookupdic(moo, (moo_oop_dic_t)dic, &seg);
			if (ass)
			{
				if (MOO_CLASSOF(moo, ass->value) == moo->_namespace)
				{
					/* ok - the current segment is a namespace name */
					dic = (moo_oop_nsdic_t)ass->value;
				}
				else if (MOO_CLASSOF(moo, ass->value) == moo->_class)
				{
					/* the segment is a class name. use the nsdic field.
					 *   class X {}
					 *   class X.Y {}
					 *  when processing X in X.Y, this part is reached. */
					moo_oop_nsdic_t t;

					t = ((moo_oop_class_t)ass->value)->nsdic;
					if ((moo_oop_t)t == moo->_nil)
					{
						if (flags & PDN_DONT_ADD_NS) goto wrong_name;

						/* attach a new namespace dictionary to the nsdic field
						 * of the class */
						t = attach_nsdic_to_class(moo, (moo_oop_class_t)ass->value);
						if (!t) return -1;

						dic = t;
					}
					else dic = t;
				}
				else if ((flags & PDN_ACCEPT_POOLDIC_AS_NS) && MOO_CLASSOF(moo, ass->value) == moo->_pool_dictionary)
				{
					/* A pool dictionary is treated as if it's a name space.
					 * However, the pool dictionary can only act as a name space
					 * if it's the second last segment. A pool dictionary 
					 * cannot be nested in another pool dictionary */
					dic = (moo_oop_nsdic_t)ass->value;
					pooldic_gotten = 1;
				}
				else
				{
					goto wrong_name;
				}
			}
			else
			{
				moo_oop_nsdic_t t;

				/* the segment does not exist. add it */
				if (flags & PDN_DONT_ADD_NS)
				{
					/* in '#extend Planet.Earth', it's an error
					 * if Planet doesn't exist */
					goto wrong_name;
				}
				
				/* When definining a new class, add a missing namespace */
				t = add_namespace(moo, dic, &seg);
				if (!t) return -1;

				dic = t;
			}
		}
		else
		{
			/* this is the last segment. it should be a class name or an item name */
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
	moo_setsynerr (moo, MOO_SYNERR_NAMESPACEINVAL, fqn_loc, &seg);
	return -1;
}

static int import_pool_dictionary (moo_t* moo, moo_oop_nsdic_t ns_oop, const moo_oocs_t* tok_lastseg, const moo_oocs_t* tok_name, const moo_ioloc_t* tok_loc)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oop_association_t ass;
	moo_oow_t i;

	/* check if the name refers to a pool dictionary */
	ass = moo_lookupdic(moo, (moo_oop_dic_t)ns_oop, tok_lastseg);
	if (!ass || MOO_CLASSOF(moo, ass->value) != moo->_pool_dictionary)
	{
		moo_setsynerr (moo, MOO_SYNERR_PDIMPINVAL, tok_loc, tok_name);
		return -1;
	}

	/* check if the same dictionary pool has been declared for import */
	for (i = 0; i < cc->pdimp.dcl_count; i++)
	{
		if ((moo_oop_dic_t)ass->value == cc->pdimp.oops[i])
		{
			moo_setsynerr (moo, MOO_SYNERR_PDIMPDUPL, tok_loc, tok_name);
			return -1;
		}
	}

	if (add_pooldic_import(moo, tok_name, (moo_oop_dic_t)ass->value) <= -1) return -1;
	if (copy_string_to(moo, tok_name, &cc->pdimp.dcl, &cc->pdimp.dcl_capa, 1, ' ') <= -1)
	{
		cc->pdimp.dcl_count--; /* roll back add_pool_dictionary() */
		return -1;
	}

	return 0;
}

static int compile_class_level_variables_vbar (moo_t* moo, var_type_t dcl_type)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	/* class level variable declaration using vbar.
	 *  - no assignment of a default value is allowed
	 *  - same syntax as local variable declaration within a method */

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
			if (dcl_type == VAR_INSTANCE && (cc->flags & CLASS_INDEXED) && (cc->indexed_type != MOO_OBJ_TYPE_OOP))
			{
				/* a non-pointer object cannot have instance variables */
				moo_setsynerr (moo, MOO_SYNERR_VARDCLBANNED, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (find_class_level_variable(moo, MOO_NULL, TOKEN_NAME(moo), &var) >= 0 ||
			    moo_lookupdic(moo, (moo_oop_dic_t)moo->sysdic, TOKEN_NAME(moo)) ||  /* conflicts with a top global name */
			    moo_lookupdic(moo, (moo_oop_dic_t)cc->ns_oop, TOKEN_NAME(moo))) /* conflicts with a global name in the class'es name space */
			{
				moo_setsynerr (moo, MOO_SYNERR_VARNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (add_class_level_variable(moo, dcl_type, TOKEN_NAME(moo), TOKEN_LOC(moo)) <= -1) return -1;
		}
		else
		{
			break;
		}

		GET_TOKEN (moo);
	}
	while (1);

	if (!is_token_binary_selector(moo, VOCA_VBAR))
	{
		moo_setsynerr (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_class_level_variables (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	var_type_t dcl_type = VAR_INSTANCE;
	int varacc_type = 0;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process variable declaration modifiers  */
		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			do
			{
				if (is_token_symbol(moo, VOCA_CLASS_S))
				{
					/* variable(#class) */
					if (dcl_type != VAR_INSTANCE)
					{
						moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					dcl_type = VAR_CLASS;
					GET_TOKEN (moo);
				}
				else if (is_token_symbol(moo, VOCA_CLASSINST_S))
				{
					/* variable(#classinst) */
					if (dcl_type != VAR_INSTANCE)
					{
						moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					dcl_type = VAR_CLASSINST;
					GET_TOKEN (moo);
				}
				else if (is_token_symbol(moo, VOCA_GET_S))
				{
					/* variable(#get) */
					if (varacc_type & VARACC_GETTER)
					{
						moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					varacc_type |= VARACC_GETTER;
					GET_TOKEN (moo);
				}
				else if (is_token_symbol(moo, VOCA_SET_S))
				{
					/* variable(#set) */
					if (varacc_type & VARACC_SETTER)
					{
						moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					varacc_type |= VARACC_SETTER;
					GET_TOKEN (moo);
				}
				else if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
				{
					/* no modifier is present */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIER, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				else
				{
					/* invalid modifier */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully ) */
				GET_TOKEN (moo); /* get the token after , */
			}
			while (1);
		}

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			/* ) expected */
			moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

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
			if (dcl_type == VAR_INSTANCE && (cc->flags & CLASS_INDEXED) && (cc->indexed_type != MOO_OBJ_TYPE_OOP))
			{
				/* a non-pointer object cannot have instance variables */
				moo_setsynerr (moo, MOO_SYNERR_VARDCLBANNED, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (find_class_level_variable(moo, MOO_NULL, TOKEN_NAME(moo), &var) >= 0 ||
			    moo_lookupdic(moo, (moo_oop_dic_t)moo->sysdic, TOKEN_NAME(moo)) ||  /* conflicts with a top global name */
			    moo_lookupdic(moo, (moo_oop_dic_t)cc->ns_oop, TOKEN_NAME(moo))) /* conflicts with a global name in the class'es name space */
			{
				moo_setsynerr (moo, MOO_SYNERR_VARNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (add_class_level_variable(moo, dcl_type, TOKEN_NAME(moo), TOKEN_LOC(moo)) <= -1) return -1;

			GET_TOKEN (moo);

			if (TOKEN_TYPE(moo) == MOO_IOTOK_ASSIGN)
			{
				moo_oop_t lit;

				GET_TOKEN (moo);

				/* [NOTE] default value assignment. only a literal is allowed.
				 *    the initial values for instance variables and 
				 *    class instance variables are set to read-only.
				 *    I may change this if i get the actual initial
				 *    value assignment upon instantiation to employ
				 *    deep-copying in moo_instantiate() and in the compiler. */
				lit = token_to_literal(moo, dcl_type != VAR_CLASS);
				if (!lit) return -1;

				/* set the initial value for the variable added above */
				if (set_class_level_variable_initv(moo, dcl_type, cc->var[dcl_type].count - 1, lit, varacc_type) <= -1) return -1;

				GET_TOKEN (moo);
			}
			else if (varacc_type)
			{
				/* this part is to remember the variable access type that indicates
				 * whether to generate a getter method and a setter method */
				if (set_class_level_variable_initv (moo, dcl_type, cc->var[dcl_type].count - 1, MOO_NULL, varacc_type) <= -1) return -1;
			}
		}
		else if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD)
		{
			/* no variable name is present */
			moo_setsynerr (moo, MOO_SYNERR_VARNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		else
		{
			break;
		}

		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
		{
			moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		else if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully . */
		GET_TOKEN (moo);
	}
	while (1);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_class_level_imports (moo_t* moo)
{
	/* pool dictionary import declaration
	 * import ... */

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oocs_t last;
	moo_oop_nsdic_t ns_oop;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process import declaration modifiers  */
		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			do
			{
			#if 0
			/* TODO: do i need any modifier for import declaration?
			 *       import(#pooldic) - import a pool dictionary specifically?
			 *       import(#xxxxx) - to do what???
			 */
				if (is_token_symbol(moo, VOCA_POOLDIC_S))
				{
					/* import(#pooldic) - import a pool dictionary */
					if (dcl_type != IMPORT_POOLDIC)
					{
						moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					dcl_type = IMPORT_POOLDIC;
					GET_TOKEN (moo);
				}
				else 
			#endif
				if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
				{
					/* no modifier is present */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIER, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				else
				{
					/* invalid modifier */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully ) */
				GET_TOKEN (moo); /* get the token after , */
			}
			while (1);
		}

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			/* ) expected */
			moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	do
	{
		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
		{
			if (preprocess_dotted_name(moo, 0, MOO_NULL, TOKEN_NAME(moo), TOKEN_LOC(moo), &last, &ns_oop) <= -1) return -1;
		}
		else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
		{
			last = moo->c->tok.name;
			/* it falls back to the name space of the class */
			ns_oop = cc->ns_oop; 
		}
		else if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD)
		{
			/* no variable name is present */
			moo_setsynerr (moo, MOO_SYNERR_VARNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		else
		{
			break;
		}

		if (import_pool_dictionary(moo, ns_oop, &last, TOKEN_NAME(moo), TOKEN_LOC(moo)) <= -1) return -1;
		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED || TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
		{
			moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		else if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully . */
		GET_TOKEN (moo);
	}
	while (1);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_unary_method_name (moo_t* moo, moo_method_data_t* mth)
{
	MOO_ASSERT (moo, mth->name.len == 0);
	MOO_ASSERT (moo, mth->tmpr_nargs == 0);

	if (add_method_name_fragment(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;
	GET_TOKEN (moo);

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* this is a procedural style method */
		MOO_ASSERT (moo, mth->tmpr_nargs == 0);

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
		{
			do
			{
				if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
				{
					/* wrong argument name. identifier is expected */
					moo_setsynerr (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (find_temporary_variable(moo, mth, TOKEN_NAME(moo), MOO_NULL) >= 0)
				{
					moo_setsynerr (moo, MOO_SYNERR_ARGNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (add_temporary_variable(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;
				mth->tmpr_nargs++;
				GET_TOKEN (moo); 

				if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN) break;

				if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
				{
					moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				GET_TOKEN (moo);
			} 
			while (1);
		}

		GET_TOKEN (moo);
		return 9999; /* to indicate the method definition is in a procedural style */
	}

	return 0;
}

static int compile_binary_method_name (moo_t* moo, moo_method_data_t* mth)
{
	MOO_ASSERT (moo, mth->name.len == 0);
	MOO_ASSERT (moo, mth->tmpr_nargs == 0);

	if (add_method_name_fragment(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;
	GET_TOKEN (moo);

	/* collect the argument name */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
	{
		/* wrong argument name. identifier expected */
		moo_setsynerr (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	MOO_ASSERT (moo, mth->tmpr_nargs == 0);

	/* no duplication check is performed against class-level variable names.
	 * a duplcate name will shade a previsouly defined variable. */
	if (add_temporary_variable(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;
	mth->tmpr_nargs++;

	MOO_ASSERT (moo, mth->tmpr_nargs == 1);
	/* this check should not be not necessary
	if (mth->tmpr_nargs > MAX_CODE_NARGS)
	{
		moo_setsynerr (moo, MOO_SYNERR_ARGFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}
	*/

	GET_TOKEN (moo);
	return 0;
}

static int compile_keyword_method_name (moo_t* moo, moo_method_data_t* mth)
{
	MOO_ASSERT (moo, mth->name.len == 0);
	MOO_ASSERT (moo, mth->tmpr_nargs == 0);

	do 
	{
		if (add_method_name_fragment(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
		{
			/* wrong argument name. identifier is expected */
			moo_setsynerr (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (find_temporary_variable(moo, mth, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			moo_setsynerr (moo, MOO_SYNERR_ARGNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, mth, TOKEN_NAME(moo)) <= -1) return -1;
		mth->tmpr_nargs++;

		GET_TOKEN (moo);
	} 
	while (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD);

	return 0;
}

static int compile_method_name (moo_t* moo, moo_method_data_t* mth)
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

	MOO_ASSERT (moo, mth->tmpr_count == 0);

	mth->name_loc = moo->c->tok.loc;
	switch (TOKEN_TYPE(moo))
	{
		case MOO_IOTOK_IDENT:
			n = compile_unary_method_name(moo, mth);
			break;

		case MOO_IOTOK_BINSEL:
			n = compile_binary_method_name(moo, mth);
			break;

		case MOO_IOTOK_KEYWORD:
			n = compile_keyword_method_name(moo, mth);
			break;

		default:
			/* illegal method name  */
			moo_setsynerr (moo, MOO_SYNERR_MTHNAME, TOKEN_LOC(moo), TOKEN_NAME(moo));
			n = -1;
	}

	if (n >= 0)
	{
		if (method_exists(moo, &mth->name)) 
 		{
			moo_setsynerr (moo, MOO_SYNERR_MTHNAMEDUPL, &mth->name_loc, &mth->name);
			return -1;
		}

		/* compile_unary_method_name() returns 9999 if the name is followed by () */
		if (mth->variadic && n != 9999)
		{
			moo_setsynerr (moo, MOO_SYNERR_VARIADMTHINVAL, &mth->name_loc, &mth->name);
			return -1;
		}
	}

	MOO_ASSERT (moo, mth->tmpr_nargs < MAX_CODE_NARGS);
	/* the total number of temporaries is equal to the number of 
	 * arguments after having processed the message pattern. it's because
	 * moo treats arguments the same as temporaries */
	mth->tmpr_count = mth->tmpr_nargs;
	return n;
}

static int compile_method_temporaries (moo_t* moo)
{
	/* 
	 * method-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (moo);
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT) 
	{
		/* a temporary variable name may get the same as a class level variable name 
		 * or even a class name in such as case, it shadows the class level variable
		 * name or the class name. however, it can't be the same as another temporary 
		 * variable */
		if (find_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			moo_setsynerr (moo, MOO_SYNERR_TMPRNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo)) <= -1) return -1;
		cc->mth.tmpr_count++;

		if (cc->mth.tmpr_count > MAX_CODE_NARGS)
		{
			moo_setsynerr (moo, MOO_SYNERR_TMPRFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		moo_setsynerr (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_method_pragma (moo_t* moo)
{
	/* 
	 * method-pragma := "<"  "primitive:" integer ">" |
	 *                  "<"  "primitive:" symbol ">" |
	 *                  "<"  "exception" ">" |
	 *                  "<"  "ensure" ">"
	 */
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
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
			case MOO_IOTOK_NUMLIT:
	/*TODO: more checks the validity of the primitive number. support number with radix and so on support more extensive syntax. support primitive name, not number*/
				ptr = TOKEN_NAME_PTR(moo);
				end = ptr + TOKEN_NAME_LEN(moo);
				pfnum = 0;
				while (ptr < end && is_digitchar(*ptr)) 
				{
					pfnum = pfnum * 10 + (*ptr - '0');
					if (!MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(pfnum))
					{
						moo_setsynerr (moo, MOO_SYNERR_PFNUMINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					ptr++;
				}

/* TODO: check if the number points to one of the internal primitive function */

				cc->mth.pfnum = pfnum;
				break;

			case MOO_IOTOK_SYMLIT:
			{
				const moo_ooch_t* tptr;
				moo_oow_t tlen;
				moo_pfbase_t* pfbase;

				tptr = TOKEN_NAME_PTR(moo) + 1;
				tlen = TOKEN_NAME_LEN(moo) - 1;

				/* attempt get a primitive function number by name */
				pfbase = moo_getpfnum(moo, tptr, tlen, &pfnum);
				if (!pfbase)
				{
					/* a built-in primitive function is not found 
					 * check if it is a primitive function identifier */
					moo_oow_t lit_idx;

					if (!moo_rfind_oochar(tptr, tlen, '.'))
					{
						/* wrong primitive function identifier */
						moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					/* external named primitive containing a period. */

					/* perform some sanity checks. see compile_method_definition() for similar checks */
					pfbase = moo_querymod(moo, tptr, tlen, MOO_NULL);
					if (!pfbase)
					{
						MOO_DEBUG2 (moo, "Cannot find module primitive function - %.*js\n", tlen, tptr);
						moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					if (cc->mth.tmpr_nargs < pfbase->minargs || cc->mth.tmpr_nargs > pfbase->maxargs)
					{
						MOO_DEBUG5 (moo, "Unsupported argument count in primitive method definition of %.*js - %zd-%zd expected, %zd specified\n", 
							tlen, tptr, pfbase->minargs, pfbase->maxargs, cc->mth.tmpr_nargs);
						moo_setsynerr (moo, MOO_SYNERR_PFARGDEFINVAL, &cc->mth.name_loc, &cc->mth.name);
						return -1;
					}

					if (add_symbol_literal(moo, TOKEN_NAME(moo), 1, &lit_idx) <= -1) return -1;

					/* the primitive definition is placed at the very beginning of a method defintion.
					 * so the index to the symbol registered should be very small like 0 */
					MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(lit_idx));

					cc->mth.pftype = PFTYPE_NAMED; 
					cc->mth.pfnum = lit_idx;
				}
				else if (!MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(pfnum))
				{
					moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				else
				{
					if (cc->mth.tmpr_nargs < pfbase->minargs || cc->mth.tmpr_nargs > pfbase->maxargs)
					{
						MOO_DEBUG5 (moo, "Unsupported argument count in primitive method definition of %.*js - %zd-%zd expected, %zd specified\n", 
							tlen, tptr, pfbase->minargs, pfbase->maxargs, cc->mth.tmpr_nargs);
						moo_setsynerr (moo, MOO_SYNERR_PFARGDEFINVAL, &cc->mth.name_loc, &cc->mth.name);
						return -1;
					}

					cc->mth.pftype = PFTYPE_NUMBERED; 
					cc->mth.pfnum = pfnum;
				}

				break;
			}

			default:
				moo_setsynerr (moo, MOO_SYNERR_INTEGER, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
		}

	}
	else if (is_token_word(moo, VOCA_EXCEPTION))
	{
/* TODO: exception handler is supposed to be used by BlockContext on:do:. 
 *       it needs to check the number of arguments at least */
		cc->mth.pftype = PFTYPE_EXCEPTION;
	}
	else if (is_token_word(moo, VOCA_ENSURE))
	{
		cc->mth.pftype = PFTYPE_ENSURE;
	}
	else
	{
		moo_setsynerr (moo, MOO_SYNERR_PRIMITIVE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	if (!is_token_binary_selector(moo, VOCA_GT)) 
	{
		moo_setsynerr (moo, MOO_SYNERR_GT, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int validate_class_level_variable (moo_t* moo, var_info_t* var, const moo_oocs_t* name, const moo_ioloc_t* name_loc)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	switch (var->type)
	{
		case VAR_INSTANCE:
			if (cc->mth.type == MOO_METHOD_CLASS || cc->mth.type == MOO_METHOD_DUAL)
			{
				/* a class(or dual) method cannot access an instance variable */
				moo_setsynerr (moo, MOO_SYNERR_VARINACC, name_loc, name);
				return -1;
			}
			break;

		case VAR_CLASSINST:
			/* class instance variable can be accessed by only pure class methods */
			if (cc->mth.type == MOO_METHOD_INSTANCE || cc->mth.type == MOO_METHOD_DUAL)
			{
				/* an instance method cannot access a class-instance variable */
				moo_setsynerr (moo, MOO_SYNERR_VARINACC, name_loc, name);
				return -1;
			}

			/* to a class object itself, a class-instance variable is
			 * just an instance variriable. but these are located
			 * after the named instance variables. */
			var->pos += MOO_CLASS_NAMED_INSTVARS;
			break;

		case VAR_CLASS:
			/* a class variable can be accessed by both instance methods and class methods */
			MOO_ASSERT (moo, var->cls != MOO_NULL);
			MOO_ASSERT (moo, MOO_CLASSOF(moo, var->cls) == moo->_class);

			/* increment the position by the number of class instance variables
			 * as the class variables are placed after the class instance variables */
			var->pos += MOO_CLASS_NAMED_INSTVARS + 
					  MOO_CLASS_SELFSPEC_CLASSINSTVARS(MOO_OOP_TO_SMOOI(var->cls->selfspec));
			break;

		default:
			/* internal error - it must not happen */
			moo_seterrnum (moo, MOO_EINTERN);
			return -1;
	}

	return 0;
}

static moo_oow_t is_dotted_ident_prefixed (const moo_oocs_t* name, int voca_id)
{
	if (name->len > vocas[voca_id].len &&
	    moo_equal_oochars(name->ptr, vocas[voca_id].str, vocas[voca_id].len) &&
	    name->ptr[vocas[voca_id].len] == '.') return vocas[voca_id].len;

	return 0;
}

static MOO_INLINE int find_dotted_ident (moo_t* moo, const moo_oocs_t* name, const moo_ioloc_t* name_loc, var_info_t* var)
{
	/* if a name is dotted,
	 * 
	 *   self.XXX - instance variable
	 *   A.B.C    - namespace or pool dictionary related reference.
	 *   self.B.C - B.C under the current class where B is not an instance variable
	 */

	moo_oocs_t last;
	moo_oop_nsdic_t top_dic, ns_oop;
	moo_oop_association_t ass;

	moo_oow_t pxlen;

	moo_oocs_t xname;
	moo_ioloc_t xname_loc;

	top_dic = MOO_NULL;
	xname = *name;
	xname_loc = *name_loc;

	if ((pxlen = is_dotted_ident_prefixed(name, VOCA_SELF)) > 0)
	{
		/* the first word in the dotted notation is self */

		if (!moo_find_oochar(name->ptr + pxlen + 1, name->len - pxlen - 1, '.'))
		{
			/* the dotted name is composed of 2 segments only */
			last.ptr = name->ptr + pxlen + 1;
			last.len = name->len - pxlen - 1;

			if (is_reserved_word(&last))
			{
				/* self. is followed by a reserved word.
				 * a proper variable name is expected. */
				moo_setsynerr (moo, MOO_SYNERR_VARNAME, name_loc, name);
				return -1;
			}

			MOO_ASSERT (moo, moo->c->cunit != MOO_NULL);

			if (moo->c->cunit->cunit_type == MOO_CUNIT_POOLDIC)
			{
				moo_oop_t v;

				/* called inside a pooldic definition */
				MOO_ASSERT (moo, ((moo_cunit_pooldic_t*)moo->c->cunit)->ns_oop != MOO_NULL);
				MOO_ASSERT (moo, ((moo_cunit_pooldic_t*)moo->c->cunit)->pd_oop == MOO_NULL);

				v = find_element_in_compiling_pooldic(moo, &last);
				if (!v)
				{
					moo_setsynerr (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
					return -1;
				}

				var->type = VAR_LITERAL;
				var->u.lit = v; /* TODO: change this */
				return 0;
			}
			else
			{
				moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

				MOO_ASSERT (moo, moo->c->cunit->cunit_type == MOO_CUNIT_CLASS);
				/* called inside a class definition */

				if (cc->in_class_body)
				{
					/* [NOTE] 
					 *  cls.ns_oop is set when the class name is enountered.
					 *  cls.super_oop is set when the parent class name is enountered.
					 *  cls.super_oop may still be MOO_NULL even if cls.ns_oop is not.
					 *  on the other hand, cls.ns_oop is not MOO_NULL as long as
					 *  cls.super_oop is not MOO_NULL.
					 */
					MOO_ASSERT (moo, cc->super_oop != MOO_NULL);
					MOO_ASSERT (moo, cc->ns_oop != MOO_NULL);
					/* cc->self_oop may still be MOO_NULL if the class has not been instantiated */

					if (find_class_level_variable(moo, cc->self_oop, &last, var) >= 0)
					{
						/* if the current class has not been instantiated, 
						 * no validation nor adjustment of the var->pos field is performed */
						if (cc->self_oop && validate_class_level_variable(moo, var, name, name_loc) <= -1) return -1;
						return 0;
					}
				}
			}
		}

		if (moo->c->cunit->cunit_type != MOO_CUNIT_CLASS || !((moo_cunit_class_t*)moo->c->cunit)->self_oop)
		{
			/* self is not usable when it's not compiling in a class.
			 * a pooldic definition cannot contain subdictionaries.
			 * the pooldic is a terminal point and the items inside
			 * are final nodes. if self is followed by more than 1 
			 * subsegments, it's an error */
			moo_setsynerr (moo, MOO_SYNERR_VARINACC, name_loc, name);
			return -1;
		}

		/* namespace that the current class starts */
		top_dic = ((moo_cunit_class_t*)moo->c->cunit)->self_oop->nsdic;
		pxlen++; /* include . into the length */
	}
	else if ((pxlen = is_dotted_ident_prefixed(name, VOCA_SELFNS)) > 0)
	{
		/* the first segment is selfns which indicates a namespace that
		 * the current class belongs to */
		if (moo->c->cunit->cunit_type == MOO_CUNIT_POOLDIC)
		{
			/* compiling in a pooldic definition */
			top_dic = ((moo_cunit_pooldic_t*)moo->c->cunit)->ns_oop;
			MOO_ASSERT (moo, top_dic != MOO_NULL);
		}
		else if (moo->c->cunit->cunit_type == MOO_CUNIT_CLASS)
		{
			/* compiling in a class definition */
			top_dic = ((moo_cunit_class_t*)moo->c->cunit)->ns_oop;
			MOO_ASSERT (moo, top_dic != MOO_NULL);
		}

		pxlen++; /* include . into the length */
	}

	if ((moo_oop_t)top_dic == moo->_nil) top_dic = MOO_NULL;
	xname.ptr += pxlen;
	xname.len -= pxlen;
	xname_loc.colm += pxlen;

	if (preprocess_dotted_name(moo, PDN_DONT_ADD_NS | PDN_ACCEPT_POOLDIC_AS_NS, top_dic, &xname, &xname_loc, &last, &ns_oop) <= -1) return -1;

	ass = moo_lookupdic(moo, (moo_oop_dic_t)ns_oop, &last);
	if (!ass)
	{
		/* undeclared identifier */
		moo_setsynerr (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
		return -1;
	}

	var->type = VAR_GLOBAL;
	var->u.gbl = ass;
	return 0;
}

static MOO_INLINE int find_undotted_ident (moo_t* moo, const moo_oocs_t* name, const moo_ioloc_t* name_loc, var_info_t* var)
{
	moo_oow_t index;
	moo_oop_association_t ass;

	if (moo->c->cunit->cunit_type == MOO_CUNIT_POOLDIC)
	{
		moo_oop_t v;

		/* called inside a pooldic definition */
		MOO_ASSERT (moo, ((moo_cunit_pooldic_t*)moo->c->cunit)->ns_oop != MOO_NULL);
		MOO_ASSERT (moo, ((moo_cunit_pooldic_t*)moo->c->cunit)->pd_oop == MOO_NULL);

		v = find_element_in_compiling_pooldic(moo, name);
		if (!v)
		{
			moo_setsynerr (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
			return -1;
		}

		var->type = VAR_LITERAL;
		var->u.lit = v; /* TODO: change this */
	}
	else
	{
		moo_cunit_class_t* cc = ((moo_cunit_class_t*)moo->c->cunit);
		MOO_ASSERT (moo, moo->c->cunit->cunit_type == MOO_CUNIT_CLASS);

		if (cc->self_oop)
		{
			/* the current class being compiled has been instantiated.
			 * look up in the temporary variable list if compiling in a method */
			if (cc->mth.active && find_temporary_variable(moo, &cc->mth, name, &index) >= 0)
			{
				var->type = (index < cc->mth.tmpr_nargs)? VAR_ARGUMENT: VAR_TEMPORARY;
				var->pos = index;
				return 0;
			}
		}

		if (cc->in_class_body)
		{
			/* called inside a class definition */
			/* read a comment in find_dotted_ident() for the reason behind
			 * the 'if' condition above. */

			MOO_ASSERT (moo, cc->super_oop != MOO_NULL);
			MOO_ASSERT (moo, cc->ns_oop != MOO_NULL);

			if (find_class_level_variable(moo, cc->self_oop, name, var) >= 0)
			{
				/* if the current class being compiled has not been instantiated,
				 * no validation nor adjustment of the var->pos field is performed */
				return cc->self_oop? validate_class_level_variable(moo, var, name, name_loc): 0;
			}
		}

		/* find an undotted identifier in dictionaries */
		if (cc->ns_oop)
		{
			ass = moo_lookupdic(moo, (moo_oop_dic_t)cc->ns_oop, name); /* in the current name space */
			if (!ass && cc->ns_oop != moo->sysdic) 
				ass = moo_lookupdic(moo, (moo_oop_dic_t)moo->sysdic, name); /* in the top-level system dictionary */
		}
		else
		{
			ass = moo_lookupdic(moo, (moo_oop_dic_t)moo->sysdic, name); /* in the top-level system dictionary */
		}

		if (!ass)
		{
			moo_oow_t i;
			moo_oop_association_t ass2 = MOO_NULL;

			/* attempt to find the variable in pool dictionaries */
			for (i = 0; i < cc->pdimp.dcl_count; i++)
			{
				ass = moo_lookupdic(moo, cc->pdimp.oops[i], name);
				if (ass)
				{
					if (ass2)
					{
						/* the variable name has been found at least in 2 dictionaries - ambiguous */
						moo_setsynerr (moo, MOO_SYNERR_VARAMBIG, name_loc, name);
						return -1;
					}
					ass2 = ass;
				}
			}

			ass = ass2;
			if (!ass) 
			{
				moo_setsynerr (moo, MOO_SYNERR_VARUNDCL, name_loc, name);
				return -1;
			}
		}

		var->type = VAR_GLOBAL;
		var->u.gbl = ass;
	}

	return 0;
}

static int get_variable_info (moo_t* moo, const moo_oocs_t* name, const moo_ioloc_t* name_loc, int name_dotted, var_info_t* var)
{
	int x;

	MOO_MEMSET (var, 0, MOO_SIZEOF(*var));

	x = name_dotted? find_dotted_ident(moo, name, name_loc, var): find_undotted_ident(moo, name, name_loc, var);
	if (x <= -1) return -1;

	/* final validation on the range of pos field. just check regardless of var->type.
	 * i assume the functions above don't taint the var-type field when it's meaningless. */
	if (var->pos > MAX_CODE_INDEX)
	{
		/* the assignee is not usable because its index is too large 
		 * to be expressed in byte-codes. */
		moo_setsynerr (moo, MOO_SYNERR_VARUNUSE, name_loc, name);
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (moo);
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT) 
	{
		if (find_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			moo_setsynerr (moo, MOO_SYNERR_TMPRNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		if (add_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo)) <= -1) return -1;
		cc->mth.tmpr_count++;
		if (cc->mth.tmpr_count > MAX_CODE_NTMPRS)
		{
			moo_setsynerr (moo, MOO_SYNERR_TMPRFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo)); 
			return -1;
		}

		GET_TOKEN (moo);
	}

	if (!is_token_binary_selector(moo, VOCA_VBAR)) 
	{
		moo_setsynerr (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int store_tmpr_count_for_block (moo_t* moo, moo_oow_t tmpr_count)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (cc->mth.blk_depth >= cc->mth.blk_tmprcnt_capa)
	{
		moo_oow_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (cc->mth.blk_depth + 1, BLK_TMPRCNT_BUFFER_ALIGN);
		tmp = (moo_oow_t*)moo_reallocmem(moo, cc->mth.blk_tmprcnt, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		cc->mth.blk_tmprcnt_capa = new_capa;
		cc->mth.blk_tmprcnt = tmp;
	}

	/* [NOTE] i don't increment blk_depth here. it's updated
	 *        by the caller after this function has been called for
	 *        a new block entered. */
	cc->mth.blk_tmprcnt[cc->mth.blk_depth] = tmpr_count;
	return 0;
}

static int compile_block_expression (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
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

	if (cc->mth.loop) 
	{
		/* this block is placed inside the {} loop */
		cc->mth.loop->blkcount++; 
	}
	block_loc = *TOKEN_LOC(moo);
	GET_TOKEN (moo);

	saved_tmprs_len = cc->mth.tmprs.len;
	saved_tmpr_count = cc->mth.tmpr_count;
	MOO_ASSERT (moo, cc->mth.blk_depth > 0);
	MOO_ASSERT (moo, cc->mth.blk_tmprcnt[cc->mth.blk_depth - 1] == saved_tmpr_count);

	if (TOKEN_TYPE(moo) == MOO_IOTOK_COLON) 
	{
		colon_loc = moo->c->tok.loc;

		/* block temporary variables  - argument to blocks */
		do 
		{
			GET_TOKEN (moo);

			if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT) 
			{
				/* wrong argument name. identifier expected */
				moo_setsynerr (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

/* TODO: check conflicting names as well */
			if (find_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo), MOO_NULL) >= 0)
			{
				moo_setsynerr (moo, MOO_SYNERR_BLKARGNAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (add_temporary_variable(moo, &cc->mth, TOKEN_NAME(moo)) <= -1) return -1;
			cc->mth.tmpr_count++;
			if (cc->mth.tmpr_count > MAX_CODE_NARGS)
			{
				moo_setsynerr (moo, MOO_SYNERR_BLKARGFLOOD, TOKEN_LOC(moo), TOKEN_NAME(moo)); 
				return -1;
			}

			GET_TOKEN (moo);
		} 
		while (TOKEN_TYPE(moo) == MOO_IOTOK_COLON);

		if (!is_token_binary_selector(moo, VOCA_VBAR))
		{
			moo_setsynerr (moo, MOO_SYNERR_VBAR, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
	}

	block_arg_count = cc->mth.tmpr_count - saved_tmpr_count;
	if (block_arg_count > MAX_CODE_NBLKARGS)
	{
		/* while an integer object is pused to indicate the number of
		 * block arguments, evaluation which is done by message passing
		 * limits the number of arguments that can be passed. so the
		 * check is implemented */
		moo_setsynerr (moo, MOO_SYNERR_BLKARGFLOOD, &colon_loc, MOO_NULL); 
		return -1;
	}

	tmpr_loc = moo->c->tok.loc;
	if (compile_block_temporaries(moo) <= -1) return -1;

	/* this is a block-local temporary count including arguments */
	block_tmpr_count = cc->mth.tmpr_count - saved_tmpr_count;
	if (block_tmpr_count > MAX_CODE_NBLKTMPRS)
	{
		moo_setsynerr (moo, MOO_SYNERR_BLKTMPRFLOOD, &tmpr_loc, MOO_NULL); 
		return -1;
	}

	/* store the accumulated number of temporaries for the current block.
	 * block depth is not raised as it's not entering a new block but
	 * updating the temporaries count for the current block. */
	if (store_tmpr_count_for_block (moo, cc->mth.tmpr_count) <= -1) return -1;

#if defined(MOO_USE_MAKE_BLOCK)
	if (emit_double_param_instruction(moo, BCODE_MAKE_BLOCK, block_arg_count, cc->mth.tmpr_count/*block_tmpr_count*/) <= -1) return -1;
#else
	if (emit_byte_instruction(moo, BCODE_PUSH_CONTEXT) <= -1 ||
	    emit_push_smooi_literal(moo, block_arg_count) <= -1 ||
	    emit_push_smooi_literal(moo, cc->mth.tmpr_count/*block_tmpr_count*/) <= -1 ||
	    emit_byte_instruction(moo, BCODE_SEND_BLOCK_COPY) <= -1) return -1;
#endif

	/* insert dummy instructions before replacing them with a jump instruction */
	jump_inst_pos = cc->mth.code.len;
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
				moo_setsynerr (moo, MOO_SYNERR_RBRACK, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
	}

	if (emit_byte_instruction(moo, BCODE_RETURN_FROM_BLOCK) <= -1) return -1;

	if (patch_long_forward_jump_instruction(moo, jump_inst_pos, cc->mth.code.len, &block_loc) <= -1) return -1;

	/* restore the temporary count */
	cc->mth.tmprs.len = saved_tmprs_len;
	cc->mth.tmpr_count = saved_tmpr_count;

	if (cc->mth.loop) 
	{
		MOO_ASSERT (moo, cc->mth.loop->blkcount > 0);
		cc->mth.loop->blkcount--;
	}

	GET_TOKEN (moo); /* read the next token after ] */

	return 0;
}

static int add_to_byte_array_literal_buffer (moo_t* moo, moo_oob_t b)
{
	if (moo->c->balit.count >= moo->c->balit.capa)
	{
		moo_oob_t* tmp;
		moo_oow_t new_capa;

		new_capa = MOO_ALIGN (moo->c->balit.count + 1, BALIT_BUFFER_ALIGN);
		tmp = (moo_oob_t*)moo_reallocmem(moo, moo->c->balit.ptr, new_capa * MOO_SIZEOF(*tmp));
		if (!tmp) return -1;

		moo->c->balit.capa = new_capa;
		moo->c->balit.ptr = tmp;
	}

/* TODO: overflow check of moo->c->balit.count itself */
	moo->c->balit.ptr[moo->c->balit.count++] = b;
	return 0;
}

static MOO_INLINE int add_to_array_literal_buffer (moo_t* moo, moo_oop_t item)
{
	return add_oop_to_oopbuf(moo, &moo->c->arlit, item);
}

static int read_byte_array_literal (moo_t* moo, int rdonly, moo_oop_t* xlit)
{
	moo_ooi_t tmp;
	moo_oop_t ba;
	moo_oow_t saved_balit_count;
	int comma_used = -1; /* unknown */

	saved_balit_count = moo->c->balit.count;

	GET_TOKEN_GOTO (moo, oops); /* skip #[ and read the next token */

	while (TOKEN_TYPE(moo) == MOO_IOTOK_NUMLIT || TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT || TOKEN_TYPE(moo) == MOO_IOTOK_CHARLIT)
	{
		/* TODO: check if the number is an integer */

		if (TOKEN_TYPE(moo) == MOO_IOTOK_CHARLIT)
		{
			/* accept a character literal inside a byte array literal */
			tmp = TOKEN_NAME_PTR(moo)[0];
		}
		else if (string_to_smooi(moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT, &tmp) <= -1)
		{
			/* the token reader reads a valid token. no other errors
			 * than the range error must not occur */
			MOO_ASSERT (moo, moo->errnum == MOO_ERANGE);

			/* if the token is out of the SMOOI range, it's too big or 
			 * to small to be a byte */
			moo_setsynerr (moo, MOO_SYNERR_BYTERANGE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			goto oops;
		}

		if (tmp < 0 || tmp > 255)
		{
			moo_setsynerr (moo, MOO_SYNERR_BYTERANGE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			goto oops;
		}

		if (add_to_byte_array_literal_buffer(moo, tmp) <= -1) goto oops;
		GET_TOKEN_GOTO (moo, oops);

		if (comma_used == -1)
		{
			/* check if a comma has been used after the first element */
			if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA)
			{
				comma_used = 1;
				GET_TOKEN_GOTO (moo, oops);
			}
			else
			{
				comma_used = 0;
			}
		}
		else if (comma_used == 0)
		{
			/* a comma is not expected */
			if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA)
			{
				break;
			}
		}
		else if (comma_used == 1)
		{
			/* a comma is expected */
			if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACK) break;
			if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
			{
				moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
				goto oops;
			}
			GET_TOKEN_GOTO (moo, oops);
			if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_RBRACK)
			{
				moo_setsynerr (moo, MOO_SYNERR_LITERAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				goto oops;
			}
		}
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACK)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACK, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

	ba = moo_instantiate (moo, moo->_byte_array, &moo->c->balit.ptr[saved_balit_count], moo->c->balit.count - saved_balit_count);
	if (!ba) goto oops;

	if (rdonly)
	{
		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(ba));
		MOO_OBJ_SET_FLAGS_RDONLY (ba, 1);
	}
	*xlit = ba;

	moo->c->balit.count = saved_balit_count;
	return 0;

oops:
	moo->c->balit.count = saved_balit_count;
	return -1;
}


static int read_array_literal (moo_t* moo, int rdonly, moo_oop_t* xlit)
{
	moo_oop_t lit, a;
	moo_oow_t i, j, saved_arlit_count;

	saved_arlit_count = moo->c->arlit.count;

	GET_TOKEN_GOTO (moo, oops); /* skip #( */

	do
	{
		if (TOKEN_TYPE(moo) == MOO_IOTOK_EOF) 
		{
			moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			goto oops;
		}
		else if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN) break;

		lit = token_to_literal (moo, rdonly);
		if (!lit || add_to_array_literal_buffer(moo, lit) <= -1) goto oops;

		GET_TOKEN_GOTO (moo, oops);
	}
	while (1);

	a = moo_instantiate(moo, moo->_array, MOO_NULL, moo->c->arlit.count - saved_arlit_count);
	if (!a) goto oops;

	for (i = saved_arlit_count, j = 0; i < moo->c->arlit.count; i++, j++)
	{
		((moo_oop_oop_t)a)->slot[j] = moo->c->arlit.ptr[i];
	}


	if (rdonly)
	{
		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(a));
		MOO_OBJ_SET_FLAGS_RDONLY (a, 1);
	}
	*xlit = a;

	moo->c->arlit.count = saved_arlit_count;
	return 0;

oops:
	moo->c->arlit.count = saved_arlit_count;
	return -1;
}

static int compile_byte_array_literal (moo_t* moo)
{
	moo_oop_t lit;
	moo_oow_t index;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_HASHBRACK);

	if (read_byte_array_literal(moo, 1, &lit) <= -1 ||
	    add_literal(moo, lit, &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (moo);
	return 0;
}

static int compile_array_literal (moo_t* moo)
{
	moo_oop_t lit;
	moo_oow_t index;

	MOO_ASSERT (moo, moo->c->arlit.count == 0);
	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_HASHPAREN);

	if (read_array_literal(moo, 1, &lit) <= -1 ||
	    add_literal(moo, lit, &index) <= -1 ||
	    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (moo);
	return 0;
}

static int _compile_array_expression (moo_t* moo, int closer_token, int bcode_make, int bcode_pop_into)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t maip;
	moo_ioloc_t aeloc;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_PERCPAREN || TOKEN_TYPE(moo) == MOO_IOTOK_PERCBRACK);

	maip = cc->mth.code.len;
	if (emit_single_param_instruction(moo, bcode_make, 0) <= -1) return -1;

	aeloc = *TOKEN_LOC(moo);
	GET_TOKEN (moo); /* read a token after #{ */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		moo_oow_t index;

		index = 0;
		do
		{
			if (compile_method_expression(moo, 0) <= -1) return -1;
			if (emit_single_param_instruction(moo, bcode_pop_into, index) <= -1) return -1;
			index++;

			if (index > MAX_CODE_PARAM)
			{
				moo_setsynerr (moo, MOO_SYNERR_ARREXPFLOOD, &aeloc, MOO_NULL);
				return -1;
			}

			if (TOKEN_TYPE(moo) == closer_token) break;

			if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
			{
				moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			GET_TOKEN (moo);
		} 
		while (1);

	/* TODO: devise a double_param MAKE_ARRAY to increase the number of elementes supported... */
		/* patch the MAKE_ARRAY instruction */
	#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
		cc->mth.code.ptr[maip + 1] = index >> 8;
		cc->mth.code.ptr[maip + 2] = index & 0xFF;
	#else
		cc->mth.code.ptr[maip + 1] = index;
	#endif
	}

	GET_TOKEN (moo); /* read a token after } */
	return 0;
}

static int compile_array_expression (moo_t* moo)
{
	return _compile_array_expression(moo, MOO_IOTOK_RPAREN, BCODE_MAKE_ARRAY, BCODE_POP_INTO_ARRAY);
}

static int compile_bytearray_expression (moo_t* moo)
{
	return _compile_array_expression (moo, MOO_IOTOK_RBRACK, BCODE_MAKE_BYTEARRAY, BCODE_POP_INTO_BYTEARRAY);
}

static int compile_dictionary_expression (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t mdip;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_PERCBRACE);

	GET_TOKEN (moo); /* read a token after :{ */

	mdip = cc->mth.code.len;
	if (emit_single_param_instruction (moo, BCODE_MAKE_DICTIONARY, 0) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_oow_t count;

		count = 0;
		do
		{
			if (compile_method_expression (moo, 0) <= -1 ||
			    emit_byte_instruction (moo, BCODE_POP_INTO_DICTIONARY) <= -1) return -1;

			count++;

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
		cc->mth.code.ptr[mdip + 1] = count >> 8;
		cc->mth.code.ptr[mdip + 2] = count & 0xFF;
	#else
		cc->mth.code.ptr[mdip + 1] = count;
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

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	var_info_t var;
	int read_next_token = 0;
	moo_oow_t index;

	*to_super = 0;

	if (ident) 
	{
		/* the caller has read the identifier and the next word */
	handle_ident:
		MOO_ASSERT (moo, cc->super_oop != MOO_NULL && cc->self_oop != MOO_NULL);
		if (get_variable_info(moo, ident, ident_loc, ident_dotted, &var) <= -1) return -1;

		switch (var.type)
		{
			case VAR_ARGUMENT:
			case VAR_TEMPORARY:
			{
			#if defined(MOO_USE_CTXTEMPVAR)
				if (cc->mth.blk_depth > 0)
				{
					moo_oow_t i;

					/* if a temporary variable is accessed inside a block,
					 * use a special instruction to indicate it */
					MOO_ASSERT (moo, var.pos < cc->mth.blk_tmprcnt[cc->mth.blk_depth]);
					for (i = cc->mth.blk_depth; i > 0; i--)
					{
						if (var.pos >= cc->mth.blk_tmprcnt[i - 1])
						{
							if (emit_double_param_instruction(moo, BCODE_PUSH_CTXTEMPVAR_0, cc->mth.blk_depth - i, var.pos - cc->mth.blk_tmprcnt[i - 1]) <= -1) return -1;
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
				/* get_variable_info must not set var.cls to MOO_NULL 
				 * because the class object pointer should be available
				 * when a method definition is compiled */
				MOO_ASSERT (moo, var.cls != MOO_NULL); 

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
				if (add_literal(moo, (moo_oop_t)var.u.gbl, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_OBJECT_0, index) <= -1) return -1;
				break;

			default:
				moo_seterrnum (moo, MOO_EINTERN);
				return -1;
		}

		if (read_next_token) GET_TOKEN (moo);
	}
	else
	{
		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_IDENT_DOTTED:
			case MOO_IOTOK_IDENT:
				ident_dotted = (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED);
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

			case MOO_IOTOK_ERRLIT:
			{
				moo_oop_t tmp;

				tmp = string_to_error(moo, TOKEN_NAME(moo), TOKEN_LOC(moo));
				if (!tmp) return -1;

				if (add_literal(moo, tmp, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

				GET_TOKEN (moo);
				break;
			}

			case MOO_IOTOK_SMPTRLIT:
			{
				moo_oop_t tmp;

				tmp = string_to_smptr(moo, TOKEN_NAME(moo), TOKEN_LOC(moo));
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

			case MOO_IOTOK_SELFNS:
				if (emit_byte_instruction(moo, BCODE_PUSH_RECEIVER_NS) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_CHARLIT:
				MOO_ASSERT (moo, TOKEN_NAME_LEN(moo) == 1);
				if (emit_push_character_literal(moo, TOKEN_NAME_PTR(moo)[0]) <= -1) return -1;
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

			case MOO_IOTOK_SYMLIT:
				if (add_symbol_literal(moo, TOKEN_NAME(moo), 1, &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				GET_TOKEN (moo);
				break;

			case MOO_IOTOK_STRLIT:
				if (add_string_literal(moo, TOKEN_NAME(moo), &index) <= -1 ||
				    emit_single_param_instruction(moo, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
				GET_TOKEN (moo);
				break;


			case MOO_IOTOK_HASHPAREN: /* #( */
				/*GET_TOKEN (moo);*/
				if (compile_array_literal(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_HASHBRACK: /* #[ */
				/*GET_TOKEN (moo);*/
				if (compile_byte_array_literal(moo) <= -1) return -1;
				break;

#if 0
/* TODO: */
			case MOO_IOTOK_HASHBRACE: /* #{ */
				if (compile_dictionary_literal(moo) <= -1) return -1;
				break;
#endif

			case MOO_IOTOK_PERCPAREN: /* %( */
				if (compile_array_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_PERCBRACK: /* %[ */
				if (compile_bytearray_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_PERCBRACE: /* %{ */
				if (compile_dictionary_expression(moo) <= -1) return -1;
				break;

			case MOO_IOTOK_LBRACK: /* [ */
			{
				int n;

				/*GET_TOKEN (moo);*/
				if (store_tmpr_count_for_block (moo, cc->mth.tmpr_count) <= -1) return -1;
				cc->mth.blk_depth++;
				/*
				 * cc->mth.tmpr_count[0] contains the number of temporaries for a method.
				 * cc->mth.tmpr_count[1] contains the number of temporaries for the block plus the containing method.
				 * ...
				 * cc->mth.tmpr_count[n] contains the number of temporaries for the block plus all containing method and blocks.
				 */
				n = compile_block_expression(moo);
				cc->mth.blk_depth--;
				if (n <= -1) return -1;
				break;
			}

			case MOO_IOTOK_LPAREN:
				GET_TOKEN (moo);
				if (compile_method_expression(moo, 0) <= -1) return -1;
				if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
				{
					moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				GET_TOKEN (moo);
				break;

			default:
				moo_setsynerr (moo, MOO_SYNERR_PRIMARYINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
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
					if (compile_method_expression(moo, 0) <= -1) return -1;
					nargs++;

					if (TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN) break;

					if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA)
					{
						moo_setsynerr (moo, MOO_SYNERR_COMMA, TOKEN_LOC(moo), TOKEN_NAME(moo));
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

		/* In 'super new xxx', xxx is sent to the object returned by new.
		 * that means it is not sent to 'super' */
		to_super = 0;
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t index;
	int to_super2;
	moo_oocs_t binsel;
	moo_oow_t saved_binsels_len, binsel_offset;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL);

	do
	{
		binsel = moo->c->tok.name;
		saved_binsels_len = cc->mth.binsels.len;

		if (clone_binary_selector(moo, &binsel, &binsel_offset) <= -1) goto oops;

		GET_TOKEN (moo);

		if (compile_expression_primary(moo, MOO_NULL, MOO_NULL, 0, &to_super2) <= -1) goto oops;

		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && compile_unary_message(moo, to_super2) <= -1) goto oops;

		/* update the pointer to the cloned selector now 
		 * to be free from reallocation risk for the recursive call
		 * to compile_expression_primary(). */
		binsel.ptr = &cc->mth.binsels.ptr[binsel_offset];
		if (add_symbol_literal(moo, &binsel, 0, &index) <= -1 ||
		    emit_double_param_instruction(moo, send_message_cmd[to_super], 1, index) <= -1) goto oops;

		to_super = 0; /* In super + 2 - 3, '-' is sent to the return value of '+', not to super */
		cc->mth.binsels.len = saved_binsels_len;
	}
	while (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL);

	return 0;

oops:
	cc->mth.binsels.len = saved_binsels_len;
	return -1;
}

static int compile_keyword_message (moo_t* moo, int to_super)
{
	/*
	 * keyword-message := (keyword keyword-argument)+
	 * keyword-argument := expression-primary unary-message* binary-message*
	 */

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t index;
	int to_super2;
	moo_oocs_t kw, kwsel;
	moo_ioloc_t saved_kwsel_loc;
	moo_oow_t saved_kwsel_len;
	moo_oow_t kw_offset;
	moo_oow_t nargs = 0;

	saved_kwsel_loc = moo->c->tok.loc;
	saved_kwsel_len = cc->mth.kwsels.len;

/* TODO: optimization for ifTrue: ifFalse: whileTrue: whileFalse .. */
	do 
	{
		kw = moo->c->tok.name;
		if (clone_keyword(moo, &kw, &kw_offset) <= -1) goto oops;

		GET_TOKEN (moo);

		if (compile_expression_primary(moo, MOO_NULL, MOO_NULL, 0, &to_super2) <= -1) goto oops;
		if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && compile_unary_message(moo, to_super2) <= -1) goto oops;
		if (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL && compile_binary_message(moo, to_super2) <= -1) goto oops;

		kw.ptr = &cc->mth.kwsels.ptr[kw_offset];
		if (nargs >= MAX_CODE_NARGS)
		{
			/* 'kw' points to only one segment of the full keyword message. 
			 * if it parses an expression like 'aBlock value: 10 with: 20',
			 * 'kw' may point to 'value:' or 'with:'.
			 */
			moo_setsynerr (moo, MOO_SYNERR_ARGFLOOD, &saved_kwsel_loc, &kw); 
			goto oops;
		}

		nargs++;
	} 
	while (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD) /* loop */;

	kwsel.ptr = &cc->mth.kwsels.ptr[saved_kwsel_len];
	kwsel.len = cc->mth.kwsels.len - saved_kwsel_len;

	if (add_symbol_literal(moo, &kwsel, 0, &index) <= -1 ||
	    emit_double_param_instruction(moo, send_message_cmd[to_super], nargs, index) <= -1) goto oops;

	cc->mth.kwsels.len = saved_kwsel_len;
	return 0;

oops:
	cc->mth.kwsels.len = saved_kwsel_len;
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t noop_pos;

	do
	{
		switch (TOKEN_TYPE(moo))
		{
			case MOO_IOTOK_IDENT:
				/* insert NOOP to change to DUP_STACKTOP if there is a 
				 * cascaded message */
				noop_pos = cc->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_unary_message(moo, to_super) <= -1) return -1;

				if (TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL)
				{
					MOO_ASSERT (moo, cc->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&cc->mth.code.ptr[noop_pos], &cc->mth.code.ptr[noop_pos + 1], cc->mth.code.len - noop_pos - 1);
					cc->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = cc->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

					/* to_super is reset to 0 because a unary message
					 * has been sent to super and this binary message
					 * is following the unary message.
					 * for instance, in "super abc + 10", + is sent
					 * to the 'self' which is the result of super abc */
					if (compile_binary_message(moo, 0/*to_super*/) <= -1) return -1;
				}

				if (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
				{
					MOO_ASSERT (moo, cc->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&cc->mth.code.ptr[noop_pos], &cc->mth.code.ptr[noop_pos + 1], cc->mth.code.len - noop_pos - 1);
					cc->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = cc->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;
					/* don't pass to_super. pass 0 as it can't be the 
					 * first message after 'super' */
					if (compile_keyword_message(moo, 0/*to_super*/) <= -1) return -1;
				}
				break;

			case MOO_IOTOK_BINSEL:
				noop_pos = cc->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_binary_message(moo, to_super) <= -1) return -1;
				if (TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
				{
					MOO_ASSERT (moo, cc->mth.code.len > noop_pos);
					/*MOO_MEMMOVE (&cc->mth.code.ptr[noop_pos], &cc->mth.code.ptr[noop_pos + 1], cc->mth.code.len - noop_pos - 1);
					cc->mth.code.len--;*/
					/* eliminate the NOOP instruction */
					eliminate_instructions (moo, noop_pos, noop_pos);

					noop_pos = cc->mth.code.len;
					if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;
					/* don't pass to_super. pass 0 as it can't be the 
					 * first message after 'super' */
					if (compile_keyword_message(moo, 0/*to_super*/) <= -1) return -1;
				}
				break;

			case MOO_IOTOK_KEYWORD:
				noop_pos = cc->mth.code.len;
				if (emit_byte_instruction(moo, BCODE_NOOP) <= -1) return -1;

				if (compile_keyword_message(moo, to_super) <= -1) return -1;
				break;

			default:
				goto done;

		}

		if (TOKEN_TYPE(moo) == MOO_IOTOK_SEMICOLON)
		{
			cc->mth.code.ptr[noop_pos] = BCODE_DUP_STACKTOP;
			if (emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) return -1;
			GET_TOKEN(moo);
		}
		else 
		{
			MOO_ASSERT (moo, cc->mth.code.len > noop_pos);
			/*MOO_MEMMOVE (&cc->mth.code.ptr[noop_pos], &cc->mth.code.ptr[noop_pos + 1], cc->mth.code.len - noop_pos - 1);
			cc->mth.code.len--;*/
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_pool_t jumptoend;
	moo_oow_pool_chunk_t* jumptoend_chunk;
	moo_ioloc_t expr_loc;
	moo_oow_t i, j;
	int to_super;

	expr_loc = *TOKEN_LOC(moo);
	init_oow_pool (moo, &jumptoend);

start_over:
	if (compile_expression_primary(moo, ident, ident_loc, ident_dotted, &to_super) <= -1) goto oops;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT ||
	    TOKEN_TYPE(moo) == MOO_IOTOK_BINSEL ||
	    TOKEN_TYPE(moo) == MOO_IOTOK_KEYWORD)
	{
		if (compile_message_expression(moo, to_super) <= -1) goto oops;
	}

	if (TOKEN_TYPE(moo) == MOO_IOTOK_AND || TOKEN_TYPE(moo) == MOO_IOTOK_OR)
 	{
		int bcode;
		bcode = (TOKEN_TYPE(moo) == MOO_IOTOK_AND)? BCODE_JUMP_FORWARD_IF_FALSE: BCODE_JUMP_FORWARD_IF_TRUE;
		/* TODO: optimization if the expression is a known constant that can be determined to be boolean */
		if (add_to_oow_pool(moo, &jumptoend, cc->mth.code.len) <= -1 ||
 		    emit_single_param_instruction(moo, bcode, MAX_CODE_JUMP) <= -1 ||
 		    emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) goto oops;
		GET_TOKEN (moo);

		/* compile_method_expression() calls this function with a non-null
		 * identifer if it encounters an assignment operator after an identifer.
		 * i only allow a basic expression after a logical operator.
		 * the basic expression doesn't include an assignment expression
		 * if it is not in the parenthesis. so i nullify the following 2 variables 
		 *
		 * a := 30 > 20 and 10 > 20 is equavelent to a := (30 > 20 and 10 > 20).
		 *
		 * you will encounter a syntax error for the following expression as the
		 * expression end after y.
		 *   a := 30 > 20 and y := 10 > 20
		 * 
		 * a := 30 > 20 and (y := 10 > 20) is perfectly valid.
		 */
		ident = MOO_NULL;
		ident_loc = MOO_NULL;
		goto start_over;
 	}

	/* patch instructions that jumps to the end of if expression */
	for (jumptoend_chunk = jumptoend.head, i = 0; jumptoend_chunk; jumptoend_chunk = jumptoend_chunk->next)
	{
		/* pass expr_loc to every call to patch_long_forward_jump_instruction().
		 * it's harmless because if the first call doesn't flood, the subseqent 
		 * call will never flood either. */
		for (j = 0; j < MOO_COUNTOF(jumptoend.static_chunk.buf) && i < jumptoend.count; j++)
		{
			if (patch_long_forward_jump_instruction (moo, jumptoend_chunk->buf[j], cc->mth.code.len, &expr_loc) <= -1) goto oops;
			i++;
		}
	}

	fini_oow_pool (moo, &jumptoend);
	return 0;

oops:
	fini_oow_pool (moo, &jumptoend);
	return -1;
}

static int compile_braced_block (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	/* handle a code block enclosed in { } */

/*TODO: support local variable declaration inside {} */
	moo_oow_t code_start;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	code_start = cc->mth.code.len;
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
				moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
	}

	if (cc->mth.code.len == code_start)
	{
		/* the block doesn't contain an instruction at all */
		if (emit_byte_instruction(moo, BCODE_PUSH_NIL) <= -1) return -1;
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	return 0;
}

static int compile_conditional (moo_t* moo)
{
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LPAREN)
	{
		moo_setsynerr (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	/* a weird expression like this is also allowed for the call to  compile_method_expression() 
	 *   if (if (a == 10) { ^20 }) { ^40 }. */
	if (compile_method_expression(moo, 0) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		moo_setsynerr (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	return 0;
}

static int compile_if_expression (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_pool_t jumptoend;
	moo_oow_pool_chunk_t* jumptoend_chunk;
	moo_oow_t i, j;
	moo_oow_t jumptonext, precondpos, postcondpos, endoftrueblock;
	moo_ioloc_t if_loc, brace_loc;
	int jumpop_inst, push_true_inst, push_false_inst;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_IF || TOKEN_TYPE(moo) == MOO_IOTOK_IFNOT);
	if_loc = *TOKEN_LOC(moo);

	init_oow_pool (moo, &jumptoend);
	jumptonext = INVALID_IP;
	endoftrueblock = INVALID_IP;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_IF)
	{
		push_true_inst = BCODE_PUSH_TRUE;
		push_false_inst = BCODE_PUSH_FALSE;
		jumpop_inst = BCODE_JUMPOP_FORWARD_IF_FALSE;
	}
	else
	{
		push_true_inst = BCODE_PUSH_FALSE;
		push_false_inst = BCODE_PUSH_TRUE;
		jumpop_inst = BCODE_JUMPOP_FORWARD_IF_TRUE;
	}

	do
	{
		int falseblock = 0;

		GET_TOKEN (moo); /* get ( */
		precondpos = cc->mth.code.len;

		if (jumptonext != INVALID_IP &&
		    patch_long_forward_jump_instruction(moo, jumptonext, precondpos, &brace_loc) <= -1) goto oops;

		if (compile_conditional(moo) <= -1) goto oops;
		postcondpos = cc->mth.code.len;

		if (precondpos + 1 == postcondpos && cc->mth.code.ptr[precondpos] == push_true_inst)
		{
			/* do not generate jump */
			jumptonext = INVALID_IP;
			falseblock = 0;

			/* eliminate PUSH_TRUE as well */
			eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
			postcondpos = precondpos;
		}
		else if (precondpos + 1 == postcondpos && cc->mth.code.ptr[precondpos] == push_false_inst)
		{
			jumptonext = INVALID_IP;
			/* mark that the conditional is false. instructions will get eliminated below */
			falseblock = 1; 
		}
		else
		{
			/* remember position of the jumpop_forward_if_false instruction to be generated */
			jumptonext = cc->mth.code.len; 
			/* BCODE_JUMPOP_FORWARD_IF_FALSE is always a long jump instruction.
			 * just specify MAX_CODE_JUMP for consistency with short jump variants */
			if (emit_single_param_instruction(moo, jumpop_inst, MAX_CODE_JUMP) <= -1) goto oops;
		}

		GET_TOKEN (moo); /* get { */
		brace_loc = *TOKEN_LOC(moo);
		if (compile_braced_block(moo) <= -1) goto oops;

		if (jumptonext == INVALID_IP)
		{
			if (falseblock) 
			{
				/* the conditional was false. eliminate instructions emitted
				 * for the block attached to the conditional */
				eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
				postcondpos = precondpos;
			}
			else if (endoftrueblock == INVALID_IP) 
			{
				/* update the end position of the first true block */
				endoftrueblock = cc->mth.code.len;
			}
		}
		else
		{
			if (endoftrueblock == INVALID_IP)
			{
				/* emit an instruction to jump to the end */
				if (add_to_oow_pool(moo, &jumptoend, cc->mth.code.len) <= -1 ||
				    emit_single_param_instruction (moo, BCODE_JUMP_FORWARD_0, MAX_CODE_JUMP) <= -1) goto oops;
			}
		}

		GET_TOKEN (moo); /* get the next token after } */

		if (TOKEN_TYPE(moo) !=  MOO_IOTOK_ELSIF && TOKEN_TYPE(moo) != MOO_IOTOK_ELSIFNOT) break;

		if (TOKEN_TYPE(moo) == MOO_IOTOK_ELSIF)
		{
			push_true_inst = BCODE_PUSH_TRUE;
			push_false_inst = BCODE_PUSH_FALSE;
			jumpop_inst = BCODE_JUMPOP_FORWARD_IF_FALSE;
		}
		else
		{
			push_true_inst = BCODE_PUSH_FALSE;
			push_false_inst = BCODE_PUSH_TRUE;
			jumpop_inst = BCODE_JUMPOP_FORWARD_IF_TRUE;
		}
	}
	while (1);

	if (jumptonext != INVALID_IP &&
	    patch_long_forward_jump_instruction(moo, jumptonext, cc->mth.code.len, &brace_loc) <= -1) goto oops;

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
		eliminate_instructions (moo, endoftrueblock, cc->mth.code.len - 1);
	}

	/* patch instructions that jumps to the end of if expression */
	for (jumptoend_chunk = jumptoend.head, i = 0; jumptoend_chunk; jumptoend_chunk = jumptoend_chunk->next)
	{
		/* pass if_loc to every call to patch_long_forward_jump_instruction().
		 * it's harmless because if the first call doesn't flood, the subseqent 
		 * call will never flood either. */
		for (j = 0; j < MOO_COUNTOF(jumptoend.static_chunk.buf) && i < jumptoend.count; j++)
		{
			if (patch_long_forward_jump_instruction (moo, jumptoend_chunk->buf[j], cc->mth.code.len, &if_loc) <= -1) goto oops;
			i++;
		}
	}

	fini_oow_pool (moo, &jumptoend);
	return 0;

oops:
	fini_oow_pool (moo, &jumptoend);
	return -1;
}

static int compile_while_expression (moo_t* moo) /* or compile_until_expression */
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_ioloc_t while_loc, brace_loc;
	moo_oow_t precondpos, postcondpos, prebbpos, postbbpos;
	int cond_style = 0, loop_pushed = 0, is_until_loop;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_WHILE || TOKEN_TYPE(moo) == MOO_IOTOK_UNTIL);

	is_until_loop = (TOKEN_TYPE(moo) == MOO_IOTOK_UNTIL);
	while_loc = *TOKEN_LOC(moo);

	GET_TOKEN (moo); /* get (, verification is done inside compile_conditional() */
	precondpos = cc->mth.code.len;
	if (compile_conditional(moo) <= -1) goto oops;

	postcondpos = cc->mth.code.len;
	if (precondpos + 1 == postcondpos)
	{
		moo_uint8_t inst1, inst2;

		if (is_until_loop)
		{
			inst1 = BCODE_PUSH_FALSE;
			inst2 = BCODE_PUSH_TRUE;
		}
		else
		{
			inst1 = BCODE_PUSH_TRUE;
			inst2 = BCODE_PUSH_FALSE;
		}

		/* simple optimization - 
		 *   if the conditional is known to be true, emit the absolute jump instruction.
		 *   if it is known to be false, kill all generated instructions. */
		if (cc->mth.code.ptr[precondpos] == inst1)
		{
			/* the conditional is always true for while, or false for until*/
			cond_style = 1;
			eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
			postcondpos = precondpos;
		}
		else if (cc->mth.code.ptr[precondpos] == inst2)
		{
			/* the conditional is always false for while, or false for until */
			cond_style = -1;
		}
/* TODO: at least check some other literals for optimization. 
 *       most literal values must be evaluate to true. */
	}

	if (cond_style != 1)
	{
		/* BCODE_JUMPOP_FORWARD_IF_FALSE is always a long jump instruction.
		 * just specify MAX_CODE_JUMP for consistency with short jump variants */
		if (emit_single_param_instruction(moo, (is_until_loop? BCODE_JUMPOP_FORWARD_IF_TRUE: BCODE_JUMPOP_FORWARD_IF_FALSE), MAX_CODE_JUMP) <= -1) goto oops;
	}

	/* remember information about this while loop. */
	if (push_loop(moo, MOO_LOOP_WHILE, precondpos) <= -1) goto oops;
	loop_pushed = 1;

	GET_TOKEN (moo); /* get { */
	brace_loc = *TOKEN_LOC(moo);
	prebbpos = cc->mth.code.len;
	if (compile_braced_block(moo) <= -1) goto oops;
	GET_TOKEN (moo); /* get the next token after } */
	postbbpos = cc->mth.code.len;

	if (prebbpos + 1 == postbbpos && cc->mth.code.ptr[prebbpos] == BCODE_PUSH_NIL)
	{
		/* optimization -
		 *  the braced block is kind of empty as it only pushes nil.
		 *  get rid of this push instruction and don't generate the POP_STACKTOP */
		eliminate_instructions (moo, prebbpos, cc->mth.code.len - 1);
	}
	else if (prebbpos < postbbpos)
	{
		/* emit an instruction to pop the value pushed by the braced block */
		if (emit_byte_instruction(moo, BCODE_POP_STACKTOP) <= -1) goto oops;
	}

	/* emit an instruction to jump back to the condition */
	if (emit_backward_jump_instruction(moo, BCODE_JUMP_BACKWARD_0, cc->mth.code.len - precondpos) <= -1) 
	{
		if (moo->errnum == MOO_ERANGE) 
		{
			/* the jump offset is out of the representable range by the offset
			 * portion of the jump instruction */
			moo_setsynerr (moo, MOO_SYNERR_BLKFLOOD, &while_loc, MOO_NULL);
		}
		goto oops;
	}

	if (cond_style != 1)
	{
		/* patch the jump instruction */
		if (patch_long_forward_jump_instruction(moo, postcondpos, cc->mth.code.len, &brace_loc) <= -1) goto oops;
	}

	if (cond_style == -1) 
	{
		/* optimization - get rid of instructions generated for the while
		 * loop including the conditional as the condition was false */
		eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
	}

	/* patch the jump instructions for break */
	if (update_loop_breaks(moo, cc->mth.code.len) <= -1) goto oops;

	/* destroy the loop information stored earlier in this function  */
	pop_loop (moo);
	loop_pushed = 0;

	/* push nil as a result of the while expression. TODO: is it the best value? anything else? */
	if (emit_byte_instruction(moo, BCODE_PUSH_NIL) <= -1) goto oops;

	return 0;

oops:
	if (loop_pushed) pop_loop (moo);
	return -1;
}

static int compile_do_while_expression (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_ioloc_t do_loc;
	moo_oow_t precondpos, postcondpos, prebbpos, postbbpos;
	int jbinst = 0, loop_pushed = 0, is_until_loop;
	moo_loop_t* loop = MOO_NULL;

	MOO_ASSERT (moo, TOKEN_TYPE(moo) == MOO_IOTOK_DO);
	do_loc = *TOKEN_LOC(moo);

	GET_TOKEN (moo); /* get { */
	prebbpos = cc->mth.code.len;

	/* remember information about this loop. 
	 * position of the conditional is not known yet.*/
	if (push_loop (moo, MOO_LOOP_DO_WHILE, prebbpos) <= -1) goto oops;
	loop_pushed = 1;

	if (compile_braced_block(moo) <= -1) goto oops;

	GET_TOKEN (moo); /* get the next token after } */
	if (TOKEN_TYPE(moo) == MOO_IOTOK_UNTIL) is_until_loop = 1;
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_WHILE) is_until_loop = 0;
	else
	{
		moo_setsynerr (moo, MOO_SYNERR_WHILE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}
	GET_TOKEN (moo); /* get ( */
	postbbpos = cc->mth.code.len;

	if (prebbpos + 1 == postbbpos && cc->mth.code.ptr[prebbpos] == BCODE_PUSH_NIL)
	{
		/* optimization -
		 *  the braced block is kind of empty as it only pushes nil.
		 *  get rid of this push instruction and don't generate the POP_STACKTOP */
		eliminate_instructions (moo, prebbpos, cc->mth.code.len - 1);
		precondpos = prebbpos;
	}
	else if (prebbpos < postbbpos)
	{
		/* emit code to pop the value pushed by the braced block */
		if (emit_byte_instruction (moo, BCODE_POP_STACKTOP) <= -1) goto oops;
	}

	precondpos = cc->mth.code.len;

	/* update jump instructions emitted for continue */
	if (update_loop_continues (moo, precondpos) <= -1) goto oops;
	/* cannnot destroy the loop information because of pending jump updates
	 * for break. but need to unlink it as the conditional is not really
	 * part of the loop body */
	loop = unlink_loop (moo); 

	if (compile_conditional (moo) <= -1) goto oops;
	postcondpos = cc->mth.code.len;
	jbinst = (is_until_loop? BCODE_JUMPOP_BACKWARD_IF_FALSE_0: BCODE_JUMPOP_BACKWARD_IF_TRUE_0);
	if (precondpos + 1 == postcondpos)
	{
		/* simple optimization - 
		 *   if the conditional is known to be true, emit the absolute jump instruction.
		 *   if it is known to be false, kill all generated instructions. */
		if (cc->mth.code.ptr[precondpos] == (is_until_loop? BCODE_PUSH_FALSE: BCODE_PUSH_TRUE))
		{
			/* the conditional is always true. eliminate PUSH_TRUE and emit an absolute jump */
			eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
			postcondpos = precondpos;
			jbinst = BCODE_JUMP_BACKWARD_0;
		}
		else if (cc->mth.code.ptr[precondpos] == (is_until_loop? BCODE_PUSH_TRUE: BCODE_PUSH_FALSE))
		{
			/* the conditional is always false. eliminate PUSH_FALSE and don't emit jump */
			eliminate_instructions (moo, precondpos, cc->mth.code.len - 1);
			postcondpos = precondpos;
			goto skip_emitting_jump_backward;
		}
	}

	if (emit_backward_jump_instruction (moo, jbinst, cc->mth.code.len - prebbpos) <= -1) 
	{
		if (moo->errnum == MOO_ERANGE) 
		{
			/* the jump offset is out of the representable range by the offset
			 * portion of the jump instruction */
			moo_setsynerr (moo, MOO_SYNERR_BLKFLOOD, &do_loc, MOO_NULL);
		}
		goto oops;
	}

skip_emitting_jump_backward:
	GET_TOKEN (moo); /* get the next token after ) */

	/* update jump instructions emitted for break */
	if (update_loop_jumps (moo, &loop->break_ip_pool, cc->mth.code.len) <= -1) return -1;
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

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oocs_t assignee;
	moo_oow_t index;
	int ret = 0;

	MOO_ASSERT (moo, pop == 0 || pop == 1);
	MOO_MEMSET (&assignee, 0, MOO_SIZEOF(assignee));

	if (TOKEN_TYPE(moo) == MOO_IOTOK_IF || TOKEN_TYPE(moo) == MOO_IOTOK_IFNOT)
	{
		if (compile_if_expression (moo) <= -1) return -1;
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_WHILE ||
	         TOKEN_TYPE(moo) == MOO_IOTOK_UNTIL)
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
			assignee.ptr = &cc->mth.assignees.ptr[assignee_offset];
			MOO_ASSERT (moo, cc->super_oop != MOO_NULL && cc->self_oop != MOO_NULL);
			if (get_variable_info(moo, &assignee, &assignee_loc, assignee_dotted, &var) <= -1) goto oops;

			switch (var.type)
			{
				case VAR_ARGUMENT: /* TODO: consider if assigning to an argument should be disallowed */
				case VAR_TEMPORARY:
				{
				#if defined(MOO_USE_CTXTEMPVAR)
					if (cc->mth.blk_depth > 0)
					{
						moo_oow_t i;

						/* if a temporary variable is accessed inside a block,
						 * use a special instruction to indicate it */
						MOO_ASSERT (moo, var.pos < cc->mth.blk_tmprcnt[cc->mth.blk_depth]);
						for (i = cc->mth.blk_depth; i > 0; i--)
						{
							if (var.pos >= cc->mth.blk_tmprcnt[i - 1])
							{
								if (emit_double_param_instruction(moo, (pop? BCODE_POP_INTO_CTXTEMPVAR_0: BCODE_STORE_INTO_CTXTEMPVAR_0), cc->mth.blk_depth - i, var.pos - cc->mth.blk_tmprcnt[i - 1]) <= -1) return -1;
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
					if (add_literal(moo, (moo_oop_t)var.u.gbl, &index) <= -1 ||
					    emit_single_param_instruction(moo, (pop? BCODE_POP_INTO_OBJECT_0: BCODE_STORE_INTO_OBJECT_0), index) <= -1) return -1;
					ret = pop;
					break;

				default:
					moo_seterrnum (moo, MOO_EINTERN);
					goto oops;
			}
		}
		else 
		{
			/* what is held in assignee is not an assignee any more.
			 * potentially it is a variable or object reference
			 * to be pushed on to the stack */
			assignee.ptr = &cc->mth.assignees.ptr[assignee_offset];
			if (compile_basic_expression(moo, &assignee, &assignee_loc, assignee_dotted) <= -1) goto oops;
		}
	}
	else 
	{
		assignee.len = 0;
		if (compile_basic_expression(moo, MOO_NULL, MOO_NULL, 0) <= -1) goto oops;
	}

	cc->mth.assignees.len -= assignee.len;
	return ret;

oops:
	cc->mth.assignees.len -= assignee.len;
	return -1;
}

static int compile_special_statement (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

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
		if (!cc->mth.loop)
		{
			/* break outside a loop */
			moo_setsynerr (moo, MOO_SYNERR_NOTINLOOP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		if (cc->mth.loop->blkcount > 0)
		{
			/* break cannot cross boundary of a block */
			moo_setsynerr (moo, MOO_SYNERR_INBLOCK, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo); /* read the next token to break */
		return inject_break_to_loop (moo);
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_CONTINUE)
	{
		if (!cc->mth.loop)
		{
			moo_setsynerr (moo, MOO_SYNERR_NOTINLOOP, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
		if (cc->mth.loop->blkcount > 0)
		{
			/* continue cannot cross boundary of a block */
			moo_setsynerr (moo, MOO_SYNERR_INBLOCK, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo); /* read the next token to continue */

		return (cc->mth.loop->type == MOO_LOOP_DO_WHILE)?
			inject_continue_to_loop (moo): /* in a do-while loop, the position to the conditional is not known yet */
			emit_backward_jump_instruction (moo, BCODE_JUMP_BACKWARD_0, cc->mth.code.len - cc->mth.loop->startpos);
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
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

		preexprpos = cc->mth.code.len;
		n = compile_method_expression(moo, 1);
		if (n <= -1) return -1;

		/* if n is 1, no stack popping is required as POP_INTO_XXX has been
		 * emitted in place of STORE_INTO_XXX. */
		if (n == 0)
		{
			if (preexprpos + 1 == cc->mth.code.len)
			{
/* TODO: MORE optimization. if expresssion is a literal, no push and pop are required. check for multie-byte instructions as well */
				switch (cc->mth.code.ptr[preexprpos])
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
						eliminate_instructions (moo, preexprpos, cc->mth.code.len - 1);
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
				moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
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
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oop_char_t name; /* selector */
	moo_oop_method_t mth; /* method */
#if defined(MOO_USE_METHOD_TRAILER)
	/* nothing extra */
#else
	moo_oop_byte_t code;
#endif
	moo_oow_t tmp_count = 0;
	moo_oow_t i;
	moo_ooi_t preamble_code, preamble_index, preamble_flags;

	name = (moo_oop_char_t)moo_makesymbol(moo, cc->mth.name.ptr, cc->mth.name.len);
	if (!name) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&name); tmp_count++;

	/* The variadic data part passed to moo_instantiate() is not GC-safe. 
	 * let's delay initialization of variadic data a bit. */
#if defined(MOO_USE_METHOD_TRAILER)
	mth = (moo_oop_method_t)moo_instantiatewithtrailer (moo, moo->_method, cc->mth.literals.count, cc->mth.code.ptr, cc->mth.code.len);
#else
	mth = (moo_oop_method_t)moo_instantiate (moo, moo->_method, MOO_NULL, cc->mth.literals.count);
#endif
	if (!mth) goto oops;

	for (i = 0; i < cc->mth.literals.count; i++)
	{
		/* let's do the variadic data initialization here */
		mth->slot[i] = cc->mth.literals.ptr[i];
	}
	moo_pushtmp (moo, (moo_oop_t*)&mth); tmp_count++;

#if defined(MOO_USE_METHOD_TRAILER)
	/* do nothing */
#else
	code = (moo_oop_byte_t)moo_instantiate(moo, moo->_byte_array, cc->mth.code.ptr, cc->mth.code.len);
	if (!code) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&code); tmp_count++;
#endif

	preamble_code = MOO_METHOD_PREAMBLE_NONE;
	preamble_index = 0;
	preamble_flags = 0;

	if (cc->mth.pftype <= 0)
	{
		/* no primitive is set - perform some mutation for simplicity and efficiency */
		if (cc->mth.code.len <= 0)
		{
			preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
		}
		else
		{
			if (cc->mth.code.ptr[0] == BCODE_RETURN_RECEIVER)
			{
				preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
			}
			else if (cc->mth.code.len > 1 && cc->mth.code.ptr[1] == BCODE_RETURN_STACKTOP)
			{
				switch (cc->mth.code.ptr[0])
				{
					case BCODE_PUSH_RECEIVER:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER;
						break;

					case BCODE_PUSH_CONTEXT:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_CONTEXT;
						break;

					case BCODE_PUSH_PROCESS:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_PROCESS;
						break;

					case BCODE_PUSH_RECEIVER_NS:
						preamble_code = MOO_METHOD_PREAMBLE_RETURN_RECEIVER_NS;
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
						preamble_index = cc->mth.code.ptr[0] & 0x7; /* low 3 bits */
						break;
				}
			}
			else if (cc->mth.code.len > MOO_BCODE_LONG_PARAM_SIZE + 1 &&
			         cc->mth.code.ptr[MOO_BCODE_LONG_PARAM_SIZE + 1] == BCODE_RETURN_STACKTOP)
			{
				int i;
				switch (cc->mth.code.ptr[0])
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
							preamble_index = (preamble_index << 8) | cc->mth.code.ptr[i];
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
	else if (cc->mth.pftype == PFTYPE_NUMBERED)
	{
		preamble_code = MOO_METHOD_PREAMBLE_PRIMITIVE;
		preamble_index = cc->mth.pfnum;
	}
	else if (cc->mth.pftype == PFTYPE_NAMED)
	{
		preamble_code = MOO_METHOD_PREAMBLE_NAMED_PRIMITIVE;
		preamble_index = cc->mth.pfnum; /* index to literal frame */
	}
	else if (cc->mth.pftype == PFTYPE_EXCEPTION)
	{
		preamble_code = MOO_METHOD_PREAMBLE_EXCEPTION;
		preamble_index = 0;
	}
	else 
	{
		MOO_ASSERT (moo, cc->mth.pftype == PFTYPE_ENSURE);
		preamble_code = MOO_METHOD_PREAMBLE_ENSURE;
		preamble_index = 0;
	}

	preamble_flags |= cc->mth.variadic; /* MOO_METHOD_PREAMBLE_FLAG_VARIADIC or MOO_METHOD_PREAMBLE_FLAG_LIBERAL */
	if (cc->mth.type == MOO_METHOD_DUAL) preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_DUAL;
	if (cc->mth.lenient) preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_LENIENT;

	MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(preamble_index));

	mth->owner = cc->self_oop;
	mth->name = name;
	mth->preamble = MOO_SMOOI_TO_OOP(MOO_METHOD_MAKE_PREAMBLE(preamble_code, preamble_index, preamble_flags));
	/*mth->preamble_data[0] = MOO_SMOOI_TO_OOP(0);
	mth->preamble_data[1] = MOO_SMOOI_TO_OOP(0);*/
	mth->preamble_data[0] = MOO_SMPTR_TO_OOP(0);
	mth->preamble_data[1] = MOO_SMPTR_TO_OOP(0);
	mth->tmpr_count = MOO_SMOOI_TO_OOP(cc->mth.tmpr_count);
	mth->tmpr_nargs = MOO_SMOOI_TO_OOP(cc->mth.tmpr_nargs);

#if defined(MOO_USE_METHOD_TRAILER)
	/* do nothing */
#else
	mth->code = code;
#endif

	/*TODO: preserve source??? mth->text = cc->mth.text
the compiler must collect all source method string collected so far.
need to write code to collect string.
*/

#if defined(MOO_DEBUG_COMPILER)
	moo_decode (moo, mth, &cc->fqn);
#endif

	if (cc->mth.type == MOO_METHOD_DUAL)
	{
		if (!moo_putatdic(moo, cc->self_oop->mthdic[MOO_METHOD_INSTANCE], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
		if (!moo_putatdic(moo, cc->self_oop->mthdic[MOO_METHOD_CLASS], (moo_oop_t)name, (moo_oop_t)mth)) 
		{
			/* 'name' is a symbol created of cc->mth.name. so use it as a key for deletion */
			moo_deletedic (moo, cc->self_oop->mthdic[MOO_METHOD_INSTANCE], &cc->mth.name);
			goto oops;
		}
	}
	else
	{
		MOO_ASSERT (moo, cc->mth.type < MOO_COUNTOF(cc->self_oop->mthdic));
		if (!moo_putatdic(moo, cc->self_oop->mthdic[cc->mth.type], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
	}

	moo_poptmps (moo, tmp_count); tmp_count = 0;

	return 0;

oops:
	moo_poptmps (moo, tmp_count);
	return -1;
}

static void clear_pooldic_import_data (moo_t* moo, moo_pooldic_import_data_t* pdimp)
{
	if (pdimp->dcl.ptr) moo_freemem (moo, pdimp->dcl.ptr);
	if (pdimp->oops) moo_freemem (moo, pdimp->oops);
	MOO_MEMSET (pdimp, 0, MOO_SIZEOF(*pdimp));
}

static void clear_method_data (moo_t* moo, moo_method_data_t* mth)
{
	if (mth->text.ptr) moo_freemem (moo, mth->text.ptr);
	if (mth->assignees.ptr) moo_freemem (moo, mth->assignees.ptr);
	if (mth->binsels.ptr) moo_freemem (moo, mth->binsels.ptr);
	if (mth->kwsels.ptr) moo_freemem (moo, mth->kwsels.ptr);
	if (mth->name.ptr) moo_freemem (moo, mth->name.ptr);
	if (mth->tmprs.ptr) moo_freemem (moo, mth->tmprs.ptr);
	if (mth->code.ptr) moo_freemem (moo, mth->code.ptr);
	if (mth->literals.ptr) moo_freemem (moo, mth->literals.ptr);
	if (mth->blk_tmprcnt) moo_freemem (moo, mth->blk_tmprcnt);
	MOO_MEMSET (mth, 0, MOO_SIZEOF(*mth));
}

static void reset_method_data (moo_t* moo, moo_method_data_t* mth)
{
	/* unlike clear_method_data(), this function doesn't free memory allocated */
	mth->type = MOO_METHOD_INSTANCE;
	mth->primitive = 0;
	mth->lenient = 0;
	mth->text.len = 0;
	mth->assignees.len = 0;
	mth->binsels.len = 0;
	mth->kwsels.len = 0;
	mth->name.len = 0;
	MOO_MEMSET (&mth->name_loc, 0, MOO_SIZEOF(mth->name_loc));
	mth->variadic = 0;
	mth->tmprs.len = 0;
	mth->tmpr_count = 0;
	mth->tmpr_nargs = 0;
	mth->literals.count = 0;
	mth->pftype = PFTYPE_NONE;
	mth->pfnum = 0;
	mth->blk_depth = 0;
	mth->code.len = 0;
}

static int process_method_modifiers (moo_t* moo, moo_method_data_t* mth)
{
	GET_TOKEN (moo);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		do
		{
			if (is_token_symbol(moo, VOCA_CLASS_S))
			{
				/* method(#class) */
				if (mth->type == MOO_METHOD_CLASS || mth->type == MOO_METHOD_DUAL)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				mth->type = MOO_METHOD_CLASS;
				GET_TOKEN (moo);
			}
			else if (is_token_symbol(moo, VOCA_DUAL_S))
			{
				/* method(#dual) */
				if (mth->type == MOO_METHOD_CLASS || mth->type == MOO_METHOD_DUAL)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				mth->type = MOO_METHOD_DUAL;
				GET_TOKEN (moo);
			}
			else if (is_token_symbol(moo, VOCA_PRIMITIVE_S))
			{
				/* method(#primitive) */
				if (mth->primitive)
				{
					/* #primitive duplicate modifier */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				mth->primitive = 1;
				GET_TOKEN (moo);
			}
			else if (is_token_symbol(moo, VOCA_LENIENT_S))
			{
				/* method(#lenient) */
				if (mth->lenient)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				mth->lenient = 1;
				GET_TOKEN (moo);
			}
			else if (is_token_symbol(moo, VOCA_VARIADIC_S) ||
					 is_token_symbol(moo, VOCA_LIBERAL_S))
			{
				/* method(#variadic) or method(#liberal) */
				if (mth->variadic)
				{
					/* #variadic duplicate modifier */
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}

				if (is_token_symbol(moo, VOCA_LIBERAL_S))
					mth->variadic = MOO_METHOD_PREAMBLE_FLAG_LIBERAL;
				else
					mth->variadic = MOO_METHOD_PREAMBLE_FLAG_VARIADIC;

				GET_TOKEN (moo);
			}
			else if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
			{
				/* no modifier is present */
				moo_setsynerr (moo, MOO_SYNERR_MODIFIER, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
			else
			{
				/* invalid modifier */
				moo_setsynerr (moo, MOO_SYNERR_MODIFIERINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully ) */
			GET_TOKEN (moo); /* get the token after , */
		}
		while (1);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		/* ) expected */
		moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	return 0;
}

static int resolve_primitive_method (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oocs_t mthname;

	/* the primitive method must be of this form
	 *  method(#primitive) method_name.
	 */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		/* . expected */
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	/*
	 * remove all leading underscores from the method name when building a primitive
	 * identifer. multiple methods can map to the same primitive handler.
	 * for class X, you may have method(#primitive) aa and method(#primitive) _aa
	 * to map to the X_aa primitive handler.
	 */
	mthname = cc->mth.name;
	while (mthname.len > 0)
	{
		if (*mthname.ptr != '_') break;
		mthname.ptr++;
		mthname.len--;
	}

	if (mthname.len == 0)
	{
		MOO_DEBUG2 (moo, "Invalid primitive function name - %.*js\n", cc->mth.name.len, cc->mth.name.ptr);
		moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, &cc->mth.name_loc, &cc->mth.name);
		return -1;
	}
	if (cc->self_oop->modname == moo->_nil)
	{
		/* no module name specified in the class definition using 'from'.
		 * it's a builtin primitive function */

		moo_oow_t savedlen;
		moo_ooi_t pfnum;
		moo_pfbase_t* pfbase;

		/* primitive identifer = classname_methodname */

		/* compose the identifer into the back of the cls.modname buffer.
		 * i'll revert it when done. */
		savedlen = cc->modname.len;

		if (copy_string_to(moo, &cc->name, &cc->modname, &cc->modname_capa, 1, '\0') <= -1 ||
		    copy_string_to(moo, &mthname, &cc->modname, &cc->modname_capa, 1, '_') <= -1)
		{
			cc->modname.len = savedlen;
			return -1;
		}

		pfbase = moo_getpfnum (moo, &cc->modname.ptr[savedlen], cc->modname.len - savedlen, &pfnum);
		if (!pfbase)
		{
			MOO_DEBUG2 (moo, "Cannot find intrinsic primitive function - %.*js\n", 
				cc->modname.len - savedlen, &cc->modname.ptr[savedlen]);
			moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, &cc->mth.name_loc, &cc->mth.name);
			cc->modname.len = savedlen;
			return -1;
		}

		if (cc->mth.tmpr_nargs < pfbase->minargs || cc->mth.tmpr_nargs > pfbase->maxargs)
		{
			MOO_DEBUG5 (moo, "Unsupported argument count in primitive method definition of %.*js - %zd-%zd expected, %zd specified\n", 
				cc->modname.len - savedlen, &cc->modname.ptr[savedlen],
				pfbase->minargs, pfbase->maxargs, cc->mth.tmpr_nargs);
			moo_setsynerr (moo, MOO_SYNERR_PFARGDEFINVAL, &cc->mth.name_loc, &cc->mth.name);
			cc->modname.len = savedlen;
			return -1;
		}

		cc->modname.len = savedlen;

		MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(pfnum));
		cc->mth.pftype = PFTYPE_NUMBERED; 
		cc->mth.pfnum = pfnum;
	}
	else
	{
		moo_oow_t litidx, savedlen;
		moo_oocs_t tmp;
		moo_pfbase_t* pfbase;

		/* combine the module name and the method name delimited by a period
		 * when doing it, let me reuse the cls.modname buffer and restore it
		 * back once done */
		savedlen = cc->modname.len;

		MOO_ASSERT (moo, MOO_CLASSOF(moo, cc->self_oop->modname) == moo->_symbol);
		tmp.ptr = MOO_OBJ_GET_CHAR_SLOT(cc->self_oop->modname);
		tmp.len = MOO_OBJ_GET_SIZE(cc->self_oop->modname);

		if (copy_string_to (moo, &tmp, &cc->modname, &cc->modname_capa, 1, '\0') <= -1 ||
		    copy_string_to (moo, &mthname, &cc->modname, &cc->modname_capa, 1, '.') <= -1 ||
		    add_symbol_literal(moo, &cc->modname, savedlen, &litidx) <= -1)
		{
			cc->modname.len = savedlen;
			return -1;
		}

		/* check if the primitive function exists at the compile time and perform some checks.
		 * see compile_method_primitive() for similar checks */
		pfbase = moo_querymod(moo, &cc->modname.ptr[savedlen], cc->modname.len - savedlen, MOO_NULL);
		if (!pfbase)
		{
			MOO_DEBUG2 (moo, "Cannot find module primitive function - %.*js\n", 
				cc->modname.len - savedlen, &cc->modname.ptr[savedlen]);
			moo_setsynerr (moo, MOO_SYNERR_PFIDINVAL, &cc->mth.name_loc, &cc->mth.name);
			cc->modname.len = savedlen;
			return -1;
		}

		if (cc->mth.tmpr_nargs < pfbase->minargs || cc->mth.tmpr_nargs > pfbase->maxargs)
		{
			MOO_DEBUG5 (moo, "Unsupported argument count in primitive method definition of %.*js - %zd-%zd expected, %zd specified\n", 
				cc->modname.len - savedlen, &cc->modname.ptr[savedlen],
				pfbase->minargs, pfbase->maxargs, cc->mth.tmpr_nargs);
			moo_setsynerr (moo, MOO_SYNERR_PFARGDEFINVAL, &cc->mth.name_loc, &cc->mth.name);
			cc->modname.len = savedlen;
			return -1;
		}

		cc->modname.len = savedlen;
		/* the symbol added must be the first literal to the current method.
		 * so this condition must be true. */
		MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(litidx));

		/* external named primitive containing a period. */
		cc->mth.pftype = PFTYPE_NAMED; 
		cc->mth.pfnum = litidx;
	}

	return 0;
}

static int __compile_method_definition (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process method modifiers  */
		if (process_method_modifiers(moo, &cc->mth) <= -1) return -1;
	}

	if (compile_method_name(moo, &cc->mth) <= -1) return -1;

	if (cc->mth.primitive)
	{
		/* the primitive method doesn't have body */
		if (resolve_primitive_method(moo) <= -1) return -1;
	}
	else
	{
		if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
		{
			/* { expected */
			moo_setsynerr (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);

		if (compile_method_temporaries(moo) <= -1 ||
		    compile_method_pragma(moo) <= -1 ||
		    compile_method_statements(moo) <= -1) return -1;

		if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
		{
			/* } expected */
			moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
	}

	GET_TOKEN (moo);

	/* add a compiled method to the method dictionary */
	if (add_compiled_method(moo) <= -1) return -1;

	return 0;
}

static int compile_method_definition (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int n;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	/* clear data required to compile a method */
	cc->mth.active = 1;
	reset_method_data (moo, &cc->mth);

	n = __compile_method_definition(moo);

	cc->mth.active = 0;

	return n;
}

static int make_getter_method (moo_t* moo, const moo_oocs_t* name, const var_info_t* var)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	MOO_ASSERT (moo, cc->mth.name.len == 0);
	if (add_method_name_fragment(moo, &cc->mth, name) <= -1) return -1;

	switch (var->type)
	{
		case VAR_INSTANCE:
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_INSTANCE);
			if (emit_single_param_instruction(moo, BCODE_PUSH_INSTVAR_0, var->pos) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_STACKTOP) <= -1) return -1;
			break;

		case VAR_CLASSINST:
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_CLASS);
			if (emit_single_param_instruction(moo, BCODE_PUSH_INSTVAR_0, var->pos) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_STACKTOP) <= -1) return -1;
			break;

		case VAR_CLASS:
		{
			moo_oow_t index;
			MOO_ASSERT (moo, var->cls != MOO_NULL);
			MOO_ASSERT (moo, var->cls == cc->self_oop);
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_CLASS);

			if (add_literal(moo, (moo_oop_t)var->cls, &index) <= -1 ||
			    emit_double_param_instruction(moo, BCODE_PUSH_OBJVAR_0, var->pos, index) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_STACKTOP) <= -1) return -1;
			break;
		}

		default:
			MOO_DEBUG1 (moo, "internal error - invalid variable type in make_getter_method - %d\n", (int)var->type);
			moo_seterrnum (moo, MOO_EINTERN);
			return -1;
	}

	return add_compiled_method(moo);
}


static int make_setter_method (moo_t* moo, const moo_oocs_t* name, const var_info_t* var)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	static moo_ooch_t colon = ':';
	static moo_oocs_t colons = { &colon, 1 };

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	MOO_ASSERT (moo, cc->mth.name.len == 0);
	if (add_method_name_fragment(moo, &cc->mth, name) <= -1 ||
	    add_method_name_fragment(moo, &cc->mth, &colons) <= -1) return -1;

	switch (var->type)
	{
		case VAR_INSTANCE:
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_INSTANCE);
			if (emit_single_param_instruction(moo, BCODE_PUSH_TEMPVAR_0, 0) <= -1 ||
			    emit_single_param_instruction(moo, BCODE_POP_INTO_INSTVAR_0, var->pos) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_RECEIVER) <= -1) return -1;
			break;

		case VAR_CLASSINST:
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_CLASS);
			if (emit_single_param_instruction(moo, BCODE_PUSH_TEMPVAR_0, 0) <= -1 ||
			    emit_single_param_instruction(moo, BCODE_POP_INTO_INSTVAR_0, var->pos) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_RECEIVER) <= -1) return -1;
			break;

		case VAR_CLASS:
		{
			moo_oow_t index;
			MOO_ASSERT (moo, var->cls != MOO_NULL);
			MOO_ASSERT (moo, var->cls == cc->self_oop);
			MOO_ASSERT (moo, cc->mth.type == MOO_METHOD_CLASS);

			if (add_literal(moo, (moo_oop_t)var->cls, &index) <= -1 ||
			    emit_single_param_instruction(moo, BCODE_PUSH_TEMPVAR_0, 0) <= -1 ||
			    emit_double_param_instruction(moo, BCODE_POP_INTO_OBJVAR_0, var->pos, index) <= -1 ||
			    emit_byte_instruction(moo, BCODE_RETURN_RECEIVER) <= -1) return -1;
			break;
		}

		default:
			MOO_DEBUG1 (moo, "internal error - invalid variable type in make_setter_method - %d\n", (int)var->type);
			moo_seterrnum (moo, MOO_EINTERN);
			return -1;
	}

	return add_compiled_method(moo);
}

static int make_getters_and_setters (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t i, var_type;
	moo_oocs_t var_name;
	moo_ioloc_t fake_loc;
	var_info_t var_info;
	int x;

	fake_loc.line = 0;
	fake_loc.colm = 0;
	for (var_type = VAR_INSTANCE; var_type <= VAR_CLASS; var_type++)
	{
		for (i = 0; i < cc->var[var_type].initv_count; i++)
		{
			if (!cc->var[var_type].initv[i].flags) continue;

			/* cc->mth.type needs to be set because get_variable_info()
			 * uses it to validate variable's accessibility */
			reset_method_data (moo, &cc->mth);
			cc->mth.type = (var_type == VAR_INSTANCE? MOO_METHOD_INSTANCE: MOO_METHOD_CLASS);

			/* the following two function calls must not fail unless the compiler
			 * is buggy. */

			x = fetch_word_from_string (&cc->var[var_type].str, i, &var_name);
			MOO_ASSERT (moo, x >= 0);
			x = get_variable_info (moo, &var_name, &fake_loc, 0, &var_info);
			MOO_ASSERT (moo, x >= 0);

			MOO_ASSERT (moo, var_info.type == var_type);

			if (cc->var[var_type].initv[i].flags & VARACC_GETTER)
			{
				/* the method data has been reset above. */
				if (make_getter_method (moo, &var_name, &var_info) <= -1) return -1;
			}

			if (cc->var[var_type].initv[i].flags & VARACC_SETTER)
			{
				/* i set the method data here because make_getter_method()
				 * pollutes it if triggered */
				reset_method_data (moo, &cc->mth);
				cc->mth.type = (var_type == VAR_INSTANCE? MOO_METHOD_INSTANCE: MOO_METHOD_CLASS);

				/* hack to simulate a parameter. note i don't manipulate tmprs or tmprs_capa
				 * because there is no method body code to process. i simply generate a setter
				 * method */
				cc->mth.tmpr_count = 1; 
				cc->mth.tmpr_nargs = 1;

				MOO_ASSERT (moo, var_info.type == var_type);
				if (make_setter_method (moo, &var_name, &var_info) <= -1) return -1;
			}
		}
	}

	return 0;
}

static int make_default_initial_values (moo_t* moo, var_type_t var_type)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t initv_count, super_initv_count;

	MOO_ASSERT (moo, var_type == VAR_INSTANCE || var_type == VAR_CLASSINST);
	MOO_ASSERT (moo, VAR_INSTANCE == 0);
	MOO_ASSERT (moo, VAR_CLASSINST == 1);

	initv_count = cc->var[var_type].initv_count;
	if (cc->super_oop != moo->_nil && ((moo_oop_class_t)cc->super_oop)->initv[var_type] != moo->_nil)
	{
		super_initv_count = MOO_OBJ_GET_SIZE(((moo_oop_class_t)cc->super_oop)->initv[var_type]);
	}
	else
	{
		super_initv_count = 0;
		
	}
	initv_count += super_initv_count;

	if (initv_count > 0)
	{
		moo_oow_t i, j;
		moo_oop_t tmp;

		/* [NOTE]
		 *  if some elements at the back of a class definition are lacking default values,
		 *  initv_count is less than total_count. 
		 *  in the following case(no inheritance for simplicity):
		 *    class ... { var a, b := 10, c. } 
		 *  initv_count is 1 whereas total_count is 3. */
		MOO_ASSERT (moo, initv_count <= cc->var[var_type].total_count);

		tmp = moo_instantiate (moo, moo->_array, MOO_NULL, cc->var[var_type].total_count);
		if (!tmp) return -1;

		if (super_initv_count > 0)
		{
			/* handle default values defined in the superclass chain.
			 * i merge them into a single array for convenience and
			 * efficiency of object instantiation by moo_instantiate(). 
			 * it can avoid looking up superclasses upon instantiaion */
			moo_oop_oop_t initv;

			j = 0;
			MOO_ASSERT (moo, cc->super_oop != moo->_nil);
			initv = (moo_oop_oop_t)((moo_oop_class_t)cc->super_oop)->initv[var_type];
			MOO_ASSERT (moo, MOO_CLASSOF(moo, initv) == moo->_array);
			for (i = 0; i < super_initv_count; i++)
			{
				if (initv->slot[i]) ((moo_oop_oop_t)tmp)->slot[j] = initv->slot[i];
				j++;
			}
		}
		else
		{
			/* superclass chain have variables but no default values are defined */
			j = cc->var[var_type].total_count - cc->var[var_type].count;
		}

		for (i = 0; i < cc->var[var_type].initv_count; i++)
		{
			if (cc->var[var_type].initv[i].v)
				((moo_oop_oop_t)tmp)->slot[j] = cc->var[var_type].initv[i].v;
			j++;
		}

		cc->self_oop->initv[var_type] = tmp;
	}

	return 0;
}

static int make_defined_class (moo_t* moo)
{
	/* this function makes a class object with no functions/methods */

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oop_t tmp;
	moo_ooi_t spec, self_spec;
	int just_made = 0, flags;

	flags = 0;
	if (cc->flags & CLASS_INDEXED) flags |= MOO_CLASS_SPEC_FLAG_INDEXED;
	if (cc->flags & CLASS_IMMUTABLE) flags |= MOO_CLASS_SPEC_FLAG_IMMUTABLE;

	if (cc->non_pointer_instsize > 0)
	{
		/* class(#byte(N)), class(#word(N)), etc */
		MOO_ASSERT (moo, cc->var[VAR_INSTANCE].total_count == 0);
		MOO_ASSERT (moo, cc->flags & CLASS_INDEXED);
		MOO_ASSERT (moo, cc->indexed_type != MOO_OBJ_TYPE_OOP);
		spec = MOO_CLASS_SPEC_MAKE(cc->non_pointer_instsize, flags, cc->indexed_type);
	}
	else
	{
		MOO_ASSERT (moo, cc->non_pointer_instsize == 0);
		spec = MOO_CLASS_SPEC_MAKE(cc->var[VAR_INSTANCE].total_count, flags, cc->indexed_type);
	}

	flags = 0;
	if (cc->flags & CLASS_FINAL) flags |= MOO_CLASS_SELFSPEC_FLAG_FINAL;
	if (cc->flags & CLASS_LIMITED) flags |= MOO_CLASS_SELFSPEC_FLAG_LIMITED;
	self_spec = MOO_CLASS_SELFSPEC_MAKE(cc->var[VAR_CLASS].total_count,
	                                    cc->var[VAR_CLASSINST].total_count, flags);

	if (cc->self_oop)
	{
		/* this is an internally created class object being defined. */

		MOO_ASSERT (moo, MOO_CLASSOF(moo, cc->self_oop) == moo->_class);
		MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_KERNEL(cc->self_oop) == MOO_OBJ_FLAGS_KERNEL_IMMATURE);

		if (spec != MOO_OOP_TO_SMOOI(cc->self_oop->spec) ||
		    self_spec != MOO_OOP_TO_SMOOI(cc->self_oop->selfspec))
		{
			/* it conflicts with internal definition */
			moo_setsynerr (moo, MOO_SYNERR_CLASSCONTRA, &cc->fqn_loc, &cc->name);
			return -1;
		}
	}
	else
	{
		/* the class variables and class instance variables are placed
		 * inside the class object after the fixed part. */
		tmp = moo_instantiate(moo, moo->_class, MOO_NULL,
		                      cc->var[VAR_CLASSINST].total_count + cc->var[VAR_CLASS].total_count);
		if (!tmp) return -1;

		just_made = 1;
		cc->self_oop = (moo_oop_class_t)tmp;

		MOO_ASSERT (moo, MOO_CLASSOF(moo, cc->self_oop) == moo->_class);

		cc->self_oop->spec = MOO_SMOOI_TO_OOP(spec);
		cc->self_oop->selfspec = MOO_SMOOI_TO_OOP(self_spec);
	}

/* TODO: check if the current class definition conflicts with the superclass.
 * if superclass is byte variable, the current class cannot be word variable or something else.
*  TODO: TODO: TODO:
 */
	MOO_OBJ_SET_FLAGS_KERNEL (cc->self_oop,  MOO_OBJ_FLAGS_KERNEL_MATURE);

	cc->self_oop->superclass = cc->super_oop;

	if (just_made)
	{
		/* set the name of a class if it's not set. at this point,
		 * only kernel classes must have a name which has been set
		 * during ignition phase. See ignite_3() */
		tmp = moo_makesymbol(moo, cc->name.ptr, cc->name.len);
		if (!tmp) return -1;
		cc->self_oop->name = (moo_oop_char_t)tmp;
	}

	MOO_ASSERT (moo, (moo_oop_t)cc->self_oop->name != moo->_nil);

	if (cc->modname.len > 0)
	{
		tmp = moo_makesymbol(moo, cc->modname.ptr, cc->modname.len);
		if (!tmp) return -1;
		cc->self_oop->modname = tmp;
	}

	tmp = moo_makestring(moo, cc->var[VAR_INSTANCE].str.ptr, cc->var[VAR_INSTANCE].str.len);
	if (!tmp) return -1;
	cc->self_oop->instvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring(moo, cc->var[VAR_CLASS].str.ptr, cc->var[VAR_CLASS].str.len);
	if (!tmp) return -1;
	cc->self_oop->classvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring(moo, cc->var[VAR_CLASSINST].str.ptr, cc->var[VAR_CLASSINST].str.len);
	if (!tmp) return -1;
	cc->self_oop->classinstvars = (moo_oop_char_t)tmp;

	tmp = moo_makestring(moo, cc->pdimp.dcl.ptr, cc->pdimp.dcl.len);
	if (!tmp) return -1;
	cc->self_oop->pooldics = (moo_oop_char_t)tmp;

/* TOOD: good dictionary size */
	tmp = (moo_oop_t)moo_makedic(moo, moo->_method_dictionary, INSTANCE_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	cc->self_oop->mthdic[MOO_METHOD_INSTANCE] = (moo_oop_dic_t)tmp;

/* TOOD: good dictionary size */
	tmp = (moo_oop_t)moo_makedic(moo, moo->_method_dictionary, CLASS_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	cc->self_oop->mthdic[MOO_METHOD_CLASS] = (moo_oop_dic_t)tmp;

	/* store the default intial values for instance variables */
	if (make_default_initial_values(moo, VAR_INSTANCE) <= -1) return -1;

	/* store the default intial values for class instance variables */
	if (make_default_initial_values(moo, VAR_CLASSINST) <= -1) return -1;
	if (cc->self_oop->initv[VAR_CLASSINST] != moo->_nil)
	{
		moo_oow_t i, initv_count;
		moo_oop_oop_t initv;

		/* apply the default initial values for class instance variables to this class now */
		initv = (moo_oop_oop_t)cc->self_oop->initv[VAR_CLASSINST];
		MOO_ASSERT (moo, MOO_CLASSOF(moo, initv) == moo->_array);
		initv_count = MOO_OBJ_GET_SIZE(initv);

		for (i = 0; i < initv_count; i++)
		{
			cc->self_oop->slot[i] = initv->slot[i];
		}
	}

	/* initialize class variables with default initial values */
	if (cc->var[VAR_CLASS].initv_count > 0)
	{
		moo_oow_t i, j, initv_count;

		initv_count = cc->var[VAR_CLASS].initv_count;

		/* name instance variables and class instance variables are placed
		 * in the front part. set j such that they can be skipped. */
		j = cc->var[VAR_CLASSINST].total_count;

		MOO_ASSERT (moo, MOO_CLASS_NAMED_INSTVARS + j + initv_count <= MOO_OBJ_GET_SIZE(cc->self_oop));
		for (i = 0; i < initv_count; i++) 
		{
			cc->self_oop->slot[j] = cc->var[VAR_CLASS].initv[i].v;
			j++;
		}
	}

	/* [NOTE] don't create a dictionary on the nsdic. keep it to be nil.
	 *        add_nsdic_to_class() instantiates a dictionary if necessary.  */

	/* [NOTE] don't set the trsize field yet here. */

/* TODO: initialize more fields??? what else. */

/* TODO: update the subclasses field of the superclass if it's not nil */


	if (make_getters_and_setters(moo) <= -1) return -1;

	if (just_made)
	{
		/* register the class to the system dictionary. kernel classes have 
		 * been registered at the ignition phase. */
		if (!moo_putatdic(moo, (moo_oop_dic_t)cc->ns_oop, (moo_oop_t)cc->self_oop->name, (moo_oop_t)cc->self_oop)) return -1;
		cc->self_oop->nsup = cc->ns_oop;
	}

	return 0;
}

static MOO_INLINE int _set_class_indexed_type (moo_t* moo, moo_obj_type_t type)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	if (cc->flags & CLASS_INDEXED)
	{
		moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	cc->flags |= CLASS_INDEXED;
	cc->indexed_type = type;
	return 0;
}

static int process_class_modifiers (moo_t* moo, moo_ioloc_t* type_loc)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	GET_TOKEN (moo);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		do
		{
			int permit_non_pointer_instsize = 0;
			
			if (is_token_symbol(moo, VOCA_BYTE_S))
			{
				/* class(#byte) */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_BYTE) <= -1) return -1;
				GET_TOKEN (moo);
				permit_non_pointer_instsize = 1;
			}
			else if (is_token_symbol(moo, VOCA_CHARACTER_S))
			{
				/* class(#character) */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_CHAR) <= -1) return -1;
				GET_TOKEN (moo);
				permit_non_pointer_instsize = 1;
			}
			else if (is_token_symbol(moo, VOCA_HALFWORD_S))
			{
				/* class(#halfword) */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_HALFWORD) <= -1) return -1;
				GET_TOKEN (moo);
				permit_non_pointer_instsize = 1;
			}
			else if (is_token_symbol(moo, VOCA_WORD_S))
			{
				/* class(#word) */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_WORD) <= -1) return -1;
				GET_TOKEN (moo);
				permit_non_pointer_instsize = 1;
			}
			else if (is_token_symbol(moo, VOCA_LIWORD_S))
			{
				/* class(#liword) -
				 *   the liword type maps to one of word or halfword.
				 *   see the definiton of MOO_OBJ_TYPE_LIWORD in moo.h */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_LIWORD) <= -1) return -1;
				GET_TOKEN (moo);
				permit_non_pointer_instsize = 1;
			}
			else if (is_token_symbol(moo, VOCA_POINTER_S))
			{
				/* class(#pointer) */
				if (_set_class_indexed_type(moo, MOO_OBJ_TYPE_OOP) <= -1) return -1;
				GET_TOKEN (moo);
			}
			else if (is_token_symbol(moo, VOCA_FINAL_S))
			{
				if (cc->flags & CLASS_FINAL)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				cc->flags |= CLASS_FINAL;
				GET_TOKEN(moo);
			}
			else if (is_token_symbol(moo, VOCA_LIMITED_S))
			{
				if (cc->flags & CLASS_LIMITED)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				cc->flags |= CLASS_LIMITED;
				GET_TOKEN(moo);
			}
			else if (is_token_symbol(moo, VOCA_IMMUTABLE_S))
			{
				if (cc->flags & CLASS_IMMUTABLE)
				{
					moo_setsynerr (moo, MOO_SYNERR_MODIFIERDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo));
					return -1;
				}
				cc->flags |= CLASS_IMMUTABLE;
				GET_TOKEN(moo);
			}
			else if (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA || TOKEN_TYPE(moo) == MOO_IOTOK_EOF || TOKEN_TYPE(moo) == MOO_IOTOK_RPAREN)
			{
				/* no modifier is present */
				moo_setsynerr (moo, MOO_SYNERR_MODIFIER, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
			else
			{
				/* invalid modifier */
				moo_setsynerr (moo, MOO_SYNERR_MODIFIERINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			if (permit_non_pointer_instsize)
			{
				/* class(#byte(20))
				 * class(#word(3))
				 * ... */
				*type_loc = moo->c->tok.loc;

				if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
				{
					moo_ooi_t tmp;

					GET_TOKEN (moo);

					if (TOKEN_TYPE(moo) != MOO_IOTOK_NUMLIT && TOKEN_TYPE(moo) != MOO_IOTOK_RADNUMLIT)
					{
						moo_setsynerr (moo, MOO_SYNERR_LITERAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}

					if (string_to_smooi(moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT, &tmp) <= -1 || 
						tmp < 0 || tmp > MOO_MAX_NAMED_INSTVARS)
					{
						/* the class type size has nothing to do with the name instance variables
						 * in the semantics. but it is stored into the named-instvar bits in the
						 * spec field of a class. so i check it against MOO_MAX_NAMED_INSTVARS. */
						moo_setsynerr (moo, MOO_SYNERR_NPINSTSIZEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo)); 
						return -1;
					}

					GET_TOKEN (moo);

					if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
					{
						moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
						return -1;
					}
					GET_TOKEN (moo);

					cc->non_pointer_instsize = tmp;
				}
			}

			if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break; /* hopefully ) */
			GET_TOKEN (moo); /* get the token after , */
		}
		while (1);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo); /* consume the closing ) */

	return 0;
}

static int process_class_superclass (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	int super_is_nil = 0;
	int superfqn_is_dotted;
	
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LPAREN)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo), "superclass must be specified");
		return -1;
	}

	/* superclass is specified. new class defintion.
	 * for example, #class Dag(Object) */
	GET_TOKEN (moo); /* skip ( and read superclass name */

	/* TODO: multiple inheritance */

	if (TOKEN_TYPE(moo) == MOO_IOTOK_NIL)
	{
		/* #class Dag(nil) */
		super_is_nil = 1;
	}
	else if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		/* superclass name expected */
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "superclass name expected");
		return -1;
	}

	if (set_superclass_fqn(moo, cc, TOKEN_NAME(moo)) <= -1) return -1;
	cc->superfqn_loc = moo->c->tok.loc;

	superfqn_is_dotted = (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED);

	GET_TOKEN (moo);
	if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
	{
		moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo); /* skip ) and read the next token */

	if (super_is_nil)
	{
		cc->super_oop = moo->_nil;
	}
	else
	{
		var_info_t var;

		if (get_variable_info(moo, &cc->superfqn, &cc->superfqn_loc, superfqn_is_dotted, &var) <= -1) return -1;

		if (var.type != VAR_GLOBAL) goto unknown_superclass;
		if (MOO_CLASSOF(moo, var.u.gbl->value) == moo->_class && 
		    MOO_OBJ_GET_FLAGS_KERNEL(var.u.gbl->value) != MOO_OBJ_FLAGS_KERNEL_IMMATURE)
		{
			/* the value found must be a class and it must not be an incomplete internal class object. 
			 *  0(non-kernel object)
			 *  1(incomplete kernel object),
			 *  2(complete kernel object) */

			cc->super_oop = var.u.gbl->value;

			/* the superclass became known. */
			if (((moo_oop_class_t)cc->super_oop)->trsize != moo->_nil &&
				(cc->flags & CLASS_INDEXED) &&
				cc->indexed_type != MOO_OBJ_TYPE_OOP)
			{
				/* non-pointer object cannot inherit from a superclass with trailer size set */
				moo_setsynerrbfmt (moo, MOO_SYNERR_INHERITBANNED, &cc->fqn_loc, &cc->fqn,
					"the non-pointer class %.*js cannot inherit from a class set with trailer size",
					cc->fqn.len, cc->fqn.ptr);
				return -1;
			}

			if (MOO_CLASS_SELFSPEC_FLAGS(MOO_OOP_TO_SMOOI(((moo_oop_class_t)cc->super_oop)->selfspec)) & MOO_CLASS_SELFSPEC_FLAG_FINAL)
			{
				/* cannot inherit a #final class */
				moo_setsynerrbfmt (moo, MOO_SYNERR_INHERITBANNED, &cc->fqn_loc, &cc->fqn,
					"the %.*js class cannot inherit from a final class", cc->fqn.len, cc->fqn.ptr);
				return -1;
			}
		}
		else
		{
		unknown_superclass:
			/* there is no object with such a name. or,
			 * the object found with the name is not a class object. or,
			 * the class object found is a internally defined kernel
			 * class object. */
			moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEUNDEF, &cc->superfqn_loc, &cc->superfqn, "superclass name undefined");
			return -1;
		}
	}

	return 0;
}

static int process_class_module_import (moo_t* moo)
{
	/* handle the module importing(from) part.
	 *     class XXX from 'mod.name' */

	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	GET_TOKEN (moo); /* skip 'from' */
	if (TOKEN_TYPE(moo) != MOO_IOTOK_STRLIT)
	{
		moo_setsynerr (moo, MOO_SYNERR_STRING, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	if (TOKEN_NAME_LEN(moo) <= 0 || TOKEN_NAME_LEN(moo) > MOO_MOD_NAME_LEN_MAX ||
	    moo_find_oochar(TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo), '-') )
	{
		/* check for a bad module name. 
		 * also disallow a dash in the name - i like converting
		 * a period to a dash when mapping the module name to an
		 * actual module file. disallowing a dash lowers confusion
		 * when loading a module. */
		moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo), "invalid module name");
		return -1;
	}

	if (set_class_modname (moo, cc, TOKEN_NAME(moo)) <= -1) return -1;
	cc->modname_loc = *TOKEN_LOC(moo);

	GET_TOKEN (moo); /* skip the module name and read the next token */
	return 0;
}

static int process_class_interfaces (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;

	GET_TOKEN (moo); /* skip [ */

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "interface name expected");
		return -1;
	}

	do
	{
		var_info_t var;
		moo_oow_t old_ifce_count;
		moo_oow_t ifce_index;

		if (get_variable_info(moo, TOKEN_NAME(moo), TOKEN_LOC(moo), TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED, &var) <= -1 ||
		    var.type != VAR_GLOBAL || MOO_CLASSOF(moo, var.u.gbl->value) != moo->_interface)
		{
			moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEUNDEF, TOKEN_LOC(moo), TOKEN_NAME(moo), "interface name undefined");
			return -1;
		}

		old_ifce_count = cc->ifces.count;
		if (add_oop_to_oopbuf_nodup(moo, &cc->ifces, var.u.gbl->value, &ifce_index) <= -1) return -1;
		if (ifce_index < old_ifce_count)
		{
			/* add_oop_to_oopbuf_nodup() returns the index to an existing item
			 * if it's found. the index should be between 0 and the previous count - 1 inclusive.
			 * the index returned will be the previous count if it's added this time */
			moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo), "duplicate interface name");
			return -1;
		}

#if 0
		if (find_word_in_string(&cc->ifce_names, TOKEN_NAME(moo), MOO_NULL) >= 0)
		{
			moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo), "duplicate interface name");
			return -1;
		}

		/* just store the interface name instead of resolved interface object */
		if (copy_string_to(moo, TOKEN_NAME(moo), &cc->ifce_names, &cc->ifce_names_capa, 1, ' ') <= -1) return -1;
#endif

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) != MOO_IOTOK_COMMA) break;

		GET_TOKEN (moo);
	}
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT || TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACK)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACK, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo); /* skip ] and read the next token */
	return 0;
}

struct ciim_t
{
	moo_oop_interface_t ifce;
	moo_oop_class_t _class;
	int mth_type;
};
typedef struct ciim_t ciim_t;

static int ciim_on_each_method (moo_t* moo, moo_oop_dic_t dic, moo_oop_association_t ass, void* ctx)
{
	ciim_t* ciim = (ciim_t*)ctx;
	moo_oocs_t name;
	moo_oop_method_t mth;
	moo_oop_methsig_t sig;

	name.ptr = MOO_OBJ_GET_CHAR_SLOT(ass->key);
	name.len = MOO_OBJ_GET_SIZE(ass->key);

	/* [IMPORT] the method lookup logic should be the same as moo_findmethod() in exec.c */
	mth = moo_findmethodinclasschain(moo, ciim->_class, ciim->mth_type, &name);
	if (!mth)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_CLASSNCIFCE, MOO_NULL, MOO_NULL, 
			"%.*js not implementing %.*js>>%.*js",
			MOO_OBJ_GET_SIZE(ciim->_class->name), MOO_OBJ_GET_CHAR_SLOT(ciim->_class->name),
			MOO_OBJ_GET_SIZE(ciim->ifce->name), MOO_OBJ_GET_CHAR_SLOT(ciim->ifce->name),
			name.len, name.ptr
		);
		return -1;
	}

	sig = (moo_oop_methsig_t)ass->value;
	if (MOO_METHOD_GET_PREAMBLE_FLAGS(MOO_OOP_TO_SMOOI(mth->preamble)) != MOO_METHOD_GET_PREAMBLE_FLAGS(MOO_OOP_TO_SMOOI(sig->preamble)))
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_CLASSNCIFCE, MOO_NULL, MOO_NULL, 
			"%.*js>>%.*js modifiers conficting with %.*js>>%.*js",
			MOO_OBJ_GET_SIZE(ciim->_class->name), MOO_OBJ_GET_CHAR_SLOT(ciim->_class->name),
			name.len, name.ptr,
			MOO_OBJ_GET_SIZE(ciim->ifce->name), MOO_OBJ_GET_CHAR_SLOT(ciim->ifce->name),
			name.len, name.ptr
		);
		return -1;
	}
	
	if (mth->tmpr_nargs != sig->tmpr_nargs) /* don't need MOO_OOP_TO_SMOOI */
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_CLASSNCIFCE, MOO_NULL, MOO_NULL, 
			"%.*js>>%.*js parameters conflicting with %.*js>>%.*js",
			MOO_OBJ_GET_SIZE(ciim->_class->name), MOO_OBJ_GET_CHAR_SLOT(ciim->_class->name),
			name.len, name.ptr,
			MOO_OBJ_GET_SIZE(ciim->ifce->name), MOO_OBJ_GET_CHAR_SLOT(ciim->ifce->name),
			name.len, name.ptr
		);
		return -1;
	}
	
	return 0;
}

static int class_implements_interface (moo_t* moo, moo_oop_class_t _class, moo_oop_interface_t ifce)
{
	ciim_t ciim;

	ciim.ifce = ifce;
	ciim._class = _class;
	ciim.mth_type = MOO_METHOD_INSTANCE;
	if (moo_walkdic(moo, ifce->mthdic[MOO_METHOD_INSTANCE], ciim_on_each_method, &ciim) <= -1) return 0;

	ciim.ifce = ifce;
	ciim._class = _class;
	ciim.mth_type = MOO_METHOD_CLASS;
	if (moo_walkdic(moo, ifce->mthdic[MOO_METHOD_CLASS], ciim_on_each_method, &ciim) <= -1) return 0;

	return 1;
}

static int check_class_interface_conformance (moo_t* moo)
{
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oow_t i;

	for (i = 0; i < cc->ifces.count; i++)
	{
		if (!class_implements_interface(moo, cc->self_oop, (moo_oop_interface_t)cc->ifces.ptr[i])) return -1;
	}

	return 0;
}

static int __compile_class_definition (moo_t* moo, int class_type)
{
	/* 
	 * class-definition := class class-modifier? class-name super-class-spec interface-spec?  (class-body | class-module-import)
	 *
	 * class-modifier := "(" (#byte | #character | #word | #pointer)? ")"
	 * super-class-spec := "(" super-class-name ")"
	 * interface-spec := "[" interface-name-list "]"
	 * interface-name-list := interface-name | interface-name "," inteface-name-list
	 * class-body := "{" variable-definition* method-definition* "}"
	 * class-module-import := from "module-name-string"
	 * 
	 * TOOD: add | |, |+ |, |* |
	 * variable-definition := (#var | #variable) variable-modifier? variable-list "."
	 * variable-modifier := "(" (#class | #classinst)? ")"
	 * variable-list := identifier*
	 *
	 * method-definition := method method-modifier? method-actual-definition
	 * method-modifier := "(" (#class | #instance)? ")"
	 * method-actual-definition := method-name "{" method-tempraries? method-pragma? method-statements* "}"
	 *
	 * NOTE: when extending a class, class-module-import and variable-definition are not allowed.
	 */
	moo_cunit_class_t* cc = (moo_cunit_class_t*)moo->c->cunit;
	moo_oop_association_t ass;
	moo_ioloc_t type_loc;

	if (class_type == CLASS_TYPE_NORMAL && TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process class modifiers */
		MOO_ASSERT (moo, (cc->flags & CLASS_INDEXED) == 0);
		if (process_class_modifiers(moo, &type_loc) <= -1) return -1;
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "class name expected");
		return -1;
	}

	if (cc->cunit_parent && cc->cunit_parent->cunit_type == MOO_CUNIT_CLASS && TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		/* the name of a nested class cannot be multi-segmented */
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "undotted class name expected");
		return -1;
	}

#if 0
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && is_restricted_word(TOKEN_NAME(moo)))
	{
		/* wrong class name */
		moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo), "invalid class name");
		return -1;
	}
#endif

	/* [NOTE] TOKEN_NAME(moo) doesn't contain the full name if it's nested 
	 *        inside a class. it is merely a name that appeared in the source
	 *        code.
	 *  TODO: compose the full name by traversing the namespace chain. */
	if (set_class_fqn(moo, cc, TOKEN_NAME(moo)) <= -1) return -1;
	cc->fqn_loc = moo->c->tok.loc;

	if (cc->cunit_parent && cc->cunit_parent->cunit_type == MOO_CUNIT_CLASS)
	{
		moo_cunit_class_t* c = (moo_cunit_class_t*)cc->cunit_parent;
		if ((moo_oop_t)c->self_oop->nsdic == moo->_nil)
		{
			/* attach a new namespace dictionary to the nsdic field of the class */
			if (!attach_nsdic_to_class(moo, c->self_oop)) return -1;
		}
		cc->ns_oop = c->self_oop->nsdic; /* TODO: if c->nsdic is nil, create one? */
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		if (preprocess_dotted_name(moo, (class_type == CLASS_TYPE_EXTEND? PDN_DONT_ADD_NS: 0), MOO_NULL, &cc->fqn, &cc->fqn_loc, &cc->name, &cc->ns_oop) <= -1) return -1;
	}
	else
	{
		cc->ns_oop = moo->sysdic;
	}
	GET_TOKEN (moo); 

	if (class_type == CLASS_TYPE_EXTEND)
	{
		/* extending class */
		MOO_ASSERT (moo, cc->flags == 0);

		MOO_INFO2 (moo, "Extending a class %.*js\n", cc->fqn.len, cc->fqn.ptr);

		ass = moo_lookupdic(moo, (moo_oop_dic_t)cc->ns_oop, &cc->name);
		if (ass && MOO_CLASSOF(moo, ass->value) == moo->_class &&
		    MOO_OBJ_GET_FLAGS_KERNEL(ass->value) != MOO_OBJ_FLAGS_KERNEL_IMMATURE)
		{
			/* the value must be a class object.
			 * and it must be either a user-defined(0) or completed kernel built-in(2). 
			 * an incomplete kernel built-in class object(1) can not be extended */
			cc->self_oop = (moo_oop_class_t)ass->value;
		}
		else
		{
			/* only an existing class can be extended. */
			moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEUNDEF, &cc->fqn_loc, &cc->name, "class name undefined");
			return -1;
		}

		cc->super_oop = cc->self_oop->superclass;
		MOO_ASSERT (moo, cc->super_oop == moo->_nil || MOO_CLASSOF(moo, cc->super_oop) == moo->_class);
		
		if (TOKEN_TYPE(moo) == MOO_IOTOK_LBRACK)
		{
			if (process_class_interfaces(moo) <= -1) return -1;
		}
	}
	else
	{
		MOO_INFO2 (moo, "Defining a class %.*js\n", cc->fqn.len, cc->fqn.ptr);

		ass = moo_lookupdic(moo, (moo_oop_dic_t)cc->ns_oop, &cc->name);
		if (ass)
		{
			if (MOO_CLASSOF(moo, ass->value) != moo->_class || 
			    MOO_OBJ_GET_FLAGS_KERNEL(ass->value) == MOO_OBJ_FLAGS_KERNEL_MATURE)
			{
				/* the object found with the name is not a class object 
				 * or the the class object found is a fully defined kernel 
				 * class object */
				moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEDUPL, &cc->fqn_loc, &cc->name, "duplicate class name");
				return -1;
			}

			cc->self_oop = (moo_oop_class_t)ass->value;
		}
		else
		{
			/* no class of such a name is found. it's a new definition,
			 * which is normal for most new classes. */
			MOO_ASSERT (moo, cc->self_oop == MOO_NULL);
		}

		if (process_class_superclass(moo) <= -1) return -1;

		if (TOKEN_TYPE(moo) == MOO_IOTOK_LBRACK)
		{
			if (process_class_interfaces(moo) <= -1) return -1;
		}

		if (is_token_word (moo, VOCA_FROM))
		{
			if (process_class_module_import(moo) <= -1) return -1;
		}
	}

	cc->in_class_body = 1;
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	MOO_ASSERT (moo, cc->super_oop != MOO_NULL);
	if (cc->super_oop != moo->_nil)
	{
		/* adjust the instance variable count and the class instance variable
		 * count to include that of a superclass */
		moo_oop_class_t c;
		moo_oow_t spec, self_spec;

		c = (moo_oop_class_t)cc->super_oop;
		spec = MOO_OOP_TO_SMOOI(c->spec);
		self_spec = MOO_OOP_TO_SMOOI(c->selfspec);

		/* [NOTE] class variables are not inherited. 
		 *        so no data about them are not transferred over */

		if ((cc->flags & CLASS_INDEXED) && cc->indexed_type != MOO_OBJ_TYPE_OOP)
		{
			/* the class defined is a non-pointer object. */
			if (MOO_CLASS_SPEC_INDEXED_TYPE(spec) == MOO_OBJ_TYPE_OOP && MOO_CLASS_SPEC_NAMED_INSTVARS(spec) > 0)
			{
				/* a non-pointer object cannot inherit from a pointer object with instance variables */
				moo_setsynerrbfmt (moo, MOO_SYNERR_INHERITBANNED, &cc->fqn_loc, &cc->fqn, 
					"the non-pointer class %.*js cannot inherit from a pointer class defined with instance variables", 
					cc->fqn.len, cc->fqn.ptr);
				return -1;
			}

			/* NOTE: I don't mandate that the parent and the child be of the same type.
			 *       Say, for a parent class(#byte(4)), a child can be defined to be
			 *       class(#word(4)). */

			if (cc->non_pointer_instsize < MOO_CLASS_SPEC_NAMED_INSTVARS(spec))
			{
				moo_setsynerrbfmt (moo, MOO_SYNERR_NPINSTSIZEINVAL, &type_loc, MOO_NULL,
					"the instance size(%zu) for the non-pointer class %.*js must not be less than the size(%zu) defined in the superclass ", 
					cc->non_pointer_instsize, cc->fqn.len, cc->fqn.ptr, (moo_oow_t)MOO_CLASS_SPEC_NAMED_INSTVARS(spec));
				return -1;
			}
		}
		else
		{
			/* the class defined is a pointer object or a variable-pointer object */
			MOO_ASSERT (moo, cc->non_pointer_instsize == 0); /* no such thing as class(#pointer(N)). so it must be 0 */
			cc->var[VAR_INSTANCE].total_count = MOO_CLASS_SPEC_NAMED_INSTVARS(spec);
		}
		cc->var[VAR_CLASSINST].total_count = MOO_CLASS_SELFSPEC_CLASSINSTVARS(self_spec);
	}

	GET_TOKEN (moo);

	if (class_type == CLASS_TYPE_EXTEND)
	{
		moo_oop_char_t pds;

		/* when a class is extended, a new variable cannot be added */
		if (is_token_word(moo, VOCA_VAR) || is_token_word(moo, VOCA_VARIABLE) || 
		    is_token_binary_selector(moo, VOCA_VBAR) ||
		    is_token_binary_selector(moo, VOCA_VBAR_PLUS) ||
		    is_token_binary_selector(moo, VOCA_VBAR_ASTER))
		{
			moo_setsynerr (moo, MOO_SYNERR_VARDCLBANNED, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		/* load the pooldic definition from the existing class object */
		pds = cc->self_oop->pooldics;
		if ((moo_oop_t)pds != moo->_nil)
		{
			moo_ooch_t* ptr, * end;

			MOO_ASSERT (moo, MOO_CLASSOF(moo, pds) == moo->_string);

			ptr = pds->slot;
			end = pds->slot + MOO_OBJ_GET_SIZE(pds);

			/* this loop handles the pooldic string as if it's a pooldic import.
			 * see compile_class_level_variables() for mostly identical code except token handling */
			do
			{
				moo_oocs_t last, tok;
				moo_ioloc_t loc;
				int dotted = 0;
				moo_oop_nsdic_t ns_oop;

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
					if (preprocess_dotted_name(moo, 0, MOO_NULL, &tok, &loc, &last, &ns_oop) <= -1) return -1;
				}
				else
				{
					last = tok;
					/* it falls back to the name space of the class */
					ns_oop = cc->ns_oop; 
				}

				if (import_pool_dictionary(moo, ns_oop, &last, &tok, &loc) <= -1) return -1;
			}
			while (1);
		}
	}
	else
	{
		/* a new class including an internally defined class object */
		do
		{
			if (is_token_binary_selector(moo, VOCA_VBAR))
			{
				/* instance variables | a b c | */
				GET_TOKEN (moo);
				if (compile_class_level_variables_vbar(moo, VAR_INSTANCE) <= -1) return -1;
			}
			else if (is_token_binary_selector(moo, VOCA_VBAR_ASTER))
			{
				/* class instance variables |* a b c | */
				GET_TOKEN (moo);
				if (compile_class_level_variables_vbar(moo, VAR_CLASSINST) <= -1) return -1;
			}
			else if (is_token_binary_selector(moo, VOCA_VBAR_PLUS))
			{
				/* class variables |+ a b c | */
				GET_TOKEN (moo);
				if (compile_class_level_variables_vbar(moo, VAR_CLASS) <= -1) return -1;
			}
			else if (is_token_word(moo, VOCA_VAR) || is_token_word(moo, VOCA_VARIABLE))
			{
				/* variable declaration */
				GET_TOKEN (moo);
				if (compile_class_level_variables(moo) <= -1) return -1;
			}
			else if (is_token_word(moo, VOCA_IMPORT))
			{
				/* import declaration */
				GET_TOKEN (moo);
				if (compile_class_level_imports(moo) <= -1) return -1;
			}
			else break;
		}
		while (1);

		if (make_defined_class(moo) <= -1) return -1;

		if (cc->modname.len > 0)
		{
			MOO_ASSERT (moo, MOO_CLASSOF(moo, cc->self_oop->modname) == moo->_symbol);
			/* [NOTE]
			 *  even if the modname field is set in the class object itself
			 *  by make_define_class(), i pass the module name in the compiler
			 *  memory(not part of the object memory) to moo_importmod().
			 *  no big overhead as it's already available. but Accessing 
			 *  this extra module name, i'm free from GC headache */
			if (moo_importmod(moo, cc->self_oop, cc->modname.ptr, cc->modname.len) <= -1)
			{
				const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
				moo_setsynerrbfmt (moo, MOO_SYNERR_MODIMPFAIL, &cc->modname_loc, &cc->modname,
					"unable to import %.*js - %js", cc->modname.len, cc->modname.ptr, oldmsg);
				return -1;
			}
		}

		if (cc->self_oop->trsize == moo->_nil &&
		    cc->self_oop->superclass != moo->_nil)
		{
			/* the trailer size has not been set by the module importer.
			 * if the superclass has the trailer size set, this class must
			 * inherit so that the inherited methods work well when they
			 * access the trailer space */
			cc->self_oop->trsize = ((moo_oop_class_t)cc->self_oop->superclass)->trsize;
			cc->self_oop->trgc = ((moo_oop_class_t)cc->self_oop->superclass)->trgc;
		}
	}

	do
	{
		if (is_token_word(moo, VOCA_METHOD))
		{
			/* method definition in class definition */
			GET_TOKEN (moo);
			if (compile_method_definition(moo) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_POOLDIC))
		{
			/* pooldic definition nested inside class definition */
			GET_TOKEN (moo);
			if (compile_pooldic_definition(moo) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_CLASS))
		{
			/* class definition nested inside another class definition */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, CLASS_TYPE_NORMAL) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_EXTEND))
		{
			/* class extension nested inside another class definition */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, CLASS_TYPE_EXTEND) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_INTERFACE))
		{
			/* interface nested inside another class definition */
			GET_TOKEN (moo);
			if (compile_interface_definition(moo) <= -1) return -1;
		}
		else break;
	}
	while (1);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	if (cc->ifces.count > 0)
	{
		/* interface has been specified. */
		if (check_class_interface_conformance(moo) <= -1) return -1;
	}

	GET_TOKEN (moo);
	return 0;
}

static int compile_class_definition (moo_t* moo, int class_type)
{
	int n;

	if (!push_cunit(moo, MOO_CUNIT_CLASS)) return -1;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	n = __compile_class_definition (moo, class_type);

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	pop_cunit (moo);
	return n;
}

static int add_method_signature (moo_t* moo)
{
	moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;
	moo_oop_char_t name; /* selector */
	moo_oop_methsig_t mth; /* method signature */
	moo_ooi_t preamble_flags = 0;
	moo_oow_t tmp_count = 0;

	name = (moo_oop_char_t)moo_makesymbol(moo, ifce->mth.name.ptr, ifce->mth.name.len);
	if (!name) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&name); tmp_count++;

	mth = (moo_oop_methsig_t)moo_instantiate(moo, moo->_methsig, MOO_NULL, 0);
	if (!mth) goto oops;
	moo_pushtmp (moo, (moo_oop_t*)&mth); tmp_count++;

	mth->owner = ifce->self_oop;
	mth->name = name;

	preamble_flags |= ifce->mth.variadic; /* MOO_METHOD_PREAMBLE_FLAG_VARIADIC or MOO_METHOD_PREAMBLE_FLAG_LIBERAL */
	if (ifce->mth.lenient) preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_LENIENT;
	if (ifce->mth.type == MOO_METHOD_DUAL) preamble_flags |= MOO_METHOD_PREAMBLE_FLAG_DUAL;

	mth->preamble = MOO_SMOOI_TO_OOP(MOO_METHOD_MAKE_PREAMBLE(0,0,preamble_flags));
	mth->tmpr_nargs = MOO_SMOOI_TO_OOP(ifce->mth.tmpr_nargs);

	if (ifce->mth.type == MOO_METHOD_DUAL)
	{
		if (!moo_putatdic(moo, ifce->self_oop->mthdic[MOO_METHOD_INSTANCE], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
		if (!moo_putatdic(moo, ifce->self_oop->mthdic[MOO_METHOD_CLASS], (moo_oop_t)name, (moo_oop_t)mth)) 
		{
			/* 'name' is a symbol created of ifce->mth.name. so use it as a key for deletion */
			moo_deletedic (moo, ifce->self_oop->mthdic[MOO_METHOD_INSTANCE], &ifce->mth.name);
			goto oops;
		}
	}
	else
	{
		MOO_ASSERT (moo, ifce->mth.type < MOO_COUNTOF(ifce->self_oop->mthdic));
		if (!moo_putatdic(moo, ifce->self_oop->mthdic[ifce->mth.type], (moo_oop_t)name, (moo_oop_t)mth)) goto oops;
	}

	moo_poptmps (moo, tmp_count); tmp_count = 0;
	return 0;

oops:
	moo_poptmps (moo, tmp_count);
	return -1;
}

static int __compile_method_signature (moo_t* moo)
{
	moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;

	if (TOKEN_TYPE(moo) == MOO_IOTOK_LPAREN)
	{
		/* process method modifiers  */
		if (process_method_modifiers(moo, &ifce->mth) <= -1) return -1;
	}

	if (compile_method_name(moo, &ifce->mth) <= -1) return -1;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		/* . expected */
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	if (add_method_signature(moo) <= -1) return -1;

	GET_TOKEN (moo);
	return 0;
}

static int compile_method_signature (moo_t* moo)
{
	moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;
	int n;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	ifce->mth.active = 1;
	reset_method_data (moo, &ifce->mth);

	n = __compile_method_signature(moo);

	ifce->mth.active = 0;
	return n;
}

static int make_defined_interface (moo_t* moo)
{
	moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;
	moo_oop_t tmp;

	tmp = moo_instantiate(moo, moo->_interface, MOO_NULL, 0);
	if (!tmp) return -1;
	ifce->self_oop = (moo_oop_interface_t)tmp;

	tmp = moo_makesymbol(moo, ifce->name.ptr, ifce->name.len);
	if (!tmp) return -1;
	ifce->self_oop->name = (moo_oop_char_t)tmp;

	tmp = (moo_oop_t)moo_makedic(moo, moo->_method_dictionary, INSTANCE_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	ifce->self_oop->mthdic[MOO_METHOD_INSTANCE] = (moo_oop_dic_t)tmp;

	tmp = (moo_oop_t)moo_makedic(moo, moo->_method_dictionary, CLASS_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	ifce->self_oop->mthdic[MOO_METHOD_CLASS] = (moo_oop_dic_t)tmp;

	if (!moo_putatdic(moo, (moo_oop_dic_t)ifce->ns_oop, (moo_oop_t)ifce->self_oop->name, (moo_oop_t)ifce->self_oop)) return -1;
	ifce->self_oop->nsup = ifce->ns_oop;

	return 0;
}

static int __compile_interface_definition (moo_t* moo)
{
	moo_cunit_interface_t* ifce = (moo_cunit_interface_t*)moo->c->cunit;
	moo_oop_association_t ass;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "interface name expected");
		return -1;
	}

	if (ifce->cunit_parent && ifce->cunit_parent->cunit_type == MOO_CUNIT_CLASS && TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		/* the name of a nested class cannot be multi-segmented */
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "undotted interface name expected");
		return -1;
	}

#if 0
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && is_restricted_word(TOKEN_NAME(moo)))
	{
		/* wrong class name */
		moo_setsynerr (moo, MOO_SYNERR_NAMEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo), "invalid interface name");
		return -1;
	}
#endif

	/* [NOTE] TOKEN_NAME(moo) doesn't contain the full name if it's nested 
	 *        inside a class. it is merely a name that appeared in the source
	 *        code.
	 *  TODO: compose the full name by traversing the namespace chain. */
	if (set_interface_fqn(moo, ifce, TOKEN_NAME(moo)) <= -1) return -1;
	ifce->fqn_loc = moo->c->tok.loc;

	if (ifce->cunit_parent && ifce->cunit_parent->cunit_type == MOO_CUNIT_CLASS)
	{
		/* nested inside a class */
		moo_cunit_class_t* c = (moo_cunit_class_t*)ifce->cunit_parent;
		if ((moo_oop_t)c->self_oop->nsdic == moo->_nil)
		{
			/* attach a new namespace dictionary to the nsdic field of the class */
			if (!attach_nsdic_to_class(moo, c->self_oop)) return -1;
		}
		ifce->ns_oop = c->self_oop->nsdic; /* TODO: if c->nsdic is nil, create one? */
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		if (preprocess_dotted_name(moo, 0, MOO_NULL, &ifce->fqn, &ifce->fqn_loc, &ifce->name, &ifce->ns_oop) <= -1) return -1;
	}
	else
	{
		ifce->ns_oop = moo->sysdic;
	}
	GET_TOKEN (moo); 

	MOO_INFO2 (moo, "Defining an interface %.*js\n", ifce->fqn.len, ifce->fqn.ptr);

	ass = moo_lookupdic(moo, (moo_oop_dic_t)ifce->ns_oop, &ifce->name);
	if (ass)
	{
		/* The interface name already exists. An interface cannot be defined with an existing name */
		moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEDUPL, &ifce->fqn_loc, &ifce->name, "duplicate interface name");
		return -1;
	}


	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);

	/* an interface cannot have variables */
	if (is_token_word(moo, VOCA_VAR) || is_token_word(moo, VOCA_VARIABLE) || 
	    is_token_binary_selector(moo, VOCA_VBAR) ||
	    is_token_binary_selector(moo, VOCA_VBAR_PLUS) ||
	    is_token_binary_selector(moo, VOCA_VBAR_ASTER))
	{
		moo_setsynerr (moo, MOO_SYNERR_VARDCLBANNED, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	if (make_defined_interface(moo) <= -1) return -1;

	do
	{
		if (is_token_word(moo, VOCA_METHOD))
		{
			/* method definition in class definition */
			GET_TOKEN (moo);
			if (compile_method_signature(moo) <= -1) return -1;
		}
		else break;
	}
	while (1);

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}

	GET_TOKEN (moo);
	return 0;

}

static int compile_interface_definition (moo_t* moo)
{
	int n;

	if (!push_cunit(moo, MOO_CUNIT_INTERFACE)) return -1;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	n = __compile_interface_definition (moo);

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	pop_cunit (moo);
	return n;
}

static moo_oop_t token_to_literal (moo_t* moo, int rdonly)
{
	switch (TOKEN_TYPE(moo))
	{
		case MOO_IOTOK_NIL:
			return moo->_nil;

		case MOO_IOTOK_TRUE:
			return moo->_true;

		case MOO_IOTOK_FALSE:
			return moo->_false;

		case MOO_IOTOK_ERRLIT:
			return string_to_error(moo, TOKEN_NAME(moo), TOKEN_LOC(moo));

		case MOO_IOTOK_SMPTRLIT:
			return string_to_smptr(moo, TOKEN_NAME(moo), TOKEN_LOC(moo));

		case MOO_IOTOK_CHARLIT:
			MOO_ASSERT (moo, TOKEN_NAME_LEN(moo) == 1);
			return MOO_CHAR_TO_OOP(TOKEN_NAME_PTR(moo)[0]);

		case MOO_IOTOK_NUMLIT:
		case MOO_IOTOK_RADNUMLIT:
		{
			moo_oop_t lit;
			lit = string_to_num(moo, TOKEN_NAME(moo), TOKEN_TYPE(moo) == MOO_IOTOK_RADNUMLIT);
			if (rdonly && lit && MOO_OOP_IS_POINTER(lit)) MOO_OBJ_SET_FLAGS_RDONLY (lit, 1);
			return lit;
		}

		case MOO_IOTOK_SYMLIT:
			/* a symbol is always set with RDONLY. no additional check is needed here */
			return moo_makesymbol(moo, TOKEN_NAME_PTR(moo) + 1, TOKEN_NAME_LEN(moo) - 1);

		case MOO_IOTOK_STRLIT:
		{
			moo_oop_t lit;
			lit = moo_instantiate(moo, moo->_string, TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo));
			if (rdonly && lit)
			{
				MOO_ASSERT (moo, MOO_OOP_IS_POINTER(lit));
				MOO_OBJ_SET_FLAGS_RDONLY (lit, 1);
			}
			return lit;
		}

		case MOO_IOTOK_IDENT:
		case MOO_IOTOK_IDENT_DOTTED:
		{
			var_info_t var;
			if (get_variable_info(moo, TOKEN_NAME(moo), TOKEN_LOC(moo), TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED, &var) <= -1) return MOO_NULL;
			if (var.type == VAR_GLOBAL)
			{
				return var.u.gbl->value;
			}
			else if (var.type == VAR_LITERAL)
			{
				return var.u.lit;
			}
			else
			{
				moo_setsynerr (moo, MOO_SYNERR_VARINACC, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return MOO_NULL;
			}

			/* [NOTE] i don't mark RDONLY on a value resolved via an identifier */
		}

		case MOO_IOTOK_HASHBRACK: /* #[ - byte array literal parenthesis */
		{
			moo_oop_t lit;
			if (read_byte_array_literal(moo, rdonly, &lit) <= -1) return MOO_NULL;
			return lit;
		}

		case MOO_IOTOK_HASHPAREN: /* #( - array literal parenthesis */
		{
			moo_oop_t lit;
			if (read_array_literal(moo, rdonly, &lit) <= -1) return MOO_NULL;
			return lit;
		}

		default:
			moo_setsynerr (moo, MOO_SYNERR_LITERAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return MOO_NULL;
	}
}

static moo_oop_t find_element_in_compiling_pooldic (moo_t* moo, const moo_oocs_t* name)
{
	moo_oow_t i;
	moo_oop_char_t s;
	moo_cunit_pooldic_t* pd;

	MOO_ASSERT (moo, moo->c->cunit->cunit_type == MOO_CUNIT_POOLDIC);
	pd = (moo_cunit_pooldic_t*)moo->c->cunit;

	for (i = pd->start; i < pd->end; i += 2)
	{
		s = (moo_oop_char_t)moo->c->arlit.ptr[i];
		MOO_ASSERT (moo, MOO_CLASSOF(moo,s) == moo->_symbol);
		if (MOO_OBJ_GET_SIZE(s) == name->len &&
		    moo_equal_oochars(name->ptr, MOO_OBJ_GET_CHAR_SLOT(s), name->len))
		{
			return moo->c->arlit.ptr[i + 1];
		}
	}

	moo_seterrnum (moo, MOO_ENOENT);
	return MOO_NULL;
}

static int __compile_pooldic_definition (moo_t* moo)
{
	moo_oop_t lit;
	moo_ooi_t tally;
	moo_oow_t i;
	moo_oow_t saved_arlit_count;
	moo_cunit_pooldic_t* pd;

	pd = (moo_cunit_pooldic_t*)moo->c->cunit;

	saved_arlit_count = moo->c->arlit.count;

	if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT && TOKEN_TYPE(moo) != MOO_IOTOK_IDENT_DOTTED)
	{
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "pooldic name expected");
		goto oops;
	}

	if (pd->cunit_parent && pd->cunit_parent->cunit_type == MOO_CUNIT_CLASS && TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		/* the pool dictionary nested inside a class cannot be multi-segmented */
		moo_setsynerrbfmt (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo), "undotted pooldic name expected");
		goto oops;
	}

#if 0
	if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT && is_restricted_word(TOKEN_NAME(moo)))
	{
		/* wrong pooldic name */
		moo_setsynerr (moo, MOO_SYNERR_NAMEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo), "invalid pooldic name");
		return -1;
	}
#endif

	/* [NOTE] TOKEN_NAME(moo) doesn't contain the full name if it's nested 
	 *        inside a class. it is merely a name that appeared in the source
	 *        code. 
	 *  TODO: compose the full name by traversing the namespace chain. */
	if (set_pooldic_fqn(moo, pd, TOKEN_NAME(moo)) <= -1) goto oops;
	pd->fqn_loc = moo->c->tok.loc;

	if (pd->cunit_parent && pd->cunit_parent->cunit_type == MOO_CUNIT_CLASS)
	{
		moo_cunit_class_t* c = (moo_cunit_class_t*)pd->cunit_parent;
		if ((moo_oop_t)c->self_oop->nsdic == moo->_nil)
		{
			/* attach a new namespace dictionary to the nsdic field of the class */
			if (!attach_nsdic_to_class(moo, c->self_oop)) return -1;
		}
		pd->ns_oop = c->self_oop->nsdic; /* TODO: if c->nsdic is nil, create one? */
	}
	else if (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT_DOTTED)
	{
		if (preprocess_dotted_name(moo, 0, MOO_NULL, &pd->fqn, &pd->fqn_loc, &pd->name, &pd->ns_oop) <= -1) goto oops;
	}
	else
	{
		pd->ns_oop = moo->sysdic;
	}

	if (moo_lookupdic(moo, (moo_oop_dic_t)pd->ns_oop, &pd->name))
	{
		/* a conflicting entry has been found */
		moo_setsynerrbfmt (moo, MOO_SYNERR_NAMEDUPL, TOKEN_LOC(moo), TOKEN_NAME(moo), "duplicate pooldic name");
		goto oops;
	}

	GET_TOKEN (moo);
	if (TOKEN_TYPE(moo) != MOO_IOTOK_LBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_LBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

	MOO_INFO2 (moo, "Defining a pool dictionary %.*js\n", pd->fqn.len, pd->fqn.ptr);

	GET_TOKEN (moo);

	pd->start = moo->c->arlit.count;
	pd->end = moo->c->arlit.count;
	while (TOKEN_TYPE(moo) == MOO_IOTOK_IDENT)
	{
		lit = moo_makesymbol(moo, TOKEN_NAME_PTR(moo), TOKEN_NAME_LEN(moo));
		if (!lit || add_to_array_literal_buffer(moo, lit) <= -1) goto oops;

		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) != MOO_IOTOK_ASSIGN)
		{
			moo_setsynerr (moo, MOO_SYNERR_ASSIGN, TOKEN_LOC(moo), TOKEN_NAME(moo));
			goto oops;
		}

		GET_TOKEN (moo);

		/* [NOTE]
		 *   values assigned to a pool dictinary member are not read-only
		 *   unlike the default initial values defined in a class */
		lit = token_to_literal(moo, 0);
		if (!lit) goto oops;

		/* for this definition, #pooldic MyPoolDic { a := 10. b := 20 },
		 * arlit_buffer contains (#a 10 #b 20) when the 'while' loop is over. */
		if (add_to_array_literal_buffer(moo, lit) <= -1) goto oops;
		pd->end = moo->c->arlit.count;

		GET_TOKEN (moo);

		/*if (TOKEN_TYPE(moo) == MOO_IOTOK_RBRACE) goto done;
		else*/ if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
		{
			moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
			goto oops;
		}

		GET_TOKEN (moo);
	}

	if (TOKEN_TYPE(moo) != MOO_IOTOK_RBRACE)
	{
		moo_setsynerr (moo, MOO_SYNERR_RBRACE, TOKEN_LOC(moo), TOKEN_NAME(moo));
		goto oops;
	}

/*done:*/
	GET_TOKEN (moo);

	tally = (moo->c->arlit.count - saved_arlit_count) / 2;
/*TODO: tally and arlit_count range check */
	/*if (!MOO_IN_SMOOI_RANGE(tally)) ERROR??*/

	pd->pd_oop = moo_makedic(moo, moo->_pool_dictionary, MOO_ALIGN(tally + 10, POOL_DICTIONARY_SIZE_ALIGN));
	if (!pd->pd_oop) goto oops;
	for (i = saved_arlit_count; i < moo->c->arlit.count; i += 2)
	{
		/* TODO: handle duplicate keys? */
		if (!moo_putatdic(moo, pd->pd_oop, moo->c->arlit.ptr[i], moo->c->arlit.ptr[i + 1])) goto oops;
	}

	/* eveything seems ok. register the pool dictionary to the main
	 * system dictionary or to the name space it belongs to */
	lit = moo_makesymbol(moo, pd->name.ptr, pd->name.len);
	if (!lit || !moo_putatdic(moo, (moo_oop_dic_t)pd->ns_oop, lit, (moo_oop_t)pd->pd_oop)) goto oops;

	moo->c->arlit.count = saved_arlit_count;
	return 0;

oops:
	moo->c->arlit.count = saved_arlit_count;
	return -1;
}

static int compile_pooldic_definition (moo_t* moo)
{
	int n;

	if (!push_cunit(moo, MOO_CUNIT_POOLDIC)) return -1;

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	n = __compile_pooldic_definition(moo);

	MOO_ASSERT (moo, moo->c->balit.count == 0);
	MOO_ASSERT (moo, moo->c->arlit.count == 0);

	pop_cunit (moo);
	return n;
}

static int compile_pragma_definition (moo_t* moo)
{
	do
	{
		GET_TOKEN (moo);

		if (TOKEN_TYPE(moo) != MOO_IOTOK_IDENT)
		{
			moo_setsynerr (moo, MOO_SYNERR_IDENT, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

#if 0
/* TODO: pragma push */
		if (is_token_word(moo, VOCA_PUSH))
		{
			/* #pragma push() - saves the pragma flags and keep the existing flags */
			/* #pragma push(reset) - saves the pragma flags and reset the flags to 0 */
		
		}
		else if (is_token_word(moo, VOCA_POP))
		{
			/* #pragma pop() */
		}
		else
#endif
		if (is_token_word(moo, VOCA_QC))
		{
			/* #pragma qc(on). #pragma qc(off) - enable/disable double-quoted comments */

			GET_TOKEN(moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_LPAREN)
			{
				moo_setsynerr (moo, MOO_SYNERR_LPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			GET_TOKEN(moo);
			if (is_token_word(moo, VOCA_ON))
			{
				/* #pragma qc(on) */
				moo->c->pragma_flags |= MOO_PRAGMA_QC;
			}
			else if (is_token_word(moo, VOCA_OFF))
			{
				/* #pragma qc(off) */
				moo->c->pragma_flags &= ~MOO_PRAGMA_QC;
			}
			else
			{
				moo_setsynerr (moo, MOO_SYNERR_DIRECTIVEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}

			GET_TOKEN(moo);
			if (TOKEN_TYPE(moo) != MOO_IOTOK_RPAREN)
			{
				moo_setsynerr (moo, MOO_SYNERR_RPAREN, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
		}
		else
		{
			moo_setsynerr (moo, MOO_SYNERR_PRAGMAINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}

		GET_TOKEN (moo);
		if (TOKEN_TYPE(moo) == MOO_IOTOK_PERIOD) goto done;
	}
	while (TOKEN_TYPE(moo) == MOO_IOTOK_COMMA);
	
	if (TOKEN_TYPE(moo) != MOO_IOTOK_PERIOD)
	{
		moo_setsynerr (moo, MOO_SYNERR_PERIOD, TOKEN_LOC(moo), TOKEN_NAME(moo));
		return -1;
	}
	
done:
	GET_TOKEN (moo);
	return 0;
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
				moo_setsynerr (moo, MOO_SYNERR_STRING, TOKEN_LOC(moo), TOKEN_NAME(moo));
				return -1;
			}
			if (begin_include(moo) <= -1) return -1;
		}
		else if (is_token_symbol(moo, VOCA_PRAGMA_S))
		{
			/*GET_TOKEN (moo);*/
			if (compile_pragma_definition(moo) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_CLASS))
		{
			/* class Selfclass(Superclass) { } */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, CLASS_TYPE_NORMAL) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_EXTEND))
		{
			/* extend Selfclass {} */
			GET_TOKEN (moo);
			if (compile_class_definition(moo, CLASS_TYPE_EXTEND) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_INTERFACE))
		{
			/* interface InterfaceName { } */
			GET_TOKEN (moo);
			if (compile_interface_definition(moo) <= -1) return -1;
		}
		else if (is_token_word(moo, VOCA_POOLDIC))
		{
			/* pooldic SharedPoolDic { ABC := 20. DEFG := 'ayz' } */
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
			moo_setsynerr(moo, MOO_SYNERR_DIRECTIVEINVAL, TOKEN_LOC(moo), TOKEN_NAME(moo));
			return -1;
		}
	}

	return 0;
}

static void gc_oopbuf (moo_t* moo, moo_oopbuf_t* oopbuf)
{
	moo_oow_t i;

	for (i = 0; i < oopbuf->count; i++)
	{
		register moo_oop_t x = moo_moveoop(moo, oopbuf->ptr[i]);
		oopbuf->ptr[i] = x;
	}
}

static void gc_compiler (moo_t* moo)
{
	/* called when garbage collection is performed */
	if (moo->c)
	{
		gc_oopbuf (moo, &moo->c->arlit);
		gc_cunit_chain (moo);
	}
}

static void gc_cunit_chain (moo_t* moo)
{
	moo_cunit_t* cunit;

	for (cunit = moo->c->cunit; cunit; cunit = cunit->cunit_parent)
	{
		switch (cunit->cunit_type)
		{
			case MOO_CUNIT_POOLDIC:
			{
				moo_cunit_pooldic_t* pd;
				pd = (moo_cunit_pooldic_t*)cunit;
				if (pd->pd_oop)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)pd->pd_oop);
					pd->pd_oop = (moo_oop_dic_t)x;
				}
				if (pd->ns_oop)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)pd->ns_oop);
					pd->ns_oop = (moo_oop_nsdic_t)x;
				}
				break;
			}

			case MOO_CUNIT_CLASS:
			{
				moo_oow_t i, j;
				moo_cunit_class_t* cc;
				cc = (moo_cunit_class_t*)cunit;

				if (cc->self_oop) 
					cc->self_oop = (moo_oop_class_t)moo_moveoop(moo, (moo_oop_t)cc->self_oop);

				if (cc->super_oop)
					cc->super_oop = moo_moveoop(moo, cc->super_oop);

				for (i = 0; i < MOO_COUNTOF(cc->var); i++)
				{
					for (j = 0; j < cc->var[i].initv_count; j++)
					{
						register moo_oop_t x = cc->var[i].initv[j].v;
						if (x) cc->var[i].initv[j].v = moo_moveoop(moo, x);
					}
				}

				if (cc->ns_oop)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)cc->ns_oop);
					cc->ns_oop = (moo_oop_nsdic_t)x;
				}

				if (cc->superns_oop)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)cc->superns_oop);
					cc->superns_oop = (moo_oop_nsdic_t)x;
				}

				gc_oopbuf (moo, &cc->ifces);

				for (i = 0; i < cc->pdimp.dcl_count; i++)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)cc->pdimp.oops[i]);
					cc->pdimp.oops[i] = (moo_oop_dic_t)x;
				}

				gc_oopbuf (moo, &cc->mth.literals);
				break;
			}

			case MOO_CUNIT_INTERFACE:
			{
				moo_cunit_interface_t* ifce;
				ifce = (moo_cunit_interface_t*)cunit;

				if (ifce->self_oop) 
					ifce->self_oop = (moo_oop_interface_t)moo_moveoop(moo, (moo_oop_t)ifce->self_oop);

				if (ifce->ns_oop)
				{
					register moo_oop_t x = moo_moveoop(moo, (moo_oop_t)ifce->ns_oop);
					ifce->ns_oop = (moo_oop_nsdic_t)x;
				}

				break;
			}
		}
	}
}

static moo_cunit_t* push_cunit (moo_t* moo, moo_cunit_type_t type)
{
	moo_cunit_t* cunit;
	moo_oow_t size;

	switch (type)
	{
		case MOO_CUNIT_POOLDIC:
			size = MOO_SIZEOF(moo_cunit_pooldic_t);
			break;

		case MOO_CUNIT_CLASS:
			size = MOO_SIZEOF(moo_cunit_class_t);
			break;

		case MOO_CUNIT_INTERFACE:
			size = MOO_SIZEOF(moo_cunit_interface_t);
			break;

		default:
			size = MOO_SIZEOF(moo_cunit_t);
			break;
	}

	cunit = (moo_cunit_t*)moo_callocmem(moo, size);
	if (!cunit) return MOO_NULL;

	cunit->cunit_type = type;
	cunit->cunit_parent = moo->c->cunit;
	moo->c->cunit = cunit;

	switch (type)
	{
		case MOO_CUNIT_POOLDIC:
		{
			/*moo_cunit_pooldic_t* pd;
			pd = (moo_cunit_pooldic_t*)cunit;*/
			/* all fields are 0s or NULLs */
			/* nothing to do */
			break;
		}

		case MOO_CUNIT_CLASS:
		{
			moo_cunit_class_t* c;
			c = (moo_cunit_class_t*)cunit;
			c->indexed_type = MOO_OBJ_TYPE_OOP; /* whether indexed or not, it's the pointer type by default */
			/* other fields are all 0s or NULLs */
			break;
		}

		case MOO_CUNIT_INTERFACE:
		{
			/*moo_cunit_interface_t* ifce;
			ifce = (moo_cunit_interface_t*)cunit;*/
			/* all fields are 0s or NULLs */
			break;
		}

		default:
			break;
	}
	return cunit;
}

static void pop_cunit (moo_t* moo)
{
	moo_cunit_t* cunit;

	MOO_ASSERT (moo, moo->c->cunit != MOO_NULL);

	cunit = moo->c->cunit;
	moo->c->cunit = cunit->cunit_parent;

	switch (cunit->cunit_type)
	{
		case MOO_CUNIT_POOLDIC:
		{
			moo_cunit_pooldic_t* pd;
			pd = (moo_cunit_pooldic_t*)cunit;
			if (pd->fqn.ptr) moo_freemem (moo, pd->fqn.ptr);
			break;
		}

		case MOO_CUNIT_CLASS:
		{
			moo_oow_t i;
			moo_cunit_class_t* c;
			c = (moo_cunit_class_t*)cunit;

			if (c->fqn.ptr) moo_freemem (moo, c->fqn.ptr); /* strbuf */
			if (c->superfqn.ptr) moo_freemem (moo, c->superfqn.ptr); /* strbuf */
			if (c->modname.ptr) moo_freemem (moo, c->modname.ptr); /* strbuf */
			if (c->ifces.ptr) moo_freemem (moo, c->ifces.ptr); /* oopbuf */

			for (i = 0; i < MOO_COUNTOF(c->var); i++)
			{
				if (c->var[i].str.ptr) moo_freemem (moo, c->var[i].str.ptr);
				if (c->var[i].initv) moo_freemem (moo, c->var[i].initv);
			}

			clear_pooldic_import_data (moo, &c->pdimp);
			clear_method_data (moo, &c->mth);

			break;
		}

		case MOO_CUNIT_INTERFACE:
		{
			moo_cunit_interface_t* ifce;
			ifce = (moo_cunit_interface_t*)cunit;

			if (ifce->fqn.ptr) moo_freemem (moo, ifce->fqn.ptr);
			break;
		}

		default:
			break;
	}

	moo_freemem (moo, cunit);
}

static void fini_compiler (moo_t* moo)
{
	/* called before the moo object is closed */
	if (moo->c)
	{
		clear_io_names (moo);

		if (moo->c->tok.name.ptr) moo_freemem (moo, moo->c->tok.name.ptr);
		if (moo->c->balit.ptr) moo_freemem (moo, moo->c->balit.ptr);
		if (moo->c->arlit.ptr) moo_freemem (moo, moo->c->arlit.ptr);

		while (moo->c->cunit) pop_cunit (moo);

		moo_freemem (moo, moo->c);
		moo->c = MOO_NULL;
	}
}

static MOO_INLINE int _compile (moo_t* moo, moo_ioimpl_t io)
{
	int n;

	if (!io)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "no IO implementation provided");
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

	/* reset pragma flags */
	moo->c->pragma_flags = 0;
	
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
	if (compile_stream(moo) <= -1) goto oops;

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

int moo_compile (moo_t* moo, moo_ioimpl_t io)
{
	int n;
	int log_default_type_mask;
	
	log_default_type_mask = moo->log.default_type_mask;
	moo->log.default_type_mask |= MOO_LOG_COMPILER;
	n = _compile (moo, io);
	moo->log.default_type_mask = log_default_type_mask;
	
	return n;
}
