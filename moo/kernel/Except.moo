

//
// TODO: is it better to inherit from Object???
//       or treat Exception specially like UndefinedObject or Class???
//
class Exception(Apex)
{
	var signalContext, handlerContext.
	var(#get) messageText.

/*
TODO: can i convert 'thisProcess primError' to a relevant exception?
	var(#class) primExceptTable.

	method(#class) forPrimitiveError: no
	{
		^self.primExceptTable at: no
	}
*/

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


	method asString
	{
		^(self class name) & " - " & self.messageText.
	}

	method signal
	{
		| exctx exblk retval actpos ctx |

		self.signalContext := thisContext.
		exctx := (thisContext sender) findExceptionContext.

		//[exctx notNil] whileTrue: [
		while (exctx notNil)
		{
			exblk := exctx findExceptionHandler: (self class).
			//if (exblk notNil and: [actpos := exctx basicLastIndex. exctx basicAt: actpos])
			if ((exblk notNil) and (exctx basicAt: (actpos := exctx basicLastIndex)))
			{
				self.handlerContext := exctx.
				exctx basicAt: actpos put: false.
				[ retval := exblk value: self ] ensure: [ exctx basicAt: actpos put: true ].
				thisContext unwindTo: (exctx sender) return: nil.
				System return: retval to: (exctx sender).
			}.
			exctx := (exctx sender) findExceptionContext.
		}.

		// -----------------------------------------------------------------
		// FATAL ERROR - no exception handler.
		// -----------------------------------------------------------------
		//thisContext unwindTo: nil return: nil.
		//thisContext unwindTo: (Processor activeProcess initialContext) return: nil.

		System backtrace.

		thisContext unwindTo: (thisProcess initialContext) return: nil.
		("### EXCEPTION NOT HANDLED(Exception) #### " & self class name & " - " & self messageText) dump.
		// TODO: debug the current process???? "

		//Processor activeProcess terminate.
		thisProcess terminate.
	}

	method signal: text
	{
		self.messageText := text.
		^self signal.
	}

	method pass
	{
		// pass the exception to the outer context
		((self.handlerContext sender) findExceptionContext) handleException: self.
	}

	method return: value
	{
		if (self.handlerContext notNil) { System return: value to: self.handlerContext }
	}

	method retry
	{
// TODO: verify if return:to: causes unnecessary stack growth.
		if (self.handlerContext notNil) 
		{
			self.handlerContext pc: 0.
			System return: self to: self.handlerContext.
		}
	}

	method resume: value
	{
// TODO: verify if return:to: causes unnecessary stack growth.
// is this correct???
		| ctx |
		if ((self.signalContext notNil) and (self.handlerContext notNil))
		{
			ctx := self.signalContext sender.
			self.signalContext := nil.
			self.handlerContext := nil.
			System return: value to: ctx.
		}.
	}

	method resume
	{
		^self resume: nil.
	}
}

//============================================================================
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
		// -------------------------------------------------------------------
		// <<private>>
		// private: called by VM upon unwinding as well as by 
		//          Exception>>signal, Error>>signal, Process>>terminate
		// -------------------------------------------------------------------

		| ctx stop eb pending_pos |

		ctx := self.
		stop := false.
		until (stop)
		{
			eb := ctx ensureBlock.
			if (eb notNil)
			{
				/* position of the temporary variable in the ensureBlock that indicates
				 * if the block has been evaluated. see the method BlockContext>>ensure:.
				 * it is the position of the last temporary variable of the method */
				pending_pos := ctx basicLastIndex.
				/*
				if (ctx basicAt: pending_pos)
				{
					ctx basicAt: pending_pos put: false.
					eb value.
				}*/
				if (ctx basicAt: pending_pos test: true put: false) 
				{
					// TODO: what is the best way to handle an exception raised in an ensure block?
					[ eb value ] on: Exception do: [:ex | System logNl: "WARNING: unhandled exception in ensure block - " & ex messageText ].
				}.
			}.
			stop := (ctx == context).
			ctx := ctx sender.

			// stop ifFalse: [ stop := ctx isNil ].
		}.

		^retval
	}
}

//============================================================================
pooldic MethodContext.Preamble
{
	// this must follow MOO_METHOD_PREAMBLE_EXCEPTION in moo.h
	EXCEPTION := 13,

	// this must follow MOO_METHOD_PREAMBLE_ENSURE in moo.h
	ENSURE := 14
}

pooldic MethodContext.Index
{
	// [ value-block ] ensure: [ ensure-block ]
	// assuming ensure block is a parameter the ensure: method to a 
	// block context, the first parameter is placed after the fixed
	// instance variables of the method context. As MethodContex has
	// instance variables, the ensure block must be at the 9th position
	// which translates to index 8 
	ENSURE := 8,

	// [ ... ] on: Exception: do: [:ex | ... ]
	FIRST_ON := 8
}

extend MethodContext
{
	method isExceptionContext
	{
		^self.method preambleCode == MethodContext.Preamble.EXCEPTION.
	}

	method isEnsureContext
	{
		^self.method preambleCode == MethodContext.Preamble.ENSURE.
	}

	method ensureBlock
	{
		^if (self.method preambleCode == MethodContext.Preamble.ENSURE) { self basicAt: MethodContext.Index.ENSURE } else { nil }
	}


	method findExceptionHandler: exception_class
	{
		/* find an exception handler block for a given exception class.
		 *
		 * for this to work, self must be an exception handler context.
		 * For a single on:do: call,
		 *   self class specNumInstVars must return 8.(i.e.MethodContext has 8 instance variables.)
		 *   basicAt: 8 must be the on: argument.
		 *   basicAt: 9 must be the do: argument
		 */
		| size exc i |

		<primitive: #MethodContext_findExceptionHandler:>

		if (self isExceptionContext) 
		{
			/* [NOTE] the following loop scans all parameters to the on:do: method.
			 *       if the on:do: method contains local temporary variables,
			 *       you must change this function to skip scanning local variables. 
			 *       the current on:do: method has 1 local variable declared.
			 *       as local variables are placed after method arguments and 
			 *       the loop increments 'i' by 2, the last element is naturally
			 *       get excluded from inspection.
			 */
			size := self basicSize.

			// start scanning from the position of the first parameter
			i := MethodContext.Index.FIRST_ON. 
			while (i < size)
			{
				exc := self basicAt: i.
				if ((exception_class == exc) or (exception_class inheritsFrom: exc)) 
				{ 
					^self basicAt: (i + 1).
				}.
				i := i + 2.
			}.
		}.
		^nil.
	}

	method handleException: exception
	{
		/* -----------------------------------------------------------------
		 * <<private>>
		 *   called by Exception>>signal.
		 *   this method only exists in the MethodContext and UndefinedObject.
		 *   the caller must make sure that the receiver object is
		 *   a method context or nil. Exception>>signal invokes this method
		 *   only for an exception context which is a method context. it 
		 *   invokes it for nil when no exception context is found.
		 * ---------------------------------------------------------------- */

		| excblk retval actpos |

		/* position of the temporary variable 'exception_active' in MethodContext>>on:do.
		 * for this code to work, it must be the last temporary variable in the method. */
		//actpos := (self basicSize) - 1. 
		actpos := self basicLastIndex.

		excblk := self findExceptionHandler: (exception class).
		if ((excblk isNil) or ((self basicAt: actpos) not))
		{
			// self is an exception context but doesn't have a matching
			// exception handler or the exception context is already
			// in the middle of evaluation. 
			^(self.sender findExceptionContext) handleException: exception.
		}.

		exception handlerContext: self.

		/* -----------------------------------------------------------------
		 * if an exception occurs within an exception handler block,
		 * the search will reach this context again as the exception block
		 * is evaluated before actual unwinding. set the temporary variable
		 * in the exception context to mask out this context from the search
		 * list.
		 * ---------------------------------------------------------------- */
		self basicAt: actpos put: false.
		[ retval := excblk value: exception ] ensure: [
			self basicAt: actpos put: true
		].

		//(self.sender isNil) ifTrue: [ "TODO: CANNOT RETURN" ].

		/* -----------------------------------------------------------------
		 * return to self.sender which is a caller of the exception context (on:do:)
		 * pass the first ensure context between thisContext and self.sender.
		 * [ [Exception signal: 'xxx'] ensure: [20] ] on: Exception do: [:ex | ...]
		 * ---------------------------------------------------------------- */
		thisContext unwindTo: self.sender return: nil.
		System return: retval to: self.sender.
	}
}

//============================================================================
extend BlockContext
{
	method on: anException do: anExceptionBlock
	{
		| exception_active |
		<exception>

/* ------------------------------- 
thisContext isExceptionContext dump.
(thisContext basicSize) dump.
(thisContext basicAt: 8) dump.  // this should be anException
(thisContext basicAt: 9) dump.  // this should be anExceptionBlock
(thisContext basicAt: 10) dump.  // this should be handlerActive
'on:do: ABOUT TO EVALUE THE RECEIVER BLOCK' dump.
---------------------------------- */
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

	method on: exc1 do: blk1 on: exc2 do: blk2 on: exc3 do: blk3
	{
		| exception_active |
		<exception>
		exception_active := true.
		^self value.
	}

	method on: exc1 do: blk1 on: exc2 do: blk2 on: exc3 do: blk3 on: exc4 do: blk4
	{
		| exception_active |
		<exception>
		exception_active := true.
		^self value.
	}

	method on: exc1 do: blk1 on: exc2 do: blk2 on: exc3 do: blk3 on: exc4 do: blk4 on: exc5 do: blk5
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

		/* the temporary variable 'pending' may get changed
		 * during evaluation for exception handling. 
		 * it gets chagned in Context>>unwindTo:return: */
		/*if (pending) { pending := false. aBlock value }.*/
		if (thisContext basicAt: (thisContext basicLastIndex) test: true put: false) { aBlock value }.
		^retval
	}

	method ifCurtailed: aBlock
	{
		| v pending |
		pending := true.
		[ v := self value. pending := false. ] ensure: [ if (pending) { aBlock value } ].
		^v.
	}

	method catch
	{
		^self on: Exception do: [:ex | ex ] 
		      on: Error do: [:err | err ].
	}
}


//============================================================================
class PrimitiveFailureException(Exception)
{
	var errcode.
	
	method(#class) withErrorCode: code
	{
		^(self new) errorCode: code; yourself.
	}
	
	method signal
	{
		self.errcode := thisProcess primError.
		^super signal
	}
	
	method errorCode
	{
		^self.errcode
	}
	
	method errorCode: code
	{
		self.errcode := code
	}
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

class InvalidArgumentException(Exception)
{
}

class ProhibitedMessageException(Exception)
{
}

class NotFoundException(Exception)
{
}

class KeyNotFoundException(NotFoundException)
{
}

class ValueNotFoundException(NotFoundException)
{
}

class IndexNotFoundException(NotFoundException)
{
}

extend Apex
{
	method(#dual) error: text
	{
		Exception signal: text.
	}

	method(#dual,#liberal) primitiveFailed(method)
	{
		| a b msg ec ex |

		/* since method is an argument, the caller can call this method
		 * from a context chain where the method context actually doesn't exist.
		 * when a primitive method is defined using the #primitive method,
		 * the VM invokes this primtiveFailed method without creating 
		 * the context for the primitive method.
		 *    method(#primitive) xxx.
		 *    method(#primitive) xxx { <primitive: #_xxx> ^self primitiveFailed(thisContext method). }
		 * in the former definition, primitiveFailed is called without
		 * an activate context for the method xxx if the primitive fails.
		 * on the other handle, in the latter definition, the context
		 * for the method is activated first before primitiveFailed is 
		 * invoked. in the context chain, the method for xxx is found.
		 */
		 
		/*System logNl: 'Arguments: '.
		a := 0.
		b := thisContext vargCount.
		while (a < b)
		{
			System logNl: (thisContext vargAt: a) asString.
			a := a + 1.
		}.*/

		ec := thisProcess primError.
		msg := thisProcess primErrorMessage.
		if (msg isNil) { msg := ec asString }.
		if (method notNil) { msg := msg & " - " & (method owner name) & ">>" & (method name) }.

		//# TODO: convert an exception to a more specific one depending on the error code.
		//#if (ec == Error.Code.ERANGE) { self index: index outOfRange: (self basicSize) }
		//# elif (ec == Error.Code.EPERM) { self messageProhibited: method name }
		//# elif (ec == Error.Code.ENOIMPL) { self subclassResponsibility: method name }.

		(PrimitiveFailureException /* in: method */ withErrorCode: ec) signal: msg.
	}

	method(#dual) doesNotUnderstand: message_name
	{
		// TODO: implement this properly
		| class_name ctx |
		class_name := if (self class == Class) { self name } else { self class name }.

		System backtrace.

		NoSuchMessageException signal: (message_name & " not understood by " & class_name).
	}

	method(#dual) index: index outOfRange: ubound
	{
		IndexOutOfRangeException signal: "Out of range".
	}

	method(#dual) subclassResponsibility: method_name
	{
		SubclassResponsibilityException signal: ("Subclass must implement " & method_name).
	}

	method(#dual) shouldNotImplement: method_name
	{
		SubclassResponsibilityException signal: ("Message should not be implemented - " & method_name).
	}

	method(#dual) notImplemented: method_name
	{
		| class_name |
		class_name := if (self class == Class) { self name } else { self class name }.
		NotImplementedException signal: (method_name & " not implemented by " & class_name).
	}
	
	method(#dual) messageProhibited: method_name
	{
		| class_name |
		class_name := if (self class == Class) { self name } else { self class name }.
		ProhibitedMessageException signal: (method_name & " not allowed for " & class_name).
	}
}
