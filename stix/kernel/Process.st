

#class Delay(Object)
{
## TODO: support milliseconds or nanoseconds
	#dcl delay.

	#method(#class) forSeconds: anInteger
	{
		^super basicNew initWith: anInteger.
	}

	#method initWith: anInteger
	{
		self.delay := anInteger.
	}

	#method wait
	{
		Processor sleep: self.delay.
	}

	#method resume
	{
		" TODO: .............. "
	}
}

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
	#dcl count waiting_head waiting_tail.

	#method initialize
	{
		count := 0.
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
}

#class ProcessScheduler(Object)
{
	#dcl tally active runnable_head runnable_tail.

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
}
