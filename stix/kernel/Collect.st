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
	#method , aString
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

	#method log: message level: level
	{
		<primitive: #_log>
		## do nothing upon logging failure
	}

	#method logNl: message level: level
	{
		self log: message level: level.
		self log: S'\n' level: level.
		^self.
	}

	#method log: message
	{
		^self log: message level: Log.INFO.
	}

	#method logNl: message
	{
		^self logNl: message level: Log.INFO.
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

