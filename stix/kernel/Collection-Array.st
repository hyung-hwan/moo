#class(#pointer) Array(Collection)
{
	#method size
	{
		^self basicSize.
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
		^self at: 1.
	}

	#method last
	{
		^self at: self size.
	}

	#method do: aBlock
	{
		1 to: self size do: [:i | aBlock value: (self at: i)].
	}
}

#class(#character) String(Array)
{
}

#class(#character) Symbol(Array)
{
}
