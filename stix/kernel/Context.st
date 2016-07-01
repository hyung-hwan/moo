#class(#pointer) Context(Apex)
{
	#dcl sender ip sp ntmprs.

	#method sender
	{
		^self.sender
	}

	#method isDead
	{
		^self.ip < 0
	}
}

#class(#pointer) MethodContext(Context)
{
	#dcl method receiver home origin.

	#method pc
	{
		^self.ip
	}

	#method pcplus1
	{
		^self.ip + 1
	}

	#method goto: anInteger
	{
		<primitive: #_context_goto>
		self primitiveFailed. ## TODO: need to make this a hard failure?
	}

	#method pc: anInteger
	{
		self.ip := anInteger.
	}

	#method sp
	{
		^self.sp.

	}
	#method sp: new_sp
	{
		self.sp := new_sp
	}

	#method pc: new_pc sp: new_sp
	{
		self.ip := new_pc.
		self.sp := new_sp.
	}

	#method method
	{
		^self.method
	}
}

#class(#pointer) BlockContext(Context)
{
	#dcl nargs source home origin.

	#method fork 
	{
		"crate a new process in the runnable state"
		^self newProcess resume.
	}

	#method newProcess
	{
		"create a new process in the suspended state"
		<primitive: #_block_new_process>
		self primitiveFailed.
	}

	#method newProcessWith: anArray
	{
		"create a new process in the suspended state passing the elements
		 of anArray as block arguments"
		<primitive: #_block_new_process>
		self primitiveFailed.
	}

	#method value
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}
	#method value: a 
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}
	#method value: a value: b
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}
	#method value: a value: b value: c
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}
	#method value: a value: b value: c value: d
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}
	#method value: a value: b value: c value: d value: e
	{
		<primitive: #_block_value>
		self primitiveFailed.
	}

	#method ifTrue: aBlock
	{
		^(self value) ifTrue: aBlock.
	}

	#method ifFalse: aBlock
	{
		^(self value) ifFalse: aBlock.
	}

	#method ifTrue: trueBlock ifFalse: falseBlock
	{
		^(self value) ifTrue: trueBlock ifFalse: falseBlock
	}

	#method whileTrue: aBlock
	{
		## --------------------------------------------------
		## Naive implementation
		## --------------------------------------------------
		## (self value) ifFalse: [^nil].
		## aBlock value. 
		## self whileTrue: aBlock.
		## --------------------------------------------------

		## --------------------------------------------------
		## Non-recursive implementation
		## --------------------------------------------------
		| pc sp |

		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil ].
		aBlock value.

		## the pc: method leaves thisContext and pc in the stack after
		## having changes the instruction poointer.
		## as a result, the stack keeps growing. the goto method
		## clears thisContext and pc off the stack unlike normal methods.
		##thisContext pc: pc.
		thisContext goto: pc.
		## --------------------------------------------------
	}

	#method whileTrue
	{
		## (self value) ifFalse: [^nil].
		## self whileTrue.

		## --------------------------------------------------
		## Non-recursive implementation
		## --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil ].
		thisContext goto: pc.
		## --------------------------------------------------
	}

	#method whileFalse: aBlock
	{
		## --------------------------------------------------
		## Naive implementation
		## --------------------------------------------------
		## (self value) ifTrue: [^nil].
		## aBlock value. 
		## self whileFalse: aBlock.
		## --------------------------------------------------

		## --------------------------------------------------
		## Non-recursive implementation
		## --------------------------------------------------
		##  The assignment to 'pc' uses the POP_INTO_TEMPVAR_1.
		##  It pops a value off the stack top, stores it to the second
		##  temporary variable(aBlock is the first temporary variable).
		##  It is a single byte instruction. 'pc' returned by 
		##  'thisContext pcplus1'' should point to the instruction after
		##  the POP_INTO_TEMPVAR_0 instruction.
		##
		##  It would need the increment of 2 if the pair of 
		##  STORE_INTO_TEMPVAR_1 and POP_STACKTOP were used. 
		##  This implementation is subject to the instructions chosen
		##  by the compiler.
		##
		| pc |
		pc := thisContext pcplus1.
		(self value) ifTrue: [ ^nil "^self" ].
		aBlock value.
		thisContext goto: pc.
		## --------------------------------------------------
	}

	#method whileFalse
	{
		## (self value) ifTrue: [^nil].
		## self whileFalse.

		## --------------------------------------------------
		## Non-recursive implementation
		## --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifTrue: [ ^nil "^self" ].
		thisContext goto: pc.
		## --------------------------------------------------
	}

	#method pc
	{
		^self.ip
	}

	#method pc: anInteger
	{
		self.ip := anInteger.
	}
	
	#method sp
	{
		^self.sp
	}

	#method sp: anInteger
	{
		self.sp := anInteger.
	}

	#method restart
	{
		ip := self.source pc.
	}
}
