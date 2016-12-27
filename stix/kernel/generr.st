#include 'Stix.st'.

#class MyObject(Object)
{

	#method(#class) main
	{
		| s c f |

		s := #( 
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
		).

		f := Stdio open: 'generr.out' for: 'w'.
		[ f isError ] ifTrue: [ System logNl: 'Cannot open generr.out'. thisProcess terminate. ].

		c := s size - 1.
		0 to: c do: [:i |
			self printString: (s at: i) index: i on: f.
		].

		f puts: S'static stix_ooch_t* errstr[] =\n{\n'.
		0 to: c do: [:i |
			((i rem: 8) = 0) ifTrue: [ f putc: C'\t' ].
			f puts: S'e'.
			f puts: (i asString).
			(i = c) ifFalse: [f puts: S',' ].
			(((i  + 1) rem: 8) = 0) ifTrue: [ f putc: C'\n' ] ifFalse: [ f putc: C' ' ].
		].
		(((c + 1) rem: 8) = 0) ifFalse: [ f putc: C'\n' ].
		f puts: S'};\n'.

		f close.
	}

	#method(#class) printString: s index: index on: f
	{
		| c  |
		c := s size - 1.

		f puts: 'static stix_ooch_t e'.
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
