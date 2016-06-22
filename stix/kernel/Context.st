#class Exception(Apex)
{
	#dcl signalContext handlerContext messageText.

	## To be extended below.
}

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

	#method hasEnsureBlock
	{
		^false
	}

	#method ensureBlock
	{
		^nil
	}

	#method findExceptionHandlerContext
	{
		| ctx |
		ctx := self.
		[ ctx isExceptionHandlerContext ifTrue: [^ctx].
		  ctx := ctx sender.
		  ctx isNil ] whileFalse.
		^nil
	}

	#method findExceptionHandlerBlock
	{
		^nil 
	}

	#method handleException: exception
	{
		| excblk retval posact |

		## position of the temporary variable 'active' in MethodContext>>on:do.
		## for this code to work, it must be the last temporary variable in the method.
		posact := (self basicSize) - 1. 

		excblk := self findExceptionHandlerBlock: (exception class).
		(excblk isNil or: [(self basicAt: posact) not]) ifTrue: [ 
			^self.sender findExceptionHandlerContext handleException: exception
		].

		exception handlerContext: self.

		self basicAt: posact put: false.
		[ retval := excblk value: exception ] ensure: [
			self basicAt: posact put: true
		].

		##self return: retval.
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

	#method hasEnsureBlock
	{
		## 10 - STIX_METHOD_PREAMBLE_ENSURE in VM.
		^self.method preambleCode == 11
	}

	#method ensureBlock
	{
## TODO: 9 is the number of named instance variables of a context.
## TODO: change 9 to a constant when stix is enhanced to support constant definition
		
		(self.method preambleCode == 11) ifFalse: [^nil].
		^self basicAt: 8.
	}

	#method findExceptionHandlerBlock: anExceptionClass
	{
		## find an exception handler block for a given exception class.
		##
		## for this to work, self must be an exception handler context.
		## For a single on:do: call,
		##   self class specNumInstVars must return 8.
		##   basicAt: 8 must be the on: argument.
		##   basicAt: 9 must be the do: argument

		(self isExceptionHandlerContext) ifTrue: [
			| bound exc |
			## NOTE: if on:do: has a temporary varible, bound must be adjusted to reflect it.
			bound := self basicSize - 1.
## TODO: change 8 to a constant when stix is enhanced to support constant definition
##       or calcuate the minimum size using the class information.
			8 to: bound by: 2 do: [ :i | 
				exc := self basicAt: i.
				((anExceptionClass == exc) or: [anExceptionClass inheritsFrom: exc]) ifTrue: [^self basicAt: (i + 1)].
			]
		].
		^nil.
	}

	#method unwindTo: aContext return: anObject
	{
		## private: called by VM upon unwinding
		| ctx eb |
		ctx := self.
		[ctx ~~ aContext] whileTrue: [
			eb := ctx ensureBlock.
			(eb notNil) ifTrue: [eb value].
			ctx := ctx sender.
		].
		eb := ctx ensureBlock.
		(eb notNil) ifTrue: [eb value].

		^anObject
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
		## If VM is built with STIX_USE_PROCSTK
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
		thisContext goto: pc.
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
		ip := source pc.
	}


	#method on: anException do: anExceptionBlock
	{
		| exception_active |
		<exception>

"thisContext isExceptionHandlerContext dump.
(thisContext basicSize) dump.
(thisContext basicAt: 8) dump.  ## this should be anException
(thisContext basicAt: 9) dump.  ## this should be anExceptionBlock
(thisContext basicAt: 10) dump.  ## this should be handlerActive
'on:do: ABOUT TO EVALUE THE RECEIVER BLOCK' dump."

		exception_active := true.
		^self value.
	}

	#method on: exc1 do: blk1 on: exc2 do: blk2
	{
		| active |
		<exception>
		active := true.
		^self value.
	}

	#method ensure: aBlock
	{
		| v |
		<ensure>
		v := self value.
		aBlock value.
		^v
	}

	#method ifCurtailed: aBlock
	{
		| v ok |

		ok := false.
		[ v := self value. ok := true. ] ensure: [ ok ifFalse: [aBlock value] ].
		^v.
	}

}

##
## TODO: is it better to inherit from Object???
##       or treat Exception specially like UndefinedObject or Class???
##
#extend Exception
{
	#method(#class) signal
	{
		self new signal
	}

	#method(#class) signal: text
	{
		self new signal: text
	}

	#method handlerContext: context
	{
		self.handlerContext := context.
	}

	#method messageText
	{
		^self.messageText
	}

	#method signal 
	{
		self.signalContext := thisContext.
		((thisContext sender) findExceptionHandlerContext) handleException: self.
	}

	#method signal: text
	{
		self.messageText := text.
		self signal.
	}

	#method pass
	{
		## pass the exception to the outer context
		((self.handlerContext sender) findExceptionHandlerContext) handleException: self.
	}

"
TODO: implement these methods....
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
"

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
'------ END OF BACKTRACE -----------' dump.
		PrimitiveFailureException signal: 'PRIMITIVE FAILED'.
	}

	#method(#class) cannotInstantiate
	{
		Exception signal: 'Cannot instantiate'.
	}

	#method(#class) doesNotUnderstand: messageSymbol
	{
		## TODO: implement this properly
		NoSuchMessageException signal: (messageSymbol, ' not understood by ', (self name)).
	}
}
