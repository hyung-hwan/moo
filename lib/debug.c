/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
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


/* TODO: moo_loaddbgifromimage() -> load debug information from compiled image?
moo_storedbgitoimage()? -> store debug information to compiled image?
moo_compactdbgi()? -> compact debug information by scaning dbgi data. find class and method. if not found, drop the portion.
*/

int moo_initdbgi (moo_t* moo, moo_oow_t capa)
{
	moo_dbgi_t* tmp;

	if (capa < MOO_SIZEOF(*tmp)) capa = MOO_SIZEOF(*tmp);

	tmp = (moo_dbgi_t*)moo_callocmem(moo, capa);
	if (!tmp) return -1;

	tmp->_capa = capa;
	tmp->_len = MOO_SIZEOF(*tmp);
	/* tmp->_last_file = 0;
	tmp->_last_class = 0;
	tmp->_last_text = 0;
	tmp->_last_method = 0; */

	moo->dbgi = tmp;
	return 0;
}

void moo_finidbgi (moo_t* moo)
{
	if (moo->dbgi)
	{
		moo_freemem (moo, moo->dbgi);
		moo->dbgi = MOO_NULL;
	}
}

static MOO_INLINE moo_uint8_t* secure_dbgi_space (moo_t* moo, moo_oow_t req_bytes)
{
	if (moo->dbgi->_capa - moo->dbgi->_len < req_bytes)
	{
		moo_dbgi_t* tmp;
		moo_oow_t newcapa;

		newcapa = moo->dbgi->_len + req_bytes;
		newcapa = MOO_ALIGN_POW2(newcapa, 65536); /* TODO: make the align value configurable */
		tmp = moo_reallocmem(moo, moo->dbgi, newcapa);
		if (!tmp) return MOO_NULL;

		moo->dbgi = tmp;
		moo->dbgi->_capa = newcapa;
	}

	return &((moo_uint8_t*)moo->dbgi)[moo->dbgi->_len];
}

int moo_addfiletodbgi (moo_t* moo, const moo_ooch_t* file_name, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbgi_file_t* di;

	if (!moo->dbgi) 
	{
		if (start_offset) *start_offset = 0;
		return 0; /* debug information is disabled*/
	}

	if (moo->dbgi->_last_file > 0)
	{
		/* TODO: avoid linear search. need indexing for speed up */
		moo_oow_t offset = moo->dbgi->_last_file;
		do
		{
			di = (moo_dbgi_file_t*)&((moo_uint8_t*)moo->dbgi)[offset];
			if (moo_comp_oocstr((moo_ooch_t*)(di + 1), file_name) == 0) 
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
	req_bytes = MOO_SIZEOF(moo_dbgi_file_t) + name_bytes_aligned;

	di = (moo_dbgi_file_t*)secure_dbgi_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGI_MAKE_TYPE(MOO_DBGI_TYPE_CODE_FILE, 0);
	di->_len = req_bytes;
	di->_next = moo->dbgi->_last_file;
	moo_copy_oocstr ((moo_ooch_t*)(di + 1), name_len + 1, file_name);

	moo->dbgi->_last_file = moo->dbgi->_len;
	moo->dbgi->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbgi->_last_file;
	return 0;
}

int moo_addclasstodbgi (moo_t* moo, const moo_ooch_t* class_name, moo_oow_t file_offset, moo_oow_t file_line, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbgi_class_t* di;

	if (!moo->dbgi) return 0; /* debug information is disabled*/

	if (moo->dbgi->_last_class > 0)
	{
		/* TODO: avoid linear search. need indexing for speed up */
		moo_oow_t offset = moo->dbgi->_last_class;
		do
		{
			di = (moo_dbgi_class_t*)&((moo_uint8_t*)moo->dbgi)[offset];
			if (moo_comp_oocstr((moo_ooch_t*)(di + 1), class_name) == 0 && di->_file == file_offset && di->_line == file_line) 
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
	req_bytes = MOO_SIZEOF(moo_dbgi_class_t) + name_bytes_aligned;

	di = (moo_dbgi_class_t*)secure_dbgi_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGI_MAKE_TYPE(MOO_DBGI_TYPE_CODE_CLASS, 0);
	di->_len = req_bytes;
	di->_next = moo->dbgi->_last_class;
	di->_file = file_offset;
	di->_line = file_line;
	moo_copy_oocstr ((moo_ooch_t*)(di + 1), name_len + 1, class_name);

	moo->dbgi->_last_class = moo->dbgi->_len;
	moo->dbgi->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbgi->_last_class;
	return 0;
}

int moo_addmethodtodbgi (moo_t* moo, moo_oow_t file_offset, moo_oow_t class_offset, const moo_ooch_t* method_name, moo_oow_t start_line, const moo_oow_t* code_loc_ptr, moo_oow_t code_loc_len, const moo_ooch_t* text_ptr, moo_oow_t text_len, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, code_loc_bytes, code_loc_bytes_aligned, text_bytes, text_bytes_aligned, req_bytes;
	moo_dbgi_method_t* di;
	moo_uint8_t* curptr;

	if (!moo->dbgi) return 0; /* debug information is disabled*/

	name_len = moo_count_oocstr(method_name);
	name_bytes = (name_len + 1) * MOO_SIZEOF(*method_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	code_loc_bytes = code_loc_len * MOO_SIZEOF(*code_loc_ptr);
	code_loc_bytes_aligned = MOO_ALIGN_POW2(code_loc_bytes, MOO_SIZEOF_OOW_T);
	text_bytes = text_len * MOO_SIZEOF(*text_ptr);
	text_bytes_aligned = MOO_ALIGN_POW2(text_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbgi_method_t) + name_bytes_aligned + code_loc_bytes_aligned + text_bytes_aligned;

	di = (moo_dbgi_method_t*)secure_dbgi_space(moo, req_bytes);
	if (!di) return -1;

	di->_type = MOO_DBGI_MAKE_TYPE(MOO_DBGI_TYPE_CODE_METHOD, 0);
	di->_len = req_bytes;
	di->_next = moo->dbgi->_last_method;
	di->_file = file_offset;
	di->_class = class_offset;
	di->start_line = start_line;
	di->code_loc_start = name_bytes_aligned; /* distance from the beginning of the variable payload */
	di->code_loc_len = code_loc_len;
	di->text_start = name_bytes_aligned + code_loc_bytes_aligned; /* distance from the beginning of the variable payload */
	di->text_len = text_len;

	curptr = (moo_uint8_t*)(di + 1);
	moo_copy_oocstr ((moo_ooch_t*)curptr, name_len + 1, method_name);

	curptr += name_bytes_aligned;
	MOO_MEMCPY (curptr, code_loc_ptr, code_loc_bytes);

	if (text_len > 0)
	{
		curptr += code_loc_bytes_aligned;
		moo_copy_oochars ((moo_ooch_t*)curptr, text_ptr, text_len);
	}

	moo->dbgi->_last_method = moo->dbgi->_len;
	moo->dbgi->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbgi->_last_method;
	return 0;
}
