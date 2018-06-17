class Stream(Object)
{
	method(#class) new
	{
		## you must use dedicated class methods for instantiation
		self messageProhibited: #new.
	}

	method(#class) on: object
	{
		^self subclassResponsibility: #on:.
	}

	method contents
	{
		^self subclassResponsibility: #contents.
	}

	method next
	{
		## return the next object in the receiver
		^self subclassResponsibility: #next.
	}

	method nextPut: object
	{
		## insert an object at the next position in the receiver
		^self subclassResponsibility: #next.
	}

	method nextPutAll: collection
	{
		collection do: [:elem | self nextPut: elem ].
		^collection.
	}

	method atEnd
	{
		^self subclassResponsibility: #next.
	}

	method print: object
	{
		object printOn: self.
	}
}

class PositionableStream(Stream)
{
	var collection.
	var(#get) position.
	var readLimit.

	method(#class) on: collection
	{
		^self new __on collection.
	}

	method initialize
	{
		super initialize.
		self.position := 0.
		self.readLimit := collection size.
	}

	method __on: collection
	{
		self.collection := collection.
	}

	method contents
	{
		^self.collection copyFrom: 0 to: self.readLimit
	}

	method next: count
	{
		| newobj i |
		newobj := self contents class new: count.
		while (i < count)
		{
			newobj at: i put: self next.
			i := i + 1.
		}.
	}

	method peek
	{
	}

	method atEnd
	{
		^self.position >= self.readLimit.
	}

	method isEmpty
	{
		^self.position == 0.
	}

	method position: pos
	{
		##if (pos >= 0 and 
	}

	method reset
	{
		self.position := 0.
	}

	method setToEnd
	{
		self.position := self.readLimit.
	}
}

class ReadWriteStream(PositionableStream)
{
	method next
	{
	}
}

class ExternalStream(ReadWriteStream)
{
}

(*
## mimic java's interface...
interface ByteStreamable
{
	readBytesInto:
	writeBytesFrom:
}
*)

### TODO: specify interface inside [] 
(* 
difficulty: how to ensure that the class implements the defined interface?

1) check when a new instance is created.
   -> i don't want this runtime overhead.

2) create an empty methods as soon as an interface is encountered.
   allows the class body or extend to redefine the methods.
   give a warning that the interface is not fully implemened if
   it doesn't redefined all inteface methods at the end of the class
   definition or at the end of the extend defintion.
   ->
 
3) don't allow 'extend' for the class that implements inteface
   and do the check at the end of class definition. or allow extend
   but the inteface methods must stay inside the main class definition
   -> if i support pooldic definition from within the class, it looks more natural
   -> i use 'extend' more simply because i want to have some constants defined
   -> before i use them in the methods. if i support 'const' inside the class
   -> defintion, it looks also ok.

   interface Shoutable
   {
      method shout.
      method shoutWith: number.
   }
   class xxx(Object) [Shoutable]  <---- allow multiple intefaces here
   {
       const DEFVAL := 10.
       method shout {}
       method shoutWith: number {}
   }

4) other methods???

Let me think about it..
*) 

class ByteStreamAdapter(Object) ### [ByteStreamable, ByteXXX]
{
	var bsobj.
	var inbuf.
	var inpos.
	var inlen.
	var indown.

	var outbuf.
	var outlen.
	var outdown.

	method(#class) new
	{
		self messageProhibited: #new.
	}

	method(#class) new: obj
	{
		self messageProhibited: #new:.
	}

	method initialize
	{
		super initialize.
		self.bsobj := nil.

		self.inbuf := ByteArray new: 1024.
		self.inpos := 0.
		self.inlen := 0.
		self.indown := false.

		self.outbuf := ByteArray new: 1024.
		self.outlen := 0.
		self.outdown := false.
	}

	method(#class) on: bsobj
	{
		^super new __on: bsobj.
	}

	method __on: bsobj
	{
		self.bsobj := bsobj.
	}

	method __fill_inbuf
	{
		| v |
		v := self.bsobj readBytesInto: self.inbuf.
		## if the streamable object is not blocking, it may return an 
		## error object when data is not ready. 
		if (v isError) { ^v }. 
		if (v == 0) 
		{
			## end of input
			self.indown := true.
			^v
		}.

		self.inlen := v.
		self.inpos := 0.
		^v.
	}

	method next
	{
		| v |

		if (self.indown) { ^nil }.
		if (self.inpos >= self.inlen)
		{ 
			v := self __fill_inbuf.
			if (v isError) { ^v }. ## TODO: change error handling
			if (v <= 0) { ^nil }.
			####if (self.inpos >= self.inlen) { ^nil }.
		}.
		
		v := self.inbuf at: self.inpos.
		self.inpos := self.inpos + 1.
		^v.
	}

	method next: count into: byte_array startingAt: pos
	{
		## return the count bytes
		| taken avail needed v incapa |

		if (self.indown) { ^0 }.

		## i assume the given byte array is large enough
		## to store as many as count bytes starting at the pos position.
		## if the parameters cannot meet this assumption, you will get
		## into various system exceptions.
		needed := count.
		incapa := self.inbuf size.

		while (needed > 0)
		{
			avail := self.inlen - self.inpos.
			if (avail <= 0)
			{
				if (needed >= incapa)
				{
					## don't rely on the internal buffer if the number of bytes 
					## needed are equal to or greater than the capacity of the
					## buffer.
					v := self.bsobj readBytesInto: byte_array offset: pos length: needed.
					if (v isError or v <= 0) { break }.  ## <<< TODO: change the error handling
					pos := pos + v.
					needed := needed - v.
					continue.
				}
				else
				{
					v := self __fill_inbuf.
					if (v isError or v <= 0) { break }. ## <<< TODO: change the error handling
				}.
			}.

			taken := if (avail <= needed) { avail } else { needed }.
			byte_array replaceFrom: pos count: taken with: self.inbuf startingAt: self.inpos. 
			self.inpos := self.inpos + taken.
			pos := pos + taken.
			needed := needed - taken.
		}.

		^count - needed.
	}

	method nextPut: count from: byte_array startingAt: pos
	{
		| consumed free rem outcapa |

		if (self.outdown) { ^0 }.

		rem := count.
		outcapa := self.outbuf size.

		while (rem > 0)
		{
			free := outcapa - self.outlen.
			if (free <= 0)
			{
				self flush. ## TODO: error handling...
			}.

			if (self.outlen <= 0 and rem >= outcapa)
			{
				consumed := self.bsobj writeBytesFrom: byte_array offset: pos length: rem.
				if (consumed <= 0) { break }. ## TODO: error handling. also handle exceptions
			}
			else
			{
				consumed := if (free <= rem) { free } else { rem }.
				self.outbuf replaceFrom: self.outlen count: consumed with: byte_array startingAt: pos.
				self.outlen := self.outlen + consumed.
			}.

			pos := pos + consumed.
			rem := rem - consumed.
		}.

		^count - rem.
	}

	method flush
	{
		| v pos |

		pos := 0.
		while (pos < self.outlen)
		{
			v := self.bsobj writeBytesFrom: self.outbuf offset: pos length: (self.outlen - pos).
			if (v <= 0) { break }. ## TODO: error handling. also handle exceptions
			pos := pos + v.
		}.

		self.outlen := 0.
	}
}

class ByteStream(ByteStreamAdapter)
{
	method close
	{
		if (self.bsobj notNil)
		{
			self.bsobj close.
			self.bsobj := nil.
		}.
	}
}

(*
class TextStream(ByteStream)
{
	method initialize
	{
		super initialize.
		self.
	}

	method next
	{
	}

	method __utf8_to_uc
	{
		if (self.seq at: 0) bitAnd: 
	}
}*)

