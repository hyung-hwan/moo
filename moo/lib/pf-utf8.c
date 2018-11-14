/*
 * $Id$
 *
    Copyright (c) 2014-2018 Chung, Hyung-Hwan. All rights reserved.

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


moo_pfrc_t moo_pf_utf8_seqlen (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_uch_t uch;
	const moo_bch_t* utf8;
	moo_oow_t size;
	int n;

	/* the receiver should be a byte array or the like.  */
	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, rcv, moo->_byte_array));

	utf8 = (const moo_bch_t*)MOO_OBJ_GET_BYTE_SLOT(rcv);
	size = MOO_OBJ_GET_SIZE(rcv);


#if 0
	/* TODO: check the first byte and return bytes required */
	if ((n = moo_utf8_to_uc(utf8, size, &uch)) == 0)
	{
		/* invalid sequence */
		moo_seterrnum (moo, MOO_EECERR);
		return MOO_PF_FAILURE;
	}
#endif

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(n));
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_utf8_to_uc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_uch_t uch;
	const moo_bch_t* utf8;
	moo_oow_t size;
	int n;

	/* the receiver should be a byte array or the like.  */
	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, rcv, moo->_byte_array));

/* TODO: accept offset and size */
	utf8 = (const moo_bch_t*)MOO_OBJ_GET_BYTE_SLOT(rcv);
	size = MOO_OBJ_GET_SIZE(rcv);

	if ((n = moo_utf8_to_uc(utf8, size, &uch)) == 0)
	{
		/* invalid sequence */
		moo_seterrnum (moo, MOO_EECERR);
		return MOO_PF_FAILURE;
	}
	else if (n >= size)
	{
		/* incomplete sequence */
		moo_seterrnum (moo, MOO_EECMORE);
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRET(moo, nargs, MOO_CHAR_TO_OOP(uch));
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_uc_to_utf8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs)
{
	MOO_STACK_SETRET (moo, nargs, moo->_nil);
	/* TODO: */
	return MOO_PF_SUCCESS;
}
