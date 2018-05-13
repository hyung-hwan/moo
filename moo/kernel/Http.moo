###include 'Moo.moo'.
#include 'Socket.moo'.

class HttpConnReg(Object)
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
		if (self.first_free_slot < 0) { ^Error.Code.ELIMIT }.
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
}

class HttpSocket(Socket)
{
	var(#get) server := nil.
	var(#get) cid := -1.

	method onSocketDataIn
	{
		'CLIENT got DATA' dump.
		self close.
	}

	method close
	{
('Http Connection closing.......... handle ' & self.handle asString) dump.
		if (self.server notNil)
		{
('Http Connection ' & self.cid asString &  ' removing from server  ..........') dump.
			self.server removeConnection: self.
		}.
		^super close.
	}
	
	method server: server cid: cid
	{
		self.server := server.
		self.cid := cid.
	}
}

class HttpListener(ServerSocket)
{
	var(#get,#set) server := nil.

	method initialize
	{
		super initialize.
	}

	method onSocketAccepted: clisck from: cliaddr
	{
		| cid |

		'CLIENT accepted ..............' dump.
clisck dump.
		cliaddr dump.

		if (self.server notNil)
		{
			server addConnection: clisck.
		}.
	}

	method acceptedSocketClass
	{
		##^if (self currentAddress port == 80) { HttpSocket } else { HttpSocket }.
		^HttpSocket.
	}
}

class HttpServer(Object)
{
	var listeners.
	var connreg.

	method initialize
	{
		super initialize.
		self.listeners := LinkedList new.
		self.connreg := HttpConnReg new.
	}

	method start: laddr
	{
		| listener |

		if (laddr class == Array)
		{
			laddr do: [:addr |
				listener := HttpListener family: (addr family) type: Socket.Type.STREAM.
				self.listeners addLast: listener.
				listener server: self.
				listener bind: addr.
			].
		}
		else
		{
			listener := HttpListener family: (laddr family) type: Socket.Type.STREAM.
			self.listeners addLast: listener.
			listener server: self.
			listener bind: laddr.
		}.

		self.listeners do: [:l_listener |
			l_listener listen: 128.
		].
	}

	method close
	{
		self.listeners do: [:listener |
			listener server: nil.
			listener close.
		].

		while (self.listeners size > 0)
		{
			self.listeners removeLastLink.
		}.
	}

	method addConnection: conn
	{
		| cid |
		cid := self.connreg add: conn.
		if (cid isError)
		{
			'ERROR - CANNOT REGISTER NEW CONNECTION >>>>>>>>>> ' dump.
			conn close.
			^self.
		}.

('ADD NEW CONNECTION ' & cid asString) dump.
		conn server: self cid: cid.
	}
	
	method removeConnection: conn
	{
		self.connreg remove: (conn cid).
		conn server: nil cid: -1.
	}
}

class MyObject(Object)
{
	method(#class) start_server_socket
	{
		| s2 buf |
		s2 := ServerSocket family: Socket.Family.INET type: Socket.Type.STREAM.
		buf := ByteArray new: 128.

		s2 onEvent: #accepted do: [ :sck :clisck :cliaddr |
'SERVER ACCEPTED new client' dump.
			clisck onEvent: #data_in do: [ :csck |
				| nbytes |
				while (true)
				{
					nbytes := csck readBytes: buf. 
					if (nbytes <= 0)
					{
						if (nbytes == 0) { csck close }.
						('Got ' & (nbytes asString)) dump.
						break.
					}.

					buf dump.
					csck writeBytes: buf offset: 0 length: nbytes.
				}.
			].
			clisck onEvent: #data_out do: [ :csck |
				##csck writeBytes: #[ $a, $b, C'\n' ].
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
		s := ClientSocket family: Socket.Family.INET type: Socket.Type.STREAM.
		buf := ByteArray new: 128.

		count := 0.

		s onEvent: #connected do: [ :sck :state |
			if (state)
			{
				s writeBytes: #[ $a, $b, $c ].
				s writeBytes: #[ $d, $e, $f ].
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
				nbytes := sck readBytes: buf. 
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
			if (count < 10) { sck writeBytes: #[ $a, $b, C'\n' ]. count := count + 1. }.
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


	method(#class) main
	{
		| httpd |

		[
			httpd := HttpServer new.
			[
				| ss |
				httpd start: %(
					SocketAddress fromString: '[::]:7777',
					SocketAddress fromString: '0.0.0.0:7776'
				).

				while (true) {
					ss := System handleAsyncEvent.
					if (ss isError) { break }.
				}.
			] ensure: [ 
				if (httpd notNil) { httpd close } 
			]

		] on: Exception do: [:ex | ('Exception - '  & ex messageText) dump].

		'----- END OF MAIN ------' dump.
	}
}
