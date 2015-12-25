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

#if defined(USE_DYNCALL)
/* TODO: defined dcAllocMem and dcFreeMeme before builing the dynload and dyncall library */
#	include <dyncall.h> /* TODO: remove this. make dyXXXX calls to callbacks */
#endif

/* TODO: context's stack overflow check in various part of this file */
/* TOOD: determine the right stack size */
#define CONTEXT_STACK_SIZE 96

#define LOAD_IP(stix, v_ctx) ((stix)->ip = STIX_OOP_TO_SMOOI((v_ctx)->ip))
#define STORE_IP(stix, v_ctx) ((v_ctx)->ip = STIX_SMOOI_TO_OOP((stix)->ip))

#define LOAD_SP(stix, v_ctx) ((stix)->sp = STIX_OOP_TO_SMOOI((v_ctx)->sp))
#define STORE_SP(stix, v_ctx) ((v_ctx)->sp = STIX_SMOOI_TO_OOP((stix)->sp))

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
		SET_ACTIVE_METHOD_CODE(stix); \
		LOAD_ACTIVE_IP (stix); \
		LOAD_ACTIVE_SP (stix); \
	} while (0) \

#define FETCH_BYTE_CODE(stix) ((stix)->active_code[(stix)->ip++])
#define FETCH_BYTE_CODE_TO(stix, v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
#define FETCH_PARAM_CODE_TO(stix, v_ooi) \
	do { \
		v_ooi = FETCH_BYTE_CODE(stix); \
		v_ooi = (v_ooi << 8) | FETCH_BYTE_CODE(stix); \
	} while (0)
#else
#define FETCH_PARAM_CODE_TO(stix, v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#endif


#if defined(STIX_DEBUG_EXEC)
#	define DBGOUT_EXEC_0(fmt) printf(fmt "\n")
#	define DBGOUT_EXEC_1(fmt,a1) printf(fmt "\n",a1)
#	define DBGOUT_EXEC_2(fmt,a1,a2) printf(fmt "\n", a1, a2)
#	define DBGOUT_EXEC_3(fmt,a1,a2,a3) printf(fmt "\n", a1, a2, a3)
#else
#	define DBGOUT_EXEC_0(fmt)
#	define DBGOUT_EXEC_1(fmt,a1)
#	define DBGOUT_EXEC_2(fmt,a1,a2)
#	define DBGOUT_EXEC_3(fmt,a1,a2,a3)
#endif

static stix_oop_process_t make_process (stix_t* stix, stix_oop_context_t c)
{
	stix_oop_process_t proc;

/* TODO: do something about the stack. */
	stix_pushtmp (stix, (stix_oop_t*)&c);
	proc = (stix_oop_process_t)stix_instantiate (stix, stix->_process, STIX_NULL, stix->option.dfl_procstk_size);
	stix_poptmp (stix);
	if (!proc) return STIX_NULL;

	proc->state = STIX_SMOOI_TO_OOP(0);
	proc->initial_context = c;

	return proc;
}

static void switch_process (stix_t* stix, stix_oop_process_t proc)
{
	if (stix->processor->active != proc)
	{
#if defined(STIX_DEBUG_PROCESSOR)
printf ("ACTUAL PROCESS SWITCHING BF...%d %p\n", (int)stix->ip, stix->active_context);
#endif

/* store the active context to the active process */
STIX_ASSERT ((stix_oop_t)stix->processor->active != stix->_nil);
stix->processor->active->active_context = stix->active_context;

		SWITCH_ACTIVE_CONTEXT (stix, proc->active_context);
#if defined(STIX_DEBUG_PROCESSOR)
printf ("ACTUAL PROCESS SWITCHING AF...%d %p\n", (int)stix->ip, stix->active_context);
#endif
		/*TODO: set the state to RUNNING */
		stix->processor->active = proc;
	}
}

static void switch_to_next_process (stix_t* stix)
{
/* TODO: this is experimental. rewrite it */
	if ((stix_oop_t)stix->processor->active->next == stix->_nil)
	{
#if defined(STIX_DEBUG_PROCESSOR)
printf ("SWITCHING TO THE HEAD PROCESS\n");
#endif
		switch_process (stix, stix->processor->head);
	}
	else
	{
#if defined(STIX_DEBUG_PROCESSOR)
printf ("SWITCHING TO THE NEXT PROCESS\n");
#endif
		switch_process (stix, stix->processor->active->next);
	}
}

static STIX_INLINE int register_new_process (stix_t* stix, stix_oop_process_t proc)
{
	/* the process is not scheduled at all. 
	 * link it to the processor's process list. */
	stix_ooi_t tally;

	STIX_ASSERT (proc->state == STIX_SMOOI_TO_OOP(0));
	STIX_ASSERT ((stix_oop_t)proc->prev == stix->_nil);
	STIX_ASSERT ((stix_oop_t)proc->next == stix->_nil);
	STIX_ASSERT ((stix_oop_t)proc->active_context == stix->_nil);

	tally = STIX_OOP_TO_SMOOI(stix->processor->tally);
	if (tally <= 0)
	{
		/* the process schedule has no process.
		 * it is the first process */
		stix->processor->head = proc;
		stix->processor->tail = proc;
		stix->processor->tally  = STIX_SMOOI_TO_OOP(1);
#if defined(STIX_DEBUG_PROCESSOR)
printf ("ADD NEW PROCESS X - %d\n", (int)1);
#endif
	}
	else if (tally >= STIX_SMOOI_MAX)
	{
#if defined(STIX_DEBUG_PROCESSOR)
printf ("TOO MANY PROCESS\n");
#endif
		stix->errnum = STIX_EPFULL;
		return -1;
	}
	else
	{
		proc->next = stix->processor->head;
		stix->processor->head->prev = proc;
		stix->processor->head = proc;
		stix->processor->tally = STIX_SMOOI_TO_OOP(tally + 1);
#if defined(STIX_DEBUG_PROCESSOR)
printf ("ADD NEW PROCESS Y - %d\n", (int)tally + 1);
#endif
	}

	proc->state = STIX_SMOOI_TO_OOP(1); /* TODO: change the code properly... changing state alone doesn't help */
	proc->active_context = proc->initial_context;
	return 0;
}

static int schedule_process (stix_t* stix, stix_oop_process_t proc)
{
	if (proc->state == STIX_SMOOI_TO_OOP(0))
	{
		/* the process is not scheduled at all. it must not exist in the
		 * process list of the process scheduler. */
		if (register_new_process (stix, proc) <= -1) return -1;
		switch_process (stix, proc);
	}
	else if (stix->processor->active != proc)
	{
		switch_process (stix, proc);
	}

	return 0;
}

#if 0
static stix_oop_process_t start_new_process (stix_t* stix, stix_oop_context_t c)
{
	stix_oop_process_t proc;

	proc = make_process (stix, c);
	if (!proc) return STIX_NULL;

	if (schedule_process (stix, proc) <= -1) return STIX_NULL;
	return proc;
}
#endif

static stix_oop_process_t start_initial_process (stix_t* stix, stix_oop_context_t c)
{
	stix_oop_process_t proc;

	proc = make_process (stix, c);
	if (!proc) return STIX_NULL;

	if (register_new_process (stix, proc) <= -1) return STIX_NULL;

	/* do somthing that schedule_process() would do with less overhead */
	STIX_ASSERT ((stix_oop_t)proc->active_context != stix->_nil);
	STIX_ASSERT (proc->active_context == proc->initial_context);
	SWITCH_ACTIVE_CONTEXT (stix, proc->active_context);
	/*TODO: set the state to RUNNING */
	stix->processor->active = proc;
	
	return proc;
}

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
	ntmprs = STIX_OOP_TO_SMOOI(mth->tmpr_count);
	nargs = STIX_OOP_TO_SMOOI(mth->tmpr_nargs);

	STIX_ASSERT (ntmprs >= 0);
	STIX_ASSERT (nargs <= ntmprs);
	STIX_ASSERT (stix->sp >= 0);
	STIX_ASSERT (stix->sp >= nargs);

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, CONTEXT_STACK_SIZE);
	stix_poptmp (stix);
	if (!ctx) return -1;

	ctx->sender = (stix_oop_t)stix->active_context; 
	ctx->ip = STIX_SMOOI_TO_OOP(0);
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
	ctx->sp = STIX_SMOOI_TO_OOP(ntmprs - 1);
	ctx->ntmprs = STIX_SMOOI_TO_OOP(ntmprs);
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

#if defined(STIX_DEBUG_EXEC)
printf ("<<ENTERING>> SP=%d\n", (int)stix->sp);
#endif
	return 0;

#if 0
reuse_context:
	/* force the class to become a method context */
	ctx->_class = stix->_method_context;

	ctx->receiver_or_source = ACTIVE_STACK_GET(stix, stix->sp - nargs);
#if defined(STIX_DEBUG_EXEC)
printf ("####### REUSING CONTEXT INSTEAD OF <<ENTERING>> WITH RECEIVER ");
print_object (stix, ctx->receiver_or_source);
printf ("\n");
#endif

	for (i = 0; i < nargs; i++)
	{
		ctx->slot[i] = ACTIVE_STACK_GET (stix, stix->sp - nargs + i + 1);
#if defined(STIX_DEBUG_EXEC)
printf ("REUSING ARGUMENT %d - ", (int)i);
print_object (stix, ctx->slot[i]);
printf ("\n");
#endif
	}
	for (; i <= stix->sp; i++) ctx->slot[i] = stix->_nil;
	/* keep the sender 
	ctx->sender = 
	*/
	
	ctx->ntmprs = STIX_SMOOI_TO_OOP(ntmprs);
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

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t receiver, const stix_oocs_t* message, int super)
{
	stix_oop_class_t cls;
	stix_oop_association_t ass;
	stix_oop_t c;
	stix_oop_set_t mthdic;
	int dic_no;
/* TODO: implement method lookup cache */

#if defined(STIX_DEBUG_EXEC)
printf ("==== FINDING METHOD FOR %p [", receiver);
print_oocs (message);
printf ("] in ");
#endif

	cls = (stix_oop_class_t)STIX_CLASSOF(stix, receiver);
	if ((stix_oop_t)cls == stix->_class)
	{
		/* receiver is a class object */
		c = receiver; 
		dic_no = STIX_CLASS_MTHDIC_CLASS;
#if defined(STIX_DEBUG_EXEC)
printf ("class method dictioanry of ");
print_object(stix, (stix_oop_t)((stix_oop_class_t)c)->name); 
printf ("\n");
#endif
	}
	else
	{
		c = (stix_oop_t)cls;
		dic_no = STIX_CLASS_MTHDIC_INSTANCE;
#if defined(STIX_DEBUG_EXEC)
printf ("instance method dictioanry of ");
print_object(stix, (stix_oop_t)((stix_oop_class_t)c)->name);
printf ("\n");
#endif
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

static int activate_initial_context (stix_t* stix, const stix_oocs_t* objname, const stix_oocs_t* mthname)
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
	stix_oop_process_t proc;

	/* create a fake initial context */
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, 1);
	if (!ctx) return -1;

	ass = stix_lookupsysdic (stix, objname);
	if (!ass) return -1;

	mth = find_method (stix, ass->value, mthname, 0);
	if (!mth) return -1;

	if (STIX_OOP_TO_SMOOI(mth->tmpr_nargs) > 0)
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
	 * TODO: verify this theory of mine. */
	stix->ip = 0;
	stix->sp = -1;

	ctx->origin = ctx;
	ctx->method_or_nargs = (stix_oop_t)mth; /* fake. help SWITCH_ACTIVE_CONTEXT() not fail*/

	/* [NOTE]
	 *  the receiver field and the sender field of ctx are nils.
	 *  especially, the fact that the sender field is nil is used by 
	 *  the main execution loop for breaking out of the loop */

	STIX_ASSERT (stix->active_context == STIX_NULL);
	/* i can't use SWITCH_ACTIVE_CONTEXT() macro as there is no active 
	 * context before switching. let's force set active_context to ctx
	 * directly. */
	stix->active_context = ctx;
	ACTIVE_STACK_PUSH (stix, ass->value); /* push the receiver */

	STORE_ACTIVE_IP (stix); /* stix->active_context->ip = STIX_SMOOI_TO_OOP(stix->ip) */
	STORE_ACTIVE_SP (stix); /* stix->active_context->sp = STIX_SMOOI_TO_OOP(stix->sp) */

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	/* call start_initial_process() intead of start_new_process() */
	proc = start_initial_process (stix, ctx); 
	stix_poptmp (stix);
	if (!proc) return -1;

	return activate_new_method (stix, mth);
}

/* ------------------------------------------------------------------------- */
static int prim_dump (stix_t* stix, stix_ooi_t nargs)
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

static int prim_identical (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, b;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	b = (rcv == arg)? stix->_true: stix->_false;

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, b);
	return 1;
}

static int prim_not_identical (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, b;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	b = (rcv != arg)? stix->_true: stix->_false;

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, b);
	return 1;
}

static int prim_class (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, c;

	STIX_ASSERT (nargs ==  0);
	rcv = ACTIVE_STACK_GETTOP (stix);
	c = STIX_CLASSOF(stix, rcv);
	ACTIVE_STACK_SETTOP (stix, c);
	return 1; /* success */
}

static int prim_basic_new (stix_t* stix, stix_ooi_t nargs)
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

static int prim_basic_new_with_size (stix_t* stix, stix_ooi_t nargs)
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
	if (STIX_OOP_IS_SMOOI(szoop))
	{
		size = STIX_OOP_TO_SMOOI(szoop);
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

static int prim_shallow_copy (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, obj;

	STIX_ASSERT (nargs ==  0);

	rcv = ACTIVE_STACK_GETTOP (stix);

	obj = stix_shallowcopy (stix, rcv);
	if (!obj) return -1;

	/* emulate 'pop receiver' and 'push result' */
	ACTIVE_STACK_SETTOP (stix, obj);
	return 1; /* success */
}

static int prim_basic_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = ACTIVE_STACK_GETTOP(stix);
	ACTIVE_STACK_SETTOP(stix, STIX_SMOOI_TO_OOP(STIX_OBJ_GET_SIZE(rcv)));
/* TODO: use LargeInteger if the size is very big */
	return 1;
}

static int prim_basic_at (stix_t* stix, stix_ooi_t nargs)
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
	if (!STIX_OOP_IS_SMOOI(pos))
	{
/* TODO: handle LargeInteger */
		/* the position must be an integer */
		return 0;
	}

	idx = STIX_OOP_TO_SMOOI(pos);
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
			v = STIX_SMOOI_TO_OOP(((stix_oop_byte_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_CHAR:
			v = STIX_CHAR_TO_OOP(((stix_oop_char_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_HALFWORD:
			/* TODO: LargeInteger if the halfword is too large */
			v = STIX_SMOOI_TO_OOP(((stix_oop_halfword_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: LargeInteger if the word is too large */
			v = STIX_SMOOI_TO_OOP(((stix_oop_word_t)rcv)->slot[idx]);
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

static int prim_basic_at_put (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, val;
	stix_ooi_t idx;

	STIX_ASSERT (nargs == 2);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 2);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = ACTIVE_STACK_GET(stix, stix->sp - 1);
	if (!STIX_OOP_IS_SMOOI(pos))
	{
/* TODO: handle LargeInteger */
		/* the position must be an integer */
		return 0;
	}

	val = ACTIVE_STACK_GET(stix, stix->sp);

	idx = STIX_OOP_TO_SMOOI(pos);
	if (idx < 1 || idx > STIX_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		return 0;
	}

	if (STIX_OBJ_GET_CLASS(rcv) == stix->_symbol)
	{
/* TODO: disallow change of some key kernel objects???? */
		/* TODO: is it better to introduct a read-only mark in the object header instead of this class check??? */
		/* read-only object */ /* TODO: DEVISE A WAY TO PASS a proper error from the primitive handler to STIX */
		return 0;
	}

	/* [NOTE] basicAt: and basicAt:put: used a 1-based index. */
	idx = idx - 1;

	switch (STIX_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case STIX_OBJ_TYPE_BYTE:
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a character */
				return 0;
			}
/* TOOD: must I check the range of the value? */
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
			break;

		case STIX_OBJ_TYPE_CHAR:
			if (!STIX_OOP_IS_CHAR(val))
			{
				/* the value is not a character */
				return 0;
			}
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_CHAR(val);
			break;

		case STIX_OBJ_TYPE_HALFWORD:
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a number */
				return 0;
			}

			/* if the small integer is too large, it will get truncated */
			((stix_oop_halfword_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: handle largeINteger  */
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a number */
				return 0;
			}
			((stix_oop_word_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
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

static int __block_value (stix_t* stix, stix_ooi_t nargs, stix_ooi_t num_first_arg_elems, stix_oop_context_t* pblkctx)
{
	stix_oop_context_t blkctx, org_blkctx;
	stix_ooi_t local_ntmprs, i;
	stix_ooi_t actual_arg_count;


	actual_arg_count = (num_first_arg_elems > 0)? num_first_arg_elems: nargs;

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
#if defined(STIX_DEBUG_EXEC)
printf ("PRIMITVE VALUE RECEIVER IS NOT A BLOCK CONTEXT\n");
#endif
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
#if defined(STIX_DEBUG_EXEC)
printf ("PRIM REVALUING AN BLOCKCONTEXT\n");
#endif
		return 0;
	}
	STIX_ASSERT (STIX_OBJ_GET_SIZE(org_blkctx) == STIX_CONTEXT_NAMED_INSTVARS);

	if (STIX_OOP_TO_SMOOI(org_blkctx->method_or_nargs) != actual_arg_count /* nargs */)
	{
		/* the number of argument doesn't match */
#if defined(STIX_DEBUG_EXEC)
/* TODO: better handling of primitive failure */
printf ("PRIM BlockContext value FAIL - NARGS MISMATCH\n");
#endif
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
#if defined(STIX_DEBUG_EXEC)
printf ("~~~~~~~~~~ BLOCK VALUING %p TO NEW BLOCK %p\n", org_blkctx, blkctx);
#endif
#endif

/* TODO: check the stack size of a block context to see if it's large enough to hold arguments */
	if (num_first_arg_elems > 0)
	{
		stix_oop_oop_t xarg;
		STIX_ASSERT (nargs == 1);
		xarg = (stix_oop_oop_t)ACTIVE_STACK_GETTOP (stix);
		STIX_ASSERT (STIX_ISTYPEOF(stix,xarg,STIX_OBJ_TYPE_OOP)); 
		STIX_ASSERT (STIX_OBJ_GET_SIZE(xarg) == num_first_arg_elems); 
		for (i = 0; i < num_first_arg_elems; i++)
		{
			blkctx->slot[i] = xarg->slot[i];
		}
	}
	else
	{
		/* copy the arguments to the stack */
		for (i = 0; i < nargs; i++)
		{
			blkctx->slot[i] = ACTIVE_STACK_GET(stix, stix->sp - nargs + i + 1);
		}
	}
	ACTIVE_STACK_POPS (stix, nargs + 1); /* pop arguments and receiver */
	

	STIX_ASSERT (blkctx->home != stix->_nil);

	/* the number of temporaries stored in the block context
	 * accumulates the number of temporaries starting from the origin.
	 * simple calculation is needed to find the number of local temporaries */
	local_ntmprs = STIX_OOP_TO_SMOOI(blkctx->ntmprs) -
	               STIX_OOP_TO_SMOOI(((stix_oop_context_t)blkctx->home)->ntmprs);
	STIX_ASSERT (local_ntmprs >= nargs);

	blkctx->sp = STIX_SMOOI_TO_OOP(local_ntmprs);
	blkctx->sender = (stix_oop_t)stix->active_context;

	*pblkctx = blkctx;
	return 1;
}

static int prim_block_value (stix_t* stix, stix_ooi_t nargs)
{
	int x;
	stix_oop_context_t blkctx;

	x = __block_value (stix, nargs, 0, &blkctx);
	if (x <= 0) return x; /* hard failure and soft failure */

#if defined(STIX_DEBUG_EXEC)
printf ("<<ENTERING BLOCK>>\n");
#endif
	SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)blkctx);
	return 1;
}

static int prim_block_new_process (stix_t* stix, stix_ooi_t nargs)
{
	int x;
	stix_oop_context_t blkctx;
	stix_oop_process_t proc;
	stix_ooi_t num_first_arg_elems = 0;

	if (nargs > 1)
	{
		/* too many arguments */
/* TODO: proper error handling */
		return 0;
	}

	if (nargs == 1)
	{
		stix_oop_t xarg;

		xarg = ACTIVE_STACK_GETTOP(stix);
		if (!STIX_ISTYPEOF(stix,xarg,STIX_OBJ_TYPE_OOP))
		{
			return 0;
		}

		num_first_arg_elems = STIX_OBJ_GET_SIZE(xarg);
	}

	/* this primitive creates a new process with a block as if the block
	 * is sent the value message */
	x = __block_value (stix, nargs, num_first_arg_elems, &blkctx);
	if (x <= 0) return x; /* hard failure and soft failure */

	proc = make_process (stix, blkctx);
	if (!proc) return -1; /* hard failure */ /* TOOD: can't this be a soft failure? */

	/* __block_value() has popped all arguments and the receiver. 
	 * PUSH the return value instead of changing the stack top */
	ACTIVE_STACK_PUSH (stix, (stix_oop_t)proc);
	return 1;

#if 0
	stix_oop_process_t proc;
	stix_oop_context_t rcv;

	rcv = (stix_oop_context_t)ACTIVE_STACK_GETTOP(stix);
	if (STIX_CLASSOF(stix, rcv) != stix->_block_context)
	{
#if defined(STIX_DEBUG_EXEC)
printf ("PRIMITVE VALUE RECEIVER IS NOT A BLOCK CONTEXT\n");
#endif
		return 0;
	}

	proc = make_process (stix, rcv);
	if (!proc) return -1; /* hard failure */ /* TOOD: can't this be a soft failure? */

	ACTIVE_STACK_SETTOP (stix, (stix_oop_t)proc);
	return 1;
#endif
}

static int prim_integer_add (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_addints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_sub (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_subints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_mul (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_mulints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_quo (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	quo = stix_divints (stix, rcv, arg, 0, STIX_NULL);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, quo);
	return 1;
}

static int prim_integer_rem (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo, rem;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	quo = stix_divints (stix, rcv, arg, 0, &rem);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, rem);
	return 1;
}

static int prim_integer_quo2 (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	quo = stix_divints (stix, rcv, arg, 1, STIX_NULL);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, quo);
	return 1;
}

static int prim_integer_rem2 (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo, rem;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	quo = stix_divints (stix, rcv, arg, 1, &rem);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, rem);
	return 1;
}

static int prim_integer_negated (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, res;

	STIX_ASSERT (nargs == 0);

	rcv = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_negateint (stix, rcv);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitat (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 2);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitatint (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitand (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitandints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitor (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitorints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitxor (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitxorints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitinv (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, res;

	STIX_ASSERT (nargs == 0);

	rcv = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitinvint (stix, rcv);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_bitshift (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	res = stix_bitshiftint (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	ACTIVE_STACK_POP (stix);
	ACTIVE_STACK_SETTOP (stix, res);
	return 1;
}

static int prim_integer_eq (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) == STIX_OOP_TO_SMOOI(arg))
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

static int prim_integer_ne (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) != STIX_OOP_TO_SMOOI(arg))
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
static int prim_integer_lt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) < STIX_OOP_TO_SMOOI(arg))
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

static int prim_integer_gt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) > STIX_OOP_TO_SMOOI(arg))
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

static int prim_integer_le (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) <= STIX_OOP_TO_SMOOI(arg))
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

static int prim_integer_ge (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (STIX_OOP_IS_SMOOI(rcv) && STIX_OOP_IS_SMOOI(arg))
	{
		ACTIVE_STACK_POP (stix);
		if (STIX_OOP_TO_SMOOI(rcv) >= STIX_OOP_TO_SMOOI(arg))
		{
			ACTIVE_STACK_SETTOP (stix, stix->_true);
		}
		else
		{
			ACTIVE_STACK_SETTOP (stix, stix->_false);
		}
		
		return 1;
	}

	return 0;
}

static int prim_integer_inttostr (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, str;
	stix_ooi_t radix;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(arg)) return 0; /* soft failure */
	radix = STIX_OOP_TO_SMOOI(arg);

	if (radix < 2 || radix > 36) return 0; /* soft failure */
	str = stix_inttostr (stix, rcv, radix);
	if (!str) return (stix->errnum == STIX_EINVAL? 0: -1);

	ACTIVE_STACK_SETTOP (stix, str);
	return 1;
}

static int prim_processor_schedule (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (rcv != (stix_oop_t)stix->processor || STIX_CLASSOF(stix,arg) != stix->_process)
	{
		return 0;
	}

	if (!schedule_process (stix, (stix_oop_process_t)arg)) 
	{
/* TODO: Can this be a soft failure? */
		return (stix->errnum == STIX_EPFULL)? 0: -1;
	}

	return 1;
}

static int prim_processor_remove (stix_t* stix, stix_ooi_t nargs)
{
/* TODO: */
	return 0;
}

static int prim_processor_sleep (stix_t* stix, stix_ooi_t nargs)
{
/* TODO: */
	return 0;
}


static int prim_ffi_open (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;
	void* handle;

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (!STIX_ISTYPEOF(stix, arg, STIX_OBJ_TYPE_CHAR))
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!stix->vmprim.mod_open)
	{
		/* TODO: more info on error */
		return 0;
	}


/* TODO: check null-termination... */
	handle = stix->vmprim.mod_open (stix, ((stix_oop_char_t)arg)->slot);
	if (!handle)
	{
		/* TODO: more info on error */
		return 0;
	}

	ACTIVE_STACK_POP (stix);
/* TODO: how to hold an address? as an integer????  or a byte array? fix this not to loose accuracy*/
	ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(handle));

	return 1;
}

static int prim_ffi_close (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;
	void* handle;

	STIX_ASSERT (nargs == 1);

	STIX_ASSERT (nargs == 1);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 1);
	arg = ACTIVE_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(arg))
	{
		/* TODO: more info on error */
		return 0;
	}

	ACTIVE_STACK_POP (stix);

	handle = (void*)STIX_OOP_TO_SMOOI(arg); /* TODO: how to store void* ???. fix this not to loose accuracy */
	if (stix->vmprim.mod_close) stix->vmprim.mod_close (stix, handle);
	return 1;
}

static int prim_ffi_call (stix_t* stix, stix_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	stix_oop_t rcv, fun, sig, args;

	STIX_ASSERT (nargs == 3);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 3);
	fun = ACTIVE_STACK_GET(stix, stix->sp - 2);
	sig = ACTIVE_STACK_GET(stix, stix->sp - 1);
	args = ACTIVE_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(fun)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!STIX_ISTYPEOF(stix, sig, STIX_OBJ_TYPE_CHAR) || STIX_OBJ_GET_SIZE(sig) <= 0)
	{
printf ("wrong signature...\n");
		return 0;
	}

	if (STIX_CLASSOF(stix,args) != stix->_array) /* TODO: check if arr is a kind of array??? or check if it's indexed */
	{
		/* TODO: more info on error */
		return 0;
	}

	{
		stix_oow_t i;
		DCCallVM* dc;
		void* f;
		stix_oop_oop_t arr;
		int mode_set;

		f = STIX_OOP_TO_SMOOI(fun); /* TODO: decode pointer properly */
		arr = (stix_oop_oop_t)args;

		dc = dcNewCallVM (4096);
		if (!dc) return -1; /* TODO: proper error handling */

printf ("CALLING............%p\n", f);
		/*dcMode (dc, DC_CALL_C_DEFAULT);
		dcReset (dc);*/

		/*for (i = 2; i < STIX_OBJ_GET_SIZE(sig); i++)
		{
			if (((stix_oop_char_t)sig)->slot[i] == '|') 
			{
				dcMode (dc, DC_CALL_C_ELLIPSIS);
printf ("CALL MODE 111 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				mode_set = 1;
				break;
			}
		}
		if (!mode_set) */ dcMode (dc, DC_CALL_C_DEFAULT);

		for (i = 2; i < STIX_OBJ_GET_SIZE(sig); i++)
		{
printf ("CALLING ARG %c\n", ((stix_oop_char_t)sig)->slot[i]);
			switch (((stix_oop_char_t)sig)->slot[i])
			{
			/* TODO: support more types... */
				/*
				case '|':
					dcMode (dc, DC_CALL_C_ELLIPSIS_VARARGS);
printf ("CALL MODE 222 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
					break;
				*/

				case 'c':
					/* TODO: sanity check on the argument type */
					dcArgChar (dc, STIX_OOP_TO_CHAR(arr->slot[i - 2]));
					break;

				case 'i':
					dcArgInt (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'l':
					dcArgLong (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'L':
					dcArgLongLong (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 's':
				{
					stix_oow_t bcslen, ucslen;
					stix_bch_t bcs[1024];

					ucslen = STIX_OBJ_GET_SIZE(arr->slot[i - 2]);
					stix_ucstoutf8 (((stix_oop_char_t)arr->slot[i - 2])->slot, &ucslen, bcs, &bcslen); /* proper string conversion */

					bcs[bcslen] = '\0';
					dcArgPointer (dc, bcs);
					break;
				}

				default:
					/* TODO: ERROR HANDLING */
					break;
			}

		}

		ACTIVE_STACK_POPS (stix, nargs);

		switch (((stix_oop_char_t)sig)->slot[0])
		{
/* TODO: support more types... */
/* TODO: proper return value conversion */
			case 'c':
			{
				char r = dcCallChar (dc, f);
				ACTIVE_STACK_SETTOP (stix, STIX_CHAR_TO_OOP(r));
				break;
			}

			case 'i':
			{
				int r = dcCallInt (dc, f);
printf ("CALLED... %d\n", r);
printf ("CALL ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 'l':
			{
				long r = dcCallLong (dc, f);
				ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 'L':
			{
				long long r = dcCallLongLong (dc, f);
				ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 's':
			{
				stix_oow_t bcslen, ucslen;
				stix_ooch_t ucs[1024];
				stix_oop_t s;
				char* r = dcCallPointer (dc, f);
				
				bcslen = strlen(r); 
				stix_utf8toucs (r, &bcslen, ucs, &ucslen); /* proper string conversion */

				s = stix_makestring(stix, ucs, ucslen)
				if (!s) 
				{
					dcFree (dc);
					return -1; /* TODO: proper error h andling */
				}

				ACTIVE_STACK_SETTOP (stix, s); 
				break;
			}

			default:
				/* TOOD: ERROR HANDLING */
				break;
		}

		dcFree (dc);
	}

	return 1;
#else
	return 0;
#endif
}

static int prim_ffi_getsym (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, hnd, fun;
	void* sym;

	STIX_ASSERT (nargs == 2);

	rcv = ACTIVE_STACK_GET(stix, stix->sp - 2);
	fun = ACTIVE_STACK_GET(stix, stix->sp - 1);
	hnd = ACTIVE_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(hnd)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!STIX_ISTYPEOF(stix,fun,STIX_OBJ_TYPE_CHAR))
	{
printf ("wrong function name...\n");
		return 0;
	}

	if (!stix->vmprim.mod_getsym)
	{
		return 0;
	}

	sym = stix->vmprim.mod_getsym (stix, (void*)STIX_OOP_TO_SMOOI(hnd), ((stix_oop_char_t)fun)->slot);
	if (!sym)
	{
		return 0;
	}

	ACTIVE_STACK_POPS (stix, 2);
/* TODO: how to hold an address? as an integer????  or a byte array? */
	ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(sym));

	return 1;
}


struct prim_t
{
	stix_ooi_t          nargs;   /* expected number of arguments */
	stix_prim_impl_t    handler;
	const char*         name;    /* the name is supposed to be 7-bit ascii only */
};
typedef struct prim_t prim_t;

static prim_t primitives[] =
{
	{  -1,   prim_dump,                 "_dump"                },
	{   1,   prim_identical,            "_identical"           },
	{   1,   prim_not_identical,        "_not_identical"       },
	{   0,   prim_class,                "_class"               },

	{   0,   prim_basic_new,            "_basic_new"           },
	{   1,   prim_basic_new_with_size,  "_basic_new_with_size" },
	{   0,   prim_shallow_copy,         "_shallow_copy"        },

	{   0,   prim_basic_size,           "_basic_size"          },
	{   1,   prim_basic_at,             "_basic_at"            },
	{   2,   prim_basic_at_put,         "_basic_at_put"        },


	{  -1,   prim_block_value,          "_block_value"         },
	{  -1,   prim_block_new_process,    "_block_new_process"   },

	{   1,   prim_integer_add,          "_integer_add"         },
	{   1,   prim_integer_sub,          "_integer_sub"         },
	{   1,   prim_integer_mul,          "_integer_mul"         },
	{   1,   prim_integer_quo,          "_integer_quo"         },
	{   1,   prim_integer_rem,          "_integer_rem"         },
	{   1,   prim_integer_quo2,         "_integer_quo2"        },
	{   1,   prim_integer_rem2,         "_integer_rem2"        },
	{   0,   prim_integer_negated,      "_integer_negated"     },
	{   1,   prim_integer_bitat,        "_integer_bitat"       },
	{   1,   prim_integer_bitand,       "_integer_bitand"      },
	{   1,   prim_integer_bitor,        "_integer_bitor"       },
	{   1,   prim_integer_bitxor,       "_integer_bitxor"      },
	{   0,   prim_integer_bitinv,       "_integer_bitinv"      },
	{   1,   prim_integer_bitshift,     "_integer_bitshift"    },
	{   1,   prim_integer_eq,           "_integer_eq"          },
	{   1,   prim_integer_ne,           "_integer_ne"          },
	{   1,   prim_integer_lt,           "_integer_lt"          },
	{   1,   prim_integer_gt,           "_integer_gt"          },
	{   1,   prim_integer_le,           "_integer_le"          },
	{   1,   prim_integer_ge,           "_integer_ge"          },
	{   1,   prim_integer_inttostr,     "_integer_inttostr"    },

	{   1,   prim_processor_schedule,   "_processor_schedule"  },
	{   1,   prim_processor_remove,     "_processor_remove"    },
	{   1,   prim_processor_sleep,      "_processor_sleep"     },

	{   1,   prim_ffi_open,             "_ffi_open"            },
	{   1,   prim_ffi_close,            "_ffi_close"           },
	{   2,   prim_ffi_getsym,           "_ffi_getsym"          },
	{   3,   prim_ffi_call,             "_ffi_call"            }
	
};

int stix_getprimno (stix_t* stix, const stix_oocs_t* name)
{
	int i;

	for (i = 0; i < STIX_COUNTOF(primitives); i++)
	{
		if (stix_compucxbcstr(name->ptr, name->len, primitives[i].name) == 0)
		{
			return i;
		}
	}

	stix->errnum = STIX_ENOENT;
	return -1;
}

static stix_prim_impl_t query_prim_module (stix_t* stix, const stix_ooch_t* name, stix_oow_t len)
{
	stix_rbt_pair_t* pair;
	stix_prim_mod_data_t* mdp;
	const stix_ooch_t* sep;
	stix_oow_t mod_name_len;
	stix_prim_impl_t handler;
	int n;

	sep = stix_findoochar (name, len, '_');
	STIX_ASSERT (sep != STIX_NULL);
	mod_name_len = sep - name;

	pair = stix_rbt_search (&stix->pmtable, name, mod_name_len);
	if (pair)
	{
		mdp = (stix_prim_mod_data_t*)STIX_RBT_VPTR(pair);
	}
	else
	{
		stix_prim_mod_data_t md;
		stix_prim_mod_load_t load = STIX_NULL;

		/* maximum module name length is STIX_MOD_NAME_LEN_MAX. 
		 * 16 is decomposed to 14 + 1 + 1.
		 *   14 for stix_prim_mod_.
		 *   1 for _ at the end when stix_prim_mod_xxx_ is attempted.
		 *   1 for the terminating '\0'.
		 */
		stix_ooch_t buf[STIX_MOD_NAME_LEN_MAX + 16]; 

		/* the terminating null isn't needed in buf here */
		stix_copybchtooochars (buf, "stix_prim_mod_", 14); 

		if (mod_name_len > STIX_COUNTOF(buf) - 16)
		{
			/* module name too long  */
			stix->errnum = STIX_EINVAL; /* TODO: change the  error number to something more specific */
			return STIX_NULL;
		}

		stix_copyoochars (&buf[14], name, mod_name_len);
		buf[14 + mod_name_len] = '\0';

#if defined(STIX_ENABLE_STATIC_MODULE)
		/* attempt to find a statically linked module */

/*TODO: CHANGE THIS PART */

		/* TODO: binary search ... */
		for (n = 0; n < STIX_COUNTOF(static_modtab); n++)
		{
			if (stix_compoocstr (static_modtab[n].modname, name) == 0) 
			{
				load = static_modtab[n].modload;
				break;
			}
		}

		if (n >= STIX_COUNTOF(static_modtab))
		{
			stix->errnum = STIX_ENOENT;
			return STIX_NULL;
		}

		if (load)
		{
			/* found the module in the staic module table */

			STIX_MEMSET (&md, 0, STIX_SIZEOF(md));
			/* Note md.handle is STIX_NULL for a static module */

			/* i copy-insert 'md' into the table before calling 'load'.
			 * to pass the same address to load(), query(), etc */
			pair = stix_rbt_insert (stix->modtab, name, mod_name_len, &md, STIX_SIZEOF(md));
			if (pair == STIX_NULL)
			{
				stix->errnum = STIX_ESYSMEM;
				return STIX_NULL;
			}

			mdp = (stix_mod_data_t*)STIX_RBT_VPTR(pair);
			if (load (&mdp->mod, stix) <= -1)
			{
				stix_rbt_delete (stix->modtab, segs[0].ptr, segs[0].len);
				return STIX_NULL;
			}

			goto done;
		}
#endif

		/* attempt to find an external module */
		STIX_MEMSET (&md, 0, STIX_SIZEOF(md));
		if (stix->vmprim.mod_open && stix->vmprim.mod_getsym && stix->vmprim.mod_close)
		{
			md.handle = stix->vmprim.mod_open (stix, &buf[14]);
		}

		if (md.handle == STIX_NULL) 
		{
			stix->errnum = STIX_ENOENT; /* TODO: be more descriptive about the error */
			return STIX_NULL;
		}

		/* attempt to get stix_prim_mod_xxx */
		load = stix->vmprim.mod_getsym (stix, md.handle, buf);
		if (!load) 
		{
			stix->errnum = STIX_ENOENT; /* TODO: be more descriptive about the error */
			stix->vmprim.mod_close (stix, md.handle);
			return STIX_NULL;
		}

		/* i copy-insert 'md' into the table before calling 'load'.
		 * to pass the same address to load(), query(), etc */
		pair = stix_rbt_insert (&stix->pmtable, (void*)name, mod_name_len, &md, STIX_SIZEOF(md));
		if (pair == STIX_NULL)
		{
			stix->errnum = STIX_ESYSMEM;
			stix->vmprim.mod_close (stix, md.handle);
			return STIX_NULL;
		}

		mdp = (stix_prim_mod_data_t*)STIX_RBT_VPTR(pair);
		if (load (stix, &mdp->mod) <= -1)
		{
			stix->errnum = STIX_ENOENT; /* TODO: proper error code and handling */
			stix_rbt_delete (&stix->pmtable, name, mod_name_len);
			stix->vmprim.mod_close (stix, mdp->handle);
			return STIX_NULL;
		}

		/* the module loader must ensure to set a proper query handler */
		STIX_ASSERT (mdp->mod.query != STIX_NULL);
	}

done:
	if ((handler = mdp->mod.query (stix, &mdp->mod, sep + 1)) == STIX_NULL) 
	{
		stix->errnum = STIX_ENOENT; /* TODO: proper error code and handling */
		return STIX_NULL;
	}
	return handler;
}

/* ------------------------------------------------------------------------- */

int stix_execute (stix_t* stix)
{
	stix_oob_t bcode;
	stix_ooi_t b1, b2;
	stix_oop_t return_value;

#if defined(STIX_PROFILE_EXEC)
	stix_oow_t inst_counter = 0;
#endif

	STIX_ASSERT (stix->active_context != STIX_NULL);

	while (1)
	{
#if 0
printf ("IP<BF> => %d\n", (int)stix->ip);
#endif
switch_to_next_process (stix);
#if 0
printf ("IP<AF> => %d\n", (int)stix->ip);
#endif

		FETCH_BYTE_CODE_TO (stix, bcode);
		/*while (bcode == BCODE_NOOP) FETCH_BYTE_CODE_TO (stix, bcode);*/

#if 0
printf ("BCODE = %x\n", bcode);
#endif

#if defined(STIX_PROFILE_EXEC)
		inst_counter++;
#endif

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
				DBGOUT_EXEC_1 ("PUSH_INSTVAR %d", (int)b1);
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
				DBGOUT_EXEC_1 ("STORE_INTO_INSTVAR %d", (int)b1);
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
				DBGOUT_EXEC_1 ("POP_INTO_INSTVAR %d", (int)b1);
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
				stix_oop_context_t ctx;
				stix_ooi_t bx;

				b1 = bcode & 0x7; /* low 3 bits */
			handle_tempvar:

			#if defined(STIX_USE_CTXTEMPVAR)
				ctx = stix->active_context->origin;
				bx = b1;
			#else
				if (stix->active_context->home != stix->_nil)
				{
					/* this code assumes that the method context and
					 * the block context place some key fields in the
					 * same offset. such fields include 'home', 'ntmprs' */
					stix_oop_t home;
					stix_ooi_t home_ntmprs;

					ctx = stix->active_context;
					home = ctx->home;

					do
					{
						home_ntmprs = STIX_OOP_TO_SMOOI(((stix_oop_context_t)home)->ntmprs);
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
			#endif

				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on*/
					DBGOUT_EXEC_1 ("PUSH_TEMPVAR %d", (int)b1);
					ACTIVE_STACK_PUSH (stix, ctx->slot[bx]);
				}
				else
				{
					/* store or pop - bit 5 off */
					ctx->slot[bx] = ACTIVE_STACK_GETTOP(stix);

					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						DBGOUT_EXEC_1 ("POP_INTO_TEMPVAR %d", (int)b1);
						ACTIVE_STACK_POP (stix);
					}
					else
					{
						DBGOUT_EXEC_1 ("STORE_INTO_TEMPVAR %d", (int)b1);
					}
				}
				/*
				print_object (stix, ctx->slot[bx]);
				printf ("\n");
 				*/
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
				DBGOUT_EXEC_1 ("PUSH_LITERAL %d", (int)b1);
				/*
				print_object (stix, stix->active_method->slot[b1]);
				printf ("\n");
				*/
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
						DBGOUT_EXEC_1("POP_INTO_OBJECT %d", (int)b1);
						ACTIVE_STACK_POP (stix);
					}
					else
					{
						DBGOUT_EXEC_1("STORE_INTO_OBJECT %d", (int)b1);
					}
				}
				else
				{
					/* push */
					DBGOUT_EXEC_1("PUSH_OBJECT %d", (int)b1);
					ACTIVE_STACK_PUSH (stix, ass->value);
				}
				break;
			}

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("JUMP_FORWARD %d", (int)b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
				DBGOUT_EXEC_1 ("JUMP_FORWARD %d", (int)(bcode & 0x3));
				stix->ip += (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("JUMP_BACKWARD %d", (int)b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
				DBGOUT_EXEC_1 ("JUMP_BACKWARD %d", (int)(bcode & 0x3));
				stix->ip -= (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_IF_TRUE_X:
			case BCODE_JUMP_IF_FALSE_X:
			case BCODE_JUMP_IF_TRUE_0:
			case BCODE_JUMP_IF_TRUE_1:
			case BCODE_JUMP_IF_TRUE_2:
			case BCODE_JUMP_IF_TRUE_3:
			case BCODE_JUMP_IF_FALSE_0:
			case BCODE_JUMP_IF_FALSE_1:
			case BCODE_JUMP_IF_FALSE_2:
			case BCODE_JUMP_IF_FALSE_3:
printf ("<<<<<<<<<<<<<< JUMP NOT IMPLEMENTED YET >>>>>>>>>>>> \n");
stix->errnum = STIX_ENOIMPL;
return -1;

			case BCODE_JUMP2_FORWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("JUMP2_FORWARD %d", (int)b1);
				stix->ip += MAX_CODE_JUMP + b1;
				break;
				break;

			case BCODE_JUMP2_BACKWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("JUMP2_BACKWARD %d", (int)b1);
				stix->ip -= MAX_CODE_JUMP + b1;
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
						DBGOUT_EXEC_2 ("POP_INTO_CTXTEMPVAR %d %d", (int)b1, (int)b2);
					}
					else
					{
						DBGOUT_EXEC_2 ("STORE_INTO_CTXTEMPVAR %d %d", (int)b1, (int)b2);
					}
				}
				else
				{
					/* push */
					ACTIVE_STACK_PUSH (stix, ctx->slot[b2]);
					DBGOUT_EXEC_2 ("PUSH_CTXTEMPVAR %d %d", (int)b1, (int)b2);
				}
/*
print_object (stix, ctx->slot[b2]);
printf ("\n");
*/

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
						DBGOUT_EXEC_2 ("POP_INTO_OBJVAR %d %d", (int)b1, (int)b2);
					}
					else
					{
						DBGOUT_EXEC_2 ("STORE_INTO_OBJVAR %d %d", (int)b1, (int)b2);
					}
				}
				else
				{
					/* push */
					DBGOUT_EXEC_2 ("PUSH_OBJVAR %d %d", (int)b1, (int)b2);
					ACTIVE_STACK_PUSH (stix, t->slot[b1]);
				}
/*
print_object (stix, t->slot[b1]);
printf ("\n");
*/
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
				stix_oocs_t mthname;
				stix_oop_t newrcv;
				stix_oop_method_t newmth;
				stix_oop_char_t selector;
				stix_ooi_t preamble, preamble_code;


			handle_send_message:
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)stix->active_method->slot[b2];


#if defined(STIX_DEBUG_EXEC)
printf ("SEND_MESSAGE%s TO RECEIVER AT STACKPOS=%d NARGS=%d SELECTOR=", (((bcode >> 2) & 1)? "_TO_SUPER": ""), (int)(stix->sp - b1), (int)b1);
print_object (stix, (stix_oop_t)selector);
fflush (stdout);
#endif
				STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);
				newrcv = ACTIVE_STACK_GET(stix, stix->sp - b1);

#if defined(STIX_DEBUG_EXEC)
printf (" RECEIVER = ");
print_object(stix, newrcv);
printf ("\n");
#endif
				mthname.ptr = selector->slot;
				mthname.len = STIX_OBJ_GET_SIZE(selector);
				newmth = find_method (stix, newrcv, &mthname, ((bcode >> 2) & 1));
				if (!newmth) 
				{
/* TODO: implement doesNotUnderstand: XXXXX  instead of returning -1. */
stix_oop_t c;

c = STIX_CLASSOF(stix,newrcv);

printf ("ERROR [NOT IMPLEMENTED YET] - receiver [");
print_object (stix, newrcv);
printf ("] class ");
print_object (stix, c);
printf (" doesNotUnderstand: [");
print_oocs (&mthname);
printf ("]\n");
					goto oops;
				}

				STIX_ASSERT (STIX_OOP_TO_SMOOI(newmth->tmpr_nargs) == b1);

				preamble = STIX_OOP_TO_SMOOI(newmth->preamble);
				preamble_code = STIX_METHOD_GET_PREAMBLE_CODE(preamble);
				switch (preamble_code)
				{
					case STIX_METHOD_PREAMBLE_RETURN_RECEIVER:
						DBGOUT_EXEC_0 ("METHOD_PREAMBLE_RETURN_RECEIVER");
						ACTIVE_STACK_POPS (stix, b1); /* pop arguments only*/
						break;

					case STIX_METHOD_PREAMBLE_RETURN_NIL:
						DBGOUT_EXEC_0 ("METHOD_PREAMBLE_RETURN_NIL");
						ACTIVE_STACK_POPS (stix, b1);
						ACTIVE_STACK_SETTOP (stix, stix->_nil);
						break;

					case STIX_METHOD_PREAMBLE_RETURN_TRUE:
						DBGOUT_EXEC_0 ("METHOD_PREAMBLE_RETURN_TRUE");
						ACTIVE_STACK_POPS (stix, b1);
						ACTIVE_STACK_SETTOP (stix, stix->_true);
						break;

					case STIX_METHOD_PREAMBLE_RETURN_FALSE:
						DBGOUT_EXEC_0 ("METHOD_PREAMBLE_RETURN_FALSE");
						ACTIVE_STACK_POPS (stix, b1);
						ACTIVE_STACK_SETTOP (stix, stix->_false);
						break;

					case STIX_METHOD_PREAMBLE_RETURN_INDEX:
						DBGOUT_EXEC_1 ("METHOD_PREAMBLE_RETURN_INDEX %d", (int)STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
						ACTIVE_STACK_POPS (stix, b1);
						ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(STIX_METHOD_GET_PREAMBLE_INDEX(preamble)));
						break;

					case STIX_METHOD_PREAMBLE_RETURN_NEGINDEX:
						DBGOUT_EXEC_1 ("METHOD_PREAMBLE_RETURN_NEGINDEX %d", (int)STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
						ACTIVE_STACK_POPS (stix, b1);
						ACTIVE_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(-STIX_METHOD_GET_PREAMBLE_INDEX(preamble)));
						break;

					case STIX_METHOD_PREAMBLE_RETURN_INSTVAR:
					{
						stix_oop_oop_t rcv;

						ACTIVE_STACK_POPS (stix, b1); /* pop arguments only */

						DBGOUT_EXEC_1 ("METHOD_PREAMBLE_RETURN_INSTVAR %d", (int)STIX_METHOD_GET_PREAMBLE_INDEX(preamble));

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
						DBGOUT_EXEC_1 ("METHOD_PREAMBLE_PRIMITIVE %d", (int)prim_no);

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

						/* primitive handler failed */
						if (activate_new_method (stix, newmth) <= -1) goto oops;
						break;
					}

					case STIX_METHOD_PREAMBLE_NAMED_PRIMITIVE:
					{
						stix_ooi_t prim_name_index;
						stix_oop_t name;
						stix_prim_impl_t handler;
						register stix_oow_t w;

						prim_name_index = STIX_METHOD_GET_PREAMBLE_INDEX(preamble);
						DBGOUT_EXEC_1 ("METHOD_PREAMBLE_NAMED_PRIMITIVE %d", (int)prim_name_index);

						name = newmth->slot[prim_name_index];

						STIX_ASSERT (STIX_ISTYPEOF(stix,name,STIX_OBJ_TYPE_CHAR));
						STIX_ASSERT (STIX_OBJ_GET_FLAGS_EXTRA(name));
						STIX_ASSERT (STIX_CLASSOF(stix,name) == stix->_symbol);

						/* merge two SmallIntegers to get a full pointer */
						w = (stix_oow_t)STIX_OOP_TO_SMOOI(newmth->preamble_data[0]) << (STIX_OOW_BITS / 2) | 
						          (stix_oow_t)STIX_OOP_TO_SMOOI(newmth->preamble_data[1]);
						handler = (stix_prim_impl_t)w;
						if (!handler) handler = query_prim_module (stix, ((stix_oop_char_t)name)->slot, STIX_OBJ_GET_SIZE(name));

						if (handler)
						{
							int n;

							/* split a pointer to two OOP fields as SmallIntegers for storing. */
							newmth->preamble_data[0] = STIX_SMOOI_TO_OOP((stix_oow_t)handler >> (STIX_OOW_BITS / 2));
							newmth->preamble_data[1] = STIX_SMOOI_TO_OOP((stix_oow_t)handler & STIX_LBMASK(stix_oow_t, STIX_OOW_BITS / 2));

							stix_pushtmp (stix, (stix_oop_t*)&newmth);
							n = handler (stix, b1);
							stix_poptmp (stix);
							if (n <= -1) goto oops;
							if (n >= 1) break;
						}
						

						/* primitive handler failed or not found*/
						if (activate_new_method (stix, newmth) <= -1) goto oops;
						break;
					}

					default:
						STIX_ASSERT (preamble_code == STIX_METHOD_PREAMBLE_NONE);
						if (activate_new_method (stix, newmth) <= -1) goto oops;
						break;
				}

				break; /* CMD_SEND_MESSAGE */
			}

			/* -------------------------------------------------------- */

			case BCODE_PUSH_RECEIVER:
				DBGOUT_EXEC_0 ("PUSH_RECEIVER");
				ACTIVE_STACK_PUSH (stix, stix->active_context->origin->receiver_or_source);
				break;

			case BCODE_PUSH_NIL:
				DBGOUT_EXEC_0 ("PUSH_NIL");
				ACTIVE_STACK_PUSH (stix, stix->_nil);
				break;

			case BCODE_PUSH_TRUE:
				DBGOUT_EXEC_0 ("PUSH_TRUE");
				ACTIVE_STACK_PUSH (stix, stix->_true);
				break;

			case BCODE_PUSH_FALSE:
				DBGOUT_EXEC_0 ("PUSH_FALSE");
				ACTIVE_STACK_PUSH (stix, stix->_false);
				break;

			case BCODE_PUSH_CONTEXT:
				DBGOUT_EXEC_0 ("PUSH_CONTEXT");
				ACTIVE_STACK_PUSH (stix, (stix_oop_t)stix->active_context);
				break;

			case BCODE_PUSH_NEGONE:
				DBGOUT_EXEC_0 ("PUSH_NEGONE");
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(-1));
				break;

			case BCODE_PUSH_ZERO:
				DBGOUT_EXEC_0 ("PUSH_ZERO");
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(0));
				break;

			case BCODE_PUSH_ONE:
				DBGOUT_EXEC_0 ("PUSH_ONE");
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(1));
				break;

			case BCODE_PUSH_TWO:
				DBGOUT_EXEC_0 ("PUSH_TWO");
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(2));
				break;

			case BCODE_PUSH_INTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("PUSH_INTLIT %d", (int)b1);
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(b1));
				break;

			case BCODE_PUSH_NEGINTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				DBGOUT_EXEC_1 ("PUSH_NEGINTLIT %d", (int)-b1);
				ACTIVE_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(-b1));
				break;

			/* -------------------------------------------------------- */

			case BCODE_DUP_STACKTOP:
			{
				stix_oop_t t;
				DBGOUT_EXEC_0 ("DUP_STACKTOP");
				STIX_ASSERT (!ACTIVE_STACK_ISEMPTY(stix));
				t = ACTIVE_STACK_GETTOP(stix);
				ACTIVE_STACK_PUSH (stix, t);
				break;
			}

			case BCODE_POP_STACKTOP:
				DBGOUT_EXEC_0 ("POP_STACKTOP");
				STIX_ASSERT (!ACTIVE_STACK_ISEMPTY(stix));
				ACTIVE_STACK_POP (stix);
				break;

			case BCODE_RETURN_STACKTOP:
				DBGOUT_EXEC_0 ("RETURN_STACKTOP");
				return_value = ACTIVE_STACK_GETTOP(stix);
				ACTIVE_STACK_POP (stix);
				goto handle_return;

			case BCODE_RETURN_RECEIVER:
				DBGOUT_EXEC_0 ("RETURN_RECEIVER");
				return_value = stix->active_context->origin->receiver_or_source;

			handle_return:
#if defined(STIX_DEBUG_EXEC)
printf ("<<LEAVING>> SP=%d\n", (int)stix->sp);
#endif

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

				if (stix->processor->active->initial_context == stix->active_context)
				{
/* TODO: terminate a proces... */
printf ("TERMINATING XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
				}
				else
				{
					SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->origin->sender);

					/* push the return value to the stack of the new active context */
					ACTIVE_STACK_PUSH (stix, return_value);

					if (stix->active_context->sender == stix->_nil)
					{
						/* the sending context of the intial context has been set to nil.
						 * use this fact to tell an initial context from a normal context. */
						STIX_ASSERT (stix->active_context->receiver_or_source == stix->_nil);
#if defined(STIX_DEBUG_EXEC)
printf ("<<<RETURNIGN TO THE INITIAL CONTEXT>>>\n");
#endif
						STIX_ASSERT (stix->sp == 0);
						goto done;
					}
				}

				break;

			case BCODE_RETURN_FROM_BLOCK:
				DBGOUT_EXEC_0 ("RETURN_FROM_BLOCK");

				STIX_ASSERT(STIX_CLASSOF(stix, stix->active_context)  == stix->_block_context);

				if (stix->active_context == stix->processor->active->initial_context)
				{
					/* TODO: terminate the process */
printf ("TERMINATE A PROCESS............\n");
/* **************************************** */
				}
				else
				{
					return_value = ACTIVE_STACK_GETTOP(stix);
					SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->sender);
					ACTIVE_STACK_PUSH (stix, return_value);
				}

				break;

			case BCODE_MAKE_BLOCK:
			{
				stix_oop_context_t blkctx;

				/* b1 - number of block arguments
				 * b2 - number of block temporaries */
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);

				DBGOUT_EXEC_2 ("MAKE_BLOCK %d %d", (int)b1, (int)b2);

				STIX_ASSERT (b1 >= 0);
				STIX_ASSERT (b2 >= b1);

				/* the block context object created here is used
				 * as a base object for block context activation.
				 * prim_block_value() clones a block 
				 * context and activates the cloned context.
				 * this base block context is created with no 
				 * stack for this reason. */
				blkctx = (stix_oop_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 0); 
				if (!blkctx) return -1;

				/* the long forward jump instruction has the format of 
				 *   11000100 KKKKKKKK or 11000100 KKKKKKKK KKKKKKKK 
				 * depending on STIX_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
				 * the instruction after the jump. */
				blkctx->ip = STIX_SMOOI_TO_OOP(stix->ip + STIX_BCODE_LONG_PARAM_SIZE + 1);
				/* stack pointer below the bottom. this block context has an
				 * empty stack anyway. */
				blkctx->sp = STIX_SMOOI_TO_OOP(-1);
				/* the number of arguments for a block context is local to the block */
				blkctx->method_or_nargs = STIX_SMOOI_TO_OOP(b1);
				/* the number of temporaries here is an accumulated count including
				 * the number of temporaries of a home context */
				blkctx->ntmprs = STIX_SMOOI_TO_OOP(b2);

				blkctx->home = (stix_oop_t)stix->active_context;
				blkctx->receiver_or_source = stix->_nil; /* no source */

				blkctx->origin = stix->active_context->origin;
				ACTIVE_STACK_PUSH (stix, (stix_oop_t)blkctx);
				break;
			}

			case BCODE_SEND_BLOCK_COPY:
			{
				stix_ooi_t nargs, ntmprs;
				stix_oop_context_t rctx;
				stix_oop_context_t blkctx;

				DBGOUT_EXEC_0 ("SEND_BLOCK_COPY");

				/* it emulates thisContext blockCopy: nargs ofTmprCount: ntmprs */
				STIX_ASSERT (stix->sp >= 2);

				STIX_ASSERT (STIX_CLASSOF(stix, ACTIVE_STACK_GETTOP(stix)) == stix->_small_integer);
				ntmprs = STIX_OOP_TO_SMOOI(ACTIVE_STACK_GETTOP(stix));
				ACTIVE_STACK_POP (stix);

				STIX_ASSERT (STIX_CLASSOF(stix, ACTIVE_STACK_GETTOP(stix)) == stix->_small_integer);
				nargs = STIX_OOP_TO_SMOOI(ACTIVE_STACK_GETTOP(stix));
				ACTIVE_STACK_POP (stix);

				STIX_ASSERT (nargs >= 0);
				STIX_ASSERT (ntmprs >= nargs);

				/* the block context object created here is used
				 * as a base object for block context activation.
				 * prim_block_value() clones a block 
				 * context and activates the cloned context.
				 * this base block context is created with no 
				 * stack for this reason. */
				blkctx = (stix_oop_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 0); 
				if (!blkctx) return -1;

				/* get the receiver to the block copy message after block context instantiation
				 * not to get affected by potential GC */
				rctx = (stix_oop_context_t)ACTIVE_STACK_GETTOP(stix);
				STIX_ASSERT (rctx == stix->active_context);

				/* [NOTE]
				 *  blkctx->caller is left to nil. it is set to the 
				 *  active context before it gets activated. see
				 *  prim_block_value().
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
				blkctx->ip = STIX_SMOOI_TO_OOP(stix->ip + STIX_BCODE_LONG_PARAM_SIZE + 1);
				blkctx->sp = STIX_SMOOI_TO_OOP(-1);
				/* the number of arguments for a block context is local to the block */
				blkctx->method_or_nargs = STIX_SMOOI_TO_OOP(nargs);
				/* the number of temporaries here is an accumulated count including
				 * the number of temporaries of a home context */
				blkctx->ntmprs = STIX_SMOOI_TO_OOP(ntmprs);

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

#if defined(STIX_PROFILE_EXEC)
	printf ("TOTAL_INST_COUTNER = %lu\n", (unsigned long int)inst_counter);
#endif
	return 0;


oops:
	/* TODO: anything to do here? */
	return -1;
}

int stix_invoke (stix_t* stix, const stix_oocs_t* objname, const stix_oocs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}

