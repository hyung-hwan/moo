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

	var inwc := 0, outwc := 0. ## input watcher count and ouput watcher count
	var insem, outsem.
	var(#get,#set) inputAction, outputAction.
	var(#get) inputReady := false, outputReady := false.

	method close
	{
		if (self.handle >= 0)
		{
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

			self.outwc := 0.
			self.inwc := 0.

			self _close.
			self.handle := -1.
		}
	}

## TODO: how to specify a timeout for an action? using another semaphore??
	method watchInput
	{
		if (self.inwc == 0) 
		{ 
			if (self.insem isNil)
			{
				self.insem := Semaphore new.
				self.insem signalAction: [:sem | 
					self.inputReady := true.
					self.inputAction value: self value: true
				].
				System addAsyncSemaphore: self.insem.
			}.
			self.inputReady := false.
			System signal: self.insem onInput: self.handle 
		}.
		self.inwc := self.inwc + 1.
	}

	method unwatchInput
	{
		if (self.inwc > 0)
		{
			self.inwc := self.inwc - 1.
			if (self.inwc == 0)
			{
				##if (self.insem notNil) { System unsignal: self.insem }.
				System unsignal: self.insem.
				System removeAsyncSemaphore: self.insem.
				self.insem := nil.
			}.
		}.
	}

	method watchOutput
	{
		if (self.outwc == 0)
		{
			if (self.outsem isNil)
			{
				self.outsem := Semaphore new.
				self.outsem signalAction: [:sem | 
					self.outputReady := true.
					self.outputAction value: self value: true 
				].
				System addAsyncSemaphore: self.outsem.
			}.
			self.outputReady := false.
			System signal: self.outsem onOutput: self.handle.
		}.
		self.outwc := self.outwc + 1.
	}


	method unwatchOutput
	{
		if (self.outwc > 0)
		{
			self.outwc := self.outwc - 1.
			if (self.outwc == 0)
			{
				## self.outsem must not be nil here.
				System unsignal: self.outsem.
				System removeAsyncSemaphore: self.outsem.
				self.outsem := nil.
			}.
		}.
	}

	method writeBytes: bytes offset: offset length: length signal: sem
	{
		| oldact n |
#######################################
## TODO: if data still in progress, failure... or success while concatening the message? 
##       for a stream, concatening is not bad. but it's not good if the socket requires message boundary preservation.
######################################

		if (self.outputReady)
		{
			## n >= 0: written
			## n <= -1: tolerable error (e.g. EAGAIN)
			## exception: fatal error
			##while (true) ## TODO: loop to write as much as possible
			##{
				n := self _writeBytes: bytes offset: offset length: length.
				if (n >= 0) 
				{ 
					if (sem notNil) { sem signal }.
					^n 
				}.
			##}.

			self.outputReady := false.
		}.

		oldact := self.outputAction.
		self.outputAction := [ :sck :state |
			##### schedule write.
			if (state)
			{
				if ((self _writeBytes: bytes offset: offset length: length) <= -1) 
				{
					## EAGAIN
					self.outputReady := false.
					^self.
				}.
## TODO: handle _writeBytes may not write in full.
			}.

			self.outputAction := oldact.
			self unwatchOutput.
		].

		## TODO: set timeout?
		self watchOutput.
	}

	method writeBytes: bytes signal: sem
	{
		^self writeBytes: bytes offset: 0 length: (bytes size) signal: sem.
	}

	method writeBytes: bytes offset: offset length: length
	{
		^self writeBytes: bytes offset: offset length: length signal: nil.
	}

	method writeBytes: bytes
	{
		^self writeBytes: bytes offset: 0 length: (bytes size) signal: nil.
	}
}

class Socket(AsyncHandle) from 'sck'
{
	method(#primitive) open(domain, type, proto).
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

extend Socket
{
	method(#class) new { self messageProhibited: #new }
	method(#class) new: size { self messageProhibited: #new: }

	method(#class) domain: domain type: type
	{
		^super new open(domain, type, 0).
	}

	method listen: backlog do: acceptBlock
	{
		self.inputAction := acceptBlock.
		self watchInput.
		^self _listen: backlog.
	}

	method connect: target do: connectBlock
	{
		| conblk oldact |

		if ((self _connect: target) <= -1)
		{
			## connection in progress

			oldact := self.outputAction.
			self.outputAction := [ :sck :state |
				| soerr |

				if (state)
				{
					## i don't map a connection error to an exception.
					## it's not a typical exception. it is a normal failure
					## that is caused by an external system. 
					##
					## or should i map an error to an exception?
					## i can treat EINPROGRESS, ECONNREFUSED as failure.
					## all other errors may get treated as an exception?
					## what about timeout???

					soerr := self _socketError.
					if (soerr >= 0) 
					{
						## finalize connection if not in progress
						self.outputAction := oldact.
						self unwatchOutput.
						if (connectBlock notNil)
						{
							connectBlock value: sck value: (soerr == 0).
						}.
					}.
				}
				else
				{
					## timed out
					self.outputAction := oldact.
					self unwatchOutput.
					if (connectBlock notNil) 
					{
						## TODO: tri-state? success, failure, timeout? or boolean with extra error code
						connectBlock value: sck value: false. 
					}.
				}.
			].

			###self.outputTimeout: 10 do: xxxx.
			self watchOutput.
		}
		else
		{
			## connected immediately.
			if (connectBlock notNil) 
			{
				connectBlock value: self value: true.
			}
		}
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
no data -> -1. (e.g. EAGAIN)
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

					##sck writeBytes: #[ $h, $e, $l, $l, $o, $., $., $., C'\n' ].
					sck writeBytes: data offset: 0 length: n.
				}.
			}
			while (true).
		].

## TODO: what should it accept as block parameter
## socket, output result? , output object?
		outact := [:sck :state |
			if (state)
			{
				## what if i want write more data???
				##[ sck writeBytes: #[ $h, $e, $l, $l, $o, C'\n' ] ] 
				##	on: Exception do: [:ex | sck close. ].
			}
			else
			{
			}
		].

	
		conact := [:sck :state |

			| x write_more count |

			count := 0.
			if (state)
			{
				'CONNECTED NOW.............' dump.
				###sck inputTimeout: 10; outputTimeout: 10; connectTimeout: 10.

#############################################
				write_more := [:sem |
					if (count <= 26)
					{
						sck writeBytes: %[ $h, $e, $l, $l, $o, $-, $m, $o, count + 65, $o, $o, C'\n' ] signal: x.
						count := count + 1.
					}
					else
					{
						System removeAsyncSemaphore: x.
					}.
				].

				x := Semaphore new.
				x signalAction: write_more.
				System addAsyncSemaphore: x.
				x signal.

				##sck outputAction: outact.
				##sck writeBytes: #[ $h, $e, $l, $l, $o, $-, $m, $o, $o, C'\n' ] signal: x.
###############################################

				sck inputAction: inact.
				sck watchInput.
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
			##newsck watchInput; watchOutput.
			newsck watchInput.

			
			newsck writeBytes: #[ $W, $e, $l, $c, $o, $m, $e, $., C'\n' ].
		].



		[
			| s s2 st sg ss |
			[
				s := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
				s connect: (SocketAddress fromString: '127.0.0.1:9999') do: conact.

##				s2 := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
##				s2 bind: (SocketAddress fromString: '0.0.0.0:9998').
##				##s2 inputAction: accact.
##				###s2 listen: 10; watchInput.
##				s2 listen: 10 do: accact.

(*
st := Semaphore new.
System addAsyncSemaphore: st.
System signal: st afterSecs: 5.
'JJJJJJJJJJJ' dump.
sg := SemaphoreGroup new.
'JJJJJJJJJJJ' dump.
sg wait.
'YYYYYYYYYYYYYYY' dump.
*)

###[ while (1) { '1111' dump. System sleepForSecs: 1 } ] fork.

(*
st := Semaphore new.
System addAsyncSemaphore: st.
System signal: st afterSecs: 20.
*)


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

