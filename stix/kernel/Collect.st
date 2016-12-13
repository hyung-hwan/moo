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
	#method & aString
	{
		## concatenate two strings.
## TOOD: make this a primitive for performance.
		| newsize newstr self_ubound |

		newsize := self basicSize + aString basicSize.
		##newstr := self class basicNew: newsize.
		newstr := String basicNew: newsize.  ## TODO: redefine , for symbol... it's a work arouind... symbols are not concatenated to a symbol at this moment.
		self_ubound := self ubound.

		0 to: self_ubound do: [:i | newstr at: i put: (self at: i) ].
		0 to: (aString ubound) do: [:i | newstr at: (i + self_ubound + 1) put: (aString at: i) ].

		^newstr
	}
}

## -------------------------------------------------------------------------------

#class(#character) Symbol(String)
{
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
