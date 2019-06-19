###include 'Moo.moo'.
#include 'Socket.moo'.

/* -------------------------------------------



 ----------------------------------------------- */

class Fcgi(Object)
{
}

class Fcgi.Header(Object) 
{
	var(#get,#set) version := 0.
	var(#get,#set) type := 0.
	var(#get,#set) requestId := 0.
	var(#get,#set) contentLength := 0.
	var(#get) paddingLength := 0.
	var reserved := 0.

	method readFrom: aStream 
	{
		self.version := aStream uint8.
		self.type := aStream uint8.
		self.requestId := aStream uint16.
		self.contentLength := aStream uint16.
		self.paddingLength := aStream uint8.
		self.reserved := aStream uint8
	}

	method storeOn: aStream 
	{
		aStream uint8: version.
		aStream uint8: type.
		aStream uint16: requestId.
		aStream uint16: self contentLength.
		aStream uint8: self paddingLength.
		aStream uint8: 0
	}
}

class Fcgi.Record(Object)
{
	var(#get,#set) header.

	method(#class) type: anInteger
	{
		^self new type: anInteger
	}

	method ensureHeader 
	{
		^self.header ifNil: [self.header := Fcgi.Header new]
	}

	method type 
	{
		^self.header type
	}

	method type: anInteger 
	{
		self ensureHeader type: anInteger
	}
}

class Fcgi.BeginRequestRecord(Fcgi.Record)
{
	var role.
	var(#get) flags.
	var reserved.

	method readFrom: aStream 
	{
		self.role := aStream uint16.
		self.flags := aStream uint8.
		self.reserved := aStream next: 5
	}
}

class Fcgi.EndRequestRecord(Fcgi.Record)
{
	var(#get,#set) appStatus.
	var(#get,#set) protocolStatus.
	var reserved.

	method readFrom: aStream 
	{
		self.appStatus := aStream uint32.
		self.protocolStatus := aStream uint8.
		self.reserved := aStream next: 3
	}

	method storeOn: aStream 
	{
		aStream uint32: self.appStatus.
		aStream uint8: self.protocolStatus.
		1 to: 3 do: [:each | aStream uint8: 0]
	}
}

class Fcgi.DefaultRecord(Fcgi.Record)
{
	var contentData.
	var paddingData.

	method content
	{
		^self.contentData
	}

	method content: aString 
	{
		self.contentData := aString.
		self ensureHeader contentLength: aString size
	}

	method readFrom: aStream 
	{
		self.contentData := aStream next: header contentLength.
		self.paddingData := aStream next: header paddingLength
	}

	method storeOn: aStream 
	{
		aStream nextPutAll: self.contentData.
		1 to: self header paddingLength do: [:each | aStream uint8: 0]
	}
}


class Fcgi.ParamRecord(Fcgi.Record)
{
	var namesAndValues.
	var cookies.
	var(#get,#set) post.
	var fields.
	var postFields.
	var getFields.


	method initialize 
	{
		super initialize.
		self.namesAndValues := OrderedCollection new
	}

	method at: aKey 
	{
		^(self.namesAndValues detect: [:assoc | assoc key = aKey]) value
	}

	method at: aKey ifAbsent: aBlock 
	{
		^(self.namesAndValues detect: [:assoc | assoc key = aKey] ifNone: aBlock) value
	}

	method at: aKey put: aValue 
	{
		self.namesAndValues add: aKey -> aValue
	}

	method cookieString 
	{
		^self at: 'HTTP_COOKIE' ifAbsent: ['']
	}

	method cookies 
	{
		^self.cookies ifNil: [self.cookies := self parseToFields: self cookieString separatedBy: $;]
	}

	method fields
	{
		^self.fields ifNil: [self.fields := (Dictionary new) addAll: self postFields; addAll: self getFields; yourself]
	}

	method getFields 
	{
		^getFields ifNil: [getFields := self parseToFields: self query  separatedBy: $&]
	}

	method header 
	{
		^self
	}

	method method 
	{
		^self at: 'REQUEST_METHOD'
	}

	method postFields 
	{
		^self.postFields ifNil: [
			postFields := self method = 'POST' 
				ifTrue: [ self parseToFields: self post unescapePercents separatedBy: $&]
				ifFalse: [Dictionary new]
		]
	}

	method query 
	{
		^self at: 'QUERY_STRING'
	}

	method url
	{
		^(self at: 'SCRIPT_NAME') & (self at: 'PATH_INFO')
	}

	method parseToFields: aString separatedBy: char 
	{
		| equal tempFields |

		tempFields := Dictionary new.

		if (aString notNil)
		{
### TODO: implement this...
/*
			(aString subStrings: %(char)) do: [:each | 
				equal := each indexOf: $=.
				equal = 0 
					ifTrue: [tempFields at: each put: nil]
					ifFalse: [
					tempFields 
						at: (each first: equal - 1) 
						put: (each allButFirst: equal)] 
			] */
		}.

		^tempFields
	}

	method readFrom: aStream 
	{
		| buffer stream |
		buffer := aStream next: header contentLength.
/* TODO:
		stream := ReadStream on: buffer.
		[stream atEnd] whileFalse: [self readNameValueFrom: stream]
*/
	}

	method readNameValueFrom: aStream 
	{
		| nameLength valueLength name value |
		nameLength := aStream uint8.
		(nameLength bitShift: -7) = 0 ifFalse: [
			nameLength := (nameLength bitShift: 24) + aStream uint24].
		valueLength := aStream uint8.
		(valueLength bitShift: -7) = 0 ifFalse: 
			[valueLength := (valueLength bitShift: 24) + aStream uint24].
		name := aStream next: nameLength.
		value := aStream next: valueLength.
		self at: name put: value
	}
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
		/*while (true)
		{
			req := self readRequest.

		}. */

		buf := ByteArray new: 128.
'IM RUNNING SERVICE...............' dump.


		## typedef struct {
		##   unsigned char version;
		##   unsigned char type;
		##   unsigned char requestIdB1;
		##   unsigned char requestIdB0;
		##   unsigned char contentLengthB1;
		##   unsigned char contentLengthB0;
		##   unsigned char paddingLength;
		##   unsigned char reserved;
		## } FCGI_Header;
/*
		ver := self.bs next.
		type := self.bs next.
		reqid := (self.bs next bitShift: 8) bitAnd: (self.bs next).  ## can i implement nextUint16??
		clen := (self.bs next bitShift: 8) bitAnd: (self.bs next).
		plen := self.bs next.
		self.bs next. ## eat up the reserved byte.
*/

		## typedef struct {
		##  unsigned char roleB1;
		##  unsigned char roleB0;
		##  unsigned char flags;
		##  unsigned char reserved[5];
		## } FCGI_BeginRequestBody;
		## typedef struct {
		##  unsigned char appStatusB3;
		##  unsigned char appStatusB2;
		##  unsigned char appStatusB1;
		##  unsigned char appStatusB0;
		##  unsigned char protocolStatus;
		##  unsigned char reserved[3];
		## } FCGI_EndRequestBody;

/*
		if (type == Fcgi.Type.BEGIN_REQUEST)
		{
		} */

/*
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
*/

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
		| fcgi addr oc |

oc := Fcgi.ParamRecord new.
(oc parseToFields: 'a=b&d=f' separatedBy: $&) dump.
thisProcess terminate.

/*
[
addr := SocketAddress fromString: '1.2.3.4:5555'.
##addr := SocketAddress fromString: '127.0.0.1:22'.
fcgi := SyncSocket family: (addr family) type: Socket.Type.STREAM.
fcgi timeout: 5.
fcgi connect: addr.
] on: Exception do: [:ex | ].
*/

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
