

##
## TODO: is it better to inherit from Object???
##       or treat Exception specially like UndefinedObject or Class???
##
class Exception(Apex)
{
	var signalContext, handlerContext, messageText.

(*
TODO: can i convert 'thisProcess primError' to a relevant exception?
	var(#class) primExceptTable.

	method(#class) forPrimitiveError: no
	{
		^self.primExceptTable at: no
	}
*)


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

	(* TODO: remove this....
	method __signal 
	{
		self.signalContext := thisContext.
		((thisContext sender) findExceptionContext) handleException: self.
	}*)

	method signal
	{
		| exctx exblk retval actpos ctx |

		self.signalContext := thisContext.
		exctx := (thisContext sender) findExceptionContext.
		
		##[exctx notNil] whileTrue: [
		while (exctx notNil)
		{
			exblk := exctx findExceptionHandlerFor: (self class).
			if (exblk notNil and: 
			    [actpos := exctx basicSize - 1. exctx basicAt: actpos])
			{
				self.handlerContext := exctx.
				exctx basicAt: actpos put: false.
				[ retval := exblk value: self ] ensure: [ exctx basicAt: actpos put: true ].
				thisContext unwindTo: (exctx sender) return: nil.
				Processor return: retval to: (exctx sender).
			}.
			exctx := (exctx sender) findExceptionContext.
		}.

		## -----------------------------------------------------------------
		## FATAL ERROR - no exception handler.
		## -----------------------------------------------------------------
		##thisContext unwindTo: nil return: nil.
		##thisContext unwindTo: (Processor activeProcess initialContext) return: nil.
		
## TOOD: IMPROVE THIS EXPERIMENTAL BACKTRACE...
ctx := thisContext.
while (ctx notNil)
{
	if (ctx class == MethodContext) { (ctx method owner name & '>>' & ctx method name) dump }.
	## TODO: include blockcontext???
	ctx := ctx sender.
}.

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
		if (self.handlerContext notNil) { Processor return: value to: self.handlerContext }
	}

	method retry
	{
## TODO: verify if return:to: causes unnecessary stack growth.
		if (self.handlerContext notNil) 
		{
			self.handlerContext pc: 0.
			Processor return: self to: self.handlerContext.
		}
	}

	method resume: value
	{
## TODO: verify if return:to: causes unnecessary stack growth.
## is this correct???
		| ctx |
		if (self.signalContext notNil and: [self.handlerContext notNil])
		{
			ctx := self.signalContext sender.
			self.signalContext := nil.
			self.handlerContext := nil.
			Processor return: value to: ctx.
		}.
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
		while (ctx notNil)
		{
			if (ctx isExceptionContext) { ^ctx }.
			ctx := ctx sender.
		}.
		^nil
	}

	method unwindTo: context return: retval
	{
		## -------------------------------------------------------------------
		## <<private>>
		## private: called by VM upon unwinding
		## -------------------------------------------------------------------

		| ctx stop eb pending_pos |

		ctx := self.
		stop := false.
		until (stop)
		{
			eb := ctx ensureBlock.
			if (eb notNil)
			{
				(* position of the temporary variable in the ensureBlock that indicates
				 * if the block has been evaluated *)
				pending_pos := ctx basicSize - 1. 
				if (ctx basicAt: pending_pos)
				{
					ctx basicAt: pending_pos put: false.
					eb value.
				}
			}.
			stop := (ctx == context).
			ctx := ctx sender.

			## stop ifFalse: [ stop := ctx isNil ].
		}.

		^retval
	}
}

##============================================================================
extend MethodContext
{
	method isExceptionContext
	{
		## 12 - MOO_METHOD_PREAMBLE_EXCEPTION in VM.
		^self.method preambleCode == 12.
	}

	method isEnsureContext
	{
		## 13 - MOO_METHOD_PREAMBLE_ENSURE in VM.
		^self.method preambleCode == 13
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
		^if (self.method preambleCode == 13) { self basicAt: 8 } else { nil }
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

		| size exc |
		
		if (self isExceptionContext) 
		{
			(* NOTE: the following loop scans all parameters to the on:do: method.
			 *       if the on:do: method contains local temporary variables,
			 *       those must be skipped from scanning. *)

			size := self basicSize.
			8 priorTo: size by: 2 do: [ :i | 
				exc := self basicAt: i.
				if ((exception_class == exc) or: [exception_class inheritsFrom: exc]) { ^self basicAt: (i + 1) }.
			]
		}.
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

		(* position of the temporary variable 'exception_active' in MethodContext>>on:do.
		 * for this code to work, it must be the last temporary variable in the method. *)
		actpos := (self basicSize) - 1. 

		excblk := self findExceptionHandlerFor: (exception class).
		if (excblk isNil or: [(self basicAt: actpos) not])
		{
			## self is an exception context but doesn't have a matching
			## exception handler or the exception context is already
			## in the middle of evaluation. 
			^(self.sender findExceptionContext) handleException: exception.
		}.

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
		| retval pending |
		<ensure>

		pending := true.
		retval := self value. 

		(* the temporary variable 'pending' may get changed
		 * during evaluation for exception handling. 
		 * it gets chagned in Context>>unwindTo:return: *)
		if (pending) { pending := false. aBlock value }.
		^retval
	}

	method ifCurtailed: aBlock
	{
		| v pending |
		pending := true.
		[ v := self value. pending := false. ] ensure: [ if (pending) { aBlock value } ].
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

class ProhibitedMessageException(Exception)
{
}

extend Apex
{
	method(#dual) primitiveFailed
	{
		PrimitiveFailureException signal: 'PRIMITIVE FAILED'.
	}

	method(#dual) cannotInstantiate
	{
		## TODO: use displayString or something like that instead of name....
		InstantiationFailureException signal: 'Cannot instantiate ' & (self name).
	}

	method(#dual) doesNotUnderstand: message_name
	{
		## TODO: implement this properly
		| class_name |
		class_name := if (self class == Class) { self name } else { self class name }.
		NoSuchMessageException signal: (message_name & ' not understood by ' & class_name).
	}

	method(#dual) index: index outOfRange: ubound
	{
		IndexOutOfRangeException signal: 'Out of range'.
	}

	method(#dual) subclassResponsibility: method_name
	{
		SubclassResponsibilityException signal: ('Subclass must implement ' & method_name).
	}

	method(#dual) notImplemented: method_name
	{
		| class_name |
		class_name := if (self class == Class) { self name } else { self class name }.
		NotImplementedException signal: (method_name & ' not implemented by ' & class_name).
	}
	
	method(#dual) messageProhibited: method_name
	{
		| class_name |
		class_name := if (self class == Class) { self name } else { self class name }.
		ProhibitedMessageException signal: (method_name & ' not allowed for ' & class_name).
	}

	method(#dual) cannotExceptionizeError
	{
## todo: accept the object
		ErrorExceptionizationFailureException signal: 'Cannot exceptionize an error'
	}
}
