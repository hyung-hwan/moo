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

moo_oop_process_t moo_addnewproc (moo_t* moo)
{
	moo_oop_process_t proc;

	proc = (moo_oop_process_t)moo_instantiate (moo, moo->_process, MOO_NULL, moo->option.dfl_procstk_size);
	if (!proc) return MOO_NULL;

	proc->state = MOO_SMOOI_TO_OOP(0);

	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(proc) == MOO_PROCESS_NAMED_INSTVARS + moo->option.dfl_procstk_size);
	return proc;
}

void moo_schedproc (moo_t* moo, moo_oop_process_t proc)
{
	/* TODO: if scheduled, don't add */
	/*proc->next = moo->_active_process;
	proc->_active_process = proc;*/
}

void moo_unschedproc (moo_t* moo, moo_oop_process_t proc)
{
}
