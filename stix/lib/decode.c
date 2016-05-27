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

#	define DECODE_OUT_0(fmt) printf("\t" fmt "\n")
#	define DECODE_OUT_1(fmt,a1) printf("\t" fmt "\n",a1)
#	define DECODE_OUT_2(fmt,a1,a2) printf("\t" fmt "\n", a1, a2)
#	define DECODE_OUT_3(fmt,a1,a2,a3) printf("\t" fmt "\n", a1, a2, a3)


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
static int decode (stix_t* stix, stix_oop_method_t mth)
{
	stix_oob_t bcode, * cdptr;
	stix_oow_t ip = 0, cdlen;
	stix_ooi_t b1, b2;
 
	cdptr = STIX_METHOD_GET_CODE_BYTE(mth);
	cdlen = STIX_METHOD_GET_CODE_SIZE(mth); 

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
				DECODE_OUT_1 ("push_instvar %lu", (unsigned long int)b1);
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
				DECODE_OUT_1 ("store_into_instvar %lu", (unsigned long int)b1);
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
				DECODE_OUT_1 ("pop_into_instvar %lu", (unsigned long int)b1);
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
					DECODE_OUT_1 ("push_tempvar %d", (int)b1);
				}
				else
				{
					/* store or pop - bit 5 off */
					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						DECODE_OUT_1 ("pop_into_tempvar %d", (int)b1);
					}
					else
					{
						DECODE_OUT_1 ("store_into_tempvar %d", (int)b1);
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
				DECODE_OUT_1 ("push_literal %d", (int)b1);
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
						DECODE_OUT_1("pop_into_object %d", (int)b1);
					}
					else
					{
						DECODE_OUT_1("store_into_object %d", (int)b1);
					}
				}
				else
				{
					DECODE_OUT_1("push_object %d", (int)b1);
				}
				break;

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				DECODE_OUT_1 ("jump_forward %d", (int)b1);
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
				DECODE_OUT_1 ("jump_forward %d", (int)(bcode & 0x3)); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				DECODE_OUT_1 ("jump_backward %d", (int)b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
				DECODE_OUT_1 ("jump_backward %d", (int)(bcode & 0x3)); /* low 2 bits */
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
				DECODE_OUT_1 ("jump2_forward %d", (int)b1);
				break;

			case BCODE_JUMP2_BACKWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				DECODE_OUT_1 ("jump2_backward %d", (int)b1);
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
						DECODE_OUT_2 ("pop_into_ctxtempvar %d %d", (int)b1, (int)b2);
					}
					else
					{
						DECODE_OUT_2 ("store_into_ctxtempvar %d %d", (int)b1, (int)b2);
					}
				}
				else
				{
					/* push */
					DECODE_OUT_2 ("push_ctxtempvar %d %d", (int)b1, (int)b2);
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
						DECODE_OUT_2 ("pop_into_objvar %d %d", (int)b1, (int)b2);
					}
					else
					{
						DECODE_OUT_2 ("store_into_objvar %d %d", (int)b1, (int)b2);
					}
				}
				else
				{
					DECODE_OUT_2 ("push_objvar %d %d", (int)b1, (int)b2);
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
				DECODE_OUT_3 ("send_message%s nargs=%d selector=%d", (((bcode >> 2) & 1)? "_to_super": ""), (int)b1, (int)b2);
				break; 

			/* -------------------------------------------------------- */

			case BCODE_PUSH_RECEIVER:
				DECODE_OUT_0 ("push_receiver");
				break;

			case BCODE_PUSH_NIL:
				DECODE_OUT_0 ("push_nil");
				break;

			case BCODE_PUSH_TRUE:
				DECODE_OUT_0 ("push_true");
				break;

			case BCODE_PUSH_FALSE:
				DECODE_OUT_0 ("push_false");
				break;

			case BCODE_PUSH_CONTEXT:
				DECODE_OUT_0 ("push_context");
				break;

			case BCODE_PUSH_NEGONE:
				DECODE_OUT_0 ("push_negone");
				break;

			case BCODE_PUSH_ZERO:
				DECODE_OUT_0 ("push_zero");
				break;

			case BCODE_PUSH_ONE:
				DECODE_OUT_0 ("push_one");
				break;

			case BCODE_PUSH_TWO:
				DECODE_OUT_0 ("push_two");
				break;

			case BCODE_PUSH_INTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				DECODE_OUT_1 ("push_intlit %d", (int)b1);
				break;

			case BCODE_PUSH_NEGINTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				DECODE_OUT_1 ("push_negintlit %d", (int)-b1);
				break;

			/* -------------------------------------------------------- */

			case BCODE_DUP_STACKTOP:
				DECODE_OUT_0 ("dup_stacktop");
				break;

			case BCODE_POP_STACKTOP:
				DECODE_OUT_0 ("pop_stacktop");
				break;

			case BCODE_RETURN_STACKTOP:
				DECODE_OUT_0 ("return_stacktop");
				break;

			case BCODE_RETURN_RECEIVER:
				DECODE_OUT_0 ("return_receiver");
				break;

			case BCODE_RETURN_FROM_BLOCK:
				DECODE_OUT_0 ("return_from_block");
				break;

			case BCODE_MAKE_BLOCK:
				/* b1 - number of block arguments
				 * b2 - number of block temporaries */
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);

				DECODE_OUT_2 ("make_block %d %d", (int)b1, (int)b2);

				STIX_ASSERT (b1 >= 0);
				STIX_ASSERT (b2 >= b1);
				break;
			

			case BCODE_SEND_BLOCK_COPY:
				DECODE_OUT_0 ("send_block_copy");
				break;

			case BCODE_NOOP:
				/* do nothing */
				DECODE_OUT_0 ("noop");
				break;

			default:
printf ("UNKNOWN BYTE CODE ENCOUNTERED %x\n", (int)bcode);
				stix->errnum = STIX_EINTERN;
				break;

		}
	}

	/* print literal frame contents */
	for (ip = 0; ip < STIX_OBJ_GET_SIZE(mth) - STIX_METHOD_NAMED_INSTVARS; ip++)
	{
		printf ("  %8lu ", (unsigned long int)ip);
		print_object (stix, mth->slot[ip]);
		printf ("\n");
	}

	return 0;
}

int stix_decode (stix_t* stix, stix_oop_method_t mth)
{
	return decode (stix, mth);
}
