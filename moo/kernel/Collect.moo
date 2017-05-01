
class Collection(Object)
{
	method isEmpty
	{
		^self size <= 0
	}

	method notEmpty
	{
		^self size > 0
	}
	
	method size
	{
		(* Each subclass must override this method because
		 * it interates over the all elements for counting *)
		| count |
		count := 0.
		self do: [ :el | count := count + 1 ].
		^count
	}
	
	method do: block
	{
		^self subclassResponsibility: #do
	}
	
	method detect: block
	{
		self do: [ :el | if (block value: el) { ^el } ].
		^Error.Code.ENOENT
	}
	
	method detect: block ifNone: exception_block
	{
		self do: [ :el | if (block value: el) { ^el } ].
		^exception_block value.
	}
}

## -------------------------------------------------------------------------------
class(#pointer) Array(Collection)
{
	method size
	{
		^self basicSize
	}

	method at: anInteger
	{
		^self basicAt: anInteger.
	}

	method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}

	method first
	{
		^self at: 0.
	}

	method last
	{
		^self at: (self basicSize - 1).
	}

	method do: aBlock
	{
		0 priorTo: (self basicSize) do: [:i | aBlock value: (self at: i)].
	}

	method copy: anArray
	{
		0 priorTo: (anArray basicSize) do: [:i | self at: i put: (anArray at: i) ].
	}
}

## -------------------------------------------------------------------------------

class(#character) String(Array)
{
	method & string
	{
		(* TOOD: make this a primitive for performance. *)
		
		(* concatenate two strings. *)
		| newsize newstr cursize appsize |

		cursize := self basicSize.
		appsize := string basicSize.
		newsize := cursize + appsize.
		(*newstr := self class basicNew: newsize.*)
		newstr := String basicNew: newsize.

		0 priorTo: cursize do: [:i | newstr at: i put: (self at: i) ].
		0 priorTo: appsize do: [:i | newstr at: (i + cursize) put: (string at: i) ].

		^newstr
	}
	
	method asString
	{
		^self
	}
	
	(* TODO: Symbol is a #final class. Symbol new is not allowed. To create a symbol programatically, you should
	 *       build a string and send asSymbol to the string............
	method asSymbol
	{
	}
	*)
}

## -------------------------------------------------------------------------------

class(#character,#final,#limited) Symbol(String)
{
	method asString
	{
		(* TODO: make this a primitive for performance *)

		(* convert a symbol to a string *)
		| size str i |
		size := self basicSize.
		str := String basicNew: size.

		##0 priorTo: size do: [:i | str at: i put: (self at: i) ].
		i := 0.
		while (i < size) 
		{ 
			str at: i put: (self at: i).
			i := i + 1
		}.
		^str.
	}

	method = anObject
	{
		(* for a symbol, equality check is the same as the identity check *)
		<primitive: #_identical>
		self primitiveFailed.
	}

	method ~= anObject
	{
		(* for a symbol, equality check is the same as the identity check *)
		<primitive: #_not_identical>
		^(self == anObject) not.
	}
}

## -------------------------------------------------------------------------------

class(#byte) ByteArray(Collection)
{
	method at: anInteger
	{
		^self basicAt: anInteger.
	}

	method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}
}

## -------------------------------------------------------------------------------

class Set(Collection)
{
	var tally, bucket.

	method(#class) new: size
	{
		^self new initialize: size.
	}

	method initialize
	{
		^self initialize: 128. (* TODO: default initial size *)
	}

	method initialize: size
	{
		if (size <= 0) { size := 2 }.
		self.tally := 0.
		self.bucket := Array new: size.
	}

	method size
	{
		^self.tally
	}

	method __make_expanded_bucket: bs
	{
		| newbuc newsz ass index |

		(* expand the bucket *)
		newsz := bs + 128. (* TODO: keep this growth policy in sync with VM(dic.c) *)
		newbuc := Array new: newsz.
		0 priorTo: bs do: [:i |
			ass := self.bucket at: i.
			if (ass notNil) 
			{
				index := (ass key hash) rem: newsz.
				while ((newbuc at: index) notNil)  { index := (index + 1) rem: newsz }.
				newbuc at: index put: ass
			}.
		].

		^newbuc.
	}
	
	
	method __find: key or_upsert: upsert with: value
	{
		| hv ass bs index ntally |

		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key)
			{
				(* found *)
				if (upsert) { ass value: value }.
				^ass
			}.
			index := (index + 1) rem: bs.
		}.

		##upsert ifFalse: [^ErrorCode.NOENT].
		if (upsert) {} else { ^Error.Code.ENOENT }.

		ntally := self.tally + 1.
		if (ntally >= bs)
		{
			self.bucket := self __make_expanded_bucket: bs.
			bs := self.bucket size.
			index := hv rem: bs.
			while ((self.bucket at: index) notNil) { index := (index + 1) rem: bs }.
		}.
		
		ass := Association key: key value: value.
		self.tally := ntally.
		self.bucket at: index put: ass.
		
		^ass
	}

	method at: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		if (ass isError) { ^ass }.
		^ass value
	}

	method at: key ifAbsent: error_block
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		if (ass isError)  { ^error_block value }.
		^ass value
	}

	method associationAt: assoc
	{
		^self __find: (assoc key) or_upsert: false with: nil.
	}

	method associationAt: assoc ifAbsent: error_block
	{
		| ass |
		ass := self __find: (assoc key) or_upsert: false with: nil.
		if (ass isError) { ^error_block value }.
		^ass
	}

	method at: key put: value
	{
		(* returns the affected/inserted association *)
		^self __find: key or_upsert: true with: value.
	}

	method includesKey: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		^ass notError
	}

	method includesAssociation: assoc
	{
		| ass |
		ass := self __find: (assoc key) or_upsert: false with: nil.
		^ass = assoc.
	}
	
	method includesKey: key value: value
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		^ass key = key and: [ass value = value]
	}

	method __find_index: key
	{
		| bs ass index |
		
		bs := self.bucket size.
		index := (key hash) rem: bs.
		
		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key) { ^index }.
			index := (index + 1) rem: bs.
		}.

		^Error.Code.ENOENT.
	}

	method __remove_at: index
	{
		| bs x y i v ass z |

		bs := self.bucket size.
		v := self.bucket at: index.

		x := index.
		y := index.
		i := 0.
		while (i < self.tally)
		{
			y := (y + 1) rem: bs.

			ass := self.bucket at: y.
			if (ass isNil) { (* done. the slot at the current index is nil *) break }.
			
			(* get the natural hash index *)
			z := (ass key hash) rem: bs.

			(* move an element if necessary *)
			if ((y > x and: [(z <= x) or: [z > y]]) or: [(y < x) and: [(z <= x) and: [z > y]]])
			{
				self.bucket at: x put: (self.bucket at: y).
				x := y.
			}.
			
			i := i + 1.
		}.
		
		self.bucket at: x put: nil.
		self.tally := self.tally - 1.
		
		(* return the affected association *)
		^v
	}

	method removeKey: key
	{
		| index |
		index := self __find_index: key.
		if (index isError) { ^index }.
		^self __remove_at: index.
	}

	method removeKey: key ifAbsent: error_block
	{
		| index |
		index := self __find_index: key.
		if (index isError) { ^error_block value }.
		^self __remove_at: index.
	}

	method removeAllKeys
	{
		(* remove all items from a dictionary *)
		| bs |
		bs := self.bucket size.
		0 priorTo: bs do: [:i | self.bucket at: i put: nil ].
		self.tally := 0
	}

(* TODO: ... keys is an array of keys.
	method removeAllKeys: keys
	{
		self notImplemented: #removeAllKeys:
	}
*)

	method remove: assoc
	{
		^self removeKey: (assoc key)
	}

	method remove: assoc ifAbsent: error_block
	{
		^self removeKey: (assoc key) ifAbsent: error_block
	}


	method do: block
	{
		| bs |
		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			if ((ass := self.bucket at: i) notNil) { block value: ass value }.
		].
	}
	
	method keysDo: block
	{
		| bs |
		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			if ((ass := self.bucket at: i) notNil) { block value: ass key }.
		].
	}

	method keysAndValuesDo: block
	{
		| bs |
		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			if ((ass := self.bucket at: i) notNil)  { block value: ass key value: ass value }.
		].	
	}
}

class SymbolSet(Set)
{
}

class Dictionary(Set)
{
	(* [NOTE] 
	 *  VM require Dictionary to implement new: and __put_assoc
	 *  for the dictionary expression notation - :{ }
	 *)
	
	## TODO: implement Dictionary as a Hashed List/Table or Red-Black Tree
	##       Do not inherit Set upon reimplementation
	##
	method(#class) new: size
	{
		^super new: (size + 10).
	}

	(* put_assoc: is called internally by VM to add an association
	 * to a dictionary with the dictionary/association expression notation
	 * like this:
	 *
	 *   :{ 1 -> 20, #moo -> 999 } 
	 *
	 * it must return self for the way VM works.
	 *)
	method put_assoc: assoc
	{
		| hv ass bs index ntally key |

		key := assoc key.
		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		(* as long as 'assoc' supports the message 'key' and 'value'
		 * this dictionary should work. there is no explicit check 
		 * on this protocol of key and value. *)

		while ((ass := self.bucket at: index) notNil)
		{
			if (key = ass key) 
			{
				(* found *)
				self.bucket at: index put: assoc.
				^self. ## it must return self for the instructions generated by the compiler.
			}.
			index := (index + 1) rem: bs.
		}.

		(* not found *)
		ntally := self.tally + 1.
		if (ntally >= bs) 
		{
			self.bucket := self __make_expanded_bucket: bs.
			bs := self.bucket size.
			index := hv rem: bs.
			while ((self.bucket at: index) notNil) { index := (index + 1) rem: bs }.
		}.

		self.tally := ntally.
		self.bucket at: index put: assoc.

		(* it must return self for the instructions generated by the compiler.
		 * otherwise, VM will break. *)
		^self.
	}
}

class SystemDictionary(Set)
{
	method at: key
	{
		if (key class ~= Symbol) { InvalidArgumentException signal: 'key is not a symbol' }.
		^super at: key.
	}
	
	method at: key put: value
	{
		if (key class ~= Symbol) { InvalidArgumentException signal: 'key is not a symbol' }.
		^super at: key put: value
	}
}

class Namespace(Set)
{
}

class PoolDictionary(Set)
{
}

class MethodDictionary(Dictionary)
{

}

extend Apex
{
	## -------------------------------------------------------
	## Association has been defined now. let's add association
	## creating methods
	## -------------------------------------------------------

	method(#class) -> object
	{
		^Association new key: self value: object
	}

	method -> object
	{
		^Association new key: self value: object
	}
}

## -------------------------------------------------------------------------------

class Link(Object)
{
	var prev, next, value.
	
	method(#class) new: value
	{
		| x | 
		x := self new.
		x value: value.
		^x
	}

	method prev { ^self.prev }
	method next { ^self.next }
	method value { ^self.value }
	method prev: link { self.prev := link }
	method next: link { self.next := link }
	method value: value { self.value := value }
}

class LinkedList(Collection)
{
	var first, last, tally.

	method initialize
	{
		self.tally := 0.
	}
	
	method size
	{
		^self.tally
	}

	method first
	{
		^self.first
	}

	method last
	{
		^self.last
	}

	method insertLink: link at: pos
	{
		if (pos isNil)
		{
			(* add link at the back *)
			if (self.tally == 0)
			{
				self.first := link.
				self.last := link.
				link prev: nil.
			}
			else
			{
				link prev: self.last.
				self.last next: link.
				self.last := link.
			}.
			link next: nil.
		}
		else
		{
			(* insert the link before pos *)
			link next: pos.
			link prev: pos prev.
			if (pos prev notNil) { pos prev next: link }
			else { self.first := link }.
			pos prev: link
		}.

		self.tally := self.tally + 1.
		^link
	}

	method insert: value at: pos
	{
		^self insertLink: (Link new: value) at: pos
	}
	
	method addFirst: value
	{
		^self insert: value at: self.first
	}
	
	method addLast: value
	{
		^self insert: value at: nil
	}

	method addFirstLink: link
	{
		^self insertLink: link at: self.first
	}
	
	method addLastLink: link
	{
		^self insertLink: link at: nil
	}
	
	method removeLink: link
	{
		if (link next notNil) { link next prev: link prev }
		else { self.last := link prev }.
		
		if (link prev notNil) { link prev next: link next }
		else { self.first := link next }.
		
		self.tally := self.tally - 1.
		^link.
	}

	method removeFirstLink
	{
		^self removeLink: self.first
	}

	method removeLastLink
	{
		^self removeLink: self.last
	}
	
	method do: block
	{
		| link next |
		link := self.first.
		while (link notNil)
		{
			next := link next.
			block value: link value.
			link := next.
		}
	}

	method doOverLink: block
	{
		| link next |
		link := self.first.
		while (link notNil)
		{
			next := link next.
			block value: link.
			link := next.
		}
	}

	method findLink: value
	{
		self doOverLink: [:el | if (el value = value) { ^el }].
		^Error.Code.ENOENT
	}
}
