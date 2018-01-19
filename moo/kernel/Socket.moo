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

(**********************************************************
class X(Object)
{
	var tt,yy.
	
	method x {
	self.yy := 9.
	}
}
##class(#byte(3)) Y(X)
class(#pointer) Y(X)
{
}
##class (#word(2)) Z(Y)
##{
##}
##class InetSocketAddress(SocketAddress)
##{
##}
************************************************************)

class Socket(Object) from 'sck'
{
	var(#get) handle := -1.
	var insem, outsem.
	var(#get,#set) inputAction, outputAction.

	method(#primitive) open(domain, type, proto).
	method(#primitive) _close.
	method(#primitive) bind: addr.
	method(#primitive) listen: backlog.
	method(#primitive) accept: addr.
	method(#primitive) _connect: addr.
	method(#primitive) _socketError.

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

	method connectTo: target do: connectBlock
	{
		| s1 s2 sa |

		s1 := Semaphore new.
		s2 := Semaphore new.

		sa := [:sem | 
		
			| connected |

			connected := false.
			System unsignal: s1;
			       unsignal: s2;
			       removeAsyncSemaphore: s1;
			       removeAsyncSemaphore: s2.

			if (sem == s1)
			{
				[ connected := (self _socketError == 0) ] ifCurtailed: [ connected := false ].
			}.

			connectBlock value: self value: connected.
		].

		s1 signalAction: sa.
		s2 signalAction: sa.

		[
			System signal: s1 onOutput: self.handle;
			       signal: s2 afterSecs: 10;
			       addAsyncSemaphore: s1;
			       addAsyncSemaphore: s2.
			self _connect: target.
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
			System addAsyncSemaphore: self.insem.
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
		| conact inact outact accact |


(SocketAddress fromString: '192.168.123.232:99') dump.
'****************************' dump.

(*
s:= X new: 20.
s basicSize dump.
'****************************' dump.

s := Y new: 10.
s x.
s basicAt: 1 put: 20.
s dump.
s basicSize dump.
'****************************' dump.
*)

(***********************************
s := ByteArray new: 100.
s basicFillFrom: 0 with: ($a asInteger) count: 100.
s basicFillFrom: 50 with: ($b asInteger) count: 50.
(s basicShiftFrom: 50 to: 94 count: 10) dump.
s dump.
##thisProcess terminate.

s := IP4Address fromString: '192.168.123.232'.
s dump.
s basicSize dump.

s := IP6Address fromString: 'fe80::c225:e9ff:fe47:99.2.3.4'.
##s := IP6Address fromString: '::99.12.34.54'.
##s := IP6Address fromString: '::FFFF:0:0'.
##s := IP6Address fromString: 'fe80::'.
s dump.
s basicSize dump.

s := IP6Address fromString: 'fe80::c225:e9ff:fe47:b1b6'.
s dump.
s basicSize dump.
##s := IP6Address new.
##s dump.
##s := IP4SocketAddress new.
##s dump.
thisProcess terminate.
**************************)

		inact := [:sck :state |
			| data n |
(*
end of data -> 0.
no data -> -1. (e.g. EINPROGRESS)
has data -> 1 or more
error -> exception
*)

			data := ByteArray new: 100.
			do
			{
				n := sck readBytes: data.
				if (n <= 0)
				{
					if (n == 0) { sck close }. ## end of data
					break.
				}
				elsif (n > 0)
				{
					(n asString & ' bytes read') dump.
					data dump.
				}.

				sck writeBytes: #[ $h, $e, $l, $l, $o, $., $., $., C'\n' ].
			}
			while (true).
		].

		outact := [:sck :state |
			if (state)
			{
				[ sck writeBytes: #[ $h, $e, $l, $l, $o, C'\n' ] ] 
					on: Exception do: [:ex | sck close. ].
			}
			else
			{
			}
		].

		conact := [:sck :state |
			if (state)
			{
				'CONNECTED NOW.............' dump.
				sck writeBytes: #[ $h, $e, $l, $l, $o, $w, $o, C'\n' ].
				sck watchInput; watchOutput.

			}
			else
			{
				'UNABLE TO CONNECT............' dump.
			}
		].

		## ------------------------------------------------------
		accact := [:sck :state |
			| newsck newaddr |

			newaddr := SocketAddress new.
			newsck := sck accept: newaddr.

			System log: 'new connection - '; log: newaddr; log: ' '; log: (newsck handle); logNl.

			newsck inputAction: inact; outputAction: outact.
			newsck watchInput; watchOutput.
		].

		[
			| s s2 |
			[
				s := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
				s inputAction: inact; outputAction: outact.
				s connectTo: (SocketAddress fromString: '127.0.0.1:9999') do: conact.

				s2 := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
				s2 inputAction: accact.
				s2 bind: (SocketAddress fromString: '0.0.0.0:9998').
				s2 listen: 10; watchInput.

				while (true)
				{
					System handleAsyncEvent.
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

