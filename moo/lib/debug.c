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

void moo_dumpsymtab (moo_t* moo)
{
	moo_oow_t i;
	moo_oop_char_t symbol;

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
	MOO_DEBUG1 (moo, "MOO Symbol Table %zu\n", MOO_OBJ_GET_SIZE(moo->symtab->bucket));
	MOO_DEBUG0 (moo, "--------------------------------------------\n");

	for (i = 0; i < MOO_OBJ_GET_SIZE(moo->symtab->bucket); i++)
	{
		symbol = (moo_oop_char_t)MOO_OBJ_GET_OOP_VAL(moo->symtab->bucket, i);
		if ((moo_oop_t)symbol != moo->_nil)
		{
			MOO_DEBUG2 (moo, " %07zu %O\n", i, symbol);
		}
	}

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
}

void moo_dumpdic (moo_t* moo, moo_oop_dic_t dic, const moo_bch_t* title)
{
	moo_oow_t i;
	moo_oop_association_t ass;

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
	MOO_DEBUG2 (moo, "%s %zu\n", title, MOO_OBJ_GET_SIZE(dic->bucket));
	MOO_DEBUG0 (moo, "--------------------------------------------\n");

	for (i = 0; i < MOO_OBJ_GET_SIZE(dic->bucket); i++)
	{
		ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, i);
		if ((moo_oop_t)ass != moo->_nil)
		{
			MOO_DEBUG2 (moo, " %07zu %O\n", i, ass->key);
		}
	}
	MOO_DEBUG0 (moo, "--------------------------------------------\n");
}


/* TODO: moo_loaddbginfofromimage() -> load debug information from compiled image?
moo_storedbginfotoimage()? -> store debug information to compiled image?
moo_compactdbginfo()? -> compact debug information by scaning dbginfo data. find class and method. if not found, drop the portion.
*/

int moo_initdbginfo (moo_t* moo, moo_oow_t capa)
{
	moo_dbginfo_t* info;

	info = (moo_dbginfo_t*)moo_allocmem(moo, MOO_SIZEOF(*info) + capa);
	if (!info) return -1;

	info->_capa = capa;
	info->_len = 0;

	moo->dbginfo = info;
	return 0;
}

void moo_finidbginfo (moo_t* moo)
{
	if (moo->dbginfo)
	{
		moo_freemem (moo, moo->dbginfo);
		moo->dbginfo = MOO_NULL;
	}
}

int moo_addmethodtodbginfo (moo_t* moo, const moo_uch_t* class_name, const moo_uch_t* method_name)
{
	return 0;
}