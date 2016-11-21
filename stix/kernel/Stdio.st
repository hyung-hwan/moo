#class(#word) Stdio(Object)
{

	#method(#class) new: size
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new: 1)
	}

	#method(#class) new
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new: 1)
	}

	#method(#class) open: name for: mode
	{
		^(super new: 1) open: name for: mode
	}

	#method open: name for: mode
	{
		<primitive: #stdio_open>
	}

	#method close
	{
		<primitive: #stdio_close>
	}

	#method xxx
	{
		self basicSize dump.
	}
}

#class(#word) Stdio2(Stdio)
{
	#method(#class) new
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new).
	}

}
