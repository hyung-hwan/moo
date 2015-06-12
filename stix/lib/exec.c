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

#define LOAD_IP_AND_SP(v_ctx, v_ip, v_sp) \
	do \
	{ \
		v_ip = STIX_OOP_TO_SMINT((v_ctx)->ip); \
		v_sp = STIX_OOP_TO_SMINT((v_ctx)->sp); \
	} while(0)

#define STORE_IP_AND_SP(v_ctx, v_ip, v_sp) \
	do \
	{ \
		(v_ctx)->ip = STIX_OOP_FROM_SMINT(v_ip); \
		(v_ctx)->sp = STIX_OOP_FROM_SMINT(v_sp); \
	} while(0)

#define LOAD_SP(v_ctx, v_sp) \
	do \
	{ \
		v_sp = STIX_OOP_TO_SMINT((v_ctx)->sp); \
	} while(0)

#define STORE_SP(v_ctx, v_sp) \
	do \
	{ \
		(v_ctx)->sp = STIX_OOP_FROM_SMINT(v_sp); \
	} while(0)


static int activate_new_method (stix_t* stix, stix_oop_method_t mth, stix_ooi_t* xip, stix_ooi_t* xsp)
{
	stix_oow_t stack_size;
	stix_oop_context_t ctx;
	stix_ooi_t i;
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
	for (i = nargs; i > 0; )
	{
		/* copy argument */
		ctx->slot[--i] = stix->active_context->slot[sp--];
	}
	/* copy receiver */
	ctx->receiver = stix->active_context->slot[sp--];
	STIX_ASSERT (sp >= -1);

	/* store an instruction pointer and a stack pointer to the active context
	 * before switching it to a new context */
	STORE_IP_AND_SP (stix->active_context, *xip, sp);

	/* swtich the active context */
	stix->active_context = ctx;

	/* load an instruction pointer and a stack pointer from the new active
	 * context */
	LOAD_IP_AND_SP (stix->active_context, *xip, *xsp);

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
		/* receiver is a class receiverect */
		c = receiver; 
		dic_no = STIX_CLASS_MTHDIC_CLASS;
printf ("class method dictioanry ====\n");
	}
	else
	{
		c = (stix_oop_t)cls;
		dic_no = STIX_CLASS_MTHDIC_INSTANCE;
printf ("instance method dictioanry ====\n");
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
	stix_ooi_t ip, sp;

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

	/* the initial context starts the life of the entire VM
	 * and is not really worked on except that it is used to call the
	 * initial method. so it doesn't really require any extra stack space.
	 * TODO: verify my theory above is true */
	ip = 0;
	sp = -1;

	ctx->slot[++sp] = ass->value; /* push receiver */
	STORE_IP_AND_SP (ctx, ip, sp);
	/* receiver, sender, method are nils */

	stix->active_context = ctx;
	return activate_new_method (stix, mth, &ip, &sp);
}

#define PUSH(stix,v) \
	do { \
		++sp; \
		(stix)->active_context->slot[sp] = v; \
	} while (0)

static int execute_primitive (stix_t* stix, int prim_no, stix_ooi_t nargs)
{
	/* a primitive handler must pop off all arguments and the receiver and
	 * push a return value when it's successful. otherwise, it must not touch
	 * the stack. */
	stix_ooi_t sp;

	LOAD_SP (stix->active_context, sp);
	switch (prim_no)
	{
		case 0:
		{
			stix_ooi_t i;
		
			dump_object (stix, stix->active_context->slot[sp - nargs], "receiver");
			for (i = nargs; i > 0; )
			{
				--i;
				dump_object (stix, stix->active_context->slot[sp - i], "argument");
			}

			sp -= nargs; /* pop off arguments */
			goto success;
		}

		default:
			/* this is hard failure */
			stix->errnum = STIX_ENOIMPL;
			return -1;
	}

/* TODO: when it returns 1, it should pop up receiver and argumetns.... 
 * when it returns 0, it should not touch the stack */
	return 0;

success:
	STORE_SP (stix->active_context, sp);
	return 1;
}


int stix_execute (stix_t* stix)
{
	stix_oop_method_t mth;
	stix_oop_byte_t code;
	stix_ooi_t ip, sp;

	stix_byte_t bc, cmd;
	stix_oow_t b1;

	STIX_ASSERT (stix->active_context != STIX_NULL);
	ip = STIX_OOP_TO_SMINT(stix->active_context->ip);
	sp = STIX_OOP_TO_SMINT(stix->active_context->sp);

	/* store the address of the stack pointer variable. 
	 * this address is used to update the sp of the active context
	 * before actual garbage collection is collected. */
	stix->active_context_sp = &sp;

/* TODO: stack popping requires the elelement to be reset to nil or smallinteger.
 *       otherwide, the element beyond the stack top (sp) won't be GCed.
 */
	while (1)
	{
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

printf ("CMD => %d, B1 = %d, SP = %d, IP AFTER INC %d\n", (int)cmd, (int)b1, (int)sp, (int)ip);
		switch (cmd)
		{

			case CMD_PUSH_INSTVAR:
printf ("PUSH_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver) == STIX_OBJ_TYPE_OOP);
				PUSH (stix, ((stix_oop_oop_t)stix->active_context->receiver)->slot[b1]);
				break;

			case CMD_PUSH_TEMPVAR:
printf ("PUSH_TEMPVAR %d\n", (int)b1);
				PUSH (stix, stix->active_context->slot[b1]);
				break;

			case CMD_PUSH_LITERAL:
printf ("PUSH_LITERAL %d\n", (int)b1);
				PUSH (stix, mth->slot[b1]);
				break;

			case CMD_STORE_INTO_INSTVAR:
printf ("STORE_INSTVAR %d\n", (int)b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->receiver)->slot[b1] = stix->active_context->slot[sp];
				break;

			case CMD_STORE_INTO_TEMPVAR:
printf ("STORE_TEMPVAR %d\n", (int)b1);
				stix->active_context->slot[b1] = stix->active_context->slot[sp];
				break;

			/*
			case CMD_POP_AND_STORE_INTO_OBJECT_POINTED_TO_BY_LITERAL???
			*/

		/* -------------------------------------------------------- */

			case CMD_PUSH_OBJVAR:
			{
/* TODO: HANDLE DOUBLE EXTEND */
				/* b1 -> variable index */
				stix_ooi_t obj_index;
				stix_oop_oop_t obj;
				obj_index = code->slot[ip++];

				obj = (stix_oop_oop_t)mth->slot[obj_index];
printf ("PUSH OBJVAR %d %d\n", (int)b1, (int)obj_index);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(obj) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (obj_index < STIX_OBJ_GET_SIZE(obj));
				PUSH (stix, obj->slot[b1]);
				break;
			}

			case CMD_STORE_INTO_OBJVAR:
			{
				stix_ooi_t obj_index;
				stix_oop_oop_t obj;
				obj_index = code->slot[ip++];

printf ("STORE OBJVAR %d %d\n", (int)b1, (int)obj_index);
				obj = (stix_oop_oop_t)mth->slot[obj_index];
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(obj) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (obj_index < STIX_OBJ_GET_SIZE(obj));
				obj->slot[b1] = stix->active_context->slot[sp];
				break;
			}

		/* -------------------------------------------------------- */
			case CMD_SEND_MESSAGE:
			case CMD_SEND_MESSAGE_TO_SUPER:
			{
				/* b1 -> number of arguments 
TODO: handle double extension 
				 */
				stix_ucs_t mthname;
				stix_oop_t newrcv;
				stix_oop_method_t newmth;
				stix_oop_char_t selector;
				stix_ooi_t selector_index;
				stix_ooi_t preamble;


				/* the next byte is the message selector index to the
				 * literal frame. */
				selector_index = code->slot[ip++];

				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)mth->slot[selector_index];

if (cmd == CMD_SEND_MESSAGE)
printf ("SEND_MESSAGE TO RECEIVER AT %d\n", (int)(sp - b1));
else
printf ("SEND_MESSAGE_TO_SUPER TO RECEIVER AT %d\n", (int)(sp - b1));
				STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);

				newrcv = stix->active_context->slot[sp - b1];
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
						sp = sp - b1; /* pop arguments */
						break;

					case STIX_METHOD_PREAMBLE_RETURN_INSTVAR:
					{
						stix_oop_oop_t receiver;

						sp = sp - b1; /* pop arguments */

printf ("RETURN INSTVAR AT PREAMBLE\n");
						/* replace the receiver by an instance variable of the receiver */
						receiver = (stix_oop_oop_t)stix->active_context->slot[sp];
						STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(receiver) == STIX_OBJ_TYPE_OOP);
						STIX_ASSERT (STIX_OBJ_GET_SIZE(receiver) > STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
						stix->active_context->slot[sp] = receiver->slot[STIX_METHOD_GET_PREAMBLE_INDEX(preamble)];
						break;
					}

					case STIX_METHOD_PREAMBLE_PRIMITIVE:
					{
						int n;

printf ("JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ\n");
						stix_pushtmp (stix, (stix_oop_t*)&newmth);
						STORE_SP (stix->active_context, sp);
						n = execute_primitive (stix, STIX_METHOD_GET_PREAMBLE_INDEX(preamble), b1);
						LOAD_SP (stix->active_context, sp);
						stix_poptmp (stix);

						if (n <= -1) goto oops;
						if (n >= 1) break;
						/* primitive failed. fall thru */
					}

					default:
						if (activate_new_method (stix, newmth, &ip, &sp) <= -1) goto oops;
						break;
				}

				break; /* CMD_SEND_MESSAGE */
			}

		/* -------------------------------------------------------- */

			case CMD_PUSH_SPECIAL:
				switch (b1)
				{
					case SUBCMD_PUSH_RECEIVER:
printf ("PUSH_RECEIVER %p TO STACK INDEX %d\n", stix->active_context->receiver, (int)sp);
						PUSH (stix, stix->active_context->receiver);
						break;

					case SUBCMD_PUSH_NIL:
printf ("PUSH_NIL\n");
						PUSH (stix, stix->_nil);
						break;

					case SUBCMD_PUSH_TRUE:
printf ("PUSH_TRUE\n");
						PUSH (stix, stix->_true);
						break;

					case SUBCMD_PUSH_FALSE:
printf ("PUSH_FALSE\n");
						PUSH (stix, stix->_false);
						break;

					case SUBCMD_PUSH_CONTEXT:
printf ("PUSH_CONTEXT\n");
						PUSH (stix, (stix_oop_t)stix->active_context);
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
						STIX_ASSERT (sp > -1);
						sp--;
						break;

					case SUBCMD_RETURN_STACKTOP:
printf ("RETURN_STACKTOP\n");
						return_value = stix->active_context->slot[sp--];
						goto handle_return;

					case SUBCMD_RETURN_RECEIVER:
printf ("RETURN_RECEIVER\n");
						return_value = stix->active_context->receiver;
						goto handle_return;

					/*case CMD_RETURN_BLOCK_STACKTOP:*/
						/* TODO: */

					default:
						stix->errnum = STIX_EINTERN;
						break;


					handle_return:
						/* store the instruction pointer and the stack pointer to the active context */
						STORE_IP_AND_SP (stix->active_context, ip, sp);

						/* switch the active context to the sending context */
						stix->active_context = (stix_oop_context_t)stix->active_context->sender;

						/* load the instruction pointer and the stack pointer from the new active context */
						LOAD_IP_AND_SP (stix->active_context, ip, sp);

						/* push the return value to the stack of the new active context */
						PUSH (stix, return_value);

printf ("<<LEAVING>>\n");
						if (stix->active_context->sender == stix->_nil) 
						{
							/* the sending context of the intial context has been set to nil.
							 * use this fact to tell an initial context from a normal context. */
printf ("<<<RETURNIGN TO THE INITIAL CONTEXT>>>\n");
							STIX_ASSERT (sp == 0);
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
