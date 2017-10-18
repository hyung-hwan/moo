class Socket(Object) from 'sck'
{
	var handle := -1.

	method(#primitive) _open(domain, type, proto).
	method(#primitive) _connect(a,b,c).
	method(#primitive) _close.
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
	method(#class) domain: domain type: type
	{
		^super new _open(domain, type, 0).
	}

	method beginConnect
	{
		(*
		| s1 s1 |
		s1 = Semaphore new.
		s2 = Semaphore new.

		s1 signalHandler: [xxxxx].
		s2 signalHandler: [xxxxxx].

		fcntl (nonblock);

		Processor signal: s1 onOutput: self.handle.
		Processor signal: s2 after: timeout.

		connect ();
		*)
	}
}
