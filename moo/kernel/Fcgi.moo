###include 'Moo.moo'.
#include 'Socket.moo'.

(* -------------------------------------------

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

 ----------------------------------------------- *)

class Fcgi(Object)
{
}

pooldic Fcgi.Type
{
	BEGIN_REQUEST     := 1.
	ABORT_REQUEST     := 2.
	END_REQUEST       := 3.
	PARAMS            := 4.
	STDIN             := 5.
	STDOUT            := 6.
	STDERR            := 7.
	DATA              := 8.
	GET_VALUES        := 9.
	GET_VALUES_RESULT := 10.
	UNKNOWN_TYPE      := 11.
}

class FcgiConnReg(Object)
{
	var connections.
	var first_free_slot.
	var last_free_slot.

	method initialize
	{
		| i size |
		self.connections := Array new: 32. ## TODO: make it dynamic

		i := self.connections size.
		if (i <= 0) 
		{
			self.first_free_slot := -1.
			self.last_free_slot := -1.
		}
		else
		{
			i := (self.first_free_slot := i - 1).
			while (i >= 0)
			{
				self.connections at: i put: (i - 1).
				i := i - 1.
			}.
			self.last_free_slot := 0.
		}.
	}

	method add: elem
	{
		| index |
		if (self.first_free_slot < 0) { Exception signal: 'no free slot' }.
		index := self.first_free_slot.
		self.first_free_slot := (self.connections at: index).
		self.connections at: index put: elem.
		if (self.first_free_slot < 0) { self.last_free_slot = -1 }.
		^index.
	}

	method remove: index
	{
		if (self.last_free_slot >= 0)
		{
			self.connections at: self.last_free_slot put: index.
		}
		else
		{
			self.first_free_slot := self.last_free_slot.
		}.
		self.connections at: index put: -1.
		self.last_free_slot := index.
	}
	
	method do: block
	{
		| index size conn |
		## the following loop won't evaluate the given block for an element added after 
		## resizing of self.connections at present, there is no self.connections resizing
		## impelemented. so no worry on this.
		size := self.connections size. 
		index := 0.  
		while (index < size)
		{
			conn := self.connections at: index.
			if ((conn isKindOf: Integer) not)
			{
				block value: (self.connections at: index).
			}.
			index := index + 1.
		}.
	}
}

class FcgiSocket(SyncSocket)
{
	var(#get) server := nil.
	var(#get) rid := -1.
	var bs.

	method initialize
	{
		super initialize.
		self.bs := ByteStreamAdapter on: self.
	}

	method close
	{
('Fcgi Connection closing.......... handle ' & self.handle asString) dump.
		if (self.server notNil)
		{
('Fcgi Connection ' & self.rid asString &  ' removing from server  ..........') dump.
			self.server removeConnection: self.
		}.
		^super close.
	}
	
	method server: server rid: rid
	{
		self.server := server.
		self.rid := rid.
	}

	method getLine
	{
		
	}

	method getBytes: size
	{
	}

	method _run_service
	{
		| buf k i hdr |

		self timeout: 10.
		(*while (true)
		{
			req := self readRequest.

		}. *)

		buf := ByteArray new: 128.
'IM RUNNING SERVICE...............' dump.


		hdr := ByteArray new: 8.
		k := self.bs next: 8 into: hdr startingAt: 0.


(*
		i := 0.
		while (i < 3)
		{
			k := self.bs next: 7 into: buf startingAt: 10.
'KKKKKKKKKKKKKKKKKKKK' dump.
			(buf copyFrom: 10 count: k) dump.
			i := i + 1.

			(buf copyFrom: 10 count: k) decodeToCharacter dump.

			self.bs nextPut: k from: buf startingAt: 10.
		}.
		self.bs flush.
*)

		self close.
	}

	method runService
	{
		[ self _run_service ] on: Exception do: [:ex | 
			self close.
			('EXCEPTION IN FcgiSocket ---> ' & ex messageText) dump 
		].
	}
}

class FcgiListener(AsyncServerSocket)
{
	var(#get) server := nil.
	var(#get) rid := nil.

	method initialize
	{
		super initialize.
	}

	method close
	{
		if (self.server notNil) { self.server removeListener: self }.
		^super close.
	}

	method onSocketAccepted: clisck from: cliaddr
	{
		| rid |

'CLIENT accepted ..............' dump.
clisck dump.
cliaddr dump.

		if (self.server notNil)
		{
			[
				server addConnection: clisck.
				if (clisck isKindOf: SyncSocket)
				{
'SERVICE READLLY STARTING' dump.
					[clisck runService] fork.
				}
			]
			on: Exception do: [:ex |
				clisck close.
				Exception signal: ('unable to handle a new connection - ' & ex messageText).
			].
		}.
	}

	method acceptedSocketClass
	{
		##^if (self currentAddress port == 80) { FcgiSocket } else { FcgiSocket }.
		^FcgiSocket.
	}

	method server: server rid: rid
	{
		self.server := server.
		self.rid := rid.
	}
}

class FcgiServer(Object)
{
	var listeners.
	var connreg.

	method initialize
	{
		super initialize.
		self.listeners := FcgiConnReg new.
		self.connreg := FcgiConnReg new.
	}

	method __add_listener: listener
	{
		| rid |
		rid := self.listeners add: listener.
('ADD NEW LISTENER ' & rid asString) dump.
		listener server: self rid: rid.
	}

	method removeListener: listener
	{
		| rid |
		rid := listener rid.
		if (rid notNil)
		{
('REALLY REMOVE LISTENER ' & rid asString) dump.
			self.listeners remove: (listener rid).
			listener server: nil rid: nil.
		}.
	}

	method __add_new_listener: addr
	{
		| listener |
		listener := FcgiListener family: (addr family) type: Socket.Type.STREAM.
		[
			self __add_listener: listener.
			listener bind: addr.
			listener listen: 128.
		] on: Exception do: [:ex |
			listener close.
			## ex pass.
			Exception signal: ('unable to add new listener - ' & ex messageText).
		].
	}

	method addConnection: conn
	{
		| rid |
		rid := self.connreg add: conn.
('ADD NEW CONNECTION ' & rid asString) dump.
		conn server: self rid: rid.
	}
	
	method removeConnection: conn
	{
		| rid |
		rid := conn rid.
		if (rid notNil)
		{
('REMOVE CONNECTION ' & rid asString) dump.
			self.connreg remove: (conn rid).
			conn server: nil rid: nil.
		}.
	}

	method start: laddr
	{
		| listener |
		if (laddr class == Array)
		##if (laddr respondsTo: #do:) ## can i check if the message receives a block and the block accepts 1 argument?
		{
			laddr do: [:addr | self __add_new_listener: addr ].
		}
		else
		{
			self __add_new_listener: laddr.
		}.
	}

	method close
	{
		self.listeners do: [:listener |
			listener close.
		].

		self.connreg do: [:conn |
			conn close.
		].
	}
}

class MyObject(Object)
{
	method(#class) start_server_socket
	{
		| s2 buf |
		s2 := AsyncServerSocket family: Socket.Family.INET type: Socket.Type.STREAM.
		buf := ByteArray new: 128.

		s2 onEvent: #accepted do: [ :sck :clisck :cliaddr |
'SERVER ACCEPTED new client' dump.
			clisck onEvent: #data_in do: [ :csck |
				| nbytes |
				while (true)
				{
					nbytes := csck readBytesInto: buf. 
					if (nbytes <= 0)
					{
						if (nbytes == 0) { csck close }.
						('Got ' & (nbytes asString)) dump.
						break.
					}.

					buf dump.
					csck writeBytesFrom: buf offset: 0 length: nbytes.
				}.
			].
			clisck onEvent: #data_out do: [ :csck |
				##csck writeBytesFrom: #[ $a, $b, C'\n' ].
			].
			clisck onEvent: #closed do: [ :csck |
				'Socket CLOSED....' dump.
			].
		].

		s2 bind: (SocketAddress fromString: '0.0.0.0:7777').
		s2 listen: 10.
		^s2.
	}

	method(#class) start_client_socket
	{
		| s buf count |
		s := AsyncClientSocket family: Socket.Family.INET type: Socket.Type.STREAM.
		buf := ByteArray new: 128.

		count := 0.

		s onEvent: #connected do: [ :sck :state |
			if (state)
			{
				s writeBytesFrom: #[ $a, $b, $c ].
				s writeBytesFrom: #[ $d, $e, $f ].
			}
			else
			{
				'FAILED TO CONNECT' dump.
			}.
		]. 

		s onEvent: #data_in do: [ :sck |
			| nbytes |
			while (true)
			{
				nbytes := sck readBytesInto: buf. 
				if (nbytes <= 0) 
				{
					if (nbytes == 0) { sck close }.
					break.
				}.
				('Got ' & (nbytes asString)) dump.
				buf dump.
			}.
		].
		s onEvent: #data_out do: [ :sck |
			if (count < 10) { sck writeBytesFrom: #[ $a, $b, C'\n' ]. count := count + 1. }.
		].

		s connect: (SocketAddress fromString: '127.0.0.1:9999').
	}

	method(#class) mainxx
	{
		[
			| s s2 ss |

			[
				s := self start_client_socket.
				s2 := self start_server_socket.

				while (true)
				{
					ss := thisProcess handleAsyncEvent.
					if (ss isError) { break }.
					###if (ss == st) { thisProcess removeAsyncSemaphore: st }.
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


	method(#class) another_proc: base_port
	{
		| fcgi |

		[
			thisProcess initAsync.
			fcgi := FcgiServer new.
			[
				| ss |
				fcgi start: %(
					SocketAddress fromString: ('[::]:' & base_port asString),
					SocketAddress fromString: ('0.0.0.0:' & (base_port + 1) asString)
				).

				while (true) 
				{
					ss := thisProcess handleAsyncEvent.
					if (ss isError) { break }.
				}.
			] ensure: [ 
				if (fcgi notNil) { fcgi close } 
			]

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump].

		'----- END OF ANOTHER PROC ------' dump.
	}
	
	method(#class) main
	{
		| fcgi addr |

(*
[
addr := SocketAddress fromString: '1.2.3.4:5555'.
##addr := SocketAddress fromString: '127.0.0.1:22'.
fcgi := SyncSocket family: (addr family) type: Socket.Type.STREAM.
fcgi timeout: 5.
fcgi connect: addr.
] on: Exception do: [:ex | ].
*)

		[ self another_proc: 5000 ] fork.
		[ self another_proc: 5100 ] fork.
		[ self another_proc: 5200 ] fork.

		[
			thisProcess initAsync.
			fcgi := FcgiServer new.
			[
				| ss |
				fcgi start: %(
					SocketAddress fromString: '[::]:7777',
					SocketAddress fromString: '0.0.0.0:7776'
				).

				while (true) 
				{
					ss := thisProcess handleAsyncEvent.
					if (ss isError) { break }.
				}.
			] ensure: [ 
				if (fcgi notNil) { fcgi close } 
			]

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump].

		'----- END OF MAIN ------' dump.
	}

	method(#class) mainqq
	{
		| fcgi addr sg ss |

		sg := SemaphoreGroup new.

		[
			fcgi := FcgiServer new.
			[
				fcgi start: %(
					SocketAddress fromString: '[::]:7777',
					SocketAddress fromString: '0.0.0.0:7776'
				).

				while (true)
				{
					ss := sg wait.
					if (ss isError) { break }.
				}
			] ensure: [ 
				if (fcgi notNil) { fcgi close } 
			]

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump].

		'----- END OF MAIN ------' dump.
	}
}
