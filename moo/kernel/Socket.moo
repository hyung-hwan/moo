#include 'Moo.moo'.

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

	method watchInput
	{
		if (self.insem isNil)
		{
			self.insem := Semaphore new.
			self.insem signalAction: [:sem | self.inputAction value: self value: true].
			System addAsyncSemaphore: self.insem;       
		}.

		System signal: self.insem onInput: self.handle.

		###s2 := Semaphore new.
		###s2 signalAction: [:sem | inputActionBlock value: self value: false].
		###System signal: s2 afterSecs: 10.
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

		inact := [:sck :state |
			| data n |

			data := ByteArray new: 100.
			n := sck readBytes: data.
			if (n == 0)
			{
				sck close.
			}
			else
			{
				(n asString & ' bytes read') dump.
				data dump.
			}.
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

