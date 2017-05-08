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
	
	(*
	method(#class) basicNew
	{
		| perr |
		<primitive: #Apex__basicNew>
		self primitiveFailed.

	##	perr := thisProcess primError.
	##	if (perr == xxxx) { self cannotInstantiate }
	##	else { self primitiveFailed }.
	}
	
	method(#class) basicNew: size
	{
		| perr |
		<primitive: #Apex__basicNew:>
		self primitiveFailed.

	##	perr := thisProcess primError.
	##	if (perr == xxxx) { self cannotInstantiate }
	##	else { self primitiveFailed }.
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
	##method(#primitive,#lenient) _shallowCopy.
	
	method(#dual) shallowCopy
	{
		<primitive: #_shallow_copy>
		self primitiveFailed.
	}

	
	## -------------------------------------------------------
	## -------------------------------------------------------
	method(#dual,#primitive,#lenient) _basicSize.
	method(#dual,#primitive) basicSize.

	method(#dual) basicAt: index
	{
		| perr |
		
		<primitive: #_basic_at>

## TODO: create a common method that translate a primitive error to some standard exceptions of primitive failure.
		perr := thisProcess primError.
		if (perr == Error.Code.ERANGE) { self index: index outOfRange: (self basicSize) }
		elsif (perr == Error.Code.EPERM) { self messageProhibited: #basicAt }
		else { self primitiveFailed }
	}

	method(#dual) basicAt: index put: anObject
	{
		| perr |
		
		<primitive: #_basic_at_put>

		perr := thisProcess primError.
		if (perr == Error.Code.ERANGE) { self index: index outOfRange: (self basicSize) }
		elsif (perr == Error.Code.EPERM) { self messageProhibited: #basicAt:put: }
		else { self primitiveFailed }
	}

	(* ------------------------------------------------------------------
	 * HASHING
	 * ------------------------------------------------------------------ *)
	method(#dual) hash
	{
		<primitive: #_hash>
		self subclassResponsibility: #hash
	}

	(* ------------------------------------------------------------------
	 * IDENTITY TEST
	 * ------------------------------------------------------------------ *)

	method(#dual) == anObject
	{
		(* check if the receiver is identical to anObject.
		 * this doesn't compare the contents *)
		<primitive: #_identical>
		self primitiveFailed.
	}

	method(#dual) ~~ anObject
	{
		<primitive: #_not_identical>
		^(self == anObject) not.
	}

	(* ------------------------------------------------------------------
	 * EQUALITY TEST
	 * ------------------------------------------------------------------ *)
	method(#dual) = anObject
	{
		<primitive: #_equal>
		self subclassResponsibility: #=
	}
	
	method(#dual) ~= anObject
	{
		<primitive: #_not_equal>
		^(self = anObject) not.
	}


	(* ------------------------------------------------------------------
	 * COMMON QUERIES
	 * ------------------------------------------------------------------ *)

	method(#dual,#primitive) class.
	 
	method(#dual) isNil
	{
		"^self == nil."
		^false
	}

	method(#dual) notNil
	{
		"^(self == nil) not"
		"^self ~= nil."
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
		[c notNil] whileTrue: [
			[ c == aClass ] ifTrue: [^true].
			c := c superclass.
		].
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

	method(#class) isKindOf: aClass
	{
		^(self isMemberOf: aClass) or: [self inheritsFrom: aClass].
	}

	method isMemberOf: aClass
	{
		^self class == aClass
	}

	method isKindOf: aClass
	{
		^(self isMemberOf: aClass) or: [self class inheritsFrom: aClass].
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#dual) respondsTo: selector
	{
		<primitive: #_responds_to>
		self primitiveFailed
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#dual,#variadic) perform(selector)
	{
		<primitive: #_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector
	{
		<primitive: #_perform>
		self primitiveFailed
	}
	

	method(#dual) perform: selector with: arg1
	{
		<primitive: #_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector with: arg1 with: arg2
	{
		<primitive: #_perform>
		self primitiveFailed
	}

	method(#dual) perform: selector with: arg1 with: arg2 with: arg3
	{
		<primitive: #_perform>
		self primitiveFailed
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method exceptionizeError: trueOrFalse
	{
		<primitive: #_exceptionize_error>
		self class cannotExceptionizeError
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

	method(#primitive) asInteger.
	method(#primitive) asCharacter.
	method(#primitive) asString.
}
