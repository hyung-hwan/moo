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

/* this is useful for debugging. stix_gc() can be called 
 * while stix has not been fully initialized when this is defined*/
#define STIX_SUPPORT_GC_DURING_IGNITION
#define STIX_DEBUG_GC_1


#include <stdio.h> /* TODO: delete these header inclusion lines */
#include <string.h>
#include <assert.h>

#define STIX_MEMSET(dst,src,size) memset(dst,src,size)
#define STIX_MEMCPY(dst,src,size) memcpy(dst,src,size)
#define STIX_ASSERT(x)            assert(x)

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
 *   2 ^ ((BITS-IN-OOW - STIX_OOP_TAG_BITS) - STIX_OBJ_TYPE_BITS - 1) - 1 
 * This limit is set because the number must be encoded into the spec field
 * of the class with limited number of bits assigned to the number of
 * named instance variables.
 */
#define STIX_MAX_NAMED_INSTVARS \
	STIX_BITS_MAX(stix_oow_t, STIX_OOW_BITS - STIX_OOP_TAG_BITS - (STIX_OBJ_FLAGS_TYPE_BITS + 1))

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

#define STIX_MAX_CLASSVARS      STIX_BITS_MAX(stix_oow_t, (STIX_OOW_BITS - STIX_OOP_TAG_BITS) / 2)
#define STIX_MAX_CLASSINSTVARS  STIX_BITS_MAX(stix_oow_t, (STIX_OOW_BITS - STIX_OOP_TAG_BITS) / 2)

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

enum stix_ioarg_flag_t
{
	STIX_IO_INCLUDED = (1 << 0)
};
typedef enum stix_ioarg_flag_t stix_ioarg_flag_t;

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
		int pos, len;
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
		STIX_IOTOK_CHRLIT,
		STIX_IOTOK_STRLIT,
		STIX_IOTOK_SYMLIT,
		STIX_IOTOK_NUMLIT,
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
	STIX_SYNERR_ILCHR,       /* illegal character */
	STIX_SYNERR_CMTNC,       /* comment not closed */
	STIX_SYNERR_STRNC,       /* string not closed */
	STIX_SYNERR_CLTNT,       /* character literal not terminated */
	STIX_SYNERR_HLTNT,       /* hased literal not terminated */
	STIX_SYNERR_CLNMS,       /* colon missing */
	STIX_SYNERR_STRING,      /* string expected */
	STIX_SYNERR_LBRACE,      /* { expected */
	STIX_SYNERR_RBRACE,      /* } expected */
	STIX_SYNERR_LPAREN,      /* ( expected */
	STIX_SYNERR_RPAREN,      /* ) expected */
	STIX_SYNERR_PERIOD,      /* . expected */
	STIX_SYNERR_VBAR,        /* | expected */
	STIX_SYNERR_GT,          /* > expected */
	STIX_SYNERR_IDENT,       /* identifier expected */
	STIX_SYNERR_INTEGER,     /* integer expected */
	STIX_SYNERR_PRIMITIVE,   /* primitive: expected */
	STIX_SYNERR_DIRECTIVE,   /* wrong directive */
	STIX_SYNERR_CLASSMOD,    /* wrong class modifier */
	STIX_SYNERR_CLASSMODDUP, /* duplicate class modifier */
	STIX_SYNERR_CLASSUNDEF,  /* undefined class */
	STIX_SYNERR_CLASSDUP,    /* duplicate class */
	STIX_SYNERR_DCLBANNED,   /* #dcl not allowed */
	STIX_SYNERR_FUNNAME,     /* wrong function name */
	STIX_SYNERR_ARGNAMEDUP,  /* duplicate argument name */
	STIX_SYNERR_TMPRNAMEDUP, /* duplicate temporary variable name */
	STIX_SYNERR_VARNAMEDUP   /* duplicate variable name */
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


/*
 * The Smalltalk-80 Bytecodes
 * Range     Bits               Function
 * -------------------------------------------------------------
 * 0-15      0000iiii           Push Receiver Variable #iiii
 * 16-31     0001iiii           Push Temporary Location #iiii
 * 32-63     001iiiii           Push Literal Constant #iiiii
 * 64-95     010iiiii           Push Literal Variable #iiiii
 * 96-103    01100iii           Pop and Store Receiver Variable #iii
 * 104-111   01101iii           Pop and Store Temporary Location #iii
 * 112-119   01110iii           Push (receiver, _true, _false, _nil, -1, 0, 1, 2) [iii]
 * 120-123   011110ii           Return (receiver, _true, _false, _nil) [ii] From Message
 * 124-125   0111110i           Return Stack Top From (Message, Block) [i]
 * 126-127   0111111i           unused
 * 128       10000000 jjkkkkkk  Push (Receiver Variable, Temporary Location, Literal Constant, Literal Variable) [jj] #kkkkkk
 * 129       10000001 jjkkkkkk  Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
 * 130       10000010 jjkkkkkk  Pop and Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
 * 131       10000011 jjjkkkkk  Send Literal Selector #kkkkk With jjj Arguments
 * 132       10000100 jjjjjjjj kkkkkkkk     Send Literal Selector #kkkkkkkk With jjjjjjjj Arguments
 * 133       10000101 jjjkkkkk  Send Literal Selector #kkkkk To Superclass With jjj Arguments
 * 134       10000110 jjjjjjjj kkkkkkkk     Send Literal Selector #kkkkkkkk To Superclass With jjjjjjjj Arguments
 * 135       10000111           Pop Stack Top
 * 136       10001000           Duplicate Stack Top
 * 137       10001001           Push Active Context
 * 138-143   unused
 * 144-151   10010iii           Jump iii + 1 (i.e., 1 through 8)
 * 152-159   10011iii           Pop and Jump On False iii +1 (i.e., 1 through 8)
 * 160-167   10100iii jjjjjjjj  Jump(iii - 4) *256+jjjjjjjj
 * 168-171   101010ii jjjjjjjj  Pop and Jump On True ii *256+jjjjjjjj
 * 172-175   101011ii jjjjjjjj  Pop and Jump On False ii *256+jjjjjjjj
 * 176-191   1011iiii           Send Arithmetic Message #iiii
 * 192-207   1100iiii           Send Special Message #iiii
 * 208-223   1101iiii           Send Literal Selector #iiii With No Arguments
 * 224-239   1110iiii           Send Literal Selector #iiii With 1 Argument
 * 240-255   1111iiii           Send Literal Selector #iiii With 2 Arguments  
 */

/**
 * The stix_code_t type defines byte-code enumerators.
 */
enum stix_code_id_t
{
	/* 0-15 */
	STIX_PUSH_RECEIVER_VARIABLE            = 0x00,

	/* 16-31 */
	STIX_PUSH_TEMPORARY_LOCATION           = 0x10,

	/* 32-63 */
	STIX_PUSH_LITERAL_CONSTANT             = 0x20,

	/* 64-95 */
	STIX_PUSH_LITERAL_VARIABLE             = 0x40,

	/* 96-103 */
	STIX_POP_STORE_RECEIVER_VARIABLE       = 0x60,

	/* 104-111 */
	STIX_POP_STORE_TEMPORARY_LOCATION      = 0x68,

	/*  112-119 */
	STIX_PUSH_RECEIVER                     = 0x70,
	STIX_PUSH_TRUE                         = 0x71,
	STIX_PUSH_FALSE                        = 0x72,
	STIX_PUSH_NIL                          = 0x73,
	STIX_PUSH_MINUSONE                     = 0x74,
	STIX_PUSH_ZERO                         = 0x75,
	STIX_PUSH_ONE                          = 0x76,
	STIX_PUSH_TWO                          = 0x77,

	/* 120-123 */
	STIX_RETURN_RECEIVER                   = 0x78,
	STIX_RETURN_TRUE                       = 0x79,
	STIX_RETURN_FALSE                      = 0x7A,
	STIX_RETURN_NIL                        = 0x7B,

	/* 124-125 */
	STIX_RETURN_FROM_MESSAGE               = 0x7C,
	STIX_RETURN_FROM_BLOCK                 = 0x7D,

	/* 128 */ 
	STIX_PUSH_EXTENDED                     = 0x80,

	/* 129 */ 
	STIX_STORE_EXTENDED                    = 0x81,

	/* 130 */ 
	STIX_POP_STORE_EXTENDED                = 0x82,

	/* 131 */
	STIX_SEND_TO_SELF                      = 0x83,

	/* 132 */
	STIX_SEND_TO_SUPER                     = 0x84,

	/* 133 */
	STIX_SEND_TO_SELF_EXTENDED             = 0x85,

	/* 134 */
	STIX_SEND_TO_SUPER_EXTENDED            = 0x86,

	/* 135 */
	STIX_POP_STACK_TOP                     = 0x87,

	/* 136 */
	STIX_DUP_STACK_TOP                     = 0x88,

	/* 137 */
	STIX_PUSH_ACTIVE_CONTEXT               = 0x89,

	/* 138 */
	STIX_DO_PRIMITIVE                      = 0x8A,

	/* 144-151 */
	STIX_JUMP                              = 0x90,

	/* 152-159 */
	STIX_POP_JUMP_ON_FALSE                 = 0x98,

	/* 160-167 */
	STIX_JUMP_EXTENDED                     = 0xA0,

	/* 168-171 */
	STIX_POP_JUMP_ON_TRUE_EXTENDED         = 0xA8,

	/* 172-175 */
	STIX_POP_JUMP_ON_FALSE_EXTENDED        = 0xAC,

#if 0
	STIX_PUSH_RECEIVER_VARIABLE_EXTENDED   = 0x60
	STIX_PUSH_TEMPORARY_LOCATION_EXTENDED  = 0x61
	STIX_PUSH_LITERAL_CONSTANT_EXTENDED    = 0x62
	STIX_PUSH_LITERAL_VARIABLE_EXTENDED    = 0x63
	STIX_STORE_RECEIVER_VARIABLE_EXTENDED  = 0x64
	STIX_STORE_TEMPORARY_LOCATION_EXTENDED = 0x65

	STIX_POP_STACK_TOP                     = 0x67
	STIX_DUPLICATE_STACK_TOP               = 0x68
	STIX_PUSH_ACTIVE_CONTEXT               = 0x69
	STIX_PUSH_NIL                          = 0x6A
	STIX_PUSH_TRUE                         = 0x6B
	STIX_PUSH_FALSE                        = 0x6C
	STIX_PUSH_RECEIVER                     = 0x6D

	STIX_SEND_TO_SELF                      = 0x70
	STIX_SEND_TO_SUPER                     = 0x71
	STIX_SEND_TO_SELF_EXTENDED             = 0x72
	STIX_SEND_TO_SUPER_EXTENDED            = 0x73

	STIX_RETURN_RECEIVER                   = 0x78
	STIX_RETURN_TRUE                       = 0x79
	STIX_RETURN_FALSE                      = 0x7A
	STIX_RETURN_NIL                        = 0x7B
	STIX_RETURN_FROM_MESSAGE               = 0x7C
	STIX_RETURN_FROM_BLOCK                 = 0x7D

	STIX_DO_PRIMITIVE                      = 0xF0
#endif
};

typedef enum stix_code_id_t stix_code_id_t;


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

		stix_ucs_t name;
		stix_size_t name_capa;

		stix_ucs_t supername;
		stix_size_t supername_capa;

		/* instance variable, class variable, class instance variable */
		stix_ucs_t vars[3]; 
		stix_size_t vars_capa[3];

		/* var_count, unlike vars above, includes superclass counts as well.*/
		stix_size_t var_count[3];
	} _class;

	/* information about a function being comipled */
	struct
	{
		int flags;

		stix_ucs_t name;
		stix_size_t name_capa;

		/* single string containing a space separated list of temporaries */
		stix_ucs_t tmprs; 
		stix_size_t tmprs_capa;

		stix_size_t tmpr_count; /* total number of temporaries including arguments */
		stix_size_t tmpr_nargs;

		/* literals */
		int literal_count;

		stix_oow_t prim_no; /* primitive number */

		stix_code_t code;
		stix_size_t code_capa;
	} fun; 
};

#endif


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

stix_oop_t stix_alloccharobj (
	stix_t*            stix,
	const stix_uch_t* ptr,
	stix_oow_t         len
);

stix_oop_t stix_allocuint8obj (
	stix_t*             stix,
	const stix_uint8_t* ptr,
	stix_oow_t          len
);

stix_oop_t stix_allocuint16obj (
	stix_t*              stix,
	const stix_uint16_t* ptr,
	stix_oow_t           len
);

stix_oop_t stix_allocuint32obj (
	stix_t*              stix,
	const stix_uint32_t* ptr,
	stix_oow_t           len
);

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
stix_oop_t stix_putatsysdic (
	stix_t*     stix,
	stix_oop_t  key,
	stix_oop_t  value
);

stix_oop_t stix_getatsysdic (
	stix_t*     stix,
	stix_oop_t  key
);

stix_oop_t stix_lookupsysdic (
	stix_t*           stix,
	const stix_ucs_t* name
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


/**
 * The stix_ucslen() function returns the number of characters before 
 * a terminating null.
 */
/*
stix_size_t stix_ucslen (
	const stix_uch_t* ucs
);
*/

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

#if defined(__cplusplus)
}
#endif


#endif
