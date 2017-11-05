#include 'Moo.moo'.

class Socket(Object) from 'sck'
{
	var handle := -1.

	method(#primitive) _open(domain, type, proto).
	method(#primitive) _close.

	method(#primitive) _connect(a,b,c).
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

	method(#class) _domain: domain _type: type
	{
		^super new _open(domain, type, 0).
	}

	method(#class) domain: domain type: type
	{
		| s |
		s := super new _open(domain, type, 0).
		if (s isError) { Exception signal: 'Cannot open socket' }.
		^s.
	}

	method close
	{
		if (self.handle >= 0)
		{
			## this primitive method may return failure. 
			## but ignore it in this normal method.
			self _close.
			self.handle := -1.
		}
	}

	method asyncConnect: connectBlock
	{
		| s1 s2 sg sa |

		s1 := Semaphore new.
		s2 := Semaphore new.

		sa := [:sem | 
			System unsignal: s1.
			System unsignal: s2.
			System removeAsyncSemaphore: s1.
			System removeAsyncSemaphore: s2.
			connectBlock value: (sem == s1)
		].

		s1 signalAction: sa.
		s2 signalAction: sa.

## TODO: unsignal s1 s2, remove them from System when exception occurs.
		System signal: s1 onOutput: self.handle.
		System signal: s2 after: 10.

		System addAsyncSemaphore: s1.
		System addAsyncSemaphore: s2.

		if (self _connect(1, 2, 3) isError)
		{
			Exception signal: 'Cannot connect to 1,2,3'.
		}.
	}

	method asyncRead: readBlock
	{
		| s1 s2 | 
		s1 := Semaphore new.
		s2 := Semaphore new.

		s1 signalAction: [:sem | readBlock value: true].
		s2 signalAction: [:sem | readBlock value: false].

		System signal: s1 onInput: self.handle.
		System signal: s2 after: 10.
	}

(*
	method asyncWrite: 
	{
		| s1 s2 | 
		s1 := Semaphore new.
		s2 := Semaphore new.

		s1 signalAction: [:sem | writeBlock value: true].
		s2 signalAction: [:sem | writeBlock value: false].

		System signal: s1 onOutput: self.handle.
		System signal: s2 after: 10.
	}
*)

}


class MyObject(Object)
{
	method(#class) main
	{
		| s |
		[
			s := Socket domain: Socket.Domain.INET type: Socket.Type.STREAM.
			s dump.

			s asyncConnect: [:result | 
				##s endConnect: result.
				##s beginRead: xxx.
				'CONNECTED NOW.............' dump.
			].

			while (true)
			{
				System handleAsyncEvent.
			}.
			s close dump.
		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump ].

		'----- END OF MAIN ------' dump.
	}
}

