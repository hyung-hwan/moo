

##
## TODO: is it better to inherit from Object???
##       or treat Exception specially like UndefinedObject or Class???
##
class Exception(Apex)
{
	dcl signalContext handlerContext messageText.

	method(#class) signal
	{
		^self new signal
	}

	method(#class) signal: text
	{
		^self new signal: text
	}

	method handlerContext: context
	{
		self.handlerContext := context.
	}

	method messageText
	{
		^self.messageText
	}

	method asString
	{
		^(self class name) & ' - ' & self.messageText.
	}

	method __signal 
	{
		self.signalContext := thisContext.
		((thisContext sender) findExceptionContext) handleException: self.
	}

	method signal
	{
		| exctx exblk retval actpos |

		self.signalContext := thisContext.
		exctx := (thisContext sender) findExceptionContext.
		[exctx notNil] whileTrue: [
			exblk := exctx findExceptionHandlerFor: (self class).
			(exblk notNil and: 
			 [actpos := exctx basicSize - 1. exctx basicAt: actpos]) ifTrue: [
				self.handlerContext := exctx.
				exctx basicAt: actpos put: false.
				[ retval := exblk value: self ] ensure: [ 
					exctx basicAt: actpos put: true 
				].

				thisContext unwindTo: (exctx sender) return: nil.
				Processor return: retval to: (exctx sender).
			].
			exctx := (exctx sender) findExceptionContext.
		].

		## -----------------------------------------------------------------
		## FATAL ERROR - no exception handler.
		## -----------------------------------------------------------------
		##thisContext unwindTo: nil return: nil.
		##thisContext unwindTo: (Processor activeProcess initialContext) return: nil.
		thisContext unwindTo: (thisProcess initialContext) return: nil.
		('### EXCEPTION NOT HANDLED #### ' & self class name & ' - ' & self messageText) dump.
		## TODO: debug the current process???? "

		##Processor activeProcess terminate.
		thisProcess terminate.
	}

	method signal: text
	{
		self.messageText := text.
		^self signal.
	}

	method pass
	{
		## pass the exception to the outer context
		((self.handlerContext sender) findExceptionContext) handleException: self.
	}

	method return: value
	{
		(self.handlerContext notNil) ifTrue: [
			Processor return: value to: self.handlerContext.
		].
	}

	method retry
	{
## TODO: verify if return:to: causes unnecessary stack growth.
		(self.handlerContext notNil)  ifTrue: [
			self.handlerContext pc: 0.
			Processor return: self to: self.handlerContext.
		].
	}

	method resume: value
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

	method resume
	{
		^self resume: nil.
	}
}

##============================================================================
extend Context
{
	method isExceptionContext
	{
		^false
	}

	method isEnsureContext
	{
		^false
	}

	method ensureBlock
	{
		^nil
	}

	method findExceptionContext
	{
		| ctx |
		ctx := self.
		[ ctx notNil ] whileTrue: [
			(ctx isExceptionContext) ifTrue: [^ctx].
			ctx := ctx sender.
		].
		^nil
	}

	method unwindTo: context return: retval
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

			## stop ifFalse: [ stop := ctx isNil ].
		].

		^retval
	}
}

##============================================================================
extend MethodContext
{
	method isExceptionContext
	{
		## 10 - MOO_METHOD_PREAMBLE_EXCEPTION in VM.
		^self.method preambleCode == 10.
	}

	method isEnsureContext
	{
		## 10 - MOO_METHOD_PREAMBLE_ENSURE in VM.
		^self.method preambleCode == 11
	}

	method ensureBlock
	{
## TODO: change 8 to a constant when moo is enhanced to support constant definition

		(* [ value-block ] ensure: [ ensure-block ]
		 * assuming ensure block is a parameter the ensure: method to a 
		 * block context, the first parameter is placed after the fixed
		 * instance variables of the method context. As MethodContex has
		 * 8 instance variables, the ensure block must be at the 9th position
		 * which translates to index 8 *)

		(self.method preambleCode == 11) ifFalse: [^nil].
		^self basicAt: 8.
	}


	method findExceptionHandlerFor: exception_class
	{
		(* find an exception handler block for a given exception class.
		 *
		 * for this to work, self must be an exception handler context.
		 * For a single on:do: call,
		 *   self class specNumInstVars must return 8.(i.e.MethodContext has 8 instance variables.)
		 *   basicAt: 8 must be the on: argument.
		 *   basicAt: 9 must be the do: argument  *)

## TODO: change 8 to a constant when moo is enhanced to support constant definition
##       or calcuate the minimum size using the class information.

		(self isExceptionContext) ifTrue: [
			| size exc |

			(* NOTE: the following loop scans all parameters to the on:do: method.
			 *       if the on:do: method contains local temporary variables,
			 *       those must be skipped from scanning. *)

			size := self basicSize.
			8 priorTo: size by: 2 do: [ :i | 
				exc := self basicAt: i.
				((exception_class == exc) or: [exception_class inheritsFrom: exc]) ifTrue: [^self basicAt: (i + 1)].
			]
		].
		^nil.
	}

	method handleException: exception
	{
		(* -----------------------------------------------------------------
		 * <<private>>
		 *   called by Exception>>signal.
		 *   this method only exists in the MethodContext and UndefinedObject.
		 *   the caller must make sure that the receiver object is
		 *   a method context or nil. Exception>>signal invokes this method
		 *   only for an exception context which is a method context. it 
		 *   invokes it for nil when no exception context is found.
		 * ---------------------------------------------------------------- *)

		| excblk retval actpos |

		(* position of the temporary variable 'active' in MethodContext>>on:do.
		 * for this code to work, it must be the last temporary variable in the method. *)
		actpos := (self basicSize) - 1. 

		excblk := self findExceptionHandlerFor: (exception class).
		(excblk isNil or: [(self basicAt: actpos) not]) ifTrue: [ 
			## self is an exception context but doesn't have a matching
			## exception handler or the exception context is already
			## in the middle of evaluation. 
			^(self.sender findExceptionContext) handleException: exception.
		].

		exception handlerContext: self.

		(* -----------------------------------------------------------------
		 * if an exception occurs within an exception handler block,
		 * the search will reach this context again as the exception block
		 * is evaluated before actual unwinding. set the temporary variable
		 * in the exception context to mask out this context from the search
		 * list.
		 * ---------------------------------------------------------------- *)
		self basicAt: actpos put: false.
		[ retval := excblk value: exception ] ensure: [
			self basicAt: actpos put: true
		].

		##(self.sender isNil) ifTrue: [ "TODO: CANNOT RETURN" ].

		(* -----------------------------------------------------------------
		 * return to self.sender which is a caller of the exception context (on:do:)
		 * pass the first ensure context between thisContext and self.sender.
		 * [ [Exception signal: 'xxx'] ensure: [20] ] on: Exception do: [:ex | ...]
		 * ---------------------------------------------------------------- *)
		thisContext unwindTo: self.sender return: nil.
		Processor return: retval to: self.sender.
	}
}

##============================================================================
extend BlockContext
{
	method on: anException do: anExceptionBlock
	{
		| exception_active |
		<exception>

(* ------------------------------- 
thisContext isExceptionContext dump.
(thisContext basicSize) dump.
(thisContext basicAt: 8) dump.  ## this should be anException
(thisContext basicAt: 9) dump.  ## this should be anExceptionBlock
(thisContext basicAt: 10) dump.  ## this should be handlerActive
'on:do: ABOUT TO EVALUE THE RECEIVER BLOCK' dump.
---------------------------------- *)
		exception_active := true.
		^self value.
	}

	method on: exc1 do: blk1 on: exc2 do: blk2
	{
		| exception_active |
		<exception>
		exception_active := true.
		^self value.
	}

	method ensure: aBlock
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

	method ifCurtailed: aBlock
	{
		| v ok |

		ok := false.
		[ v := self value. ok := true. ] ensure: [ ok ifFalse: [aBlock value] ].
		^v.
	}
}


##============================================================================
class PrimitiveFailureException(Exception)
{
}

class InstantiationFailureException(Exception)
{
}

class NoSuchMessageException(Exception)
{
}

class IndexOutOfRangeException(Exception)
{
}

class SubclassResponsibilityException(Exception)
{
}

class NotImplementedException(Exception)
{
}

class ErrorExceptionizationFailureException(Exception)
{
}

class InvalidArgumentException(Exception)
{
}

extend Apex
{
	method(#class) primitiveFailed
	{
		## TODO: implement this
## experimental backtrace...
| ctx |
ctx := thisContext.
[ctx notNil] whileTrue: [
	(ctx class == MethodContext) 
		ifTrue: [ (ctx method owner name & '>>' & ctx method name) dump ].
	## TODO: include blockcontext???
	ctx := ctx sender.
].
'------ END OF BACKTRACE -----------' dump.
		PrimitiveFailureException signal: 'PRIMITIVE FAILED'.
	}

	method(#class) cannotInstantiate
	{
## TOOD: accept a class
		InstantiationFailureException signal: 'Cannot instantiate'.
	}

	method(#class) doesNotUnderstand: message_name
	{
		## TODO: implement this properly
		NoSuchMessageException signal: (message_name & ' not understood by ' & (self name)).
	}

	method(#class) index: index outOfRange: ubound
	{
		IndexOutOfRangeException signal: 'Out of range'.
	}

	method(#class) subclassResponsibility: method_name
	{
		SubclassResponsibilityException signal: ('Subclass must implement ' & method_name).
	}

	method(#class) notImplemented: method_name
	{
		NotImplementedException signal: (method_name & ' not implemented by ' & (self name)).
	}
	
	method(#class) cannotExceptionizeError
	{
## todo: accept the object
		ErrorExceptionizationFailureException signal: 'Cannot exceptionize an error'
	}
}
