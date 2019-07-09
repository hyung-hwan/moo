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
	moo_dbginfo_t* tmp;

	if (capa < MOO_SIZEOF(*tmp)) capa = MOO_SIZEOF(*tmp);

	tmp = (moo_dbginfo_t*)moo_callocmem(moo, capa);
	if (!tmp) return -1;

	tmp->_capa = capa;
	tmp->_len = MOO_SIZEOF(*tmp);
	tmp->_last_class = 0;
	tmp->_last_file = 0;

	moo->dbginfo = tmp;
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

static MOO_INLINE moo_uint8_t* secure_dbginfo_space (moo_t* moo, moo_oow_t req_bytes)
{
	if (moo->dbginfo->_capa - moo->dbginfo->_len < req_bytes)
	{
		moo_dbginfo_t* tmp;
		moo_oow_t newcapa;

		newcapa = moo->dbginfo->_len + req_bytes;
		newcapa = MOO_ALIGN_POW2(newcapa, 65536); /* TODO: make the align value configurable */
		tmp = moo_reallocmem(moo, moo->dbginfo, newcapa);
		if (!tmp) return MOO_NULL;

		moo->dbginfo = tmp;
		moo->dbginfo->_capa = newcapa;
	}

	return &((moo_uint8_t*)moo->dbginfo)[moo->dbginfo->_len];
}

int moo_addfiletodbginfo (moo_t* moo, const moo_ooch_t* file_name, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbginfo_file_t* di;

	if (!moo->dbginfo) 
	{
		if (start_offset) *start_offset = MOO_NULL;
		return 0; /* debug information is disabled*/
	}

	if (moo->dbginfo->_last_file > 0)
	{
		/* TODO: avoid linear search. need indexing for speed up */
		moo_oow_t offset = moo->dbginfo->_last_file;
		do
		{
			di = &((moo_uint8_t*)moo->dbginfo)[offset];
			if (moo_comp_oocstr(di + 1, file_name) == 0) 
			{
				if (start_offset) *start_offset = offset;
				return 0;
			}
			offset = di->_next;
		}
		while (offset > 0);
	}

	name_len = moo_count_oocstr(file_name);
	name_bytes = (name_len + 1) * MOO_SIZEOF(*file_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_file_t) + name_bytes_aligned;

	di = (moo_dbginfo_file_t*)secure_dbginfo_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_FILE, 0);
	di->_len = req_bytes;
	di->_next = moo->dbginfo->_last_file;
	moo_copy_oocstr (di + 1, name_len + 1, file_name);

	moo->dbginfo->_last_file = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_file;
	return 0;
}

int moo_addclasstodbginfo (moo_t* moo, const moo_ooch_t* class_name, moo_oow_t file_offset, moo_oow_t file_line, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbginfo_class_t* di;

	if (!moo->dbginfo) return 0; /* debug information is disabled*/

	if (moo->dbginfo->_last_class > 0)
	{
		/* TODO: avoid linear search. need indexing for speed up */
		moo_oow_t offset = moo->dbginfo->_last_class;
		do
		{
			di = &((moo_uint8_t*)moo->dbginfo)[offset];
			if (moo_comp_oocstr(di + 1, class_name) == 0 && di->_file == file_offset && di->_line == file_line) 
			{
				if (start_offset) *start_offset = offset;
				return 0;
			}
			offset = di->_next;
		}
		while (offset > 0);
	}

	name_len = moo_count_oocstr(class_name);
	name_bytes = (name_len + 1) * MOO_SIZEOF(*class_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_class_t) + name_bytes_aligned;

	di = (moo_dbginfo_class_t*)secure_dbginfo_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_CLASS, 0);
	di->_len = req_bytes;
	di->_next = moo->dbginfo->_last_class;
	di->_file = file_offset;
	di->_line = file_line;
	moo_copy_oocstr (di + 1, name_len + 1, class_name);

	moo->dbginfo->_last_class = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_class;
	return 0;
}

int moo_addmethodtodbginfo (moo_t* moo, moo_oow_t file_offset, moo_oow_t class_offset, const moo_ooch_t* method_name, const moo_oow_t* code_loc_ptr, moo_oow_t code_loc_len, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, code_loc_bytes, code_loc_bytes_aligned, req_bytes;
	moo_dbginfo_method_t* di;

	if (!moo->dbginfo) return 0; /* debug information is disabled*/

	name_len = moo_count_oocstr(method_name);
	name_bytes = (name_len + 1) * MOO_SIZEOF(*method_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	code_loc_bytes = code_loc_len * MOO_SIZEOF(*code_loc_ptr);
	code_loc_bytes_aligned = MOO_ALIGN_POW2(code_loc_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_method_t) + name_bytes_aligned + code_loc_bytes_aligned;

	di = (moo_dbginfo_method_t*)secure_dbginfo_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_METHOD, 0);
	di->_len = req_bytes;
	di->_next = moo->dbginfo->_last_method;
	di->_file = file_offset;
	di->_class = class_offset;
	di->code_loc_len = code_loc_len;

	moo_copy_oocstr (di + 1, name_len + 1, method_name);
	MOO_MEMCPY ((moo_uint8_t*)(di + 1) + name_bytes_aligned, code_loc_ptr, code_loc_bytes);

	moo->dbginfo->_last_method = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_method;
	return 0;
}
