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
		^Scheduler resume: self.
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

	#method add: aProcess
	{
		<primitive: #_scheduler_add>
		self primitiveFailed.

		"The primitive does something like the following.

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
			]."
	}

	#method resume: aProcess
	{
		<primitive: #_scheduler_add>
		self primitiveFailed.

		"self add: aProcess.
		 TODO: need to change state of a Process??? 
		self.active := aProcess."
	}

	#method remove: aProcess
	{
		"<primitive: #_scheduler_remove>"

"TODO: "
	}

	#method suspendActive
	{
		" TODO: .........."
	}

	"
	#method yield
	{
		<primitive: #processYield>
		self primitiveFailed
	}

	#method enter: aContext
	{
		<primitive: #processEnter>
		self primitiveFailed
	}
	"
}
