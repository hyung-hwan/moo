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

#include "stix-prv.h"


int stix_ignite (stix_t* stix)
{
	STIX_ASSERT (stix->_nil == STIX_NULL);

	stix->_nil = stix_allocbytes (stix, STIX_SIZEOF(stix_obj_t));
	if (!stix->_nil) return -1;

	stix->_nil->_flags = STIX_OBJ_MAKE_FLAGS (STIX_OBJ_TYPE_OOP, STIX_SIZEOF(stix_oop_t), 0, 1, 0);
	stix->_nil->_size= 0;
	stix->_nil->_class = stix->_nil;


printf ("%lu\n", (unsigned long int)stix->_nil->_flags);
	/*stix->_true = stix_instantiate (stix, stix->_true_class, STIX_NULL, 0);
	stix->_false = stix_instantiate (stix, stix->_false_class STIX_NULL, 0);

	if (!stix->_true || !stix->_false) return -1;*/


	return 0;
}
