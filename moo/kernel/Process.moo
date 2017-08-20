
class(#pointer,#final,#limited) Process(Object)
{
	var(#get) initialContext, currentContext, id, state.
	var(#get) sp, ps_prev, ps_next, sem_wait_prev, sem_wait_next.
	var sem, perr, perrmsg.

	method primError { ^self.perr }
	method primErrorMessage { ^self.perrmsg }

	method(#primitive) resume.
	method(#primitive) yield.
	method(#primitive) _terminate.
	method(#primitive) _suspend.

	method terminate
	{
		## search from the top context of the process down to intial_context and find ensure blocks and execute them.
		## if a different process calls 'terminate' on a process,
		## the ensureblock is not executed in the context of the
		## process being terminated, but in the context of terminatig process.
		##
		## 1) process termianted by another process
		##   p := [ 
		##       [  1 to: 10000 by: 1 do: [:ex | System logNl: i asString] ] ensure: [System logNl: 'ensured....'] 
		##   ] newProcess.
		##   p resume.
		##   p terminate.
		##
		## 2) process terminated by itself
		##   p := [ 
		##       [ thisProcess terminate. ] ensure: [System logNl: 'ensured....'] 
		##   ] newProcess.
		##   p resume.
		##   p terminate.
		## ----------------------------------------------------------------------------------------------------------
		## the process must be frozen first. while unwinding is performed, 
		## the process must not be scheduled.
		## ----------------------------------------------------------------------------------------------------------

		##if (Processor activeProcess ~~ self) { self _suspend }.
		if (thisProcess ~~ self) { self _suspend }.
		self.currentContext unwindTo: self.initialContext return: nil.
		^self _terminate
	}
}

class Semaphore(Object)
{
	var count         :=   0, 
	    waiting_head  := nil,
	    waiting_tail  := nil,
	    heapIndex     :=  -1,
	    fireTimeSec   :=   0,
	    fireTimeNsec  :=   0,
	    ioIndex       :=  -1,
	    ioHandle      := nil,
	    ioMask        :=   0.

	method(#class) forMutualExclusion
	{
		| sem |
		sem := self new.
		sem signal.
		^sem
	}

	## ==================================================================

	method(#primitive) signal.
	method(#primitive) wait.

(*
	method waitWithTimeout: seconds
	{
		<primitive: #_semaphore_wait>
		self primitiveFailed
	}

	method waitWithTimeout: seconds and: nanoSeconds
	{
		<primitive: #_semaphore_wait>
		self primitiveFailed
	}
*)
	method critical: aBlock
	{
		self wait.
		^aBlock ensure: [ self signal ]
	}

	## ==================================================================

	method heapIndex
	{
		^heapIndex
	}

	method heapIndex: anIndex
	{
		heapIndex := anIndex
	}

	method fireTime
	{
		^fireTimeSec
	}

	method fireTime: anInteger
	{
		self.fireTimeSec := anInteger.
	}

	method youngerThan: aSemaphore
	{
		^self.fireTimeSec < (aSemaphore fireTime)
	}

	method notYoungerThan: aSemaphore
	{
		^self.fireTimeSec >= (aSemaphore fireTime)
	}
}

class SemaphoreHeap(Object)
{
	var arr, size.

	method initialize
	{
		self.size := 0.
		self.arr := Array new: 100.
	}

	method size
	{
		^self.size
	}

	method at: anIndex
	{
		^self.arr at: anIndex.
	}

	method insert: aSemaphore
	{
		| index |

		index := self.size.
		(index >= (self.arr size)) ifTrue: [
			| newarr newsize |
			newsize := (self.arr size) * 2.
			newarr := Array new: newsize.
			newarr copy: self.arr.
			self.arr := newarr.
		].

		self.arr at: index put: aSemaphore.
		aSemaphore heapIndex: index.
		self.size := self.size + 1.

		^self siftUp: index
	}

	method popTop
	{
		| top |

		top := self.arr at: 0.
		self deleteAt: 0.
		^top
	}

	method updateAt: anIndex with: aSemaphore
	{
		| item |

		item := self.arr at: anIndex.
		item heapIndex: -1.

		self.arr at: anIndex put: aSemaphore.
		aSemaphore heapIndex: anIndex.

		^if (aSemaphore youngerThan: item) { self siftUp: anIndex } else { self siftDown: anIndex }.
	}

	method deleteAt: anIndex
	{
		| item xitem |

		item := self.arr at: anIndex.
		item heapIndex: -1.

		self.size := self.size - 1.
		if (anIndex == self.size) 
		{
			## the last item
			self.arr at: self.size put: nil.
		}
		else
		{
			xitem := self.arr at: self.size.
			self.arr at: anIndex put: xitem.
			xitem heapIndex: anIndex.
			self.arr at: self.size put: nil.
			if (xitem youngerThan: item) { self siftUp: anIndex } else { self siftDown: anIndex }.
		}
	}

	method parentIndex: anIndex
	{
		^(anIndex - 1) quo: 2
	}

	method leftChildIndex: anIndex
	{
		^(anIndex * 2) + 1.
	}

	method rightChildIndex: anIndex
	{
		 ^(anIndex * 2) + 2.
	}

	method siftUp: anIndex
	{
		| pindex cindex par item |

		if (anIndex <= 0) { ^anIndex }.

		pindex := anIndex.
		item := self.arr at: anIndex.

		while (true)
		{
			cindex := pindex.

			if (pindex <= 0) { break }.

			pindex := self parentIndex: cindex.
			par := self.arr at: pindex.

			if (item notYoungerThan: par)  { break }.

			## item is younger than the parent. 
			## move the parent down
			self.arr at: cindex put: par.
			par heapIndex: cindex.
		}.

		## place the item as high as it can
		self.arr at: cindex put: item.
		item heapIndex: cindex.

		^cindex
	}

	method siftDown: anIndex
	{
		| base capa cindex item 
		  left right younger xitem |

		base := self.size quo: 2.
		if (anIndex >= base) { ^anIndex }.

		cindex := anIndex.
		item := self.arr at: cindex.

		while (cindex < base) 
		{
			left := self leftChildIndex: cindex.
			right := self rightChildIndex: cindex.

			younger := if ((right < self.size) and: [(self.arr at: right) youngerThan: (self.arr at: left)]) { right } else { left }.

			xitem := self.arr at: younger.
			if (item youngerThan: xitem)  { break }.

			self.arr at: cindex put: xitem.
			xitem heapIndex: cindex.
			cindex := younger.
		}.

		self.arr at: cindex put: item.
		item heapIndex: cindex.

		^cindex
	}
}

class(#final,#limited) ProcessScheduler(Object)
{
	var(#get) active, should_exit := false, total_count := 0.
	var(#get) runnable_count := 0.
	var runnable_head, runnable_tail.
	var(#get) suspended_count := 0.
	var suspended_head, suspended_tail.

	method activeProcess
	{
		^self.active.
	}

	method resume: process
	{
		<primitive: #_processor_schedule>
		self primitiveFailed.

		(* The primitive does something like the following in principle:
		(self.tally == 0)
			ifTrue: [
				self.head := process.
				self.tail := process.
				self.tally := 1.
			]
			ifFalse: [
				process ps_next: self.head.
				self.head ps_prev: process.
				self.head := process.
				self.tally := self.tally + 1.
			].
		*)
	}

	(* -------------------
	method yield
	{
		<primitive: #_processor_yield>
		self primitiveFailed
	}
	----------------- *)

	method signal: semaphore after: secs
	{
		<primitive: #_processor_add_timed_semaphore>
		self primitiveFailed.
	}

	method signal: semaphore after: secs and: nanosecs
	{
		<primitive: #_processor_add_timed_semaphore>
		self primitiveFailed.
	}

	method unsignal: semaphore
	{
		<primitive: #_processor_remove_semaphore>
		self primitiveFailed.
	}

	method signalOnGCFin: semaphore
	{
		<primitive: #_processor_add_gcfin_semaphore>
		self primitiveFailed.
	}
	
	method signal: semaphore onInput: file
	{
		<primitive: #_processor_add_input_semaphore>
		self primitiveFailed.
	}
	method signal: semaphore onOutput: file
	{
		<primitive: #_processor_add_output_semaphore>
		self primitiveFailed.
	}
	method signal: semaphore onInOutput: file
	{
		<primitive: #_processor_add_inoutput_semaphore>
		self primitiveFailed.
	}

	method return: object to: context
	{
		<primitive: #_processor_return_to>
		self primitiveFailed.
	}

	method sleepFor: secs
	{
		## -----------------------------------------------------
		## put the calling process to sleep for given seconds.
		## -----------------------------------------------------
		| s |
		s := Semaphore new.
		self signal: s after: secs.
		s wait.
	}

	method sleepFor: secs and: nanosecs
	{
		## -----------------------------------------------------
		## put the calling process to sleep for given seconds.
		## -----------------------------------------------------
		| s |
		s := Semaphore new.
		self signal: s after: secs and: nanosecs 
		s wait.
	}
}
