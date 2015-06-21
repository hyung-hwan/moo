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

#include "stix-prv.h"

#define LOAD_IP(stix, v_ctx) ((stix)->ip = STIX_OOP_TO_SMINT((v_ctx)->ip))
#define STORE_IP(stix, v_ctx) ((v_ctx)->ip = STIX_OOP_FROM_SMINT((stix)->ip))

#define LOAD_SP(stix, v_ctx) ((stix)->sp = STIX_OOP_TO_SMINT((v_ctx)->sp))
#define STORE_SP(stix, v_ctx) ((v_ctx)->sp = STIX_OOP_FROM_SMINT((stix)->sp))

#define LOAD_ACTIVE_IP(stix) LOAD_IP(stix, (stix)->active_context)
#define STORE_ACTIVE_IP(stix) STORE_IP(stix, (stix)->active_context)

#define LOAD_ACTIVE_SP(stix) LOAD_SP(stix, (stix)->active_context)
#define STORE_ACTIVE_SP(stix) STORE_SP(stix, (stix)->active_context)


#define STACK_PUSH(stix,v) \
	do { \
		(stix)->sp = (stix)->sp + 1; \
		(stix)->active_context->slot[(stix)->sp] = v; \
	} while (0)

#define STACK_POP(stix) ((stix)->sp = (stix)->sp - 1)
#define STACK_UNPOP(stix) ((stix)->sp = (stix)->sp + 1)
#define STACK_POPS(stix,count) ((stix)->sp = (stix)->sp - (count))

#define STACK_GET(stix,v_sp) ((stix)->active_context->slot[v_sp])
#define STACK_SET(stix,v_sp,v_obj) ((stix)->active_context->slot[v_sp] = v_obj)
#define STACK_GETTOP(stix) STACK_GET(stix, (stix)->sp)
#define STACK_SETTOP(stix,v_obj) STACK_SET(stix, (stix)->sp, v_obj)

#define STACK_ISEMPTY(stix) ((stix)->sp <= -1)

#define SWITCH_ACTIVE_CONTEXT(stix,v_ctx) \
	do \
	{ \
		STORE_ACTIVE_IP (stix); \
		STORE_ACTIVE_SP (stix); \
		(stix)->active_context = (v_ctx); \
		LOAD_ACTIVE_IP (stix); \
		LOAD_ACTIVE_SP (stix); \
	} while (0) \

static int activate_new_method (stix_t* stix, stix_oop_method_t mth)
{
	stix_oow_t stack_size;
	stix_oop_context_t ctx;
	stix_ooi_t i;
	stix_ooi_t ntmprs, nargs;

	stack_size = 256; /* TODO: make the stack size configurable or let the compiler choose the rightr value and store it in the compiled method. if it's stored in the compiled method, the code here can take it*/

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, stack_size);
	stix_poptmp (stix);
	if (!ctx) return -1;

	/* message sending requires a receiver to be pushed. 
	 * the stack pointer of the sending context cannot be -1.
	 * if one-argumented message is invoked the stack of the
	 * sending context looks like this.
	 *
	 * Sending Context
	 *
	 *   +---------------------+
	 *   | fixed part          |
	 *   |                     |
	 *   |                     |
	 *   |                     |
	 *   +---------------------+
	 *   | ....                | slot[0]
	 *   | ....                | slot[..]
	 *   | ....                | slot[..] 
	 *   | receiver            | slot[..] <-- sp - nargs(1)
	 *   | arg1                | slot[..] <-- sp
 	 *   | ....                | slot[..] 
	 *   |                     | slot[stack_size - 1] 
	 *   +---------------------+
	 */
	ntmprs = STIX_OOP_TO_SMINT (mth->tmpr_count);
	nargs = STIX_OOP_TO_SMINT (mth->tmpr_nargs);

	STIX_ASSERT (ntmprs >= 0);
	STIX_ASSERT (nargs <= ntmprs);
	STIX_ASSERT (stix->sp >= 0);
	STIX_ASSERT (stix->sp >= nargs);

	ctx->sender = (stix_oop_t)stix->active_context; 
	ctx->ip = 0;

	/* the stack front has temporary variables including arguments.
	 *
	 * New Context
	 *
	 *   +---------------------+
	 *   | fixed part          |
	 *   |                     |
	 *   |                     |
	 *   |                     |
	 *   +---------------------+
	 *   | tmp1 (arg1)         | slot[0]
	 *   | tmp2 (arg2)         | slot[1]
	 *   | ....                | slot[..] 
	 *   | tmpX                | slot[..]     <-- initial sp
	 *   |                     | slot[..] 
	 *   |                     | slot[..] 
	 *   |                     | slot[stack_size - 2] 
	 *   |                     | slot[stack_size - 1] 
	 *   +---------------------+
	 *
	 * if no temporaries exist, the initial sp is -1.
	 */
	ctx->sp = STIX_OOP_FROM_SMINT(ntmprs - 1);
	ctx->method = mth;

	/* 
	 * Assume this message sending expression:
	 *   obj1 do: #this with: #that with: #it
	 * 
	 * It would be compiled to these logical byte-code sequences shown below:
	 *   push obj1
	 *   push #this
	 *   push #that
	 *   push #it
	 *   send #do:with:
	 *
	 * After three pushes, the stack looks like this.
	 * 
	 *  | #it   | <- sp
	 *  | #that |    sp - 1  
	 *  | #this |    sp - 2
	 *  | obj1  |    sp - nargs
	 *
	 * Since the number of arguments is 3, stack[sp - 3] points to
	 * the receiver. When the stack is empty, sp is -1.
	 */
	for (i = nargs; i > 0; )
	{
		/* copy argument */
		ctx->slot[--i] = STACK_GETTOP (stix);
		STACK_POP (stix);
	}
	/* copy receiver */
	ctx->receiver = STACK_GETTOP (stix);
	STACK_POP (stix);

	STIX_ASSERT (stix->sp >= -1);

	/* swtich the active context */
	SWITCH_ACTIVE_CONTEXT (stix, ctx);

printf ("<<ENTERING>>\n");
	return 0;
}

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t receiver, const stix_ucs_t* message, int super)
{
	stix_oop_class_t cls;
	stix_oop_association_t ass;
	stix_oop_t c;
	stix_oop_set_t mthdic;
	int dic_no;
/* TODO: implement method lookup cache */

printf ("==== FINDING METHOD FOR %p [", receiver);
print_ucs (message);
printf ("] in ");

	cls = (stix_oop_class_t)STIX_CLASSOF(stix, receiver);
	if ((stix_oop_t)cls == stix->_class)
	{
		/* receiver is a class object */
		c = receiver; 
		dic_no = STIX_CLASS_MTHDIC_CLASS;
printf ("class method dictioanry of ");
print_object(stix, (stix_oop_t)((stix_oop_class_t)c)->name); 
printf ("\n");
	}
	else
	{
		c = (stix_oop_t)cls;
		dic_no = STIX_CLASS_MTHDIC_INSTANCE;
printf ("instance method dictioanry of ");
print_object(stix, (stix_oop_t)((stix_oop_class_t)c)->name);
printf ("\n");
	}


	if (c != stix->_nil)
	{
		if (super) 
		{
			c = ((stix_oop_class_t)c)->superclass;
			if (c == stix->_nil) goto not_found;
		}

		do
		{
			mthdic = ((stix_oop_class_t)c)->mthdic[dic_no];
			STIX_ASSERT ((stix_oop_t)mthdic != stix->_nil);
			STIX_ASSERT (STIX_CLASSOF(stix, mthdic) == stix->_method_dictionary);

dump_dictionary (stix, mthdic, "Method dictionary");
			ass = (stix_oop_association_t)stix_lookupdic (stix, mthdic, message);
			if (ass) 
			{
				STIX_ASSERT (STIX_CLASSOF(stix, ass->value) == stix->_method);
				return (stix_oop_method_t)ass->value;
			}
			c = ((stix_oop_class_t)c)->superclass;
		}
		while (c != stix->_nil);
	}

not_found:
	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}

static int activate_initial_context (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	/* the initial context is a fake context. if objname is 'Stix' and
	 * mthname is 'main', this function emulates message sending 'Stix main'.
	 * it should emulate the following logical byte-code sequences:
	 *
	 *    push Stix
	 *    send #main
	 */

	stix_oop_context_t ctx;
	stix_oop_association_t ass;
	stix_oop_method_t mth;

	/* create a fake initial context */
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, 1);
	if (!ctx) return -1;

	ass = stix_lookupsysdic (stix, objname);
	if (!ass) return -1;

	mth = find_method (stix, ass->value, mthname, 0);
	if (!mth) return -1;

	if (STIX_OOP_TO_SMINT(mth->tmpr_nargs) > 0)
	{
		/* this method expects more than 0 arguments. 
		 * i can't use it as a start-up method.
TODO: overcome this problem 
		 */
		stix->errnum = STIX_EINVAL;
		return -1;
	}

/* TODO: handle preamble */

	


	/* the initial context starts the life of the entire VM
	 * and is not really worked on except that it is used to call the
	 * initial method. so it doesn't really require any extra stack space.
	 * TODO: verify my theory above is true */
	stix->ip = 0;
	stix->sp = -1;
	/* receiver, sender, method are nils */

	STIX_ASSERT (stix->active_context == STIX_NULL);
	/* i can't use SWITCH_ACTIVE_CONTEXT() macro as there is no active context before switching */
	stix->active_context = ctx;
	STACK_PUSH (stix, ass->value); /* push the receiver */

	STORE_ACTIVE_IP (stix);
	STORE_ACTIVE_SP (stix);

	return activate_new_method (stix, mth);
}


static int primitive_dump (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t i;

	STIX_ASSERT (nargs >=  0);

	dump_object (stix, STACK_GET(stix, stix->sp - nargs), "receiver");
	for (i = nargs; i > 0; )
	{
		--i;
		dump_object (stix, STACK_GET(stix, stix->sp - i), "argument");
	}

	STACK_POPS (stix, nargs);
	return 1; /* success */
}

static int primitive_new (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, obj;

	STIX_ASSERT (nargs ==  0);

	rcv = STACK_GETTOP (stix);

	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	obj = stix_instantiate (stix, rcv, STIX_NULL, 0);
	if (!obj) return -1;

	/* emulate 'pop receiver' and 'push result' */
	STACK_SETTOP (stix, obj);
	return 1; /* success */
}

static int primitive_new_with_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, szoop, obj;
	stix_oow_t size;

	STIX_ASSERT (nargs ==  1);

	rcv = STACK_GET(stix, stix->sp - 1);
	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	szoop = STACK_GET(stix, stix->sp);
	if (STIX_OOP_IS_SMINT(szoop))
	{
		size = STIX_OOP_TO_SMINT(szoop);
	}
/* TODO: support LargeInteger */
	else
	{
		/* size is not a proper numeric object */
		return 0;
	}

	/* stix_instantiate() ignores size if the instance specification 
	 * disallows indexed(variable) parts. */
	/* TODO: should i check the specification before calling 
	 *       stix_instantiate()? */
	obj = stix_instantiate (stix, rcv, STIX_NULL, size);
	if (!obj) 
	{
		return -1; /* hard failure */
	}

	/* remove the argument and replace the receiver with a new object
	 * instantiated */
	STACK_POP (stix);
	STACK_SETTOP (stix, obj);

	return 1; /* success */
}

static int primitive_basic_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);


	rcv = STACK_GETTOP(stix);
	STACK_SETTOP(stix, STIX_OOP_FROM_SMINT(STIX_OBJ_GET_SIZE(rcv)));
/* TODO: use LargeInteger if the size is very big */
	return 1;
}

static int primitive_basic_at (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, v;
	stix_ooi_t idx;

	STIX_ASSERT (nargs == 1);

	rcv = STACK_GET(stix, stix->sp - 1);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = STACK_GET(stix, stix->sp);
	if (!STIX_OOP_IS_SMINT(pos))
	{
/* TODO: handle LargeInteger */
		/* the position must be an integer */
		return 0;
	}

	idx = STIX_OOP_TO_SMINT(pos);
	if (idx < 1 || idx > STIX_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		return 0;
	}

	/* [NOTE] basicAt: and basicAt:put: used a 1-based index. */
	idx = idx - 1;

	switch (STIX_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case STIX_OBJ_TYPE_BYTE:
			v = STIX_OOP_FROM_SMINT(((stix_oop_byte_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_CHAR:
			v = STIX_OOP_FROM_CHAR(((stix_oop_char_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: largeINteger if the word is too large */
			v = STIX_OOP_FROM_SMINT(((stix_oop_word_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_OOP:
			v = ((stix_oop_oop_t)rcv)->slot[idx];
			break;

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}

	STACK_POP (stix);
	STACK_SETTOP (stix, v);
	return 1;
}

static int primitive_basic_at_put (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, val;
	stix_ooi_t idx;

	STIX_ASSERT (nargs == 2);

/* TODO: disallow change of some key kernel objects */
 
	rcv = STACK_GET(stix, stix->sp - 2);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = STACK_GET(stix, stix->sp - 1);
	if (!STIX_OOP_IS_SMINT(pos))
	{
/* TODO: handle LargeInteger */
		/* the position must be an integer */
		return 0;
	}

	val = STACK_GET(stix, stix->sp);

	idx = STIX_OOP_TO_SMINT(pos);
	if (idx < 1 || idx > STIX_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		return 0;
	}

	/* [NOTE] basicAt: and basicAt:put: used a 1-based index. */
	idx = idx - 1;

	switch (STIX_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case STIX_OBJ_TYPE_BYTE:
			if (!STIX_OOP_IS_SMINT(val))
			{
				/* the value is not a character */
				return 0;
			}
/* TOOD: must I check the range of the value? */
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_SMINT(val);
			break;
		

		case STIX_OBJ_TYPE_CHAR:
			if (!STIX_OOP_IS_CHAR(val))
			{
				/* the value is not a character */
				return 0;
			}
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_CHAR(val);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: handle largeINteger  */
			if (!STIX_OOP_IS_SMINT(val))
			{
				/* the value is not a character */
				return 0;
			}
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_SMINT(val);
			break;

		case STIX_OBJ_TYPE_OOP:
			((stix_oop_oop_t)rcv)->slot[idx] = val;
			break;

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}

	STACK_POPS (stix, 2);
/* TODO: return receiver or value? */
	STACK_SETTOP (stix, val);
	return 1;
}

static int primitive_block_context_value (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_block_context_t blkctx;

	blkctx = (stix_oop_block_context_t)STACK_GET(stix, stix->sp - nargs);
	STIX_ASSERT (STIX_CLASSOF(stix, blkctx) == stix->_block_context);

	if (STIX_OOP_TO_SMINT(blkctx->nargs) != nargs) 
	{
		/* the number of argument doesn't match */
printf ("PRIM BlockContext value FAIL - NARGS MISMATCH\n");
		return 0;
	}

	STACK_POPS (stix, nargs + 1); /* pop arguments and receiver */

	blkctx->ip = blkctx->iip;
	blkctx->sp = STIX_OOP_FROM_SMINT(nargs);
	blkctx->caller = (stix_oop_t)stix->active_context;

	SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)blkctx);
	return 1;
}

static int primitive_integer_add (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t tmp;
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = STACK_GET(stix, stix->sp - 1);
	arg = STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMINT(rcv) && STIX_OOP_IS_SMINT(arg))
	{
		tmp = STIX_OOP_TO_SMINT(rcv) + STIX_OOP_TO_SMINT(arg);
		/* TODO: check overflow. if so convert it to LargeInteger */

		STACK_POP (stix);
		STACK_SETTOP (stix, STIX_OOP_FROM_SMINT(tmp));
		return 1;
	}

/* TODO: handle LargeInteger */
	return 0;
}


typedef int (*primitive_handler_t) (stix_t* stix, stix_ooi_t nargs);

struct primitive_t
{
	stix_ooi_t          nargs; /* expected number of arguments */
	primitive_handler_t handler;
};
typedef struct primitive_t primitive_t;

static primitive_t primitives[] =
{
	{  -1,   primitive_dump                 },
	{   0,   primitive_new                  },
	{   1,   primitive_new_with_size        },
	{   0,   primitive_basic_size           },
	{   1,   primitive_basic_at             },
	{   2,   primitive_basic_at_put         },
	{  -1,   primitive_block_context_value  },
	{   1,   primitive_integer_add          },
};

int stix_execute (stix_t* stix)
{
	stix_oop_method_t mth;
	stix_oop_byte_t code;

	stix_byte_t bc, cmd;
	stix_ooi_t b1;

	STIX_ASSERT (stix->active_context != STIX_NULL);

	while (1)
	{
/* TODO: improve how to access method??? */
		if (stix->active_context->home == stix->_nil)
			mth = stix->active_context->method;
		else
			mth = ((stix_oop_block_context_t)stix->active_context)->origin->method;
		code = mth->code;

printf ("IP => %d ", (int)stix->ip);
		bc = code->slot[stix->ip++];
		/*if (bc == CODE_NOOP) continue; TODO: DO I NEED THIS???*/

		cmd = bc >> 4;
		if (cmd == CMD_EXTEND)
		{
			cmd = bc & 0xF;
			if (cmd == CMD_JUMP || cmd == CMD_JUMP_IF_FALSE)
				b1 = (stix_int8_t)code->slot[stix->ip++];
			else
				b1 = code->slot[stix->ip++];
		}
		else if (cmd == CMD_EXTEND_DOUBLE)
		{
			cmd = bc & 0xF;
			b1 = code->slot[stix->ip++];

			if (cmd == CMD_JUMP || cmd == CMD_JUMP_IF_FALSE)
				b1 = (stix_int16_t)((b1 << 8) | code->slot[stix->ip++]); /* JUMP encodes a signed offset */
			else
				b1 = (b1 << 8) | code->slot[stix->ip++];
		}
		else
		{
			b1 = bc & 0xF;
		}

printf ("CMD => %d, B1 = %d, SP = %d, IP AFTER INC %d\n", (int)cmd, (int)b1, (int)stix->sp, (int)stix->ip);
		switch (cmd)
		{

			case CMD_PUSH_INSTVAR:
printf ("PUSH_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver) == STIX_OBJ_TYPE_OOP);
				STACK_PUSH (stix, ((stix_oop_oop_t)stix->active_context->receiver)->slot[b1]);
				break;

			case CMD_PUSH_TEMPVAR:
/* TODO: consider temp offset, block context, etc */

printf ("PUSH_TEMPVAR idx=%d - ", (int)b1);
print_object (stix, STACK_GET(stix, b1));
printf ("\n");
				STACK_PUSH (stix, STACK_GET(stix, b1));
				break;

			case CMD_PUSH_LITERAL:
printf ("PUSH_LITERAL idx=%d - ", (int)b1);
print_object (stix, mth->slot[b1]);
printf ("\n");
				STACK_PUSH (stix, mth->slot[b1]);
				break;

			case CMD_STORE_INTO_INSTVAR:
printf ("STORE_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->receiver)->slot[b1] = STACK_GETTOP(stix);
				break;

			case CMD_STORE_INTO_TEMPVAR:
printf ("STORE_TEMPVAR %d\n", (int)b1); /* TODO: consider temp offset block context etc */
				STACK_SET (stix, b1, STACK_GETTOP(stix));
				break;

		/* -------------------------------------------------------- */

/* TODO: CMD_JUMP_IF_FALSE */
			case CMD_JUMP:
printf ("JUMP %d\n", (int)b1);
				stix->ip += b1;
				break;

		/* -------------------------------------------------------- */

			case CMD_PUSH_OBJVAR:
			{
/* COMPACT CODE FOR CMD_PUSH_OBJVAR AND CMD_STORE_INTO_OBJVAR by sharing */
				/* b1 -> variable index */
				stix_ooi_t obj_index;
				stix_oop_oop_t obj;

				obj_index = code->slot[stix->ip++];
				if (cmd == CMD_EXTEND_DOUBLE) 
					obj_index = (obj_index << 8) | code->slot[stix->ip++];

				obj = (stix_oop_oop_t)mth->slot[obj_index];
printf ("PUSH OBJVAR %d %d\n", (int)b1, (int)obj_index);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(obj) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (obj_index < STIX_OBJ_GET_SIZE(obj));
				STACK_PUSH (stix, obj->slot[b1]);
				break;
			}

			case CMD_STORE_INTO_OBJVAR:
			{
				stix_ooi_t obj_index;
				stix_oop_oop_t obj;
				obj_index = code->slot[stix->ip++];
				if (cmd == CMD_EXTEND_DOUBLE) 
					obj_index = (obj_index << 8) | code->slot[stix->ip++];

printf ("STORE OBJVAR %d %d\n", (int)b1, (int)obj_index);
				obj = (stix_oop_oop_t)mth->slot[obj_index];
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(obj) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (obj_index < STIX_OBJ_GET_SIZE(obj));
				obj->slot[b1] = STACK_GETTOP(stix);
				break;
			}

		/* -------------------------------------------------------- */
			case CMD_SEND_MESSAGE:
			case CMD_SEND_MESSAGE_TO_SUPER:
			{
/* TODO: tail call optimization */
				/* b1 -> number of arguments 
				 */
				stix_ucs_t mthname;
				stix_oop_t newrcv;
				stix_oop_method_t newmth;
				stix_oop_char_t selector;
				stix_ooi_t selector_index;
				stix_ooi_t preamble;

				/* the next byte is the message selector index to the
				 * literal frame. */
				selector_index = code->slot[stix->ip++];
				if (cmd == CMD_EXTEND_DOUBLE) 
					selector_index = (selector_index << 8) | code->slot[stix->ip++];

				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)mth->slot[selector_index];

if (cmd == CMD_SEND_MESSAGE)
printf ("SEND_MESSAGE TO RECEIVER AT %d NARGS=%d\n", (int)(stix->sp - b1), (int)b1);
else
printf ("SEND_MESSAGE_TO_SUPER TO RECEIVER AT %d NARGS=%d\n", (int)(stix->sp - b1), (int)b1);
				STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);

				newrcv = STACK_GET(stix, stix->sp - b1);
print_object(stix, newrcv);
printf ("\n");
				mthname.ptr = selector->slot;
				mthname.len = STIX_OBJ_GET_SIZE(selector);
				newmth = find_method (stix, newrcv, &mthname, (cmd == CMD_SEND_MESSAGE_TO_SUPER));
				if (!newmth) 
				{
/* TODO: implement doesNotUnderstand: XXXXX  instead of returning -1. */
printf ("no such method .........[");
print_ucs (&mthname);
printf ("]\n");
					goto oops;
				}

				STIX_ASSERT (STIX_OOP_TO_SMINT(newmth->tmpr_nargs) == b1);

				preamble = STIX_OOP_TO_SMINT(newmth->preamble);
				switch (STIX_METHOD_GET_PREAMBLE_CODE(preamble))
				{
					case STIX_METHOD_PREAMBLE_RETURN_RECEIVER:
printf ("RETURN RECEIVER AT PREAMBLE\n");
						STACK_POPS (stix, b1); /* pop arguments only*/
						break;

					case STIX_METHOD_PREAMBLE_RETURN_INSTVAR:
					{
						stix_oop_oop_t rcv;

						STACK_POPS (stix, b1); /* pop arguments only */

printf ("RETURN INSTVAR AT PREAMBLE\n");
						/* replace the receiver by an instance variable of the receiver */
						rcv = (stix_oop_oop_t)STACK_GETTOP(stix);
						STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(rcv) == STIX_OBJ_TYPE_OOP);
						STIX_ASSERT (STIX_OBJ_GET_SIZE(rcv) > STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
						STACK_SET (stix, stix->sp, rcv->slot[STIX_METHOD_GET_PREAMBLE_INDEX(preamble)]);
						break;
					}

					case STIX_METHOD_PREAMBLE_PRIMITIVE:
					{
						int n;
						stix_ooi_t prim_no;

						prim_no = STIX_METHOD_GET_PREAMBLE_INDEX(preamble);
						if (prim_no >= 0 && prim_no < STIX_COUNTOF(primitives) && 
						    (primitives[prim_no].nargs < 0 || primitives[prim_no].nargs == b1))
						{
							stix_pushtmp (stix, (stix_oop_t*)&newmth);
							n = primitives[prim_no].handler (stix, b1);
							stix_poptmp (stix);
							if (n <= -1) goto oops;
							if (n >= 1) break;
						}

						/* primitive failed. fall through */
					}

					default:
						if (activate_new_method (stix, newmth) <= -1) goto oops;
						break;
				}

				break; /* CMD_SEND_MESSAGE */
			}

		/* -------------------------------------------------------- */

			case CMD_PUSH_SPECIAL:
				switch (b1)
				{
					case SUBCMD_PUSH_RECEIVER:
printf ("PUSH_RECEIVER %p TO STACK INDEX %d\n", stix->active_context->receiver, (int)stix->sp);
						STACK_PUSH (stix, stix->active_context->receiver);
						break;

					case SUBCMD_PUSH_NIL:
printf ("PUSH_NIL\n");
						STACK_PUSH (stix, stix->_nil);
						break;

					case SUBCMD_PUSH_TRUE:
printf ("PUSH_TRUE\n");
						STACK_PUSH (stix, stix->_true);
						break;

					case SUBCMD_PUSH_FALSE:
printf ("PUSH_FALSE\n");
						STACK_PUSH (stix, stix->_false);
						break;

					case SUBCMD_PUSH_CONTEXT:
printf ("PUSH_CONTEXT\n");
						STACK_PUSH (stix, (stix_oop_t)stix->active_context);
						break;

					case SUBCMD_PUSH_NEGONE:
printf ("PUSH_NEGONE\n");
						STACK_PUSH (stix, STIX_OOP_FROM_SMINT(-1));
						break;

					case SUBCMD_PUSH_ZERO:
printf ("PUSH_ZERO\n");
						STACK_PUSH (stix, STIX_OOP_FROM_SMINT(0));
						break;

					case SUBCMD_PUSH_ONE:
printf ("PUSH_SMINT\n");
						STACK_PUSH (stix, STIX_OOP_FROM_SMINT(1));
						break;
				}
				break; /* CMD_PUSH_SPECIAL */

		/* -------------------------------------------------------- */

			case CMD_DO_SPECIAL:
			{
				stix_oop_t return_value;

				switch (b1)
				{
					case SUBCMD_POP_STACKTOP:
printf ("POP_STACKTOP\n");
						STIX_ASSERT (!STACK_ISEMPTY(stix));
						STACK_POP (stix);
						break;

					case SUBCMD_RETURN_STACKTOP:
printf ("RETURN_STACKTOP\n");
						return_value = STACK_GETTOP(stix);
						STACK_POP (stix);
						goto handle_return;

					case SUBCMD_RETURN_RECEIVER:
printf ("RETURN_RECEIVER\n");
						return_value = stix->active_context->receiver;
						goto handle_return;

					case SUBCMD_RETURN_FROM_BLOCK:
					{
						stix_oop_block_context_t blkctx;
						

						STIX_ASSERT(STIX_CLASSOF(stix, stix->active_context)  == stix->_block_context);

						return_value = STACK_GETTOP(stix);
						blkctx = (stix_oop_block_context_t)stix->active_context;
						SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)blkctx->caller);
						STACK_PUSH (stix, return_value);
						break;
					}


					case SUBCMD_SEND_BLOCK_COPY:
					{
printf ("SEND_BLOCK_COPY\n");
						stix_ooi_t nargs, ntmprs;
						stix_oop_t rctx;
						stix_oop_block_context_t blkctx;

						/* it emulates thisContext blockCopy: nargs ofTmprCount: ntmprs */
						STIX_ASSERT (stix->sp >= 2);

						STIX_ASSERT (STIX_CLASSOF(stix, STACK_GETTOP(stix)) == stix->_small_integer);
						ntmprs = STIX_OOP_TO_SMINT(STACK_GETTOP(stix));
						STACK_POP (stix);

						STIX_ASSERT (STIX_CLASSOF(stix, STACK_GETTOP(stix)) == stix->_small_integer);
						nargs = STIX_OOP_TO_SMINT(STACK_GETTOP(stix));
						STACK_POP (stix);

						STIX_ASSERT (nargs >= 0);
						STIX_ASSERT (ntmprs >= nargs);

						blkctx = (stix_oop_block_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 255); /* TODO: proper stack size */
						if (!blkctx) return -1;

						rctx = STACK_GETTOP(stix);

						/* blkctx->caller is left to nil */
						blkctx->ip = STIX_OOP_FROM_SMINT(stix->ip + 3); /* skip the following JUMP */
						blkctx->sp = STIX_OOP_FROM_SMINT(0);
						blkctx->nargs = STIX_OOP_FROM_SMINT(nargs);
						blkctx->ntmprs = STIX_OOP_FROM_SMINT(ntmprs);
						blkctx->iip = STIX_OOP_FROM_SMINT(stix->ip + 3);

						blkctx->home = rctx;
						if (((stix_oop_context_t)rctx)->home == stix->_nil)
						{
							/* the context that receives the blockCopy message is a method context */
							STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_context);
							STIX_ASSERT (rctx == (stix_oop_t)stix->active_context);
							blkctx->origin = (stix_oop_context_t)rctx;
						}
						else
						{
							/* block context is active */
							STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_block_context);
							blkctx->origin = ((stix_oop_block_context_t)rctx)->origin;
						}

						STACK_SETTOP (stix, (stix_oop_t)blkctx);
						break;
					}

					default:
						stix->errnum = STIX_EINTERN;
						break;


					handle_return:
/* TODO: consider block context.. jump to origin if in a block context */
						SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->sender);

						/* push the return value to the stack of the new active context */
						STACK_PUSH (stix, return_value);

printf ("<<LEAVING>>\n");
						if (stix->active_context->sender == stix->_nil) 
						{
							/* the sending context of the intial context has been set to nil.
							 * use this fact to tell an initial context from a normal context. */
printf ("<<<RETURNIGN TO THE INITIAL CONTEXT>>>\n");
							STIX_ASSERT (stix->sp == 0);
							goto done;
						}

						break;
				}
				break; /* CMD_DO_SPECIAL */
			}

		}
	}

done:
	stix->active_context_sp = STIX_NULL;
	return 0;


oops:
	stix->active_context_sp = STIX_NULL;
	return -1;
}

int stix_invoke (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}
