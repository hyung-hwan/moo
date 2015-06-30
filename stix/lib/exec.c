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

/* TODO: context's stack overflow check in various part of this file */
/* TOOD: determine the right stack size */
#define CONTEXT_STACK_SIZE 96

#define LOAD_IP(stix, v_ctx) ((stix)->ip = STIX_OOP_TO_SMINT((v_ctx)->ip))
#define STORE_IP(stix, v_ctx) ((v_ctx)->ip = STIX_OOP_FROM_SMINT((stix)->ip))

#define LOAD_SP(stix, v_ctx) ((stix)->sp = STIX_OOP_TO_SMINT((v_ctx)->sp))
#define STORE_SP(stix, v_ctx) ((v_ctx)->sp = STIX_OOP_FROM_SMINT((stix)->sp))

#define LOAD_ACTIVE_IP(stix) LOAD_IP(stix, (stix)->active_context)
#define STORE_ACTIVE_IP(stix) STORE_IP(stix, (stix)->active_context)

#define LOAD_ACTIVE_SP(stix) LOAD_SP(stix, (stix)->active_context)
#define STORE_ACTIVE_SP(stix) STORE_SP(stix, (stix)->active_context)


#define ACTIVE_STACK_PUSH(stix,v) \
	do { \
		(stix)->sp = (stix)->sp + 1; \
		(stix)->active_context->slot[(stix)->sp] = v; \
	} while (0)

#define ACTIVE_STACK_POP(stix) ((stix)->sp = (stix)->sp - 1)
#define ACTIVE_STACK_UNPOP(stix) ((stix)->sp = (stix)->sp + 1)
#define ACTIVE_STACK_POPS(stix,count) ((stix)->sp = (stix)->sp - (count))

#define ACTIVE_STACK_GET(stix,v_sp) ((stix)->active_context->slot[v_sp])
#define ACTIVE_STACK_SET(stix,v_sp,v_obj) ((stix)->active_context->slot[v_sp] = v_obj)
#define ACTIVE_STACK_GETTOP(stix) ACTIVE_STACK_GET(stix, (stix)->sp)
#define ACTIVE_STACK_SETTOP(stix,v_obj) ACTIVE_STACK_SET(stix, (stix)->sp, v_obj)

#define ACTIVE_STACK_ISEMPTY(stix) ((stix)->sp <= -1)


#define SWITCH_ACTIVE_CONTEXT(stix,v_ctx) \
	do \
	{ \
		STORE_ACTIVE_IP (stix); \
		STORE_ACTIVE_SP (stix); \
		(stix)->active_context = (v_ctx); \
		(stix)->active_method = (stix_oop_method_t)(stix)->active_context->origin->method_or_nargs; \
		(stix)->active_code = (stix)->active_method->code; \
		LOAD_ACTIVE_IP (stix); \
		LOAD_ACTIVE_SP (stix); \
	} while (0) \

static STIX_INLINE int activate_new_method (stix_t* stix, stix_oop_method_t mth)
{
	stix_oop_context_t ctx;
	stix_ooi_t i;
	stix_ooi_t ntmprs, nargs;

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
	ntmprs = STIX_OOP_TO_SMINT(mth->tmpr_count);
	nargs = STIX_OOP_TO_SMINT(mth->tmpr_nargs);

	STIX_ASSERT (ntmprs >= 0);
	STIX_ASSERT (nargs <= ntmprs);
	STIX_ASSERT (stix->sp >= 0);
	STIX_ASSERT (stix->sp >= nargs);

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, CONTEXT_STACK_SIZE);
	stix_poptmp (stix);
	if (!ctx) return -1;

	ctx->sender = (stix_oop_t)stix->active_context; 
	ctx->ip = STIX_OOP_FROM_SMINT(0);
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
	ctx->ntmprs = STIX_OOP_FROM_SMINT(ntmprs);
	ctx->method_or_nargs = (stix_oop_t)mth;
	/* the 'home' field of a method context is always stix->_nil.
	ctx->home = stix->_nil;*/
	ctx->origin = ctx; /* point to self */

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
		ctx->slot[--i] = ACTIVE_STACK_GETTOP (stix);
		ACTIVE_STACK_POP (stix);
	}
	/* copy receiver */
	ctx->receiver_or_source = ACTIVE_STACK_GETTOP (stix);
	ACTIVE_STACK_POP (stix);

	STIX_ASSERT (stix->sp >= -1);

	/* swtich the active context */
	SWITCH_ACTIVE_CONTEXT (stix, ctx);

printf ("<<ENTERING>> SP=%d\n", (int)stix->sp);
	return 0;

#if 0
reuse_context:
	/* force the class to become a method context */
	ctx->_class = stix->_method_context;

	ctx->receiver_or_source = ACTIVE_STACK_GET(stix, stix->sp - nargs);
printf ("####### REUSING CONTEXT INSTEAD OF <<ENTERING>> WITH RECEIVER ");
print_object (stix, ctx->receiver_or_source);
printf ("\n");

	for (i = 0; i < nargs; i++)
	{
		ctx->slot[i] = ACTIVE_STACK_GET (stix, stix->sp - nargs + i + 1);
printf ("REUSING ARGUMENT %d - ", (int)i);
print_object (stix, ctx->slot[i]);
printf ("\n");
	}
	for (; i <= stix->sp; i++) ctx->slot[i] = stix->_nil;
	/* keep the sender 
	ctx->sender = 
	*/
	
	ctx->ntmprs = STIX_OOP_FROM_SMINT(ntmprs);
	ctx->method_or_nargs = (stix_oop_t)mth;
	ctx->home = stix->_nil;
	ctx->origin = ctx;

	/* let SWITCH_ACTIVE_CONTEXT() fill 'ctx->ip' and 'ctx->sp' by putting
	 * the values to stix->ip and stix->sp */
	stix->ip = 0;
	stix->sp = ntmprs - 1;
	SWITCH_ACTIVE_CONTEXT (stix, ctx);

	return 0;
#endif
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

/*dump_dictionary (stix, mthdic, "Method dictionary");*/
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
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, 1);
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

	ctx->origin = ctx;
	ctx->method_or_nargs = (stix_oop_t)mth; /* fake. help SWITCH_ACTIVE_CONTEXT() not fail*/
	/* receiver, sender of ctx are nils */

	STIX_ASSERT (stix->active_context == STIX_NULL);
	/* i can't use SWITCH_ACTIVE_CONTEXT() macro as there is no active context before switching */
	stix->active_context = ctx;
	ACTIVE_STACK_PUSH (stix, ass->value); /* push the receiver */

	STORE_ACTIVE_IP (stix);
	STORE_ACTIVE_SP (stix);

	return activate_new_method (stix, mth);
}

static int primitive_dump (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t i;

	STIX_ASSERT (nargs >=  0);

	printf ("RECEIVER: ");
	print_object (stix, ACTIVE_STACK_GET(stix, stix->sp - nargs));
	printf ("\n");
	for (i = nargs; i > 0; )
	{
		--i;
		printf ("ARGUMENT: ");
		print_object (stix, ACTIVE_STACK_GET(stix, stix->sp - i));
		printf ("\n");
	}

	ACTIVE_STACK_POPS (stix, nargs);
	return 1; /* success */
}

static int primitive_new (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, obj;

	STIX_ASSERT (nargs ==  0);

	rcv = ACTIVE_STACK_GETTOP (stix);

	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	obj = stix_instantiate (stix, rcv, STIX_NULL, 0);
	if (!obj) return -1;

	/* emulate 'pop receiver' and 'push result' */
	ACTIVE_STACK_SETTOP (stix, obj);
	return 1; /* success */
}

static int primitive_new_with_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, szoop, obj;
	stix_oow_t size;

	STIX_ASSERT (nargs ==  1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	szoop = ACTIVE_STACK_GET(stix, stix->sp);
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
	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, obj);

	return 1; /* success */
}

static int primitive_basic_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);


	rcv = ACTIVE_STACK_GETTOP(stix);
	ACTIVE_STACK_SETTOP(stix, STIX_OOP_FROM_SMINT(STIX_OBJ_GET_SIZE(rcv)));
/* TODO: use LargeInteger if the size is very big */
	return 1;
}

static int primitive_basic_at (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, v;
	stix_ooi_t idx;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = ACTIVE_STACK_GET(stix, stix->sp);
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

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, v);
	return 1;
}

static int primitive_basic_at_put (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, val;
	stix_ooi_t idx;

	STIX_ASSERT (nargs == 2);

/* TODO: disallow change of some key kernel objects */
 
	rcv = ACTIVE_STACK_GET(stix, stix->sp - 2);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = ACTIVE_STACK_GET(stix, stix->sp - 1);
	if (!STIX_OOP_IS_SMINT(pos))
	{
/* TODO: handle LargeInteger */
		/* the position must be an integer */
		return 0;
	}

	val = ACTIVE_STACK_GET(stix, stix->sp);

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

	ACTIVE_STACK_POPS (stix, 2);
/* TODO: return receiver or value? */
	ACTIVE_STACK_SETTOP (stix, val);
	return 1;
}

static int primitive_block_context_value (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_context_t blkctx, org_blkctx;
	stix_ooi_t local_ntmprs, i;

	/* TODO: find a better way to support a reentrant block context. */

	/* | sum |
	 * sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1))] ].
	 * (sum value: 10).
	 * 
	 * For the code above, sum is a block context and it is sent value: inside
	 * itself. Let me simply clone a block context to allow reentrancy like this
	 * while the block context is active
	 */
	org_blkctx = (stix_oop_context_t)ACTIVE_STACK_GET(stix, stix->sp - nargs);
	if (STIX_CLASSOF(stix, org_blkctx) != stix->_block_context)
	{
printf ("PRIMITVE VALUE RECEIVER IS NOT A BLOCK CONTEXT\n");
		return 0;
	}

	if (org_blkctx->receiver_or_source != stix->_nil)
	{
		/* the 'source' field is not nil.
		 * this block context has already been activated once.
		 * you can't send 'value' again to reactivate it.
		 * For example, [thisContext value] value.
		 */
		STIX_ASSERT (STIX_OBJ_GET_SIZE(org_blkctx) > STIX_CONTEXT_NAMED_INSTVARS);
printf ("PRIM REVALUING AN BLOCKCONTEXT\n");
		return 0;
	}
	STIX_ASSERT (STIX_OBJ_GET_SIZE(org_blkctx) == STIX_CONTEXT_NAMED_INSTVARS);

	if (STIX_OOP_TO_SMINT(org_blkctx->method_or_nargs) != nargs) 
	{
		/* the number of argument doesn't match */
/* TODO: better handling of primitive failure */
printf ("PRIM BlockContext value FAIL - NARGS MISMATCH\n");
		return 0;
	}

	/* create a new block context to clone org_blkctx */
	blkctx = (stix_oop_context_t) stix_instantiate (stix, stix->_block_context, STIX_NULL, CONTEXT_STACK_SIZE); 
	if (!blkctx) return -1;

	/* getting org_blkctx again to be GC-safe for stix_instantiate() above */
	org_blkctx = (stix_oop_context_t)ACTIVE_STACK_GET(stix, stix->sp - nargs); 
	STIX_ASSERT (STIX_CLASSOF(stix, org_blkctx) == stix->_block_context);

#if 0
	/* shallow-copy the named part including home, origin, etc. */
	for (i = 0; i < STIX_CONTEXT_NAMED_INSTVARS; i++)
	{
		((stix_oop_oop_t)blkctx)->slot[i] = ((stix_oop_oop_t)org_blkctx)->slot[i];
	}
#else
	blkctx->ip = org_blkctx->ip;
	blkctx->ntmprs = org_blkctx->ntmprs;
	blkctx->method_or_nargs = org_blkctx->method_or_nargs;
	blkctx->receiver_or_source = (stix_oop_t)org_blkctx;
	blkctx->home = org_blkctx->home;
	blkctx->origin = org_blkctx->origin;
printf ("~~~~~~~~~~ BLOCK VALUING %p TO NEW BLOCK %p\n", org_blkctx, blkctx);
#endif

/* TODO: check the stack size of a block context to see if it's large enough to hold arguments */
	/* copy the arguments to the stack */
	for (i = 0; i < nargs; i++)
	{
		blkctx->slot[i] = ACTIVE_STACK_GET(stix, stix->sp - nargs + i + 1);
	}
	ACTIVE_STACK_POPS (stix, nargs + 1); /* pop arguments and receiver */

	/*blkctx->ip = blkctx->iip;*/

	STIX_ASSERT (blkctx->home != stix->_nil);

	/* the number of temporaries stored in the block context
	 * accumulates the number of temporaries starting from the origin.
	 * simple calculation is needed to find the number of local temporaries */
	local_ntmprs = STIX_OOP_TO_SMINT(blkctx->ntmprs) -
	               STIX_OOP_TO_SMINT(((stix_oop_context_t)blkctx->home)->ntmprs);
	STIX_ASSERT (local_ntmprs >= nargs);

	blkctx->sp = STIX_OOP_FROM_SMINT(local_ntmprs);
	blkctx->sender = (stix_oop_t)stix->active_context;

printf ("<<ENTERING BLOCK>>\n");
	SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)blkctx);
	return 1;
}

static int primitive_integer_add (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t tmp;
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMINT(rcv) && STIX_OOP_IS_SMINT(arg))
	{
		tmp = STIX_OOP_TO_SMINT(rcv) + STIX_OOP_TO_SMINT(arg);
		/* TODO: check overflow. if so convert it to LargeInteger */

		ACTIVE_STACK_POP (stix);
		ACTIVE_STACK_SETTOP (stix, STIX_OOP_FROM_SMINT(tmp));
		return 1;
	}

/* TODO: handle LargeInteger */
	return 0;
}

static int primitive_integer_sub (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t tmp;
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMINT(rcv) && STIX_OOP_IS_SMINT(arg))
	{
		tmp = STIX_OOP_TO_SMINT(rcv) - STIX_OOP_TO_SMINT(arg);
		/* TODO: check overflow. if so convert it to LargeInteger */

		ACTIVE_STACK_POP (stix);
		ACTIVE_STACK_SETTOP (stix, STIX_OOP_FROM_SMINT(tmp));
		return 1;
	}

/* TODO: handle LargeInteger */
	return 0;
}

static int primitive_integer_lt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMINT(rcv) && STIX_OOP_IS_SMINT(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMINT(rcv) < STIX_OOP_TO_SMINT(arg))
		{
			ACTIVE_STACK_SETTOP (stix, stix->_true);
		}
		else
		{
			ACTIVE_STACK_SETTOP (stix, stix->_false);
		}
		
		return 1;
	}

/* TODO: handle LargeInteger */
	return 0;
}

static int primitive_integer_gt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMINT(rcv) && STIX_OOP_IS_SMINT(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMINT(rcv) > STIX_OOP_TO_SMINT(arg))
		{
			ACTIVE_STACK_SETTOP (stix, stix->_true);
		}
		else
		{
			ACTIVE_STACK_SETTOP (stix, stix->_false);
		}
		
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
	{   1,   primitive_integer_sub          },
	{   1,   primitive_integer_lt           },
	{   1,   primitive_integer_gt           }
};


#define FETCH_BYTE_CODE_TO(stix, v_ooi) (v_ooi = (stix)->active_code->slot[(stix)->ip++])

#if (STIX_BCODE_LONG_PARAM_SIZE == 2)

#define FETCH_PARAM_CODE_TO(stix, v_ooi) \
	do { \
		v_ooi = (stix)->active_code->slot[(stix)->ip++]; \
		v_ooi = (v_ooi << 8) | (stix)->active_code->slot[stix->ip++]; \
	} while (0)

#else /* STIX_BCODE_LONG_PARAM_SIZE == 2 */

#define FETCH_PARAM_CODE_TO(stix, v_ooi) (v_ooi = (stix)->active_code->slot[(stix)->ip++])

#endif /* STIX_BCODE_LONG_PARAM_SIZE == 2 */


int stix_execute (stix_t* stix)
{
	stix_byte_t bcode;
	stix_ooi_t b1, b2, bx;
	stix_oop_t return_value;

stix_size_t inst_counter;

	STIX_ASSERT (stix->active_context != STIX_NULL);

	inst_counter = 0;

	while (1)
	{

#if 0
printf ("IP => %d ", (int)stix->ip);
#endif
		FETCH_BYTE_CODE_TO (stix, bcode);
		/*while (bcode == BCODE_NOOP) FETCH_BYTE_CODE_TO (stix, bcode);*/

#if 0
printf ("BCODE = %x\n", bcode);
#endif
inst_counter++;

		switch (bcode)
		{
			/* ------------------------------------------------- */

			case BCODE_PUSH_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto push_instvar;
			case BCODE_PUSH_INSTVAR_0:
			case BCODE_PUSH_INSTVAR_1:
			case BCODE_PUSH_INSTVAR_2:
			case BCODE_PUSH_INSTVAR_3:
			case BCODE_PUSH_INSTVAR_4:
			case BCODE_PUSH_INSTVAR_5:
			case BCODE_PUSH_INSTVAR_6:
			case BCODE_PUSH_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			push_instvar:
printf ("PUSH_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->origin->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				ACTIVE_STACK_PUSH (stix, ((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1]);
				break;

			/* ------------------------------------------------- */

			case BCODE_STORE_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto store_instvar;
			case BCODE_STORE_INTO_INSTVAR_0:
			case BCODE_STORE_INTO_INSTVAR_1:
			case BCODE_STORE_INTO_INSTVAR_2:
			case BCODE_STORE_INTO_INSTVAR_3:
			case BCODE_STORE_INTO_INSTVAR_4:
			case BCODE_STORE_INTO_INSTVAR_5:
			case BCODE_STORE_INTO_INSTVAR_6:
			case BCODE_STORE_INTO_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			store_instvar:
printf ("STORE_INTO_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1] = ACTIVE_STACK_GETTOP(stix);
				break;

			/* ------------------------------------------------- */
			case BCODE_POP_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto pop_into_instvar;
			case BCODE_POP_INTO_INSTVAR_0:
			case BCODE_POP_INTO_INSTVAR_1:
			case BCODE_POP_INTO_INSTVAR_2:
			case BCODE_POP_INTO_INSTVAR_3:
			case BCODE_POP_INTO_INSTVAR_4:
			case BCODE_POP_INTO_INSTVAR_5:
			case BCODE_POP_INTO_INSTVAR_6:
			case BCODE_POP_INTO_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			pop_into_instvar:
printf ("POP_INTO_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1] = ACTIVE_STACK_GETTOP(stix);
				ACTIVE_STACK_POP (stix);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_TEMPVAR_X:
			case BCODE_STORE_INTO_TEMPVAR_X:
			case BCODE_POP_INTO_TEMPVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto handle_tempvar;

			case BCODE_PUSH_TEMPVAR_0:
			case BCODE_PUSH_TEMPVAR_1:
			case BCODE_PUSH_TEMPVAR_2:
			case BCODE_PUSH_TEMPVAR_3:
			case BCODE_PUSH_TEMPVAR_4:
			case BCODE_PUSH_TEMPVAR_5:
			case BCODE_PUSH_TEMPVAR_6:
			case BCODE_PUSH_TEMPVAR_7:
			case BCODE_STORE_INTO_TEMPVAR_0:
			case BCODE_STORE_INTO_TEMPVAR_1:
			case BCODE_STORE_INTO_TEMPVAR_2:
			case BCODE_STORE_INTO_TEMPVAR_3:
			case BCODE_STORE_INTO_TEMPVAR_4:
			case BCODE_STORE_INTO_TEMPVAR_5:
			case BCODE_STORE_INTO_TEMPVAR_6:
			case BCODE_STORE_INTO_TEMPVAR_7:
			case BCODE_POP_INTO_TEMPVAR_0:
			case BCODE_POP_INTO_TEMPVAR_1:
			case BCODE_POP_INTO_TEMPVAR_2:
			case BCODE_POP_INTO_TEMPVAR_3:
			case BCODE_POP_INTO_TEMPVAR_4:
			case BCODE_POP_INTO_TEMPVAR_5:
			case BCODE_POP_INTO_TEMPVAR_6:
			case BCODE_POP_INTO_TEMPVAR_7:
			{
			#if defined(STIX_USE_CTXTEMPVAR)
				b1 = bcode & 0x7; /* low 3 bits */

			handle_tempvar:
				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on*/
printf ("PUSH_TEMPVAR %d - ", (int)b1);

					ACTIVE_STACK_PUSH (stix, ACTIVE_STACK_GET(stix, b1]));
				}
				else
				{
					/* store or pop - bit 5 off */
					ACTIVE_STACK_SET(stix, b1, ACTIVE_STACK_GETTOP(stix));

					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						ACTIVE_STACK_POP (stix);
printf ("POP_INTO_TEMPVAR %d - ", (int)b1);

					}
else
{
printf ("STORE_INTO_TEMPVAR %d - ", (int)b1);
}
				}

print_object (stix, ctx->slot[bx]);
printf ("\n");
			#else
				stix_oop_context_t ctx;

				b1 = bcode & 0x7; /* low 3 bits */
			handle_tempvar:
				if (stix->active_context->home != stix->_nil)
				{
/*TODO: improve this slow temporary access */
					/* this code assumes that the method context and
					 * the block context place some key fields in the
					 * same offset. such fields include 'home', 'ntmprs' */
					stix_oop_t home;
					stix_ooi_t home_ntmprs;

					ctx = stix->active_context;
					home = ctx->home;

					do
					{
						home_ntmprs = STIX_OOP_TO_SMINT(((stix_oop_context_t)home)->ntmprs);
						if (b1 >= home_ntmprs) break;

						ctx = (stix_oop_context_t)home;
						home = ((stix_oop_context_t)home)->home;
						if (home == stix->_nil)
						{
							home_ntmprs = 0;
							break;
						}
					}
					while (1);

					bx = b1 - home_ntmprs;
				}
				else
				{
					ctx = stix->active_context;
					bx = b1;
				}

				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on*/
printf ("PUSH_TEMPVAR %d - ", (int)b1);

					ACTIVE_STACK_PUSH (stix, ctx->slot[bx]);
				}
				else
				{
					/* store or pop - bit 5 off */
					ctx->slot[bx] = ACTIVE_STACK_GETTOP(stix);

					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						ACTIVE_STACK_POP (stix);
printf ("POP_INTO_TEMPVAR %d - ", (int)b1);

					}
else
{
printf ("STORE_INTO_TEMPVAR %d - ", (int)b1);
}
				}

print_object (stix, ctx->slot[bx]);
printf ("\n");
			#endif
				break;
			}

			/* ------------------------------------------------- */
			case BCODE_PUSH_LITERAL_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto push_literal;

			case BCODE_PUSH_LITERAL_0:
			case BCODE_PUSH_LITERAL_1:
			case BCODE_PUSH_LITERAL_2:
			case BCODE_PUSH_LITERAL_3:
			case BCODE_PUSH_LITERAL_4:
			case BCODE_PUSH_LITERAL_5:
			case BCODE_PUSH_LITERAL_6:
			case BCODE_PUSH_LITERAL_7:
				b1 = bcode & 0x7; /* low 3 bits */
			push_literal:
printf ("PUSH_LITERAL idx=%d - ", (int)b1);
print_object (stix, stix->active_method->slot[b1]);
printf ("\n");
				ACTIVE_STACK_PUSH (stix, stix->active_method->slot[b1]);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_OBJECT_X:
			case BCODE_STORE_INTO_OBJECT_X:
			case BCODE_POP_INTO_OBJECT_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto handle_object;

			case BCODE_PUSH_OBJECT_0:
			case BCODE_PUSH_OBJECT_1:
			case BCODE_PUSH_OBJECT_2:
			case BCODE_PUSH_OBJECT_3:
			case BCODE_STORE_INTO_OBJECT_0:
			case BCODE_STORE_INTO_OBJECT_1:
			case BCODE_STORE_INTO_OBJECT_2:
			case BCODE_STORE_INTO_OBJECT_3:
			case BCODE_POP_INTO_OBJECT_0:
			case BCODE_POP_INTO_OBJECT_1:
			case BCODE_POP_INTO_OBJECT_2:
			case BCODE_POP_INTO_OBJECT_3:
			{
				stix_oop_association_t ass;

				b1 = bcode & 0x3; /* low 2 bits */
			handle_object:
				ass = (stix_oop_association_t)stix->active_method->slot[b1];
				STIX_ASSERT (STIX_CLASSOF(stix, ass) == stix->_association);

				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					ass->value = ACTIVE_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						ACTIVE_STACK_POP (stix);
printf ("POP_INTO_OBJECT %d - ", (int)b1);
					}
else
{
printf ("STORE_INTO_OBJECT %d - ", (int)b1);
}
				}
				else
				{
					/* push */
					ACTIVE_STACK_PUSH (stix, ass->value);
printf ("PUSH_OBJECT %d - ", (int)b1);
				}
				break;
			}

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
printf ("JUMP_FORWARD %d\n", (int)b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
printf ("JUMP_FORWARD %d\n", (int)(bcode & 0x3));
				stix->ip += (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
printf ("JUMP_BACKWARD %d\n", (int)b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
printf ("JUMP_BACKWARD %d\n", (int)(bcode & 0x3));
				stix->ip -= (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_IF_TRUE_X:
			case BCODE_JUMP_IF_FALSE_X:
			case BCODE_JUMP_BY_OFFSET_X:
			case BCODE_JUMP_IF_TRUE_0:
			case BCODE_JUMP_IF_TRUE_1:
			case BCODE_JUMP_IF_TRUE_2:
			case BCODE_JUMP_IF_TRUE_3:
			case BCODE_JUMP_IF_FALSE_0:
			case BCODE_JUMP_IF_FALSE_1:
			case BCODE_JUMP_IF_FALSE_2:
			case BCODE_JUMP_IF_FALSE_3:
			case BCODE_JUMP_BY_OFFSET_0:
			case BCODE_JUMP_BY_OFFSET_1:
			case BCODE_JUMP_BY_OFFSET_2:
			case BCODE_JUMP_BY_OFFSET_3:
printf ("<<<<<<<<<<<<<< JUMP NOT IMPLEMENTED YET >>>>>>>>>>>> \n");
stix->errnum = STIX_ENOIMPL;
return -1;
				break;

			/* -------------------------------------------------------- */

			case BCODE_PUSH_CTXTEMPVAR_X:
			case BCODE_STORE_INTO_CTXTEMPVAR_X:
			case BCODE_POP_INTO_CTXTEMPVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_ctxtempvar;
			case BCODE_PUSH_CTXTEMPVAR_0:
			case BCODE_PUSH_CTXTEMPVAR_1:
			case BCODE_PUSH_CTXTEMPVAR_2:
			case BCODE_PUSH_CTXTEMPVAR_3:
			case BCODE_STORE_INTO_CTXTEMPVAR_0:
			case BCODE_STORE_INTO_CTXTEMPVAR_1:
			case BCODE_STORE_INTO_CTXTEMPVAR_2:
			case BCODE_STORE_INTO_CTXTEMPVAR_3:
			case BCODE_POP_INTO_CTXTEMPVAR_0:
			case BCODE_POP_INTO_CTXTEMPVAR_1:
			case BCODE_POP_INTO_CTXTEMPVAR_2:
			case BCODE_POP_INTO_CTXTEMPVAR_3:
			{
				stix_ooi_t i;
				stix_oop_context_t ctx;

				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_ctxtempvar:

				ctx = stix->active_context;
				STIX_ASSERT ((stix_oop_t)ctx != stix->_nil);
				for (i = 0; i < b1; i++)
				{
					ctx = (stix_oop_context_t)ctx->home;
				}

				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					ctx->slot[b2] = ACTIVE_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						ACTIVE_STACK_POP (stix);
printf ("POP_INTO_CTXTEMPVAR %d %d - ", (int)b1, (int)b2);
					}
else
{
printf ("STORE_INTO_CTXTEMPVAR %d %d - ", (int)b1, (int)b2);
}
				}
				else
				{
					/* push */
					ACTIVE_STACK_PUSH (stix, ctx->slot[b2]);

printf ("PUSH_CTXTEMPVAR %d %d - ", (int)b1, (int)b2);
				}

print_object (stix, ctx->slot[b2]);
printf ("\n");
				break;
			}
			/* -------------------------------------------------------- */

			case BCODE_PUSH_OBJVAR_X:
			case BCODE_STORE_INTO_OBJVAR_X:
			case BCODE_POP_INTO_OBJVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_objvar;

			case BCODE_PUSH_OBJVAR_0:
			case BCODE_PUSH_OBJVAR_1:
			case BCODE_PUSH_OBJVAR_2:
			case BCODE_PUSH_OBJVAR_3:
			case BCODE_STORE_INTO_OBJVAR_0:
			case BCODE_STORE_INTO_OBJVAR_1:
			case BCODE_STORE_INTO_OBJVAR_2:
			case BCODE_STORE_INTO_OBJVAR_3:
			case BCODE_POP_INTO_OBJVAR_0:
			case BCODE_POP_INTO_OBJVAR_1:
			case BCODE_POP_INTO_OBJVAR_2:
			case BCODE_POP_INTO_OBJVAR_3:
			{
				stix_oop_oop_t t;

				/* b1 -> variable index to the object indicated by b2.
				 * b2 -> object index stored in the literal frame. */
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_objvar:
				t = (stix_oop_oop_t)stix->active_method->slot[b2];
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(t) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (b1 < STIX_OBJ_GET_SIZE(t));

				if ((bcode >> 3) & 1)
				{
					/* store or pop */

					t->slot[b1] = ACTIVE_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						ACTIVE_STACK_POP (stix);
printf ("POP_INTO_OBJVAR %d %d - ", (int)b1, (int)b2);
					}
else
{
printf ("STORE_INTO_OBJVAR %d %d - ", (int)b1, (int)b2);
}
				}
				else
				{
					/* push */
printf ("PUSH_OBJVAR %d %d - ", (int)b1, (int)b2);
					ACTIVE_STACK_PUSH (stix, t->slot[b1]);
				}

print_object (stix, t->slot[b1]);
printf ("\n");
				break;
			}

			/* -------------------------------------------------------- */
			case BCODE_SEND_MESSAGE_X:
			case BCODE_SEND_MESSAGE_TO_SUPER_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_send_message;

			case BCODE_SEND_MESSAGE_0:
			case BCODE_SEND_MESSAGE_1:
			case BCODE_SEND_MESSAGE_2:
			case BCODE_SEND_MESSAGE_3:
			case BCODE_SEND_MESSAGE_TO_SUPER_0:
			case BCODE_SEND_MESSAGE_TO_SUPER_1:
			case BCODE_SEND_MESSAGE_TO_SUPER_2:
			case BCODE_SEND_MESSAGE_TO_SUPER_3:
			{
				/* b1 -> number of arguments 
				 * b2 -> index to the selector stored in the literal frame
				 */
				stix_ucs_t mthname;
				stix_oop_t newrcv;
				stix_oop_method_t newmth;
				stix_oop_char_t selector;
				stix_ooi_t preamble;


			handle_send_message:
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)stix->active_method->slot[b2];


printf ("SEND_MESSAGE%s TO RECEIVER AT STACKPOS=%d NARGS=%d SELECTOR=", (((bcode >> 2) & 1)? "_TO_SUPER": ""), (int)(stix->sp - b1), (int)b1);
print_object (stix, (stix_oop_t)selector);
fflush (stdout);
				STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);

				newrcv = ACTIVE_STACK_GET(stix, stix->sp - b1);
printf (" RECEIVER = ");
print_object(stix, newrcv);
printf ("\n");
				mthname.ptr = selector->slot;
				mthname.len = STIX_OBJ_GET_SIZE(selector);
				newmth = find_method (stix, newrcv, &mthname, ((bcode >> 2) & 1));
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
						ACTIVE_STACK_POPS (stix, b1); /* pop arguments only*/
						break;

					case STIX_METHOD_PREAMBLE_RETURN_INSTVAR:
					{
						stix_oop_oop_t rcv;

						ACTIVE_STACK_POPS (stix, b1); /* pop arguments only */

printf ("RETURN INSTVAR AT PREAMBLE index %d\n", (int)STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
						/* replace the receiver by an instance variable of the receiver */
						rcv = (stix_oop_oop_t)ACTIVE_STACK_GETTOP(stix);
						STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(rcv) == STIX_OBJ_TYPE_OOP);
						STIX_ASSERT (STIX_OBJ_GET_SIZE(rcv) > STIX_METHOD_GET_PREAMBLE_INDEX(preamble));

						if (rcv == (stix_oop_oop_t)stix->active_context)
						{
							/* the active context object doesn't keep
							 * the most up-to-date information in the 
							 * 'ip' and 'sp' field. commit these fields
							 * when the object to be accessed is 
							 * the active context. this manual commit
							 * is required because this premable handling
							 * skips activation of a new method context
							 * that would commit these fields. 
							 */
							STORE_ACTIVE_IP (stix);
							STORE_ACTIVE_SP (stix);
						}

						/* this accesses the instance variable of the receiver */
						ACTIVE_STACK_SET (stix, stix->sp, rcv->slot[STIX_METHOD_GET_PREAMBLE_INDEX(preamble)]);
						break;
					}

					case STIX_METHOD_PREAMBLE_PRIMITIVE:
					{
						stix_ooi_t prim_no;

						prim_no = STIX_METHOD_GET_PREAMBLE_INDEX(preamble);
						if (prim_no >= 0 && prim_no < STIX_COUNTOF(primitives) && 
						    (primitives[prim_no].nargs < 0 || primitives[prim_no].nargs == b1))
						{
							int n;

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

			case BCODE_PUSH_RECEIVER:
printf ("PUSH_RECEIVER %p TO STACK INDEX %d\n", stix->active_context->origin->receiver_or_source, (int)stix->sp);
				ACTIVE_STACK_PUSH (stix, stix->active_context->origin->receiver_or_source);
				break;

			case BCODE_PUSH_NIL:
printf ("PUSH_NIL\n");
				ACTIVE_STACK_PUSH (stix, stix->_nil);
				break;

			case BCODE_PUSH_TRUE:
printf ("PUSH_TRUE\n");
				ACTIVE_STACK_PUSH (stix, stix->_true);
				break;

			case BCODE_PUSH_FALSE:
printf ("PUSH_FALSE\n");
				ACTIVE_STACK_PUSH (stix, stix->_false);
				break;

			case BCODE_PUSH_CONTEXT:
printf ("PUSH_CONTEXT\n");
				ACTIVE_STACK_PUSH (stix, (stix_oop_t)stix->active_context);
				break;

			case BCODE_PUSH_NEGONE:
printf ("PUSH_NEGONE\n");
				ACTIVE_STACK_PUSH (stix, STIX_OOP_FROM_SMINT(-1));
				break;

			case BCODE_PUSH_ZERO:
printf ("PUSH_ZERO\n");
				ACTIVE_STACK_PUSH (stix, STIX_OOP_FROM_SMINT(0));
				break;

			case BCODE_PUSH_ONE:
printf ("PUSH_ONE\n");
				ACTIVE_STACK_PUSH (stix, STIX_OOP_FROM_SMINT(1));
				break;

			case BCODE_PUSH_TWO:
printf ("PUSH_TWO\n");
				ACTIVE_STACK_PUSH (stix, STIX_OOP_FROM_SMINT(2));
				break;

			/* -------------------------------------------------------- */

			case BCODE_DUP_STACKTOP:
			{
				stix_oop_t t;
printf ("DUP_STACKTOP SP=%d\n", (int)stix->sp);
				STIX_ASSERT (!ACTIVE_STACK_ISEMPTY(stix));
				t = ACTIVE_STACK_GETTOP(stix);
				ACTIVE_STACK_PUSH (stix, t);
				break;
			}

			case BCODE_POP_STACKTOP:
printf ("POP_STACKTOP\n");
				STIX_ASSERT (!ACTIVE_STACK_ISEMPTY(stix));
				ACTIVE_STACK_POP (stix);
				break;

			case BCODE_RETURN_STACKTOP:
printf ("RETURN_STACKTOP\n");
				return_value = ACTIVE_STACK_GETTOP(stix);
				ACTIVE_STACK_POP (stix);
				goto handle_return;

			case BCODE_RETURN_RECEIVER:
printf ("RETURN_RECEIVER\n");
				return_value = stix->active_context->origin->receiver_or_source;
			handle_return:
printf ("<<LEAVING>> SP=%d\n", (int)stix->sp);

				/* put the instruction pointer back to the return
				 * instruction (RETURN_RECEIVER or RETURN_RECEIVER)
				 * if a context returns into this context again,
				 * it'll be able to return as well again.
				 * 
				 * Consider a program like this:
				 *
				 * #class MyObject(Object)
				 * {
				 *   #declare(#classinst) t1 t2.
				 *   #method(#class) xxxx
				 *   {
				 *     | g1 g2 |
				 *     t1 dump.
				 *     t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
				 *     (t1 < 100) ifFalse: [ ^self ].
				 *     t1 := t1 + 1. 
				 *     ^self xxxx.
				 *   }
				 *   #method(#class) main
				 *   {
				 *     t1 := 1.
				 *     self xxxx.
				 *     t2 := t2 value.  
				 *     t2 dump.
				 *   }
				 * }
				 *
				 * the 'xxxx' method invoked by 'self xxxx' has 
				 * returned even before 't2 value' is executed.
				 * the '^' operator makes the active context to
				 * switch to its 'origin->sender' which is the
				 * method context of 'xxxx' itself. placing its
				 * instruction pointer at the 'return' instruction
				 * helps execute another return when the switching
				 * occurs.
				 * 
				 * TODO: verify if this really works
				 *
				 */
				stix->ip--; 

				SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->origin->sender);

				/* push the return value to the stack of the new active context */
				ACTIVE_STACK_PUSH (stix, return_value);

				if (stix->active_context->sender == stix->_nil)
				{
					/* the sending context of the intial context has been set to nil.
					 * use this fact to tell an initial context from a normal context. */
					STIX_ASSERT (stix->active_context->receiver_or_source == stix->_nil);

printf ("<<<RETURNIGN TO THE INITIAL CONTEXT>>>\n");
					STIX_ASSERT (stix->sp == 0);
					goto done;
				}

				break;

			case BCODE_RETURN_FROM_BLOCK:
printf ("LEAVING_BLOCK\n");
				STIX_ASSERT(STIX_CLASSOF(stix, stix->active_context)  == stix->_block_context);

				return_value = ACTIVE_STACK_GETTOP(stix);
				SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->sender);
				ACTIVE_STACK_PUSH (stix, return_value);
				break;

			case BCODE_SEND_BLOCK_COPY:
			{
				stix_ooi_t nargs, ntmprs;
				stix_oop_context_t rctx;
				stix_oop_context_t blkctx;
printf ("SEND_BLOCK_COPY\n");
				/* it emulates thisContext blockCopy: nargs ofTmprCount: ntmprs */
				STIX_ASSERT (stix->sp >= 2);

				STIX_ASSERT (STIX_CLASSOF(stix, ACTIVE_STACK_GETTOP(stix)) == stix->_small_integer);
				ntmprs = STIX_OOP_TO_SMINT(ACTIVE_STACK_GETTOP(stix));
				ACTIVE_STACK_POP (stix);

				STIX_ASSERT (STIX_CLASSOF(stix, ACTIVE_STACK_GETTOP(stix)) == stix->_small_integer);
				nargs = STIX_OOP_TO_SMINT(ACTIVE_STACK_GETTOP(stix));
				ACTIVE_STACK_POP (stix);

				STIX_ASSERT (nargs >= 0);
				STIX_ASSERT (ntmprs >= nargs);

				/* the block context object created here is used
				 * as a base object for block context activation.
				 * primitive_block_context_value() clones a block 
				 * context and activates the cloned context.
				 * this base block context is created with no 
				 * stack for this reason. */
				blkctx = (stix_oop_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 0); 
				if (!blkctx) return -1;

				/* get the receiver to the block copy message after block context instantiation
				 * not to get affected by potential GC */
				rctx = (stix_oop_context_t)ACTIVE_STACK_GETTOP(stix);

				/* [NOTE]
				 *  blkctx->caller is left to nil. it is set to the 
				 *  active context before it gets activated. see
				 *  primitive_block_context_value().
				 *
				 *  blkctx->home is set here to the active context.
				 *  it's redundant to have them pushed to the stack
				 *  though it is to emulate the message sending of
				 *  blockCopy:withNtmprs:. 
				 *  TODO: devise a new byte code to eliminate stack pushing.
				 *
				 *  blkctx->origin is set here by copying the origin
				 *  of the active context.
				 */

				/* the extended jump instruction has the format of 
				 *   0000XXXX KKKKKKKK or 0000XXXX KKKKKKKK KKKKKKKK 
				 * depending on STIX_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
				 * the instruction after the jump. */
				blkctx->ip = STIX_OOP_FROM_SMINT(stix->ip + STIX_BCODE_LONG_PARAM_SIZE + 1);
				blkctx->sp = STIX_OOP_FROM_SMINT(-1);
				/* the number of arguments for a block context is local to the block */
				blkctx->method_or_nargs = STIX_OOP_FROM_SMINT(nargs);
				/* the number of temporaries here is an accumulated count including
				 * the number of temporaries of a home context */
				blkctx->ntmprs = STIX_OOP_FROM_SMINT(ntmprs);

				blkctx->home = (stix_oop_t)rctx;
				blkctx->receiver_or_source = stix->_nil;

#if 0
				if (rctx->home == stix->_nil)
				{
					/* the context that receives the blockCopy message is a method context */
					STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_method_context);
					STIX_ASSERT (rctx == (stix_oop_t)stix->active_context);
					blkctx->origin = (stix_oop_context_t)rctx;
				}
				else
				{
					/* a block context is active */
					STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_block_context);
					blkctx->origin = ((stix_oop_block_context_t)rctx)->origin;
				}
#else

				/* [NOTE]
				 * the origin of a method context is set to itself
				 * when it's created. so it's safe to simply copy
				 * the origin field this way.
				 */
				blkctx->origin = rctx->origin;
#endif

				ACTIVE_STACK_SETTOP (stix, (stix_oop_t)blkctx);
				break;
			}

			case BCODE_NOOP:
				/* do nothing */
				break;


			default:
printf ("UNKNOWN BYTE CODE ENCOUNTERED %x\n", (int)bcode);
				stix->errnum = STIX_EINTERN;
				break;

		}
	}

done:
	printf ("TOTAL_INST_COUTNER = %lu\n", (unsigned long int)inst_counter);
	return 0;


oops:
	/* TODO: anything to do here? */
	return -1;
}

int stix_invoke (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}
