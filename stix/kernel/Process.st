
#class(#pointer) Process(Object)
{
	#dcl initial_context current_context state sp prev next sem.

	#method new
	{
		"instantiation is not allowed"
		^nil. "TODO: raise an exception"
	}

	#method prev
	{
		^self.prev.
	}

	#method next
	{
		^self.next.
	}

	#method next: aProcess
	{
		self.next := aProcess.
	}

	#method prev: aProcess
	{
		self.prev := aProcess.
	}

	#method resume
	{
		<primitive: #_process_resume>
		self primitiveFailed

		##^Processor resume: self.
	}

	#method terminate
	{
		<primitive: #_process_terminate>
		self primitiveFailed
	}

	#method yield
	{
		<primitive: #_process_yield>
		self primitiveFailed
	}

	#method sp
	{
		^sp.
	}

	#method sleep: seconds
	{
		| s |
		s := Semaphore new.
		Processor signal: s after: seconds.
		## Processor activeProcess dump.
		s wait.
	}
}

#class Semaphore(Object)
{
	#dcl count waiting_head waiting_tail heapIndex fireTimeSec fireTimeNsec.

	#method(#class) forMutualExclusion
	{
		| sem |
		sem := self new.
		sem signal.
		^sem
	}

	#method initialize
	{
		self.count := 0.
		self.heapIndex := -1.
		self.fireTimeSec := 0.
		self.fireTimeNsec := 0.
	}

	## ==================================================================

	#method signal 
	{
		<primitive: #_semaphore_signal>
		self primitiveFailed.
	}

	#method wait
	{
		<primitive: #_semaphore_wait>
		self primitiveFailed.
	}

	#method waitWithTimeout: seconds
	{
		<primitive: #_semaphore_wait>
		self primitiveFailed
	}

	#method waitWithTimeout: seconds and: nanoSeconds
	{
		<primitive: #_semaphore_wait>
		self primitiveFailed
	}

	#method critical: aBlock
	{
		self wait.
		^aBlock ensure: [ self signal ]
	}

	## ==================================================================

	#method heapIndex
	{
		^heapIndex
	}

	#method heapIndex: anIndex
	{
		heapIndex := anIndex
	}

	#method fireTime
	{
		^fireTimeSec
	}

	#method fireTime: anInteger
	{
		self.fireTimeSec := anInteger.
	}

	#method youngerThan: aSemaphore
	{
		^self.fireTimeSec < (aSemaphore fireTime)
	}
}

#class SemaphoreHeap(Object)
{
	#dcl arr size.

	#method initialize
	{
		self.size := 0.
		self.arr := Array new: 100.
	}

	#method size
	{
		^self.size
	}

	#method at: anIndex
	{
		^self.arr at: anIndex.
	}

	#method insert: aSemaphore
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

	#method popTop
	{
		| top |

		top := self.arr at: 0.
		self deleteAt: 0.
		^top
	}

	#method updateAt: anIndex with: aSemaphore
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

	#method deleteAt: anIndex
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

	#method parentIndex: anIndex
	{
		^(anIndex - 1) quo: 2
	}

	#method leftChildIndex: anIndex
	{
		^(anIndex * 2) + 1.
	}

	#method rightChildIndex: anIndex
	{
		 ^(anIndex * 2) + 2.
	}

	#method siftUp: anIndex
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

	#method siftDown: anIndex
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

#class ProcessScheduler(Object)
{
	#dcl tally active runnable_head runnable_tail sem_heap.

	#method new
	{
		"instantiation is not allowed"
		^nil. "TODO: raise an exception"
	}

	#method activeProcess
	{
		^self.active.
	}

	#method resume: aProcess
	{
		<primitive: #_processor_schedule>
		self primitiveFailed.

		"The primitive does something like the following in principle:
		(self.tally = 0)
			ifTrue: [
				self.head := aProcess.
				self.tail := aProcess.
				self.tally := 1.
			]
			ifFalse: [
				aProcess next: self.head.
				self.head prev: aProcess.
				self.head := aProcess.
				self.tally := self.tally + 1.
			].
		"
	}

	"
	#method yield
	{
		<primitive: #_processor_yield>
		self primitiveFailed
	}
	"

	#method signal: aSemaphore after: anInteger
	{
		<primitive: #_processor_add_timed_semaphore>
		self primitiveFailed.
	}

	#method signal: aSemaphore after: aSecond and: aNanoSecond
	{
		<primitive: #_processor_add_timed_semaphore>
		self primitiveFailed.
	}

	#method unsignal: aSemaphore
	{
		<primitive: #_processor_remove_semaphore>
		self primitiveFailed.
	}

	"#method signal: aSemaphore onInput: file
	{
	}"
	"#method signal: aSemaphore onOutput: file
	{
	}"

	#method return: anObject to: aContext
	{
		<primitive: #_processor_return_to>
		self primitiveFailed.
	}

	#method returnTo: aContext andEval: aBlock with: arg
	{
		<primitive: #_processor_return_to_and_eval>
		self primitiveFailed.
	}

	#method forceContext: aContext
	{
		<primitive: #_processor_force_context>
		self primitiveFailed.
	}

}
