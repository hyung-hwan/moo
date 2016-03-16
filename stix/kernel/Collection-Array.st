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
}

#class(#character) Symbol(Array)
{
}
