###include 'Moo.moo'.
#include 'Socket.moo'.

class HttpSocket(Socket)
{
	method onSocketDataIn
	{
		'CLIENT got DATA' dump.
		self close.
	}
}

class HttpServerSocket(ServerSocket)
{
	method initialize
	{
		super initialize.
	}

	method onSocketAccepted: clisck from: cliaddr
	{
		'CLIENT accepted ..............' dump.
clisck dump.
		cliaddr dump.
	##	clisck close.
	}

	method acceptedSocketClass
	{
		^HttpSocket
	}
}

class HttpServer(Object)
{
	var server_sockets.

	method initialize
	{
		super initialize.
		server_sockets := LinkedList new.
	}

	method start: laddr
	{
		| sck |

		if (laddr class == Array)
		{
			laddr do: [:addr |
				sck := HttpServerSocket family: (addr family) type: Socket.Type.STREAM.
				self.server_sockets addLast: sck.
				sck bind: addr.
			].
		}
		else
		{
			sck := HttpServerSocket family: (laddr family) type: Socket.Type.STREAM.
			self.server_sockets addLast: sck.
			sck bind: laddr.
		}.

		self.server_sockets do: [:ssck |
			ssck listen: 128.
		].
	}

	method close
	{
		self.server_sockets do: [:sck |
			sck close.
		].

		while (self.server_sockets size > 0)
		{
			self.server_sockets removeLastLink.
		}.
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
