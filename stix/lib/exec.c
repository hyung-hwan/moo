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

	/* the stack front has temporary variables including arguments */
	ctx->sp = STIX_OOP_FROM_SMINT(ntmprs - 1);
	ctx->method = mth;

	/* 
	 * This message sending expression is compiled to 
	 *   obj1 do: #this with: #that with: #it
	 * 
	 * the logical byte-code sequences shown below:
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
	ctx->receiver = stix->active_context->slot[j++];
	for (i = 0; i < nargs; i++) 
	{
		/* copy an argument */
		ctx->slot[i] = stix->active_context->slot[++j];
	}
	STIX_ASSERT (j == sp);

/* TODO: store the contennts of internal registers to stix->active_context */

	stix->active_context = ctx;

/* TODO: copy the contens of ctx to internal registers */
	return 0;
}

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t obj, const stix_ucs_t* mthname)
{
	stix_oop_class_t cls;

	cls = (stix_oop_class_t)STIX_CLASSOF(stix, obj);
	if (cls == stix->_class)
	{
		
	}
	else
	{
		
	}

	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}

static int activate_initial_context (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	/* the initial context is a fake context */

	stix_oow_t stack_size;
	stix_oop_context_t ctx;
	stix_oop_association_t ass;
	stix_oop_method_t mth;

	stack_size = 1;  /* grow it if you want to pass arguments */

	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, stack_size);
	if (!ctx) return -1;

	ass = stix_lookupsysdic (stix, objname);
	if (!ass) return -1;

	mth = find_method (stix, ass->value, mthname);
	if (!mth) return -1;

	ctx->slot[0] = ass->value;
	ctx->sp = STIX_OOP_FROM_SMINT(1);

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

	while (1)
	{
		
	}

	return 0;
}

int stix_invoke (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}


