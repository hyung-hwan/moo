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

void stix_dumpsymtab (stix_t* stix)
{
	stix_oow_t i;
	stix_oop_char_t symbol;

	STIX_DEBUG0 (stix, "--------------------------------------------\n");
	STIX_DEBUG1 (stix, "Stix Symbol Table %zu\n", STIX_OBJ_GET_SIZE(stix->symtab->bucket));
	STIX_DEBUG0 (stix, "--------------------------------------------\n");

	for (i = 0; i < STIX_OBJ_GET_SIZE(stix->symtab->bucket); i++)
	{
		symbol = (stix_oop_char_t)stix->symtab->bucket->slot[i];
 		if ((stix_oop_t)symbol != stix->_nil)
		{
			STIX_DEBUG2 (stix, " %07zu %O\n", i, symbol);
		}
	}

	STIX_DEBUG0 (stix, "--------------------------------------------\n");
}

void stix_dumpdic (stix_t* stix, stix_oop_set_t dic, const stix_bch_t* title)
{
	stix_oow_t i, j;
	stix_oop_association_t ass;

	STIX_DEBUG0 (stix, "--------------------------------------------\n");
	STIX_DEBUG2 (stix, "%s %zu\n", title, STIX_OBJ_GET_SIZE(dic->bucket));
	STIX_DEBUG0 (stix, "--------------------------------------------\n");

	for (i = 0; i < STIX_OBJ_GET_SIZE(dic->bucket); i++)
	{
		ass = (stix_oop_association_t)dic->bucket->slot[i];
		if ((stix_oop_t)ass != stix->_nil)
		{
			STIX_DEBUG2 (stix, " %07zu %O\n", i, ass->key);
		}
	}
	STIX_DEBUG0 (stix, "--------------------------------------------\n");
}



