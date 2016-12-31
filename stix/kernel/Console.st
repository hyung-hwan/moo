#class Point(Object)
{
	#dcl x y.

	#method(#class) new
	{
		^self basicNew x: 0 y: 0.
	}
	#method(#class) x: x y: y
	{
		^self basicNew x: x y: y.
	}

	#method x 
	{
		^self.x
	}

	#method y
	{
		^self.y
	}

	#method x: x
	{
		self.x := x
	}

	#method y: y
	{
		self.y := y
	}

	#method x: x y: y
	{
		self.x := x.
		self.y := y
	}
}

#extend SmallInteger
{
	#method @ y
	{
		^Point x: self y: y
	}
}




#class Console(Object)
{
	#dcl handle.
"
	#method finalize
	{
		handle notNil ifTrue: [
			self _close: handle.
		]
	}
"


##	#method(#class) input
##	{
##		^self new _open: filename mode: mode
##	}

	#method(#class) output
	{
		| c |

		c := self new.
		c handle: (c _open).
		^c
	}

##	#method(#class) error
##	{
##	}


	#method handle: v
	{
		self.handle := v.
	}

	#method close
	{
		self _close: self.handle.
		self.handle := nil.
	}

	#method write: text
	{
		^self _writeOn: self.handle text: text.
	}

	#method clear
	{
		^self _clear: self.handle
	}
	#method setCursor: point
	{
		^self _setCursor: self.handle point: point.
	}
"
	#method _open: filename mode: mode
	{
		self.handle := self __open: filename mode: mode.
		^self.
	}

	#method __open: filename mode: mode
	{
		<primitive: #console.open>
		##StdioException signal: ('cannot open ' & filename).
	}
"

	#method _open
	{
		<primitive: #console.open>
	}

	#method _close: handle
	{
		<primitive: #console.close>
		self primitiveFailed.
	}

	#method _clear: handle
	{
		<primitive: #console.clear>
		self primitiveFailed.
	}

	#method _writeOn: handle text: text
	{
		<primitive: #console.write>
		self primitiveFailed.
	}

	#method _setCursor: handle point: point
	{
		<primitive: #console.setcursor>
		self primitiveFailed.
	}



"
	#method(#class) open
	{
		<primitive: #console.open>
		self primitiveFailed.
	}

	#method close
	{
		<primitive: #console.close>
		self primitiveFailed.
	}

	#method setCursorTo: point
	{
		<primitive: #console.setcursor>
		self primitiveFailed.
	}
"

##x := Colsole new.
##x := Console open.
##(x isError) ifTrue: [ 
##		handle error... 
##	]
##	ifFalse: [
##		x setCursor (1, 2).
##		x clear.
##		x close.
##	]

##x := File open: 'abc.def'
##t := x read: 1000.
##x close.
}

"
Stix define: 'console_write' 
	forClass: Console
	method: 'write: aString upto: length'
	returns: 'size_t'
	arguments: 'void* size_t'

---> produces a method like this internally...

#class Console
{
	#method write: aString upto: length
	{
		<ffi: int console_write (int*, char*, [int, int, char]* )> <== parse the string, create a descriptor table, key is console_write, value is resolved to a function pointer.
	}
}
"

