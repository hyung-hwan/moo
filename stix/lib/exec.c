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


static int activate_new_method (stix_t* stix, stix_oop_method_t mth)
{
	stix_oow_t stack_size;
	stix_oop_context_t ctx;
	stix_ooi_t i, j;
	stix_ooi_t sp, ntmprs, nargs;

	stack_size = 256; /* TODO: make the stack size configurable or let the compiler choose the rightr value and store it in the compiled method. if it's stored in the compiled method, the code here can take it*/

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, stack_size);
	stix_poptmp (stix);
	if (!ctx) return -1;

	/* message sending requires a receiver to be pushed. 
	 * the stack pointer of the sending context cannot be -1 */
	sp = STIX_OOP_TO_SMINT (stix->active_context->sp);
	STIX_ASSERT (sp >= 0);
	ntmprs = STIX_OOP_TO_SMINT (mth->tmpr_count);
	STIX_ASSERT (ntmprs >= 0);
	nargs = STIX_OOP_TO_SMINT (mth->tmpr_nargs);
	STIX_ASSERT (nargs <= ntmprs);
	STIX_ASSERT (sp >= ntmprs + 1);

	ctx->sender = (stix_oop_t)stix->active_context; 
	ctx->ip = 0;

	/* the stack front has temporary variables including arguments.
	 *
	 * Context
	 *
	 *   +---------------------+
	 *   | fixed part          |
	 *   |                     |
	 *   |                     |
	 *   |                     |
	 *   +---------------------+
	 *   | tmp1 (arg1)         | slot[0]
	 *   | tmp2 (arg2)         | slot[1]
	 *   | ....                | slot[1] 
	 *   | tmpX                | slot[1]     <-- initial sp
	 *   |                     | slot[1] 
	 *   |                     | slot[1] 
	 *   |                     | slot[1] 
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
	j = sp - nargs;
	ctx->receiver = stix->active_context->slot[j];
	for (i = 0; i < nargs; i++) 
	{
		/* copy an argument*/
		ctx->slot[i] = stix->active_context->slot[++j];
	}
printf ("j = %d, sp = %d\n", (int)j, (int)sp);
	STIX_ASSERT (j == sp);

/* TODO: store the contennts of internal registers to stix->active_context */

	stix->active_context = ctx;

/* TODO: copy the contens of ctx to internal registers */
	return 0;
}

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t obj, const stix_ucs_t* mthname)
{
	stix_oop_class_t cls;
	stix_oop_association_t ass;
	stix_oop_t c;
	stix_oop_set_t mthdic;
	int dic_no;

	cls = (stix_oop_class_t)STIX_CLASSOF(stix, obj);
	if ((stix_oop_t)cls == stix->_class)
	{
		/* obj is a class object */
		c = obj; 
		dic_no = STIX_CLASS_MTHDIC_CLASS;
printf ("going to lookup class method dictioanry...\n");
	}	
	else
	{
		c = (stix_oop_t)cls;
		dic_no = STIX_CLASS_MTHDIC_INSTANCE;
printf ("going to lookup class instance dictioanry...\n");
	}

	while (c != stix->_nil)
	{
		mthdic = ((stix_oop_class_t)c)->mthdic[dic_no];
		STIX_ASSERT (STIX_CLASSOF(stix, mthdic) == stix->_method_dictionary);

dump_dictionary (stix, mthdic, "Method dictionary");
		ass = (stix_oop_association_t)stix_lookupdic (stix, mthdic, mthname);
		if (ass) 
		{
			STIX_ASSERT (STIX_CLASSOF(stix, ass->value) == stix->_method);
			return (stix_oop_method_t)ass->value;
		}
		c = ((stix_oop_class_t)c)->superclass;
	}

	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}

static int activate_initial_context (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	/* the initial context is a fake context */
	stix_oop_context_t ctx;
	stix_oop_association_t ass;
	stix_oop_method_t mth;
	stix_ooi_t sp;

	ass = stix_lookupsysdic (stix, objname);
	if (!ass) return -1;

printf ("found object...\n");
	mth = find_method (stix, ass->value, mthname);
	if (!mth) return -1;

printf ("found method...\n");
	if (STIX_OOP_TO_SMINT(mth->tmpr_nargs) > 0)
	{
		/* this method expects more than 0 arguments. 
		 * i can't use it as a start-up method.
TODO: overcome this problem 
		 */
		stix->errnum = STIX_EINVAL;
		return -1;
	}

	/* the initial context starts the life of the entire VM
	 * and is not really worked on except that it is used to call the
	 * initial method. so it doesn't really require any extra stack space.
	 * TODO: verify my theory above is true */
	sp = 1 + STIX_OOP_TO_SMINT(mth->tmpr_count);

	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, sp);
	if (!ctx) return -1;

	ctx->slot[0] = ass->value;
	ctx->sp = STIX_OOP_FROM_SMINT(sp);

	stix->active_context = ctx;
	return activate_new_method (stix, mth);
}

#if 0
static stix_oop_process_t make_process (stix_t* stix, stix_oop_context_t ctx)
{
	stix_oop_process_t proc;

	stix_pushtmp (stix, (stix_oop_t*)&ctx);
	proc = (stix_oop_process_t)stix_instantiate (stix, stix->_process, STIX_NULL, 0);
	stix_poptmp (stix);
	if (!proc) return STIX_NULL;

	proc->context = ctx;
	return proc;
}
#endif

int stix_execute (stix_t* stix)
{
	stix_oop_method_t mth;
	stix_oop_byte_t code;
	stix_ooi_t ip;
	stix_byte_t bc;

	STIX_ASSERT (stix->active_context != STIX_NULL);
	mth = stix->active_context->method;
	ip = STIX_OOP_TO_SMINT(stix->active_context->ip);
	code = mth->code;

	while (1)
	{


		bc = code->slot[ip++];
		cmd = bc >> 4;
		if (cmd == CMD_EXTEND)
		{
			cmd = bc & 0xF;
		}

	}

	return -1;
}

int stix_invoke (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}


