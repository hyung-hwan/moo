## TODO: move Pointe to a separate file 
class Point(Object)
{
	dcl x y.

	method(#class) new
	{
		^self basicNew x: 0 y: 0.
	}
	method(#class) x: x y: y
	{
		^self basicNew x: x y: y.
	}

	method x 
	{
		^self.x
	}

	method y
	{
		^self.y
	}

	method x: x
	{
		self.x := x
	}

	method y: y
	{
		self.y := y
	}

	method x: x y: y
	{
		self.x := x.
		self.y := y
	}
}

extend SmallInteger
{
	method @ y
	{
		^Point x: self y: y
	}
}

class Console(Object) from 'console'
{
	method(#primitive) _open.
	method(#primitive) _close.
	method(#primitive) _clear.
	method(#primitive) _setcursor(x, y).
	method(#primitive) _write(msg).

(*
	method finalize
	{
		if (still open) {
			self _close.
		}
	}
*)


##	method(#class) input
##	{
##		^self new _open: filename mode: mode
##	}

	method(#class) output
	{
		| c |

		c := self new.
		c _open. ## TODO error check - if ((e := c _open) isError) { ^e }.
		^c
	}

##	method(#class) error
##	{
##	}

	method close
	{
		self _close.
	}

	method write: text
	{
		^self _write(text)
	}

	method clear
	{
		^self _clear.
	}

	method setCursor: point
	{
		^self _setcursor(point x, point y)
	}
}

