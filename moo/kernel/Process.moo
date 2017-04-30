
class(#pointer,#limited) Process(Object)
{
	var initial_context, current_context, state, sp, prev, next, sem, perr.

	method(#class) basicNew
	{
		(* instantiation is not allowed. a process is strictly a VM managed object *)
		self cannotInstantiate
	}
	
	method(#class) basicNew: size 
	{
		(* instantiation is not allowed. a process is strictly a VM managed object *)
		self cannotInstantiate
	}

	method prev { ^self.prev }
	method next { ^self.next }

	method next: process { self.next := process }
	method prev: process { self.prev := process }

	method primError { ^self.perr }

	method resume
	{
		<primitive: #_process_resume>
		self primitiveFailed

		##^Processor resume: self.
	}

	method _terminate
	{
		<primitive: #_process_terminate>
		self primitiveFailed
	}

	method _suspend
	{
		<primitive: #_process_suspend>
		self primitiveFailed
	}

	method terminate
	{
##search from the top contextof the process down to intial_contextand find ensure blocks and execute them.
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

		##(Processor activeProcess ~~ self) ifTrue: [ self _suspend ].
		(thisProcess ~~ self) ifTrue: [ self _suspend ].
		self.current_context unwindTo: self.initial_context return: nil.
		^self _terminate
	}

	method yield
	{
		<primitive: #_process_yield>
		self primitiveFailed
	}

	method sp
	{
		^self.sp.
	}

	method initialContext
	{
		^self.initial_context
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
	    ioData        := nil,
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

		^(aSemaphore youngerThan: item)
			ifTrue: [ self siftUp: anIndex ]
			ifFalse: [ self siftDown: anIndex ].
	}

	method deleteAt: anIndex
	{
		| item |

		item := self.arr at: anIndex.
		item heapIndex: -1.

		self.size := self.size - 1.
		(anIndex == self.size) 
			ifTrue: [
				"the last item"
				self.arr at: self.size put: nil.
			]
			ifFalse: [
				| xitem |

				xitem := self.arr at: self.size.
				self.arr at: anIndex put: xitem.
				xitem heapIndex: anIndex.
				self.arr at: self.size put: nil.

				(xitem youngerThan: item) 
					ifTrue: [self siftUp: anIndex ]
					ifFalse: [self siftDown: anIndex ]
			]
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
		| pindex cindex par item stop |

		(anIndex <= 0) ifTrue: [ ^anIndex ].

		pindex := anIndex.
		item := self.arr at: anIndex.

		stop := false.
		[ stop ] whileFalse: [

			cindex := pindex.

			(cindex > 0)
				ifTrue: [
					pindex := self parentIndex: cindex.
					par := self.arr at: pindex.

					(item youngerThan: par) 
						ifTrue: [
							## move the parent down
							self.arr at: cindex put: par.
							par heapIndex: cindex.
						]
						ifFalse: [ stop := true ].
				]
				ifFalse: [ stop := true ].
		].

		self.arr at: cindex put: item.
		item heapIndex: cindex.

		^cindex
	}

	method siftDown: anIndex
	{
		| base capa cindex item |

		base := self.size quo: 2.
		(anIndex >= base) ifTrue: [^anIndex].

		cindex := anIndex.
		item := self.arr at: cindex.

		[ cindex < base ] whileTrue: [
			| left right younger xitem |

			left := self leftChildIndex: cindex.
			right := self rightChildIndex: cindex.

			((right < self.size) and: [(self.arr at: right) youngerThan: (self.arr at: left)])
				ifTrue: [ younger := right ]
				ifFalse: [ younger := left ].

			xitem := self.arr at: younger.
			(item youngerThan: xitem) 
				ifTrue: [
					"break the loop"
					base := anIndex 
				]
				ifFalse: [
					self.arr at: cindex put: xitem.
					xitem heapIndex: cindex.
					cindex := younger.
				]
		].

		self.arr at: cindex put: item.
		item heapIndex: cindex.

		^cindex
	}
}

class(#limited) ProcessScheduler(Object)
{
	var tally, active, runnable_head, runnable_tail, sem_heap.

	method new
	{
		"instantiation is not allowed"
		^nil. "TODO: raise an exception"
	}

	method activeProcess
	{
		^self.active.
	}

	method resume: process
	{
		<primitive: #_processor_schedule>
		self primitiveFailed.

		"The primitive does something like the following in principle:
		(self.tally = 0)
			ifTrue: [
				self.head := process.
				self.tail := process.
				self.tally := 1.
			]
			ifFalse: [
				process next: self.head.
				self.head prev: process.
				self.head := process.
				self.tally := self.tally + 1.
			].
		"
	}

	"
	method yield
	{
		<primitive: #_processor_yield>
		self primitiveFailed
	}
	"

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
