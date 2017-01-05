#class Collection(Object)
{
}

## -------------------------------------------------------------------------------

#class(#pointer) Array(Collection)
{
	#method size
	{
		^self basicSize.
	}

	#method ubound
	{
		^(self basicSize - 1).
	}

	#method at: anInteger
	{
		^self basicAt: anInteger.
	}

	#method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}

	#method first
	{
		^self at: 0.
	}

	#method last
	{
		^self at: (self ubound).
	}

	#method do: aBlock
	{
		0 to: (self ubound) do: [:i | aBlock value: (self at: i)].
	}

	#method copy: anArray
	{
		0 to: (anArray ubound) do: [:i | self at: i put: (anArray at: i) ].
	}
}

## -------------------------------------------------------------------------------

#class(#character) String(Array)
{
	#method & string
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
	
	#method asString
	{
		^self
	}
}

## -------------------------------------------------------------------------------

#class(#character) Symbol(String)
{
	#method asString
	{
		(* TODO: make this a primitive for performance *)

		(* convert a symbol to a string *)
		| size str |
		size := self basicSize.
		str := String basicNew: size.
		
		0 priorTo: size do: [:i | str at: i put: (self at: i) ].
		^str.
	}
}

## -------------------------------------------------------------------------------

#class(#byte) ByteArray(Collection)
{
	#method at: anInteger
	{
		^self basicAt: anInteger.
	}

	#method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}
}

## -------------------------------------------------------------------------------

#class Set(Collection)
{
	#dcl tally bucket.
	
	#method initialize
	{
		self.tally := 0.
		self.bucket := Array new: 100. 
	}

	#method size
	{
		^self.tally
	}

	#method at: key
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		(ass notNil) ifTrue: [^ass value].
		^ErrorCode.NOENT
	}

	#method at: key ifAbsent: error_block
	{
		| ass |
		ass := self __find: key or_upsert: false with: nil.
		(ass notNil) ifTrue: [^ass value].
		^error_block value.
	}

	#method at: key put: value
	{
		self __find: key or_upsert: true with: value.
		^value
	}

	#method __find: key or_upsert: upsert with: value
	{
		| hv ass bs index ntally |

		bs := self.bucket size.
		hv := key hash.
		index := hv rem: bs.

		[(ass := self.bucket at: index) notNil] 
			whileTrue: [
				[key == ass key] ifTrue: [
					(* found *)
					upsert ifTrue: [ass value: value].
					^ass
				].
				index := (index + 1) rem: bs.
			].

		upsert ifFalse: [^nil].

		ntally := self.tally + 1.
		(ntally >= bs) ifTrue: [
			(* expand the bucket *)
			
		].
		
		ass := Association key: key value: value.
		self.tally := ntally.
		self.bucket at: index put: ass.
		
		^ass
	}

	#method do: block
	{
		| bs |

		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			(ass := self.bucket at: i) notNil ifTrue: [block value: ass value]
		].
	}
	
	#method keysAndValuesDo: block
	{
		| bs |

		bs := self.bucket size.
		0 priorTo: bs by: 1 do: [:i |
			| ass |
			(ass := self.bucket at: i) notNil ifTrue: [block value: ass key value: ass value]
		].	
	}
}

#class SymbolSet(Set)
{
}

#class Dictionary(Set)
{
}

#pooldic Log
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

#class SystemDictionary(Dictionary)
{
	## the following methods may not look suitable to be placed
	## inside a system dictionary. but they are here for quick and dirty
	## output production from the stix code.
	##   System logNl: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'.
	##

	#dcl(#pooldic) Log.

	#method atLevel: level log: message
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	#method atLevel: level log: message and: message2
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	#method atLevel: level log: message and: message2 and: message3
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	#method atLevel: level logNl: message 
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

	#method atLevel: level logNl: message and: message2
	{
		^self atLevel: level log: message and: message2 and: S'\n'.
	}

	#method log: message
	{
		^self atLevel: Log.INFO log: message.
	}

	#method log: message and: message2
	{
		^self atLevel: Log.INFO log: message and: message2.
	}

	#method logNl: message
	{
		^self atLevel: Log.INFO logNl: message.
	}

	#method logNl: message and: message2
	{
		^self atLevel: Log.INFO logNl: message and: message2.
	}
}

#class Namespace(Set)
{
}

#class PoolDictionary(Set)
{
}

#class MethodDictionary(Dictionary)
{

}

#extend Apex
{
	## -------------------------------------------------------
	## Association has been defined now. let's add association
	## creating methods
	## -------------------------------------------------------

	#method(#class) -> object
	{
		^Association new key: self value: object
	}

	#method -> object
	{
		^Association new key: self value: object
	}
}
