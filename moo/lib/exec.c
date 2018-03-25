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

#define PROC_STATE_RUNNING 3
#define PROC_STATE_WAITING 2
#define PROC_STATE_RUNNABLE 1
#define PROC_STATE_SUSPENDED 0
#define PROC_STATE_TERMINATED -1

static MOO_INLINE const char* proc_state_to_string (int state)
{
	static const moo_bch_t* str[] = 
	{
		"TERMINATED",
		"SUSPENDED",
		"RUNNABLE",
		"WAITING",
		"RUNNING"
	};

	return str[state + 1];
}

/* TODO: adjust this process map increment value */
#define PROC_MAP_INC 64

/* TODO: adjust these max semaphore pointer buffer capacity,
 *       proably depending on the object memory size? */
#define SEM_LIST_INC 256
#define SEM_HEAP_INC 256
#define SEM_IO_TUPLE_INC 256
#define SEM_LIST_MAX (SEM_LIST_INC * 1000)
#define SEM_HEAP_MAX (SEM_HEAP_INC * 1000)
#define SEM_IO_TUPLE_MAX (SEM_IO_TUPLE_INC * 1000)
#define SEM_IO_MAP_ALIGN 1024 /* this must a power of 2 */

#define SEM_HEAP_PARENT(x) (((x) - 1) / 2)
#define SEM_HEAP_LEFT(x)   ((x) * 2 + 1)
#define SEM_HEAP_RIGHT(x)  ((x) * 2 + 2)

#define SEM_HEAP_EARLIER_THAN(stx,x,y) ( \
	(MOO_OOP_TO_SMOOI((x)->heap_ftime_sec) < MOO_OOP_TO_SMOOI((y)->heap_ftime_sec)) || \
	(MOO_OOP_TO_SMOOI((x)->heap_ftime_sec) == MOO_OOP_TO_SMOOI((y)->heap_ftime_sec) && MOO_OOP_TO_SMOOI((x)->heap_ftime_nsec) < MOO_OOP_TO_SMOOI((y)->heap_ftime_nsec)) \
)

#define LOAD_IP(moo, v_ctx) ((moo)->ip = MOO_OOP_TO_SMOOI((v_ctx)->ip))
#define STORE_IP(moo, v_ctx) ((v_ctx)->ip = MOO_SMOOI_TO_OOP((moo)->ip))

#define LOAD_SP(moo, v_ctx) ((moo)->sp = MOO_OOP_TO_SMOOI((v_ctx)->sp))
#define STORE_SP(moo, v_ctx) ((v_ctx)->sp = MOO_SMOOI_TO_OOP((moo)->sp))

#define LOAD_ACTIVE_IP(moo) LOAD_IP(moo, (moo)->active_context)
#define STORE_ACTIVE_IP(moo) STORE_IP(moo, (moo)->active_context)

#define LOAD_ACTIVE_SP(moo) LOAD_SP(moo, (moo)->processor->active)
#define STORE_ACTIVE_SP(moo) STORE_SP(moo, (moo)->processor->active)

#define SWITCH_ACTIVE_CONTEXT(moo,v_ctx) \
	do { \
		STORE_ACTIVE_IP (moo); \
		(moo)->active_context = (v_ctx); \
		(moo)->active_method = (moo_oop_method_t)(moo)->active_context->origin->method_or_nargs; \
		SET_ACTIVE_METHOD_CODE(moo); \
		LOAD_ACTIVE_IP (moo); \
		(moo)->processor->active->current_context = (moo)->active_context; \
	} while (0)

#define FETCH_BYTE_CODE(moo) ((moo)->active_code[(moo)->ip++])
#if (MOO_BCODE_LONG_PARAM_SIZE == 2)
#	define FETCH_PARAM_CODE_TO(moo, v_ooi) \
		do { \
			v_ooi = FETCH_BYTE_CODE(moo); \
			v_ooi = (v_ooi << 8) | FETCH_BYTE_CODE(moo); \
		} while (0)
#else
#	define FETCH_PARAM_CODE_TO(moo, v_ooi) (v_ooi = FETCH_BYTE_CODE(moo))
#endif

#if defined(MOO_DEBUG_VM_EXEC)
#	define LOG_MASK_INST (MOO_LOG_IC | MOO_LOG_MNEMONIC | MOO_LOG_DEBUG)

/* TODO: for send_message, display the method name. or include the method name before 'ip' */
#	define LOG_INST0(moo,fmt) MOO_LOG1(moo, LOG_MASK_INST, " %06zd " fmt "\n", (moo)->last_inst_pointer)
#	define LOG_INST1(moo,fmt,a1) MOO_LOG2(moo, LOG_MASK_INST, " %06zd " fmt "\n",(moo)->last_inst_pointer, a1)
#	define LOG_INST2(moo,fmt,a1,a2) MOO_LOG3(moo, LOG_MASK_INST, " %06zd " fmt "\n", (moo)->last_inst_pointer, a1, a2)
#	define LOG_INST3(moo,fmt,a1,a2,a3) MOO_LOG4(moo, LOG_MASK_INST, " %06zd " fmt "\n", (moo)->last_inst_pointer, a1, a2, a3)
#else
#	define LOG_INST0(moo,fmt)
#	define LOG_INST1(moo,fmt,a1)
#	define LOG_INST2(moo,fmt,a1,a2)
#	define LOG_INST3(moo,fmt,a1,a2,a3)
#endif

#if defined(__DOS__) && (defined(_INTELC32_) || (defined(__WATCOMC__) && (__WATCOMC__ <= 1000)))
	/* the old intel c code builder doesn't support __FUNCTION__ */
#	define __PRIMITIVE_NAME__ "<<primitive>>"
#elif defined(_SCO_DS)
#	define __PRIMITIVE_NAME__ "<<primitive>>"
#else
#	define __PRIMITIVE_NAME__ (&__FUNCTION__[0])
#endif

static int delete_from_sem_io (moo_t* moo, moo_oop_semaphore_t sem, int force);
static void signal_io_semaphore (moo_t* moo, moo_ooi_t io_handle, moo_ooi_t mask);
static int send_message (moo_t* moo, moo_oop_char_t selector, int to_super, moo_ooi_t nargs);
static int send_message_with_str (moo_t* moo, const moo_ooch_t* nameptr, moo_oow_t namelen, int to_super, moo_ooi_t nargs);

/* ------------------------------------------------------------------------- */
static MOO_INLINE int vm_startup (moo_t* moo)
{
	moo_oow_t i;
	
	MOO_DEBUG0 (moo, "VM started up\n");

	for (i = 0; i < moo->sem_io_map_capa; i++)
	{
		moo->sem_io_map[i] = -1;
	}

	moo->sem_gcfin = (moo_oop_semaphore_t)moo->_nil;
	moo->sem_gcfin_sigreq = 0;

	if (moo->vmprim.vm_startup (moo) <= -1) return -1;
	moo->vmprim.vm_gettime (moo, &moo->exec_start_time); /* raw time. no adjustment */

	return 0;
}

static MOO_INLINE void vm_cleanup (moo_t* moo)
{
	moo_oow_t i;

/* TODO: clean up semaphores being waited on 
	MOO_ASSERT (moo, moo->sem_io_wait_count == 0); */

	for (i = 0; i < moo->sem_io_map_capa;)
	{
		moo_ooi_t sem_io_index;
		if ((sem_io_index = moo->sem_io_map[i]) >= 0)
		{
			MOO_ASSERT (moo, sem_io_index < moo->sem_io_tuple_count);
			MOO_ASSERT (moo, moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT] ||
			                 moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT]);

			if (moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT])
			{
				delete_from_sem_io (moo, moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT], 1);
			}
			if (moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT])
			{
				delete_from_sem_io (moo, moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT], 1);
			}

			MOO_ASSERT (moo, moo->sem_io_map[i] <= -1);
		}
		else 
		{
			i++;
		}
	}

	MOO_ASSERT (moo, moo->sem_io_tuple_count == 0);
	MOO_ASSERT (moo, moo->sem_io_count == 0);

	moo->vmprim.vm_gettime (moo, &moo->exec_end_time); /* raw time. no adjustment */
	moo->vmprim.vm_cleanup (moo);
	
	moo->sem_gcfin = (moo_oop_semaphore_t)moo->_nil;
	moo->sem_gcfin_sigreq = 0;

	MOO_DEBUG0 (moo, "VM cleaned up\n");
}

static MOO_INLINE void vm_gettime (moo_t* moo, moo_ntime_t* now)
{
	moo->vmprim.vm_gettime (moo, now);
	/* in vm_startup(), moo->exec_start_time has been set to the time of
	 * that moment. time returned here get offset by moo->exec_start_time and 
	 * thus becomes relative to it. this way, it is kept small such that it
	 * can be represented in a small integer with leaving almost zero chance
	 * of overflow. */
	MOO_SUBNTIME (now, now, &moo->exec_start_time);  /* now = now - exec_start_time */
}

static MOO_INLINE void vm_sleep (moo_t* moo, const moo_ntime_t* dur)
{
	moo->vmprim.vm_sleep (moo, dur);
}

static MOO_INLINE void vm_muxwait (moo_t* moo, const moo_ntime_t* dur)
{
	moo->vmprim.vm_muxwait (moo, dur, signal_io_semaphore);
}

/* ------------------------------------------------------------------------- */

static MOO_INLINE int prepare_to_alloc_pid (moo_t* moo)
{
	moo_oow_t new_capa;
	moo_ooi_t i, j;
	moo_oop_t* tmp;

	MOO_ASSERT (moo, moo->proc_map_free_first <= -1);
	MOO_ASSERT (moo, moo->proc_map_free_last <= -1);

	new_capa = moo->proc_map_capa + PROC_MAP_INC;
	if (new_capa > MOO_SMOOI_MAX)
	{
		if (moo->proc_map_capa >= MOO_SMOOI_MAX)
		{
		#if defined(MOO_DEBUG_VM_PROCESSOR)
			MOO_LOG0 (moo, MOO_LOG_IC | MOO_LOG_FATAL, "Processor - too many processes\n");
		#endif
			moo_seterrbfmt (moo, MOO_EPFULL, "maximum number(%zd) of processes reached", MOO_SMOOI_MAX);
			return -1;
		}

		new_capa = MOO_SMOOI_MAX;
	}

	tmp = moo_reallocmem (moo, moo->proc_map, MOO_SIZEOF(moo_oop_t) * new_capa);
	if (!tmp) return -1;

	moo->proc_map_free_first = moo->proc_map_capa;
	for (i = moo->proc_map_capa, j = moo->proc_map_capa + 1; j < new_capa; i++, j++)
	{
		tmp[i] = MOO_SMOOI_TO_OOP(j);
	}
	tmp[i] = MOO_SMOOI_TO_OOP(-1);
	moo->proc_map_free_last = i;

	moo->proc_map = tmp;
	moo->proc_map_capa = new_capa;

	return 0;
}

static MOO_INLINE void alloc_pid (moo_t* moo, moo_oop_process_t proc)
{
	moo_ooi_t pid;

	pid = moo->proc_map_free_first;
	proc->id = MOO_SMOOI_TO_OOP(pid);
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(moo->proc_map[pid]));
	moo->proc_map_free_first = MOO_OOP_TO_SMOOI(moo->proc_map[pid]);
	if (moo->proc_map_free_first <= -1) moo->proc_map_free_last = -1;
	moo->proc_map[pid] = (moo_oop_t)proc;
}

static MOO_INLINE void free_pid (moo_t* moo, moo_oop_process_t proc)
{
	moo_ooi_t pid;

	pid = MOO_OOP_TO_SMOOI(proc->id);
	MOO_ASSERT (moo, pid < moo->proc_map_capa);

	moo->proc_map[pid] = MOO_SMOOI_TO_OOP(-1);
	if (moo->proc_map_free_last <= -1)
	{
		MOO_ASSERT (moo, moo->proc_map_free_first <= -1);
		moo->proc_map_free_first = pid;
	}
	else
	{
		moo->proc_map[moo->proc_map_free_last] = MOO_SMOOI_TO_OOP(pid);
	}
	moo->proc_map_free_last = pid;
}

static moo_oop_process_t make_process (moo_t* moo, moo_oop_context_t c)
{
	moo_oop_process_t proc;
	moo_ooi_t total_count;
	moo_ooi_t suspended_count;

	total_count = MOO_OOP_TO_SMOOI(moo->processor->total_count);
	if (total_count >= MOO_SMOOI_MAX)
	{
	#if defined(MOO_DEBUG_VM_PROCESSOR)
		MOO_LOG0 (moo, MOO_LOG_IC | MOO_LOG_FATAL, "Processor - too many processes\n");
	#endif
		moo_seterrbfmt (moo, MOO_EPFULL, "maximum number(%zd) of processes reached", MOO_SMOOI_MAX);
		return MOO_NULL;
	}

	if (moo->proc_map_free_first <= -1 && prepare_to_alloc_pid(moo) <= -1) return MOO_NULL;

	moo_pushtmp (moo, (moo_oop_t*)&c);
	proc = (moo_oop_process_t)moo_instantiate (moo, moo->_process, MOO_NULL, moo->option.dfl_procstk_size);
	moo_poptmp (moo);
	if (!proc) return MOO_NULL;

	proc->state = MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED);

	/* assign a process id to the process */
	alloc_pid (moo, proc);

	proc->initial_context = c;
	proc->current_context = c;
	proc->sp = MOO_SMOOI_TO_OOP(-1);
	proc->perr = MOO_ERROR_TO_OOP(MOO_ENOERR);
	proc->perrmsg = moo->_nil;

	MOO_ASSERT (moo, (moo_oop_t)c->sender == moo->_nil);

#if defined(MOO_DEBUG_VM_PROCESSOR)
	MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] **CREATED**->%hs\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
#endif

	/* a process is created in the SUSPENDED state. chain it to the suspended process list */
	suspended_count = MOO_OOP_TO_SMOOI(moo->processor->suspended.count);
	MOO_APPEND_TO_OOP_LIST (moo, &moo->processor->suspended, moo_oop_process_t, proc, ps);
	suspended_count++;
	moo->processor->suspended.count = MOO_SMOOI_TO_OOP(suspended_count);

	total_count++;
	moo->processor->total_count = MOO_SMOOI_TO_OOP(total_count);

	return proc;
}

static MOO_INLINE void sleep_active_process (moo_t* moo, int state)
{
	STORE_ACTIVE_SP(moo);

	/* store the current active context to the current process.
	 * it is the suspended context of the process to be suspended */
	MOO_ASSERT (moo, moo->processor->active != moo->nil_process);

#if defined(MOO_DEBUG_VM_PROCESSOR)
	MOO_LOG3 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->%hs in sleep_active_process\n", MOO_OOP_TO_SMOOI(moo->processor->active->id), proc_state_to_string(MOO_OOP_TO_SMOOI(moo->processor->active->state)), proc_state_to_string(state));
#endif

	moo->processor->active->current_context = moo->active_context;
	moo->processor->active->state = MOO_SMOOI_TO_OOP(state);
}

static MOO_INLINE void wake_process (moo_t* moo, moo_oop_process_t proc)
{
	/* activate the given process */
#if defined(MOO_DEBUG_VM_PROCESSOR)
	MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->RUNNING in wake_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
#endif

	MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
	proc->state = MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING);
	moo->processor->active = proc;

	/* load the stack pointer from 'proc'. 
	 * moo->processor->active points to 'proc' now. */
	LOAD_ACTIVE_SP(moo); 

	/* activate the suspended context of the new process */
	SWITCH_ACTIVE_CONTEXT (moo, proc->current_context);

#if defined(MOO_DEBUG_VM_PROCESSOR) && (MOO_DEBUG_VM_PROCESSOR >= 2)
	MOO_LOG3 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - woke up process[%zd] context %O ip=%zd\n", MOO_OOP_TO_SMOOI(moo->processor->active->id), moo->active_context, moo->ip);
#endif
}

static void switch_to_process (moo_t* moo, moo_oop_process_t proc, int new_state_for_old_active)
{
	/* the new process must not be the currently active process */
	MOO_ASSERT (moo, moo->processor->active != proc);

	/* the new process must be in the runnable state */
	MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE) ||
	                 proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_WAITING));

	sleep_active_process (moo, new_state_for_old_active);
	wake_process (moo, proc);

	moo->proc_switched = 1;
}

static MOO_INLINE void switch_to_process_from_nil (moo_t* moo, moo_oop_process_t proc)
{
	MOO_ASSERT (moo, moo->processor->active == moo->nil_process);
	wake_process (moo, proc);
	moo->proc_switched = 1;
}

static MOO_INLINE moo_oop_process_t find_next_runnable_process (moo_t* moo)
{
	moo_oop_process_t nrp;
	MOO_ASSERT (moo, moo->processor->active->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING));
	nrp = moo->processor->active->ps.next;
	if ((moo_oop_t)nrp == moo->_nil) nrp = moo->processor->runnable.first;
	return nrp;
}

static MOO_INLINE void switch_to_next_runnable_process (moo_t* moo)
{
	moo_oop_process_t nrp;
	nrp = find_next_runnable_process (moo);
	if (nrp != moo->processor->active) switch_to_process (moo, nrp, PROC_STATE_RUNNABLE);
}

static MOO_INLINE void chain_into_processor (moo_t* moo, moo_oop_process_t proc, int new_state)
{
	/* the process is not scheduled at all. 
	 * link it to the processor's process list. */
	moo_ooi_t runnable_count;
	moo_ooi_t suspended_count;

	/*MOO_ASSERT (moo, (moo_oop_t)proc->ps.prev == moo->_nil);
	MOO_ASSERT (moo, (moo_oop_t)proc->ps.next == moo->_nil);*/

	MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED));
	MOO_ASSERT (moo, new_state == PROC_STATE_RUNNABLE || new_state == PROC_STATE_RUNNING);

#if defined(MOO_DEBUG_VM_PROCESSOR)
	MOO_LOG3 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, 
		"Processor - process[%zd] %hs->%hs in chain_into_processor\n",
		MOO_OOP_TO_SMOOI(proc->id),
		proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)),
		proc_state_to_string(new_state));
#endif

	runnable_count = MOO_OOP_TO_SMOOI(moo->processor->runnable.count);

	MOO_ASSERT (moo, runnable_count >= 0);

	suspended_count = MOO_OOP_TO_SMOOI(moo->processor->suspended.count);
	MOO_DELETE_FROM_OOP_LIST (moo, &moo->processor->suspended, proc, ps);
	suspended_count--;
	moo->processor->suspended.count = MOO_SMOOI_TO_OOP(suspended_count);

	/* append to the runnable list */
	MOO_APPEND_TO_OOP_LIST (moo, &moo->processor->runnable, moo_oop_process_t, proc, ps);
	proc->state = MOO_SMOOI_TO_OOP(new_state);

	runnable_count++;
	moo->processor->runnable.count = MOO_SMOOI_TO_OOP(runnable_count);
}

static MOO_INLINE void unchain_from_processor (moo_t* moo, moo_oop_process_t proc, int new_state)
{
	moo_ooi_t runnable_count;
	moo_ooi_t suspended_count;
	moo_ooi_t total_count;

	MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	                 proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE) ||
	                 proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED));

	MOO_ASSERT (moo, proc->state != MOO_SMOOI_TO_OOP(new_state));

#if defined(MOO_DEBUG_VM_PROCESSOR)
	MOO_LOG3 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->%hs in unchain_from_processor\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)), proc_state_to_string(MOO_OOP_TO_SMOOI(new_state)));
#endif

	if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED))
	{
		suspended_count = MOO_OOP_TO_SMOOI(moo->processor->suspended.count);
		MOO_ASSERT (moo, suspended_count > 0);
		MOO_DELETE_FROM_OOP_LIST (moo, &moo->processor->suspended, proc, ps);
		suspended_count--;
		moo->processor->suspended.count = MOO_SMOOI_TO_OOP(suspended_count);
	}
	else
	{
		runnable_count = MOO_OOP_TO_SMOOI(moo->processor->runnable.count);
		MOO_ASSERT (moo, runnable_count > 0);
		MOO_DELETE_FROM_OOP_LIST (moo, &moo->processor->runnable, proc, ps);
		runnable_count--;
		moo->processor->runnable.count = MOO_SMOOI_TO_OOP(runnable_count);
		if (runnable_count == 0) moo->processor->active = moo->nil_process;
	}

	if (new_state == PROC_STATE_TERMINATED)
	{
		/* do not chain it to the suspended process list as it's being terminated */
		proc->ps.prev = (moo_oop_process_t)moo->_nil;
		proc->ps.next = (moo_oop_process_t)moo->_nil;

		total_count = MOO_OOP_TO_SMOOI(moo->processor->total_count);
		total_count--;
		moo->processor->total_count = MOO_SMOOI_TO_OOP(total_count);
	}
	else
	{
		/* append to the suspended process list */
		MOO_ASSERT (moo, new_state == PROC_STATE_SUSPENDED);

		suspended_count = MOO_OOP_TO_SMOOI(moo->processor->suspended.count);
		MOO_APPEND_TO_OOP_LIST (moo, &moo->processor->suspended, moo_oop_process_t, proc, ps);
		suspended_count++;
		moo->processor->suspended.count= MOO_SMOOI_TO_OOP(suspended_count);
	}

	proc->state = MOO_SMOOI_TO_OOP(new_state);
}

static MOO_INLINE void chain_into_semaphore (moo_t* moo, moo_oop_process_t proc, moo_oop_semaphore_t sem)
{
	/* append a process to the process list of a semaphore or a semaphore group */

	/* a process chained to a semaphore cannot get chained to
	 * a semaphore again. a process can get chained to a single semaphore 
	 * or a single semaphore group only */
	MOO_ASSERT (moo, (moo_oop_t)proc->sem == moo->_nil);
	MOO_ASSERT (moo, (moo_oop_t)proc->sem_wait.prev == moo->_nil);
	MOO_ASSERT (moo, (moo_oop_t)proc->sem_wait.next == moo->_nil);

	/* a semaphore or a semaphore group must be given for process chaining */
	MOO_ASSERT (moo, moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore) ||
	                 moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore_group));

	/* i assume the head part of the semaphore has the same layout as
	 * the semaphore group */
	MOO_ASSERT (moo, MOO_OFFSETOF(moo_semaphore_t,waiting) ==
	                 MOO_OFFSETOF(moo_semaphore_group_t,waiting));

	MOO_APPEND_TO_OOP_LIST (moo, &sem->waiting, moo_oop_process_t, proc, sem_wait);

	proc->sem = (moo_oop_t)sem;
}

static MOO_INLINE void unchain_from_semaphore (moo_t* moo, moo_oop_process_t proc)
{
	moo_oop_semaphore_t sem;

	MOO_ASSERT (moo, (moo_oop_t)proc->sem != moo->_nil);

	MOO_ASSERT (moo, moo_iskindof(moo, proc->sem, moo->_semaphore) ||
	                 moo_iskindof(moo, proc->sem, moo->_semaphore_group));

	MOO_ASSERT (moo, MOO_OFFSETOF(moo_semaphore_t,waiting) ==
	                 MOO_OFFSETOF(moo_semaphore_group_t,waiting));

	/* proc->sem may be one of a semaphore or a semaphore group.
	 * i assume that 'waiting' is defined to the same position 
	 * in both Semaphore and SemaphoreGroup. there is no need to 
	 * write different code for each class. */
	sem = (moo_oop_semaphore_t)proc->sem;  /* semgrp = (moo_oop_semaphore_group_t)proc->sem */
	MOO_DELETE_FROM_OOP_LIST (moo, &sem->waiting, proc, sem_wait); 

	proc->sem_wait.prev = (moo_oop_process_t)moo->_nil;
	proc->sem_wait.next = (moo_oop_process_t)moo->_nil;
	proc->sem = moo->_nil;
}

static void terminate_process (moo_t* moo, moo_oop_process_t proc)
{
	if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	    proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNING/RUNNABLE ---> TERMINATED */
	#if defined(MOO_DEBUG_VM_PROCESSOR)
		MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->TERMINATED in terminate_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
	#endif

		if (proc == moo->processor->active)
		{
			moo_oop_process_t nrp;

			nrp = find_next_runnable_process (moo);

			unchain_from_processor (moo, proc, PROC_STATE_TERMINATED);
			proc->sp = MOO_SMOOI_TO_OOP(-1); /* invalidate the process stack */
			proc->current_context = proc->initial_context; /* not needed but just in case */

			/* a runnable or running process must not be chanined to the
			 * process list of a semaphore */
			MOO_ASSERT (moo, (moo_oop_t)proc->sem == moo->_nil);

			if (nrp == proc)
			{
				/* no runnable process after termination */
				MOO_ASSERT (moo, moo->processor->active == moo->nil_process);
				MOO_LOG5 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, 
					"No runnable process after termination of process %zd - total %zd runnable/running %zd suspended %zd - sem_io_wait_count %zu\n", 
					MOO_OOP_TO_SMOOI(proc->id),
					MOO_OOP_TO_SMOOI(moo->processor->total_count),
					MOO_OOP_TO_SMOOI(moo->processor->runnable.count),
					MOO_OOP_TO_SMOOI(moo->processor->suspended.count),
					moo->sem_io_wait_count
				);
			}
			else
			{
				switch_to_process (moo, nrp, PROC_STATE_TERMINATED);
			}
		}
		else
		{
			unchain_from_processor (moo, proc, PROC_STATE_TERMINATED);
			proc->sp = MOO_SMOOI_TO_OOP(-1); /* invalidate the process stack */
		}

		/* when terminated, clear it from the pid table and set the process id to a negative number */
		free_pid (moo, proc);
	}
	else if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED))
	{
		/* SUSPENDED ---> TERMINATED */
	#if defined(MOO_DEBUG_VM_PROCESSOR)
		MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->TERMINATED in terminate_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
	#endif

		/*proc->state = MOO_SMOOI_TO_OOP(PROC_STATE_TERMINATED);*/
		unchain_from_processor (moo, proc, PROC_STATE_TERMINATED);
		proc->sp = MOO_SMOOI_TO_OOP(-1); /* invalidate the proce stack */

		if ((moo_oop_t)proc->sem != moo->_nil)
		{
			unchain_from_semaphore (moo, proc);
		}

		/* when terminated, clear it from the pid table and set the process id to a negative number */
		free_pid (moo, proc);
	}
#if 0
	else if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_WAITING))
	{
		/* WAITING ---> TERMINATED */
		/* TODO: */
	}
#endif
}

static void resume_process (moo_t* moo, moo_oop_process_t proc)
{
	if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_SUSPENDED))
	{
		/* SUSPENDED ---> RUNNABLE */
		/*MOO_ASSERT (moo, (moo_oop_t)proc->ps.prev == moo->_nil);
		MOO_ASSERT (moo, (moo_oop_t)proc->ps.next == moo->_nil);*/

	#if defined(MOO_DEBUG_VM_PROCESSOR)
		MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->RUNNABLE in resume_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
	#endif

		/* don't switch to this process. just change the state to RUNNABLE.
		 * process switching should be triggerd by the process scheduler. */
		chain_into_processor (moo, proc, PROC_STATE_RUNNABLE); 
		/*proc->current_context = proc->initial_context;*/
	}
#if 0
	else if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNABLE ---> RUNNING */
		/* TODO: should i allow this? */
		MOO_ASSERT (moo, moo->processor->active != proc);
		switch_to_process (moo, proc, PROC_STATE_RUNNABLE);
	}
#endif
}

static void suspend_process (moo_t* moo, moo_oop_process_t proc)
{
	if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	    proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNING/RUNNABLE ---> SUSPENDED */

	#if defined(MOO_DEBUG_VM_PROCESSOR)
		MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->SUSPENDED in suspend_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
	#endif

		if (proc == moo->processor->active)
		{
			/* suspend the active process */
			moo_oop_process_t nrp;

			nrp = find_next_runnable_process (moo);

			if (nrp == proc)
			{
				/* no runnable process after suspension */
				sleep_active_process (moo, PROC_STATE_RUNNABLE);
				unchain_from_processor (moo, proc, PROC_STATE_SUSPENDED);

				/* the last running/runnable process has been unchained 
				 * from the processor and set to SUSPENDED. the active
				 * process must be the nil process */
				MOO_ASSERT (moo, moo->processor->active == moo->nil_process);
			}
			else
			{
				/* unchain_from_processor moves the process to the suspended
				 * process and sets its state to the given state(SUSPENDED here).
				 * it doesn't change the active process. we switch the active
				 * process with switch_to_process(). setting the state of the
				 * old active process to SUSPENDED is redundant because it's
				 * done in unchain_from_processor(). the state of the active
				 * process is somewhat wrong for a short period of time until
				 * switch_to_process() has changed the active process. */
				unchain_from_processor (moo, proc, PROC_STATE_SUSPENDED);
				MOO_ASSERT (moo, moo->processor->active != moo->nil_process);
				switch_to_process (moo, nrp, PROC_STATE_SUSPENDED); 
			}
		}
		else
		{
			unchain_from_processor (moo, proc, PROC_STATE_SUSPENDED);
		}
	}
}

static void yield_process (moo_t* moo, moo_oop_process_t proc)
{
	if (proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNING))
	{
		/* RUNNING --> RUNNABLE */

		moo_oop_process_t nrp;

		MOO_ASSERT (moo, proc == moo->processor->active);

		nrp = find_next_runnable_process (moo); 
		/* if there are more than 1 runnable processes, the next
		 * runnable process must be different from proc */
		if (nrp != proc) 
		{
		#if defined(MOO_DEBUG_VM_PROCESSOR)
			MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - process[%zd] %hs->RUNNABLE in yield_process\n", MOO_OOP_TO_SMOOI(proc->id), proc_state_to_string(MOO_OOP_TO_SMOOI(proc->state)));
		#endif
			switch_to_process (moo, nrp, PROC_STATE_RUNNABLE);
		}
	}
}

#if 0
static int async_signal_semaphore (moo_t* moo, moo_oop_semaphore_t sem)
{
	if (moo->sem_list_count >= SEM_LIST_MAX)
	{
		moo_seterrnum (moo, MOO_ESEMFLOOD, "too many semaphores in the semaphore list");
		return -1;
	}

	if (moo->sem_list_count >= moo->sem_list_capa)
	{
		moo_oow_t new_capa;
		moo_oop_semaphore_t* tmp;

		new_capa = moo->sem_list_capa + SEM_LIST_INC; /* TODO: overflow check.. */
		tmp = moo_reallocmem (moo, moo->sem_list, MOO_SIZEOF(moo_oop_semaphore_t) * new_capa);
		if (!tmp) return -1;

		moo->sem_list = tmp;
		moo->sem_list_capa = new_capa;
	}

	moo->sem_list[moo->sem_list_count] = sem;
	moo->sem_list_count++;
	return 0;
}
#endif

static moo_oop_process_t signal_semaphore (moo_t* moo, moo_oop_semaphore_t sem)
{
	moo_oop_process_t proc;
	moo_ooi_t count;
	moo_oop_semaphore_group_t semgrp;

	semgrp = sem->group;
	if ((moo_oop_t)semgrp != moo->_nil)
	{
		/* the semaphore belongs to a semaphore group */
		if ((moo_oop_t)semgrp->waiting.first != moo->_nil)
		{
			/* there is a process waiting on the process group */
			proc = semgrp->waiting.first;

			unchain_from_semaphore (moo, proc);
			resume_process (moo, proc);

			/* [IMPORTANT] RETURN VALUE of SemaphoreGroup's wait.
			 * ------------------------------------------------------------
			 * the waiting process has been suspended after a waiting
			 * primitive function in Semaphore or SemaphoreGroup.
			 * the top of the stack of the process must hold the temporary 
			 * return value set by await_semaphore() or await_semaphore_group().
			 * change the return value forcibly to the actual signaled 
			 * semaphore */
			MOO_ASSERT (moo, MOO_OOP_TO_SMOOI(proc->sp) < (moo_ooi_t)(MOO_OBJ_GET_SIZE(proc) - MOO_PROCESS_NAMED_INSTVARS));
			proc->slot[MOO_OOP_TO_SMOOI(proc->sp)] = (moo_oop_t)sem;

			if ((moo_oop_t)sem->io_index != moo->_nil) moo->sem_io_wait_count--;
			return proc;
		}
	}

	/* if the semaphore belongs to a semaphore group and the control reaches 

	 * here, no process is waiting on the semaphore group. however, a process
	 * may still be waiting on the semaphore. If a process waits on a semaphore
	 * group and another process wait on a semaphore that belongs to the 
	 * semaphore group, the process waiting on the group always wins. 
	 * 
	 *    TODO: implement a fair scheduling policy. or do i simply have to disallow individual wait on a semaphore belonging to a group?
	 *       
	 * if it doesn't belong to a sempahore group, i'm free from the starvation issue.
	 */
	if ((moo_oop_t)sem->waiting.first == moo->_nil)
	{
		/* no process is waiting on this semaphore */
		count = MOO_OOP_TO_SMOOI(sem->count);
		count++;
		sem->count = MOO_SMOOI_TO_OOP(count);

		MOO_ASSERT (moo, count >= 1);
		if (count == 1 && (moo_oop_t)semgrp != moo->_nil)
		{
			/* move the semaphore from the unsignaled list to the signaled list
			 * if the semaphore count has changed from 0 to 1 in a group */
			MOO_DELETE_FROM_OOP_LIST (moo, &semgrp->sems[MOO_SEMAPHORE_GROUP_SEMS_UNSIG], sem, grm);
			MOO_APPEND_TO_OOP_LIST (moo, &semgrp->sems[MOO_SEMAPHORE_GROUP_SEMS_SIG], moo_oop_semaphore_t, sem, grm);
		}

		/* no process has been resumed */
		return (moo_oop_process_t)moo->_nil;
	}
	else
	{
		proc = sem->waiting.first;

		/* [NOTE] no GC must occur as 'proc' isn't protected with moo_pushtmp(). */

		/* detach a process from a semaphore's waiting list and 
		 * make it runnable */
		unchain_from_semaphore (moo, proc);
		resume_process (moo, proc);

		if ((moo_oop_t)sem->io_index != moo->_nil) moo->sem_io_wait_count--;

		/* return the resumed(runnable) process */
		return proc;
	}
}

static MOO_INLINE int can_await_semaphore (moo_t* moo, moo_oop_semaphore_t sem)
{
	/* a sempahore that doesn't belong to a gruop can be waited on */
	return (moo_oop_t)sem->group == moo->_nil;
}

static MOO_INLINE void await_semaphore (moo_t* moo, moo_oop_semaphore_t sem)
{
	moo_oop_process_t proc;
	moo_ooi_t count;
	moo_oop_semaphore_group_t semgrp;

	semgrp = sem->group;

	/* the caller of this function ensure that the semaphore doesn't belong to a group */
	MOO_ASSERT (moo, (moo_oop_t)semgrp == moo->_nil);

	count = MOO_OOP_TO_SMOOI(sem->count);
	if (count > 0)
	{
		/* it's already signaled */
		count--;
		sem->count = MOO_SMOOI_TO_OOP(count);

		if ((moo_oop_t)semgrp != moo->_nil && count == 0)
		{

			int sems_idx;
			/* TODO: if i disallow individual wait on a semaphore in a group,
			 *       this membership manipulation is redundant */
			MOO_DELETE_FROM_OOP_LIST (moo, &semgrp->sems[MOO_SEMAPHORE_GROUP_SEMS_SIG], sem, grm);
			sems_idx = count > 0? MOO_SEMAPHORE_GROUP_SEMS_SIG: MOO_SEMAPHORE_GROUP_SEMS_UNSIG;
			MOO_APPEND_TO_OOP_LIST (moo, &semgrp->sems[sems_idx], moo_oop_semaphore_t, sem, grm);
		}
	}
	else
	{
		/* not signaled. need to wait */
		proc = moo->processor->active;

		/* suspend the active process */
		suspend_process (moo, proc); 

		/* link the suspended process to the semaphore's process list */
		chain_into_semaphore (moo, proc, sem); 

		MOO_ASSERT (moo, sem->waiting.last == proc);

		if ((moo_oop_t)sem->io_index != moo->_nil) moo->sem_io_wait_count++;

		MOO_ASSERT (moo, moo->processor->active != proc);
	}
}

static MOO_INLINE moo_oop_t await_semaphore_group (moo_t* moo, moo_oop_semaphore_group_t semgrp)
{
	/* wait for one of semaphores in the group to be signaled */

	moo_oop_process_t proc;
	moo_oop_semaphore_t sem;

	MOO_ASSERT (moo, moo_iskindof(moo, (moo_oop_t)semgrp, moo->_semaphore_group));

	sem = semgrp->sems[MOO_SEMAPHORE_GROUP_SEMS_SIG].first;
	if ((moo_oop_t)sem != moo->_nil)
	{
		moo_ooi_t count;
		int sems_idx;

		count = MOO_OOP_TO_SMOOI(sem->count);
		MOO_ASSERT (moo, count > 0);
		count--;
		sem->count = MOO_SMOOI_TO_OOP(count);

		MOO_DELETE_FROM_OOP_LIST (moo, &semgrp->sems[MOO_SEMAPHORE_GROUP_SEMS_SIG], sem, grm);
		sems_idx = count > 0? MOO_SEMAPHORE_GROUP_SEMS_SIG: MOO_SEMAPHORE_GROUP_SEMS_UNSIG;
		MOO_APPEND_TO_OOP_LIST (moo, &semgrp->sems[sems_idx], moo_oop_semaphore_t, sem, grm);

		return (moo_oop_t)sem;
	}

	/* no semaphores have been signaled. suspend the current process
	 * until at least one of them is signaled */
	proc = moo->processor->active;

	/* suspend the active process */
	suspend_process (moo, proc); 

	/* link the suspended process to the semaphore group's process list */
	chain_into_semaphore (moo, proc, (moo_oop_semaphore_t)semgrp); 
	MOO_ASSERT (moo, semgrp->waiting.last == proc);

	if (MOO_OOP_TO_SMOOI(semgrp->sem_io_count) > 0) 
	{
		/* there might be more than 1 IO semaphores in the group
		 * but i increment moo->sem_io_wait_count by 1 only */
		moo->sem_io_wait_count++; 
	}

	/* the current process will get suspended after the caller (mostly a 
	 * a primitive function handler) is over as it's added to a suspened
	 * process list above */
	MOO_ASSERT (moo, moo->processor->active != proc);
	return moo->_nil; 
}

static void sift_up_sem_heap (moo_t* moo, moo_ooi_t index)
{
	if (index > 0)
	{
		moo_ooi_t parent;
		moo_oop_semaphore_t sem, parsem;

		parent = SEM_HEAP_PARENT(index);
		sem = moo->sem_heap[index];
		parsem = moo->sem_heap[parent];
		if (SEM_HEAP_EARLIER_THAN(moo, sem, parsem))
		{
			do
			{
				/* move down the parent to the current position */
				parsem->heap_index = MOO_SMOOI_TO_OOP(index);
				moo->sem_heap[index] = parsem;

				/* traverse up */
				index = parent;
				if (index <= 0) break;

				parent = SEM_HEAP_PARENT(parent);
				parsem = moo->sem_heap[parent];
			}
			while (SEM_HEAP_EARLIER_THAN(moo, sem, parsem));

			sem->heap_index = MOO_SMOOI_TO_OOP(index);
			moo->sem_heap[index] = sem;
		}
	}
}

static void sift_down_sem_heap (moo_t* moo, moo_ooi_t index)
{
	moo_ooi_t base = moo->sem_heap_count / 2;

	if (index < base) /* at least 1 child is under the 'index' position */
	{
		moo_ooi_t left, right, child;
		moo_oop_semaphore_t sem, chisem;

		sem = moo->sem_heap[index];
		do
		{
			left = SEM_HEAP_LEFT(index);
			right = SEM_HEAP_RIGHT(index);

			if (right < moo->sem_heap_count && SEM_HEAP_EARLIER_THAN(moo, moo->sem_heap[right], moo->sem_heap[left]))
			{
				child = right;
			}
			else
			{
				child = left;
			}

			chisem = moo->sem_heap[child];
			if (SEM_HEAP_EARLIER_THAN(moo, sem, chisem)) break;

			chisem->heap_index = MOO_SMOOI_TO_OOP(index);
			moo->sem_heap[index] = chisem;

			index = child;
		}
		while (index < base);

		sem->heap_index = MOO_SMOOI_TO_OOP(index);
		moo->sem_heap[index] = sem;
	}
}

static int add_to_sem_heap (moo_t* moo, moo_oop_semaphore_t sem)
{
	moo_ooi_t index;

	if (moo->sem_heap_count >= SEM_HEAP_MAX)
	{
		moo_seterrbfmt(moo, MOO_ESEMFLOOD, "too many semaphores in the semaphore heap");
		return -1;
	}

	if (moo->sem_heap_count >= moo->sem_heap_capa)
	{
		moo_oow_t new_capa;
		moo_oop_semaphore_t* tmp;

		/* no overflow check when calculating the new capacity
		 * owing to SEM_HEAP_MAX check above */
		new_capa = moo->sem_heap_capa + SEM_HEAP_INC;
		tmp = moo_reallocmem (moo, moo->sem_heap, MOO_SIZEOF(moo_oop_semaphore_t) * new_capa);
		if (!tmp) return -1;

		moo->sem_heap = tmp;
		moo->sem_heap_capa = new_capa;
	}

	MOO_ASSERT (moo, moo->sem_heap_count <= MOO_SMOOI_MAX);

	index = moo->sem_heap_count;
	moo->sem_heap[index] = sem;
	sem->heap_index = MOO_SMOOI_TO_OOP(index);
	moo->sem_heap_count++;

	sift_up_sem_heap (moo, index);
	return 0;
}

static void delete_from_sem_heap (moo_t* moo, moo_ooi_t index)
{
	moo_oop_semaphore_t sem, lastsem;

	MOO_ASSERT (moo, index >= 0 && index < moo->sem_heap_count);

	sem = moo->sem_heap[index];

	sem->heap_index = MOO_SMOOI_TO_OOP(-1);

	moo->sem_heap_count--;
	if (/*moo->sem_heap_count > 0 &&*/ index != moo->sem_heap_count)
	{
		/* move the last item to the deletion position */
		lastsem = moo->sem_heap[moo->sem_heap_count];
		lastsem->heap_index = MOO_SMOOI_TO_OOP(index);
		moo->sem_heap[index] = lastsem;

		if (SEM_HEAP_EARLIER_THAN(moo, lastsem, sem)) 
			sift_up_sem_heap (moo, index);
		else
			sift_down_sem_heap (moo, index);
	}
}

#if 0
/* unused */
static void update_sem_heap (moo_t* moo, moo_ooi_t index, moo_oop_semaphore_t newsem)
{
	moo_oop_semaphore_t sem;

	sem = moo->sem_heap[index];
	sem->heap_index = MOO_SMOOI_TO_OOP(-1);

	newsem->heap_index = MOO_SMOOI_TO_OOP(index);
	moo->sem_heap[index] = newsem;

	if (SEM_HEAP_EARLIER_THAN(moo, newsem, sem))
		sift_up_sem_heap (moo, index);
	else
		sift_down_sem_heap (moo, index);
}
#endif

static int add_to_sem_io (moo_t* moo, moo_oop_semaphore_t sem, moo_ooi_t io_handle, moo_semaphore_io_type_t io_type)
{
	moo_ooi_t index;
	moo_ooi_t new_mask;
	int n, tuple_added = 0;

	MOO_ASSERT (moo, sem->io_index == (moo_oop_t)moo->_nil);
	MOO_ASSERT (moo, sem->io_handle == (moo_oop_t)moo->_nil);
	MOO_ASSERT (moo, sem->io_type == (moo_oop_t)moo->_nil);

	if (io_handle < 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "handle %zd out of supported range", io_handle);
		return -1;
	}
	
	if (io_handle >= moo->sem_io_map_capa)
	{
		moo_oow_t new_capa, i;
		moo_ooi_t* tmp;

/* TODO: specify the maximum io_handle supported and check it here instead of just relying on memory allocation success/failure? */

		new_capa = MOO_ALIGN_POW2(io_handle + 1, SEM_IO_MAP_ALIGN);

		tmp = moo_reallocmem (moo, moo->sem_io_map, MOO_SIZEOF(*tmp) * new_capa);
		if (!tmp) 
		{
			const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
			moo_seterrbfmt (moo, moo->errnum, "handle %zd out of supported range - %js", oldmsg);
			return -1;
		}

		for (i = moo->sem_io_map_capa; i < new_capa; i++) tmp[i] = -1;

		moo->sem_io_map = tmp;
		moo->sem_io_map_capa = new_capa;
	}

	index = moo->sem_io_map[io_handle];
	if (index <= -1)
	{
		/* this handle is not in any tuples. add it to a new tuple */
		if (moo->sem_io_tuple_count >= SEM_IO_TUPLE_MAX)
		{
			moo_seterrbfmt (moo, MOO_ESEMFLOOD, "too many IO semaphore tuples"); 
			return -1;
		}

		if (moo->sem_io_tuple_count >= moo->sem_io_tuple_capa)
		{
			moo_oow_t new_capa;
			moo_sem_tuple_t* tmp;

			/* no overflow check when calculating the new capacity
			 * owing to SEM_IO_TUPLE_MAX check above */
			new_capa = moo->sem_io_tuple_capa + SEM_IO_TUPLE_INC;
			tmp = moo_reallocmem (moo, moo->sem_io_tuple, MOO_SIZEOF(moo_sem_tuple_t) * new_capa);
			if (!tmp) return -1;

			moo->sem_io_tuple = tmp;
			moo->sem_io_tuple_capa = new_capa;
		}

		/* this condition must be true assuming SEM_IO_TUPLE_MAX <= MOO_SMOOI_MAX */
		MOO_ASSERT (moo, moo->sem_io_tuple_count <= MOO_SMOOI_MAX); 
		index = moo->sem_io_tuple_count;

		tuple_added = 1;

		/* safe to initialize before vm_muxadd() because
		 * moo->sem_io_tuple_count has not been incremented. 
		 * still no impact even if it fails. */
		moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT] = MOO_NULL;
		moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT] = MOO_NULL;
		moo->sem_io_tuple[index].handle = io_handle;
		moo->sem_io_tuple[index].mask = 0;

		new_mask = ((moo_ooi_t)1 << io_type);

		moo_pushtmp (moo, (moo_oop_t*)&sem);
		n = moo->vmprim.vm_muxadd(moo, io_handle, new_mask);
		moo_poptmp (moo);
	}
	else
	{
		if (moo->sem_io_tuple[index].sem[io_type])
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "handle %zd linked with an IO semaphore of type %d", io_handle, (int)io_type);
			return -1;
		}

		new_mask = moo->sem_io_tuple[index].mask; /* existing mask */
		new_mask |= ((moo_ooi_t)1 << io_type);

		moo_pushtmp (moo, (moo_oop_t*)&sem);
		n = moo->vmprim.vm_muxmod(moo, io_handle, new_mask);
		moo_poptmp (moo);
	}

	if (n <= -1) 
	{
		MOO_LOG3 (moo, MOO_LOG_WARN, "Failed to add an IO semaphore at index %zd of type %d on handle %zd\n", index, (int)io_type, io_handle);
		return -1;
	}

	MOO_LOG3 (moo, MOO_LOG_DEBUG, "Added an IO semaphore at index %zd of type %d on handle %zd\n", index, (int)io_type, io_handle);

	sem->io_index = MOO_SMOOI_TO_OOP(index);
	sem->io_handle = MOO_SMOOI_TO_OOP(io_handle);
	sem->io_type = MOO_SMOOI_TO_OOP((moo_ooi_t)io_type);

	moo->sem_io_tuple[index].handle = io_handle;
	moo->sem_io_tuple[index].mask = new_mask;
	moo->sem_io_tuple[index].sem[io_type] = sem;

	moo->sem_io_count++;
	if (tuple_added) 
	{
		moo->sem_io_tuple_count++;
		moo->sem_io_map[io_handle] = index;
	}

	/* update the number of IO semaphores in a group if necessary */
	if ((moo_oop_t)sem->group != moo->_nil)
	{
		moo_ooi_t count;
		count = MOO_OOP_TO_SMOOI(sem->group->sem_io_count);
		count++;
		sem->group->sem_io_count = MOO_SMOOI_TO_OOP(count);
	}

	return 0;
}

static int delete_from_sem_io (moo_t* moo, moo_oop_semaphore_t sem, int force)
{
	moo_ooi_t index;
	moo_ooi_t new_mask, io_handle, io_type;
	int x;

	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_handle));
	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_type));

	index = MOO_OOP_TO_SMOOI(sem->io_index);
	MOO_ASSERT (moo, index >= 0 && index < moo->sem_io_tuple_count);

	io_handle = MOO_OOP_TO_SMOOI(sem->io_handle);
	if (io_handle < 0 || io_handle >= moo->sem_io_map_capa)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "handle %zd out of supported range", io_handle);
		return -1;
	}
	MOO_ASSERT (moo, moo->sem_io_map[io_handle] == MOO_OOP_TO_SMOOI(sem->io_index));

	io_type = MOO_OOP_TO_SMOOI(sem->io_type);

	new_mask = moo->sem_io_tuple[index].mask;
	new_mask &= ~((moo_ooi_t)1 << io_type); /* this is the new mask after deletion */

	moo_pushtmp (moo, (moo_oop_t*)&sem);
	x = new_mask? moo->vmprim.vm_muxmod(moo, io_handle, new_mask):
	              moo->vmprim.vm_muxdel(moo, io_handle); 
	moo_poptmp (moo);
	if (x <= -1) 
	{
		MOO_LOG3 (moo, MOO_LOG_WARN, "Failed to delete an IO semaphored handle %zd at index %zd of type %d\n", io_handle, index, (int)io_type);
		if (!force) return -1;

		/* NOTE: 
		 *   this means there could be some issue handling the file handles.
		 *   the file handle might have been closed before reaching here.
		 *   assuming the callback works correctly, it's not likely that the
		 *   underlying operating system returns failure for no reason.
		 *   i should inspect the overall vm implementation */
		MOO_LOG1 (moo, MOO_LOG_ERROR, "Forcibly unmapping the IO semaphored handle %zd as if it's deleted\n", io_handle);
	}
	else
	{
		MOO_LOG3 (moo, MOO_LOG_DEBUG, "Deleted an IO semaphored handle %zd at index %zd of type %d\n", io_handle, index, (int)io_type);
	}

	sem->io_index = moo->_nil;
	sem->io_type = moo->_nil;
	sem->io_handle = moo->_nil;
	moo->sem_io_count--;

	if ((moo_oop_t)sem->group != moo->_nil)
	{
		moo_ooi_t count;
		count = MOO_OOP_TO_SMOOI(sem->group->sem_io_count);
		MOO_ASSERT (moo, count > 0);
		count--;
		sem->group->sem_io_count = MOO_SMOOI_TO_OOP(count);
	}

	if (new_mask)
	{
		moo->sem_io_tuple[index].mask = new_mask;
		moo->sem_io_tuple[index].sem[io_type] = MOO_NULL;
	}
	else
	{
		moo->sem_io_tuple_count--;

		if (/*moo->sem_io_tuple_count > 0 &&*/ index != moo->sem_io_tuple_count)
		{
			/* migrate the last item to the deleted slot to compact the gap */
			moo->sem_io_tuple[index] = moo->sem_io_tuple[moo->sem_io_tuple_count];

			if (moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT]) 
				moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT]->io_index = MOO_SMOOI_TO_OOP(index);
			if (moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT])
				moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT]->io_index = MOO_SMOOI_TO_OOP(index);

			moo->sem_io_map[moo->sem_io_tuple[index].handle] = index;

			MOO_LOG2 (moo, MOO_LOG_DEBUG, "Migrated an IO semaphore tuple from index %zd to %zd\n", moo->sem_io_tuple_count, index);
		}

		moo->sem_io_map[io_handle] = -1;
	}

	return 0;
}

static void _signal_io_semaphore (moo_t* moo, moo_oop_semaphore_t sem)
{
	moo_oop_process_t proc;
	
	proc = signal_semaphore (moo, sem);

	if (moo->processor->active == moo->nil_process && (moo_oop_t)proc != moo->_nil)
	{
		/* this is the only runnable process. 
		 * switch the process to the running state.
		 * it uses wake_process() instead of
		 * switch_to_process() as there is no running 
		 * process at this moment */
		MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
		MOO_ASSERT (moo, proc == moo->processor->runnable.first);

	#if 0
		wake_process (moo, proc); /* switch to running */
		moo->proc_switched = 1;
	#else
		switch_to_process_from_nil (moo, proc);
	#endif
	}
}

static void signal_io_semaphore (moo_t* moo, moo_ooi_t io_handle, moo_ooi_t mask)
{
	if (io_handle >= 0 && io_handle < moo->sem_io_map_capa && moo->sem_io_map[io_handle] >= 0)
	{
		moo_oop_semaphore_t insem, outsem;
		moo_ooi_t sem_io_index;

		sem_io_index = moo->sem_io_map[io_handle];

		insem = moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT];
		outsem = moo->sem_io_tuple[sem_io_index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT];

		if (insem)
		{
			if ((mask & (MOO_SEMAPHORE_IO_MASK_OUTPUT | MOO_SEMAPHORE_IO_MASK_ERROR)) ||
			    (!outsem && (mask & MOO_SEMAPHORE_IO_MASK_HANGUP)))
			{
				_signal_io_semaphore (moo, insem);
			}
		}

		if (outsem)
		{
			if (mask & (MOO_SEMAPHORE_IO_MASK_INPUT | MOO_SEMAPHORE_IO_MASK_HANGUP | MOO_SEMAPHORE_IO_MASK_ERROR))
			{
				_signal_io_semaphore (moo, outsem);
			}
		}
	}
	else
	{
		MOO_LOG1 (moo, MOO_LOG_WARN, "Warning - semaphore signaling requested on an unmapped handle %zd\n", io_handle);
	}
}

void moo_releaseiohandle (moo_t* moo, moo_ooi_t io_handle)
{
	/* TODO: optimize io semapore unmapping. since i'm to close the handle,
	 *       i don't need to call delete_from_sem_io() seperately for input
	 *        and output. */
	if (io_handle < moo->sem_io_map_capa)
	{
		moo_ooi_t index;
		moo_oop_semaphore_t sem;

		index = moo->sem_io_map[io_handle];
		if (index >= 0)
		{
			MOO_ASSERT(moo, moo->sem_io_tuple[index].handle == io_handle);
			sem = moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_INPUT];
			if (sem) delete_from_sem_io (moo, sem, 0);
		}
	}

	if (io_handle < moo->sem_io_map_capa)
	{
		moo_ooi_t index;
		moo_oop_semaphore_t sem;

		index = moo->sem_io_map[io_handle];
		if (index >= 0)
		{
			MOO_ASSERT(moo, moo->sem_io_tuple[index].handle == io_handle);
			sem = moo->sem_io_tuple[index].sem[MOO_SEMAPHORE_IO_TYPE_OUTPUT];
			if (sem) delete_from_sem_io (moo, sem, 0);
		}
	}
}

/* ------------------------------------------------------------------------- */

static moo_oop_process_t start_initial_process (moo_t* moo, moo_oop_context_t c)
{
	moo_oop_process_t proc;

	/* there must be no active process when this function is called */
	MOO_ASSERT (moo, moo->processor->runnable.count == MOO_SMOOI_TO_OOP(0));
	MOO_ASSERT (moo, moo->processor->active == moo->nil_process);

	proc = make_process (moo, c);
	if (!proc) return MOO_NULL;

	chain_into_processor (moo, proc, PROC_STATE_RUNNING);
	moo->processor->active = proc;

	/* do somthing that resume_process() would do with less overhead */
	MOO_ASSERT (moo, (moo_oop_t)proc->current_context != moo->_nil);
	MOO_ASSERT (moo, proc->current_context == proc->initial_context);
	SWITCH_ACTIVE_CONTEXT (moo, proc->current_context);

	return proc;
}

static MOO_INLINE int activate_new_method (moo_t* moo, moo_oop_method_t mth, moo_ooi_t actual_nargs)
{
	moo_oop_context_t ctx;
	moo_ooi_t i, j;
	moo_ooi_t ntmprs, nargs, actual_ntmprs;

	ntmprs = MOO_OOP_TO_SMOOI(mth->tmpr_count);
	nargs = MOO_OOP_TO_SMOOI(mth->tmpr_nargs);

	MOO_ASSERT (moo, ntmprs >= 0);
	MOO_ASSERT (moo, nargs <= ntmprs);

	if (actual_nargs > nargs)
	{
		/* more arguments than the method specification have been passed in. 
		 * it must be a variadic or liberal unary method. othewise, the compiler is buggy */
		MOO_ASSERT (moo, MOO_METHOD_GET_PREAMBLE_FLAGS(MOO_OOP_TO_SMOOI(mth->preamble)) & (MOO_METHOD_PREAMBLE_FLAG_VARIADIC | MOO_METHOD_PREAMBLE_FLAG_LIBERAL));
		actual_ntmprs = ntmprs + (actual_nargs - nargs);
	}
	else actual_ntmprs = ntmprs;

	moo_pushtmp (moo, (moo_oop_t*)&mth);
	ctx = (moo_oop_context_t)moo_instantiate (moo, moo->_method_context, MOO_NULL, actual_ntmprs);
	moo_poptmp (moo);
	if (!ctx) return -1;

	ctx->sender = moo->active_context; 
	ctx->ip = MOO_SMOOI_TO_OOP(0);
	/* ctx->sp will be set further down */

	/* A context is compose of a fixed part and a variable part.
	 * the variable part holds temporary varibles including arguments.
	 *
	 * Assuming a method context with 2 arguments and 3 local temporary
	 * variables, the context will look like this.
	 *   +---------------------+
	 *   | fixed part          |
	 *   |                     |
	 *   |                     |
	 *   |                     |
	 *   +---------------------+
	 *   | tmp1 (arg1)         | slot[0]
	 *   | tmp2 (arg2)         | slot[1]
	 *   | tmp3                | slot[2] 
	 *   | tmp4                | slot[3]
	 *   | tmp5                | slot[4]
	 *   +---------------------+
	 */

	ctx->ntmprs = MOO_SMOOI_TO_OOP(ntmprs);
	ctx->method_or_nargs = (moo_oop_t)mth;
	/* the 'home' field of a method context is always moo->_nil.
	ctx->home = moo->_nil;*/
	ctx->origin = ctx; /* point to self */

	/* 
	 * Assume this message sending expression:
	 *   obj1 do: #this with: #that with: #it
	 * 
	 * It would be compiled to these logical byte-code sequences shown below:
	 *   push obj1
	 *   push #this
	 *   push #that
	 *   push #it
	 *   send #do:with:
	 *
	 * After three pushes, the stack looks like this.
	 * 
	 *  | #it   | <- sp
	 *  | #that |    sp - 1  
	 *  | #this |    sp - 2
	 *  | obj1  |    sp - nargs
	 *
	 * Since the number of arguments is 3, stack[sp - 3] points to
	 * the receiver. When the stack is empty, sp is -1.
	 */
	if (actual_nargs >= nargs)
	{
		for (i = actual_nargs, j = ntmprs + (actual_nargs - nargs); i > nargs; i--)
		{
			/* place variadic arguments after local temporaries */
			ctx->slot[--j] = MOO_STACK_GETTOP (moo);
			MOO_STACK_POP (moo);
		}
		MOO_ASSERT (moo, i == nargs);
		while (i > 0)
		{
			/* place normal argument before local temporaries */
			ctx->slot[--i] = MOO_STACK_GETTOP (moo);
			MOO_STACK_POP (moo);
		}
	}
	else
	{
		for (i = actual_nargs; i > 0; )
		{
			/* place normal argument before local temporaries */
			ctx->slot[--i] = MOO_STACK_GETTOP (moo);
			MOO_STACK_POP (moo);
		}
	}
	/* copy receiver */
	ctx->receiver_or_source = MOO_STACK_GETTOP (moo);
	MOO_STACK_POP (moo);

	MOO_ASSERT (moo, moo->sp >= -1);

	/* the stack pointer in a context is a stack pointer of a process 
	 * before it is activated. this stack pointer is stored to the context
	 * so that it is used to restore the process stack pointer upon returning
	 * from a method context. */
	ctx->sp = MOO_SMOOI_TO_OOP(moo->sp);

	/* switch the active context to the newly instantiated one*/
	SWITCH_ACTIVE_CONTEXT (moo, ctx);

	return 0;
}

moo_oop_method_t moo_findmethod (moo_t* moo, moo_oop_t receiver, const moo_oocs_t* message, int super)
{
	moo_oop_class_t cls;
	moo_oop_association_t ass;
	moo_oop_t c;
	moo_oop_dic_t mthdic;
	int dic_no;
/* TODO: implement method lookup cache */

	cls = MOO_CLASSOF(moo, receiver);
	if (cls == moo->_class)
	{
		/* receiver is a class object (an instance of Class) */
		c = receiver; 
		dic_no = MOO_METHOD_CLASS;
	}
	else
	{
		/* receiver is not a class object. so take its class */
		c = (moo_oop_t)cls;
		dic_no = MOO_METHOD_INSTANCE;
	}

	MOO_ASSERT (moo, c != moo->_nil);

	if (super) 
	{
		/*
		moo_oop_method_t m;
		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context->origin) == moo->_method_context);
		m = (moo_oop_method_t)moo->active_context->origin->method_or_nargs;
		c = ((moo_oop_class_t)m->owner)->superclass;
		*/
		MOO_ASSERT (moo, moo->active_method);
		MOO_ASSERT (moo, moo->active_method->owner);
		c = ((moo_oop_class_t)moo->active_method->owner)->superclass;
		if (c == moo->_nil) goto not_found; /* reached the top of the hierarchy */
	}

	do
	{
		mthdic = ((moo_oop_class_t)c)->mthdic[dic_no];

		/* if a kernel class is not defined in the bootstrapping code,
		 * the method dictionary is still nil. the initial kernel classes
		 * must all be defined properly */
		MOO_ASSERT (moo, (moo_oop_t)mthdic != moo->_nil); 

		MOO_ASSERT (moo, MOO_CLASSOF(moo, mthdic) == moo->_method_dictionary);

		ass = (moo_oop_association_t)moo_lookupdic (moo, mthdic, message);
		if (ass) 
		{
			/* found the method */
			MOO_ASSERT (moo, MOO_CLASSOF(moo, ass->value) == moo->_method);
			return (moo_oop_method_t)ass->value;
		}
		c = ((moo_oop_class_t)c)->superclass;
	}
	while (c != moo->_nil);

not_found:
	if (cls == moo->_class)
	{
		/* the object is an instance of Class. find the method
		 * in an instance method dictionary of Class also */
		mthdic = ((moo_oop_class_t)cls)->mthdic[MOO_METHOD_INSTANCE];
		MOO_ASSERT (moo, (moo_oop_t)mthdic != moo->_nil);
		MOO_ASSERT (moo, MOO_CLASSOF(moo, mthdic) == moo->_method_dictionary);

		ass = (moo_oop_association_t)moo_lookupdic (moo, mthdic, message);
		if (ass) 
		{
			MOO_ASSERT (moo, MOO_CLASSOF(moo, ass->value) == moo->_method);
			return (moo_oop_method_t)ass->value;
		}
	}

	MOO_LOG3 (moo, MOO_LOG_DEBUG, "Method '%.*js' not found in %O\n", message->len, message->ptr, receiver);
	moo_seterrbfmt (moo, MOO_ENOENT, "unable to find the method '%.*js' in %O", message->len, message->ptr, receiver);
	return MOO_NULL;
}

static int start_initial_process_and_context (moo_t* moo, const moo_oocs_t* objname, const moo_oocs_t* mthname)
{
	/* the initial context is a fake context. if objname is 'Stix' and
	 * mthname is 'main', this function emulates message sending 'Stix main'.
	 * it should emulate the following logical byte-code sequences:
	 *
	 *    push Stix
	 *    send #main
	 */
	moo_oop_context_t ctx;
	moo_oop_method_t mth;
	moo_oop_process_t proc;
#if defined(INVOKE_DIRECTLY)
	moo_oop_association_t ass;
#else
	moo_oop_t s1, s2;
#endif
	moo_oow_t tmp_count = 0;

	moo_oocs_t startup;
	static moo_ooch_t str_startup[] = { 's', 't', 'a', 'r', 't', 'u', 'p' };

#if defined(INVOKE_DIRECTLY)

	ass = moo_lookupsysdic (moo, objname);
	if (!ass || MOO_CLASSOF(moo, ass->value) != moo->_class) 
	{
		MOO_LOG2 (moo, MOO_LOG_DEBUG, "Cannot find a class '%.*js'", objname->len, objname->ptr);
		return -1;
	}

	mth = moo_findmethod (moo, ass->value, mthname, 0);
	if (!mth) 
	{
		MOO_LOG4 (moo, MOO_LOG_DEBUG, "Cannot find a method %.*js>>%.*js", objname->len, objname->ptr, mthname->len, mthname->ptr);
		return -1;
	}

	if (MOO_OOP_TO_SMOOI(mth->tmpr_nargs) > 0)
	{
		/* this method expects more than 0 arguments. 
		 * i can't use it as a start-up method.
TODO: overcome this problem - accept parameters....
		 */
		MOO_LOG4 (moo, MOO_LOG_DEBUG, (moo, "Arguments not supported by a startup method - %.*js>>%.*js", objname->len, objname->ptr, mthname->len, mthname->ptr);
		moo_seterrbfmt (moo, MOO_EINVAL, "arguments not supported by a startup method  %.*js>>%.*js", objname->len, objname->ptr, mthname->len, mthname->ptr);
		return -1;
	}

	moo_pushtmp (moo, (moo_oop_t*)&mth); tmp_count++;
	moo_pushtmp (moo, (moo_oop_t*)&ass); tmp_count++;
#else

	startup.ptr = str_startup;
	startup.len = 7;
	mth = moo_findmethod (moo, (moo_oop_t)moo->_system, &startup, 0);
	if (!mth) 
	{
		MOO_LOG0 (moo, MOO_LOG_DEBUG, "Cannot find the startup method in the system class");
		goto oops;
	}

	if (MOO_OOP_TO_SMOOI(mth->tmpr_nargs) != 2)
	{
		MOO_LOG1 (moo, MOO_LOG_DEBUG, "Weird argument count %zd for a startup method - should be 2",  MOO_OOP_TO_SMOOI(mth->tmpr_nargs));
		moo_seterrbfmt (moo, MOO_EINVAL, "%.*js>>%.*js accepting weird argument count %zd, must accept 2 only", objname->len, objname->ptr, mthname->len, mthname->ptr, MOO_OOP_TO_SMOOI(mth->tmpr_nargs));
		goto oops;
	}
/* TODO: check if it's variadic.... it should be. and accept more than 2... */

	moo_pushtmp (moo, (moo_oop_t*)&mth); tmp_count++;
	s1 = moo_makesymbol (moo, objname->ptr, objname->len);
	if (!s1) goto oops;

	moo_pushtmp (moo, (moo_oop_t*)&s1); tmp_count++;
	s2 = moo_makesymbol (moo, mthname->ptr, mthname->len);
	if (!s2) goto oops;

	moo_pushtmp (moo, (moo_oop_t*)&s2); tmp_count++;
#endif

	/* create a fake initial context. */
	ctx = (moo_oop_context_t)moo_instantiate (moo, moo->_method_context, MOO_NULL, MOO_OOP_TO_SMOOI(mth->tmpr_nargs));
	if (!ctx) goto oops;

	moo_pushtmp (moo, (moo_oop_t*)&ctx); tmp_count++;


/* TODO: handle preamble */

	/* the initial context starts the life of the entire VM
	 * and is not really worked on except that it is used to call the
	 * initial method. so it doesn't really require any extra stack space. */
/* TODO: verify this theory of mine. */
	moo->ip = 0;
	moo->sp = -1;

	ctx->ip = MOO_SMOOI_TO_OOP(0); /* point to the beginning */
	ctx->sp = MOO_SMOOI_TO_OOP(-1); /* pointer to -1 below the bottom */
	ctx->origin = ctx; /* point to self */
	ctx->method_or_nargs = (moo_oop_t)mth; /* fake. help SWITCH_ACTIVE_CONTEXT() not fail. TODO: create a static fake method and use it... instead of 'mth' */

	/* [NOTE]
	 *  the receiver field and the sender field of ctx are nils.
	 *  especially, the fact that the sender field is nil is used by 
	 *  the main execution loop for breaking out of the loop */

	MOO_ASSERT (moo, moo->active_context == MOO_NULL);
	MOO_ASSERT (moo, moo->active_method == MOO_NULL);

	/* moo_gc() uses moo->processor when moo->active_context
	 * is not NULL. at this poinst, moo->processor should point to
	 * an instance of ProcessScheduler. */
	MOO_ASSERT (moo, (moo_oop_t)moo->processor != moo->_nil);
	MOO_ASSERT (moo, moo->processor->runnable.count == MOO_SMOOI_TO_OOP(0));

	/* start_initial_process() calls the SWITCH_ACTIVE_CONTEXT() macro.
	 * the macro assumes a non-null value in moo->active_context.
	 * let's forcefully set active_context to ctx directly. */
	moo->active_context = ctx;

	proc = start_initial_process (moo, ctx); 
	moo_poptmps (moo, tmp_count); tmp_count = 0;
	if (!proc) goto oops;

#if defined(INVOKE_DIRECTLY)
	MOO_STACK_PUSH (moo, ass->value); /* push the receiver - the object referenced by 'objname' */
#else
	MOO_STACK_PUSH (moo, (moo_oop_t)moo->_system);
	MOO_STACK_PUSH (moo, s1);
	MOO_STACK_PUSH (moo, s2);
#endif
	STORE_ACTIVE_SP (moo); /* moo->active_context->sp = MOO_SMOOI_TO_OOP(moo->sp) */

	MOO_ASSERT (moo, moo->processor->active == proc);
	MOO_ASSERT (moo, moo->processor->active->initial_context == ctx);
	MOO_ASSERT (moo, moo->processor->active->current_context == ctx);
	MOO_ASSERT (moo, moo->active_context == ctx);

	/* emulate the message sending */
#if defined(INVOKE_DIRECTLY)
	return activate_new_method (moo, mth, 0);
#else
	return activate_new_method (moo, mth, 2);
#endif

oops:
	if (tmp_count > 0) moo_poptmps (moo, tmp_count);
	return -1;
}

/* ------------------------------------------------------------------------- */
static moo_pfrc_t pf_dump (moo_t* moo, moo_ooi_t nargs)
{
	moo_ooi_t i;

	MOO_ASSERT (moo, nargs >=  0);

	/*moo_logbfmt (moo, 0, "RECEIVER: %O IN PID %d SP %d XSP %d\n", MOO_STACK_GET(moo, moo->sp - nargs), (int)MOO_OOP_TO_SMOOI(moo->processor->active->id), (int)moo->sp, (int)MOO_OOP_TO_SMOOI(moo->processor->active->sp));*/
	moo_logbfmt (moo, MOO_LOG_APP | MOO_LOG_FATAL, "PID[%d] %O\n", (int)MOO_OOP_TO_SMOOI(moo->processor->active->id), MOO_STACK_GET(moo, moo->sp - nargs));
	for (i = nargs; i > 0; )
	{
		--i;
		moo_logbfmt (moo, MOO_LOG_APP | MOO_LOG_FATAL, "ARGUMENT %zd: %O\n", i, MOO_STACK_GET(moo, moo->sp - i));
	}

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

static void log_char_object (moo_t* moo, moo_oow_t mask, moo_oop_char_t msg)
{
	moo_ooi_t n;
	moo_oow_t rem;
	const moo_ooch_t* ptr;

	MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(msg) == MOO_OBJ_TYPE_CHAR);

	rem = MOO_OBJ_GET_SIZE(msg);
	ptr = msg->slot;

start_over:
	while (rem > 0)
	{
		if (*ptr == '\0') 
		{
			n = moo_logbfmt (moo, mask, "%jc", *ptr);
			MOO_ASSERT (moo, n == 1);
			rem -= n;
			ptr += n;
			goto start_over;
		}

		n = moo_logbfmt (moo, mask, "%.*js", rem, ptr);
		if (n <= -1) break;
		if (n == 0) 
		{
			/* to skip the unprinted character. 
			 * actually, this check is not needed because of '\0' skipped
			 * at the beginning  of the loop */
			n = moo_logbfmt (moo, mask, "%jc", *ptr);
			MOO_ASSERT (moo, n == 1);
		}
		rem -= n;
		ptr += n;
	}
}

static moo_pfrc_t pf_add_to_be_finalized (moo_t* moo, moo_ooi_t nargs)
{
	/* TODO: check if it has already been added */
	moo_regfinalizable (moo, MOO_STACK_GETRCV(moo,nargs));
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_remove_to_be_finalized (moo_t* moo, moo_ooi_t nargs)
{
	/* TODO: check if it has already been added */
	moo_deregfinalizable (moo, MOO_STACK_GETRCV(moo,nargs));
	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_hash (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_oow_t hv;

	MOO_ASSERT (moo, nargs == 0);
	rcv = MOO_STACK_GETRCV(moo, nargs);

	switch (MOO_OOP_GET_TAG(rcv))
	{
		case MOO_OOP_TAG_SMOOI:
			hv = MOO_OOP_TO_SMOOI(rcv);
			break;

		case MOO_OOP_TAG_SMPTR:
			hv = (moo_oow_t)MOO_OOP_TO_SMPTR(rcv);
			break;

		case MOO_OOP_TAG_CHAR:
			hv = MOO_OOP_TO_CHAR(rcv);
			break;

		case MOO_OOP_TAG_ERROR:
			hv = MOO_OOP_TO_ERROR(rcv);
			break;

		default:
		{
			if (rcv == moo->_nil) hv = 0;
			else if (rcv == moo->_true) hv = 1;
			else if (rcv == moo->_false) hv = 2;
			else
			{
				int type;

				MOO_ASSERT (moo, MOO_OOP_IS_POINTER(rcv));
				type = MOO_OBJ_GET_FLAGS_TYPE(rcv);
				switch (type)
				{
					case MOO_OBJ_TYPE_BYTE:
						hv = moo_hashbytes(((moo_oop_byte_t)rcv)->slot, MOO_OBJ_GET_SIZE(rcv));
						break;

					case MOO_OBJ_TYPE_CHAR:
						hv = moo_hashoochars (((moo_oop_char_t)rcv)->slot, MOO_OBJ_GET_SIZE(rcv));
						break;

					case MOO_OBJ_TYPE_HALFWORD:
						hv = moo_hashhalfwords(((moo_oop_halfword_t)rcv)->slot, MOO_OBJ_GET_SIZE(rcv));
						break;

					case MOO_OBJ_TYPE_WORD:
						hv = moo_hashwords(((moo_oop_word_t)rcv)->slot, MOO_OBJ_GET_SIZE(rcv));
						break;

					default:
						/* MOO_OBJ_TYPE_OOP, ... */
						moo_seterrbfmt(moo, MOO_ENOIMPL, "no builtin hash implemented for %O", rcv); /* TODO: better error code? */
						return MOO_PF_FAILURE;
				}
			}
			break;
		}
	}

	/* moo_hashxxx() functions should limit the return value to fall 
	 * in the range between 0 and MOO_SMOOI_MAX inclusive */
	MOO_ASSERT (moo, hv >= 0 && hv <= MOO_SMOOI_MAX);

	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(hv));
	return MOO_PF_SUCCESS;
}


static moo_pfrc_t pf_perform (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t /*rcv,*/ selector;
	moo_oow_t ssp, esp, i;

	MOO_ASSERT (moo, nargs >= 1); /* at least, a selector must be specified */

	/*rcv = MOO_STACK_GETRCV(moo, nargs);*/
	selector = MOO_STACK_GETARG(moo, nargs, 0);

	if (MOO_CLASSOF(moo,selector) != moo->_symbol)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid argument - %O", selector);
		return MOO_PF_FAILURE;
	}

	/* remove the selector from the stack */
	ssp = MOO_STACK_GETARGSP (moo, nargs, 0);
	esp = MOO_STACK_GETARGSP (moo, nargs, nargs - 1);
	for (i = ssp; i < esp; i++)
	{
		moo_oop_t t;
		t = MOO_STACK_GET (moo, i + 1);
		MOO_STACK_SET(moo, i, t);
	}
	MOO_STACK_POP (moo);

	/* emulate message sending */
	if (send_message (moo, (moo_oop_char_t)selector, 0, nargs - 1) <= -1) return MOO_PF_HARD_FAILURE;
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_context_goto (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_oop_t pc;

	/* this primivie provides the similar functionality to  MethodContext>>pc:
	 * except that it pops the receiver and arguments and doesn't push a
	 * return value. it's useful when you want to change the instruction
	 * pointer while maintaining the stack level before the call */

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_method_context);

	pc = MOO_STACK_GETARG(moo, nargs, 0);
	if (!MOO_OOP_IS_SMOOI(pc) || MOO_OOP_TO_SMOOI(pc) < 0)
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid pc - %O", pc);
		return MOO_PF_FAILURE;
	}

	((moo_oop_context_t)rcv)->ip = pc;
	LOAD_ACTIVE_IP (moo);

	MOO_ASSERT (moo, nargs + 1 == 2);
	MOO_STACK_POPS (moo, 2); /* pop both the argument and the receiver */
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t __block_value (moo_t* moo, moo_oop_context_t rcv_blkctx, moo_ooi_t nargs, moo_ooi_t num_first_arg_elems, moo_oop_context_t* pblkctx)
{
	/* prepare a new block context for activation.
	 * the receiver must be a block context which becomes the base
	 * for a new block context. */

	moo_oop_context_t blkctx;
	moo_ooi_t local_ntmprs, i;
	moo_ooi_t actual_arg_count;

	actual_arg_count = (num_first_arg_elems > 0)? num_first_arg_elems: nargs;

	/* TODO: find a better way to support a reentrant block context. */

	/* | sum |
	 * sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1))] ].
	 * (sum value: 10).
	 * 
	 * For the code above, sum is a block context and it is sent value: inside
	 * itself. Let me simply clone a block context to allow reentrancy like this
	 * while the block context is active
	 */

	/* the receiver must be a block context */
	MOO_ASSERT (moo, MOO_CLASSOF(moo, rcv_blkctx) == moo->_block_context);
	if (rcv_blkctx->receiver_or_source != moo->_nil)
	{
		/* the 'source' field is not nil.
		 * this block context has already been activated once.
		 * you can't send 'value' again to reactivate it.
		 * For example, [thisContext value] value. */
		MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(rcv_blkctx) > MOO_CONTEXT_NAMED_INSTVARS);
		moo_seterrbfmt (moo, MOO_EPERM, "re-valuing of a block context - %O", rcv_blkctx);
		return MOO_PF_FAILURE;
	}
	MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(rcv_blkctx) == MOO_CONTEXT_NAMED_INSTVARS);

	if (MOO_OOP_TO_SMOOI(rcv_blkctx->method_or_nargs) != actual_arg_count /* nargs */)
	{
		moo_seterrbfmt (moo, MOO_ENUMARGS,
			"wrong number of arguments to a block context %O - %zd expected, %zd given",
			rcv_blkctx, MOO_OOP_TO_SMOOI(rcv_blkctx->method_or_nargs), actual_arg_count);
		return MOO_PF_FAILURE;
	}

	/* the number of temporaries stored in the block context
	 * accumulates the number of temporaries starting from the origin.
	 * simple calculation is needed to find the number of local temporaries */
	local_ntmprs = MOO_OOP_TO_SMOOI(rcv_blkctx->ntmprs) -
	               MOO_OOP_TO_SMOOI(((moo_oop_context_t)rcv_blkctx->home)->ntmprs);
	MOO_ASSERT (moo, local_ntmprs >= actual_arg_count);

	/* create a new block context to clone rcv_blkctx */
	moo_pushtmp (moo, (moo_oop_t*)&rcv_blkctx);
	blkctx = (moo_oop_context_t) moo_instantiate (moo, moo->_block_context, MOO_NULL, local_ntmprs); 
	moo_poptmp (moo);
	if (!blkctx) return MOO_PF_FAILURE;

#if 0
	/* shallow-copy the named part including home, origin, etc. */
	for (i = 0; i < MOO_CONTEXT_NAMED_INSTVARS; i++)
	{
		((moo_oop_oop_t)blkctx)->slot[i] = ((moo_oop_oop_t)rcv_blkctx)->slot[i];
	}
#else
	blkctx->ip = rcv_blkctx->ip;
	blkctx->ntmprs = rcv_blkctx->ntmprs;
	blkctx->method_or_nargs = rcv_blkctx->method_or_nargs;
	blkctx->receiver_or_source = (moo_oop_t)rcv_blkctx;
	blkctx->home = rcv_blkctx->home;
	blkctx->origin = rcv_blkctx->origin;
#endif

/* TODO: check the stack size of a block context to see if it's large enough to hold arguments */
	if (num_first_arg_elems > 0)
	{
		/* the first argument should be an array. this function is ordered
		 * to pass array elements to the new block */
		moo_oop_oop_t xarg;
		MOO_ASSERT (moo, nargs == 1);
		xarg = (moo_oop_oop_t)MOO_STACK_GETTOP (moo);
		MOO_ASSERT (moo, MOO_OBJ_IS_OOP_POINTER(xarg));
		MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(xarg) == num_first_arg_elems); 
		for (i = 0; i < num_first_arg_elems; i++)
		{
			blkctx->slot[i] = xarg->slot[i];
		}
	}
	else
	{
		/* copy the arguments to the stack */
		for (i = 0; i < nargs; i++)
		{
			blkctx->slot[i] = MOO_STACK_GETARG(moo, nargs, i);
		}
	}
	MOO_STACK_POPS (moo, nargs + 1); /* pop arguments and receiver */

	MOO_ASSERT (moo, blkctx->home != moo->_nil);
	blkctx->sp = MOO_SMOOI_TO_OOP(-1); /* not important at all */
	blkctx->sender = moo->active_context;

	*pblkctx = blkctx;
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_block_value (moo_t* moo, moo_ooi_t nargs)
{
	moo_pfrc_t x;
	moo_oop_context_t rcv_blkctx, blkctx;

	rcv_blkctx = (moo_oop_context_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv_blkctx) == moo->_block_context);

	x = __block_value (moo, rcv_blkctx, nargs, 0, &blkctx);
	if (x <= MOO_PF_FAILURE) return x; /* hard failure and soft failure */

	SWITCH_ACTIVE_CONTEXT (moo, (moo_oop_context_t)blkctx);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_block_new_process (moo_t* moo, moo_ooi_t nargs)
{
	/* create a new process from a block context.
	 * the receiver must be be a block.
	 *   [ 1 + 2 ] newProcess.
	 *   [ :a :b | a + b ] newProcess(1, 2)
	 */

	int x;
	moo_oop_context_t rcv_blkctx, blkctx;
	moo_oop_process_t proc;
#if 0
	moo_ooi_t num_first_arg_elems = 0;

	MOO_ASSERT (moo, nargs <= 1);

	if (nargs == 1)
	{
		moo_oop_t xarg;

		xarg = MOO_STACK_GETARG(moo, nargs, 0);
		if (!MOO_OBJ_IS_OOP_POINTER(xarg))
		{
			/* the only optional argument must be an OOP-indexable 
			 * object like an array */
			moo_seterrnum (moo, MOO_EINVAL);
			return MOO_PF_FAILURE;
		}

		num_first_arg_elems = MOO_OBJ_GET_SIZE(xarg);
	}
#endif

	rcv_blkctx = (moo_oop_context_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv_blkctx) == moo->_block_context);

	/* this primitive creates a new process with a block as if the block
	 * is sent the value message */
#if 0
	x = __block_value (moo, rcv_blkctx, nargs, num_first_arg_elems, &blkctx);
#else
	x = __block_value (moo, rcv_blkctx, nargs, 0, &blkctx);
#endif
	if (x <= 0) return x; /* both hard failure and soft failure */

	/* reset the sender field to moo->_nil because this block context
	 * will be the initial context of a new process. you can simply
	 * inspect the sender field to see if a context is an initial
	 * context of a process. */
	blkctx->sender = (moo_oop_context_t)moo->_nil;

	proc = make_process (moo, blkctx);
	if (!proc) return MOO_PF_FAILURE; /* hard failure */ /* TOOD: can't this be treated as a soft failure? throw an exception instead?? */

	/* __block_value() has popped all arguments and the receiver. 
	 * PUSH the return value instead of changing the stack top */
	MOO_STACK_PUSH (moo, (moo_oop_t)proc);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_process_sp (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_process);

	/* commit the SP register of the VM to the active process for accuracy */
	STORE_ACTIVE_SP (moo);
	MOO_STACK_SETRET (moo, nargs, moo->processor->active->sp);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_process_resume (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_process);

	/* set the return value before resume_process() in case it changes
	 * the active process. */
	MOO_STACK_SETRETTORCV (moo, nargs);
	resume_process (moo, (moo_oop_process_t)rcv);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_process_terminate (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

/* TODO: need to run ensure blocks here..
 * when it's executed here. it does't have to be in Exception>>handleException when there is no exception handler */
	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_process);

	/* set the return value before terminate_process() in case it changes
	 * the active process. */
	MOO_STACK_SETRETTORCV (moo, nargs);
	terminate_process (moo, (moo_oop_process_t)rcv);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_process_yield (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_process);

	/* set the return value before yield_process() in case it changes
	 * the active process. */
	MOO_STACK_SETRETTORCV (moo, nargs);
	yield_process (moo, (moo_oop_process_t)rcv);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_process_suspend (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_CLASSOF(moo,rcv) == moo->_process);

	/* set the return value before suspend_process() in case it changes
	 * the active process. */
	MOO_STACK_SETRETTORCV (moo, nargs);
	suspend_process (moo, (moo_oop_process_t)rcv);

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */
static moo_pfrc_t pf_semaphore_signal (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, rcv, moo->_semaphore)); 

	/* signal_semaphore() may change the active process though the 
	 * implementation as of this writing makes runnable the process waiting
	 * on the signal to be processed. it is safer to set the return value
	 * before calling signal_sempahore() */
	MOO_STACK_SETRETTORCV (moo, nargs);

	signal_semaphore (moo, (moo_oop_semaphore_t)rcv);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_semaphore_wait (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, rcv, moo->_semaphore)); 

	if (!can_await_semaphore(moo, (moo_oop_semaphore_t)rcv))
	{
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to wait on a semaphore that belongs to a semaphore group");
		return MOO_PF_FAILURE;
	}
	
	/* i must set the return value before calling await_semaphore().
	 * await_semaphore() may switch the active process and the stack
	 * manipulation macros target at the active process. i'm not supposed
	 * to change the return value of a new active process. */
	MOO_STACK_SETRETTORCV (moo, nargs);
	await_semaphore (moo, (moo_oop_semaphore_t)rcv);

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_semaphore_group_add_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_semaphore_group_t rcv;
	moo_oop_semaphore_t sem;

	rcv = (moo_oop_semaphore_group_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, (moo_oop_t)rcv, moo->_semaphore_group)); 

	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG (moo, nargs, 0);
	MOO_PF_CHECK_ARGS (moo, nargs, moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore));

	if ((moo_oop_t)sem->group == moo->_nil)
	{
		int sems_idx;
		sems_idx = MOO_OOP_TO_SMOOI(sem->count) > 0? MOO_SEMAPHORE_GROUP_SEMS_SIG: MOO_SEMAPHORE_GROUP_SEMS_UNSIG;
		MOO_APPEND_TO_OOP_LIST (moo, &rcv->sems[sems_idx], moo_oop_semaphore_t, sem, grm);
		sem->group = rcv;

		if ((moo_oop_t)sem->io_index != moo->_nil)
		{
			moo_ooi_t count;

			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index) && MOO_OOP_TO_SMOOI(sem->io_index) >= 0 && MOO_OOP_TO_SMOOI(sem->io_index) < moo->sem_io_tuple_count);

			count = MOO_OOP_TO_SMOOI(rcv->sem_io_count);
			MOO_ASSERT (moo, count >= 0);
			count++;
			rcv->sem_io_count = MOO_SMOOI_TO_OOP(count);

			if (count == 1)
			{
				moo_oop_process_t wp;
				/* TODO: add sem_wait_count to process. no traversal... */
				for (wp = rcv->waiting.first; (moo_oop_t)wp != moo->_nil; wp = wp->sem_wait.next)
				{
					moo->sem_io_wait_count++;
				}
			}
		}

		MOO_STACK_SETRETTORCV (moo, nargs);
	}
	else if (sem->group == rcv)
	{
		/* do nothing. don't change the group of the semaphore */
		MOO_STACK_SETRETTORCV (moo, nargs);
	}
	else
	{
		/* the semaphore belongs to a group already */
/* TODO: is it better to move this semaphore to the new group? */
		moo_seterrbfmt (moo, MOO_EPERM, "not allowed to relocate a semaphore to a different group");
		return MOO_PF_FAILURE;
	}
	
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_semaphore_group_remove_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_semaphore_group_t rcv;
	moo_oop_semaphore_t sem;

	rcv = (moo_oop_semaphore_group_t)MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, (moo_oop_t)rcv, moo->_semaphore_group)); 

	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG (moo, nargs, 0);
	MOO_PF_CHECK_ARGS (moo, nargs, moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore));

	if (sem->group == rcv)
	{
		int sems_idx;

#if 0
		if ((moo_oop_t)rcv->waiting.first != moo->_nil)
		{
			/* there is a process waiting on this semaphore group.
			 * i don't allow a semaphore to be removed from the group.
			 * i want to dodge potential problems arising when removal is allowed.
			 * 
			 * for instance, consider this psuedo code.
			 *   sg addSemaphore: s
			 *   [ sg wait ] fork.
			 *   [ sg wait ] fork.
			 *   [ sg wait ] fork.
			 *   sg removeSemaphore: s.
			 * 
			 */
			moo_seterrbfmt (moo, MOO_EPERM, "not allowed to remove a semaphore from a group being waited on");
			return MOO_PF_FAILURE;
		}
#endif

		sems_idx = MOO_OOP_TO_SMOOI(sem->count) > 0? MOO_SEMAPHORE_GROUP_SEMS_SIG: MOO_SEMAPHORE_GROUP_SEMS_UNSIG;
		MOO_DELETE_FROM_OOP_LIST (moo, &rcv->sems[sems_idx], sem, grm);
		sem->grm.prev = (moo_oop_semaphore_t)moo->_nil;
		sem->grm.next = (moo_oop_semaphore_t)moo->_nil;
		sem->group = (moo_oop_semaphore_group_t)moo->_nil;

		if ((moo_oop_t)sem->io_index != moo->_nil)
		{
			moo_ooi_t count;

			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index) && MOO_OOP_TO_SMOOI(sem->io_index) >= 0 && MOO_OOP_TO_SMOOI(sem->io_index) < moo->sem_io_tuple_count);

			count = MOO_OOP_TO_SMOOI(rcv->sem_io_count);
			MOO_ASSERT (moo, count > 0);
			count--;
			rcv->sem_io_count = MOO_SMOOI_TO_OOP(count);

			if (count == 0)
			{
				moo_oop_process_t wp;
				/* TODO: add sem_wait_count to process. no traversal... */
				for (wp = rcv->waiting.first; (moo_oop_t)wp != moo->_nil; wp = wp->sem_wait.next)
				{
					moo->sem_io_wait_count--;
				}
			}
		}

		MOO_STACK_SETRETTORCV (moo, nargs);
		return MOO_PF_SUCCESS;
	}

	/* it doesn't belong to a group or belongs to a different group */
	moo_seterrbfmt (moo, MOO_EPERM, "not alloed to remove a semaphore from a non-owning group");
	return MOO_PF_FAILURE;
}

static moo_pfrc_t pf_semaphore_group_wait (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, sem;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, moo_iskindof(moo, rcv, moo->_semaphore_group)); 

	/* i must set the return value before calling await_semaphore_group().
	 * MOO_STACK_SETRETTORCV() manipulates the stack of the currently active
	 * process(moo->processor->active). moo->processor->active may become
	 * moo->nil_process if the current active process must get suspended. 
	 * it is safer to set the return value of the calling method here.
	 * but the arguments and the receiver information will be lost from 
	 * the stack from this moment on. */
	MOO_STACK_SETRETTORCV (moo, nargs);

	sem = await_semaphore_group (moo, (moo_oop_semaphore_group_t)rcv);
	if (sem != moo->_nil)
	{
		/* there was a signaled semaphore. the active process won't get
		 * suspended. change the return value of the current process
		 * forcibly to the signaled semaphore */
		MOO_STACK_SETTOP (moo, sem);
	}

	/* the return value will get changed to an actual semaphore signaled
	 * when the semaphore is signaled. see signal_semaphore() */
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_system_add_gcfin_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_semaphore_t sem;

	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->processor);*/

	MOO_ASSERT (moo, nargs == 1);
	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG(moo, nargs, 0);

	if (!moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not a kind of semaphore - %O", sem);
		return MOO_PF_FAILURE;
	}

/* TODO: no overwriting.. */
	moo->sem_gcfin = sem;

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_system_add_timed_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t sec, nsec;
	moo_oop_semaphore_t sem;
	moo_ntime_t now, ft;

	/* don't care about the receiver much as the receiver is not used at all.
	 * however, it's inteded to be called from the System class. */

	MOO_ASSERT (moo, nargs >= 2 || nargs <= 3);

	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG(moo, nargs, 0);
	sec = MOO_STACK_GETARG(moo, nargs, 1);
	nsec = (nargs == 3? MOO_STACK_GETARG(moo, nargs, 2): MOO_SMOOI_TO_OOP(0));

	if (!moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not a kind of semaphore - %O", sem);
		return MOO_PF_FAILURE;
	}

	if (!MOO_OOP_IS_SMOOI(sec))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid second - %O", sec);
		return MOO_PF_FAILURE;
	}

	if (!MOO_OOP_IS_SMOOI(sec))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid nanosecond - %O", nsec);
		return MOO_PF_FAILURE;
	}

	if (MOO_OOP_IS_SMOOI(sem->heap_index) && 
	    sem->heap_index != MOO_SMOOI_TO_OOP(-1))
	{
		/* if the semaphore is already been added. remove it first */
		delete_from_sem_heap (moo, MOO_OOP_TO_SMOOI(sem->heap_index));
		MOO_ASSERT (moo, sem->heap_index == MOO_SMOOI_TO_OOP(-1));

		/*
		Is this more desired???
		MOO_STACK_SETRET (moo, nargs, moo->_false);
		return MOO_PF_SUCCESS;
		*/
	}

	/* this code assumes that the monotonic clock returns a small value
	 * that can fit into a SmallInteger, even after some additions. */
	vm_gettime (moo, &now);
	MOO_ADDNTIMESNS (&ft, &now, MOO_OOP_TO_SMOOI(sec), MOO_OOP_TO_SMOOI(nsec));
	if (ft.sec < 0 || ft.sec > MOO_SMOOI_MAX) 
	{
		/* soft error - cannot represent the expiry time in a small integer. */
		MOO_LOG3 (moo, MOO_LOG_PRIMITIVE | MOO_LOG_ERROR, 
			"Error(%hs) - time (%ld) out of range(0 - %zd) when adding a timed semaphore\n", 
			__PRIMITIVE_NAME__, (unsigned long int)ft.sec, (moo_ooi_t)MOO_SMOOI_MAX);

		moo_seterrnum (moo, MOO_ERANGE);
		return MOO_PF_FAILURE;
	}

	sem->heap_ftime_sec = MOO_SMOOI_TO_OOP(ft.sec);
	sem->heap_ftime_nsec = MOO_SMOOI_TO_OOP(ft.nsec);

	if (add_to_sem_heap (moo, sem) <= -1) return MOO_PF_HARD_FAILURE;

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t __system_add_io_semaphore (moo_t* moo, moo_ooi_t nargs, moo_semaphore_io_type_t io_type)
{
	moo_oop_t fd;
	moo_oop_semaphore_t sem;

	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->system);*/

	MOO_ASSERT (moo, nargs == 2);

	fd = MOO_STACK_GETARG(moo, nargs, 1);
	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG(moo, nargs, 0);

	if (!moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not a kind of semaphore - %O", sem);
		return MOO_PF_FAILURE;
	}

	if (!MOO_OOP_IS_SMOOI(fd))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "handle not a small integer - %O", fd);
		return MOO_PF_FAILURE;
	}

	if ((moo_oop_t)sem->io_index != moo->_nil)
	{
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index) && MOO_OOP_TO_SMOOI(sem->io_index) >= 0);
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_handle) && MOO_OOP_TO_SMOOI(sem->io_handle) >= 0);
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_type));

		moo_seterrbfmt (moo, MOO_EINVAL, "semaphore already linked with a handle %zd", MOO_OOP_TO_SMOOI(sem->io_handle));
		return MOO_PF_FAILURE;
	}

	if (add_to_sem_io(moo, sem, MOO_OOP_TO_SMOOI(fd), io_type) <= -1) 
	{
		const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
		moo_seterrbfmt (moo, moo->errnum, "cannot add the handle %zd to the multiplexer - %js", MOO_OOP_TO_SMOOI(fd), oldmsg);
		return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_system_add_input_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	return __system_add_io_semaphore (moo, nargs, MOO_SEMAPHORE_IO_TYPE_INPUT);
}

static moo_pfrc_t pf_system_add_output_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	return __system_add_io_semaphore (moo, nargs, MOO_SEMAPHORE_IO_TYPE_OUTPUT);
}

static moo_pfrc_t pf_system_remove_semaphore (moo_t* moo, moo_ooi_t nargs)
{
	/* remove a semaphore from processor's signal scheduling */

	moo_oop_semaphore_t sem;

	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->system);*/

	MOO_ASSERT (moo, nargs == 1);

	sem = (moo_oop_semaphore_t)MOO_STACK_GETARG(moo, nargs, 0);
	if (!moo_iskindof(moo, (moo_oop_t)sem, moo->_semaphore))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "parameter not a kind of semaphore - %O", sem);
		return MOO_PF_FAILURE;
	}

/* TODO: remove a semaphore from IO handler if it's registered...
 *       remove a semaphore from elsewhere registered too */

	if (sem == moo->sem_gcfin)
	{
		moo->sem_gcfin = (moo_oop_semaphore_t)moo->_nil;
	}

	if (MOO_OOP_IS_SMOOI(sem->heap_index) && sem->heap_index != MOO_SMOOI_TO_OOP(-1)) /* TODO: change to use _nil like io_index instead of -1 */
	{
		/* the semaphore is in the timed semaphore heap */
		delete_from_sem_heap (moo, MOO_OOP_TO_SMOOI(sem->heap_index));
		MOO_ASSERT (moo, sem->heap_index == MOO_SMOOI_TO_OOP(-1));
	}

	if ((moo_oop_t)sem->io_index != moo->_nil)
	{
		/* the semaphore is associated with IO */
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_index) && MOO_OOP_TO_SMOOI(sem->io_index) >= 0);
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_handle) && MOO_OOP_TO_SMOOI(sem->io_handle) >= 0);
		MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(sem->io_type));

		if (delete_from_sem_io(moo, sem, 0) <= -1)
		{
			const moo_ooch_t* oldmsg = moo_backuperrmsg(moo);
			moo_seterrbfmt (moo, moo->errnum, "cannot delete the handle %zd from the multiplexer - %js", MOO_OOP_TO_SMOOI(sem->io_handle), oldmsg);
			return MOO_PF_FAILURE;
		}

		MOO_ASSERT (moo, (moo_oop_t)sem->io_index == moo->_nil);
	}

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_system_return_value_to_context (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t ret, ctx;

	/*MOO_PF_CHECK_RCV (moo, MOO_STACK_GETRCV(moo, nargs) == (moo_oop_t)moo->processor);*/

	MOO_ASSERT (moo, nargs == 2);

	ret = MOO_STACK_GETARG(moo, nargs, 0);
	ctx = MOO_STACK_GETARG(moo, nargs, 1);

	MOO_PF_CHECK_ARGS (moo, nargs, MOO_CLASSOF(moo, ctx) == moo->_block_context ||
	                               MOO_CLASSOF(moo, ctx) == moo->_method_context);

	MOO_STACK_POPS (moo, nargs + 1); /* pop arguments and receiver */
/* TODO: verify if this is correct? does't it correctly restore the stack pointer?
 *       test complex chains of method contexts and block contexts */
	if (MOO_CLASSOF(moo, ctx) == moo->_method_context)
	{
		/* when returning to a method context, load the sp register with 
		 * the value stored in the context */
		moo->sp = MOO_OOP_TO_SMOOI(((moo_oop_context_t)ctx)->sp);
	}
	MOO_STACK_PUSH (moo, ret);

	SWITCH_ACTIVE_CONTEXT (moo, (moo_oop_context_t)ctx);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_integer_add (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_addints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_sub (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_subints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_mul (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_mulints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_quo (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, quo;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	quo = moo_divints (moo, rcv, arg, 0, MOO_NULL);
	if (!quo) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);
/* TODO: MOO_EDIVBY0 soft or hard failure? */

	MOO_STACK_SETRET (moo, nargs, quo);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_rem (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, quo, rem;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	quo = moo_divints (moo, rcv, arg, 0, &rem);
	if (!quo) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);
/* TODO: MOO_EDIVBY0 soft or hard failure? */

	MOO_STACK_SETRET (moo, nargs, rem);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_quo2 (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, quo;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	quo = moo_divints (moo, rcv, arg, 1, MOO_NULL);
	if (!quo) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);
/* TODO: MOO_EDIVBY0 soft or hard failure? */

	MOO_STACK_SETRET (moo, nargs, quo);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_rem2 (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, quo, rem;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	quo = moo_divints (moo, rcv, arg, 1, &rem);
	if (!quo) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);
/* TODO: MOO_EDIVBY0 soft or hard failure? */

	MOO_STACK_SETRET (moo, nargs, rem);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_negated (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, res;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);

	res = moo_negateint (moo, rcv);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitat (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_bitatint (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitand (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_bitandints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitor (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_bitorints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitxor (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_bitxorints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitinv (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, res;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);

	res = moo_bitinvint (moo, rcv);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_bitshift (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_bitshiftint (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_eq (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_eqints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_ne (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_neints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_lt (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_ltints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_gt (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_gtints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_le (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_leints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_ge (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	res = moo_geints (moo, rcv, arg);
	if (!res) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_integer_inttostr (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, str;
	moo_ooi_t radix;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OOP_IS_SMOOI(arg)) return MOO_PF_FAILURE;
	radix = MOO_OOP_TO_SMOOI(arg);

	if (radix < 2 || radix > 36) return MOO_PF_FAILURE;
	str = moo_inttostr (moo, rcv, radix);
	if (!str) return (moo->errnum == MOO_EINVAL? MOO_PF_FAILURE: MOO_PF_HARD_FAILURE);

	MOO_STACK_SETRET (moo, nargs, str);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_character_eq (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);
	
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));

	/*res = (MOO_OOP_IS_CHAR(arg) && MOO_OOP_TO_CHAR(rcv) == MOO_OOP_TO_CHAR(arg))? moo->_true: moo->_false;*/
	res = (rcv == arg)? moo->_true: moo->_false; /* this comparision should be sufficient for characters */

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_ne (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));

	/*res = (MOO_OOP_IS_CHAR(arg) && MOO_OOP_TO_CHAR(rcv) == MOO_OOP_TO_CHAR(arg))? moo->_false: moo->_true;*/
	res = (rcv == arg)? moo->_false: moo->_true; /* this comparision should be sufficient for characters */

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_lt (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);
	
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_CHAR(arg));

	res = (MOO_OOP_TO_CHAR(rcv) < MOO_OOP_TO_CHAR(arg))? moo->_true: moo->_false;
	
	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_gt (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_CHAR(arg));

	res = (MOO_OOP_TO_CHAR(rcv) > MOO_OOP_TO_CHAR(arg))? moo->_true: moo->_false;

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_le (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_CHAR(arg));
	
	res = (MOO_OOP_TO_CHAR(rcv) <= MOO_OOP_TO_CHAR(arg))? moo->_true: moo->_false;

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_ge (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, res;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);
	
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));
	MOO_PF_CHECK_ARGS (moo, nargs, MOO_OOP_IS_CHAR(arg));

	res = (MOO_OOP_TO_CHAR(rcv) >= MOO_OOP_TO_CHAR(arg))? moo->_true: moo->_false;

	MOO_STACK_SETRET (moo, nargs, res);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_character_as_error (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t c;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));

	c = MOO_OOP_TO_CHAR(rcv);
	MOO_STACK_SETRET (moo, nargs, MOO_ERROR_TO_OOP(c));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_character_as_smooi (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t c;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_CHAR(rcv));

	c = MOO_OOP_TO_CHAR(rcv);
	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(c));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_smooi_as_character (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t ec;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMOOI(rcv));

	ec = MOO_OOP_TO_SMOOI(rcv);
	if (ec < 0) ec = 0;
	MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(ec));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_smooi_as_error (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t ec;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_SMOOI(rcv));

	ec = MOO_OOP_TO_SMOOI(rcv);
	if (ec < MOO_ERROR_MIN) ec = MOO_ERROR_MIN;
	else if (ec > MOO_ERROR_MAX) ec = MOO_ERROR_MAX;

	MOO_STACK_SETRET (moo, nargs, MOO_ERROR_TO_OOP(ec));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_error_as_character (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t ec;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_ERROR(rcv));

	ec = MOO_OOP_TO_ERROR(rcv);
	MOO_ASSERT (moo, ec >= MOO_CHAR_MIN && ec <= MOO_CHAR_MAX);
	MOO_STACK_SETRET (moo, nargs, MOO_CHAR_TO_OOP(ec));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_error_as_integer (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	moo_ooi_t ec;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_ERROR(rcv));

	ec = MOO_OOP_TO_ERROR(rcv);
	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(ec));
	MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(ec));
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_error_as_string (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, ss;
	moo_ooi_t ec;
	const moo_ooch_t* s;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OOP_IS_ERROR(rcv));

	ec = MOO_OOP_TO_ERROR(rcv);
	MOO_ASSERT (moo, MOO_IN_SMOOI_RANGE(ec));

/* TODO: error string will be mostly the same.. do i really have to call makestring every time? */
	s = moo_errnum_to_errstr (ec);
	ss = moo_makestring (moo, s, moo_countoocstr(s));
	if (!ss) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, ss);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_strlen (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, ret;
	moo_oow_t i, limit;
	moo_ooch_t* ptr;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_PF_CHECK_RCV (moo, MOO_OBJ_IS_CHAR_POINTER(rcv));

	/* [NOTE] the length check loop is directly implemented
	 *        here to be able to handle character objects
	 *        regardless of the existence of the EXTRA flag */
	limit = MOO_OBJ_GET_SIZE(rcv);
	ptr = MOO_OBJ_GET_CHAR_SLOT(rcv);
	for (i = 0; i < limit; i++)
	{
		if (*ptr == '\0') break;
		ptr++;
	}
	ret = moo_oowtoint (moo, i);
	if (!ret) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, ret);
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------ */

static moo_pfrc_t pf_system_log (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t msg, level;
	moo_oow_t mask;
	moo_ooi_t k;

	MOO_ASSERT (moo, nargs >=  2);

/* TODO: enhance this primitive */
	level = MOO_STACK_GETARG(moo, nargs, 0);
	if (!MOO_OOP_IS_SMOOI(level)) mask = MOO_LOG_APP | MOO_LOG_INFO; 
	else mask = MOO_LOG_APP | MOO_OOP_TO_SMOOI(level);

	for (k = 1; k < nargs; k++)
	{
		msg = MOO_STACK_GETARG (moo, nargs, k);

		if (msg == moo->_nil || msg == moo->_true || msg == moo->_false) 
		{
			goto dump_object;
		}
		else if (MOO_OOP_IS_POINTER(msg))
		{
			if (MOO_OBJ_GET_FLAGS_TYPE(msg) == MOO_OBJ_TYPE_CHAR)
			{
				log_char_object (moo, mask, (moo_oop_char_t)msg);
			}
			else if (MOO_OBJ_GET_FLAGS_TYPE(msg) == MOO_OBJ_TYPE_OOP)
			{
				/* visit only 1-level down into an array-like object */
				moo_oop_t inner;
				moo_oop_class_t _class;
				moo_oow_t i, spec;

				_class = MOO_CLASSOF(moo, msg);

				spec = MOO_OOP_TO_SMOOI(((moo_oop_class_t)_class)->spec);
				if (MOO_CLASS_SPEC_NAMED_INSTVARS(spec) > 0 || !MOO_CLASS_SPEC_IS_INDEXED(spec)) goto dump_object;

				for (i = 0; i < MOO_OBJ_GET_SIZE(msg); i++)
				{
					inner = ((moo_oop_oop_t)msg)->slot[i];

					if (i > 0) moo_logbfmt (moo, mask, " ");
					if (MOO_OOP_IS_POINTER(inner) &&
					    MOO_OBJ_GET_FLAGS_TYPE(inner) == MOO_OBJ_TYPE_CHAR)
					{
						log_char_object (moo, mask, (moo_oop_char_t)inner);
					}
					else
					{
						moo_logbfmt (moo, mask, "%O", inner);
					}
				}
			}
			else goto dump_object;
		}
		else
		{
		dump_object:
			moo_logbfmt (moo, mask, "%O", msg);
		}
	}

	MOO_STACK_SETRETTORCV (moo, nargs); /* ^self */
	return MOO_PF_SUCCESS;
}

#define MA MOO_TYPE_MAX(moo_oow_t)
struct pf_t
{
	const char*     name;        /* the name is supposed to be 7-bit ascii only */
	moo_pfbase_t    pfbase;
};
typedef struct pf_t pf_t;

static pf_t pftab[] =
{
	{ "Apex_=",                                { moo_pf_equal,                            1, 1 } },
	{ "Apex_==",                               { moo_pf_identical,                        1, 1 } },
	{ "Apex_addToBeFinalized",                 { pf_add_to_be_finalized,                  0, 0 } },
	{ "Apex_basicAt:",                         { moo_pf_basic_at,                         1, 1 } },
	{ "Apex_basicAt:put:",                     { moo_pf_basic_at_put,                     2, 2 } },
	{ "Apex_basicFillFrom:with:count:",        { moo_pf_basic_fill,                       3, 3 } },
	{ "Apex_basicNew",                         { moo_pf_basic_new,                        0, 0 } },
	{ "Apex_basicNew:",                        { moo_pf_basic_new,                        1, 1 } },
	{ "Apex_basicShiftFrom:to:count:",         { moo_pf_basic_shift,                      3, 3 } },
	{ "Apex_basicSize",                        { moo_pf_basic_size,                       0, 0 } },
	{ "Apex_class",                            { moo_pf_class,                            0, 0 } },
	{ "Apex_hash",                             { pf_hash,                                 0, 0 } },
	{ "Apex_isKindOf:",                        { moo_pf_is_kind_of,                       1, 1, } },
	{ "Apex_perform",                          { pf_perform,                              1, MA } },
	{ "Apex_removeToBeFinalized",              { pf_remove_to_be_finalized,               0, 0 } },
	{ "Apex_respondsTo:",                      { moo_pf_responds_to,                      1, 1 } },
	{ "Apex_shallowCopy",                      { moo_pf_shallow_copy,                     0, 0 } },
	{ "Apex_~=",                               { moo_pf_not_equal,                        1, 1 } },
	{ "Apex_~~",                               { moo_pf_not_identical,                    1, 1 } },

	{ "BlockContext_newProcess",               { pf_block_new_process,                    0, MA } },
	{ "BlockContext_value",                    { pf_block_value,                          0, MA } },

	{ "Character_<",                           { pf_character_lt,                         1, 1 } },
	{ "Character_<=",                          { pf_character_le,                         1, 1 } },
	{ "Character_>",                           { pf_character_gt,                         1, 1 } },
	{ "Character_>=",                          { pf_character_ge,                         1, 1 } },
	{ "Character_asError",                     { pf_character_as_error,                   0, 0 } },
	{ "Character_asInteger",                   { pf_character_as_smooi,                   0, 0 } },

	{ "Error_asCharacter",                     { pf_error_as_character,                   0, 0 } },
	{ "Error_asInteger",                       { pf_error_as_integer,                     0, 0 } },
	{ "Error_asString",                        { pf_error_as_string,                      0, 0 } },

	{ "MethodContext_goto:",                   { pf_context_goto,                         1, 1  } },
	
	{ "Process_resume",                        { pf_process_resume,                       0, 0 } },
	{ "Process_sp",                            { pf_process_sp,                           0, 0 } },
	{ "Process_suspend",                       { pf_process_suspend,                      0, 0 } },
	{ "Process_terminate",                     { pf_process_terminate,                    0, 0 } },
	{ "Process_yield",                         { pf_process_yield,                        0, 0 } },

	{ "SemaphoreGroup_addSemaphore:",          { pf_semaphore_group_add_semaphore,        1, 1 } },
	{ "SemaphoreGroup_removeSemaphore:",       { pf_semaphore_group_remove_semaphore,     1, 1 } },
	{ "SemaphoreGroup_wait",                   { pf_semaphore_group_wait,                 0, 0 } },
	
	{ "Semaphore_signal",                      { pf_semaphore_signal,                     0, 0 } },
	{ "Semaphore_wait",                        { pf_semaphore_wait,                       0, 0 } },

	{ "SmallInteger_asCharacter",              { pf_smooi_as_character,                   0, 0 } },
	{ "SmallInteger_asError",                  { pf_smooi_as_error,                       0, 0 } },

	{ "SmallPointer_asString",                 { moo_pf_smptr_as_string,                  0, 0 } },
	{ "SmallPointer_free",                     { moo_pf_smptr_free,                       0, 0 } },
	{ "SmallPointer_getBytes",                 { moo_pf_smptr_get_bytes,                  4, 4 } },
	{ "SmallPointer_getInt16",                 { moo_pf_smptr_get_int16,                  1, 1 } },
	{ "SmallPointer_getInt32",                 { moo_pf_smptr_get_int32,                  1, 1 } },
	{ "SmallPointer_getInt64",                 { moo_pf_smptr_get_int64,                  1, 1 } },
	{ "SmallPointer_getInt8",                  { moo_pf_smptr_get_int8,                   1, 1 } },
	{ "SmallPointer_getUint16",                { moo_pf_smptr_get_uint16,                 1, 1 } },
	{ "SmallPointer_getUint32",                { moo_pf_smptr_get_uint32,                 1, 1 } },
	{ "SmallPointer_getUint64",                { moo_pf_smptr_get_uint64,                 1, 1 } },
	{ "SmallPointer_getUint8",                 { moo_pf_smptr_get_uint8,                  1, 1 } },
	{ "SmallPointer_putBytes",                 { moo_pf_smptr_put_bytes,                  4, 4 } },
	{ "SmallPointer_putInt16",                 { moo_pf_smptr_put_int16,                  2, 2 } },
	{ "SmallPointer_putInt32",                 { moo_pf_smptr_put_int32,                  2, 2 } },
	{ "SmallPointer_putInt64",                 { moo_pf_smptr_put_int64,                  2, 2 } },
	{ "SmallPointer_putInt8",                  { moo_pf_smptr_put_int8,                   2, 2 } },
	{ "SmallPointer_putUint16",                { moo_pf_smptr_put_uint16,                 2, 2 } },
	{ "SmallPointer_putUint32",                { moo_pf_smptr_put_uint32,                 2, 2 } },
	{ "SmallPointer_putUint64",                { moo_pf_smptr_put_uint64,                 2, 2 } },
	{ "SmallPointer_putUint8",                 { moo_pf_smptr_put_uint8,                  2, 2 } },

	{ "String_strlen",                         { pf_strlen,                               0, 0 } },

	{ "System_calloc",                         { moo_pf_system_calloc,                    1, 1 } },
	{ "System_calloc:",                        { moo_pf_system_calloc,                    1, 1 } },
	{ "System_collectGarbage",                 { moo_pf_system_collect_garbage,           0, 0 } },
	{ "System_free",                           { moo_pf_system_free,                      1, 1 } },
	{ "System_free:",                          { moo_pf_system_free,                      1, 1 } },
	{ "System_gc",                             { moo_pf_system_collect_garbage,           0, 0 } },
	{ "System_getBytes",                       { moo_pf_system_get_bytes,                 5, 5 } },
	{ "System_getInt16",                       { moo_pf_system_get_int16,                 2, 2 } },
	{ "System_getInt32",                       { moo_pf_system_get_int32,                 2, 2 } },
	{ "System_getInt64",                       { moo_pf_system_get_int64,                 2, 2 } },
	{ "System_getInt8",                        { moo_pf_system_get_int8,                  2, 2 } },
	{ "System_getUint16",                      { moo_pf_system_get_uint16,                2, 2 } },
	{ "System_getUint32",                      { moo_pf_system_get_uint32,                2, 2 } },
	{ "System_getUint64",                      { moo_pf_system_get_uint64,                2, 2 } },
	{ "System_getUint8",                       { moo_pf_system_get_uint8,                 2, 2 } },
	{ "System_log",                            { pf_system_log,                           2, MA } },
	{ "System_malloc",                         { moo_pf_system_malloc,                    1, 1 } },
	{ "System_malloc:",                        { moo_pf_system_malloc,                    1, 1 } },
	{ "System_popCollectable",                 { moo_pf_system_pop_collectable,           0, 0 } },
	{ "System_putBytes",                       { moo_pf_system_put_bytes,                 5, 5 } },
	{ "System_putInt16",                       { moo_pf_system_put_int16,                 3, 3 } },
	{ "System_putInt32",                       { moo_pf_system_put_int32,                 3, 3 } },
	{ "System_putInt64",                       { moo_pf_system_put_int64,                 3, 3 } },
	{ "System_putInt8",                        { moo_pf_system_put_int8,                  3, 3 } },
	{ "System_putUint16",                      { moo_pf_system_put_uint16,                3, 3 } },
	{ "System_putUint32",                      { moo_pf_system_put_uint32,                3, 3 } },
	{ "System_putUint64",                      { moo_pf_system_put_uint64,                3, 3 } },
	{ "System_putUint8",                       { moo_pf_system_put_uint8,                 3, 3 } },
	{ "System_return:to:",                     { pf_system_return_value_to_context,       2, 2 } },
	{ "System_signal:afterSecs:",              { pf_system_add_timed_semaphore,           2, 2 } },
	{ "System_signal:afterSecs:nanosecs:",     { pf_system_add_timed_semaphore,           3, 3 } },
	{ "System_signal:onInput:",                { pf_system_add_input_semaphore,           2, 2 } },
	{ "System_signal:onOutput:",               { pf_system_add_output_semaphore,          2, 2 } },
	{ "System_signalOnGCFin:",                 { pf_system_add_gcfin_semaphore,           1, 1 } },
	{ "System_unsignal:",                      { pf_system_remove_semaphore,              1, 1 } },

	{ "_dump",                                 { pf_dump,                                 0, MA } },

	{ "_integer_add",                          { pf_integer_add,                          1, 1 } },
	{ "_integer_bitand",                       { pf_integer_bitand,                       1, 1 } },
	{ "_integer_bitat",                        { pf_integer_bitat,                        1, 1 } },
	{ "_integer_bitinv",                       { pf_integer_bitinv,                       0, 0 } },
	{ "_integer_bitor",                        { pf_integer_bitor,                        1, 1 } },
	{ "_integer_bitshift",                     { pf_integer_bitshift,                     1, 1 } },
	{ "_integer_bitxor",                       { pf_integer_bitxor,                       1, 1 } },
	{ "_integer_eq",                           { pf_integer_eq,                           1, 1 } },
	{ "_integer_ge",                           { pf_integer_ge,                           1, 1 } },
	{ "_integer_gt",                           { pf_integer_gt,                           1, 1 } },
	{ "_integer_inttostr",                     { pf_integer_inttostr,                     1, 1 } },
	{ "_integer_le",                           { pf_integer_le,                           1, 1 } },
	{ "_integer_lt",                           { pf_integer_lt,                           1, 1 } },
	{ "_integer_mul",                          { pf_integer_mul,                          1, 1 } },
	{ "_integer_ne",                           { pf_integer_ne,                           1, 1 } },
	{ "_integer_negated",                      { pf_integer_negated,                      0, 0 } },
	{ "_integer_quo",                          { pf_integer_quo,                          1, 1 } },
	{ "_integer_quo2",                         { pf_integer_quo2,                         1, 1 } },
	{ "_integer_rem",                          { pf_integer_rem,                          1, 1 } },
	{ "_integer_rem2",                         { pf_integer_rem2,                         1, 1 } },
	{ "_integer_sub",                          { pf_integer_sub,                          1, 1 } }
};

moo_pfbase_t* moo_getpfnum (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len, moo_ooi_t* pfnum)
{
	moo_oow_t base, mid, lim;
	int n;

	for (base = 0, lim = MOO_COUNTOF(pftab); lim > 0; lim >>= 1)
	{
		mid = base + (lim >> 1);
		/* moo_compoocharsbcstr() is not aware of multibyte encoding.
		 * so the names above should be composed of the single byte 
		 * characters only */
		n = moo_compoocharsbcstr(ptr, len, pftab[mid].name);
		if (n == 0) 
		{
			MOO_ASSERT (moo, MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(mid)); /* this must never be so big */
			*pfnum = mid;
			return &pftab[mid].pfbase;
		}
		if (n > 0) { base = mid + 1; lim--; }
	}

	moo_seterrbfmt (moo, MOO_ENOENT, "unknown primitive specifier %.*js", len, ptr);
	return MOO_NULL;
}

/* ------------------------------------------------------------------------- */
static int start_method (moo_t* moo, moo_oop_method_t method, moo_oow_t nargs)
{
	moo_ooi_t preamble, preamble_code, preamble_flags;
	moo_ooi_t /*sp,*/ stack_base;

#if defined(MOO_DEBUG_VM_EXEC)
	/* set it to a fake value */
	moo->last_instruction_pointer = 0; 
#endif

	preamble = MOO_OOP_TO_SMOOI(method->preamble);
	preamble_flags = MOO_METHOD_GET_PREAMBLE_FLAGS(preamble);

	if (preamble_flags & MOO_METHOD_PREAMBLE_FLAG_LIBERAL)
	{
		/* do nothing - no argument check */
	}
	else if (preamble_flags & MOO_METHOD_PREAMBLE_FLAG_VARIADIC)
	{
		if (nargs < MOO_OOP_TO_SMOOI(method->tmpr_nargs)) goto arg_count_mismatch;
	}
	else
	{
		if (nargs != MOO_OOP_TO_SMOOI(method->tmpr_nargs))
		{
/* TODO: better to throw a moo exception so that the caller can catch it??? */
		arg_count_mismatch:
			MOO_LOG3 (moo, MOO_LOG_IC | MOO_LOG_FATAL, 
				"Fatal error - Argument count mismatch for a non-variadic method [%O] - %zd expected, %zu given\n",
				method->name, MOO_OOP_TO_SMOOI(method->tmpr_nargs), nargs);
			moo_seterrnum (moo, MOO_EINVAL);
			return -1;
		}
	}

	preamble_code = MOO_METHOD_GET_PREAMBLE_CODE(preamble);
	switch (preamble_code)
	{
		case MOO_METHOD_PREAMBLE_RETURN_RECEIVER:
			LOG_INST0 (moo, "preamble_return_receiver");
			MOO_STACK_POPS (moo, nargs); /* pop arguments only*/
			break;

		/* [NOTE] this is useless becuase  it returns a caller's context
		 *        as the callee's context has not been created yet. 
		case MOO_METHOD_PREAMBLE_RETURN_CONTEXT:
			LOG_INST0 (moo, "preamble_return_context");
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, (moo_oop_t)moo->active_context);
			break;
		*/

		case MOO_METHOD_PREAMBLE_RETURN_PROCESS:
			LOG_INST0 (moo, "preamble_return_process");
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, (moo_oop_t)moo->processor->active);
			break;

		case MOO_METHOD_PREAMBLE_RETURN_RECEIVER_NS:
		{
			moo_oop_t c;
			LOG_INST0 (moo, "preamble_return_receiver_ns");
			MOO_STACK_POPS (moo, nargs); /* pop arguments only*/
			c = MOO_STACK_GETTOP (moo); /* get receiver */
			c = (moo_oop_t)MOO_CLASSOF(moo, c);
			if (c == (moo_oop_t)moo->_class) c = MOO_STACK_GETTOP (moo);
			MOO_STACK_SETTOP (moo, (moo_oop_t)((moo_oop_class_t)c)->nsup);
			break;
		}

		case MOO_METHOD_PREAMBLE_RETURN_NIL:
			LOG_INST0 (moo, "preamble_return_nil");
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, moo->_nil);
			break;

		case MOO_METHOD_PREAMBLE_RETURN_TRUE:
			LOG_INST0 (moo, "preamble_return_true");
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, moo->_true);
			break;

		case MOO_METHOD_PREAMBLE_RETURN_FALSE:
			LOG_INST0 (moo, "preamble_return_false");
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, moo->_false);
			break;

		case MOO_METHOD_PREAMBLE_RETURN_INDEX: 
			/* preamble_index field is used to store a positive integer */
			LOG_INST1 (moo, "preamble_return_index %zd", MOO_METHOD_GET_PREAMBLE_INDEX(preamble));
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(MOO_METHOD_GET_PREAMBLE_INDEX(preamble)));
			break;

		case MOO_METHOD_PREAMBLE_RETURN_NEGINDEX:
			/* preamble_index field is used to store a negative integer */
			LOG_INST1 (moo, "preamble_return_negindex %zd", MOO_METHOD_GET_PREAMBLE_INDEX(preamble));
			MOO_STACK_POPS (moo, nargs);
			MOO_STACK_SETTOP (moo, MOO_SMOOI_TO_OOP(-MOO_METHOD_GET_PREAMBLE_INDEX(preamble)));
			break;

		case MOO_METHOD_PREAMBLE_RETURN_INSTVAR:
		{
			moo_oop_oop_t rcv;

			MOO_STACK_POPS (moo, nargs); /* pop arguments only */

			LOG_INST1 (moo, "preamble_return_instvar %zd", MOO_METHOD_GET_PREAMBLE_INDEX(preamble));

			/* replace the receiver by an instance variable of the receiver */
			rcv = (moo_oop_oop_t)MOO_STACK_GETTOP(moo);
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(rcv) == MOO_OBJ_TYPE_OOP);
			MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(rcv) > MOO_METHOD_GET_PREAMBLE_INDEX(preamble));

			if (rcv == (moo_oop_oop_t)moo->active_context)
			{
				/* the active context object doesn't keep
				 * the most up-to-date information in the 
				 * 'ip' and 'sp' field. commit these fields
				 * when the object to be accessed is 
				 * the active context. this manual commit
				 * is required because this premable handling
				 * skips activation of a new method context
				 * that would commit these fields. 
				 */
				STORE_ACTIVE_IP (moo);
				STORE_ACTIVE_SP (moo);
			}

			/* this accesses the instance variable of the receiver */
			MOO_STACK_SET (moo, moo->sp, rcv->slot[MOO_METHOD_GET_PREAMBLE_INDEX(preamble)]);
			break;
		}

		case MOO_METHOD_PREAMBLE_PRIMITIVE:
		{
			moo_ooi_t pfnum;

			stack_base = moo->sp - nargs - 1; /* stack base before receiver and arguments */

			pfnum = MOO_METHOD_GET_PREAMBLE_INDEX(preamble);
			LOG_INST1 (moo, "preamble_primitive %zd", pf_no);

			if (pfnum >= 0 && pfnum < MOO_COUNTOF(pftab))
			{
				int n;

				if ((nargs < pftab[pfnum].pfbase.minargs || nargs > pftab[pfnum].pfbase.maxargs))
				{
					MOO_LOG4 (moo, MOO_LOG_DEBUG,
						"Soft failure due to argument count mismatch for primitive function %hs - %zu-%zu expected, %zu given\n",
						pftab[pfnum].name, pftab[pfnum].pfbase.minargs, pftab[pfnum].pfbase.maxargs, nargs);
					moo_seterrnum (moo, MOO_ENUMARGS);
					goto activate_primitive_method_body;
				}

				moo_pushtmp (moo, (moo_oop_t*)&method);
				n = pftab[pfnum].pfbase.handler (moo, nargs);
				moo_poptmp (moo);
				if (n <= MOO_PF_HARD_FAILURE) 
				{
					MOO_LOG3 (moo, MOO_LOG_DEBUG,
						"Hard failure indicated by primitive function %p - %hs - return code %d\n",
						pftab[pfnum].pfbase.handler, pftab[pfnum].name, n);
					return -1;
				}
				if (n >= MOO_PF_SUCCESS) break;

				MOO_LOG2 (moo, MOO_LOG_DEBUG,
					"Soft failure indicated by primitive function %p - %hs\n",
					pftab[pfnum].pfbase.handler, pftab[pfnum].name);
			}
			else
			{
				MOO_LOG1 (moo, MOO_LOG_DEBUG, "Cannot call primitive function numbered %zd - unknown primitive function number\n", pfnum);
				moo_seterrbfmt (moo, MOO_ENOENT, "unknown primitive function number %zd", pfnum);
			}

			goto activate_primitive_method_body;
		}

		case MOO_METHOD_PREAMBLE_NAMED_PRIMITIVE:
		{
			moo_ooi_t pf_name_index;
			moo_pfbase_t* pfbase;
			moo_oop_t pfname;
			moo_oow_t w;

			stack_base = moo->sp - nargs - 1; /* stack base before receiver and arguments */

			/* index to the primitive function identifier in the literal frame */
			pf_name_index = MOO_METHOD_GET_PREAMBLE_INDEX(preamble);
			MOO_ASSERT (moo, pf_name_index >= 0);
			pfname = method->slot[pf_name_index];

		#if defined(MOO_BUILD_DEBUG)
			LOG_INST1 (moo, "preamble_named_primitive %zd", pf_name_index);
		#endif
			MOO_ASSERT (moo, MOO_OBJ_IS_CHAR_POINTER(pfname));
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_EXTRA(pfname));
			MOO_ASSERT (moo, MOO_CLASSOF(moo,pfname) == moo->_symbol);

			/* merge two SmallIntegers to get a full pointer from the cached data */
			w = (moo_oow_t)MOO_OOP_TO_SMOOI(method->preamble_data[0]) << (MOO_OOW_BITS / 2) | 
			    (moo_oow_t)MOO_OOP_TO_SMOOI(method->preamble_data[1]);
			pfbase = (moo_pfbase_t*)w;
			if (pfbase) goto exec_handler; /* skip moo_querymod() */

			pfbase = moo_querymod (moo, MOO_OBJ_GET_CHAR_SLOT(pfname), MOO_OBJ_GET_SIZE(pfname));
			if (pfbase)
			{
				int n;

				/* split a pointer to two OOP fields as SmallIntegers for storing/caching. */
				method->preamble_data[0] = MOO_SMOOI_TO_OOP((moo_oow_t)pfbase >> (MOO_OOW_BITS / 2));
				method->preamble_data[1] = MOO_SMOOI_TO_OOP((moo_oow_t)pfbase & MOO_LBMASK(moo_oow_t, MOO_OOW_BITS / 2));

			exec_handler:
				if (nargs < pfbase->minargs || nargs > pfbase->maxargs)
				{
					MOO_LOG5 (moo, MOO_LOG_DEBUG,
						"Soft failure due to argument count mismatch for primitive function %.*js - %zu-%zu expected, %zu given\n",
						MOO_OBJ_GET_SIZE(pfname), MOO_OBJ_GET_CHAR_SLOT(pfname), pfbase->minargs, pfbase->maxargs, nargs);

					moo_seterrbfmt (moo, MOO_ENUMARGS,
						"argument count mismatch for %.*js, %zu-%zu expected, %zu given",
						MOO_OBJ_GET_SIZE(pfname), MOO_OBJ_GET_CHAR_SLOT(pfname), pfbase->minargs, pfbase->maxargs, nargs);
					goto activate_primitive_method_body;
				}

				moo_pushtmp (moo, (moo_oop_t*)&method);

				/* the primitive handler is executed without activating the method itself.
				 * one major difference between the primitive function and the normal method
				 * invocation is that the primitive function handler should access arguments
				 * directly in the stack unlik a normal activated method context where the
				 * arguments are copied to the back. */

				moo_seterrnum (moo, MOO_ENOERR);
				n = pfbase->handler (moo, nargs);

				moo_poptmp (moo);
				if (n <= MOO_PF_HARD_FAILURE) 
				{
					MOO_LOG4 (moo, MOO_LOG_DEBUG, 
						"Hard failure indicated by primitive function %p - %.*js - return code %d\n",
						pfbase->handler, MOO_OBJ_GET_SIZE(pfname), MOO_OBJ_GET_CHAR_SLOT(pfname), n);
					return -1; /* hard primitive failure */
				}
				if (n >= MOO_PF_SUCCESS) break; /* primitive ok*/

				/* soft primitive failure */
				MOO_LOG3 (moo, MOO_LOG_DEBUG,
					"Soft failure indicated by primitive function %p - %.*js\n",
					pfbase->handler, MOO_OBJ_GET_SIZE(pfname), MOO_OBJ_GET_CHAR_SLOT(pfname));
			}
			else
			{
				/* no handler found */
				MOO_LOG2 (moo, MOO_LOG_DEBUG, 
					"Soft failure for non-existent primitive function - %.*js\n",
					MOO_OBJ_GET_SIZE(pfname), MOO_OBJ_GET_CHAR_SLOT(pfname));
			}

		activate_primitive_method_body:
			if (MOO_METHOD_GET_PREAMBLE_FLAGS(preamble) & MOO_METHOD_PREAMBLE_FLAG_LENIENT)
			{
				/* convert soft failure to error return */
				moo->sp = stack_base;
				MOO_STACK_PUSH (moo, MOO_ERROR_TO_OOP(moo->errnum));
				break;
			}

			/* set the error number in the current process for 'thisProcess primError' */
			moo->processor->active->perr = MOO_ERROR_TO_OOP(moo->errnum);
			if (moo->errmsg.len > 0)
			{
				/* compose an error message string. */
				/* TODO: i don't like to do this here.
				 *       is it really a good idea to compose a string here which
				 *       is not really failure safe without losing integrity???? */
				moo_oop_t tmp;
				moo_pushtmp (moo, (moo_oop_t*)&method);
				tmp = moo_makestring (moo, moo->errmsg.buf, moo->errmsg.len);
				moo_poptmp (moo);
				/* [NOTE] carry on even if instantiation fails */
				moo->processor->active->perrmsg = tmp? tmp: moo->_nil; /* TODO: set to nil or set to an empty string if instantiation fails? */
			}
			else
			{
				moo->processor->active->perrmsg = moo->_nil;
			}

		#if defined(MOO_USE_METHOD_TRAILER)
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TRAILER(method));
			if (MOO_METHOD_GET_CODE_SIZE(method) == 0) /* this trailer size field is not a small integer */
		#else
			if (method->code == moo->_nil)
		#endif
			{
				/* no byte code to execute - invoke 'self primitiveFailed' */

				static moo_ooch_t prim_fail_msg[] = { 
					'p', 'r', 'i', 'm', 'i', 't', 'i', 'v', 'e', 
					'F', 'a', 'i', 'l', 'e', 'd'
				};
				moo_oow_t i;

				if (stack_base != moo->sp - nargs - 1)
				{
					/* a primitive function handler must not touch the stack when it returns soft failure */
					MOO_LOG3 (moo, MOO_LOG_DEBUG, "Stack seems to get corrupted by a primitive handler function - %O>>%.*js\n", method->owner, MOO_OBJ_GET_SIZE(method->name), MOO_OBJ_GET_CHAR_SLOT(method->name));
					moo_seterrnum (moo, MOO_EINTERN);
					return -1;
				}

				MOO_LOG3 (moo, MOO_LOG_DEBUG, "Sending primitiveFailed - %O>>%.*js\n", method->owner, MOO_OBJ_GET_SIZE(method->name), MOO_OBJ_GET_CHAR_SLOT(method->name));
				/* 
				 *  | arg1     | <---- stack_base + 3
				 *  | arg0     | <---- stack_base + 2
				 *  | receiver | <---- stack_base + 1
				 *  |          | <---- stack_base
				 */

				/* push out arguments by one slot */
				MOO_STACK_PUSH (moo, moo->_nil); /* fake */
				for (i = moo->sp; i > stack_base + 2; i--) MOO_STACK_SET (moo, i, MOO_STACK_GET(moo, i - 1));
				/* inject the method as the first argument */
				MOO_STACK_SET (moo, stack_base + 2, (moo_oop_t)method);

				/* send primitiveFailed to self */
				if (send_message_with_str (moo, prim_fail_msg, 15, 0, nargs + 1) <= -1) return -1;
			}
			else
			{
				/* arrange to execute the method body */
				if (activate_new_method (moo, method, nargs) <= -1) return -1;
			}
			break;
		}

		default:
			MOO_ASSERT (moo, preamble_code == MOO_METHOD_PREAMBLE_NONE ||
			                 preamble_code == MOO_METHOD_PREAMBLE_RETURN_CONTEXT ||
			                 preamble_code == MOO_METHOD_PREAMBLE_EXCEPTION ||
			                 preamble_code == MOO_METHOD_PREAMBLE_ENSURE);
			if (activate_new_method (moo, method, nargs) <= -1) return -1;
			break;
	}

	return 0;
}

static int send_message (moo_t* moo, moo_oop_char_t selector, int to_super, moo_ooi_t nargs)
{
	moo_oocs_t mthname;
	moo_oop_t receiver;
	moo_oop_method_t method;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(selector));
	MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(selector) == MOO_OBJ_TYPE_CHAR);
	MOO_ASSERT (moo, MOO_CLASSOF(moo, selector) == moo->_symbol);

	receiver = MOO_STACK_GET(moo, moo->sp - nargs);

	mthname.ptr = MOO_OBJ_GET_CHAR_SLOT(selector);
	mthname.len = MOO_OBJ_GET_SIZE(selector);
	method = moo_findmethod (moo, receiver, &mthname, to_super);
	if (!method) 
	{
		static moo_ooch_t fbm[] = { 
			'd', 'o', 'e', 's', 
			'N', 'o', 't',
			'U', 'n', 'd', 'e', 'r', 's', 't', 'a', 'n', 'd', ':'
		};
		mthname.ptr = fbm;
		mthname.len = 18;

		method = moo_findmethod (moo, receiver, &mthname, 0);
		if (!method)
		{
			/* this must not happen as long as doesNotUnderstand: is implemented under Apex.
			 * this check should indicate a very serious internal problem */
			MOO_LOG4 (moo, MOO_LOG_IC | MOO_LOG_FATAL, 
				"Fatal error - receiver [%O] of class [%O] does not understand a message [%.*js]\n", 
				receiver, MOO_CLASSOF(moo, receiver), mthname.len, mthname.ptr);

			moo_seterrnum (moo, MOO_EMSGSND);
			return -1;
		}
		else
		{
			/* manipulate the stack as if 'receier doesNotUnderstand: selector' 
			 * has been called. */
/* TODO: if i manipulate the stack this way here, the stack trace for the last call is kind of lost.
 *       how can i preserve it gracefully? */
			MOO_STACK_POPS (moo, nargs);
			nargs = 1;
			MOO_STACK_PUSH (moo, (moo_oop_t)selector);
		}
	}

	return start_method (moo, method, nargs);
}

static int send_message_with_str (moo_t* moo, const moo_ooch_t* nameptr, moo_oow_t namelen, int to_super, moo_ooi_t nargs)
{
	moo_oocs_t mthname;
	moo_oop_t receiver;
	moo_oop_method_t method;

	receiver = MOO_STACK_GET(moo, moo->sp - nargs);

	mthname.ptr = (moo_ooch_t*)nameptr;
	mthname.len = namelen;
	method = moo_findmethod (moo, receiver, &mthname, to_super);
	if (!method)
	{
		MOO_LOG4 (moo, MOO_LOG_IC | MOO_LOG_FATAL, 
			"Fatal error - receiver [%O] of class [%O] does not understand a private message [%.*js]\n", 
			receiver, MOO_CLASSOF(moo, receiver), mthname.len, mthname.ptr);
		moo_seterrnum (moo, MOO_EMSGSND);
		return -1;
	}

	return start_method (moo, method, nargs);
}

/* ------------------------------------------------------------------------- */

static MOO_INLINE int switch_process_if_needed (moo_t* moo)
{
	if (moo->sem_heap_count > 0)
	{
		/* handle timed semaphores */
		moo_ntime_t ft, now;

		vm_gettime (moo, &now);

		do
		{
			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(moo->sem_heap[0]->heap_ftime_sec));
			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(moo->sem_heap[0]->heap_ftime_nsec));

			MOO_INITNTIME (&ft,
				MOO_OOP_TO_SMOOI(moo->sem_heap[0]->heap_ftime_sec),
				MOO_OOP_TO_SMOOI(moo->sem_heap[0]->heap_ftime_nsec)
			);

			if (MOO_CMPNTIME(&ft, (moo_ntime_t*)&now) <= 0)
			{
				moo_oop_process_t proc;

				/* waited long enough. signal the semaphore */

				proc = signal_semaphore (moo, moo->sem_heap[0]);
				/* [NOTE] no moo_pushtmp() on proc. no GC must occur
				 *        in the following line until it's used for
				 *        wake_process() below. */
				delete_from_sem_heap (moo, 0); /* moo->sem_heap_count is decremented */

				/* if no process is waiting on the semaphore, 
				 * signal_semaphore() returns moo->_nil. */

				if (moo->processor->active == moo->nil_process && (moo_oop_t)proc != moo->_nil)
				{
					/* this is the only runnable process. 
					 * switch the process to the running state.
					 * it uses wake_process() instead of
					 * switch_to_process() as there is no running 
					 * process at this moment */

				#if defined(MOO_DEBUG_VM_PROCESSOR) && (MOO_DEBUG_VM_PROCESSOR >= 2)
					MOO_LOG2 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Processor - switching to a process[%zd] while no process is active - total runnables %zd\n", MOO_OOP_TO_SMOOI(proc->id), MOO_OOP_TO_SMOOI(moo->processor->runnable.count));
				#endif

					MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
					MOO_ASSERT (moo, proc == moo->processor->runnable.last); /* resume_process() appends to the runnable list */
				#if 0
					wake_process (moo, proc); /* switch to running */
					moo->proc_switched = 1;
				#else
					switch_to_process_from_nil (moo, proc);
				#endif
				}
			}
			else if (moo->processor->active == moo->nil_process)
			{
				/* no running process. before firing time. */
				MOO_SUBNTIME (&ft, &ft, (moo_ntime_t*)&now);

				if (moo->sem_io_wait_count > 0)
				{
					/* no running process but io semaphore being waited on */
					vm_muxwait (moo, &ft);

					/* exit early if a process has been woken up. 
					 * the break in the else part further down will get hit
					 * eventually even if the following line doesn't exist.
					 * having the following line causes to skip firing the
					 * timed semaphore that would expire between now and the 
					 * moment the next inspection occurs. */
					if (moo->processor->active != moo->nil_process) goto switch_to_next;
				}
				else
				{
					/* no running process, no io semaphore */
					if ((moo_oop_t)moo->sem_gcfin != moo->_nil && moo->sem_gcfin_sigreq) goto signal_sem_gcfin;
					vm_sleep (moo, &ft);
				}
				vm_gettime (moo, &now);
			}
			else 
			{
				/* there is a running process. go on */
				break;
			}
		}
		while (moo->sem_heap_count > 0 && !moo->abort_req);
	}

	if (moo->sem_io_wait_count > 0) 
	{
		if (moo->processor->active == moo->nil_process)
		{
			moo_ntime_t ft;

			/* no runnable process while there is an io semaphore being waited */
			if ((moo_oop_t)moo->sem_gcfin != moo->_nil && moo->sem_gcfin_sigreq) goto signal_sem_gcfin;

			do
			{
				
				MOO_INITNTIME (&ft, 3, 0); /* TODO: use a configured time */
				vm_muxwait (moo, &ft);
			}
			while (moo->processor->active == moo->nil_process && !moo->abort_req);
		}
		else
		{
			/* well, there is a process waiting on one or more semaphores while
			 * there are other normal processes to run. check IO activities
			 * before proceeding to handle normal process scheduling */

			/* NOTE: the check with the multiplexer may happen too frequently
			 *       because this is called everytime process switching is requested
			 *       the actual callback implementation may avoid using actual
			 *       system calls for the check too frequently */
			vm_muxwait (moo, MOO_NULL);
		}
	}

	if ((moo_oop_t)moo->sem_gcfin != moo->_nil) 
	{
		moo_oop_process_t proc;

		if (moo->sem_gcfin_sigreq)
		{
		signal_sem_gcfin:
			MOO_LOG0 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "Signaled GCFIN semaphore\n");
			proc = signal_semaphore (moo, moo->sem_gcfin);

			if (moo->processor->active == moo->nil_process && (moo_oop_t)proc != moo->_nil)
			{
				MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
				MOO_ASSERT (moo, proc == moo->processor->runnable.first);
				switch_to_process_from_nil (moo, proc);
			}

			moo->sem_gcfin_sigreq = 0;
		}
		else
		{
			/* the gcfin semaphore signalling is not requested and there are 
			 * no runnable processes nor no waiting semaphores. if there is 
			 * process waiting on the gcfin semaphore, i will just schedule
			 * it to run by calling signal_semaphore() on moo->sem_gcfin.
			 */
			/* TODO: check if this is the best implementation practice */
			if (moo->processor->active == moo->nil_process)
			{
				/* there is no active process. in most cases, the only process left
				 * should be the gc finalizer process started in the System>>startup.
				 * if there are other suspended processes at this point, the processes
				 * are not likely to run again. 
				 * 
				 * imagine the following single line program that creates a process 
				 * but never start it.
				 *
				 *    method(#class) main { | p |  p := [] newProcess. }
				 *
				 * the gc finalizer process and the process assigned to p exist. 
				 * when the code reaches here, the 'p' process still is alive
				 * despite no active process nor no process waiting on timers
				 * and semaphores. so when the entire program terminates, there
				 * might still be some suspended processes that are not possible
				 * to schedule.
				 */

				MOO_LOG4 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, 
					"Signaled GCFIN semaphore without gcfin signal request - total %zd runnable/running %zd suspended %zd - sem_io_wait_count %zu\n",
					MOO_OOP_TO_SMOOI(moo->processor->total_count),
					MOO_OOP_TO_SMOOI(moo->processor->runnable.count),
					MOO_OOP_TO_SMOOI(moo->processor->suspended.count),
					moo->sem_io_wait_count);
				proc = signal_semaphore (moo, moo->sem_gcfin);
				if ((moo_oop_t)proc != moo->_nil) 
				{
					MOO_ASSERT (moo, proc->state == MOO_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
					MOO_ASSERT (moo, proc == moo->processor->runnable.first);
					moo->processor->should_exit = moo->_true; /* prepare to inform the gc finalizer process */
					switch_to_process_from_nil (moo, proc); /* sechedule the gc finalizer process */
				}
			}
		}
	}

#if 0
	while (moo->sem_list_count > 0)
	{
		/* handle async signals */
		--moo->sem_list_count;
		signal_semaphore (moo, moo->sem_list[moo->sem_list_count]);
		if (moo->processor->active == moo->nil_process)
		{
		}
	}
	/*
	if (semaphore heap has pending request)
	{
		signal them...
	}*/
#endif

	if (moo->processor->active == moo->nil_process) 
	{
		/* no more waiting semaphore and no more process */
		MOO_ASSERT (moo, moo->processor->runnable.count = MOO_SMOOI_TO_OOP(0));
		MOO_LOG0 (moo, MOO_LOG_IC | MOO_LOG_DEBUG, "No more runnable process\n");
		if (MOO_OOP_TO_SMOOI(moo->processor->suspended.count) > 0)
		{
			/* there exist suspended processes while no processes are runnable.
			 * most likely, the running program contains process/semaphore related bugs */
			MOO_LOG1 (moo, MOO_LOG_IC | MOO_LOG_WARN, 
				"%zd suspended process(es) found - check your program\n",
				MOO_OOP_TO_SMOOI(moo->processor->suspended.count));
		}
		return 0;
	}

switch_to_next:
	/* TODO: implement different process switching scheme - time-slice or clock based??? */
#if defined(MOO_EXTERNAL_PROCESS_SWITCH)
	if (moo->switch_proc)
	{
#endif
		if (!moo->proc_switched) 
		{
			switch_to_next_runnable_process (moo);
			moo->proc_switched = 0;
		}
#if defined(MOO_EXTERNAL_PROCESS_SWITCH)
		moo->switch_proc = 0;
	}
	else moo->proc_switched = 0;
#endif

	return 1;
}


static MOO_INLINE int do_return (moo_t* moo, moo_oob_t bcode, moo_oop_t return_value)
{
#if 0
	/* put the instruction pointer back to the return
	 * instruction (RETURN_RECEIVER or RETURN_RECEIVER)
	 * if a context returns into this context again,
	 * it'll be able to return as well again.
	 * 
	 * Consider a program like this:
	 *
	 * #class MyObject(Object)
	 * {
	 *   #declare(#classinst) t1 t2.
	 *   #method(#class) xxxx
	 *   {
	 *     | g1 g2 |
	 *     t1 dump.
	 *     t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
	 *     (t1 < 100) ifFalse: [ ^self ].
	 *     t1 := t1 + 1. 
	 *     ^self xxxx.
	 *   }
	 *   #method(#class) main
	 *   {
	 *     t1 := 1.
	 *     self xxxx.
	 *     t2 := t2 value.  
	 *     t2 dump.
	 *   }
	 * }
	 *
	 * the 'xxxx' method invoked by 'self xxxx' has 
	 * returned even before 't2 value' is executed.
	 * the '^' operator makes the active context to
	 * switch to its 'origin->sender' which is the
	 * method context of 'xxxx' itself. placing its
	 * instruction pointer at the 'return' instruction
	 * helps execute another return when the switching
	 * occurs.
	 * 
	 * TODO: verify if this really works
	 *
	 */
	moo->ip--; 
#else
	int unwind_protect;
	moo_oop_context_t unwind_start;
	moo_oop_context_t unwind_stop;

	if (MOO_UNLIKELY(moo->active_context->origin == moo->processor->active->initial_context->origin))
	{
		/* method return from a processified block
		 * 
		 * #method(#class) main
		 * {
		 *    [^100] newProcess resume.
		 *    '1111' dump.
		 *    '1111' dump.
		 *    '1111' dump.
		 *    ^300.
		 * }
		 * 
		 * ^100 doesn't terminate a main process as the block
		 * has been processified. on the other hand, ^100
		 * in the following program causes main to exit.
		 * 
		 * #method(#class) main
		 * {
		 *    [^100] value.
		 *    '1111' dump.
		 *    '1111' dump.
		 *    '1111' dump.
		 *    ^300.
		 * }
		 */

		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context) == moo->_block_context);
		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->processor->active->initial_context) == moo->_block_context);

		/* decrement the instruction pointer back to the return instruction.
		 * even if the context is reentered, it will just return.
		 *moo->ip--;*/

		terminate_process (moo, moo->processor->active);
	}
	else 
	{
		unwind_protect = 0;

		/* set the instruction pointer to an invalid value.
		 * this is stored into the current method context
		 * before context switching and marks a dead context */
		if (moo->active_context->origin == moo->active_context)
		{
			/* returning from a method */
			MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context) == moo->_method_context);

			/* mark that the context is dead. it will be 
			 * save to the context object by SWITCH_ACTIVE_CONTEXT() */
			moo->ip = -1;
		}
		else
		{
			moo_oop_context_t ctx;

			/* method return from within a block(including a non-local return) */
			MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context) == moo->_block_context);

			ctx = moo->active_context;
			while ((moo_oop_t)ctx != moo->_nil)
			{
				if (MOO_CLASSOF(moo, ctx) == moo->_method_context)
				{
					moo_ooi_t preamble;
					preamble = MOO_OOP_TO_SMOOI(((moo_oop_method_t)ctx->method_or_nargs)->preamble);
					if (MOO_METHOD_GET_PREAMBLE_CODE(preamble) == MOO_METHOD_PREAMBLE_ENSURE)
					{
						if (!unwind_protect)
						{
							unwind_protect = 1;
							unwind_start = ctx;
						}
						unwind_stop = ctx;
					}
				}
				if (ctx == moo->active_context->origin) goto non_local_return_ok;
				ctx = ctx->sender;
			}

			/* cannot return from a method that has returned already */
			MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context->origin) == moo->_method_context);
			MOO_ASSERT (moo, moo->active_context->origin->ip == MOO_SMOOI_TO_OOP(-1));

			MOO_LOG0 (moo, MOO_LOG_IC | MOO_LOG_ERROR, "Error - cannot return from dead context\n");
			moo_seterrnum (moo, MOO_EINTERN); /* TODO: can i make this error catchable at the moo level? */
			return -1;

		non_local_return_ok:
/*MOO_LOG2 (moo, MOO_LOG_DEBUG, "NON_LOCAL RETURN OK TO... %p %p\n", moo->active_context->origin, moo->active_context->origin->sender);*/
			if (bcode != BCODE_LOCAL_RETURN)
			{
				/* mark that the context is dead */
				moo->active_context->origin->ip = MOO_SMOOI_TO_OOP(-1);
			}
		}

		/* the origin must always be a method context for both an active block context 
		 * or an active method context */
		MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context->origin) == moo->_method_context);

		/* restore the stack pointer */
		moo->sp = MOO_OOP_TO_SMOOI(moo->active_context->origin->sp);
		if (bcode == BCODE_LOCAL_RETURN && moo->active_context != moo->active_context->origin)
		{
			SWITCH_ACTIVE_CONTEXT (moo, moo->active_context->origin);
		}
		else
		{
			SWITCH_ACTIVE_CONTEXT (moo, moo->active_context->origin->sender);
		}

		if (unwind_protect)
		{
			static moo_ooch_t fbm[] = { 
				'u', 'n', 'w', 'i', 'n', 'd', 'T', 'o', ':', 
				'r', 'e', 't', 'u', 'r', 'n', ':'
			};

			MOO_STACK_PUSH (moo, (moo_oop_t)unwind_start);
			MOO_STACK_PUSH (moo, (moo_oop_t)unwind_stop);
			MOO_STACK_PUSH (moo, (moo_oop_t)return_value);

			if (send_message_with_str (moo, fbm, 16, 0, 2) <= -1) return -1;
		}
		else
		{
			/* push the return value to the stack of the new active context */
			MOO_STACK_PUSH (moo, return_value);

			if (moo->active_context == moo->initial_context)
			{
				/* the new active context is the fake initial context.
				 * this context can't get executed further. */
				MOO_ASSERT (moo, (moo_oop_t)moo->active_context->sender == moo->_nil);
				MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context) == moo->_method_context);
				MOO_ASSERT (moo, moo->active_context->receiver_or_source == moo->_nil);
				MOO_ASSERT (moo, moo->active_context == moo->processor->active->initial_context);
				MOO_ASSERT (moo, moo->active_context->origin == moo->processor->active->initial_context->origin);
				MOO_ASSERT (moo, moo->active_context->origin == moo->active_context);

				/* NOTE: this condition is true for the processified block context also.
				 *   moo->active_context->origin == moo->processor->active->initial_context->origin
				 *   however, the check here is done after context switching and the
				 *   processified block check has been done against the context before switching */

				/* the stack contains the final return value so the stack pointer must be 0. */
				MOO_ASSERT (moo, moo->sp == 0); 

				if (moo->option.trait & MOO_AWAIT_PROCS)
				{
					terminate_process (moo, moo->processor->active);
				}
				else
				{
					/* graceful termination of the whole vm */
					return 0;
				}

				/* TODO: store the return value to the VM register.
				 * the caller to moo_execute() can fetch it to return it to the system */
			}
		}
	}
#endif

	return 1;
}


static MOO_INLINE void do_return_from_block (moo_t* moo)
{
	LOG_INST0 (moo, "return_from_block");

	MOO_ASSERT (moo, MOO_CLASSOF(moo, moo->active_context) == moo->_block_context);

	if (moo->active_context == moo->processor->active->initial_context)
	{
		/* the active context to return from is an initial context of
		 * the active process. this process must have been created 
		 * over a block using the newProcess method. let's terminate
		 * the process. */

		MOO_ASSERT (moo, (moo_oop_t)moo->active_context->sender == moo->_nil);
		terminate_process (moo, moo->processor->active);
	}
	else
	{
		/* it is a normal block return as the active block context 
		 * is not the initial context of a process */

		/* the process stack is shared. the return value 
		 * doesn't need to get moved. */
		SWITCH_ACTIVE_CONTEXT (moo, (moo_oop_context_t)moo->active_context->sender);
	}
}

static MOO_INLINE int make_block (moo_t* moo)
{
	moo_oop_context_t blkctx;
	moo_oob_t b1, b2;

	/* b1 - number of block arguments
	 * b2 - number of block temporaries */
	FETCH_PARAM_CODE_TO (moo, b1);
	FETCH_PARAM_CODE_TO (moo, b2);

	LOG_INST2 (moo, "make_block %zu %zu", b1, b2);

	MOO_ASSERT (moo, b1 >= 0);
	MOO_ASSERT (moo, b2 >= b1);

	/* the block context object created here is used as a base
	 * object for block context activation. pf_block_value()
	 * clones a block context and activates the cloned context.
	 * this base block context is created with no stack for 
	 * this reason */
	blkctx = (moo_oop_context_t)moo_instantiate (moo, moo->_block_context, MOO_NULL, 0); 
	if (!blkctx) return -1;

	/* the long forward jump instruction has the format of 
	 *   11000100 KKKKKKKK or 11000100 KKKKKKKK KKKKKKKK 
	 * depending on MOO_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
	 * the instruction after the jump. */
	blkctx->ip = MOO_SMOOI_TO_OOP(moo->ip + MOO_BCODE_LONG_PARAM_SIZE + 1);
	/* stack pointer below the bottom. this base block context
	 * has an empty stack anyway. */
	blkctx->sp = MOO_SMOOI_TO_OOP(-1);
	/* the number of arguments for a block context is local to the block */
	blkctx->method_or_nargs = MOO_SMOOI_TO_OOP(b1);
	/* the number of temporaries here is an accumulated count including
	 * the number of temporaries of a home context */
	blkctx->ntmprs = MOO_SMOOI_TO_OOP(b2);

	/* set the home context where it's defined */
	blkctx->home = (moo_oop_t)moo->active_context; 
	/* no source for a base block context. */
	blkctx->receiver_or_source = moo->_nil; 

	blkctx->origin = moo->active_context->origin;

	/* push the new block context to the stack of the active context */
	MOO_STACK_PUSH (moo, (moo_oop_t)blkctx);
	return 0;
}

static int __execute (moo_t* moo)
{
	moo_oob_t bcode;
	moo_oow_t b1, b2;
	moo_oop_t return_value;

#if defined(HAVE_LABELS_AS_VALUES)
	static void* inst_table[256] = 
	{
		#include "moo-bct.h"
	};

#	define BEGIN_DISPATCH_LOOP() __begin_inst_dispatch: 
#	define END_DISPATCH_LOOP() __end_inst_dispatch:
#	define EXIT_DISPATCH_LOOP() goto __end_inst_dispatch
#	define NEXT_INST() goto __begin_inst_dispatch

#	define BEGIN_DISPATCH_TABLE() goto *inst_table[bcode];
#	define END_DISPATCH_TABLE()

#	define ON_INST(code) case_ ## code:
#	define ON_UNKNOWN_INST() case_ ## DEFAULT:

#else
#	define BEGIN_DISPATCH_LOOP() __begin_inst_dispatch: 
#	define END_DISPATCH_LOOP() __end_inst_dispatch:
#	define EXIT_DISPATCH_LOOP() goto __end_inst_dispatch
#	define NEXT_INST() goto __begin_inst_dispatch

#	define BEGIN_DISPATCH_TABLE() switch (bcode) {
#	define END_DISPATCH_TABLE()   }

#	define ON_INST(code) case code:
#	define ON_UNKNOWN_INST()  default:

#endif

	MOO_ASSERT (moo, moo->active_context != MOO_NULL);

/* TODO: initialize semaphore stuffs 
 *   sem_heap
 *   sem_io.
 *   sem_list.
 * these can get dirty if this function is called again esepcially after failure.
 */

	BEGIN_DISPATCH_LOOP()

		/* stop requested or no more runnable process */
		if (moo->abort_req || switch_process_if_needed(moo) == 0) EXIT_DISPATCH_LOOP();

	#if defined(MOO_DEBUG_VM_EXEC)
		moo->last_inst_pointer = moo->ip;
	#endif

		bcode = FETCH_BYTE_CODE(moo);

	#if defined(MOO_PROFILE_VM)
		moo->inst_counter++;
	#endif

		/* ==== DISPATCH TABLE ==== */
		BEGIN_DISPATCH_TABLE()

		ON_INST(BCODE_PUSH_INSTVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto push_instvar;
		ON_INST(BCODE_PUSH_INSTVAR_0)
		ON_INST(BCODE_PUSH_INSTVAR_1)
		ON_INST(BCODE_PUSH_INSTVAR_2)
		ON_INST(BCODE_PUSH_INSTVAR_3)
		ON_INST(BCODE_PUSH_INSTVAR_4)
		ON_INST(BCODE_PUSH_INSTVAR_5)
		ON_INST(BCODE_PUSH_INSTVAR_6)
		ON_INST(BCODE_PUSH_INSTVAR_7)
			b1 = bcode & 0x7; /* low 3 bits */
		push_instvar:
			LOG_INST1 (moo, "push_instvar %zu", b1);
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(moo->active_context->origin->receiver_or_source) == MOO_OBJ_TYPE_OOP);
			MOO_STACK_PUSH (moo, ((moo_oop_oop_t)moo->active_context->origin->receiver_or_source)->slot[b1]);
			NEXT_INST();

		/* ------------------------------------------------- */

		ON_INST(BCODE_STORE_INTO_INSTVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto store_instvar;
		ON_INST(BCODE_STORE_INTO_INSTVAR_0)
		ON_INST(BCODE_STORE_INTO_INSTVAR_1)
		ON_INST(BCODE_STORE_INTO_INSTVAR_2)
		ON_INST(BCODE_STORE_INTO_INSTVAR_3)
		ON_INST(BCODE_STORE_INTO_INSTVAR_4)
		ON_INST(BCODE_STORE_INTO_INSTVAR_5)
		ON_INST(BCODE_STORE_INTO_INSTVAR_6)
		ON_INST(BCODE_STORE_INTO_INSTVAR_7)
			b1 = bcode & 0x7; /* low 3 bits */
		store_instvar:
			LOG_INST1 (moo, "store_into_instvar %zu", b1);
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(moo->active_context->receiver_or_source) == MOO_OBJ_TYPE_OOP);
			((moo_oop_oop_t)moo->active_context->origin->receiver_or_source)->slot[b1] = MOO_STACK_GETTOP(moo);
			NEXT_INST();

		/* ------------------------------------------------- */
		ON_INST(BCODE_POP_INTO_INSTVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto pop_into_instvar;
		ON_INST(BCODE_POP_INTO_INSTVAR_0)
		ON_INST(BCODE_POP_INTO_INSTVAR_1)
		ON_INST(BCODE_POP_INTO_INSTVAR_2)
		ON_INST(BCODE_POP_INTO_INSTVAR_3)
		ON_INST(BCODE_POP_INTO_INSTVAR_4)
		ON_INST(BCODE_POP_INTO_INSTVAR_5)
		ON_INST(BCODE_POP_INTO_INSTVAR_6)
		ON_INST(BCODE_POP_INTO_INSTVAR_7)
			b1 = bcode & 0x7; /* low 3 bits */
		pop_into_instvar:
			LOG_INST1 (moo, "pop_into_instvar %zu", b1);
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(moo->active_context->receiver_or_source) == MOO_OBJ_TYPE_OOP);
			((moo_oop_oop_t)moo->active_context->origin->receiver_or_source)->slot[b1] = MOO_STACK_GETTOP(moo);
			MOO_STACK_POP (moo);
			NEXT_INST();

		/* ------------------------------------------------- */
		ON_INST(BCODE_PUSH_TEMPVAR_X)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_X)
		ON_INST(BCODE_POP_INTO_TEMPVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto handle_tempvar;

		ON_INST(BCODE_PUSH_TEMPVAR_0)
		ON_INST(BCODE_PUSH_TEMPVAR_1)
		ON_INST(BCODE_PUSH_TEMPVAR_2)
		ON_INST(BCODE_PUSH_TEMPVAR_3)
		ON_INST(BCODE_PUSH_TEMPVAR_4)
		ON_INST(BCODE_PUSH_TEMPVAR_5)
		ON_INST(BCODE_PUSH_TEMPVAR_6)
		ON_INST(BCODE_PUSH_TEMPVAR_7)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_0)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_1)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_2)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_3)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_4)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_5)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_6)
		ON_INST(BCODE_STORE_INTO_TEMPVAR_7)
		ON_INST(BCODE_POP_INTO_TEMPVAR_0)
		ON_INST(BCODE_POP_INTO_TEMPVAR_1)
		ON_INST(BCODE_POP_INTO_TEMPVAR_2)
		ON_INST(BCODE_POP_INTO_TEMPVAR_3)
		ON_INST(BCODE_POP_INTO_TEMPVAR_4)
		ON_INST(BCODE_POP_INTO_TEMPVAR_5)
		ON_INST(BCODE_POP_INTO_TEMPVAR_6)
		ON_INST(BCODE_POP_INTO_TEMPVAR_7)
		{
			moo_oop_context_t ctx;
			moo_ooi_t bx;

			b1 = bcode & 0x7; /* low 3 bits */
		handle_tempvar:

		#if defined(MOO_USE_CTXTEMPVAR)
			/* when CTXTEMPVAR inststructions are used, the above 
			 * instructions are used only for temporary access 
			 * outside a block. i can assume that the temporary
			 * variable index is pointing to one of temporaries
			 * in the relevant method context */
			ctx = moo->active_context->origin;
			bx = b1;
			MOO_ASSERT (moo, MOO_CLASSOF(moo, ctx) == moo->_method_context);
		#else
			/* otherwise, the index may point to a temporaries
			 * declared inside a block */

			if (moo->active_context->home != moo->_nil)
			{
				/* this code assumes that the method context and
				 * the block context place some key fields in the
				 * same offset. such fields include 'home', 'ntmprs' */
				moo_oop_t home;
				moo_ooi_t home_ntmprs;

				ctx = moo->active_context;
				home = ctx->home;

				do
				{
					/* ntmprs contains the number of defined temporaries 
					 * including those defined in the home context */
					home_ntmprs = MOO_OOP_TO_SMOOI(((moo_oop_context_t)home)->ntmprs);
					if (b1 >= home_ntmprs) break;

					ctx = (moo_oop_context_t)home;
					home = ((moo_oop_context_t)home)->home;
					if (home == moo->_nil)
					{
						home_ntmprs = 0;
						break;
					}
				}
				while (1);

				/* bx is the actual index within the actual context 
				 * containing the temporary */
				bx = b1 - home_ntmprs;
			}
			else
			{
				ctx = moo->active_context;
				bx = b1;
			}
		#endif

			if ((bcode >> 4) & 1)
			{
				/* push - bit 4 on */
				LOG_INST1 (moo, "push_tempvar %zu", b1);
				MOO_STACK_PUSH (moo, ctx->slot[bx]);
			}
			else
			{
				/* store or pop - bit 5 off */
				ctx->slot[bx] = MOO_STACK_GETTOP(moo);

				if ((bcode >> 3) & 1)
				{
					/* pop - bit 3 on */
					LOG_INST1 (moo, "pop_into_tempvar %zu", b1);
					MOO_STACK_POP (moo);
				}
				else
				{
					LOG_INST1 (moo, "store_into_tempvar %zu", b1);
				}
			}

			NEXT_INST();
		}

		/* ------------------------------------------------- */
		ON_INST(BCODE_PUSH_LITERAL_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto push_literal;

		ON_INST(BCODE_PUSH_LITERAL_0)
		ON_INST(BCODE_PUSH_LITERAL_1)
		ON_INST(BCODE_PUSH_LITERAL_2)
		ON_INST(BCODE_PUSH_LITERAL_3)
		ON_INST(BCODE_PUSH_LITERAL_4)
		ON_INST(BCODE_PUSH_LITERAL_5)
		ON_INST(BCODE_PUSH_LITERAL_6)
		ON_INST(BCODE_PUSH_LITERAL_7)
			b1 = bcode & 0x7; /* low 3 bits */
		push_literal:
			LOG_INST1 (moo, "push_literal @%zu", b1);
			MOO_STACK_PUSH (moo, moo->active_method->slot[b1]);
			NEXT_INST();

		/* ------------------------------------------------- */
		ON_INST(BCODE_PUSH_OBJECT_X)
		ON_INST(BCODE_STORE_INTO_OBJECT_X)
		ON_INST(BCODE_POP_INTO_OBJECT_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			goto handle_object;

		ON_INST(BCODE_PUSH_OBJECT_0)
		ON_INST(BCODE_PUSH_OBJECT_1)
		ON_INST(BCODE_PUSH_OBJECT_2)
		ON_INST(BCODE_PUSH_OBJECT_3)
		ON_INST(BCODE_STORE_INTO_OBJECT_0)
		ON_INST(BCODE_STORE_INTO_OBJECT_1)
		ON_INST(BCODE_STORE_INTO_OBJECT_2)
		ON_INST(BCODE_STORE_INTO_OBJECT_3)
		ON_INST(BCODE_POP_INTO_OBJECT_0)
		ON_INST(BCODE_POP_INTO_OBJECT_1)
		ON_INST(BCODE_POP_INTO_OBJECT_2)
		ON_INST(BCODE_POP_INTO_OBJECT_3)
		{
			moo_oop_association_t ass;

			b1 = bcode & 0x3; /* low 2 bits */
		handle_object:
			ass = (moo_oop_association_t)moo->active_method->slot[b1];
			MOO_ASSERT (moo, MOO_CLASSOF(moo, ass) == moo->_association);

			if ((bcode >> 3) & 1)
			{
				/* store or pop */
				ass->value = MOO_STACK_GETTOP(moo);

				if ((bcode >> 2) & 1)
				{
					/* pop */
					LOG_INST1 (moo, "pop_into_object @%zu", b1);
					MOO_STACK_POP (moo);
				}
				else
				{
					LOG_INST1 (moo, "store_into_object @%zu", b1);
				}
			}
			else
			{
				/* push */
				LOG_INST1 (moo, "push_object @%zu", b1);
				MOO_STACK_PUSH (moo, ass->value);
			}
			NEXT_INST();
		}

		/* -------------------------------------------------------- */

		ON_INST(BCODE_JUMP_FORWARD_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_forward %zu", b1);
			moo->ip += b1;
			NEXT_INST();

		ON_INST(BCODE_JUMP_FORWARD_0)
		ON_INST(BCODE_JUMP_FORWARD_1)
		ON_INST(BCODE_JUMP_FORWARD_2)
		ON_INST(BCODE_JUMP_FORWARD_3)
			LOG_INST1 (moo, "jump_forward %zu", (moo_oow_t)(bcode & 0x3));
			moo->ip += (bcode & 0x3); /* low 2 bits */
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_backward %zu", b1);
			moo->ip -= b1;
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_0)
		ON_INST(BCODE_JUMP_BACKWARD_1)
		ON_INST(BCODE_JUMP_BACKWARD_2)
		ON_INST(BCODE_JUMP_BACKWARD_3)
			LOG_INST1 (moo, "jump_backward %zu", (moo_oow_t)(bcode & 0x3));
			moo->ip -= (bcode & 0x3); /* low 2 bits */
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_IF_FALSE_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_backward_if_false %zu", b1);
			if (MOO_STACK_GETTOP(moo) == moo->_false) moo->ip -= b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_IF_FALSE_0)
		ON_INST(BCODE_JUMP_BACKWARD_IF_FALSE_1)
		ON_INST(BCODE_JUMP_BACKWARD_IF_FALSE_2)
		ON_INST(BCODE_JUMP_BACKWARD_IF_FALSE_3)
			LOG_INST1 (moo, "jump_backward_if_false %zu", (moo_oow_t)(bcode & 0x3));
			if (MOO_STACK_GETTOP(moo) == moo->_false) moo->ip -= (bcode & 0x3); /* low 2 bits */
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_IF_TRUE_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_backward_if_true %zu", b1);
			/*if (MOO_STACK_GETTOP(moo) == moo->_true) moo->ip -= b1;*/
			if (MOO_STACK_GETTOP(moo) != moo->_false) moo->ip -= b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP_BACKWARD_IF_TRUE_0)
		ON_INST(BCODE_JUMP_BACKWARD_IF_TRUE_1)
		ON_INST(BCODE_JUMP_BACKWARD_IF_TRUE_2)
		ON_INST(BCODE_JUMP_BACKWARD_IF_TRUE_3)
			LOG_INST1 (moo, "jump_backward_if_true %zu", (moo_oow_t)(bcode & 0x3));
			/*if (MOO_STACK_GETTOP(moo) == moo->_true) moo->ip -= (bcode & 0x3);*/ /* low 2 bits */
			if (MOO_STACK_GETTOP(moo) != moo->_false) moo->ip -= (bcode & 0x3);
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP_FORWARD_IF_FALSE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_forward_if_false %zu", b1);
			if (MOO_STACK_GETTOP(moo) == moo->_false) moo->ip += b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP_FORWARD_IF_TRUE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump_forward_if_true %zu", b1);
			/*if (MOO_STACK_GETTOP(moo) == moo->_true) moo->ip += b1;*/
			if (MOO_STACK_GETTOP(moo) != moo->_false) moo->ip += b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP2_FORWARD)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_forward %zu", b1);
			moo->ip += MAX_CODE_JUMP + b1;
			NEXT_INST();

		ON_INST(BCODE_JUMP2_BACKWARD)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_backward %zu", b1);
			moo->ip -= MAX_CODE_JUMP + b1;
			NEXT_INST();

		ON_INST(BCODE_JUMP2_FORWARD_IF_FALSE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_forward_if_false %zu", b1);
			if (MOO_STACK_GETTOP(moo) == moo->_false) moo->ip += MAX_CODE_JUMP + b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP2_FORWARD_IF_TRUE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_forward_if_true %zu", b1);
			/*if (MOO_STACK_GETTOP(moo) == moo->_true) moo->ip += MAX_CODE_JUMP + b1;*/
			if (MOO_STACK_GETTOP(moo) != moo->_false) moo->ip += MAX_CODE_JUMP + b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP2_BACKWARD_IF_FALSE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_backward_if_false %zu", b1);
			if (MOO_STACK_GETTOP(moo) == moo->_false) moo->ip -= MAX_CODE_JUMP + b1;
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_JUMP2_BACKWARD_IF_TRUE)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "jump2_backward_if_true %zu", b1);
			/* if (MOO_STACK_GETTOP(moo) == moo->_true) moo->ip -= MAX_CODE_JUMP + b1; */
			if (MOO_STACK_GETTOP(moo) != moo->_false) moo->ip -= MAX_CODE_JUMP + b1;
			MOO_STACK_POP (moo);
			NEXT_INST();
		/* -------------------------------------------------------- */

		ON_INST(BCODE_PUSH_CTXTEMPVAR_X)
		ON_INST(BCODE_STORE_INTO_CTXTEMPVAR_X)
		ON_INST(BCODE_POP_INTO_CTXTEMPVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			FETCH_PARAM_CODE_TO (moo, b2);
			goto handle_ctxtempvar;
		ON_INST(BCODE_PUSH_CTXTEMPVAR_0)
		ON_INST(BCODE_PUSH_CTXTEMPVAR_1)
		ON_INST(BCODE_PUSH_CTXTEMPVAR_2)
		ON_INST(BCODE_PUSH_CTXTEMPVAR_3)
		ON_INST(BCODE_STORE_INTO_CTXTEMPVAR_0)
		ON_INST(BCODE_STORE_INTO_CTXTEMPVAR_1)
		ON_INST(BCODE_STORE_INTO_CTXTEMPVAR_2)
		ON_INST(BCODE_STORE_INTO_CTXTEMPVAR_3)
		ON_INST(BCODE_POP_INTO_CTXTEMPVAR_0)
		ON_INST(BCODE_POP_INTO_CTXTEMPVAR_1)
		ON_INST(BCODE_POP_INTO_CTXTEMPVAR_2)
		ON_INST(BCODE_POP_INTO_CTXTEMPVAR_3)
		{
			moo_ooi_t i;
			moo_oop_context_t ctx;

			b1 = bcode & 0x3; /* low 2 bits */
			b2 = FETCH_BYTE_CODE(moo);

		handle_ctxtempvar:

			ctx = moo->active_context;
			MOO_ASSERT (moo, (moo_oop_t)ctx != moo->_nil);
			for (i = 0; i < b1; i++)
			{
				ctx = (moo_oop_context_t)ctx->home;
			}

			if ((bcode >> 3) & 1)
			{
				/* store or pop */
				ctx->slot[b2] = MOO_STACK_GETTOP(moo);

				if ((bcode >> 2) & 1)
				{
					/* pop */
					MOO_STACK_POP (moo);
					LOG_INST2 (moo, "pop_into_ctxtempvar %zu %zu", b1, b2);
				}
				else
				{
					LOG_INST2 (moo, "store_into_ctxtempvar %zu %zu", b1, b2);
				}
			}
			else
			{
				/* push */
				MOO_STACK_PUSH (moo, ctx->slot[b2]);
				LOG_INST2 (moo, "push_ctxtempvar %zu %zu", b1, b2);
			}

			NEXT_INST();
		}
		/* -------------------------------------------------------- */

		ON_INST(BCODE_PUSH_OBJVAR_X)
		ON_INST(BCODE_STORE_INTO_OBJVAR_X)
		ON_INST(BCODE_POP_INTO_OBJVAR_X)
			FETCH_PARAM_CODE_TO (moo, b1);
			FETCH_PARAM_CODE_TO (moo, b2);
			goto handle_objvar;

		ON_INST(BCODE_PUSH_OBJVAR_0)
		ON_INST(BCODE_PUSH_OBJVAR_1)
		ON_INST(BCODE_PUSH_OBJVAR_2)
		ON_INST(BCODE_PUSH_OBJVAR_3)
		ON_INST(BCODE_STORE_INTO_OBJVAR_0)
		ON_INST(BCODE_STORE_INTO_OBJVAR_1)
		ON_INST(BCODE_STORE_INTO_OBJVAR_2)
		ON_INST(BCODE_STORE_INTO_OBJVAR_3)
		ON_INST(BCODE_POP_INTO_OBJVAR_0)
		ON_INST(BCODE_POP_INTO_OBJVAR_1)
		ON_INST(BCODE_POP_INTO_OBJVAR_2)
		ON_INST(BCODE_POP_INTO_OBJVAR_3)
		{
			moo_oop_oop_t t;

			/* b1 -> variable index to the object indicated by b2.
			 * b2 -> object index stored in the literal frame. */
			b1 = bcode & 0x3; /* low 2 bits */
			b2 = FETCH_BYTE_CODE(moo);

		handle_objvar:
			t = (moo_oop_oop_t)moo->active_method->slot[b2];
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(t) == MOO_OBJ_TYPE_OOP);
			MOO_ASSERT (moo, b1 < MOO_OBJ_GET_SIZE(t));

			if ((bcode >> 3) & 1)
			{
				/* store or pop */
				t->slot[b1] = MOO_STACK_GETTOP(moo);

				if ((bcode >> 2) & 1)
				{
					/* pop */
					MOO_STACK_POP (moo);
					LOG_INST2 (moo, "pop_into_objvar %zu %zu", b1, b2);
				}
				else
				{
					LOG_INST2 (moo, "store_into_objvar %zu %zu", b1, b2);
				}
			}
			else
			{
				/* push */
				LOG_INST2 (moo, "push_objvar %zu %zu", b1, b2);
				MOO_STACK_PUSH (moo, t->slot[b1]);
			}
			NEXT_INST();
		}

		/* -------------------------------------------------------- */
		ON_INST(BCODE_SEND_MESSAGE_X)
		ON_INST(BCODE_SEND_MESSAGE_TO_SUPER_X)
			/* b1 -> number of arguments 
			 * b2 -> selector index stored in the literal frame */
			FETCH_PARAM_CODE_TO (moo, b1);
			FETCH_PARAM_CODE_TO (moo, b2);
			goto handle_send_message;

		ON_INST(BCODE_SEND_MESSAGE_0)
		ON_INST(BCODE_SEND_MESSAGE_1)
		ON_INST(BCODE_SEND_MESSAGE_2)
		ON_INST(BCODE_SEND_MESSAGE_3)
		ON_INST(BCODE_SEND_MESSAGE_TO_SUPER_0)
		ON_INST(BCODE_SEND_MESSAGE_TO_SUPER_1)
		ON_INST(BCODE_SEND_MESSAGE_TO_SUPER_2)
		ON_INST(BCODE_SEND_MESSAGE_TO_SUPER_3)
		{
			moo_oop_char_t selector;

			b1 = bcode & 0x3; /* low 2 bits */
			b2 = FETCH_BYTE_CODE(moo);

		handle_send_message:
			/* get the selector from the literal frame */
			selector = (moo_oop_char_t)moo->active_method->slot[b2];

			LOG_INST3 (moo, "send_message%hs %zu @%zu", (((bcode >> 2) & 1)? "_to_super": ""), b1, b2);
			if (send_message (moo, selector, ((bcode >> 2) & 1), b1) <= -1) return -1;
			NEXT_INST();
		}

		/* -------------------------------------------------------- */

		ON_INST(BCODE_PUSH_RECEIVER)
			LOG_INST0 (moo, "push_receiver");
			MOO_STACK_PUSH (moo, moo->active_context->origin->receiver_or_source);
			NEXT_INST();

		ON_INST(BCODE_PUSH_NIL)
			LOG_INST0 (moo, "push_nil");
			MOO_STACK_PUSH (moo, moo->_nil);
			NEXT_INST();

		ON_INST(BCODE_PUSH_TRUE)
			LOG_INST0 (moo, "push_true");
			MOO_STACK_PUSH (moo, moo->_true);
			NEXT_INST();

		ON_INST(BCODE_PUSH_FALSE)
			LOG_INST0 (moo, "push_false");
			MOO_STACK_PUSH (moo, moo->_false);
			NEXT_INST();

		ON_INST(BCODE_PUSH_CONTEXT)
			LOG_INST0 (moo, "push_context");
			MOO_STACK_PUSH (moo, (moo_oop_t)moo->active_context);
			NEXT_INST();

		ON_INST(BCODE_PUSH_PROCESS)
			LOG_INST0 (moo, "push_process");
			MOO_STACK_PUSH (moo, (moo_oop_t)moo->processor->active);
			NEXT_INST();

		ON_INST(BCODE_PUSH_RECEIVER_NS)
		{
			register moo_oop_t c;
			LOG_INST0 (moo, "push_receiver_ns");
			c = (moo_oop_t)MOO_CLASSOF(moo, moo->active_context->origin->receiver_or_source);
			if (c == (moo_oop_t)moo->_class) c = moo->active_context->origin->receiver_or_source;
			MOO_STACK_PUSH (moo, (moo_oop_t)((moo_oop_class_t)c)->nsup);
			NEXT_INST();
		}

		ON_INST(BCODE_PUSH_NEGONE)
			LOG_INST0 (moo, "push_negone");
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(-1));
			NEXT_INST();

		ON_INST(BCODE_PUSH_ZERO)
			LOG_INST0 (moo, "push_zero");
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(0));
			NEXT_INST();

		ON_INST(BCODE_PUSH_ONE)
			LOG_INST0 (moo, "push_one");
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(1));
			NEXT_INST();

		ON_INST(BCODE_PUSH_TWO)
			LOG_INST0 (moo, "push_two");
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(2));
			NEXT_INST();

		ON_INST(BCODE_PUSH_INTLIT)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "push_intlit %zu", b1);
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(b1));
			NEXT_INST();

		ON_INST(BCODE_PUSH_NEGINTLIT)
		{
			moo_ooi_t num;
			FETCH_PARAM_CODE_TO (moo, b1);
			num = b1;
			LOG_INST1 (moo, "push_negintlit %zu", b1);
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(-num));
			NEXT_INST();
		}

		ON_INST(BCODE_PUSH_CHARLIT)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "push_charlit %zu", b1);
			MOO_STACK_PUSH (moo, MOO_CHAR_TO_OOP(b1));
			NEXT_INST();
		/* -------------------------------------------------------- */

		ON_INST(BCODE_MAKE_DICTIONARY)
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "make_dictionary %zu", b1);

			/* Dictionary new: b1 
			 *  doing this allows users to redefine Dictionary whatever way they like.
			 *  if i did the followings instead, the internal of Dictionary would get
			 *  tied to the system dictionary implementation. the system dictionary 
			 *  implementation is flawed in that it accepts only a variable character
			 *  object as a key. it's better to invoke 'Dictionary new: ...'.
			t = (moo_oop_t)moo_makedic (moo, moo->_dictionary, b1 + 10);
			MOO_STACK_PUSH (moo, t);
			 */
			MOO_STACK_PUSH (moo, (moo_oop_t)moo->_dictionary);
			MOO_STACK_PUSH (moo, MOO_SMOOI_TO_OOP(b1));
			if (send_message (moo, moo->dicnewsym, 0, 1) <= -1) return -1;
			NEXT_INST();

		ON_INST(BCODE_POP_INTO_DICTIONARY)
			LOG_INST0 (moo, "pop_into_dictionary");

			/* dic __put_assoc:  assoc
			 *  whether the system dictinoary implementation is flawed or not,
			 *  the code would look like this if it were used.
			t1 = MOO_STACK_GETTOP(moo);
			MOO_STACK_POP (moo);
			t2 = MOO_STACK_GETTOP(moo);
			moo_putatdic (moo, (moo_oop_dic_t)t2, ((moo_oop_association_t)t1)->key, ((moo_oop_association_t)t1)->value);
			 */
			if (send_message (moo, moo->dicputassocsym, 0, 1) <= -1) return -1;
			NEXT_INST();

		ON_INST(BCODE_MAKE_ARRAY)
		{
			moo_oop_t t;

			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "make_array %zu", b1);

			/* create an empty array */
			t = moo_instantiate (moo, moo->_array, MOO_NULL, b1);
			if (!t) return -1;

			MOO_STACK_PUSH (moo, t); /* push the array created */
			NEXT_INST();
		}

		ON_INST(BCODE_POP_INTO_ARRAY)
		{
			moo_oop_t t1, t2;
			FETCH_PARAM_CODE_TO (moo, b1);
			LOG_INST1 (moo, "pop_into_array %zu", b1);
			t1 = MOO_STACK_GETTOP(moo);
			MOO_STACK_POP (moo);
			t2 = MOO_STACK_GETTOP(moo);
			((moo_oop_oop_t)t2)->slot[b1] = t1;
			NEXT_INST();
		}

		ON_INST(BCODE_DUP_STACKTOP)
		{
			moo_oop_t t;
			LOG_INST0 (moo, "dup_stacktop");
			MOO_ASSERT (moo, !MOO_STACK_ISEMPTY(moo));
			t = MOO_STACK_GETTOP(moo);
			MOO_STACK_PUSH (moo, t);
			NEXT_INST();
		}

		ON_INST(BCODE_POP_STACKTOP)
			LOG_INST0 (moo, "pop_stacktop");
			MOO_ASSERT (moo, !MOO_STACK_ISEMPTY(moo));
			MOO_STACK_POP (moo);
			NEXT_INST();

		ON_INST(BCODE_RETURN_STACKTOP)
			LOG_INST0 (moo, "return_stacktop");
			return_value = MOO_STACK_GETTOP(moo);
			MOO_STACK_POP (moo);
			goto handle_return;

		ON_INST(BCODE_RETURN_RECEIVER)
			LOG_INST0 (moo, "return_receiver");
			return_value = moo->active_context->origin->receiver_or_source;
		handle_return:
			{
				int n;
				if ((n = do_return (moo, bcode, return_value)) <= -1) return -1;
				if (n == 0) EXIT_DISPATCH_LOOP();
			}
			NEXT_INST();

		ON_INST(BCODE_LOCAL_RETURN)
			LOG_INST0 (moo, "local_return");
			return_value = MOO_STACK_GETTOP(moo);
			MOO_STACK_POP (moo);
			goto handle_return;

		ON_INST(BCODE_RETURN_FROM_BLOCK)
			do_return_from_block (moo);
			NEXT_INST();

		ON_INST(BCODE_MAKE_BLOCK)
			if (make_block(moo) <= -1) return -1;
			NEXT_INST();

		ON_INST(BCODE_SEND_BLOCK_COPY)
		{
			moo_ooi_t nargs, ntmprs;
			moo_oop_context_t rctx;
			moo_oop_context_t blkctx;

			LOG_INST0 (moo, "send_block_copy");

			/* it emulates thisContext blockCopy: nargs ofTmprCount: ntmprs */
			MOO_ASSERT (moo, moo->sp >= 2);

			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(MOO_STACK_GETTOP(moo)));
			ntmprs = MOO_OOP_TO_SMOOI(MOO_STACK_GETTOP(moo));
			MOO_STACK_POP (moo);

			MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(MOO_STACK_GETTOP(moo)));
			nargs = MOO_OOP_TO_SMOOI(MOO_STACK_GETTOP(moo));
			MOO_STACK_POP (moo);

			MOO_ASSERT (moo, nargs >= 0);
			MOO_ASSERT (moo, ntmprs >= nargs);

			/* the block context object created here is used
			 * as a base object for block context activation.
			 * pf_block_value() clones a block 
			 * context and activates the cloned context.
			 * this base block context is created with no 
			 * stack for this reason. */
			blkctx = (moo_oop_context_t)moo_instantiate (moo, moo->_block_context, MOO_NULL, 0); 
			if (!blkctx) return -1;

			/* get the receiver to the block copy message after block context instantiation
			 * not to get affected by potential GC */
			rctx = (moo_oop_context_t)MOO_STACK_GETTOP(moo);
			MOO_ASSERT (moo, rctx == moo->active_context);

			/* [NOTE]
			 *  blkctx->sender is left to nil. it is set to the 
			 *  active context before it gets activated. see
			 *  pf_block_value().
			 *
			 *  blkctx->home is set here to the active context.
			 *  it's redundant to have them pushed to the stack
			 *  though it is to emulate the message sending of
			 *  blockCopy:withNtmprs:. BCODE_MAKE_BLOCK has been
			 *  added to replace BCODE_SEND_BLOCK_COPY and pusing
			 *  arguments to the stack.
			 *
			 *  blkctx->origin is set here by copying the origin
			 *  of the active context.
			 */

			/* the extended jump instruction has the format of 
			 *   0000XXXX KKKKKKKK or 0000XXXX KKKKKKKK KKKKKKKK 
			 * depending on MOO_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
			 * the instruction after the jump. */
			blkctx->ip = MOO_SMOOI_TO_OOP(moo->ip + MOO_BCODE_LONG_PARAM_SIZE + 1);
			blkctx->sp = MOO_SMOOI_TO_OOP(-1);
			/* the number of arguments for a block context is local to the block */
			blkctx->method_or_nargs = MOO_SMOOI_TO_OOP(nargs);
			/* the number of temporaries here is an accumulated count including
			 * the number of temporaries of a home context */
			blkctx->ntmprs = MOO_SMOOI_TO_OOP(ntmprs);

			blkctx->home = (moo_oop_t)rctx;
			blkctx->receiver_or_source = moo->_nil;

			/* [NOTE]
			 * the origin of a method context is set to itself
			 * when it's created. so it's safe to simply copy
			 * the origin field this way. 
			 *
			 * if the context that receives the blockCopy message 
			 * is a method context, the following conditions are all true.
			 *   rctx->home == moo->_nil
			 *   MOO_CLASSOF(moo, rctx) == moo->_method_context
			 *   rctx == (moo_oop_t)moo->active_context
			 *   rctx == rctx->origin
			 *
			 * if it is a block context, the following condition is true.
			 *   MOO_CLASSOF(moo, rctx) == moo->_block_context
			 */
			blkctx->origin = rctx->origin;

			MOO_STACK_SETTOP (moo, (moo_oop_t)blkctx);
			NEXT_INST();
		}

		ON_INST(BCODE_NOOP)
			/* do nothing */
			LOG_INST0 (moo, "noop");
			NEXT_INST();

		ON_UNKNOWN_INST()
			MOO_LOG1 (moo, MOO_LOG_IC | MOO_LOG_FATAL, "Fatal error - unknown byte code 0x%zx\n", bcode);
			moo_seterrnum (moo, MOO_EINTERN);
			return -1;

		END_DISPATCH_TABLE ()
		/* ==== END OF DISPATCH TABLE ==== */

	END_DISPATCH_LOOP()

	return 0;
}


int moo_execute (moo_t* moo)
{
	int n;
	int log_default_type_mask;

	log_default_type_mask = moo->log.default_type_mask;
	moo->log.default_type_mask |= MOO_LOG_VM;

#if defined(MOO_PROFILE_VM)
	moo->inst_counter = 0;
#endif

	if (vm_startup(moo) <= -1) return -1;

	moo->proc_switched = 0;
	moo->abort_req = 0;

	n = __execute (moo);

	vm_cleanup (moo);

#if defined(MOO_PROFILE_VM)
	MOO_LOG1 (moo, MOO_LOG_IC | MOO_LOG_INFO, "TOTAL_INST_COUTNER = %zu\n", moo->inst_counter);
#endif

	moo->log.default_type_mask = log_default_type_mask;
	return n;
}

void moo_abort (moo_t* moo)
{
	moo->abort_req = 1;
}

int moo_invoke (moo_t* moo, const moo_oocs_t* objname, const moo_oocs_t* mthname)
{
	int n;

	MOO_ASSERT (moo, moo->initial_context == MOO_NULL);
	MOO_ASSERT (moo, moo->active_context == MOO_NULL);
	MOO_ASSERT (moo, moo->active_method == MOO_NULL);

	if (start_initial_process_and_context (moo, objname, mthname) <= -1) return -1;
	moo->initial_context = moo->processor->active->initial_context;

	n = moo_execute (moo);

/* TODO: reset processor fields. set processor->tally to zero. processor->active to nil_process... */
	moo->initial_context = MOO_NULL;
	moo->active_context = MOO_NULL;
	moo->active_method = MOO_NULL;
	return n;
}

#if 0
int moo_invoke (moo_t* moo, const moo_oocs_t* objname, const moo_oocs_t* mthname)
{
/* TODO: .... */
	/* call 
	 * 	System initializeClasses
	 * and invoke 
	 *   objname mthname
	 */
}
#endif
