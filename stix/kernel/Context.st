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

	#method isExceptionContext
	{
		^false
	}

	#method isEnsureContext
	{
		^false
	}

	#method ensureBlock
	{
		^nil
	}

	#method findExceptionContext
	{
		| ctx |
		ctx := self.
		[ ctx notNil ] whileTrue: [
			(ctx isExceptionContext) ifTrue: [^ctx].
			ctx := ctx sender.
		].
		^nil
	}

	#method findEnsureContextAbove: sentinel
	{
		| ctx |
		ctx := self.
		[ ctx notNil and: [ctx ~~ sentinel] ] whileTrue: [
			(ctx isEnsureContext) ifTrue: [^ctx].
			ctx := ctx sender.
		].
		^nil
	}


	#method hasSender: context
	{
		| s | 
		(self == context) ifTrue: [^false].
		s := self.sender.
		[ s isNil ] whileFalse: [
			(s == context) ifTrue: [^true].
			s := s sender.
		].
		^false
	}

	#method isDead
	{
		^self.ip < 0
	}

	#method terminate
	{
		self.sender := nil.
		self.ip = -1;
	}

	#method terminateTo: prevctx
	{
		| ctx sctx |
		(self hasSender: prevctx) ifTrue: [
			ctx := self.sender.
			[ctx ~~ prevctx] whileTrue: [
				sctx := ctx sender.
				ctx terminate.
				ctx := sctx.
			]
		].
		self.sender := prevctx.
	}

	#method resume: retval through: context
	{
		| ctx | 
		self isDead ifTrue: ["TODO: cannot return" 'CANNOT RETURN' dump.].

		ctx := context.
		[ ctx notNil ] whileTrue: [
			| donepos |
			donepos := ctx basicSize - 1.

			(ctx basicAt: donepos) ifFalse: [
				| ensblk |
				ctx basicAt: donepos put: true.
				ensblk := ctx ensureBlock.
				thisContext terminateTo: ctx.
				ensblk value.
			].

			ctx := ctx findEnsureContextAbove: self.
		].

		thisContext terminateTo: self.
		^retval
	}

	#method unwindTo: context return: retval
	{
		## -------------------------------------------------------------------
		## <<private>>
		## private: called by VM upon unwinding
		## -------------------------------------------------------------------

		| ctx stop |
		ctx := self.
		stop := false.
		[stop] whileFalse: [
			| eb |
			eb := ctx ensureBlock.
			(eb notNil) ifTrue: [
				| donepos |
				donepos := ctx basicSize - 1.
				(ctx basicAt: donepos) ifFalse: [
					ctx basicAt: donepos put: true.
					eb value.
				].
			].
			stop := (ctx == context).
			ctx := ctx sender.
		].

		^retval
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

	#method isExceptionContext
	{
		## 10 - STIX_METHOD_PREAMBLE_EXCEPTION in VM.
		^self.method preambleCode == 10.
	}

	#method isEnsureContext
	{
		## 10 - STIX_METHOD_PREAMBLE_ENSURE in VM.
		^self.method preambleCode == 11
	}

	#method ensureBlock
	{
## TODO: change 8 to a constant when stix is enhanced to support constant definition
		(self.method preambleCode == 11) ifFalse: [^nil].
		^self basicAt: 8.
	}


	#method findExceptionHandlerFor: exception_class
	{
		## find an exception handler block for a given exception class.
		##
		## for this to work, self must be an exception handler context.
		## For a single on:do: call,
		##   self class specNumInstVars must return 8.
		##   basicAt: 8 must be the on: argument.
		##   basicAt: 9 must be the do: argument

		(self isExceptionContext) ifTrue: [
			| bound exc |
			## NOTE: if on:do: has a temporary varible, bound must be adjusted to reflect it.
			bound := self basicSize - 1.
## TODO: change 8 to a constant when stix is enhanced to support constant definition
##       or calcuate the minimum size using the class information.
			8 to: bound by: 2 do: [ :i | 
				exc := self basicAt: i.
				((exception_class == exc) or: [exception_class inheritsFrom: exc]) ifTrue: [^self basicAt: (i + 1)].
			]
		].
		^nil.
	}

	#method handleException: exception
	{
		## -------------------------------------------------------------------
		## <<private>>
		##   called by Exception>>signal.
		##   this method only exists in the MethodContext and UndefinedObject.
		##   the caller must make sure that the receiver object is
		##   a method context or nil. Exception>>signal invokes this method
		##   only for an exception context which is a method context. it 
		##   invokes it for nil when no exception context is found.
		## -------------------------------------------------------------------

		| excblk retval actpos |

		## position of the temporary variable 'active' in MethodContext>>on:do.
		## for this code to work, it must be the last temporary variable in the method.
		actpos := (self basicSize) - 1. 

		excblk := self findExceptionHandlerFor: (exception class).
		(excblk isNil or: [(self basicAt: actpos) not]) ifTrue: [ 
			## self is an exception context but doesn't have a matching
			## exception handler or the exception context is already
			## in the middle of evaluation. 
			^(self.sender findExceptionContext) handleException: exception.
		].

		exception handlerContext: self.

		## -------------------------------------------------------------------
		## if an exception occurs within an exception handler block,
		## the search will reach this context again as the exception block
		## is evaluated before actual unwinding. set the temporary variable
		## in the exception context to mask out this context from the search
		## list.
		## -------------------------------------------------------------------
		self basicAt: actpos put: false.
		[ retval := excblk value: exception ] ensure: [
			self basicAt: actpos put: true
		].

		##(self.sender isNil) ifTrue: [ "TODO: CANNOT RETURN" ].

		## -------------------------------------------------------------------
		## return to self.sender which is a caller of the exception context (on:do:)
		## pass the first ensure context between thisContext and self.sender.
		##
		## [ [Exception signal: 'xxx'] ensure: [20] ] on: Exception do: [:ex | ...]
		##
		## -------------------------------------------------------------------
		##^self.sender resume: retval through: (thisContext findEnsureContextAbove: self.sender).
		thisContext unwindTo: self.sender return: nil.
		Processor return: retval to: self.sender.
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

"thisContext isExceptionContext dump.
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
		| exception_active |
		<exception>
		exception_active := true.
		^self value.
	}

	#method ensure: aBlock
	{
		| retval done |
		<ensure>

		done := false.
		retval := self value. 

		## the temporary variable 'done' may get changed
		## during evaluation for exception handling. 
		done ifFalse: [
			done := true.
			aBlock value.
		].
		^retval
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
		^self new signal
	}

	#method(#class) signal: text
	{
		^self new signal: text
	}

	#method handlerContext: context
	{
		self.handlerContext := context.
	}

	#method messageText
	{
		^self.messageText
	}

	#method __signal 
	{
		self.signalContext := thisContext.
		((thisContext sender) findExceptionContext) handleException: self.
	}

	#method signal
	{
		| excctx excblk retval actpos |

		self.signalContext := thisContext.
		excctx := (thisContext sender) findExceptionContext.
		[excctx notNil] whileTrue: [
			excblk := excctx findExceptionHandlerFor: (self class).
			(excblk notNil and: 
			 [actpos := excctx basicSize - 1. excctx basicAt: actpos]) ifTrue: [
				self.handlerContext := excctx.
				excctx basicAt: actpos put: false.
				[ retval := excblk value: self ] ensure: [ 
					excctx basicAt: actpos put: true 
				].

				thisContext unwindTo: (excctx sender) return: nil.
				Processor return: retval to: (excctx sender).
			].
			excctx := (excctx sender) findExceptionContext.
		].

		## -----------------------------------------------------------------
		## FATAL ERROR - no exception handler.
		## -----------------------------------------------------------------
		##thisContext unwindTo: nil return: nil.
		thisContext unwindTo: (Processor activeProcess initialContext) return: nil.
		('### EXCEPTION NOT HANDLED #### ', self class name, ' - ', self messageText) dump.
		## TODO: debug the current process???? "
		Processor activeProcess terminate.
	}

	#method signal: text
	{
		self.messageText := text.
		^self signal.
	}

	#method pass
	{
		## pass the exception to the outer context
		((self.handlerContext sender) findExceptionContext) handleException: self.
	}

	#method return: value
	{
		(self.handlerContext notNil) ifTrue: [
			Processor return: value to: self.handlerContext.
		].
	}

	#method retry
	{
## TODO: verify if return:to: causes unnecessary stack growth.
		(self.handlerContext notNil)  ifTrue: [
			self.handlerContext pc: 0.
			Processor return: self to: self.handlerContext.
		].
	}

	#method resume: value
	{
## TODO: verify if return:to: causes unnecessary stack growth.
## is this correct???
		(self.signalContext notNil and: [self.handlerContext notNil]) ifTrue: [
			| ctx |
			ctx := self.signalContext sender.
			self.signalContext := nil.
			self.handlerContext := nil.
			Processor return: value to: ctx.
		].
	}

	#method resume
	{
		^self resume: nil.
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
