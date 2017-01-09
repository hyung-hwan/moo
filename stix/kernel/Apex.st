class Apex(nil)
{
	dcl(#class) sysdic.

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#class) dump
	{
		<primitive: #_dump>
	}

	method dump
	{
		<primitive: #_dump>
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#class) yourself
	{
		^self.
	}

	method yourself
	{
		^self.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method(#class) basicNew
	{
		<primitive: #_basic_new>
		self primitiveFailed.
	}

	method(#class) basicNew: anInteger
	{
		<primitive: #_basic_new_with_size>
		self primitiveFailed.
	}

	method(#class) ngcNew
	{
		<primitive: #_ngc_new>
		self primitiveFailed.
	}

	method(#class) ngcNew: anInteger
	{
		<primitive: #_ngc_new_with_size>
		self primitiveFailed.
	}

	method(#class) new
	{
		| x |
		x := self basicNew.
		x initialize. "TODO: assess if it's good to call 'initialize' from new."
		^x.
	}

	method(#class) new: anInteger
	{
		| x |
## TODO: check if the class is a fixed class.
##       if so, raise an exception.
		x := self basicNew: anInteger.
		x initialize. "TODO: assess if it's good to call 'initialize' from new."
		^x.
	}

	method initialize
	{
		"a subclass may override this method."
		^self.
	}

	method ngcDispose
	{
		<primitive: #_ngc_dispose>
		self primitiveFailed.
	}
	## -------------------------------------------------------
	## -------------------------------------------------------
	method shallowCopy
	{
		<primitive: #_shallow_copy>
		self primitiveFailed.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method class
	{
		<primitive: #_class>
	}

	method(#class) class
	{
		<primitive: #_class>
		^Class
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	method basicSize
	{
		<primitive: #_basic_size>
		self primitiveFailed.
	}

	method(#class) basicSize
	{
		<primitive: #_basic_size>
		self primitiveFailed.
	}

	method basicAt: index
	{
		<primitive: #_basic_at>
		self index: index outOfRange: (self basicSize).
	}

	method basicAt: index put: anObject
	{
		<primitive: #_basic_at_put>
		self index: index outOfRange: (self basicSize).
	}

	method(#class) basicAt: index
	{
		<primitive: #_basic_at>
		self index: index outOfRange: (self basicSize).
	}

	method(#class) basicAt: index put: anObject
	{
		<primitive: #_basic_at_put>
		self index: index outOfRange: (self basicSize).
	}

	(* ------------------------------------------------------------------
	 * HASHING
	 * ------------------------------------------------------------------ *)
	method hash
	{
		<primitive: #_hash>
		self subclassResponsibility: #hash
	}
	
	method(#class) hash
	{
		<primitive: #_hash>
		self subclassResponsibility: #hash
	}

	(* ------------------------------------------------------------------
	 * IDENTITY TEST
	 * ------------------------------------------------------------------ *)

	method == anObject
	{
		(* check if the receiver is identical to anObject.
		 * this doesn't compare the contents *)
		<primitive: #_identical>
		self primitiveFailed.
	}

	method ~~ anObject
	{
		<primitive: #_not_identical>
		^(self == anObject) not.
	}

	method(#class) == anObject
	{
		(* check if the receiver is identical to anObject.
		 * this doesn't compare the contents *)
		<primitive: #_identical>
		self primitiveFailed.
	}

	method(#class) ~~ anObject
	{
		<primitive: #_not_identical>
		^(self == anObject) not.
	}

	(* ------------------------------------------------------------------
	 * EQUALITY TEST
	 * ------------------------------------------------------------------ *)
	method = anObject
	{
		<primitive: #_equal>
		self subclassResponsibility: #=
	}
	
	method ~= anObject
	{
		<primitive: #_not_equal>
		^(self = anObject) not.
	}

	method(#class) = anObject
	{
		<primitive: #_equal>
		self subclassResponsibility: #=
	}
	
	method(#class) ~= anObject
	{
		<primitive: #_not_equal>
		^(self = anObject) not.
	}
	
	(* ------------------------------------------------------------------
	 * COMMON QUERIES
	 * ------------------------------------------------------------------ *)

	method isNil
	{
		"^self == nil."
		^false
	}

	method notNil
	{
		"^(self == nil) not"
		"^self ~= nil."
		^true.
	}

	method(#class) isNil
	{
		"^self == nil."
		^false
	}

	method(#class) notNil
	{
		"^(self == nil) not"
		"^self ~= nil."
		^true.
	}

	method isError
	{
		^false
	}

	method(#class) isError
	{
		^false
	}

	method notError
	{
		^true
	}

	method(#class) notError
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

	(* -----------------
	method(#class) respondsTo: selectorSymbol
	{
		TODO: find selectorSymbol in the class method dictionary...
	}

	method respondsTo: selectorSymbol
	{
		TODO: find selectorSymbol in the method dictionary...
	}
	------------ *)

	method exceptionizeError: trueOrFalse
	{
		<primitive: #_exceptionize_error>
		self class cannotExceptionizeError
	}

	(* ------------------------------------------------------------------
	 * COMMON ERROR/EXCEPTION HANDLERS
	 * ------------------------------------------------------------------ *)
	method primitiveFailed
	{
		^self class primitiveFailed.
	}

	method cannotInstantiate
	{
		^self class cannotInstantiate
	}

	method doesNotUnderstand: messageSymbol
	{
		^self class doesNotUnderstand: messageSymbol
	}

	method index: index outOfRange: ubound
	{
		^self class index: index outOfRange: ubound.
	}

	method subclassResponsibility: method_name
	{
		^self class subclassResponsibility: method_name
	}

	method notImplemented: method_name
	{
		^self class notImplemented: method_name
	}

	method cannotExceptionizeError
	{
		^self class cannotExceptionizeError
	}

	method(#class) error: msgText
	{
		(* TODO: implement this
		  Error signal: msgText. *)
		msgText dump.
	}

	method error: aString
	{
		self class error: aString.
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


pooldic ErrorCode
{
(* migrate it into Error class *)
	#NONE := error(0).
	#GENERIC := error(1).
	#NOENT := error(2).
}

class Error(Apex)
{
	(* ----------------------------
	TODO: support constant declaration...

	#const
	{
		#NONE := error(0).
		#GENERIC := error(1).
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

	method asInteger
	{
		<primitive: #_error_as_integer>
	}
	
	method asCharacter
	{
		<primitive: #_error_as_character>
	}

	method asString
	{
		<primitive: #_error_as_string>
	}
}
