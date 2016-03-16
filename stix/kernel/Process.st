
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
}

#class Semaphore(Object)
{
	#dcl count waiting_head waiting_tail heapIndex fireTime.

	#method initialize
	{
		self.count := 0.
		self.heapIndex := 0.
		self.fireTime := 0.
	}

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
		^fireTime
	}

	#method fireTime: anInteger
	{
		self.fireTime := anInteger.
	}

	#method youngerThan: aSemaphore
	{
		^self.fireTime < (aSemaphore fireTime)
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

	#method insert: aSemaphore
	{
		self.size >= (self.arr size) ifTrue: [
			| newarr newsize |
			newsize := (self.arr size) * 2.
			newarr := Array new: newsize.
			newarr copy: self.arr.
			self.arr := newarr.
		].

		self.size := self.size + 1.
		self.arr at: self.size put: aSemaphore.
		aSemaphore heapIndex: self.size.

		^self siftUp: self.size.
	}

	#method popTop
	{
		| top |

		top := self.arr at: 1.
		self deleteAt: 1.
		^top
	}

	#method updateAt: anIndex
	{
	}

	#method deleteAt: anIndex
	{
		| item |

		item := self.arr at: anIndex.
		item heapIndex: -1.

		(anIndex == self.size) 
			ifTrue: [
				"the last item"
				self.arr at: self.size put: nil.
				self.size := self.size - 1
			]
			ifFalse: [
				| xitem |

				xitem := self.arr at: self.size.
				self.arr at: anIndex put: xitem.
				xitem heapIndex: anIndex.
				self.arr at: self.size put: nil.
				self.size := self.size - 1.

				(xitem youngerThan: item) 
					ifTrue: [self siftUp: anIndex ]
					ifFalse: [self siftDown: anIndex ]
			]
	}

	#method parentIndex: anIndex
	{
		## ^(anIndex - 1) quo: 2
		^anIndex quo: 2
	}

	#method leftChildIndex: anIndex
	{
		## ^(anIndex * 2) + 1.
		^(anIndex * 2).
	}

	#method rightChildIndex: anIndex
	{
		## ^(anIndex * 2) + 2.
		^(anIndex * 2) + 1.
	}

	#method siftUp: anIndex
	{
		| pindex cindex par cur |

		anIndex <= 1 ifTrue: [ ^anIndex ].

		cindex := anIndex.
		pindex := self parentIndex: anIndex.

		par := self.arr at: pindex.
		cur := self.arr at: cindex.

		[ cur youngerThan: par ] whileTrue: [

			cindex := pindex.
			pindex := self parentIndex: pindex.

			par := self.arr at: pindex.
			cur := self.arr at: cindex.
		].

		self.arr at: cindex put: cur.
		cur heapIndex: cindex.

		^cindex
	}

	#method siftDown: anIndex
	{
		| base capa cindex item |


		base := self.size quo: 2.
		(anIndex > base) ifTrue: [^anIndex].

		cindex := anIndex.
		item := self.arr at: cindex.

		[ cindex < base ] whileTrue: [
			| left right younger xitem |

			left := self leftChildIndex: cindex.
			right := self rightChildIndex: cindex.

			((right <= self.size) and: [(self.arr at: right) youngerThan: (self.arr at: left)])
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

	#method sleep: anInteger
	{
		<primitive: #_processor_sleep>
		self primitiveFailed.
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

	#method remove: aProcess
	{
		"<primitive: #_processor_remove>"
"TODO: "
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
		self.sem_heap add: aSemaphore withTimeout: anInteger.
	}
}
