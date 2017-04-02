## TODO: consider if System can replace Apex itself.
##       System, being the top class, seems to give very natural way of 
##       offering global system-level functions and interfaces.
##
##           class System(nil) { ... }
##           class Object(System) { .... }
##           System at:  #
##           System logNl: 'xxxxx'.
##           System getUint8(ptr,offset)

class System(Apex)
{
}

pooldic System.Log
{
	## -----------------------------------------------------------
	## defines log levels
	## these items must follow defintions in moo.h
	## -----------------------------------------------------------

	DEBUG := 1.
	INFO  := 2.
	WARN  := 4.
	ERROR := 8.
	FATAL := 16.
}

extend System
{
	## the following methods may not look suitable to be placed
	## inside a system dictionary. but they are here for quick and dirty
	## output production from the moo code.
	##   System logNl: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'.
	##
	method(#class) atLevel: level log: message
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2 and: message3
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method(#class) atLevel: level logNl: message 
	{
		## the #_log primitive accepts an array.
		## so the following lines should work also.
		## | x |
		## x := Array new: 2.
		## x at: 0 put: message.
		## x at: 1 put: S'\n'.
		## ^self atLevel: level log: x.

		^self atLevel: level log: message and: S'\n'.
	}

	method(#class) atLevel: level logNl: message and: message2
	{
		^self atLevel: level log: message and: message2 and: S'\n'.
	}

	method(#class) log: message
	{
		^self atLevel: System.Log.INFO log: message.
	}

	method(#class) log: message and: message2
	{
		^self atLevel: System.Log.INFO log: message and: message2.
	}

	method(#class) logNl: message
	{
		^self atLevel: System.Log.INFO logNl: message.
	}

	method(#class) logNl: message and: message2
	{
		^self atLevel: System.Log.INFO logNl: message and: message2.
	}
	
	(* nsdic access *)
	method(#class) at: key
	{
		^self nsdic at: key
	}

	method(#class) at: key put: value
	{
		^self nsdic at: key put: value
	}

	(* raw memory access *)
	method(#class,#primitive) _getInt8   (rawptr, offset). ## <primitive: #System__getInt8>
	method(#class,#primitive) _getInt16  (rawptr, offset).
	method(#class,#primitive) _getInt32  (rawptr, offset).
	method(#class,#primitive) _getInt64  (rawptr, offset).
	method(#class,#primitive) _getUint8  (rawptr, offset). ## <primitive: #System__getUint8>
	method(#class,#primitive) _getUint16 (rawptr, offset).
	method(#class,#primitive) _getUint32 (rawptr, offset).
	method(#class,#primitive) _getUint64 (rawptr, offset).

	(*
	method(#class,#primitive) _putInt8   (rawptr, offset, value).
	method(#class,#primitive) _putInt16  (rawptr, offset, value).
	method(#class,#primitive) _putInt32  (rawptr, offset, value).
	method(#class,#primitive) _putInt64  (rawptr, offset, value).
	method(#class,#primitive) _putUint8  (rawptr, offset, value).
	method(#class,#primitive) _putUint16 (rawptr, offset, value).
	method(#class,#primitive) _putUint32 (rawptr, offset, value).
	method(#class,#primitive) _putUint64 (rawptr, offset, value),
	*)
}
