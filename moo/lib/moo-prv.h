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

#ifndef _MOO_PRV_H_
#define _MOO_PRV_H_

#include "moo.h"
#include "moo-utl.h"

/* you can define this to either 1 or 2 */
#define MOO_BCODE_LONG_PARAM_SIZE 2

/* this is useful for debugging. moo_gc() can be called 
 * while moo has not been fully initialized when this is defined*/
#define MOO_SUPPORT_GC_DURING_IGNITION

/* define this to generate XXXX_CTXTEMVAR instructions */
#define MOO_USE_CTXTEMPVAR

/* define this to use the MAKE_BLOCK instruction instead of
 * PUSH_CONTEXT, PUSH_INTLIT, PUSH_INTLIT, SEND_BLOCK_COPY */
#define MOO_USE_MAKE_BLOCK

#if !defined(NDEBUG)
/* this is for gc debugging */
#define MOO_DEBUG_GC
#define MOO_DEBUG_COMPILER
/*#define MOO_DEBUG_VM_PROCESSOR*/
/*#define MOO_DEBUG_VM_EXEC*/
#define MOO_DEBUG_BIGINT
#define MOO_PROFILE_VM
#endif

/* allow the caller to drive process switching by calling
 * moo_switchprocess(). */
#define MOO_EXTERNAL_PROCESS_SWITCH

/* limit the maximum object size such that:
 *   1. an index to an object field can be represented in a small integer.
 *   2. the maximum number of bits including bit-shifts can be represented
 *      in the moo_oow_t type.
 */
#define MOO_LIMIT_OBJ_SIZE

/* TODO: delete these header inclusion lines */
#include <string.h>

#if defined(__has_builtin)
#	if __has_builtin(__builtin_memset)
#		define MOO_MEMSET(dst,src,size)  __builtin_memset(dst,src,size)
#	else
#		define MOO_MEMSET(dst,src,size)  memset(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memcpy)
#		define MOO_MEMCPY(dst,src,size)  __builtin_memcpy(dst,src,size)
#	else
#		define MOO_MEMCPY(dst,src,size)  memcpy(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memmove)
#		define MOO_MEMMOVE(dst,src,size)  __builtin_memmove(dst,src,size)
#	else
#		define MOO_MEMMOVE(dst,src,size)  memmove(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memcmp)
#		define MOO_MEMCMP(dst,src,size)  __builtin_memcmp(dst,src,size)
#	else
#		define MOO_MEMCMP(dst,src,size)  memcmp(dst,src,size)
#	endif
#elif defined(__GNUC__) && (__GNUC__  >= 3 || (defined(__GNUC_MINOR) && __GNUC__ == 2 && __GNUC_MINOR__ >= 91))
	/* gcc 2.91 or higher */
#	define MOO_MEMSET(dst,src,size)  __builtin_memset(dst,src,size)
#	define MOO_MEMCPY(dst,src,size)  __builtin_memcpy(dst,src,size)
#	define MOO_MEMMOVE(dst,src,size) __builtin_memmove(dst,src,size)
#	define MOO_MEMCMP(dst,src,size)  __builtin_memcmp(dst,src,size)
#else
#	define MOO_MEMSET(dst,src,size)  memset(dst,src,size)
#	define MOO_MEMCPY(dst,src,size)  memcpy(dst,src,size)
#	define MOO_MEMMOVE(dst,src,size) memmove(dst,src,size)
#	define MOO_MEMCMP(dst,src,size)  memcmp(dst,src,size)
#endif

#define MOO_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))


/* ========================================================================= */
/* CLASS SPEC ENCODING                                                       */
/* ========================================================================= */

/*
 * The spec field of a class object encodes the number of the fixed part
 * and the type of the indexed part. The fixed part is the number of
 * named instance variables. If the spec of a class is indexed, the object
 * of the class can be i nstantiated with the size of the indexed part.
 *
 * For example, on a platform where sizeof(moo_oow_t) is 4, 
 * the layout of the spec field of a class as an OOP value looks like this:
 * 
 *  31                           10 9   8 7 6 5 4 3  2           1 0 
 * |number of named instance variables|indexed-type|indexability|oop-tag|
 *
 * the number of named instance variables is stored in high 23 bits.
 * the indexed type takes up bit 3 to bit 8 (assuming MOO_OBJ_TYPE_BITS is 6. 
 * MOO_OBJ_TYPE_XXX enumerators are used to represent actual values).
 * and the indexability is stored in bit 2.
 *
 * The maximum number of named(fixed) instance variables for a class is:
 *     2 ^ ((BITS-IN-OOW - MOO_OOP_TAG_BITS) - MOO_OBJ_TYPE_BITS - 1) - 1
 *
 * MOO_OOP_TAG_BITS are decremented from the number of bits in OOW because
 * the spec field is always encoded as a small integer.
 *
 * The number of named instance variables can be greater than 0 if the
 * class spec is not indexed or if it's a pointer indexed class
 * (indexed_type == MOO_OBJ_TYPE_OOP)
 * 
 * indexed_type is one of the #moo_obj_type_t enumerators.
 */

/*
 * The MOO_CLASS_SPEC_MAKE() macro creates a class spec value.
 *  _class->spec = MOO_SMOOI_TO_OOP(MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_CHAR));
 */
#define MOO_CLASS_SPEC_MAKE(named_instvar,is_indexed,indexed_type) ( \
	(((moo_oow_t)(named_instvar)) << (MOO_OBJ_FLAGS_TYPE_BITS + 1)) |  \
	(((moo_oow_t)(indexed_type)) << 1) | (((moo_oow_t)is_indexed) & 1) )

/* what is the number of named instance variables? 
 *  MOO_CLASS_SPEC_NAMED_INSTVAR(MOO_OOP_TO_SMOOI(_class->spec))
 */
#define MOO_CLASS_SPEC_NAMED_INSTVAR(spec) \
	(((moo_oow_t)(spec)) >> (MOO_OBJ_FLAGS_TYPE_BITS + 1))

/* is it a user-indexable class? 
 * all objects can be indexed with basicAt:.
 * this indicates if an object can be instantiated with a dynamic size
 * (new: size) and and can be indexed with at:.
 */
#define MOO_CLASS_SPEC_IS_INDEXED(spec) (((moo_oow_t)(spec)) & 1)

/* if so, what is the indexing type? character? pointer? etc? */
#define MOO_CLASS_SPEC_INDEXED_TYPE(spec) \
	((((moo_oow_t)(spec)) >> 1) & MOO_LBMASK(moo_oow_t, MOO_OBJ_FLAGS_TYPE_BITS))

/* What is the maximum number of named instance variables?
 * This limit is set so because the number must be encoded into the spec field
 * of the class with limited number of bits assigned to the number of
 * named instance variables. the trailing -1 in the calculation of number of
 * bits is to consider the sign bit of a small-integer which is a typical
 * type of the spec field in the class object.
 */
/*
#define MOO_MAX_NAMED_INSTVARS \
	MOO_BITS_MAX(moo_oow_t, MOO_OOW_BITS - MOO_OOP_TAG_BITS - (MOO_OBJ_FLAGS_TYPE_BITS + 1) - 1)
*/
#define MOO_MAX_NAMED_INSTVARS \
	MOO_BITS_MAX(moo_oow_t, MOO_SMOOI_ABS_BITS - (MOO_OBJ_FLAGS_TYPE_BITS + 1))

/* Given the number of named instance variables, what is the maximum number 
 * of indexed instance variables? The number of indexed instance variables
 * is not stored in the spec field of the class. It only affects the actual
 * size of an object(obj->_size) selectively combined with the number of 
 * named instance variables. So it's the maximum value of obj->_size minus
 * the number of named instance variables.
 */
#define MOO_MAX_INDEXED_INSTVARS(named_instvar) (MOO_OBJ_SIZE_MAX - named_instvar)

/*
#define MOO_CLASS_SELFSPEC_MAKE(class_var,classinst_var) \
	(((moo_oow_t)class_var) << ((MOO_OOW_BITS - MOO_OOP_TAG_BITS) / 2)) | ((moo_oow_t)classinst_var)
*/
#define MOO_CLASS_SELFSPEC_MAKE(class_var,classinst_var) \
	(((moo_oow_t)class_var) << (MOO_SMOOI_BITS / 2)) | ((moo_oow_t)classinst_var)

/*
#define MOO_CLASS_SELFSPEC_CLASSVAR(spec) ((moo_oow_t)spec >> ((MOO_OOW_BITS - MOO_OOP_TAG_BITS) / 2))
#define MOO_CLASS_SELFSPEC_CLASSINSTVAR(spec) (((moo_oow_t)spec) & MOO_LBMASK(moo_oow_t, (MOO_OOW_BITS - MOO_OOP_TAG_BITS) / 2))
*/
#define MOO_CLASS_SELFSPEC_CLASSVAR(spec) ((moo_oow_t)spec >> (MOO_SMOOI_BITS / 2))
#define MOO_CLASS_SELFSPEC_CLASSINSTVAR(spec) (((moo_oow_t)spec) & MOO_LBMASK(moo_oow_t, (MOO_SMOOI_BITS / 2)))

/*
 * yet another -1 in the calculation of the bit numbers for signed nature of
 * a small-integer
 */
/*
#define MOO_MAX_CLASSVARS      MOO_BITS_MAX(moo_oow_t, (MOO_OOW_BITS - MOO_OOP_TAG_BITS - 1) / 2)
#define MOO_MAX_CLASSINSTVARS  MOO_BITS_MAX(moo_oow_t, (MOO_OOW_BITS - MOO_OOP_TAG_BITS - 1) / 2)
*/
#define MOO_MAX_CLASSVARS      MOO_BITS_MAX(moo_oow_t, MOO_SMOOI_ABS_BITS / 2)
#define MOO_MAX_CLASSINSTVARS  MOO_BITS_MAX(moo_oow_t, MOO_SMOOI_ABS_BITS / 2)


#if defined(MOO_LIMIT_OBJ_SIZE)
/* limit the maximum object size such that:
 *   1. an index to an object field can be represented in a small integer.
 *   2. the maximum number of bit shifts can be represented in the moo_oow_t type.
 */
#	define MOO_OBJ_SIZE_MAX ((moo_oow_t)MOO_SMOOI_MAX)
#	define MOO_OBJ_SIZE_BITS_MAX (MOO_OBJ_SIZE_MAX * 8)
#else
#	define MOO_OBJ_SIZE_MAX ((moo_oow_t)MOO_TYPE_MAX(moo_oow_t))
#	define MOO_OBJ_SIZE_BITS_MAX (MOO_OBJ_SIZE_MAX * 8)
#endif


#if defined(MOO_INCLUDE_COMPILER)

/* ========================================================================= */
/* SOURCE CODE I/O FOR COMPILER                                              */
/* ========================================================================= */
struct moo_iolxc_t
{
	moo_ooci_t   c; /**< character */
	moo_ioloc_t  l; /**< location */
};
typedef struct moo_iolxc_t moo_iolxc_t;

/*
enum moo_ioarg_flag_t
{
	MOO_IO_INCLUDED = (1 << 0)
};
typedef enum moo_ioarg_flag_t moo_ioarg_flag_t; */

struct moo_ioarg_t
{
	/** 
	 * [IN] I/O object name.
	 * It is #MOO_NULL for the main stream and points to a non-NULL string
	 * for an included stream.
	 */
	const moo_ooch_t* name;   

	/** 
	 * [OUT] I/O handle set by an open handler. 
	 * [IN] I/O handle referenced in read and close handler.
	 * The source stream handler can set this field when it opens a stream.
	 * All subsequent operations on the stream see this field as set
	 * during opening.
	 */
	void* handle;

	/**
	 * [OUT] place data here 
	 */
	moo_ooch_t buf[1024];

	/**
	 * [IN] points to the data of the includer. It is #MOO_NULL for the
	 * main stream.
	 */
	moo_ioarg_t* includer;

	/*-----------------------------------------------------------------*/
	/*----------- from here down, internal use only -------------------*/
	struct
	{
		int pos, len, state;
	} b;

	unsigned long int line;
	unsigned long int colm;
	moo_ooci_t       nl;

	moo_iolxc_t lxc;
	/*-----------------------------------------------------------------*/
};

struct moo_iotok_t
{
	enum
	{
		MOO_IOTOK_EOF,
		
		MOO_IOTOK_CHARLIT,
		MOO_IOTOK_STRLIT,
		MOO_IOTOK_SYMLIT,
		MOO_IOTOK_NUMLIT,
		MOO_IOTOK_RADNUMLIT,
		MOO_IOTOK_ERRLIT, /* error(NN) */
		MOO_IOTOK_ERROR, /* error */
		MOO_IOTOK_NIL,
		MOO_IOTOK_SELF,
		MOO_IOTOK_SUPER,
		MOO_IOTOK_TRUE,
		MOO_IOTOK_FALSE,
		MOO_IOTOK_THIS_CONTEXT,
		MOO_IOTOK_THIS_PROCESS,
		MOO_IOTOK_IDENT,
		MOO_IOTOK_IDENT_DOTTED,
		MOO_IOTOK_BINSEL,
		MOO_IOTOK_KEYWORD,
		MOO_IOTOK_ASSIGN,
		MOO_IOTOK_COLON,
		MOO_IOTOK_RETURN, /* ^ */
		MOO_IOTOK_LOCAL_RETURN, /* ^^ */
		MOO_IOTOK_LBRACE,
		MOO_IOTOK_RBRACE,
		MOO_IOTOK_LBRACK,
		MOO_IOTOK_RBRACK,
		MOO_IOTOK_LPAREN,
		MOO_IOTOK_RPAREN,
		MOO_IOTOK_APAREN,   /* #( - array literal */
		MOO_IOTOK_BABRACK,  /* #[ - byte array literal */
		MOO_IOTOK_ABRACE,   /* #{ - array expression */
		MOO_IOTOK_DICBRACE, /* :{ - dictionary expression */
		MOO_IOTOK_ASSPAREN, /* :( - association expression */
		MOO_IOTOK_PERIOD,
		MOO_IOTOK_COMMA,
		MOO_IOTOK_SEMICOLON,

		MOO_IOTOK_IF,
		MOO_IOTOK_ELSE,
		MOO_IOTOK_ELSIF,

		MOO_IOTOK_WHILE,
		MOO_IOTOK_DO,
		MOO_IOTOK_BREAK,
		MOO_IOTOK_CONTINUE
	} type;

	moo_oocs_t name;
	moo_oow_t name_capa;

	moo_ioloc_t loc;
};
typedef struct moo_iotok_t moo_iotok_t;

typedef struct moo_iolink_t moo_iolink_t;
struct moo_iolink_t
{
	moo_iolink_t* link;
};

typedef struct moo_code_t moo_code_t;
struct moo_code_t
{
	moo_uint8_t* ptr;
	moo_oow_t    len;
};

typedef struct moo_oow_pool_chunk_t moo_oow_pool_chunk_t;
struct moo_oow_pool_chunk_t
{
	moo_oow_t buf[16];
	moo_oow_pool_chunk_t* next;
};

typedef struct moo_oow_pool_t moo_oow_pool_t;
struct moo_oow_pool_t
{
	moo_oow_pool_chunk_t  static_chunk;
	moo_oow_pool_chunk_t* head;
	moo_oow_pool_chunk_t* tail;
	moo_oow_t             count;
};

enum moo_loop_type_t
{
	MOO_LOOP_WHILE,
	MOO_LOOP_DO_WHILE
};
typedef enum moo_loop_type_t moo_loop_type_t;

typedef struct moo_loop_t moo_loop_t;
struct moo_loop_t
{
	moo_loop_type_t type;
	moo_oow_t startpos;
	moo_oow_pool_t break_ip_pool; /* a pool that holds jump instruction pointers for break */
	moo_oow_pool_t continue_ip_pool; /* a pool that hold jump instructino pointers for continue. only for do-while */
	moo_oow_t blkcount; /* number of inner blocks enclosed in square brackets */
	moo_loop_t* next;
};

struct moo_compiler_t
{
	/* input handler */
	moo_ioimpl_t impl;

	/* information about the last meaningful character read.
	 * this is a copy of curinp->lxc if no ungetting is performed.
	 * if there is something in the unget buffer, this is overwritten
	 * by a value from the buffer when the request to read a character
	 * is served */
	moo_iolxc_t  lxc;

	/* unget buffer */
	moo_iolxc_t  ungot[10];
	int          nungots;

	/* static input data buffer */
	moo_ioarg_t  arg;    

	/* pointer to the current input data. initially, it points to &arg */
	moo_ioarg_t* curinp;

	/* the last token read */
	moo_iotok_t  tok;
	moo_iolink_t* io_names;
	int in_array;

	moo_synerr_t synerr;

	/* temporary space to handle an illegal character */
	moo_ooch_t ilchr;
	moo_oocs_t ilchr_ucs;

	/* information about a class being compiled */
	struct
	{
		int flags;
		int indexed_type;

		moo_oop_class_t self_oop;
		moo_oop_t super_oop; /* this may be nil. so the type is moo_oop_t */
#ifdef MTHDIC
		moo_oop_set_t mthdic_oop[2]; /* used when compiling a method definition */
#endif
		moo_oop_set_t pooldic_oop; /* used when compiling a pooldic definition */
		moo_oop_set_t ns_oop;
		moo_oocs_t fqn;
		moo_oocs_t name;
		moo_oow_t fqn_capa;
		moo_ioloc_t fqn_loc;

		moo_oop_set_t superns_oop;
		moo_oocs_t superfqn;
		moo_oocs_t supername;
		moo_oow_t superfqn_capa;
		moo_ioloc_t superfqn_loc;

		/* instance variable, class variable, class instance variable */
		moo_oocs_t vars[3]; 
		moo_oow_t vars_capa[3];

		/* var_count, unlike vars above, includes superclass counts as well.
		 * var_count[0] - number of named instance variables
		 * var_count[1] - number of class variables
		 * var_count[2] - number of class instance variables */
		moo_oow_t var_count[3];

		/* buffer to hold pooldic import declaration */
		moo_oocs_t pooldic;
		moo_oow_t pooldic_capa;
		moo_oow_t pooldic_count;

		/* used to hold imported pool dictionarie objects */
		moo_oop_set_t* pooldic_imp_oops; 
		moo_oow_t pooldic_imp_oops_capa;
	} cls;

	/* information about a method being comipled */
	struct
	{
		moo_method_type_t type;

		/* method source text */
		moo_oocs_t text;
		moo_oow_t text_capa;

		/* buffer to store identifier names to be assigned */
		moo_oocs_t assignees;
		moo_oow_t assignees_capa;

		/* buffer to store binary selectors being worked on */
		moo_oocs_t binsels;
		moo_oow_t binsels_capa;

		/* buffer to store keyword selectors being worked on */
		moo_oocs_t kwsels;
		moo_oow_t kwsels_capa;

		/* method name */
		moo_oocs_t name;
		moo_oow_t name_capa;
		moo_ioloc_t name_loc;

		/* is the unary method followed by parameter list? */
		int variadic;

		/* single string containing a space separated list of temporaries */
		moo_oocs_t tmprs; 
		moo_oow_t tmprs_capa;
		moo_oow_t tmpr_count; /* total number of temporaries including arguments */
		moo_oow_t tmpr_nargs;

		/* literals */
		moo_oop_t* literals;
		moo_oow_t literal_count;
		moo_oow_t literal_capa;

		/* byte array elements */
		moo_oob_t* balit;
		moo_oow_t balit_count;
		moo_oow_t balit_capa;

		/* array elements */
		moo_oop_t* arlit;
		moo_oow_t arlit_count;
		moo_oow_t arlit_capa;

		/* 0 for no primitive, 1 for a normal primitive, 2 for a named primitive */
		int pftype;
		/* primitive function number */
		moo_ooi_t pfnum; 

		/* block depth */
		moo_oow_t blk_depth;
		moo_oow_t* blk_tmprcnt;
		moo_oow_t blk_tmprcnt_capa;

		/* information about loop constructs */
		moo_loop_t* loop;

		/* byte code */
		moo_code_t code;
		moo_oow_t code_capa;
	} mth; 
};



/*
typedef struct moo_bchbuf_t moo_bchbuf_t;
struct moo_bchbuf_t
{
	moo_bch_t  buf[128];
	moo_bch_t* ptr;
	moo_oow_t  capa;
};

typedef struct moo_oochbuf_t moo_oochbuf_t;
struct moo_oochbuf_t
{
	moo_ooch_t  buf[128];
	moo_ooch_t* ptr;
	moo_oow_t   capa;
};
*/

#endif

#if defined(MOO_USE_OBJECT_TRAILER)
	/* let it point to the trailer of the method */
#	define SET_ACTIVE_METHOD_CODE(moo) ((moo)->active_code = (moo_oob_t*)&(moo)->active_method->slot[MOO_OBJ_GET_SIZE((moo)->active_method) + 1 - MOO_METHOD_NAMED_INSTVARS])
#else
	/* let it point to the payload of the code byte array */
#	define SET_ACTIVE_METHOD_CODE(moo) ((moo)->active_code = (moo)->active_method->code->slot)
#endif

#if defined(MOO_BCODE_LONG_PARAM_SIZE) && (MOO_BCODE_LONG_PARAM_SIZE == 1)
#	define MAX_CODE_INDEX               (0xFFu)
#	define MAX_CODE_NTMPRS              (0xFFu)
#	define MAX_CODE_NARGS               (0xFFu)
#	define MAX_CODE_NBLKARGS            (0xFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFu)
#	define MAX_CODE_JUMP                (0xFFu)
#	define MAX_CODE_PARAM               (0xFFu)
#elif defined(MOO_BCODE_LONG_PARAM_SIZE) && (MOO_BCODE_LONG_PARAM_SIZE == 2)
#	define MAX_CODE_INDEX               (0xFFFFu)
#	define MAX_CODE_NTMPRS              (0xFFFFu)
#	define MAX_CODE_NARGS               (0xFFFFu)
#	define MAX_CODE_NBLKARGS            (0xFFFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFFFu)
#	define MAX_CODE_JUMP                (0xFFFFu)
#	define MAX_CODE_PARAM               (0xFFFFu)
#else
#	error Unsupported MOO_BCODE_LONG_PARAM_SIZE
#endif


/*
----------------------------------------------------------------------------------------------------------------
SHORT INSTRUCTION CODE                                        LONG INSTRUCTION CODE
----------------------------------------------------------------------------------------------------------------
                                                                      v v
0-3      0000 00XX STORE_INTO_INSTVAR                         128  1000 0000 XXXXXXXX STORE_INTO_INSTVAR_X                    (bit 4 off, bit 3 off) 
4-7      0000 01XX STORE_INTO_INSTVAR
8-11     0000 10XX POP_INTO_INSTVAR                           136  1000 1000 XXXXXXXX POP_INTO_INSTVAR_X                      (bit 4 off, bit 3 on)
12-15    0000 11XX POP_INTO_INSTVAR
16-19    0001 00XX PUSH_INSTVAR                               144  1001 0000 XXXXXXXX PUSH_INSTVAR_X                          (bit 4 on)
20-23    0001 01XX PUSH_INSTVAR

                                                                      v v
24-27    0001 10XX PUSH_TEMPVAR                               152  1001 1000 XXXXXXXX PUSH_TEMPVAR_X                          (bit 4 on)
28-31    0001 11XX PUSH_TEMPVAR
32-35    0010 00XX STORE_INTO_TEMPVAR                         160  1010 0000 XXXXXXXX STORE_INTO_TEMPVAR_X                    (bit 4 off, bit 3 off)
36-39    0010 01XX STORE_INTO_TEMPVAR
40-43    0010 10XX POP_INTO_TEMPVAR                           168  1010 1000 XXXXXXXX POP_INTO_TEMPVAR_X                      (bit 4 off, bit 3 on)
44-47    0010 11XX POP_INTO_TEMPVAR

48-51    0011 00XX PUSH_LITERAL                               176  1011 0000 XXXXXXXX PUSH_LITERAL_X
52-55    0011 01XX PUSH_LITERAL

                                                                        vv
56-59    0011 10XX STORE_INTO_OBJECT                          184  1011 1000 XXXXXXXX STORE_INTO_OBJECT                       (bit 3 on, bit 2 off)
60-63    0011 11XX POP_INTO_OBJECT                            188  1011 1100 XXXXXXXX POP_INTO_OBJECT                         (bit 3 on, bit 2 on)
64-67    0100 00XX PUSH_OBJECT                                192  1100 0000 XXXXXXXX PUSH_OBJECT                             (bit 3 off)


68-71    0100 01XX JUMP_FORWARD                               196  1100 0100 XXXXXXXX JUMP_FORWARD_X
72-75    0100 10XX JUMP_BACKWARD                              200  1100 1000 XXXXXXXX JUMP_BACKWARD_X
76-79    0100 11XX JUMP_BACKWARD_IF_FALSE                     204  1100 1100 XXXXXXXX JUMP_BACKWARD_IF_FALSE_X
80-83    0101 00XX JUMP_FORWARD_IF_FALSE                      208  1101 0000 XXXXXXXX JUMP_FORWARD_IF_FALSE_X
84-87    0101 01XX JUMP_FORWARD_IF_TRUE                       212  1101 0100 XXXXXXXX JUMP_FORWARD_IF_TRUE_X

                                                                        vv
88-91    0101 10XX YYYYYYYY STORE_INTO_CTXTEMPVAR             216  1101 1000 XXXXXXXX YYYYYYYY STORE_INTO_CTXTEMPVAR_X        (bit 3 on, bit 2 off)
92-95    0101 11XX YYYYYYYY POP_INTO_CTXTEMPVAR               220  1101 1100 XXXXXXXX YYYYYYYY POP_INTO_CTXTEMPVAR_X          (bit 3 on, bit 2 on)
96-99    0110 00XX YYYYYYYY PUSH_CTXTEMPVAR                   224  1110 0000 XXXXXXXX YYYYYYYY PUSH_CTXTEMPVAR_X              (bit 3 off)
# XXXth outer-frame, YYYYYYYY local variable

100-103  0110 01XX YYYYYYYY PUSH_OBJVAR                       228  1110 0100 XXXXXXXX YYYYYYYY PUSH_OBJVAR_X                  (bit 3 off)
104-107  0110 10XX YYYYYYYY STORE_INTO_OBJVAR                 232  1110 1000 XXXXXXXX YYYYYYYY STORE_INTO_OBJVAR_X            (bit 3 on, bit 2 off)
108-111  0110 11XX YYYYYYYY POP_INTO_OBJVAR                   236  1110 1100 XXXXXXXX YYYYYYYY POP_INTO_OBJVAR_X              (bit 3 on, bit 2 on)
# XXXth instance variable of YYYYYYYY object

                                                                         v
112-115  0111 00XX YYYYYYYY SEND_MESSAGE                      240  1111 0000 XXXXXXXX YYYYYYYY SEND_MESSAGE_X                 (bit 2 off)
116-119  0111 01XX YYYYYYYY SEND_MESSAGE_TO_SUPER             244  1111 0100 XXXXXXXX YYYYYYYY SEND_MESSAGE_TO_SUPER_X        (bit 2 on)
# XXX args, YYYYYYYY message

120-123  0111 10XX  UNUSED
124-127  0111 11XX  UNUSED

##
## "SHORT_CODE_0 | 0x80" becomes "LONG_CODE_X".
## A special single byte instruction is assigned an unused number greater than 128.
##
*/

enum moo_bcode_t
{
	BCODE_STORE_INTO_INSTVAR_0     = 0x00,
	BCODE_STORE_INTO_INSTVAR_1     = 0x01,
	BCODE_STORE_INTO_INSTVAR_2     = 0x02,
	BCODE_STORE_INTO_INSTVAR_3     = 0x03,

	BCODE_STORE_INTO_INSTVAR_4     = 0x04,
	BCODE_STORE_INTO_INSTVAR_5     = 0x05,
	BCODE_STORE_INTO_INSTVAR_6     = 0x06,
	BCODE_STORE_INTO_INSTVAR_7     = 0x07,

	BCODE_POP_INTO_INSTVAR_0       = 0x08,
	BCODE_POP_INTO_INSTVAR_1       = 0x09,
	BCODE_POP_INTO_INSTVAR_2       = 0x0A,
	BCODE_POP_INTO_INSTVAR_3       = 0x0B,

	BCODE_POP_INTO_INSTVAR_4       = 0x0C,
	BCODE_POP_INTO_INSTVAR_5       = 0x0D,
	BCODE_POP_INTO_INSTVAR_6       = 0x0E,
	BCODE_POP_INTO_INSTVAR_7       = 0x0F,

	BCODE_PUSH_INSTVAR_0           = 0x10,
	BCODE_PUSH_INSTVAR_1           = 0x11,
	BCODE_PUSH_INSTVAR_2           = 0x12,
	BCODE_PUSH_INSTVAR_3           = 0x13,

	BCODE_PUSH_INSTVAR_4           = 0x14,
	BCODE_PUSH_INSTVAR_5           = 0x15,
	BCODE_PUSH_INSTVAR_6           = 0x16,
	BCODE_PUSH_INSTVAR_7           = 0x17,

	BCODE_PUSH_TEMPVAR_0           = 0x18,
	BCODE_PUSH_TEMPVAR_1           = 0x19,
	BCODE_PUSH_TEMPVAR_2           = 0x1A,
	BCODE_PUSH_TEMPVAR_3           = 0x1B,

	BCODE_PUSH_TEMPVAR_4           = 0x1C,
	BCODE_PUSH_TEMPVAR_5           = 0x1D,
	BCODE_PUSH_TEMPVAR_6           = 0x1E,
	BCODE_PUSH_TEMPVAR_7           = 0x1F,

	BCODE_STORE_INTO_TEMPVAR_0     = 0x20,
	BCODE_STORE_INTO_TEMPVAR_1     = 0x21,
	BCODE_STORE_INTO_TEMPVAR_2     = 0x22,
	BCODE_STORE_INTO_TEMPVAR_3     = 0x23,

	BCODE_STORE_INTO_TEMPVAR_4     = 0x24,
	BCODE_STORE_INTO_TEMPVAR_5     = 0x25,
	BCODE_STORE_INTO_TEMPVAR_6     = 0x26,
	BCODE_STORE_INTO_TEMPVAR_7     = 0x27,

	BCODE_POP_INTO_TEMPVAR_0       = 0x28,
	BCODE_POP_INTO_TEMPVAR_1       = 0x29,
	BCODE_POP_INTO_TEMPVAR_2       = 0x2A,
	BCODE_POP_INTO_TEMPVAR_3       = 0x2B,

	BCODE_POP_INTO_TEMPVAR_4       = 0x2C,
	BCODE_POP_INTO_TEMPVAR_5       = 0x2D,
	BCODE_POP_INTO_TEMPVAR_6       = 0x2E,
	BCODE_POP_INTO_TEMPVAR_7       = 0x2F,

	BCODE_PUSH_LITERAL_0           = 0x30,
	BCODE_PUSH_LITERAL_1           = 0x31,
	BCODE_PUSH_LITERAL_2           = 0x32,
	BCODE_PUSH_LITERAL_3           = 0x33,

	BCODE_PUSH_LITERAL_4           = 0x34,
	BCODE_PUSH_LITERAL_5           = 0x35,
	BCODE_PUSH_LITERAL_6           = 0x36,
	BCODE_PUSH_LITERAL_7           = 0x37,

	/* -------------------------------------- */

	BCODE_STORE_INTO_OBJECT_0      = 0x38,
	BCODE_STORE_INTO_OBJECT_1      = 0x39,
	BCODE_STORE_INTO_OBJECT_2      = 0x3A,
	BCODE_STORE_INTO_OBJECT_3      = 0x3B,

	BCODE_POP_INTO_OBJECT_0        = 0x3C,
	BCODE_POP_INTO_OBJECT_1        = 0x3D,
	BCODE_POP_INTO_OBJECT_2        = 0x3E,
	BCODE_POP_INTO_OBJECT_3        = 0x3F,

	BCODE_PUSH_OBJECT_0            = 0x40,
	BCODE_PUSH_OBJECT_1            = 0x41,
	BCODE_PUSH_OBJECT_2            = 0x42,
	BCODE_PUSH_OBJECT_3            = 0x43,

	BCODE_JUMP_FORWARD_0           = 0x44, /* 68 */
	BCODE_JUMP_FORWARD_1           = 0x45, /* 69 */
	BCODE_JUMP_FORWARD_2           = 0x46, /* 70 */
	BCODE_JUMP_FORWARD_3           = 0x47, /* 71 */

	BCODE_JUMP_BACKWARD_0          = 0x48,
	BCODE_JUMP_BACKWARD_1          = 0x49,
	BCODE_JUMP_BACKWARD_2          = 0x4A,
	BCODE_JUMP_BACKWARD_3          = 0x4B,

	BCODE_JUMP_FORWARD_IF_FALSE_0  = 0x4C, /* 76 */
	BCODE_JUMP_FORWARD_IF_FALSE_1  = 0x4D, /* 77 */
	BCODE_JUMP_FORWARD_IF_FALSE_2  = 0x4E, /* 78 */
	BCODE_JUMP_FORWARD_IF_FALSE_3  = 0x4F, /* 79 */

	BCODE_JUMP_BACKWARD_IF_FALSE_0 = 0x50, /* 80 */
	BCODE_JUMP_BACKWARD_IF_FALSE_1 = 0x51, /* 81 */
	BCODE_JUMP_BACKWARD_IF_FALSE_2 = 0x52, /* 82 */
	BCODE_JUMP_BACKWARD_IF_FALSE_3 = 0x53, /* 83 */

	BCODE_JUMP_BACKWARD_IF_TRUE_0  = 0x54, /* 84 */
	BCODE_JUMP_BACKWARD_IF_TRUE_1  = 0x55, /* 85 */
	BCODE_JUMP_BACKWARD_IF_TRUE_2  = 0x56, /* 86 */
	BCODE_JUMP_BACKWARD_IF_TRUE_3  = 0x57, /* 87 */

	BCODE_STORE_INTO_CTXTEMPVAR_0  = 0x58, /* 88 */
	BCODE_STORE_INTO_CTXTEMPVAR_1  = 0x59, /* 89 */
	BCODE_STORE_INTO_CTXTEMPVAR_2  = 0x5A, /* 90 */
	BCODE_STORE_INTO_CTXTEMPVAR_3  = 0x5B, /* 91 */

	BCODE_POP_INTO_CTXTEMPVAR_0    = 0x5C, /* 92 */
	BCODE_POP_INTO_CTXTEMPVAR_1    = 0x5D, /* 93 */
	BCODE_POP_INTO_CTXTEMPVAR_2    = 0x5E, /* 94 */
	BCODE_POP_INTO_CTXTEMPVAR_3    = 0x5F, /* 95 */

	BCODE_PUSH_CTXTEMPVAR_0        = 0x60, /* 96 */
	BCODE_PUSH_CTXTEMPVAR_1        = 0x61, /* 97 */
	BCODE_PUSH_CTXTEMPVAR_2        = 0x62, /* 98 */
	BCODE_PUSH_CTXTEMPVAR_3        = 0x63, /* 99 */

	BCODE_PUSH_OBJVAR_0            = 0x64,
	BCODE_PUSH_OBJVAR_1            = 0x65,
	BCODE_PUSH_OBJVAR_2            = 0x66,
	BCODE_PUSH_OBJVAR_3            = 0x67,

	BCODE_STORE_INTO_OBJVAR_0      = 0x68,
	BCODE_STORE_INTO_OBJVAR_1      = 0x69,
	BCODE_STORE_INTO_OBJVAR_2      = 0x6A,
	BCODE_STORE_INTO_OBJVAR_3      = 0x6B,

	BCODE_POP_INTO_OBJVAR_0        = 0x6C,
	BCODE_POP_INTO_OBJVAR_1        = 0x6D,
	BCODE_POP_INTO_OBJVAR_2        = 0x6E,
	BCODE_POP_INTO_OBJVAR_3        = 0x6F,

	BCODE_SEND_MESSAGE_0           = 0x70,
	BCODE_SEND_MESSAGE_1           = 0x71,
	BCODE_SEND_MESSAGE_2           = 0x72,
	BCODE_SEND_MESSAGE_3           = 0x73,

	BCODE_SEND_MESSAGE_TO_SUPER_0  = 0x74,
	BCODE_SEND_MESSAGE_TO_SUPER_1  = 0x75,
	BCODE_SEND_MESSAGE_TO_SUPER_2  = 0x76,
	BCODE_SEND_MESSAGE_TO_SUPER_3  = 0x77,

	/* UNUSED 0x78 - 0x7F */

	BCODE_STORE_INTO_INSTVAR_X     = 0x80, /* 128 ## */

	BCODE_PUSH_RECEIVER            = 0x81, /* 129 */
	BCODE_PUSH_NIL                 = 0x82, /* 130 */
	BCODE_PUSH_TRUE                = 0x83, /* 131 */
	BCODE_PUSH_FALSE               = 0x84, /* 132 */
	BCODE_PUSH_CONTEXT             = 0x85, /* 133 */
	BCODE_PUSH_PROCESS             = 0x86, /* 134 */
	BCODE_PUSH_NEGONE              = 0x87, /* 135 */

	BCODE_POP_INTO_INSTVAR_X       = 0x88, /* 136 ## */ 

	BCODE_PUSH_ZERO                = 0x89, /* 137 */
	BCODE_PUSH_ONE                 = 0x8A, /* 138 */
	BCODE_PUSH_TWO                 = 0x8B, /* 139 */

	BCODE_PUSH_INSTVAR_X           = 0x90, /* 144 ## */
	BCODE_PUSH_TEMPVAR_X           = 0x98, /* 152 ## */
	BCODE_STORE_INTO_TEMPVAR_X     = 0xA0, /* 160 ## */
	BCODE_POP_INTO_TEMPVAR_X       = 0xA8, /* 168 ## */
	BCODE_PUSH_LITERAL_X           = 0xB0, /* 176 ## */

	/* UNUSED - 0xB1 */
	BCODE_PUSH_INTLIT              = 0xB2, /* 178 */
	BCODE_PUSH_NEGINTLIT           = 0xB3, /* 179 */
	BCODE_PUSH_CHARLIT             = 0xB4, /* 180 */

	BCODE_STORE_INTO_OBJECT_X      = 0xB8, /* 184 ## */
	BCODE_POP_INTO_OBJECT_X        = 0xBC, /* 188 ## */
	BCODE_PUSH_OBJECT_X            = 0xC0, /* 192 ## */

	BCODE_JUMP_FORWARD_X           = 0xC4, /* 196 ## */
	BCODE_JUMP2_FORWARD            = 0xC5, /* 197 */
	BCODE_JUMP_BACKWARD_X          = 0xC8, /* 200 ## */
	BCODE_JUMP2_BACKWARD           = 0xC9, /* 201 */

	BCODE_JUMP_FORWARD_IF_FALSE_X  = 0xCC, /* 204 ## */
	BCODE_JUMP2_FORWARD_IF_FALSE   = 0xCD, /* 205 */
	BCODE_JUMP_BACKWARD_IF_FALSE_X = 0xD0, /* 208 ## */
	BCODE_JUMP2_BACKWARD_IF_FALSE  = 0xD1, /* 209 */
	BCODE_JUMP_BACKWARD_IF_TRUE_X  = 0xD4, /* 212 ## */
	BCODE_JUMP2_BACKWARD_IF_TRUE   = 0xD5, /* 213 */

	BCODE_STORE_INTO_CTXTEMPVAR_X  = 0xD8, /* 216 ## */
	BCODE_POP_INTO_CTXTEMPVAR_X    = 0xDC, /* 220 ## */
	BCODE_PUSH_CTXTEMPVAR_X        = 0xE0, /* 224 ## */

	BCODE_PUSH_OBJVAR_X            = 0xE4, /* 228 ## */
	BCODE_STORE_INTO_OBJVAR_X      = 0xE8, /* 232 ## */
	BCODE_POP_INTO_OBJVAR_X        = 0xEC, /* 236 ## */

	BCODE_SEND_MESSAGE_X             = 0xF0, /* 240 ## */
	BCODE_MAKE_ASSOCIATION           = 0xF1, /* 241 */
	BCODE_POP_INTO_ASSOCIATION_KEY   = 0xF2, /* 242 */
	BCODE_POP_INTO_ASSOCIATION_VALUE = 0xF3, /* 243 */
	BCODE_SEND_MESSAGE_TO_SUPER_X    = 0xF4, /* 244 ## */

	/* -------------------------------------- */

	BCODE_MAKE_ARRAY               = 0xF5, /* 245 */
	BCODE_POP_INTO_ARRAY           = 0xF6, /* 246 */
	BCODE_DUP_STACKTOP             = 0xF7,
	BCODE_POP_STACKTOP             = 0xF8,
	BCODE_RETURN_STACKTOP          = 0xF9, /* ^something */
	BCODE_RETURN_RECEIVER          = 0xFA, /* ^self */
	BCODE_RETURN_FROM_BLOCK        = 0xFB, /* return the stack top from a block */
	BCODE_LOCAL_RETURN             = 0xFC,
	BCODE_MAKE_BLOCK               = 0xFD,
	BCODE_SEND_BLOCK_COPY          = 0xFE,
	BCODE_NOOP                     = 0xFF
};

#if defined(__cplusplus)
extern "C" {
#endif


/* ========================================================================= */
/* heap.c                                                                    */
/* ========================================================================= */

/**
 * The moo_makeheap() function creates a new heap of the \a size bytes.
 *
 * \return heap pointer on success and #MOO_NULL on failure.
 */
moo_heap_t* moo_makeheap (
	moo_t*     moo, 
	moo_oow_t  size
);

/**
 * The moo_killheap() function destroys the heap pointed to by \a heap.
 */
void moo_killheap (
	moo_t*      moo, 
	moo_heap_t* heap
);

/**
 * The moo_allocheapmem() function allocates \a size bytes in the heap pointed
 * to by \a heap.
 *
 * \return memory pointer on success and #MOO_NULL on failure.
 */
void* moo_allocheapmem (
	moo_t*      moo,
	moo_heap_t* heap,
	moo_oow_t   size
);


/* ========================================================================= */
/* gc.c                                                                     */
/* ========================================================================= */
moo_oop_t moo_moveoop (
	moo_t*     moo,
	moo_oop_t  oop
);

/* ========================================================================= */
/* obj.c                                                                     */
/* ========================================================================= */
void* moo_allocbytes (
	moo_t*     moo,
	moo_oow_t  size
);

/**
 * The moo_allocoopobj() function allocates a raw object composed of \a size
 * pointer fields excluding the header.
 */
moo_oop_t moo_allocoopobj (
	moo_t*    moo,
	moo_oow_t size
);

#if defined(MOO_USE_OBJECT_TRAILER)
moo_oop_t moo_allocoopobjwithtrailer (
	moo_t*           moo,
	moo_oow_t        size,
	const moo_oob_t* tptr,
	moo_oow_t        tlen
);
#endif

moo_oop_t moo_alloccharobj (
	moo_t*            moo,
	const moo_ooch_t* ptr,
	moo_oow_t         len
);

moo_oop_t moo_allocbyteobj (
	moo_t*           moo,
	const moo_oob_t* ptr,
	moo_oow_t        len
);

moo_oop_t moo_allochalfwordobj (
	moo_t*            moo,
	const moo_oohw_t* ptr,
	moo_oow_t         len
);

moo_oop_t moo_allocwordobj (
	moo_t*           moo,
	const moo_oow_t* ptr,
	moo_oow_t        len
);

#if defined(MOO_USE_OBJECT_TRAILER)
moo_oop_t moo_instantiatewithtrailer (
	moo_t*           moo, 
	moo_oop_t        _class,
	moo_oow_t        vlen,
	const moo_oob_t* tptr,
	moo_oow_t        tlen
);
#endif

/* ========================================================================= */
/* sym.c                                                                     */
/* ========================================================================= */
moo_oop_t moo_findsymbol (
	moo_t*             moo,
	const moo_ooch_t*  ptr,
	moo_oow_t          len
);

/* ========================================================================= */
/* dic.c                                                                     */
/* ========================================================================= */
moo_oop_association_t moo_putatsysdic (
	moo_t*     moo,
	moo_oop_t  key,
	moo_oop_t  value
);

moo_oop_association_t moo_getatsysdic (
	moo_t*     moo,
	moo_oop_t  key
);

moo_oop_association_t moo_lookupsysdic (
	moo_t*            moo,
	const moo_oocs_t* name
);

moo_oop_association_t moo_putatdic (
	moo_t*        moo,
	moo_oop_set_t dic,
	moo_oop_t     key,
	moo_oop_t     value
);

moo_oop_association_t moo_getatdic (
	moo_t*        moo,
	moo_oop_set_t dic,
	moo_oop_t     key
);

moo_oop_association_t moo_lookupdic (
	moo_t*            moo,
	moo_oop_set_t     dic,
	const moo_oocs_t* name
);

int moo_deletedic (
	moo_t*            moo,
	moo_oop_set_t     dic,
	const moo_oocs_t* name
);

moo_oop_set_t moo_makedic (
	moo_t*    moo,
	moo_oop_t cls,
	moo_oow_t size
);

/* ========================================================================= */
/* proc.c                                                                    */
/* ========================================================================= */
moo_oop_process_t moo_makeproc (
	moo_t* moo
);


/* ========================================================================= */
/* bigint.c                                                                  */
/* ========================================================================= */
int moo_isint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_addints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_subints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_mulints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_divints (
	moo_t*     moo,
	moo_oop_t  x,
	moo_oop_t  y,
	int         modulo,
	moo_oop_t* rem
);

moo_oop_t moo_negateint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_bitatint (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitandints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitorints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitxorints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitinvint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_bitshiftint (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_eqints (
	moo_t* moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_neints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_gtints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_geints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_ltints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_leints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_strtoint (
	moo_t*            moo,
	const moo_ooch_t* str,
	moo_oow_t         len,
	int                radix
);

moo_oop_t moo_inttostr (
	moo_t*      moo,
	moo_oop_t   num,
	int          radix
);

/* ========================================================================= */
/* exec.c                                                                    */
/* ========================================================================= */

int moo_getpfnum (
	moo_t*            moo,
	const moo_ooch_t* ptr,
	moo_oow_t         len
);


/* ========================================================================= */
/* moo.c                                                                    */
/* ========================================================================= */

moo_mod_data_t* moo_openmod (
	moo_t*            moo,
	const moo_ooch_t* name,
	moo_oow_t         namelen
);

void moo_closemod (
	moo_t*            moo,
	moo_mod_data_t*   mdp
);

int moo_importmod (
	moo_t*            moo,
	moo_oop_t         _class,
	const moo_ooch_t* name,
	moo_oow_t         len
);

/*
 * The moo_querymod() function finds a primitive function in modules
 * with a full primitive identifier.
 */
moo_pfimpl_t moo_querymod (
	moo_t*            moo,
	const moo_ooch_t* pfid,
	moo_oow_t         pfidlen
);


/* TODO: remove the following debugging functions */
/* ========================================================================= */
/* debug.c                                                                   */
/* ========================================================================= */
void moo_dumpsymtab (moo_t* moo);
void moo_dumpdic (moo_t* moo, moo_oop_set_t dic, const moo_bch_t* title);


#if defined(__cplusplus)
}
#endif


#endif
