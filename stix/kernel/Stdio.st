#class(#byte) Stdio(Object) from 'stdio'
{
	#dcl(#class) in out err.

	(*
	 * The following methods are generated by the module.
	 *   #method(#class) _newInstSize { <primitive: #stdio._newInstSize> }
	 *   #method open: name for: mode { <primitive: #stdio.open> }
	 *   #method close { <primitive: #stdio.close> }
	 *)

	#method(#class) new: size
	{
		## ignore the specified size
		^(super new: (self _newInstSize))
	}

	#method(#class) new
	{
		^(super new: (self _newInstSize))
	}

	#method(#class) open: name for: mode
	{
		^(self new) open: name for: mode
	}


(* ---------------------
	#method(#class) input
	{
		self.in isNil ifTrue: [ self.in := ^(super new) open: 0 for: 'r' ].
		^self.in.
	}

	#method(#class) output
	{
		self.out isNil ifTrue: [ self.out := ^(super new) open: 1 for: 'w' ].
		^self.out.
	}

	#method(#class) error
	{
		self.err isNil ifTrue: [ self.err := ^(super new) open: 2 for: 'w' ].
		^self.err.
	}
------------------------ *)

	(*
	#method format: fmt with: ...
	{
 
	}
	*)

	#method format (fmt, args)
	{
		| a b c |
		'THIS IS FORMAT' dump.
		fmt dump.
		args dump.
		thisContext temporaryCount dump.
	}
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