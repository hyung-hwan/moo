
class Stdio(Object) from 'stdio'
{
	dcl(#class) in out err.

	method(#primitive) open(name, mode).
	method(#primitive) close.
	method(#primitive) gets.
	method(#primitive,#variadic) puts().
	method(#primitive,#variadic) putc().

	method(#class) open: name for: mode
	{
		^(self new) open(name, mode)
	}
	
(* ---------------------
	method(#class) stdin
	{
		self.in isNil ifTrue: [ self.in := ^(super new) open: 0 for: 'r' ].
		^self.in.
	}

	method(#class) stdout
	{
		self.out isNil ifTrue: [ self.out := ^(super new) open: 1 for: 'w' ].
		^self.out.
	}

	method(#class) stderr
	{
		self.err isNil ifTrue: [ self.err := ^(super new) open: 2 for: 'w' ].
		^self.err.
	}
------------------------ *)

	(*
	method format: fmt with: ...
	{
 
	}
	*)

	method(#variadic) format (fmt)
	{
		| a b c |
		'THIS IS FORMAT' dump.
		fmt dump.
		
		thisContext temporaryCount dump.
		0 priorTo: (thisContext vargCount) do: [:k |
			(thisContext vargAt: k) dump.
		].
	}
}

extend Stdio
{
	method xxxx
	{
		self basicSize dump.
	}
}

class Stdio2(Stdio)
{
	method(#class) new
	{
		##self prohibited
		##raise exception. prohibited...
		^(super new).
	}
}
