class Apex(nil)
{
}

class(#limited) Error(Apex)
{
}

pooldic Error.Code
{
	ENOERR     := error(0).
	EGENERIC   := error(1).
	ENOIMPL    := error(2).
	ESYSERR    := error(3).
	EINTERN    := error(4).
	ESYSMEM    := error(5).
	EOOMEM     := error(6).
	EINVAL     := error(7).
	ENOENT     := error(8).
	EPERM      := error(12).
	ERANGE     := error(20).
(* add more items... *)
}

(*pooldic Error.Code2 
{
	>> CAN I SUPPORT this kind of redefnition? as of now, it's not accepted because 
	>> Error.Code2.EGENERIC is not a literal. Should i treate pooldic members as a constant
	>> and treat it as if it's a literal like? then even if the defined value changes,
	>> the definition here won't see the change... what is the best way to tackle this issue?

	EGENERIC := Error.Code2.EGENERIC.
}*)

extend Apex
{
	## -------------------------------------------------------
	## -------------------------------------------------------
	method(#dual) dump
	{
		<primitive: #_dump>
	}

	## -------------------------------------------------------
	## -------------------------------------------------------
	method(#dual) yourself { ^self }

	## -------------------------------------------------------
	## INSTANTIATION & INITIALIZATION
	## -------------------------------------------------------
	method(#class,#primitive,#lenient) _basicNew.
	method(#class,#primitive,#lenient) _basicNew: size.

	method(#class,#primitive) basicNew.
	method(#class,#primitive) basicNew: size.
	(* the following definition is almost equivalent to the simpler definition
	 *    method(#class,#primitive) basicNew: size.
	 * found above.
	 * in the following defintion, the primitiveFailed method is executed 
	 * from the basicNew: context. but in the simpler definition, it is executed
	 * in the context of the caller of the basicNew:. the context of the basicNew:
	 * method is not even created
	method(#class) basicNew: size
	{
		<primitive: #Apex__basicNew:>
		self primitiveFailed(thisContext method)
	}*)

	method(#class) new
	{
		| x |
		x := self basicNew.
		x initialize. ## TODO: assess if it's good to call 'initialize' from new
		^x.
	}

	method(#class) new: anInteger
	{
		| x |
		x := self basicNew: anInteger.
		x initialize. ## TODO: assess if it's good to call 'initialize' from new.
		^x.
	}

	method initialize
	{
		(* a subclass may override this method *)
		^self.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------
	method(#dual,#primitive,#lenient) _shallowCopy.
	method(#dual,#primitive) shallowCopy.

	## -------------------------------------------------------
	## -------------------------------------------------------
	method(#dual,#primitive,#lenient) _basicSize.
	method(#dual,#primitive) basicSize.

	method(#dual,#primitive) basicAt: index.
	method(#dual,#primitive) basicAt: index put: value.
	
	##method(#dual,#primitive) basicMoveFrom: sindex length: length to: dindex.
	##method(#dual,#primitive) basicCopyFrom: sindex length: length to: dindex.

	(* ------------------------------------------------------------------
	 * FINALIZATION SUPPORT
	 * ------------------------------------------------------------------ *)
	method(#dual,#primitive) addToBeFinalized.
	method(#dual,#primitive) removeToBeFinalized.

	(* ------------------------------------------------------------------
	 * HASHING
	 * ------------------------------------------------------------------ *)
	method(#dual,#primitive) hash.

	(*
	method(#dual) hash
	{
		<primitive: #Apex_hash>
		self subclassResponsibility: #hash
	}*)

	(* ------------------------------------------------------------------
	 * IDENTITY TEST
	 * ------------------------------------------------------------------ *)

	## check if the receiver is identical to anObject.
	## this doesn't compare the contents 
	method(#dual, #primitive) == anObject.

	method(#dual) ~~ anObject
	{
		<primitive: #'Apex_~~'>
		^(self == anObject) not.
	}

	(* ------------------------------------------------------------------
	 * EQUALITY TEST
	 * ------------------------------------------------------------------ *)
	method(#dual) = anObject
	{
		<primitive: #'Apex_='>
		self subclassResponsibility: #=
	}
	
	method(#dual) ~= anObject
	{
		<primitive: #'Apex_~='>
		^(self = anObject) not.
	}


	(* ------------------------------------------------------------------
	 * COMMON QUERIES
	 * ------------------------------------------------------------------ *)

	method(#dual,#primitive) class.

	method(#dual) isNil
	{
		## ^self == nil.
		^false
	}

	method(#dual) notNil
	{
		## ^(self == nil) not
		## ^self ~= nil.
		^true.
	}

	method(#dual) isError
	{
		^false
	}

	method(#dual) notError
	{
		^true
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#class) inheritsFrom: aClass
	{
		| c |
		c := self superclass.
		(* [c notNil] whileTrue: [
			[ c == aClass ] ifTrue: [^true].
			c := c superclass.
		]. *)
		while (c notNil)
		{
			if (c == aClass) { ^true }.
			c := c superclass.
		}.
		^false
	}

	method(#class) isMemberOf: aClass
	{
		(* a class object is an instance of Class
		 * but Class inherits from Apex. On the other hand,
		 * most of ordinary classes are under Object again under Apex.
		 * special consideration is required here. *)
		^aClass == Class
	}

	method isMemberOf: aClass
	{
		^self class == aClass
	}

	method(#dual) isKindOf: aClass
	{
		<primitive: #Apex_isKindOf:>
		^(self isMemberOf: aClass) or: [self class inheritsFrom: aClass].
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#dual,#primitive) respondsTo: selector.

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#dual,#variadic,#primitive) perform(selector).

	method(#dual) perform: selector
	{
		<primitive: #Apex_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector with: arg1
	{
		<primitive: #Apex_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector with: arg1 with: arg2
	{
		<primitive: #Apex_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector with: arg1 with: arg2 with: arg3
	{
		<primitive: #Apex_perform>
		self primitiveFailed
	}

	(* ------------------------------------------------------------------
	 * COMMON ERROR/EXCEPTION HANDLERS
	 * ------------------------------------------------------------------ *)
	method(#dual) error: msgText
	{
		(* TODO: implement this
		  Error signal: msgText. *)
		msgText dump.
	}
}

class Object(Apex)
{

}

class UndefinedObject(Apex)
{
	method isNil
	{
		^true
	}

	method notNil
	{
		^false.
	}

	method handleException: exception
	{
		('### EXCEPTION NOT HANDLED #### ' &  exception class name & ' - ' & exception messageText) dump.
		## TODO: debug the current process???? "
## TODO: ensure to execute ensure blocks as well....
		####Processor activeProcess terminate.
	}
}



extend Error
{
	(* ----------------------------
	TODO: support nested pooldic/constant declaration...

	pooldic/const
	{
		NONE := error(0).
		GENERIC := error(1).
	}
	-------------------------------- *)

	method isError
	{
		^true
	}

	method notError
	{
		^false
	}

	method(#primitive) asCharacter.
	method asError { ^self }
	method(#primitive) asInteger.

	method(#primitive) asString.

	method signal
	{
		| exctx exblk retval actpos ctx |

		exctx := (thisContext sender) findExceptionContext.

		while (exctx notNil)
		{
			exblk := exctx findExceptionHandlerFor: (self class).
			if (exblk notNil and: [actpos := exctx basicSize - 1. exctx basicAt: actpos])
			{
				exctx basicAt: actpos put: false.
				[ retval := exblk value: self ] ensure: [ exctx basicAt: actpos put: true ].
				thisContext unwindTo: (exctx sender) return: nil.
				System return: retval to: (exctx sender).
			}.
			exctx := (exctx sender) findExceptionContext.
		}.

		## -----------------------------------------------------------------
		## FATAL ERROR - no exception handler.
		## -----------------------------------------------------------------
		##thisContext unwindTo: nil return: nil.
		##thisContext unwindTo: (Processor activeProcess initialContext) return: nil.
		
## TOOD: IMPROVE THIS EXPERIMENTAL BACKTRACE...
System logNl: '== BACKTRACE =='.
ctx := thisContext.
while (ctx notNil)
{
	if (ctx class == MethodContext) { System logNl: (' ' & ctx method owner name & '>>' & ctx method name) }.
	## TODO: include blockcontext???
	ctx := ctx sender.
}.
System logNl: '== END OF BACKTRACE =='.

		thisContext unwindTo: (thisProcess initialContext) return: nil.
		('### ERROR NOT HANDLED #### ' & self class name & ' - ' & self asString) dump.
		## TODO: debug the current process???? "

		##Processor activeProcess terminate.
		thisProcess terminate.
	}
}
