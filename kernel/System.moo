// TODO: consider if System can replace Apex itself.
//       System, being the top class, seems to give very natural way of 
//       offering global system-level functions and interfaces.
//
//           class System(nil) { ... }
//           class Object(System) { .... }
//           System at:  #
//           System logNl: 'xxxxx'.
//           System getUint8(ptr,offset)

class System(Apex)
{
	var(#class) asyncsg.
	var(#class) gcfin_sem.
	var(#class) gcfin_should_exit := false.
	var(#class) ossig_pid.
	var(#class) shr. // signal handler registry

	pooldic Log
	{
		// -----------------------------------------------------------
		// defines log levels
		// these items must follow defintions in moo.h
		// -----------------------------------------------------------

		DEBUG := 1,
		INFO  := 2,
		WARN  := 4,
		ERROR := 8,
		FATAL := 16
	}

	method(#class) _initialize
	{
		self.shr := OrderedCollection new.
		self.asyncsg := SemaphoreGroup new.
	}

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

	method(#class) installSignalHandler: block
	{
		self.shr addLast: block.
	}

	method(#class) uninstallSignalHandler: block
	{
		self.shr remove: block.
	}

	method(#class) startup(class_name, method_name)
	{
		| class ret gcfin_proc ossig_proc |

		System gc.

		class := self at: class_name. // System at: class_name.
		if (class isError)
		{
			self error: ("Unable to find the class - " & class_name).
		}.

		self _initialize.

		// start the gc finalizer process and os signal handler process
		//[ self __gc_finalizer ] fork.
		//[ self __os_sig_handler ] fork.
		gcfin_proc := [ self __gc_finalizer ] newSystemProcess.
		ossig_proc := [ :caller | self __os_sig_handler: caller ] newSystemProcess(thisProcess).
		
		self.ossig_pid := ossig_proc id.

		gcfin_proc resume.
		ossig_proc resume.

		[
			// TODO: change the method signature to variadic and pass extra arguments to perform???
			ret := class perform: method_name.
		] 
		ensure: [
			// signal end of the main process.
			// __os_sig_handler will terminate all running subprocesses.
			self _setSig: 16rFF. 
		].

		^ret.
	}

	method(#class) __gc_finalizer
	{
		| tmp gc |

		gc := false.

		self.gcfin_should_exit := false.
		self.gcfin_sem := Semaphore new.
		self.gcfin_sem signalOnGCFin. // tell VM to signal this semaphore when it schedules gc finalization.

		[
			while (true)
			{
				while ((tmp := self _popCollectable) notError)
				{
					if (tmp respondsTo: #finalize)
					{ 
						// finalize is protected with an exception handler.
						// the exception is ignored except it is logged.
						[ tmp finalize ] on: Exception do: [:ex | System longNl: "Exception in finalize - " & ex messageText ]
					}.
				}.

				//if (Processor total_count == 1)
				//if (Processor gcfin_should_exit)
				if (self.gcfin_should_exit)
				{
					// exit from this loop when there are no other processes running except this finalizer process
					if (gc) 
					{ 
						System logNl: "Exiting the GC finalization process " & (thisProcess id) asString.
						break.
					}.

					System logNl: "Forcing garbage collection before termination in " & (thisProcess id) asString.
					self collectGarbage.
					gc := true.
				}
				else
				{
					gc := false.
				}.

				self.gcfin_sem wait.
			}
		] ensure: [
			self.gcfin_sem signal. // in case the process is stuck in wait.
			self.gcfin_sem unsignal.
			System logNl: "End of GC finalization process " & (thisProcess id) asString.
		].
	}

	method(#class) __os_sig_handler: caller
	{
		| os_intr_sem signo sh |

		os_intr_sem := Semaphore new.
		os_intr_sem signalOnInput: System _getSigfd.

		[
			while (true)
			{
				until ((signo := self _getSig) isError)
				{
					// TODO: Do i have to protected this in an exception handler???
					//TODO: Execute Handler for signo.

					System logNl: "Interrupt detected - signal no - " & signo asString.

//System logNl: "WWWWWWWWWWWWWWWWWWWWWWWWW ".
					// user-defined signal handler is not allowed for 16rFF
					if (signo == 16rFF) { goto done }. 
//System logNl: "OHHHHHHHHHHHHHH ".

					ifnot (self.shr isEmpty)
					{
//System logNl: "About to execute handler for the signal detected - " & signo asString.
						self.shr do: [ :handler | handler value: signo ].
					}
					else
					{
//System logNl: "Jumping to done detected - signal no - " & signo asString.
						if (signo == 2) { goto done }.
					}.
				}.
//System logNl: "Waiting for signal on os_intr_sem...".
				os_intr_sem wait.
			}.
		done:
//System logNl: "Jumped to done detected - signal no - " & signo asString.
			nil.
		]
		ensure: [
			| pid proc oldps |

//System logNl: "Aborting signal handler......".
			// stop subscribing to signals.
			os_intr_sem signal.
			os_intr_sem unsignal.

			// the caller must request to terminate all its child processes..

			// this disables autonomous process switching only. 
			// TODO: check if the ensure block code can trigger process switching?
			//       what happens if the ensure block creates new processes? this is likely to affect the termination loop below.
			//       even the id of the terminated process may get reused.... 
			oldps := self _toggleProcessSwitching: false.

			/*
			 0 -> startup  <--- this should also be stored in the "caller" variable.
			 1 -> __gc_finalizer
			 2 -> __os_sig_handler 
			 3 ..  -> other processes started by application.

			from the second run onwards, the starting pid may not be 0.
			*/
			//proc := System _findNextProcess: self.ossig_pid.
			proc := System _findFirstProcess.
			while (proc notError)
			{
				pid := proc id.
				if (proc isNormal)
				{
					System logNl: ("Requesting to terminate process of id - " & pid asString).
					proc terminate.
				}.
				proc := System _findNextProcess: pid.
			}.

			System logNl: "Requesting to terminate the caller process of id " & (caller id) asString.
			caller terminate.  // terminate the startup process.
			self _toggleProcessSwitching: oldps.

			System logNl: ">>>>End of OS signal handler process " & (thisProcess id) asString.

			self.gcfin_should_exit := true.
			self.gcfin_sem signal. // wake the gcfin process.

			self _halting. // inform VM that it should get ready for halting.
		].
	}

	method(#class,#primitive) _getSig.
	method(#class,#primitive) _getSigfd.
	method(#class,#primitive) _setSig: signo.
	method(#class,#primitive) _halting.
	method(#class,#primitive) _toggleProcessSwitching: v.
	method(#class,#primitive,#lenient) _findProcessById: id.
	method(#class,#primitive,#lenient) _findFirstProcess.
	method(#class,#primitive,#lenient) _findLastProcess.
	method(#class,#primitive,#lenient) _findPreviousProcess: p. // process id or process object
	method(#class,#primitive,#lenient) _findNextProcess: p. // process id or process object

	method(#class,#primitive) _popCollectable.
	method(#class,#primitive) collectGarbage.
	method(#class,#primitive) gc.
	method(#class,#primitive) return: object to: context.

	// =======================================================================================

	method(#class) sleepForSecs: secs
	{
		// -----------------------------------------------------
		// put the calling process to sleep for given seconds.
		// -----------------------------------------------------
		| s |
		s := Semaphore new.
		s signalAfterSecs: secs.
		s wait.
	}

	method(#class) sleepForSecs: secs nanosecs: nanosecs
	{
		// -----------------------------------------------------
		// put the calling process to sleep for given seconds.
		// -----------------------------------------------------
		| s |
		s := Semaphore new.
		s signalAfterSecs: secs nanosecs: nanosecs.
		s wait.
	}

	// the following methods may not look suitable to be placed
	// inside a system dictionary. but they are here for quick and dirty
	// output production from the moo code.
	//   System logNl: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'.
	//
	method(#class,#variadic,#primitive) log(level,msg1).

/*
TODO: how to pass all variadic arguments to another variadic methods???
	method(#class,#variadic) logInfo (msg1)
	{
		^self log (System.Log.INFO,msg1)
	}
*/	
	method(#class) atLevel: level log: message
	{
		<primitive: #System_log>
		// do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2
	{
		<primitive: #System_log>
		// do nothing upon logging failure
	}

	method(#class) atLevel: level log: message and: message2 and: message3
	{
		<primitive: #System_log>
		// do nothing upon logging failure
	}

	method(#class) atLevel: level logNl: message 
	{
		// the #_log primitive accepts an array.
		// so the following lines should work also.
		// | x |
		// x := Array new: 2.
		// x at: 0 put: message.
		// x at: 1 put: "\n".
		// ^self atLevel: level log: x.

		^self atLevel: level log: message and: "\n".
	}

	method(#class) atLevel: level logNl: message and: message2
	{
		^self atLevel: level log: message and: message2 and: "\n".
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
		^self atLevel: System.Log.INFO log: "\n".
	}
	
	method(#class) logNl: message
	{
		^self atLevel: System.Log.INFO logNl: message.
	}

	method(#class) logNl: message and: message2
	{
		^self atLevel: System.Log.INFO logNl: message and: message2.
	}

	method(#class) backtrace
	{
		| ctx oldps |
		// TOOD: IMPROVE THIS EXPERIMENTAL BACKTRACE... MOVE THIS TO  System>>backtrace and skip the first method context for backtrace itself.
// TODO: make this method atomic? no other process should get scheduled while this function is running?
//  possible imementation methods:
//    1. disable task switching? -> 
//    2. use a global lock.
//    3. make this a primitive function. -> natually no callback.
//    4. introduce a new method attribute. e.g. #atomic -> vm disables task switching or uses a lock to achieve atomicity.
// >>>> i think it should not be atomic as a while. only logging output should be produeced at one go.

		oldps := System _toggleProcessSwitching: false.
		System logNl: "== BACKTRACE ==".

		//ctx := thisContext.
		ctx := thisContext sender. // skip the current context. skip to the caller context.
		while (ctx notNil)
		{
			// if (ctx sender isNil) { break }. // to skip the fake top level call context...

			if (ctx class == MethodContext) 
			{
				System log: " ";
				       log: ctx method owner name;
				       log: ">>";
				       log: ctx method name;
				       log: " (";
				       log: ctx method sourceFile;
				       log: " ";
				       log: (ctx method ipSourceLine: (ctx pc)) asString;
				       logNl: ")".
				//System logNl: (" " & ctx method owner name & ">>" & ctx method name &
				//			   " (" & ctx method sourceFile & " " & (ctx method ipSourceLine: (ctx pc)) asString & ")"). 
			}.
			// TODO: include blockcontext???
			ctx := ctx sender.
		}.
		System logNl: "== END OF BACKTRACE ==".
		System _toggleProcessSwitching: oldps.
	}

	/* nsdic access */
	method(#class) at: key
	{
		^self nsdic at: key
	}

	method(#class) at: key put: value
	{
		^self nsdic at: key put: value
	}

	/* raw memory allocation */
	method(#class,#primitive) malloc (size).
	method(#class,#primitive) calloc (size).
	method(#class,#primitive) free (rawptr).
	
	method(#class,#primitive) malloc: size.
	method(#class,#primitive) calloc: size.
	method(#class,#primitive) free: rawptr.

	/* raw memory access */
	method(#class,#primitive) getInt8   (rawptr, offset). // <primitive: #System__getInt8>
	method(#class,#primitive) getInt16  (rawptr, offset).
	method(#class,#primitive) getInt32  (rawptr, offset).
	method(#class,#primitive) getInt64  (rawptr, offset).
	method(#class,#primitive) getUint8  (rawptr, offset). // <primitive: #System__getUint8>
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


// TODO: support Pointer arithmetic?
class(#limited) SmallPointer(Object)
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

class(#limited,#immutable,#word(1)) LargePointer(SmallPointer)
{
}
