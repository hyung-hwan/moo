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
	##method(#primitive) family.
	method(#primitive) fromString: str.
	
	method(#class) fromString: str
	{
		^self new fromString: str
	}
}

class AsyncHandle(Object)
{
	## the handle must be the first field in this object to match
	## the internal representation used by various modules. (e.g. sck)
	var(#get) handle := -1.
	var outsem := nil.

	##method initialize
	##{
	##	^super initialize
	##}

	method close
	{
		if (self.handle >= 0)
		{
			###if (self.insem notNil) 
			###{
			###	System unsignal: self.insem;
			###	       removeAsyncSemaphore: self.insem.
			###	self.insem := nil.
			###}.
			if (self.outsem notNil)
			{
				System unsignal: self.outsem;
				       removeAsyncSemaphore: self.outsem.
				self.outsem := nil.
			}.

			self _close.
			self.handle := -1.
		}
	}

	method writeBytes: bytes signal: sem
	{
		^self writeBytes: bytes offset: 0 length: (bytes size)
	}

	method writeBytes: bytes offset: offset length: length
	{
		^self writeBytes: bytes offset: offset length: length.
	}

	method writeBytes: bytes
	{
		^self writeBytes: bytes offset: 0 length: (bytes size)
	}
}

class Socket(AsyncHandle) from 'sck'
{
	var eventActions.
	var pending_bytes, pending_offset, pending_length.
	var outreadysem, outdonesem, inreadysem.

	method(#primitive) _open(domain, type, proto).
	method(#primitive) _close.
	method(#primitive) bind: addr.
	method(#primitive) _listen: backlog.
	method(#primitive) accept: addr.
	method(#primitive) _connect: addr.
	method(#primitive) _socketError.

	method(#primitive) readBytes: bytes.
	method(#primitive) _writeBytes: bytes.
	method(#primitive) _writeBytes: bytes offset: offset length: length.
}

(* TODO: generate these domain and type from the C header *)
pooldic Socket.Domain
{
	INET := 2.
	INET6 := 10.
}

pooldic Socket.Type
{
	STREAM := 1.
	DGRAM  := 2.
}

pooldic Socket.EventType
{
	CONNECTED := 0.
	DATA_IN := 1.
	DATA_OUT := 2.
}

extend Socket
{
	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }

	method(#class) domain: domain type: type
	{
		^(super new) open(domain, type, 0)
	}

	method initialize
	{
		super initialize.
		self.eventActions := #(nil nil nil).

		self.outdonesem := Semaphore new.
		self.outreadysem := Semaphore new.
		self.inreadysem := Semaphore new.

		self.outdonesem signalAction: [ :xsem |
			(self.eventActions at: Socket.EventType.DATA_OUT) value: self.
			System unsignal: self.outreadysem.
		].

		self.outreadysem signalAction: [ :xsem |
			| nwritten |
			nwritten := self _writeBytes: self.pending_bytes offset: self.pending_offset length: self.pending_length.
			if (nwritten >= 0)
			{
				self.pending_bytes := nil.
				self.pending_offset := 0.
				self.pending_length := 0.
				self.outdonesem signal.
			}
		].

		self.inreadysem signalAction: [ :ysem |
			(self.eventActions at: Socket.EventType.DATA_IN) value: self.
		].
	}

	method open(domain, type, proto)
	{
		| sck |
		sck := self _open(domain, type, proto).

		if (self.handle >= 0)
		{
			System addAsyncSemaphore: self.outdonesem.
			System addAsyncSemaphore: self.outreadysem.
		}.

		^sck
	}

	method close
	{
'Socket close' dump.
		System removeAsyncSemaphore: self.outdonesem.
		System removeAsyncSemaphore: self.outreadysem.
		^super close.
	}
###	method listen: backlog do: acceptBlock
###	{
###		self.inputAction := acceptBlock.
###		self watchInput.
###		^self _listen: backlog.
###	}

	method onEvent: event_type do: action_block
	{
		self.eventActions at: event_type put: action_block.
	}

	method connect: target
	{
		| sem |
		if ((self _connect: target) <= -1)
		{
			sem := Semaphore new.
			sem signalAction: [ :xsem |
				| soerr dra |
				soerr := self _socketError.
				if (soerr >= 0) 
				{
					## finalize connection if not in progress
'CHECKING FOR CONNECTION.....' dump.
					System unsignal: xsem.
					System removeAsyncSemaphore: xsem.

					(self.eventActions at: Socket.EventType.CONNECTED) value: self value: (soerr == 0).

					if (soerr == 0)
					{
						if ((self.eventActions at: Socket.EventType.DATA_IN) notNil)
						{
							xsem signalAction: [ :ysem |
								(self.eventActions at: Socket.EventType.DATA_IN) value: self.
							].
							System addAsyncSemaphore: xsem.
							System signal: xsem onInput: self.handle.
						}.
					}.
				}.
				(* HOW TO HANDLE TIMEOUT? *)
			].

			System addAsyncSemaphore: sem.
			System signal: sem onOutput: self.handle.
		}
		else
		{
			## connected immediately
'IMMEDIATELY CONNECTED.....' dump.
			(self.eventActions at: Socket.EventType.CONNECTED) value: self value: true.
			if ((self.eventActions at: Socket.EventType.DATA_IN) notNil)
			{
				sem := Semaphore new.
				sem signalAction: [ :xsem |
					(self.eventActions at: Socket.EventType.DATA_IN) value: self.
				].
				System addAsyncSemaphore: sem.
				System signal: sem onInput: self.handle.
			}.
		}
	}

	method writeBytes: bytes offset: offset length: length
	{
		| n |

		## n >= 0: written
		## n <= -1: tolerable error (e.g. EAGAIN)
		## exception: fatal error
		##while (true) ## TODO: loop to write as much as possible
		##{
			n := self _writeBytes: bytes offset: offset length: length.
			if (n >= 0) 
			{ 
				self.outdonesem signal.
				^n 
			}.
		##}.

		## TODO: adjust offset and length 
		self.pending_bytes := bytes.
		self.pending_offset := offset.
		self.pending_length := length.

		System signal: self.outreadysem onOutput: self.handle.
	}

}

class MyObject(Object)
{
	method(#class) main
	{
		[
			| s s2 st sg ss buf count |

			count := 0.
			[
				buf := ByteArray new: 128.
				s := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
				
				s onEvent: Socket.EventType.CONNECTED do: [ :sck :state |
					if (state)
					{
						'AAAAAAAA' dump.
						s writeBytes: #[ $a, $b, $c ] 
					}
					else
					{
						'FAILED TO CONNECT' dump.
					}.
				].
				s onEvent: Socket.EventType.DATA_IN do: [ :sck |
					| nbytes |
					nbytes := s readBytes: buf. 
					if (nbytes == 0)
					{
						sck close
					}.
					('Got ' & (nbytes asString)) dump.
					buf dump.
				].
				s onEvent: Socket.EventType.DATA_OUT do: [ :sck |
					if (count < 10) { s writeBytes: #[ $a, $b, C'\n' ]. count := count + 1. }.
				].

				s connect: (SocketAddress fromString: '127.0.0.1:9999').

				while (true)
				{
					ss := System handleAsyncEvent.
					if (ss isError) { break }.
					###if (ss == st) { System removeAsyncSemaphore: st }.
				}.
			]
			ensure:
			[
				if (s notNil) { s close }.
				if (s2 notNil) { s2 close }.
			]

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump ].

		'----- END OF MAIN ------' dump.
	}
}

