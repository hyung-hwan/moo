
class(#pointer,#final,#limited,#uncopyable) Process(Object)
{
	var(#get) initialContext, currentContext, id, state.
	var sp.
	var(#get) ps_prev, ps_next, sem_wait_prev, sem_wait_next.
	var sem, perr, perrmsg.

	var asyncsg.

	method primError { ^self.perr }
	method primErrorMessage { ^self.perrmsg }
	method(#primitive) sp.

	method(#primitive) resume.
	method(#primitive) yield.
	method(#primitive) suspend.
	method(#primitive) _terminate.

	method terminate
	{
		// search from the top context of the process down to intial_context and find ensure blocks and execute them.
		// if a different process calls 'terminate' on a process,
		// the ensureblock is not executed in the context of the
		// process being terminated, but in the context of terminatig process.
		//
		// 1) process termianted by another process
		//   p := [ 
		//       [  1 to: 10000 by: 1 do: [:ex | System logNl: i asString] ] ensure: [System logNl: 'ensured....'] 
		//   ] newProcess.
		//   p resume.
		//   p terminate.
		//
		// 2) process terminated by itself
		//   p := [ 
		//       [ thisProcess terminate. ] ensure: [System logNl: 'ensured....'] 
		//   ] newProcess.
		//   p resume.
		//   p terminate.
		// ----------------------------------------------------------------------------------------------------------
		// the process must be frozen first. while unwinding is performed, 
		// the process must not be scheduled.
		// ----------------------------------------------------------------------------------------------------------

		// if the current active process(thisProcess) is not the process to terminate(self),
		// suspend the target process so that no scheduling wakes the process until this 
		// termination is completed.
		//if (Processor activeProcess ~~ self) { self suspend }.
		if (thisProcess ~~ self) { self suspend }.

		// TODO: what if there is another process that may resume this suspended process.
		//       should i mark it unresumable?

		self.currentContext unwindTo: self.initialContext return: nil.
		^self _terminate
	}

	method initAsync
	{
		if (self.asyncsg isNil) { self.asyncsg := SemaphoreGroup new }.
	}
	
	method addAsyncSemaphore: sem
	{
		^self.asyncsg addSemaphore: sem
	}

	method removeAsyncSemaphore: sem
	{
		^self.asyncsg removeSemaphore: sem
	}

	method handleAsyncEvent
	{
		^self.asyncsg wait.
	}
}

class(#uncopyable) Semaphore(Object)
{
	var waiting_head  := nil,
	    waiting_tail  := nil.
	
	var count         := 0.    // semaphore signal count

	var subtype       := nil. // nil, io, timed

	var heapIndex     := nil, // overlaps as ioIndex
	    fireTimeSec   := nil, // overlaps as ioHandle
	    fireTimeNsec  := nil. // overlaps as ioType

	var(#get,#set) signalAction := nil.

	var(#get,#set) _group := nil, 
	               _grm_next := nil,
	               _grm_prev := nil.

	// ==================================================================

	method(#primitive) signal.
	method(#primitive) _wait.

	method wait
	{
		| k |
		k := self _wait.
		if (self.signalAction notNil) { self.signalAction value: self }.
		^k
	}

	// ==================================================================

	method(#primitive) signalAfterSecs: secs.
	method(#primitive) signalAfterSecs: secs nanosecs: nanosecs.
	method(#primitive) signalOnInput: io_handle.
	method(#primitive) signalOnOutput: io_handle.
	method(#primitive) signalOnGCFin.
	method(#primitive) unsignal.

	// ==================================================================

	method heapIndex: index
	{
		self.heapIndex :=  index.
	}
	method headpIndex
	{
		^self.heapIndex.
	}

	// ------------------------------------------
	// TODO: either put fireTimeNsec into implementation of fireTime, and related methods.
	// ------------------------------------------
	method fireTime
	{
		^self.fireTimeSec
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

class(#uncopyable) Mutex(Semaphore)
{
	method(#class) new
	{
		| s |
		s := super new.
		s signal.
		^s.
	}

/*
TODO: how to prohibit wait and signal???
	method(#prohibited) wait.
	method(#prohibited) signal.
*/

	method lock  { ^super wait }
	method unlock { ^super signal }

	method critical: block
	{
		self wait.
		^block ensure: [ self signal ]
	}
}
 

class(#uncopyable) SemaphoreGroup(Object)
{
	// the first two variables must match those of Semaphore.
	var waiting_head  := nil,
	    waiting_tail  := nil.
	    
	var first_sem := nil,
	    last_sem := nil,
	    first_sigsem := nil,
	    last_sigsem := nil,
	    sem_io_count := 0,
	    sem_count := 0.

/* TODO: good idea to a shortcut way to prohibit a certain method in the heirarchy chain?
method(#class,#prohibited) new.
method(#class,#prohibited) new: size.
method(#class,#abstract) xxx. => method(#class) xxx { self subclassResponsibility: #xxxx }
*/

/*
	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }
*/

	method(#primitive) addSemaphore: sem.
	method(#primitive) removeSemaphore: sem.
	method(#primitive) _wait.

	method wait
	{
		| x |
		x := self _wait.
		if (x notError)
		{
			// TODO: is it better to check if x is an instance of Semaphore/SemaphoreGroup?
			if (x signalAction notNil) { x signalAction value: x }.
		}.
		^x
	}

	method waitWithTimeout: seconds
	{
		| s r |

		// create an internal semaphore for timeout notification.
		s := Semaphore new. 
		self addSemaphore: s.

		[ 
			// arrange the processor to notify upon timeout.
			s signalAfterSecs: seconds.

			// wait on the semaphore group.
			r := self wait. 

			// if the internal semaphore has been signaled, 
			// arrange to return nil to indicate timeout.
			if (r == s) { r := nil } // timed out
			elif (r signalAction notNil) { r signalAction value: r }. // run the signal action block
		] ensure: [
			// System<<unsignal: doesn't thrown an exception even if the semaphore s is not
			// register with System<<signal:afterXXX:. otherwise, i would do like this line
			// commented out.
			// [ s unsignal ] ensure: [ self removeSemaphore: s ].

			s unsignal.
			self removeSemaphore: s
		].

		^r.
	} 
}

class(#uncopyable) SemaphoreHeap(Object)
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
		| index newarr newsize |

		index := self.size.
		if (index >= (self.arr size)) 
		{
			newsize := (self.arr size) * 2.
			newarr := Array new: newsize.
			newarr copy: self.arr.
			self.arr := newarr.
		}.

		self.arr at: index put: aSemaphore.
		aSemaphore heapIndex: index.
		self.size := self.size + 1.

		^self siftUp: index
	}

	method popTop
	{
		| top |

		top := self.arr at: 0.
		self removeAt: 0.
		^top
	}

	method updateAt: anIndex with: aSemaphore
	{
		| item |

		item := self.arr at: anIndex.
		item heapIndex: nil.

		self.arr at: anIndex put: aSemaphore.
		aSemaphore heapIndex: anIndex.

		^if (aSemaphore youngerThan: item) { self siftUp: anIndex } else { self siftDown: anIndex }.
	}

	method removeAt: anIndex
	{
		| item xitem |

		item := self.arr at: anIndex.
		item heapIndex: nil.

		self.size := self.size - 1.
		if (anIndex == self.size) 
		{
			// the last item
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
		^(anIndex - 1) div: 2
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

			// item is younger than the parent. 
			// move the parent down
			self.arr at: cindex put: par.
			par heapIndex: cindex.
		}.

		// place the item as high as it can
		self.arr at: cindex put: item.
		item heapIndex: cindex.

		^cindex
	}

	method siftDown: anIndex
	{
		| base capa cindex item 
		  left right younger xitem |

		base := self.size div: 2.
		if (anIndex >= base) { ^anIndex }.

		cindex := anIndex.
		item := self.arr at: cindex.

		while (cindex < base) 
		{
			left := self leftChildIndex: cindex.
			right := self rightChildIndex: cindex.

			younger := if ((right < self.size) and ((self.arr at: right) youngerThan: (self.arr at: left))) { right } else { left }.

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

class(#final,#limited,#uncopyable) ProcessScheduler(Object)
{
	var(#get) active, total_count := 0.
	var(#get) runnable_count := 0.
	var runnable_head, runnable_tail.
	var(#get) suspended_count := 0.
	var suspended_head, suspended_tail.

	method activeProcess { ^self.active }
	method resume: proc { ^proc resume }
}
