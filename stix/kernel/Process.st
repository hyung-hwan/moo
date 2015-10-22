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
	#dcl initial active state prev next.

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
		^Processor resume: self.
	}

}

#class ProcessScheduler(Object)
{
	#dcl tally head tail active.

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

	#method suspendActive
	{
		" TODO: .........."
	}

	"
	#method yield
	{
		<primitive: #_processor_yield>
		self primitiveFailed
	}
	"
}
