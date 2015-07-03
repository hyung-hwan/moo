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

#ifndef _STIX_PRV_H_
#define _STIX_PRV_H_

#include "stix.h"

/* you can define this to either 1 or 2 */
#define STIX_BCODE_LONG_PARAM_SIZE 2

/* this is useful for debugging. stix_gc() can be called 
 * while stix has not been fully initialized when this is defined*/
#define STIX_SUPPORT_GC_DURING_IGNITION

/* define this to generate XXXX_CTXTEMVAR instructions */
#define STIX_USE_CTXTEMPVAR

/* define this to use the MAKE_BLOCK instruction instead of
 * PUSH_CONTEXT, PUSH_INTLIT, PUSH_INTLIT, SEND_BLOCK_COPY */
#define STIX_USE_MAKE_BLOCK

/* define this to allow an pointer(OOP) object to have trailing bytes 
 * this is used to embed bytes codes into the back of a compile method
 * object instead of putting in in a separate byte array. */
#define STIX_USE_OBJECT_TRAILER

/* this is for gc debugging */
#define STIX_DEBUG_GC_001  

#include <stdio.h> /* TODO: delete these header inclusion lines */
#include <string.h>
#include <assert.h>

#define STIX_MEMSET(dst,src,size)  memset(dst,src,size)
#define STIX_MEMCPY(dst,src,size)  memcpy(dst,src,size)
#define STIX_MEMMOVE(dst,src,size) memmove(dst,src,size)
#define STIX_ASSERT(x)             assert(x)

#define STIX_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))

/* ========================================================================= */
/* CLASS SPEC ENCODING                                                       */
/* ========================================================================= */

/*
 * The spec field of a class object encodes the number of the fixed part
 * and the type of the indexed part. The fixed part is the number of
 * named instance variables. If the spec of a class is indexed, the object
 * of the class can be i nstantiated with the size of the indexed part.
 *
 * For example, on a platform where sizeof(stix_oow_t) is 4, 
 * the layout of the spec field of a class as an OOP value looks like this:
 * 
 *  31                           10 9   8 7 6 5 4 3  2           1 0 
 * |number of named instance variables|indexed-type|indexability|oop-tag|
 *
 * the number of named instance variables is stored in high 23 bits.
 * the indexed type takes up bit 3 to bit 8. And the indexability is
 * stored in bit 2. 
 *
 * The maximum number of named(fixed) instance variables for a class is:
 *     2 ^ ((BITS-IN-OOW - STIX_OOP_TAG_BITS) - STIX_OBJ_TYPE_BITS - 1) - 1
 *
 * STIX_OOP_TAG_BITS are decremented from the number of bits in OOW because
 * the spec field is always encoded as a small integer.
 *
 * The number of named instance variables can be greater than 0 if the
 * class spec is not indexed or if it's a pointer indexed class
 * (indexed_type == STIX_OBJ_TYPE_OOP)
 * 
 * indexed_type is one of the #stix_obj_type_t enumerators.
 */

/*
 * The STIX_CLASS_SPEC_MAKE() macro creates a class spec value.
 *  _class->spec = STIX_OOP_FROM_SMINT(STIX_CLASS_SPEC_MAKE(0, 1, STIX_OBJ_TYPE_CHAR));
 */
#define STIX_CLASS_SPEC_MAKE(named_instvar,is_indexed,indexed_type) ( \
	(((stix_oow_t)(named_instvar)) << (STIX_OBJ_FLAGS_TYPE_BITS + 1)) |  \
	(((stix_oow_t)(indexed_type)) << 1) | (((stix_oow_t)is_indexed) & 1) )

/* what is the number of named instance variables? 
 *  STIX_CLASS_SPEC_NAMED_INSTVAR(STIX_OOP_TO_SMINT(_class->spec))
 */
#define STIX_CLASS_SPEC_NAMED_INSTVAR(spec) \
	(((stix_oow_t)(spec)) >> (STIX_OBJ_FLAGS_TYPE_BITS + 1))

/* is it a user-indexable class? 
 * all objects can be indexed with basicAt:.
 * this indicates if an object can be instantiated with a dynamic size
 * (new: size) and and can be indexed with at:.
 */
#define STIX_CLASS_SPEC_IS_INDEXED(spec) (((stix_oow_t)(spec)) & 1)

/* if so, what is the indexing type? character? pointer? etc? */
#define STIX_CLASS_SPEC_INDEXED_TYPE(spec) \
	((((stix_oow_t)(spec)) >> 1) & STIX_LBMASK(stix_oow_t, STIX_OBJ_FLAGS_TYPE_BITS))

/* What is the maximum number of named instance variables?
 * This limit is set so because the number must be encoded into the spec field
 * of the class with limited number of bits assigned to the number of
 * named instance variables. the trailing -1 in the calculation of number of
 * bits is to consider the signness of a small-integer which is a typical
 * type of the spec field in the class object.
 */
#define STIX_MAX_NAMED_INSTVARS \
	STIX_BITS_MAX(stix_oow_t, STIX_OOW_BITS - STIX_OOP_TAG_BITS - (STIX_OBJ_FLAGS_TYPE_BITS + 1) - 1)

/* Given the number of named instance variables, what is the maximum number 
 * of indexed instance variables? The number of indexed instance variables
 * is not stored in the spec field of the class. It only affects the actual
 * size of an object(obj->_size) selectively combined with the number of 
 * named instance variables. So it's the maximum value of obj->_size minus
 * the number of named instance variables.
 */
#define STIX_MAX_INDEXED_INSTVARS(named_instvar) ((~(stix_oow_t)0) - named_instvar)


#define STIX_CLASS_SELFSPEC_MAKE(class_var,classinst_var) \
	(((stix_oow_t)class_var) << ((STIX_OOW_BITS - STIX_OOP_TAG_BITS) / 2)) | ((stix_oow_t)classinst_var)

#define STIX_CLASS_SELFSPEC_CLASSVAR(spec) ((stix_oow_t)spec >> ((STIX_OOW_BITS - STIX_OOP_TAG_BITS) / 2))
#define STIX_CLASS_SELFSPEC_CLASSINSTVAR(spec) (((stix_oow_t)spec) & STIX_LBMASK(stix_oow_t, (STIX_OOW_BITS - STIX_OOP_TAG_BITS) / 2))

/*
 * yet another -1 in the calculation of the bit numbers for signed nature of
 * a small-integer
 */
#define STIX_MAX_CLASSVARS      STIX_BITS_MAX(stix_oow_t, (STIX_OOW_BITS - STIX_OOP_TAG_BITS - 1) / 2)
#define STIX_MAX_CLASSINSTVARS  STIX_BITS_MAX(stix_oow_t, (STIX_OOW_BITS - STIX_OOP_TAG_BITS - 1) / 2)

#if defined(STIX_INCLUDE_COMPILER)

/* ========================================================================= */
/* SOURCE CODE I/O FOR COMPILER                                              */
/* ========================================================================= */

enum stix_iocmd_t
{
	STIX_IO_OPEN,
	STIX_IO_CLOSE,
	STIX_IO_READ
};
typedef enum stix_iocmd_t stix_iocmd_t;

struct stix_ioloc_t
{
	unsigned long     line; /**< line */
	unsigned long     colm; /**< column */
	const stix_uch_t* file; /**< file specified in #include */
};
typedef struct stix_ioloc_t stix_ioloc_t;

struct stix_iolxc_t
{
	stix_uci_t    c; /**< character */
	stix_ioloc_t  l; /**< location */
};
typedef struct stix_iolxc_t stix_iolxc_t;

/*
enum stix_ioarg_flag_t
{
	STIX_IO_INCLUDED = (1 << 0)
};
typedef enum stix_ioarg_flag_t stix_ioarg_flag_t; */

typedef struct stix_ioarg_t stix_ioarg_t;
struct stix_ioarg_t
{
	/** 
	 * [IN] I/O object name.
	 * It is #STIX_NULL for the main stream and points to a non-NULL string
	 * for an included stream.
	 */
	const stix_uch_t* name;   

	/** 
	 * [OUT] I/O handle set by a handler. 
	 * The source stream handler can set this field when it opens a stream.
	 * All subsequent operations on the stream see this field as set
	 * during opening.
	 */
	void* handle;

	/**
	 * [OUT] place data here 
	 */
	stix_uch_t buf[1024];

	/**
	 * [IN] points to the data of the includer. It is #STIX_NULL for the
	 * main stream.
	 */
	stix_ioarg_t* includer;

	/*-----------------------------------------------------------------*/
	/*----------- from here down, internal use only -------------------*/
	struct
	{
		int pos, len, state;
	} b;

	unsigned long line;
	unsigned long colm;

	stix_iolxc_t lxc;
	/*-----------------------------------------------------------------*/
};

typedef stix_ssize_t (*stix_ioimpl_t) (
	stix_t*       stix,
	stix_iocmd_t  cmd,
	stix_ioarg_t* arg
);

struct stix_iotok_t
{
	enum
	{
		STIX_IOTOK_EOF,
		STIX_IOTOK_CHARLIT,
		STIX_IOTOK_STRLIT,
		STIX_IOTOK_SYMLIT,
		STIX_IOTOK_NUMLIT,
		STIX_IOTOK_RADNUMLIT,
		STIX_IOTOK_NIL,
		STIX_IOTOK_SELF,
		STIX_IOTOK_SUPER,
		STIX_IOTOK_TRUE,
		STIX_IOTOK_FALSE,
		STIX_IOTOK_THIS_CONTEXT,
		STIX_IOTOK_IDENT,
		STIX_IOTOK_BINSEL,
		STIX_IOTOK_KEYWORD,
		STIX_IOTOK_PRIMITIVE,
		STIX_IOTOK_ASSIGN,
		STIX_IOTOK_COLON,
		STIX_IOTOK_RETURN,
		STIX_IOTOK_LBRACE,
		STIX_IOTOK_RBRACE,
		STIX_IOTOK_LBRACK,
		STIX_IOTOK_RBRACK,
		STIX_IOTOK_LPAREN,
		STIX_IOTOK_RPAREN,
		STIX_IOTOK_APAREN, /* #( */
		STIX_IOTOK_BPAREN, /* #[ */
		STIX_IOTOK_PERIOD,
		STIX_IOTOK_SEMICOLON
	} type;

	stix_ucs_t name;
	stix_size_t name_capa;

	stix_ioloc_t loc;
};
typedef struct stix_iotok_t stix_iotok_t;

enum stix_synerrnum_t
{
	STIX_SYNERR_NOERR,
	STIX_SYNERR_ILCHR,         /* illegal character */
	STIX_SYNERR_CMTNC,         /* comment not closed */
	STIX_SYNERR_STRNC,         /* string not closed */
	STIX_SYNERR_CLTNT,         /* character literal not terminated */
	STIX_SYNERR_HLTNT,         /* hased literal not terminated */
	STIX_SYNERR_CHARLIT,       /* wrong character literal */
	STIX_SYNERR_COLON,         /* : expected */
	STIX_SYNERR_STRING,        /* string expected */
	STIX_SYNERR_RADIX,         /* invalid radix */
	STIX_SYNERR_RADNUMLIT,     /* invalid numeric literal with radix */
	STIX_SYNERR_BYTERANGE,     /* byte too small or too large */
	STIX_SYNERR_LBRACE,        /* { expected */
	STIX_SYNERR_RBRACE,        /* } expected */
	STIX_SYNERR_LPAREN,        /* ( expected */
	STIX_SYNERR_RPAREN,        /* ) expected */
	STIX_SYNERR_RBRACK,        /* ] expected */
	STIX_SYNERR_PERIOD,        /* . expected */
	STIX_SYNERR_VBAR,          /* | expected */
	STIX_SYNERR_GT,            /* > expected */
	STIX_SYNERR_IDENT,         /* identifier expected */
	STIX_SYNERR_INTEGER,       /* integer expected */
	STIX_SYNERR_PRIMITIVE,     /* primitive: expected */
	STIX_SYNERR_DIRECTIVE,     /* wrong directive */
	STIX_SYNERR_CLASSUNDEF,    /* undefined class */
	STIX_SYNERR_CLASSDUP,      /* duplicate class */
	STIX_SYNERR_CLASSCONTRA,   /* contradictory class */
	STIX_SYNERR_DCLBANNED,     /* #dcl not allowed */
	STIX_SYNERR_MTHNAME,       /* wrong method name */
	STIX_SYNERR_MTHNAMEDUP,    /* duplicate method name */
	STIX_SYNERR_ARGNAMEDUP,    /* duplicate argument name */
	STIX_SYNERR_TMPRNAMEDUP,   /* duplicate temporary variable name */
	STIX_SYNERR_VARNAMEDUP,    /* duplicate variable name */
	STIX_SYNERR_BLKARGNAMEDUP, /* duplicate block argument name */
	STIX_SYNERR_VARARG,        /* cannot assign to argument */
	STIX_SYNERR_VARUNDCL,      /* undeclared variable */
	STIX_SYNERR_VARUNUSE,      /* unsuable variable in compiled code */
	STIX_SYNERR_VARINACC,      /* inaccessible variable - e.g. accessing an instance variable from a class method is not allowed. */
	STIX_SYNERR_PRIMARY,       /* wrong expression primary */
	STIX_SYNERR_TMPRFLOOD,     /* too many temporaries */
	STIX_SYNERR_ARGFLOOD,      /* too many arguments */
	STIX_SYNERR_BLKTMPRFLOOD,  /* too many block temporaries */
	STIX_SYNERR_BLKARGFLOOD,   /* too many block arguments */
	STIX_SYNERR_BLKFLOOD,      /* too large block */
	STIX_SYNERR_PRIMNO,        /* wrong primitive number */
	STIX_SYNERR_INCLUDE        /* #include error */
};
typedef enum stix_synerrnum_t stix_synerrnum_t;

typedef struct stix_iolink_t stix_iolink_t;
struct stix_iolink_t
{
	stix_iolink_t* link;
};

struct stix_synerr_t
{
	stix_synerrnum_t num;
	stix_ioloc_t     loc;
	stix_ucs_t       tgt;
};
typedef struct stix_synerr_t stix_synerr_t;


struct stix_code_t
{
	stix_uint8_t* ptr;
	stix_size_t   len;
};
typedef struct stix_code_t stix_code_t;

struct stix_compiler_t
{
	/* input handler */
	stix_ioimpl_t impl;

	/* information about the last meaningful character read.
	 * this is a copy of curinp->lxc if no ungetting is performed.
	 * if there is something in the unget buffer, this is overwritten
	 * by a value from the buffer when the request to read a character
	 * is served */
	stix_iolxc_t  lxc;

	/* unget buffer */
	stix_iolxc_t  ungot[10];
	int           nungots;

	/* static input data buffer */
	stix_ioarg_t  arg;    

	/* pointer to the current input data. initially, it points to &arg */
	stix_ioarg_t* curinp;

	/* the last token read */
	stix_iotok_t  tok;
	stix_iolink_t* io_names;
	int in_array;

	stix_synerr_t synerr;

	/* temporary space to handle an illegal character */
	stix_uch_t ilchr;
	stix_ucs_t ilchr_ucs;

	/* information about a class being compiled */
	struct
	{
		int flags;
		int indexed_type;

		stix_oop_class_t self_oop;
		stix_oop_t super_oop; /* this may be nil. so the type is stix_oop_t */
		stix_oop_set_t mthdic_oop[2];

		stix_ucs_t name;
		stix_size_t name_capa;
		stix_ioloc_t name_loc;

		stix_ucs_t supername;
		stix_size_t supername_capa;
		stix_ioloc_t supername_loc;

		/* instance variable, class variable, class instance variable */
		stix_ucs_t vars[3]; 
		stix_size_t vars_capa[3];

		/* var_count, unlike vars above, includes superclass counts as well.
		 * var_count[0] - number of named instance variables
		 * var_count[1] - number of class variables
		 * var_count[2] - number of class instance variables */
		stix_size_t var_count[3];
	} cls;

	/* information about a function being comipled */
	struct
	{
		int type;

		/* method source text */
		stix_ucs_t text;
		stix_size_t text_capa;

		/* buffer to store identifier names to be assigned */
		stix_ucs_t assignees;
		stix_size_t assignees_capa;

		/* buffer to store binary selectors being worked on */
		stix_ucs_t binsels;
		stix_size_t binsels_capa;

		/* buffer to store keyword selectors being worked on */
		stix_ucs_t kwsels;
		stix_size_t kwsels_capa;

		/* method name */
		stix_ucs_t name;
		stix_size_t name_capa;
		stix_ioloc_t name_loc;

		/* single string containing a space separated list of temporaries */
		stix_ucs_t tmprs; 
		stix_size_t tmprs_capa;
		stix_size_t tmpr_count; /* total number of temporaries including arguments */
		stix_size_t tmpr_nargs;

		/* literals */
		stix_oop_t* literals;
		stix_size_t literal_count;
		stix_size_t literal_capa;

		/* byte array elements */
		stix_byte_t* balit;
		stix_size_t balit_count;
		stix_size_t balit_capa;

		/* array elements */
		stix_oop_t* arlit;
		stix_size_t arlit_count;
		stix_size_t arlit_capa;

		/* primitive number */
		stix_ooi_t prim_no; 

		/* block depth */
		stix_size_t blk_depth;
		stix_size_t* blk_tmprcnt;
		stix_size_t blk_tmprcnt_capa;

		/* byte code */
		stix_code_t code;
		stix_size_t code_capa;
	} mth; 
};

#endif

#if defined(STIX_USE_OBJECT_TRAILER)
	/* let it point to the trailer of the method */
#	define SET_ACTIVE_METHOD_CODE(stix) ((stix)->active_code = (stix_byte_t*)&(stix)->active_method->slot[STIX_OBJ_GET_SIZE((stix)->active_method) + 1 - STIX_METHOD_NAMED_INSTVARS])
#else
	/* let it point to the payload of the code byte array */
#	define SET_ACTIVE_METHOD_CODE(stix) ((stix)->active_code = (stix)->active_method->code->slot)
#endif

#if defined(STIX_BCODE_LONG_PARAM_SIZE) && (STIX_BCODE_LONG_PARAM_SIZE == 1)
#	define MAX_CODE_INDEX               (0xFFu)
#	define MAX_CODE_NTMPRS              (0xFFu)
#	define MAX_CODE_NARGS               (0xFFu)
#	define MAX_CODE_NBLKARGS            (0xFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFu)
#	define MAX_CODE_JUMP                (0xFFu)
#	define MAX_CODE_PARAM               (0xFFu)
#elif defined(STIX_BCODE_LONG_PARAM_SIZE) && (STIX_BCODE_LONG_PARAM_SIZE == 2)
#	define MAX_CODE_INDEX               (0xFFFFu)
#	define MAX_CODE_NTMPRS              (0xFFFFu)
#	define MAX_CODE_NARGS               (0xFFFFu)
#	define MAX_CODE_NBLKARGS            (0xFFFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFFFu)
#	define MAX_CODE_JUMP                (0xFFFFu)
#	define MAX_CODE_PARAM               (0xFFFFu)
#else
#	error Unsupported STIX_BCODE_LONG_PARAM_SIZE
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
76-79    0100 11XX JUMP_IF_TRUE                               204  1100 1100 XXXXXXXX JUMP_IF_TRUE_X
80-83    0101 00XX JUMP_IF_FALSE                              208  1101 0000 XXXXXXXX JUMP_IF_FALSE_X

84-87    0101 01XX UNUSED

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

enum stix_bcode_t
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

	BCODE_JUMP_IF_TRUE_0           = 0x4C,
	BCODE_JUMP_IF_TRUE_1           = 0x4D,
	BCODE_JUMP_IF_TRUE_2           = 0x4E,
	BCODE_JUMP_IF_TRUE_3           = 0x4F,

	BCODE_JUMP_IF_FALSE_0          = 0x50, /* 80 */
	BCODE_JUMP_IF_FALSE_1          = 0x51, /* 81 */
	BCODE_JUMP_IF_FALSE_2          = 0x52, /* 82 */
	BCODE_JUMP_IF_FALSE_3          = 0x53, /* 83 */

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

	BCODE_STORE_INTO_INSTVAR_X     = 0x80, /* 128 */
	BCODE_POP_INTO_INSTVAR_X       = 0x88, /* 136 */
	BCODE_PUSH_INSTVAR_X           = 0x90, /* 144 */

	BCODE_PUSH_TEMPVAR_X           = 0x98, /* 152 */
	BCODE_STORE_INTO_TEMPVAR_X     = 0xA0, /* 160 */
	BCODE_POP_INTO_TEMPVAR_X       = 0xA8, /* 168 */

	BCODE_PUSH_LITERAL_X           = 0xB0, /* 176 */

	BCODE_STORE_INTO_OBJECT_X      = 0xB8, /* 184 */
	BCODE_POP_INTO_OBJECT_X        = 0xBC, /* 188 */
	BCODE_PUSH_OBJECT_X            = 0xC0, /* 192 */

	BCODE_JUMP_FORWARD_X           = 0xC4, /* 196 */
	BCODE_JUMP_BACKWARD_X          = 0xC8, /* 200 */
	BCODE_JUMP_IF_TRUE_X           = 0xCC, /* 204 */
	BCODE_JUMP_IF_FALSE_X          = 0xD0, /* 208 */


	BCODE_STORE_INTO_CTXTEMPVAR_X  = 0xD8, /* 216 */
	BCODE_POP_INTO_CTXTEMPVAR_X    = 0xDC, /* 220 */
	BCODE_PUSH_CTXTEMPVAR_X        = 0xE0, /* 224 */

	BCODE_PUSH_OBJVAR_X            = 0xE4, /* 228 */
	BCODE_STORE_INTO_OBJVAR_X      = 0xE8, /* 232 */
	BCODE_POP_INTO_OBJVAR_X        = 0xEC, /* 236 */

	BCODE_SEND_MESSAGE_X           = 0xF0, /* 240 */
	BCODE_SEND_MESSAGE_TO_SUPER_X  = 0xF4, /* 244 */

	/* -------------------------------------- */

	BCODE_JUMP2_FORWARD            = 0xC5, /* 197 */
	BCODE_JUMP2_BACKWARD           = 0xC9, /* 201 */

	BCODE_PUSH_RECEIVER            = 0x81, /* 129 */
	BCODE_PUSH_NIL                 = 0x82, /* 130 */
	BCODE_PUSH_TRUE                = 0x83, /* 131 */
	BCODE_PUSH_FALSE               = 0x84, /* 132 */
	BCODE_PUSH_CONTEXT             = 0x85, /* 133 */
	BCODE_PUSH_NEGONE              = 0x86, /* 134 */
	BCODE_PUSH_ZERO                = 0x87, /* 135 */
	BCODE_PUSH_ONE                 = 0x89, /* 137 */
	BCODE_PUSH_TWO                 = 0x8A, /* 138 */

	BCODE_PUSH_INTLIT              = 0xB1, /* 177 */
	BCODE_PUSH_NEGINTLIT           = 0xB2, /* 178 */

	/* UNUSED 0xE8 - 0xF7 */

	BCODE_DUP_STACKTOP             = 0xF8,
	BCODE_POP_STACKTOP             = 0xF9,
	BCODE_RETURN_STACKTOP          = 0xFA, /* ^something */
	BCODE_RETURN_RECEIVER          = 0xFB, /* ^self */
	BCODE_RETURN_FROM_BLOCK        = 0xFC, /* return the stack top from a block */
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
 * The stix_makeheap() function creates a new heap of the \a size bytes.
 *
 * \return heap pointer on success and #STIX_NULL on failure.
 */
stix_heap_t* stix_makeheap (
	stix_t*     stix, 
	stix_size_t size
);

/**
 * The stix_killheap() function destroys the heap pointed to by \a heap.
 */
void stix_killheap (
	stix_t*      stix, 
	stix_heap_t* heap
);

/**
 * The stix_allocheapmem() function allocates \a size bytes in the heap pointed
 * to by \a heap.
 *
 * \return memory pointer on success and #STIX_NULL on failure.
 */
void* stix_allocheapmem (
	stix_t*      stix,
	stix_heap_t* heap,
	stix_size_t  size
);


/* ========================================================================= */
/* stix.c                                                                    */
/* ========================================================================= */
stix_size_t stix_hashbytes (
	const stix_byte_t* ptr,
	stix_size_t        len
);

stix_size_t stix_hashchars (
	const stix_uch_t*  ptr,
	stix_size_t         len
);

int stix_equalchars (
	const stix_uch_t*  str1,
	const stix_uch_t*  str2,
	stix_size_t         len
);

void stix_copychars (
	stix_uch_t*       dst,
	const stix_uch_t* src,
	stix_size_t len
);

/* ========================================================================= */
/* gc.c                                                                     */
/* ========================================================================= */
stix_oop_t stix_moveoop (
	stix_t*     stix,
	stix_oop_t  oop
);

/* ========================================================================= */
/* obj.c                                                                     */
/* ========================================================================= */
void* stix_allocbytes (
	stix_t*     stix,
	stix_size_t size
);

/**
 * The stix_allocoopobj() function allocates a raw object composed of \a size
 * pointer fields excluding the header.
 */
stix_oop_t stix_allocoopobj (
	stix_t*    stix,
	stix_oow_t size
);

#if defined(STIX_USE_OBJECT_TRAILER)
stix_oop_t stix_allocoopobjwithtrailer (
	stix_t*            stix,
	stix_oow_t         size,
	const stix_byte_t* tptr,
	stix_oow_t         tlen
);
#endif

stix_oop_t stix_alloccharobj (
	stix_t*            stix,
	const stix_uch_t*  ptr,
	stix_oow_t         len
);

stix_oop_t stix_allocbyteobj (
	stix_t*            stix,
	const stix_byte_t* ptr,
	stix_oow_t         len
);

stix_oop_t stix_allocwordobj (
	stix_t*            stix,
	const stix_oow_t*  ptr,
	stix_oow_t         len
);

#if defined(STIX_USE_OBJECT_TRAILER)
stix_oop_t stix_instantiatewithtrailer (
	stix_t*            stix, 
	stix_oop_t         _class,
	stix_oow_t         vlen,
	const stix_byte_t* tptr,
	stix_oow_t tlen
);
#endif

/* ========================================================================= */
/* sym.c                                                                     */
/* ========================================================================= */
stix_oop_t stix_makesymbol (
	stix_t*            stix,
	const stix_uch_t*  ptr,
	stix_oow_t         len
);

stix_oop_t stix_findsymbol (
	stix_t*            stix,
	const stix_uch_t*  ptr,
	stix_oow_t         len
);

stix_oop_t stix_makestring (
	stix_t*            stix, 
	const stix_uch_t*  ptr, 
	stix_oow_t         len
);

/* ========================================================================= */
/* dic.c                                                                     */
/* ========================================================================= */
stix_oop_association_t stix_putatsysdic (
	stix_t*     stix,
	stix_oop_t  key,
	stix_oop_t  value
);

stix_oop_association_t stix_getatsysdic (
	stix_t*     stix,
	stix_oop_t  key
);

stix_oop_association_t stix_lookupsysdic (
	stix_t*           stix,
	const stix_ucs_t* name
);

stix_oop_association_t stix_putatdic (
	stix_t*        stix,
	stix_oop_set_t dic,
	stix_oop_t     key,
	stix_oop_t     value
);

stix_oop_association_t stix_getatdic (
	stix_t*        stix,
	stix_oop_set_t dic,
	stix_oop_t     key
);

stix_oop_association_t stix_lookupdic (
	stix_t*           stix,
	stix_oop_set_t    dic,
	const stix_ucs_t* name
);

stix_oop_set_t stix_makedic (
	stix_t*    stix,
	stix_oop_t cls,
	stix_oow_t size
);

/* ========================================================================= */
/* utf8.c                                                                    */
/* ========================================================================= */
stix_size_t stix_uctoutf8 (
	stix_uch_t    uc,
	stix_bch_t*   utf8,
	stix_size_t   size
);

stix_size_t stix_utf8touc (
	const stix_bch_t* utf8,
	stix_size_t       size,
	stix_uch_t*       uc
);

int stix_ucstoutf8 (
	const stix_uch_t*    ucs,
	stix_size_t*         ucslen,
	stix_bch_t*          bcs,
	stix_size_t*         bcslen
);

/**
 * The stix_utf8toucs() function converts a UTF8 string to a uncide string.
 *
 * It never returns -2 if \a ucs is #STIX_NULL.
 *
 * \code
 *  const stix_bch_t* bcs = "test string";
 *  stix_uch_t ucs[100];
 *  qse_size_t ucslen = STIX_COUNTOF(buf), n;
 *  qse_size_t bcslen = 11;
 *  int n;
 *  n = qse_bcstoucs (bcs, &bcslen, ucs, &ucslen);
 *  if (n <= -1) { invalid/incomplenete sequence or buffer to small }
 * \endcode
 *
 * For a null-terminated string, you can specify ~(stix_size_t)0 in
 * \a bcslen. The destination buffer \a ucs also must be large enough to
 * store a terminating null. Otherwise, -2 is returned.
 * 
 * The resulting \a ucslen can still be greater than 0 even if the return
 * value is negative. The value indiates the number of characters converted
 * before the error has occurred.
 *
 * \return 0 on success.
 *         -1 if \a bcs contains an illegal character.
 *         -2 if the wide-character string buffer is too small.
 *         -3 if \a bcs is not a complete sequence.
 */
int stix_utf8toucs (
	const stix_bch_t*   bcs,
	stix_size_t*        bcslen,
	stix_uch_t*         ucs,
	stix_size_t*        ucslen
);

/* ========================================================================= */
/* comp.c                                                                    */
/* ========================================================================= */
int stix_compile (
	stix_t*       stix,
	stix_ioimpl_t io
);

void stix_getsynerr (
	stix_t* stix,
	stix_synerr_t* synerr
);


/* TODO: remove debugging functions */
/* ========================================================================= */
/* debug.c                                                                   */
/* ========================================================================= */
void dump_symbol_table (stix_t* stix);
void dump_dictionary (stix_t* stix, stix_oop_set_t dic, const char* title);
void print_ucs (const stix_ucs_t* name);
void print_object (stix_t* stix, stix_oop_t oop);
void dump_object (stix_t* stix, stix_oop_t oop, const char* title);

#if defined(__cplusplus)
}
#endif


#endif
