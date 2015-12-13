#include 'Apex.st'.
#include 'Object.st'.
#include 'UndefinedObject.st'.
#include 'Class.st'.
#include 'Boolean.st'.


#class Error(Object)
{
	#method(#class) signal: aString
	{
		"accept an arbitary object instead of a string. 
		 the object can be sent displayString for string conversion"
	}
}

#class Magnitude(Object)
{
}

#class Association(Magnitude)
{
	#dcl key value.
}

#class Character(Magnitude)
{
}

#class Number(Magnitude)
{
	#method + aNumber
	{
		<primitive: #_integer_add>
		self primitiveFailed.
	}

	#method - aNumber
	{
		<primitive: #_integer_sub>
		self primitiveFailed.
	}

	#method * aNumber
	{
		<primitive: #_integer_mul>
		self primitiveFailed.
	}

	#method quo: aNumber
	{
		<primitive: #_integer_quo>
		self primitiveFailed.
	}

	#method rem: aNumber
	{
		<primitive: #_integer_rem>
		self primitiveFailed.
	}


	#method // aNumber
	{
		<primitive: #_integer_quo2>
		self primitiveFailed.
	}

	#method \\ aNumber
	{
		<primitive: #_integer_rem2>
		self primitiveFailed.
	}

	#method = aNumber
	{
		<primitive: #_integer_eq>
		self primitiveFailed.
	}

	#method ~= aNumber
	{
		<primitive: #_integer_ne>
		self primitiveFailed.
	}

	#method < aNumber
	{
		<primitive: #_integer_lt>
		self primitiveFailed.
	}

	#method > aNumber
	{
		<primitive: #_integer_gt>
		self primitiveFailed.
	}

	#method <= aNumber
	{
		<primitive: #_integer_le>
		self primitiveFailed.
	}

	#method >= aNumber
	{
		<primitive: #_integer_ge>
		self primitiveFailed.
	}

	#method bitAnd: aNumber
	{
		<primitive: #_integer_bitand>
		self primitiveFailed.
	}

	#method bitOr: aNumber
	{
		<primitive: #_integer_bitor>
		self primitiveFailed.
	}

	#method bitXor: aNumber
	{
		<primitive: #_integer_bitxor>
		self primitiveFailed.
	}

	#method bitInvert
	{
		##<primitive: #_integer_bitinvert>
		##self primitiveFailed.

		^-1 - self.
	}

	#method asString
	{
		self printStringRadix: 10
	}

	#method printStringRadix: aNumber
	{
		<primitive: #_integer_inttostr>
		self primitiveFailed.
	}

	#method to: end by: step do: aBlock
	{
		| i |

		i := self.
		(step > 0) 
			ifTrue: [
				[ i <= end ] whileTrue: [ 
					aBlock value: i.
					i := i + step.
				]
			]
			ifFalse: [
				[ i >= end ] whileTrue: [
					aBlock value: i.
					i := i - step.
				]
			].
	}

	#method to: end do: aBlock
	{
		^self to: end by: 1 do: aBlock.
	}
}

#class SmallInteger(Number)
{
}

#class(#liword) LargeInteger(Number)
{
}

#class(#liword) LargePositiveInteger(LargeInteger)
{
}

#class(#liword) LargeNegativeInteger(LargeInteger)
{
}


#include 'Collection.st'.
#include 'Collection-ByteArray.st'.
#include 'Collection-Array.st'.
#include 'Collection-Set.st'.

#class(#pointer) CompiledMethod(Object)
{
	#dcl owner preamble preamble_data_1 preamble_data_2 ntmprs nargs code source.
}

#include 'Context.st'.
#include 'Process.st'.

#class FFI(Object)
{
	#dcl name handle funcs.

	#method(#class) new: aString
	{
		^self new open: aString.
	}

	#method open: aString
	{
		self.funcs := Dictionary new.
		self.name := aString.

		self.handle := self privateOpen: self.name.

		"[ self.handle := self privateOpen: self.name ] 
			on: Error do: [
			]
			on: XError do: [
			]."

		^self.
	}

	#method close
	{
		self privateClose: self.handle.
		self.handle := nil.
	}

	#method call: aFunctionName withSig: aString withArgs: anArray
	{
		| f |

	##	f := self.funcs at: aFunctionName.
	##	f isNil ifTrue: [
	##		f := self privateGetSymbol: aFunctionName in: self.handle.
	##		f isNil ifTrue: [ self error: 'No such function' ].
	##		self.funcs at: aFunctionName put: f.
	##	].
f := self privateGetSymbol: aFunctionName in: self.handle.
f isNil ifTrue: [ self error: 'No such function' ].

		^self privateCall: f withSig: aString withArgs: anArray
	}

	#method privateOpen: aString
	{
		<primitive: #_ffi_open>
		^nil. ## TODO: Error signal: 'can not open'
	}

	#method privateClose: aHandle
	{
		<primitive: #_ffi_close>
	}

	#method privateCall: aSymbol withSig: aString withArgs: anArray
	{
		<primitive: #_ffi_call>
	}

	#method privateGetSymbol: aString in: aHandle
	{
		<primitive: #_ffi_getsym>
		^nil.
	}
}


