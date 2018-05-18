#include 'Moo.moo'.

##interface IPAddressInterface
##{
##}
## class XXX(Object,IPAddressInterface) {}

class(#byte) IPAddress(Object)
{
}

### TODO: extend the compiler
### #byte(4) basic size if 4 bytes. basicNew: xxx creates an instance of the size 4 + xxx.
### -> extend to support fixed 4 bytes by throwing an error in basicNew:.
### -> #byte(4,fixed)? 
### -> #byte -> byte variable/flexible
### -> #byte(4) -> byte variable with the mimimum size of 4
### -> (TODO)-> #byte(4,10) -> byte variable with the mimum size of 4 and maximum size of 10 => basicNew: should be allowed with upto 6.
### -> #byte(4,4) -> it can emulated fixed byte size. -> do i  have space in spec to store the upper bound?


class(#byte(4)) IP4Address(IPAddress)
{
	(*method(#class) new
	{
		^self basicNew: 4.
	}*)
	
	method(#class) fromString: str
	{
		^self new fromString: str.
	}
	
	method __fromString: str offset: string_offset offset: address_offset
	{
		| dots digits pos size c acc |

		pos := string_offset.
		size := str size.

		acc := 0.
		digits := 0.
		dots := 0.

		do
		{
			if (pos >= size)
			{
				if (dots < 3 or: [digits == 0]) { ^Error.Code.EINVAL }.
				self basicAt: (dots + address_offset) put: acc.
				break.
			}.
			
			c := str at: pos.
			pos := pos + 1.

			if (c >= $0 and: [c <= $9])
			{
				acc := acc * 10 + (c asInteger - $0 asInteger).
				if (acc > 255) { Exception signal: ('invalid IPv4 address B ' & str). }.
				digits := digits + 1.
			}
			elsif (c = $.)
			{
				if (dots >= 3 or: [digits == 0]) { ^Error.Code.EINVAL }.
				self basicAt: (dots + address_offset) put: acc.
				dots := dots + 1.
				acc := 0.
				digits := 0.
			}
			else
			{
				^Error.Code.EINVAL
				### goto @label@.
			}.
		}
		while (true).
		
		^self.
(*
	(@label@)
		Exception signal: ('invalid IPv4 address ' & str).
*)
	}
	
	method fromString: str
	{
		if ((self __fromString: str offset: 0 offset: 0) isError)
		{
			Exception signal: ('invalid IPv4 address ' & str).
		}
	}
}

class(#byte(16)) IP6Address(IP4Address)
{
	(*method(#class) new
	{
		^self basicNew: 16.
	}*)

	##method(#class) fromString: str
	##{
	##	^self new fromString: str.
	##}

	method __fromString: str
	{
		| pos size mysize ch tgpos v1 val curseg saw_xdigit colonpos |

		pos := 0.
		size := str size.
		mysize := self basicSize.

		## handle leading :: specially 
		if (size > 0 and: [(str at: pos) == $:])
		{
			pos := pos + 1.
			if (pos >= size or: [ (str at: pos) ~~ $:]) { ^Error.Code.EINVAL }.
		}.

		tgpos := 0.
		curseg := pos.
		val := 0.
		saw_xdigit := false.
		colonpos := -1.

		while (pos < size)
		{
			ch := str at: pos.
			pos := pos + 1.

			v1 := ch digitValue.
			if (v1 >= 0 and: [v1 <= 15])
			{
				val := (val bitShift: 4) bitOr: v1.
				if (val > 16rFFFF) { ^Error.Code.EINVAL }.
				saw_xdigit := true.
				continue.
			}.

			if (ch == $:)
			{
				curseg := pos.
				if (saw_xdigit not)
				{
					## no multiple double colons are allowed
					if (colonpos >= 0) { ^Error.Code.EINVAL }. 

					## capture the target position when the double colons 
					## are encountered.
					colonpos := tgpos.
					continue.
				}
				elsif (pos >= size)
				{
					## a colon can't be the last character
					^Error.Code.EINVAL
				}.

				self basicAt: tgpos put: ((val bitShift: -8) bitAnd: 16rFF).
				tgpos := tgpos + 1.
				self basicAt: tgpos put: (val bitAnd: 16rFF).
				tgpos := tgpos + 1.

				saw_xdigit := false.
				val := 0.
				continue.
			}.

			if (ch == $. and: [tgpos + 4 <= mysize])
			{
				##if ((super __fromString: (str copyFrom: curseg) offset:0  offset: tgpos) isError) { ^Error.Code.EINVAL }.
				if ((super __fromString: str offset: curseg offset: tgpos) isError) { ^Error.Code.EINVAL }.
				tgpos := tgpos + 4.
				saw_xdigit := false.
				break.
			}.

			## invalid character in the address
			^Error.Code.EINVAL.
		}.

		if (saw_xdigit)
		{
			self basicAt: tgpos put: ((val bitShift: -8) bitAnd: 16rFF).
			tgpos := tgpos + 1.
			self basicAt: tgpos put: (val bitAnd: 16rFF).
			tgpos := tgpos + 1.
		}.

		if (colonpos >= 0)
		{
			## double colon position 
			self basicShiftFrom: colonpos to: (colonpos + (mysize - tgpos)) count: (tgpos - colonpos).
			##tgpos := tgpos + (mysize - tgpos).
		}
		elsif (tgpos ~~ mysize) 
		{
			^Error.Code.EINVAL 
		}.
	}

	method fromString: str
	{
		if ((self __fromString: str) isError)
		{
			Exception signal: ('invalid IPv6 address ' & str).
		}
	}

	##method toString
	##{
	##}
}

class(#byte) SocketAddress(Object) from 'sck.addr'
{
	method(#primitive) family.
	method(#primitive) fromString: str.

	method(#class) fromString: str
	{
		^self new fromString: str
	}
}

class Socket(Object) from 'sck'
{
	## the handle must be the first field in this object to match
	## the internal representation used by various modules. (e.g. sck)
	var(#get) handle := -1.

	method(#primitive) open(family, type, proto).
	## map the open primitive again with a different name for strict internal use only.
	## this method is supposed to be used to handle an accepted socket in server sockets.
	method(#primitive) __open(handle). 

	method(#primitive) _close.
	method(#primitive) bind: addr.
	method(#primitive) _listen: backlog.
	method(#primitive) _accept: addr.
	method(#primitive) _connect: addr.
	method(#primitive) _socketError.

	method(#primitive) _readBytes: bytes.
	method(#primitive) _readBytes: bytes offset: offset length: length.
	method(#primitive) _writeBytes: bytes.
	method(#primitive) _writeBytes: bytes offset: offset length: length.

	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }

	method(#class) __with: handle
	{
		###self addToBeFinalized.
		^(self _basicNew initialize) __open(handle)
	}

	method(#class) family: family type: type
	{
		###self addToBeFinalized.

		## new is prohibited. so use _basicNew with initialize.
		##^(self new) open(family, type, 0)
		^(self _basicNew initialize) open(family, type, 0)
	}

	method close
	{
('CLOSING Socket HANDLE ' & (self.handle asString)) dump.
		if (self.handle >= 0)
		{
('REALLY CLOSING Socket HANDLE ' & (self.handle asString)) dump.
			self _close.
			self.handle := -1.
			self onSocketClosed.
		}.
	}

	method onSocketClosed
	{
		## do nothing.
	}
}

(* TODO: generate these family and type from the C header *)
pooldic Socket.Family
{
	INET := 2.
	INET6 := 10.
}

pooldic Socket.Type
{
	STREAM := 1.
	DGRAM  := 2.
}

class SyncSocket(Socket)
{
	var iosem, tmoutsem, sg.
	var tmoutsecs, tmoutnsecs.

	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }

	method(#class) __with: handle
	{
		###self addToBeFinalized.
		^(self _basicNew initialize) __open(handle)
	}

	method(#class) family: family type: type
	{
		###self addToBeFinalized.

		## new is prohibited. so use _basicNew with initialize.
		##^(self new) open(family, type, 0)
		^(self _basicNew initialize) open(family, type, 0)
	}

	method initialize
	{
		super initialize.

		self.iosem := Semaphore new.
		self.tmoutsem := Semaphore new.

		self.sg := SemaphoreGroup new.
		self.sg addSemaphore: self.iosem.
		self.sg addSemaphore: self.tmoutsem.
	}

	method beWatched
	{
		## do nothing. i don't want to be watched.
	}

	method timeout: secs
	{
		self.tmoutsecs := secs.
		self.tmoutnsecs := 0.
	}

	method __wait_for_input
	{
		| s |
		if (self.tmoutsecs notNil) { self.tmoutsem signalAfterSecs: self.tmoutsecs nanosecs: self.tmoutnsecs }.
		self.iosem signalOnInput: self.handle.
		s := self.sg wait.
		self.iosem unsignal.
		if (self.tmoutsecs notNil) { self.tmoutsem unsignal }.
		if (s == self.tmoutsem) { Exception signal: 'timed out' }.
	}

	method __wait_for_output
	{
		| s |
		if (self.tmoutsecs notNil) { self.tmoutsem signalAfterSecs: self.tmoutsecs nanosecs: self.tmoutnsecs }.
		self.iosem signalOnOutput: self.handle.
		s := self.sg wait.
		self.iosem unsignal.
		if (self.tmoutsecs notNil) { self.tmoutsem unsignal }.
		if (s == self.tmoutsem) { Exception signal: 'timed out' }.
	}

	method connect: addr
	{
		| soerr |

		## an exception is thrown upon exception failure.
		if ((super _connect: addr) <= -1)
		{
			## connection in progress
			while (true)
			{
				self __wait_for_output.
				soerr := self _socketError.
				if (soerr == 0) { break }
				elsif (soerr > 0)
				{
					Exception signal: ('unable to connect - error ' & soerr asString).
				}.
			}.
		}.
	}

	method readBytes: bytes
	{
		| n |
		while (true)
		{
			n := super _readBytes: bytes.
			if (n >= 0) { ^n }.
			self __wait_for_input.
		}
	}

	method readBytes: bytes offset: offset length: length
	{
		| n |
		while (true)
		{
			n := super _readBytes: bytes offset: offset length: length.
			if (n >= 0) { ^n }.
			self __wait_for_input.
		}
	}

	method writeBytes: bytes
	{
		| n |
		while (true)
		{
			n := super _writeBytes: bytes.
			if (n >= 0) { ^n }.
			self __wait_for_output.
		}
	}

	method writeBytes: bytes offset: offset length: length
	{
		| n |
		while (true)
		{
			n := super _writeBytes: bytes offset: offset length: length.
			if (n >= 0) { ^n }.
			self __wait_for_output.
		}
	}
}

class AsyncSocket(Socket)
{
	var pending_bytes, pending_offset, pending_length.
	var outreadysem, outdonesem, inreadysem.

	(* -------------------
	socket call-back methods
	  socketClosing
	  socketClosed
	  socketDataIn
	  socketDataOut
	  socketAccepted:from:
	  socketConnected:
	-------------------- *)

	method initialize
	{
		super initialize.

		self.outdonesem := Semaphore new.
		self.outreadysem := Semaphore new.
		self.inreadysem := Semaphore new.

		self.outdonesem signalAction: [ :sem |
			self onSocketDataOut.
			self.outreadysem unsignal.
		].

		self.outreadysem signalAction: [ :sem |
			| nbytes pos rem |

			pos := self.pending_offset.
			rem := self.pending_length.

			while (rem > 0)
			{
				nbytes := self _writeBytes: self.pending_bytes offset: pos length: rem.
				if (nbytes <= -1) { break }.
				pos := pos + nbytes.
				rem := rem - nbytes.
			}.

			if (rem <= 0)
			{
				self.pending_bytes := nil.
				self.pending_offset := 0.
				self.pending_length := 0.
				self.outdonesem signal.
			}.
		].

		self.inreadysem signalAction: [ :sem |
			self onSocketDataIn.
		].
	}


(*
	method finalize
	{
'SOCKET FINALIZED...............' dump.
		self close.
	}
*)

	method close
	{
		if (self.outdonesem notNil)
		{
			self.outdonesem unsignal.
			if (self.outdonesem _group notNil) { thisProcess removeAsyncSemaphore: self.outdonesem }.
			self.outdonesem := nil.
		}.
		
		if (self.outreadysem notNil)
		{
			self.outreadysem unsignal.
			if (self.outreadysem _group notNil) { thisProcess removeAsyncSemaphore: self.outreadysem }.
			self.outreadysem := nil.
		}.

		if (self.inreadysem notNil)
		{
			self.inreadysem unsignal.
			if (self.inreadysem _group notNil) { thisProcess removeAsyncSemaphore: self.inreadysem }.
			self.inreadysem := nil.
		}.

		^super close.
	}

	method beWatched
	{
		thisProcess addAsyncSemaphore: self.inreadysem.
		self.inreadysem signalOnInput: self.handle.
		thisProcess addAsyncSemaphore: self.outdonesem.
	}

	method readBytes: bytes
	{
		^super _readBytes: bytes.
	}

	method readBytes: bytes offset: offset length: length
	{
		^super _readBytes: bytes offset: offset length: length.
	}

	method writeBytes: bytes offset: offset length: length
	{
		| n pos rem |

		if (self.outreadysem _group notNil)
		{
			Exception signal: 'Not allowed to write again'.
		}.

		## n >= 0: written
		## n <= -1: tolerable error (e.g. EAGAIN)
		## exception: fatal error

		pos := offset.
		rem := length.

		while (rem > 0) ## TODO: loop to write as much as possible
		{
			n := self _writeBytes: bytes offset: pos length: rem.
			if (n <= -1)  { break }.
			rem := rem - n.
			pos := pos + n.
		}.

		if (rem <= 0)
		{
			self.outdonesem signal.
			^length
		}.

		self.pending_bytes := bytes.
		self.pending_offset := pos.
		self.pending_length := rem.

		thisProcess addAsyncSemaphore: self.outreadysem.
		self.outreadysem signalOnOutput: self.handle.
	}

	method writeBytes: bytes
	{
		^self writeBytes: bytes offset: 0 length: (bytes size)
	}


	##method onSocketClosed
	##{
	##}

	method onSocketDataIn
	{
	}

	method onSocketDataOut
	{
	}
}

class AsyncClientSocket(AsyncSocket)
{
	var(#get) connectedEventAction.
	var connsem.

	method initialize
	{
		super initialize.

		self.connsem := Semaphore new.

		self.connsem signalAction: [ :sem |
			| soerr |
			soerr := self _socketError.
			if (soerr >= 0) 
			{
				## finalize connection if not in progress
				sem unsignal.
				thisProcess removeAsyncSemaphore: sem.

				self onSocketConnected: (soerr == 0).
				if (soerr == 0) { self beWatched }.
			}.
			(* HOW TO HANDLE TIMEOUT? *)
		].
	}

	method close
	{
		if (self.connsem notNil)
		{
			self.connsem unsignal.
			if (self.connsem _group notNil) { thisProcess removeAsyncSemaphore: self.connsem }.
			self.connsem := nil.
		}.
		^super close
	}

	method connect: target
	{
		| sem |
		if ((self _connect: target) <= -1)
		{
			thisProcess addAsyncSemaphore: self.connsem.
			self.connsem signalOnOutput: self.handle.
		}
		else
		{
			## connected immediately
'IMMEDIATELY CONNECTED.....' dump.
			self onSocketConnected: true.

			thisProcess addAsyncSemaphore: self.inreadysem.
			self.inreadysem signalOnInput: self.handle.
			thisProcess addAsyncSemaphore: self.outdonesem.
		}
	}

	method onSocketConnected
	{
		## do nothing special. the subclass may override this method.
	}
}

class AsyncServerSocket(AsyncSocket)
{
	method initialize
	{
'Server Socket initialize...........' dump.
		super initialize.

		self.inreadysem signalAction: [ :sem |
			| cliaddr clisck cliact fd |
			cliaddr := SocketAddress new.

			fd := self _accept: cliaddr.
			##if (fd >= 0)
			if (fd notNil)
			{
				clisck := (self acceptedSocketClass) __with: fd.
				clisck beWatched.
				self onSocketAccepted: clisck from: cliaddr.
			}.
		].
	}

	method close
	{
'CLOSING SERVER SOCEKT.... ' dump.
		if (self.inreadysem notNil)
		{
			self.inreadysem unsignal.
			if (self.inreadysem _group notNil) { thisProcess removeAsyncSemaphore: self.inreadysem }.
			self.inreadysem := nil.
		}.

		^super close.
	}

	method listen: backlog
	{
		| n |

		## If listen is called before the socket handle is
		## added to the multiplexer, a spurious hangup event might
		## be generated. At least, such behavior was observed
		## in linux with epoll in the level trigger mode.
		##    self.inreadysem signalOnInput: self.handle.
		##    thisProcess addAsyncSemaphore: self.inreadysem.
		##    self _listen: backlog.

		n := self _listen: backlog.
		self.inreadysem signalOnInput: self.handle.
		thisProcess addAsyncSemaphore: self.inreadysem.
		^n.
	}

	method onSocketAccepted: clisck from: cliaddr
	{
		## close the accepted client socket immediately.
		## a subclass must override this to avoid it.
		clisck close.
	}

	method acceptedSocketClass
	{
		^Socket
	}
}
