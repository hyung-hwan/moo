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


static int activate_new_method (stix_t* stix, stix_oop_method_t mth, stix_ooi_t* xip, stix_ooi_t* xsp)
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
	/*sp = STIX_OOP_TO_SMINT (stix->active_context->sp);*/
	sp = *xsp;
printf ("@@@@@@@@@@@@@ %d %d\n", (int)*xsp, (int)STIX_OOP_TO_SMINT(stix->active_context->sp));
	ntmprs = STIX_OOP_TO_SMINT (mth->tmpr_count);
	nargs = STIX_OOP_TO_SMINT (mth->tmpr_nargs);

	STIX_ASSERT (ntmprs >= 0);
	STIX_ASSERT (nargs <= ntmprs);
	STIX_ASSERT (sp >= 0);
	STIX_ASSERT (sp >= nargs);

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
	j = sp - nargs;
	ctx->receiver = stix->active_context->slot[j];
printf ("SETTING RECEIVER %p FROM SLOT %d SP %d NARGS %d\n", ctx->receiver, (int)j, (int)sp, (int)nargs);
	for (i = 0; i < nargs; i++) 
	{
		/* copy an argument*/
		ctx->slot[i] = stix->active_context->slot[++j];
	}
printf ("j = %d, sp = %d\n", (int)j, (int)sp);
	STIX_ASSERT (j == sp);

/* TODO: store the contennts of internal registers to stix->active_context */
	sp -= nargs + 1;
	stix->active_context->ip = STIX_OOP_FROM_SMINT(*xip);
	stix->active_context->sp = STIX_OOP_FROM_SMINT(sp);

	stix->active_context = ctx;

/* TODO: copy the contens of ctx to internal registers */
	*xip = STIX_OOP_TO_SMINT(stix->active_context->ip);
	*xsp = STIX_OOP_TO_SMINT(stix->active_context->sp);

	return 0;
}

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t receiver, const stix_ucs_t* message)
{
	stix_oop_class_t cls;
	stix_oop_association_t ass;
	stix_oop_t c;
	stix_oop_set_t mthdic;
	int dic_no;
/* TODO: implement method lookup cache */

printf ("FINDING METHOD FOR %p ", receiver);
print_ucs (message);
printf ("\n");
	cls = (stix_oop_class_t)STIX_CLASSOF(stix, receiver);
	if ((stix_oop_t)cls == stix->_class)
	{
		/* receiver is a class receiverect */
		c = receiver; 
		dic_no = STIX_CLASS_MTHDIC_CLASS;
printf ("going to lookup class method dictioanry...\n");
	}
	else
	{
		c = (stix_oop_t)cls;
		dic_no = STIX_CLASS_MTHDIC_INSTANCE;
printf ("going to lookup instance method dictioanry...\n");
	}

	while (c != stix->_nil)
	{
		mthdic = ((stix_oop_class_t)c)->mthdic[dic_no];
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
	stix_ooi_t ip, sp;

	/* create a fake initial context */
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_context, STIX_NULL, 1);
	if (!ctx) return -1;

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
	ip = 0;
	sp = -1;

	ctx->slot[++sp] = ass->value; /* push receiver */
	ctx->sp = STIX_OOP_FROM_SMINT(sp);
	ctx->ip = STIX_OOP_FROM_SMINT(ip); /* fake */
	/* receiver, sender, method are nils */

	stix->active_context = ctx;
	return activate_new_method (stix, mth, &ip, &sp);
}

int stix_execute (stix_t* stix)
{
	stix_oop_method_t mth;
	stix_oop_byte_t code;
	stix_ooi_t ip, sp;
	stix_oop_t receiver;

	stix_byte_t bc, cmd;
	stix_oow_t b1;

	STIX_ASSERT (stix->active_context != STIX_NULL);
	ip = STIX_OOP_TO_SMINT(stix->active_context->ip);
	sp = STIX_OOP_TO_SMINT(stix->active_context->sp);

	while (1)
	{
		receiver = stix->active_context->receiver;
		mth = stix->active_context->method;
		code = mth->code;

printf ("IP => %d ", (int)ip);
		bc = code->slot[ip++];
		cmd = bc >> 4;
		if (cmd == CMD_EXTEND)
		{
			cmd = bc & 0xF;
			b1 = code->slot[ip++];
		}
/* TODO: handle CMD_EXTEND_DOUBLE */
		else
		{
			b1 = bc & 0xF;
		}

printf ("CMD => %d, B1 = %d, IP AFTER INC %d\n", (int)cmd, (int)b1, (int)ip);
		switch (cmd)
		{

			case CMD_PUSH_INSTVAR:
printf ("PUSHING INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(receiver) == STIX_OBJ_TYPE_OOP);
				stix->active_context->slot[++sp] = ((stix_oop_oop_t)receiver)->slot[b1];
				break;

			case CMD_PUSH_TEMPVAR:
printf ("PUSHING TEMPVAR %d\n", (int)b1);
				stix->active_context->slot[++sp] = stix->active_context->slot[b1];
				break;

			case CMD_PUSH_LITERAL:
				stix->active_context->slot[++sp] = mth->slot[b1];
				break;

			case CMD_POP_AND_STORE_INTO_INSTVAR:
printf ("STORING INSTVAR %d\n", (int)b1);
				((stix_oop_oop_t)receiver)->slot[b1] = stix->active_context->slot[sp--];
				break;

			case CMD_POP_AND_STORE_INTO_TEMPVAR:
printf ("STORING TEMPVAR %d\n", (int)b1);
				stix->active_context->slot[b1] = stix->active_context->slot[sp--];
				break;

			/*
			case CMD_POP_AND_STORE_INTO_OBJECT_POINTED_TO_BY_LITERAL???
			*/

			case CMD_SEND_MESSAGE:
			{
				/* b1 -> number of arguments 
				 * b2 -> literal index of the message symbol
TODO: handle double extension 
				 */
				stix_ucs_t mthname;
				stix_oop_t newrcv;
				stix_oop_method_t newmth;
				stix_oop_char_t selector;
				stix_ooi_t selector_index;

printf ("SENDING MESSAGE   \n");
				selector_index = code->slot[ip++];

				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)mth->slot[selector_index];

				STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);
printf ("RECEIVER INDEX %d\n", (int)(sp - b1));

				newrcv = stix->active_context->slot[sp - b1];
				mthname.ptr = selector->slot;
				mthname.len = STIX_OBJ_GET_SIZE(selector);
				newmth = find_method (stix, newrcv, &mthname);
				if (!newmth) 
				{
/* TODO: implement doesNotUnderstand: XXXXX  instead of returning -1. */
printf ("no such method .........[");
print_ucs (&mthname);
printf ("]\n");
					return -1;
				}

				STIX_ASSERT (STIX_OOP_TO_SMINT(newmth->tmpr_nargs) == b1);
				if (activate_new_method (stix, newmth, &ip, &sp) <= -1) return -1;
				break;
			}

			case CMD_SEND_MESSAGE_TO_SUPER:
				/*b2 = code->slot[ip++];*/
				break;


			case CMD_PUSH_SPECIAL:
				switch (b1)
				{
					case SUBCMD_PUSH_RECEIVER:
printf ("PUSHING RECEIVER %p TO STACK INDEX %d\n", receiver, (int)sp);
						stix->active_context->slot[++sp] = receiver;
						break;

					case SUBCMD_PUSH_NIL:
						stix->active_context->slot[++sp] = stix->_nil;
						break;

					case SUBCMD_PUSH_TRUE:
printf ("PUSHING TRUE\n");
						stix->active_context->slot[++sp] = stix->_true;
						break;

					case SUBCMD_PUSH_FALSE:
						stix->active_context->slot[++sp] = stix->_false;
						break;
				}
				break;

			case CMD_DO_SPECIAL:
				switch (b1)
				{
					case SUBCMD_RETURN_MESSAGE_RECEIVER:
					{
						stix_oop_context_t ctx;

printf ("RETURNING. RECEIVER...........\n");
						ctx = stix->active_context; /* current context */
						ctx->ip = STIX_OOP_FROM_SMINT(ip);
						ctx->sp = STIX_OOP_FROM_SMINT(sp);

						stix->active_context = (stix_oop_context_t)ctx->sender; /* return to the sending context */
						if (stix->active_context->sender == stix->_nil) 
						{
printf ("RETURNIGN TO THE INITIAL CONTEXT.......\n");
							/* returning to the initial context */
							goto done;
						}

						ip = STIX_OOP_TO_SMINT(stix->active_context->ip);
						sp = STIX_OOP_TO_SMINT(stix->active_context->sp);
						stix->active_context->slot[++sp] = ctx->receiver;

						break;
					}

					case SUBCMD_RETURN_MESSAGE_STACKTOP:
					{
						stix_oop_context_t ctx;
						stix_oop_t top;

printf ("RETURNING. RECEIVER...........\n");
						ctx = stix->active_context; /* current context */
						top = ctx->slot[sp--];
						ctx->ip = STIX_OOP_FROM_SMINT(ip);
						ctx->sp = STIX_OOP_FROM_SMINT(sp);

						stix->active_context = (stix_oop_context_t)ctx->sender; /* return to the sending context */

						ip = STIX_OOP_TO_SMINT(stix->active_context->ip);
						sp = STIX_OOP_TO_SMINT(stix->active_context->sp);
						stix->active_context->slot[++sp] = top;

						if (stix->active_context->sender == stix->_nil) 
						{
printf ("RETURNIGN TO THE INITIAL CONTEXT.......\n");
							/* returning to the initial context */
							goto done;
						}

						break;
					}

					/*case CMD_RETURN_BLOCK_STACKTOP:*/
					
				}
				break;
		}
	}

done:
	return 0;
}

int stix_invoke (stix_t* stix, const stix_ucs_t* objname, const stix_ucs_t* mthname)
{
	if (activate_initial_context (stix, objname, mthname) <= -1) return -1;
	return stix_execute (stix);
}


