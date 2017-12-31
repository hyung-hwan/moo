#include 'Moo.moo'.

##interface IPAddressInterface
##{
##}
## class XXX(Object,IPAddressInterface) {}

class(#byte) IPAddress(Object)
{
}

### TODO: extend the compiler
###class(#byte(4)) IP4Address(IPAddress) -> new basicNew always create 4 bytes. size to new: or basicNew: is ignored.
###class(#byte(16)) IP6Address(IPAddress)

class(#byte) IP4Address(IPAddress)
{
	method(#class) new
	{
		^self basicNew: 4.
	}
	
	method(#class) fromString: str
	{
		^self new fromString: str.
	}
	
	method fromString: str
	{
		| dots digits pos size c acc |

		pos := 0.
		size := str size.
		
		acc := 0.
		digits := 0.
		dots := 0.

		do
		{
			if (pos >= size)
			{
				if (dots < 3 or: [digits == 0]) 
				{
					Exception signal: ('invalid IPv4 address A ' & str).
				}.
				self basicAt: dots put: acc.
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
				if (dots >= 3 or: [digits == 0]) { Exception signal: ('invalid IPv4 address C ' & str). }.
				self basicAt: dots put: acc.
				dots := dots + 1.
				acc := 0.
				digits := 0.
			}
			else
			{
				Exception signal: ('invalid IPv4 address ' & str).
			}.
			
			
		}
		while (true).
	}
}

class(#byte) IP6Address(IPAddress)
{
	method(#class) new
	{
		^self basicNew: 16.
	}
	
	method(#class) fromString: str
	{
	
	}
}

class(#byte) IP4SocketAddress(IP4Address)
{
	method(#class) new
	{
		^self basicNew: 6.
	}
}

class SocketAddress(Object) from 'sck.addr'
{
	##method(#primitive) family.
	method(#primitive) fromString: str.
	
	method(#class) fromString: str
	{
		^self new fromString: str
	}
}


##class InetSocketAddress(SocketAddress)
##{
##}

class Socket(Object) from 'sck'
{
	var handle := -1.
	var insem, outsem.
	var(#get,#set) inputAction, outputAction.

	method(#primitive) open(domain, type, proto).
	method(#primitive) _close.
	method(#primitive) _connect(a,b,c).
	method(#primitive) endConnect.

	method(#primitive) readBytes: bytes.
	method(#primitive) writeBytes: bytes.
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

extend Socket
{
	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }

	method(#class) domain: domain type: type
	{
		^super new open(domain, type, 0).
	}

	method close
	{
		if (self.handle >= 0)
		{
			## this primitive method may return failure. 
			## but ignore it here.
			if (self.insem notNil) 
			{ 
				System unsignal: self.insem;
				       removeAsyncSemaphore: self.insem.
				self.insem := nil.
			}.
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

	method asyncConnect: connectBlock
	{
		| s1 s2 sa |

		s1 := Semaphore new.
		s2 := Semaphore new.

		sa := [:sem | 
'UNSIGNALLLING      ...........' dump.
			System unsignal: s1;
			       unsignal: s2;
			       removeAsyncSemaphore: s1;
			       removeAsyncSemaphore: s2.

'FINALIZING CONNECT' dump.
			self endConnect.
			connectBlock value: self value: (sem == s1)
		].

		s1 signalAction: sa.
		s2 signalAction: sa.

		[
			System signal: s1 onOutput: self.handle;
			       signal: s2 afterSecs: 10;
			       addAsyncSemaphore: s1;
			       addAsyncSemaphore: s2.
			self _connect(1, 2, 3).
		] ifCurtailed: [
			## rollback 
			sa value: s2.
		]
	}

## TODO: how to specify a timeout for an action? using another semaphore??

	method watchInput
	{
		if (self.insem isNil)
		{
			self.insem := Semaphore new.
			self.insem signalAction: [:sem | self.inputAction value: self value: true].
			System addAsyncSemaphore: self.insem;       
		}.

		System signal: self.insem onInput: self.handle.
	}

	method unwatchInput
	{
		if (self.insem notNil) { System unsignal: self.insem }.
	}

	method watchOutput
	{
		if (self.outsem isNil)
		{
			self.outsem := Semaphore new.
			self.outsem signalAction: [:sem | self.outputAction value: self value: true].
			System addAsyncSemaphore: self.outsem.
		}.

		System signal: self.outsem onOutput: self.handle.
	}

	method unwatchOutput
	{
		if (self.outsem notNil) { System unsignal: self.outsem }.
	}
}


class MyObject(Object)
{
	method(#class) main
	{
		| s conact inact outact |


s := IP4Address fromString: '192.168.123.232'.
s dump.
s basicSize dump.
##s := IP6Address new.
##s dump.
##s := IP4SocketAddress new.
##s dump.
thisProcess terminate.
		inact := [:sck :state |
			| data n |
(*
end of data -> 0.
no data -> -1.
has data -> 1 or moreailure indicated by primitive function 0x55a6210 - _integer_add

error -> exception
*)

			data := ByteArray new: 100.
			do
			{
				n := sck readBytes: data.
				if (n <= 0)
				{
					if (n == 0) { sck close }.
					break.
				}
				elsif (n > 0)
				{
					(n asString & ' bytes read') dump.
					data dump.
				}.
			}
			while (true).
		].

		outact := [:sck :state |
			if (state)
			{
				sck writeBytes: #[ $h, $e, $l, $l, $o, C'\n' ].
			}
			else
			{
			}
		].

		conact := [:sck :state |
			if (state)
			{
				'CONNECTED NOW.............' dump.
				s writeBytes: #[ $h, $e, $l, $l, $o, C'\n' ].

				s watchInput.
				s watchOutput.
			}
			else
			{
				'UNABLE TO CONNECT............' dump.
			}
		].

		## ------------------------------------------------------

		[
			s := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
			s inputAction: inact; outputAction: outact.
			s asyncConnect: conact.

			while (true)
			{
				System handleAsyncEvent.
			}.
			s close dump.

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump ].

		'----- END OF MAIN ------' dump.
	}
}

