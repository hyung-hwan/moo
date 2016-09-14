/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

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


#define DECODE_LOG_MASK (STIX_LOG_MNEMONIC)

#define LOG_INST_0(stix,fmt) STIX_LOG0(stix, DECODE_LOG_MASK, "\t" fmt "\n")
#define LOG_INST_1(stix,fmt,a1) STIX_LOG1(stix, DECODE_LOG_MASK, "\t" fmt "\n",a1)
#define LOG_INST_2(stix,fmt,a1,a2) STIX_LOG2(stix, DECODE_LOG_MASK, "\t" fmt "\n", a1, a2)
#define LOG_INST_3(stix,fmt,a1,a2,a3) STIX_LOG3(stix, DECODE_LOG_MASK, "\t" fmt "\n", a1, a2, a3)

#define FETCH_BYTE_CODE(stix) (cdptr[ip++])
#define FETCH_BYTE_CODE_TO(stix,v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
#	define FETCH_PARAM_CODE_TO(stix,v_ooi) \
		do { \
			v_ooi = FETCH_BYTE_CODE(stix); \
			v_ooi = (v_ooi << 8) | FETCH_BYTE_CODE(stix); \
		} while (0)
#else
#	define FETCH_PARAM_CODE_TO(stix,v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#endif

/* TODO: check if ip shoots beyond the maximum length in fetching code and parameters */
int stix_decode (stix_t* stix, stix_oop_method_t mth, const stix_oocs_t* classfqn)
{
	stix_oob_t bcode, * cdptr;
	stix_oow_t ip = 0, cdlen;
	stix_ooi_t b1, b2;
 
	cdptr = STIX_METHOD_GET_CODE_BYTE(mth);
	cdlen = STIX_METHOD_GET_CODE_SIZE(mth); 

	if (classfqn)
		STIX_LOG3 (stix, DECODE_LOG_MASK, "%.*S>>%O\n", classfqn->len, classfqn->ptr, mth->name);
	else
		STIX_LOG2 (stix, DECODE_LOG_MASK, "%O>>%O\n", mth->owner, mth->name);

/* TODO: check if ip increases beyon bcode when fetching parameters too */
	while (ip < cdlen)
	{
		FETCH_BYTE_CODE_TO(stix, bcode);

		switch (bcode)
		{
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
				LOG_INST_1 (stix, "push_instvar %zd", b1);
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
				LOG_INST_1 (stix, "store_into_instvar %zd", b1);
				break;

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
				LOG_INST_1 (stix, "pop_into_instvar %zd", b1);
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
				b1 = bcode & 0x7; /* low 3 bits */
			handle_tempvar:

				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on */
					LOG_INST_1 (stix, "push_tempvar %zd", b1);
				}
				else
				{
					/* store or pop - bit 5 off */
					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						LOG_INST_1 (stix, "pop_into_tempvar %zd", b1);
					}
					else
					{
						LOG_INST_1 (stix, "store_into_tempvar %zd", b1);
					}
				}
				break;

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
				LOG_INST_1 (stix, "push_literal @%zd", b1);
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
				b1 = bcode & 0x3; /* low 2 bits */
			handle_object:
				if ((bcode >> 3) & 1)
				{
					if ((bcode >> 2) & 1)
					{
						LOG_INST_1 (stix, "pop_into_object @%zd", b1);
					}
					else
					{
						LOG_INST_1 (stix, "store_into_object @%zd", b1);
					}
				}
				else
				{
					LOG_INST_1 (stix, "push_object @%zd", b1);
				}
				break;

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump_forward %zd", b1);
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
				LOG_INST_1 (stix, "jump_forward %zd", (bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump_backward %zd", b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
				LOG_INST_1 (stix, "jump_backward %zd", (bcode & 0x3)); /* low 2 bits */
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
LOG_INST_0 (stix, "<<<<<<<<<<<<<< JUMP NOT IMPLEMENTED YET >>>>>>>>>>>>");
stix->errnum = STIX_ENOIMPL;
return -1;

			case BCODE_JUMP2_FORWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump2_forward %zd", b1);
				break;

			case BCODE_JUMP2_BACKWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump2_backward %zd", b1);
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
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_ctxtempvar:
				if ((bcode >> 3) & 1)
				{
					/* store or pop */

					if ((bcode >> 2) & 1)
					{
						LOG_INST_2 (stix, "pop_into_ctxtempvar %zd %zd", b1, b2);
					}
					else
					{
						LOG_INST_2 (stix, "store_into_ctxtempvar %zd %zd", b1, b2);
					}
				}
				else
				{
					/* push */
					LOG_INST_2 (stix, "push_ctxtempvar %zd %zd", b1, b2);
				}

				break;
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
				/* b1 -> variable index to the object indicated by b2.
				 * b2 -> object index stored in the literal frame. */
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_objvar:
				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					if ((bcode >> 2) & 1)
					{
						LOG_INST_2 (stix, "pop_into_objvar %zd %zd", b1, b2);
					}
					else
					{
						LOG_INST_2 (stix, "store_into_objvar %zd %zd", b1, b2);
					}
				}
				else
				{
					LOG_INST_2 (stix, "push_objvar %zd %zd", b1, b2);
				}

				break;

			/* -------------------------------------------------------- */
			case BCODE_SEND_MESSAGE_X:
			case BCODE_SEND_MESSAGE_TO_SUPER_X:
				/* b1 -> number of arguments 
				 * b2 -> selector index stored in the literal frame */
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
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_send_message:
				LOG_INST_3 (stix, "send_message%hs %zd @%zd", (((bcode >> 2) & 1)? "_to_super": ""), b1, b2);
				break; 

			/* -------------------------------------------------------- */

			case BCODE_PUSH_RECEIVER:
				LOG_INST_0 (stix, "push_receiver");
				break;

			case BCODE_PUSH_NIL:
				LOG_INST_0 (stix, "push_nil");
				break;

			case BCODE_PUSH_TRUE:
				LOG_INST_0 (stix, "push_true");
				break;

			case BCODE_PUSH_FALSE:
				LOG_INST_0 (stix, "push_false");
				break;

			case BCODE_PUSH_CONTEXT:
				LOG_INST_0 (stix, "push_context");
				break;

			case BCODE_PUSH_PROCESS:
				LOG_INST_0 (stix, "push_process");
				break;

			case BCODE_PUSH_NEGONE:
				LOG_INST_0 (stix, "push_negone");
				break;

			case BCODE_PUSH_ZERO:
				LOG_INST_0 (stix, "push_zero");
				break;

			case BCODE_PUSH_ONE:
				LOG_INST_0 (stix, "push_one");
				break;

			case BCODE_PUSH_TWO:
				LOG_INST_0 (stix, "push_two");
				break;

			case BCODE_PUSH_INTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "push_intlit %zd", b1);
				break;

			case BCODE_PUSH_NEGINTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "push_negintlit %zd", -b1);
				break;

			case BCODE_PUSH_CHARLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "push_charlit %zd", b1);
				break;
			/* -------------------------------------------------------- */

			case BCODE_DUP_STACKTOP:
				LOG_INST_0 (stix, "dup_stacktop");
				break;

			case BCODE_POP_STACKTOP:
				LOG_INST_0 (stix, "pop_stacktop");
				break;

			case BCODE_RETURN_STACKTOP:
				LOG_INST_0 (stix, "return_stacktop");
				break;

			case BCODE_RETURN_RECEIVER:
				LOG_INST_0 (stix, "return_receiver");
				break;

			case BCODE_RETURN_FROM_BLOCK:
				LOG_INST_0 (stix, "return_from_block");
				break;

			case BCODE_MAKE_BLOCK:
				/* b1 - number of block arguments
				 * b2 - number of block temporaries */
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);

				LOG_INST_2 (stix, "make_block %zd %zd", b1, b2);

				STIX_ASSERT (b1 >= 0);
				STIX_ASSERT (b2 >= b1);
				break;
			

			case BCODE_SEND_BLOCK_COPY:
				LOG_INST_0 (stix, "send_block_copy");
				break;

			case BCODE_NOOP:
				/* do nothing */
				LOG_INST_0 (stix, "noop");
				break;

			default:
				LOG_INST_1 (stix, "UNKNOWN BYTE CODE ENCOUNTERED %x", (int)bcode);
				stix->errnum = STIX_EINTERN;
				break;

		}
	}

	/* print literal frame contents */
	for (ip = 0; ip < STIX_OBJ_GET_SIZE(mth) - STIX_METHOD_NAMED_INSTVARS; ip++)
	{
		LOG_INST_2 (stix, " @%-3lu %O", (unsigned long int)ip, mth->slot[ip]);
	}

	return 0;
}

