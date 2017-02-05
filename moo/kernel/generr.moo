#include 'Moo.moo'.

class MyObject(Object)
{
	method(#class) main
	{
		| errmsgs synerrmsgs f |

		errmsgs := #( 
			'no error'
			'generic error'

			'not implemented'
			'subsystem error'
			'internal error that should never have happened'
			'insufficient system memory'
			'insufficient object memory'

			'invalid parameter or argument'
			'data not found'
			'existing/duplicate data'
			'busy'
			'access denied'
			'operation not permitted'
			'not a directory'
			'interrupted'
			'pipe error'
			'resource temporarily unavailable'

			'data too large'
			'message sending error'
			'range error'
			'byte-code full'
			'dictionary full'
			'processor full'
			'semaphore heap full'
			'semaphore list full'
			'divide by zero'
			'I/O error'
			'encoding conversion error'
			'buffer full'
		).
		
		synerrmsgs := #(
			'no error'
			'illegal character'
			'comment not closed'
			'string not closed'
			'no character after $'
			'no valid character after #'
			'wrong character literal'
			'colon expected'
			'string expected'
			'invalid radix' 
			'invalid numeric literal'
			'byte too small or too large'
			'wrong error literal'
			'{ expected'
			'} expected'
			'( expected'
			') expected'
			'] expected'
			'. expected'
			' expected'
			'| expected'
			'> expected'
			':= expected'
			'identifier expected'
			'integer expected'
			'primitive: expected'
			'wrong directive'
			'undefined class'
			'duplicate class'
			'contradictory class definition'
			'wrong class name'
			'dcl not allowed'
			'wrong method name'
			'duplicate method name'
			'duplicate argument name'
			'duplicate temporary variable name'
			'duplicate variable name'
			'duplicate block argument name'
			'undeclared variable'
			'unusable variable in compiled code'
			'inaccessible variable'
			'ambiguous variable'
			'wrong expression primary'
			'too many temporaries'
			'too many arguments'
			'too many block temporaries'
			'too many block arguments'
			'too large block'
			'too large array expression'
			'wrong primitive function number'
			'wrong primitive function identifier'
			'wrong module name'
			'#include error'
			'wrong namespace name'
			'wrong pool dictionary name'
			'duplicate pool dictionary name'
			'literal expected'
			'break or continue not within a loop'
			'break or continue within a block'
			'while expected'
		).

		f := Stdio open: 'generr.out' for: 'w'.
		[ f isError ] ifTrue: [ System logNl: 'Cannot open generr.out'. thisProcess terminate. ].

		self emitMessages: errmsgs named: 'errstr' on: f.
		self emitMessages: synerrmsgs named: 'synerrstr' on: f.

		f close.
	}
	
	method(#class) emitMessages: errmsgs named: name on: f
	{
		| c prefix |
		prefix := name & '_'.
		
		c := errmsgs size - 1.
		0 to: c do: [:i |
			self printString: (errmsgs at: i) prefix: prefix index: i on: f.
		].

		
		f puts: S'static moo_ooch_t* '.
		f puts: name.
		f puts: S'[] =\n{\n'.
		0 to: c do: [:i |
			((i rem: 8) = 0) ifTrue: [ f putc: C'\t' ].
			f puts: prefix.
			f puts: (i asString).
			(i = c) ifFalse: [f puts: S',' ].
			(((i + 1) rem: 8) = 0) ifTrue: [ f putc: C'\n' ] ifFalse: [ f putc: C' ' ].
		].
		(((c + 1) rem: 8) = 0) ifFalse: [ f putc: C'\n' ].
		f puts: S'};\n'.
	}

	method(#class) printString: s prefix: prefix index: index on: f
	{
		| c  |
		c := s size - 1.

		f puts: 'static moo_ooch_t '.
		f puts: prefix.
		f puts: index asString.
		f puts: '[] = {'.

		0 to: c do: [:i |
			f putc: $'.
			f putc: (s at: i).
			f putc: $'.
			(i = c) ifFalse: [f putc: $, ].
		].

		f puts: S',\'\\0\'};\n'.
	}
}
