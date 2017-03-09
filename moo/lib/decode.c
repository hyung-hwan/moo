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

#include "moo-prv.h"

#define DECODE_LOG_MASK (MOO_LOG_MNEMONIC)

#if defined(NDEBUG)
	/* get rid of instruction logging regardless of the log mask
	 * in the release build */
#	define LOG_INST_0(moo,fmt)
#	define LOG_INST_1(moo,fmt,a1)
#	define LOG_INST_2(moo,fmt,a1,a2)
#	define LOG_INST_3(moo,fmt,a1,a2,a3)
#else
#	define LOG_INST_0(moo,fmt) MOO_LOG1(moo, DECODE_LOG_MASK, " %06zd " fmt "\n", fetched_instruction_pointer)
#	define LOG_INST_1(moo,fmt,a1) MOO_LOG2(moo, DECODE_LOG_MASK, " %06zd " fmt "\n", fetched_instruction_pointer, a1)
#	define LOG_INST_2(moo,fmt,a1,a2) MOO_LOG3(moo, DECODE_LOG_MASK, " %06zd " fmt "\n", fetched_instruction_pointer, a1, a2)
#	define LOG_INST_3(moo,fmt,a1,a2,a3) MOO_LOG4(moo, DECODE_LOG_MASK, " %06zd " fmt "\n", fetched_instruction_pointer, a1, a2, a3)
#endif

#define FETCH_BYTE_CODE(moo) (cdptr[ip++])
#define FETCH_BYTE_CODE_TO(moo,v_oow) (v_oow = FETCH_BYTE_CODE(moo))
#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
#	define FETCH_PARAM_CODE_TO(moo,v_oow) \
		do { \
			v_oow = FETCH_BYTE_CODE(moo); \
			v_oow = (v_oow << 8) | FETCH_BYTE_CODE(moo); \
		} while (0)
#else
#	define FETCH_PARAM_CODE_TO(moo,v_oow) (v_oow = FETCH_BYTE_CODE(moo))
#endif

/* TODO: check if ip shoots beyond the maximum length in fetching code and parameters */
int moo_decode (moo_t* moo, moo_oop_method_t mth, const moo_oocs_t* classfqn)
{
	moo_oob_t bcode, * cdptr;
	moo_ooi_t ip = 0, cdlen; /* byte code length is limited by the compiler. so moo_ooi_t is good enough */
	moo_ooi_t fetched_instruction_pointer;
	moo_oow_t b1, b2;
 
	cdptr = MOO_METHOD_GET_CODE_BYTE(mth);
	cdlen = MOO_METHOD_GET_CODE_SIZE(mth); 

	if (classfqn)
		MOO_LOG3 (moo, DECODE_LOG_MASK, "%.*js>>%O\n", classfqn->len, classfqn->ptr, mth->name);
	else
		MOO_LOG2 (moo, DECODE_LOG_MASK, "%O>>%O\n", mth->owner, mth->name);

/* TODO: check if ip increases beyond bcode when fetching parameters too */
	while (ip < cdlen)
	{
		fetched_instruction_pointer = ip;
		FETCH_BYTE_CODE_TO(moo, bcode);

		switch (bcode)
		{
			case BCODE_PUSH_INSTVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				LOG_INST_1 (moo, "push_instvar %zu", b1);
				break;

			/* ------------------------------------------------- */

			case BCODE_STORE_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				LOG_INST_1 (moo, "store_into_instvar %zu", b1);
				break;

			case BCODE_POP_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				LOG_INST_1 (moo, "pop_into_instvar %zu", b1);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_TEMPVAR_X:
			case BCODE_STORE_INTO_TEMPVAR_X:
			case BCODE_POP_INTO_TEMPVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				b1 = bcode & 0x7; /* low 3 bits */
			handle_tempvar:

				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on */
					LOG_INST_1 (moo, "push_tempvar %zu", b1);
				}
				else
				{
					/* store or pop - bit 5 off */
					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						LOG_INST_1 (moo, "pop_into_tempvar %zu", b1);
					}
					else
					{
						LOG_INST_1 (moo, "store_into_tempvar %zu", b1);
					}
				}
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_LITERAL_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				LOG_INST_1 (moo, "push_literal @%zu", b1);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_OBJECT_X:
			case BCODE_STORE_INTO_OBJECT_X:
			case BCODE_POP_INTO_OBJECT_X:
				FETCH_PARAM_CODE_TO (moo, b1);
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
				b1 = bcode & 0x3; /* low 2 bits */
			handle_object:
				if ((bcode >> 3) & 1)
				{
					if ((bcode >> 2) & 1)
					{
						LOG_INST_1 (moo, "pop_into_object @%zu", b1);
					}
					else
					{
						LOG_INST_1 (moo, "store_into_object @%zu", b1);
					}
				}
				else
				{
					LOG_INST_1 (moo, "push_object @%zu", b1);
				}
				break;

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump_forward %zu", b1);
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
				LOG_INST_1 (moo, "jump_forward %zu", (moo_oow_t)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump_backward %zu", b1);
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
				LOG_INST_1 (moo, "jump_backward %zu", (moo_oow_t)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_FORWARD_IF_FALSE_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump_forward_if_false %zu", b1);
				break;

			case BCODE_JUMP_FORWARD_IF_FALSE_0:
			case BCODE_JUMP_FORWARD_IF_FALSE_1:
			case BCODE_JUMP_FORWARD_IF_FALSE_2:
			case BCODE_JUMP_FORWARD_IF_FALSE_3:
				LOG_INST_1 (moo, "jump_forward_if_false %zu", (moo_oow_t)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_IF_FALSE_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump_backward_if_false %zu", b1);
				break;

			case BCODE_JUMP_BACKWARD_IF_FALSE_0:
			case BCODE_JUMP_BACKWARD_IF_FALSE_1:
			case BCODE_JUMP_BACKWARD_IF_FALSE_2:
			case BCODE_JUMP_BACKWARD_IF_FALSE_3:
				LOG_INST_1 (moo, "jump_backward_if_false %zu", (moo_oow_t)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_IF_TRUE_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump_backward_if_true %zu", b1);
				break;

			case BCODE_JUMP_BACKWARD_IF_TRUE_0:
			case BCODE_JUMP_BACKWARD_IF_TRUE_1:
			case BCODE_JUMP_BACKWARD_IF_TRUE_2:
			case BCODE_JUMP_BACKWARD_IF_TRUE_3:
				LOG_INST_1 (moo, "jump_backward_if_true %zu", (moo_oow_t)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP2_FORWARD:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump2_forward %zu", b1);
				break;

			case BCODE_JUMP2_BACKWARD:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump2_backward %zu", b1);
				break;

			case BCODE_JUMP2_FORWARD_IF_FALSE:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump2_forward_if_false %zu", b1);
				break;

			case BCODE_JUMP2_BACKWARD_IF_FALSE:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump2_backward_if_false %zu", b1);
				break;

			case BCODE_JUMP2_BACKWARD_IF_TRUE:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "jump2_backward_if_true %zu", b1);
				break;
			/* -------------------------------------------------------- */

			case BCODE_PUSH_CTXTEMPVAR_X:
			case BCODE_STORE_INTO_CTXTEMPVAR_X:
			case BCODE_POP_INTO_CTXTEMPVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				FETCH_PARAM_CODE_TO (moo, b2);
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
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (moo, b2);

			handle_ctxtempvar:
				if ((bcode >> 3) & 1)
				{
					/* store or pop */

					if ((bcode >> 2) & 1)
					{
						LOG_INST_2 (moo, "pop_into_ctxtempvar %zu %zu", b1, b2);
					}
					else
					{
						LOG_INST_2 (moo, "store_into_ctxtempvar %zu %zu", b1, b2);
					}
				}
				else
				{
					/* push */
					LOG_INST_2 (moo, "push_ctxtempvar %zu %zu", b1, b2);
				}

				break;
			/* -------------------------------------------------------- */

			case BCODE_PUSH_OBJVAR_X:
			case BCODE_STORE_INTO_OBJVAR_X:
			case BCODE_POP_INTO_OBJVAR_X:
				FETCH_PARAM_CODE_TO (moo, b1);
				FETCH_PARAM_CODE_TO (moo, b2);
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
				/* b1 -> variable index to the object indicated by b2.
				 * b2 -> object index stored in the literal frame. */
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (moo, b2);

			handle_objvar:
				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					if ((bcode >> 2) & 1)
					{
						LOG_INST_2 (moo, "pop_into_objvar %zu %zu", b1, b2);
					}
					else
					{
						LOG_INST_2 (moo, "store_into_objvar %zu %zu", b1, b2);
					}
				}
				else
				{
					LOG_INST_2 (moo, "push_objvar %zu %zu", b1, b2);
				}

				break;

			/* -------------------------------------------------------- */
			case BCODE_SEND_MESSAGE_X:
			case BCODE_SEND_MESSAGE_TO_SUPER_X:
				/* b1 -> number of arguments 
				 * b2 -> selector index stored in the literal frame */
				FETCH_PARAM_CODE_TO (moo, b1);
				FETCH_PARAM_CODE_TO (moo, b2);
				goto handle_send_message;

			case BCODE_SEND_MESSAGE_0:
			case BCODE_SEND_MESSAGE_1:
			case BCODE_SEND_MESSAGE_2:
			case BCODE_SEND_MESSAGE_3:
			case BCODE_SEND_MESSAGE_TO_SUPER_0:
			case BCODE_SEND_MESSAGE_TO_SUPER_1:
			case BCODE_SEND_MESSAGE_TO_SUPER_2:
			case BCODE_SEND_MESSAGE_TO_SUPER_3:
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (moo, b2);

			handle_send_message:
				LOG_INST_3 (moo, "send_message%hs %zu @%zu", (((bcode >> 2) & 1)? "_to_super": ""), b1, b2);
				break; 

			/* -------------------------------------------------------- */

			case BCODE_PUSH_RECEIVER:
				LOG_INST_0 (moo, "push_receiver");
				break;

			case BCODE_PUSH_NIL:
				LOG_INST_0 (moo, "push_nil");
				break;

			case BCODE_PUSH_TRUE:
				LOG_INST_0 (moo, "push_true");
				break;

			case BCODE_PUSH_FALSE:
				LOG_INST_0 (moo, "push_false");
				break;

			case BCODE_PUSH_CONTEXT:
				LOG_INST_0 (moo, "push_context");
				break;

			case BCODE_PUSH_PROCESS:
				LOG_INST_0 (moo, "push_process");
				break;

			case BCODE_PUSH_NEGONE:
				LOG_INST_0 (moo, "push_negone");
				break;

			case BCODE_PUSH_ZERO:
				LOG_INST_0 (moo, "push_zero");
				break;

			case BCODE_PUSH_ONE:
				LOG_INST_0 (moo, "push_one");
				break;

			case BCODE_PUSH_TWO:
				LOG_INST_0 (moo, "push_two");
				break;

			case BCODE_PUSH_INTLIT:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "push_intlit %zu", b1);
				break;

			case BCODE_PUSH_NEGINTLIT:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "push_negintlit %zu", b1);
				break;

			case BCODE_PUSH_CHARLIT:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "push_charlit %zu", b1);
				break;
			/* -------------------------------------------------------- */

			case BCODE_MAKE_DICTIONARY:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "make_dictionary %zu", b1);
				break;

			case BCODE_POP_INTO_DICTIONARY:
				LOG_INST_0 (moo, "pop_into_dictionary");
				break;

			case BCODE_MAKE_ARRAY:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "make_array %zu", b1);
				break;

			case BCODE_POP_INTO_ARRAY:
				FETCH_PARAM_CODE_TO (moo, b1);
				LOG_INST_1 (moo, "pop_into_array %zu", b1);
				break;

			case BCODE_DUP_STACKTOP:
				LOG_INST_0 (moo, "dup_stacktop");
				break;

			case BCODE_POP_STACKTOP:
				LOG_INST_0 (moo, "pop_stacktop");
				break;

			case BCODE_RETURN_STACKTOP:
				LOG_INST_0 (moo, "return_stacktop");
				break;

			case BCODE_RETURN_RECEIVER:
				LOG_INST_0 (moo, "return_receiver");
				break;

			case BCODE_RETURN_FROM_BLOCK:
				LOG_INST_0 (moo, "return_from_block");
				break;

			case BCODE_LOCAL_RETURN:
				LOG_INST_0 (moo, "local_return");
				break;

			case BCODE_MAKE_BLOCK:
				/* b1 - number of block arguments
				 * b2 - number of block temporaries */
				FETCH_PARAM_CODE_TO (moo, b1);
				FETCH_PARAM_CODE_TO (moo, b2);

				LOG_INST_2 (moo, "make_block %zu %zu", b1, b2);

				MOO_ASSERT (moo, b1 >= 0);
				MOO_ASSERT (moo, b2 >= b1);
				break;

			case BCODE_SEND_BLOCK_COPY:
				LOG_INST_0 (moo, "send_block_copy");
				break;

			case BCODE_NOOP:
				/* do nothing */
				LOG_INST_0 (moo, "noop");
				break;

			default:
				LOG_INST_1 (moo, "UNKNOWN BYTE CODE ENCOUNTERED %x", (int)bcode);
				moo->errnum = MOO_EINTERN;
				break;

		}
	}

	/* print literal frame contents */
	for (ip = 0; ip < MOO_OBJ_GET_SIZE(mth) - MOO_METHOD_NAMED_INSTVARS; ip++)
	{
		 MOO_LOG2(moo, DECODE_LOG_MASK, " @%-5zd %O\n", ip, mth->slot[ip]);
	}

	return 0;
}

