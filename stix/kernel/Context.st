#class(#pointer) Context(Apex)
{
	#dcl sender ip sp ntmprs.

	#method sender
	{
		^self.sender
	}

	#method isExceptionHandlerContext
	{
		^false
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

	#method pc: anInteger
	{
		self.ip := anInteger.
		"self.sp := self.sp - 1."  "whould this always work??? "
	}

	#method sp
	{
		^self.sp.

	}
	#method sp: anInteger
	{
		self.sp := anInteger.
	}

	#method pc: aPC sp: aSP
	{
		self.ip := aPC.
		self.sp := aSP.
		##sp := sp - 1.
	}

	#method method
	{
		^self.method
	}

	#method isExceptionHandlerContext
	{
		## 10 - STIX_METHOD_PREAMBLE_EXCEPTION in VM.
		^self.method preambleCode == 10. 
	}

	#method findExceptionHandlerBlock: anExceptionClass
	{
		## for this to work, self must be an exception handler context.
		## For a single on:do: call,
		##   self class specNumInstVars must return 8.
		##   basicAt: 8 must be the on: argument.
		##   basicAt: 9 must be the do: argument

		(self isExceptionHandlerContext) ifTrue: [
			| bound exc |
			## NOTE: if on:do: has a temporary varible, bound must be adjusted to reflect it.
			bound := self basicSize - 1.
			8 to: bound by: 2 do: [ :i | 
				exc := self basicAt: i.
				((anExceptionClass == exc) or: [anExceptionClass inheritsFrom: exc]) ifTrue: [^self basicAt: (i + 1)].
			]
		].
		^nil.
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
		## If VM is built without STIX_USE_PROCSTK
		## --------------------------------------------------
		## | pc sp |
		## sp := thisContext sp.
		## pc := thisContext pcplus1.
		## self value ifFalse: [ ^nil "^self" ].
		## aBlock value.
		## thisContext pc: pc sp: sp.
		## --------------------------------------------------

		## --------------------------------------------------
		## If VM is built with STIX_USE_PROCSTK
		## --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil "^self" ].
		aBlock value.
		thisContext pc: pc.
		## --------------------------------------------------
	}

	#method whileTrue
	{
		## (self value) ifFalse: [^nil].
		## self whileTrue.

		## --------------------------------------------------
		## If VM is built with STIX_USE_PROCSTK
		## --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil "^self" ].
		thisContext pc: pc.
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
		## If VM is built without STIX_USE_PROCSTK
		## --------------------------------------------------
		## | pc sp |
		## sp := thisContext sp.
		## pc := thisContext pcplus1.
		## self value ifTrue: [ ^nil "^self" ].
		## aBlock value.
		## thisContext pc: pc sp: sp.
		## --------------------------------------------------

		## --------------------------------------------------
		## If VM is built with STIX_USE_PROCSTK
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
		thisContext pc: pc.
		## --------------------------------------------------
	}

	#method whileFalse
	{
		## (self value) ifTrue: [^nil].
		## self whileFalse.

		## --------------------------------------------------
		## If VM is built with STIX_USE_PROCSTK
		## --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifTrue: [ ^nil "^self" ].
		thisContext pc: pc.
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
		ip := source pc.
	}

	"------ TODO: -------------------------------------"
	#method on: anException do: anExceptionBlock
	{
"		| handlerActive |"
		<exception>
"		handlerActive := true.

thisContext isExceptionHandlerContext dump.
(thisContext basicSize) dump.
(thisContext basicAt: 8) dump.  ## this should be anException
(thisContext basicAt: 9) dump.  ## this should be anExceptionBlock
(thisContext basicAt: 10) dump.  ## this should be handlerActive
'on:do: ABOUT TO EVALUE THE RECEIVER BLOCK' dump."

		^self value.
	}

	#method on: exc1 do: blk1 on: exc2 do: blk2
	{
		<exception>
		^self value.
	}

	#method ensure: aBlock
	{
		"##
		| complete returnValue |
		<ensure>

		returnValue := self valueNoContextSwitch.
		complete ifNil: [
			complete := true.
			aBlock value.
		].
		^returnValue. ##"
	}

	#method ifCurtailed: aBlock
	{
	}

	"------ TODO: -------------------------------------"
}

## TODO: is it better to inherit from Object???
##       or treat Exception specially like UndefinedObject or Class???
#class Exception(Apex)
{
	#dcl signalContext handlerContext handlerBlock messageText.

	#method(#class) signal
	{
		self new signal
	}

	#method(#class) signal: text
	{
		self new signal: text
	}

	#method messageText
	{
		^self.messageText
	}

	#method signal 
	{
		self.signalContext := thisContext.
		self findHandlerContextStartingFrom: self.signalContext.
		self handleException.
	}

	#method signal: text
	{
		self.messageText := text.
		self signal.
	}

	#method pass
	{
		## pass the exception to the outer context
		self.signalContext notNil 
			ifTrue: [ ## it's signaled earlier
				## TODO: Should i change the signalContex to thisContext???
				self findHandlerContextStartingFrom: (self.handlerContext sender).
				self handleException.
			]
	}

	#method return: value
	{
		self.handlerContext notNil ifTrue: [
			Processor return: value to: (self.handlerContext sender)
		]
	}

	#method retry
	{
		## TODO: verify if return:to: causes unnecessary stack growth.
		self.handlerContext notNil 
			ifTrue: [
				## TODO: should i reset self.handlerContext and self.signalContext to nil? 
				self.handlerContext pc: 0.
				Processor return: self to: self.handlerContext.
				##Processor forceContext: self.handlerContext.
			]
	}

	#method resume
	{
		## TODO: verify if return:to: causes unnecessary stack growth.
		## is this correct???
		self.signalContext notNil
			ifTrue: [
				## TODO: should i reset self.handlerContext and self.signalContext to nil? 
				Processor return: self to: (self.signalContext sender).
			]
	}

	## ####################################################################
	## ####################################################################
	#method handleException
	{
		self.handlerContext notNil
			ifTrue: [ 
				Processor return: (self.handlerBlock value: self) to: (self.handlerContext sender)
			]
			ifFalse: [ 
				('### EXCEPTION NOT HANDLED #### ', self class name, ' - ', self messageText) dump.
				## TODO: debug the current process???? "
				Processor activeProcess terminate.
			].
	}

	#method handlerContext
	{
		(self.handlerContext notNil) ifTrue: [ ^self.handlerContext ].
		self.handlerContext := self findHandlerContextStartingFrom: self.signalContext.
		^self.handlerContext.
	}

	#method findHandlerContextStartingFrom: aContext
	{
		## Find exception handling context starting from a given context
		| ctx |

		ctx := aContext.
		[ ctx notNil ] whileTrue: [
				##(ctx handles: self) ifTrue: [ ^ ctx ].
				(ctx isExceptionHandlerContext) ifTrue: [
					| blk |
					blk := ctx findExceptionHandlerBlock: (self class).
					(blk notNil) ifTrue: [ 
						self.handlerBlock := blk. 
						self.handlerContext := ctx. 
						^ctx 
					].
				].
				ctx := ctx sender
			].
		^nil
	}
}

#class NoSuchMessageException(Exception)
{
}

#class PrimitiveFailureException(Exception)
{
}

#extend Apex
{
	#method(#class) primitiveFailed
	{
		## TODO: implement this
## experimental backtrace...
| ctx |
ctx := thisContext.
[ctx notNil] whileTrue: [
	(ctx class == MethodContext) 
		ifTrue: [ (ctx method owner name, '>>', ctx method name) dump ].
	## TODO: include blockcontext???
	ctx := ctx sender.
].
		PrimitiveFailureException signal: 'PRIMITIVE FAILED'.
	}

	#method(#class) cannotInstantiate
	{
		Exception signal: 'Cannot instantiate'.
	}
}
