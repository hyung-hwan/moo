#class(#byte) Stdio(Object) from 'stdio'
{
##	#method(#class) _newInstSize
##	{
##		<primitive: #stdio._newInstSize>
##	}

	#method(#class) new: size
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new: (self _newInstSize))
	}

	#method(#class) new
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new: (self _newInstSize))
	}

	#method(#class) open: name for: mode
	{
		^(self new) open: name for: mode
	}

##	#method open: name for: mode
##	{
##		<primitive: #stdio.open>
##	}

##	#method close
##	{
##		<primitive: #stdio.close>
##	}
}

#extend Stdio
{
	#method xxxx
	{
		self basicSize dump.
	}
}

#class(#byte) Stdio2(Stdio)
{
	#method(#class) new
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new).
	}

}
