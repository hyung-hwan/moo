#include 'Apex.st'.
#include 'Class.st'.
#include 'Boolean.st'.

#########################################################################################

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

	#method key: key value: value
	{
		self.key := key.
		self.value := value.
	}

	#method key
	{
		^self.key
	}

	#method value
	{
		^self.value
	}
}

#class Character(Magnitude)
{
	## #method basicSize
	## {
	## 	^0
	## }
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

	#method negated
	{
		<primitive: #_integer_negated>
		^0 - self.
	}

	#method bitAt: index
	{
		"## index is 1-based"
		<primitive: #_integer_bitat>
		^(self bitShift: (index - 1) negated) bitAnd: 1.
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
		<primitive: #_integer_bitinv>
		^-1 - self.
	}

	#method bitShift: aNumber
	{
		<primitive: #_integer_bitshift>
		self primitiveFailed.
	}

	#method asString
	{
		^self printStringRadix: 10
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
				].
			]
			ifFalse: [
				[ i >= end ] whileTrue: [
					aBlock value: i.
					i := i - step.
				].
			].
	}

	#method to: end do: aBlock
	{
		^self to: end by: 1 do: aBlock.
	}

	#method abs
	{
		self < 0 ifTrue: [^self negated].
		^self.
	}

	#method sign
	{
		self < 0 ifTrue: [^-1].
		self > 0 ifTrue: [^1].
		^0.
	}

}


#class Integer(Number)
{
	#method timesRepeat: aBlock
	{
		1 to: self by: 1 do: [ :count | aBlock value ].
	}
}

#class SmallInteger(Integer)
{
	## #method basicSize
	## {
	## 	^0
	## }
}

#class(#liword) LargeInteger(Integer)
{
}

#class(#liword) LargePositiveInteger(LargeInteger)
{
	#method abs
	{
		^self.
	}

	#method sign
	{
		^1.
	}
}

#class(#liword) LargeNegativeInteger(LargeInteger)
{
	#method abs
	{
		^self negated.
	}
	
	#method sign
	{
		^-1.
	}
}


#include 'Collect.st'.

#class(#pointer) CompiledMethod(Object)
{
	## #dcl owner name preamble preamble_data_1 preamble_data_2 ntmprs nargs code source.
	#dcl owner name preamble preamble_data_1 preamble_data_2 ntmprs nargs source.

	#method preamble
	{
		^self.preamble
	}

	#method preambleCode
	{
		^self.preamble bitAnd: 16rFF.
	}

	#method owner
	{
		^self.owner
	}

	#method name
	{
		^self.name
	}
}

#include 'Context.st'.
#include 'Except.st'.
#include 'Process.st'.

#class Resource(Object)
{
}

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



#########################################################################################

#include 'Console.st'.
