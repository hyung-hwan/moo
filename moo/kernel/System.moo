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
	var(#class) asyncsg.

	method(#class) addAsyncSemaphore: sem
	{
		^self.asyncsg addSemaphore: sem
	}

	method(#class) removeAsyncSemaphore: sem
	{
		^self.asyncsg removeSemaphore: sem
	}

	method(#class) handleAsyncEvent
	{
		^self.asyncsg wait.
	}

	method(#class) startup(class_name, method_name)
	{
		| class ret |

		self.asyncsg := SemaphoreGroup new.

		class := self at: class_name. ## System at: class_name.
		if (class isError)
		{
			self error: ('Cannot find the class - ' & class_name).
		}.

		## start the gc finalizer process
		[ self __gc_finalizer ] fork.

		## TODO: change the method signature to variadic and pass extra arguments to perform???
		ret := class perform: method_name.

		#### System logNl: '======= END of startup ==============='.
		^ret.
	}

	method(#class) __gc_finalizer
	{
		| tmp gc gcfin_sem |

		gc := false.
		gcfin_sem := Semaphore new.

		gcfin_sem signalOnGCFin.

		[
			while (true)
			{
				while ((tmp := self _popCollectable) notError)
				{
					## TODO: Do i have to protected this in an exception handler???
					if (tmp respondsTo: #finalize) { tmp finalize }.
				}.

				##if (Processor total_count == 1)
				if (Processor should_exit)
				{
					## exit from this loop when there are no other processes running except this finalizer process
					if (gc) 
					{ 
						System logNl: 'Exiting the GC finalization process ' & (thisProcess id) asString.
						break.
					}.

					System logNl: 'Forcing garbage collection before termination in ' & (thisProcess id) asString.
					self collectGarbage.
					gc := true.
				}
				else
				{
					gc := false.
				}.

				##System logNl: '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^gc_waiting....'.
				##System sleepForSecs: 1. ## TODO: wait on semaphore instead..
				gcfin_sem wait.
				##System logNl: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX gc_waitED....'.
			}
		] ensure: [
			gcfin_sem unsignal.
			System logNl: 'End of GC finalization process ' & (thisProcess id) asString.
		].
	}

	method(#class,#primitive) _popCollectable.
	method(#class,#primitive) collectGarbage.
	method(#class,#primitive) gc.
	method(#class,#primitive) return: object to: context.

	## =======================================================================================
	method(#class) sleepForSecs: secs
	{
		## -----------------------------------------------------
		## put the calling process to sleep for given seconds.
		## -----------------------------------------------------
		| s |
		s := Semaphore new.
		self signal: s afterSecs: secs.
		s wait.
	}

	method(#class) sleepForSecs: secs nanosecs: nanosecs
	{
		## -----------------------------------------------------
		## put the calling process to sleep for given seconds.
		## -----------------------------------------------------
		| s |
		s := Semaphore new.
		self signal: s afterSecs: secs nanosecs: nanosecs.
		s wait.
	}
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
	method(#class,#variadic,#primitive) log(level,msg1).

(*
TODO: how to pass all variadic arguments to another variadic methods???
	method(#class,#variadic) logInfo (msg1)
	{
		^self log (System.Log.INFO,msg1)
	}
*)	
	method(#class) atLevel: level log: message
	{
		<primitive: #System_log>
		## do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2
	{
		<primitive: #System_log>
		## do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2 and: message3
	{
		<primitive: #System_log>
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

	method(#class) logNl
	{
		^self atLevel: System.Log.INFO log: S'\n'.
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

	(* raw memory allocation *)
	method(#class,#primitive) malloc (size).
	method(#class,#primitive) calloc (size).
	method(#class,#primitive) free (rawptr).
	
	method(#class,#primitive) malloc: size.
	method(#class,#primitive) calloc: size.
	method(#class,#primitive) free: rawptr.

	(* raw memory access *)
	method(#class,#primitive) getInt8   (rawptr, offset). ## <primitive: #System__getInt8>
	method(#class,#primitive) getInt16  (rawptr, offset).
	method(#class,#primitive) getInt32  (rawptr, offset).
	method(#class,#primitive) getInt64  (rawptr, offset).
	method(#class,#primitive) getUint8  (rawptr, offset). ## <primitive: #System__getUint8>
	method(#class,#primitive) getUint16 (rawptr, offset).
	method(#class,#primitive) getUint32 (rawptr, offset).
	method(#class,#primitive) getUint64 (rawptr, offset).

	method(#class,#primitive) putInt8   (rawptr, offset, value).
	method(#class,#primitive) putInt16  (rawptr, offset, value).
	method(#class,#primitive) putInt32  (rawptr, offset, value).
	method(#class,#primitive) putInt64  (rawptr, offset, value).
	method(#class,#primitive) putUint8  (rawptr, offset, value).
	method(#class,#primitive) putUint16 (rawptr, offset, value).
	method(#class,#primitive) putUint32 (rawptr, offset, value).
	method(#class,#primitive) putUint64 (rawptr, offset, value).

	method(#class,#primitive) getBytes (rawptr, offset, byte_array, offset_in_buffer, len_in_buffer).
	method(#class,#primitive) putBytes (rawptr, offset, byte_array, offset_in_buffer, len_in_buffer).
}


class SmallPointer(Object)
{
	method(#primitive) asString.
	
	method(#primitive) getInt8   (offset).
	method(#primitive) getInt16  (offset).
	method(#primitive) getInt32  (offset).
	method(#primitive) getInt64  (offset).
	method(#primitive) getUint8  (offset).
	method(#primitive) getUint16 (offset).
	method(#primitive) getUint32 (offset).
	method(#primitive) getUint64 (offset).

	method(#primitive) putInt8   (offset, value).
	method(#primitive) putInt16  (offset, value).
	method(#primitive) putInt32  (offset, value).
	method(#primitive) putInt64  (offset, value).
	method(#primitive) putUint8  (offset, value).
	method(#primitive) putUint16 (offset, value).
	method(#primitive) putUint32 (offset, value).
	method(#primitive) putUint64 (offset, value).

	method(#primitive) getBytes (offset, byte_array, offset_in_buffer, len_in_buffer).
	method(#primitive) putBytes (offset, byte_array, offset_in_buffer, len_in_buffer).

	method(#primitive) free.
}
