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

#class(#character) String(Array)
{
	#method , aString
	{
		## concatenate two strings.
## TOOD: make this a primitive for performance.
		| newsize newstr self_ubound|
		newsize := self basicSize + aString basicSize.
		##newstr := self class basicNew: newsize.
		newstr := String basicNew: newsize.  ## TODO: redefine , for symbol... it's a work arouind... symbols are not contacated to a symbol at this moment.
		self_ubound := self ubound.
		0 to: self_ubound do: [:i | newstr at: i put: (self at: i)].
		0 to: (aString ubound) do: [:i | newstr at: (i + self_ubound + 1) put: (aString at: i)].
		^newstr
	}
}

#class(#character) Symbol(String)
{
}
