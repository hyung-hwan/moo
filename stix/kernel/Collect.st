
class Collection(Object)
{
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
}

## -------------------------------------------------------------------------------

class(#character) Symbol(String)
{
	method asString
	{
		(* TODO: make this a primitive for performance *)

		(* convert a symbol to a string *)
		| size str |
		size := self basicSize.
		str := String basicNew: size.
		
		0 priorTo: size do: [:i | str at: i put: (self at: i) ].
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
	dcl tally bucket.
	
	method initialize
	{
		self.tally := 0.
		self.bucket := Array new: 128.  (* TODO: initial size *)
	}

	method size
	{
		^self.tally
	}

	method __find: key or_upsert: upsert with: value
	{
		| hv ass bs index ntally |

		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		[(ass := self.bucket at: index) notNil] 
			whileTrue: [
				(key = ass key) ifTrue: [
					(* found *)
					upsert ifTrue: [ass value: value].
					^ass
				].
				index := (index + 1) rem: bs.
			].

		upsert ifFalse: [^ErrorCode.NOENT].

		ntally := self.tally + 1.
		(ntally >= bs) ifTrue: [
			| newbuc newsz |
			(* expand the bucket *)
			newsz := bs + 123. (* TODO: keep this growth policy in sync with VM(dic.c) *)
			newbuc := Array new: newsz.
			0 priorTo: bs do: [:i |
				ass := self.bucket at: i.
				(ass notNil) ifTrue: [
					index := (ass key hash) rem: newsz.
					[(newbuc at: index) notNil] whileTrue: [index := (index + 1) rem: newsz].
					newbuc at: index put: ass
				]
			].

			self.bucket := newbuc.
			bs := self.bucket size.
			index := hv rem: bs.
			[(self.bucket at: index) notNil] whileTrue: [index := (index + 1) rem: bs ].
		].
		
		ass := Association key: key value: value.
		self.tally := ntally.
		self.bucket at: index put: ass.
		
		^ass
	}

	method at: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		(ass isError) ifTrue: [^ass].
		^ass value
	}

	method at: key ifAbsent: error_block
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		(ass isError) ifTrue: [^error_block value].
		^ass value
	}

	method associationAt: key
	{
		^self __find: key or_upsert: false with: nil.
	}

	method associationAt: key ifAbsent: error_block
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		(ass isError) ifTrue: [^error_block value].
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
		
		[(ass := self.bucket at: index) notNil] 
			whileTrue: [ 
				(key = ass key) ifTrue: [^index].
				index := (index + 1) rem: bs.
			].

		^ErrorCode.NOENT.
	}

	method __remove_at: index
	{
		| bs x y i v |

		bs := self.bucket size.
		v := self.bucket basicAt: index.

		x := index.
		y := index.
		i := 0.
		[i < self.tally] whileTrue: [
			| ass z |

			y := (y + 1) rem: bs.

			ass := self.bucket at: i.
			(ass isNil)
				ifTrue: [ 
					(* done. the slot at the current index is nil *)
					i := self.tally 
				]
				ifFalse: [
					(* get the natural hash index *)
					z := (ass key hash) rem: bs.

					(* move an element if necessary *)
					((y > x and: [(z <= x) or: [z > y]]) or: 
					[(y < x) and: [(z <= x) and: [z > y]]]) ifTrue: [
						self.bucket at: x put: (self.bucket at: y).
						x := y.
					].

					i := i + 1
				].
		].
		
		self.bucket at: x put: nil.
		self.tally := self.tally - 1.
		
		(* return the affected association *)
		^v
	}

	method removeKey: key
	{
		| index |
		index := self __find_index: key.
		(index isError) ifTrue: [ ^index ].
		^self __remove_at: index.
	}

	method removeKey: key ifAbsent: error_block
	{
		| index |
		index := self __find_index: key.
		(index isError) ifTrue: [ ^error_block value ].
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
			(ass := self.bucket at: i) notNil ifTrue: [block value: ass value]
		].
	}
	
	method keysDo: block
	{
		| bs |
		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			(ass := self.bucket at: i) notNil ifTrue: [block value: ass key]
		].
	}

	method keysAndValuesDo: block
	{
		| bs |
		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			(ass := self.bucket at: i) notNil ifTrue: [block value: ass key value: ass value]
		].	
	}
}

class SymbolSet(Set)
{
}

class Dictionary(Set)
{
}

pooldic Log
{
	## -----------------------------------------------------------
	## defines log levels
	## these items must follow defintions in stix.h
	## -----------------------------------------------------------

	#DEBUG := 1.
	#INFO  := 2.
	#WARN  := 4.
	#ERROR := 8.
	#FATAL := 16.
}

class SystemDictionary(Dictionary)
{
	## the following methods may not look suitable to be placed
	## inside a system dictionary. but they are here for quick and dirty
	## output production from the stix code.
	##   System logNl: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'.
	##

	dcl(#pooldic) Log.

	method atLevel: level log: message
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method atLevel: level log: message and: message2
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method atLevel: level log: message and: message2 and: message3
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	method atLevel: level logNl: message 
	{
		## the #_log primitive accepts an array.
		## so the following lines should work also.
		## | x |
		## x := Array new: 2.
		## x at: 0 put: message.
		## x at: 1 put: S'\n'.
		## ^self atLevel: level log: x.

		^self atLevel: level log: message and: S'\n'.
	}

	method atLevel: level logNl: message and: message2
	{
		^self atLevel: level log: message and: message2 and: S'\n'.
	}

	method log: message
	{
		^self atLevel: Log.INFO log: message.
	}

	method log: message and: message2
	{
		^self atLevel: Log.INFO log: message and: message2.
	}

	method logNl: message
	{
		^self atLevel: Log.INFO logNl: message.
	}

	method logNl: message and: message2
	{
		^self atLevel: Log.INFO logNl: message and: message2.
	}

	method at: key
	{
		(key class ~= Symbol) ifTrue: [InvalidArgumentException signal: 'key is not a symbol'].
		^super at: key.
	}
	
	method at: key put: value
	{
		(key class ~= Symbol) ifTrue: [InvalidArgumentException signal: 'key is not a symbol'].
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