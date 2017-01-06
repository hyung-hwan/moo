class Magnitude(Object)
{
}

class Association(Magnitude)
{
	dcl key value.

	method(#class) key: key value: value
	{
		^self new key: key value: value
	}
	method key: key value: value
	{
		self.key := key.
		self.value := value.
	}

	method key
	{
		^self.key
	}

	method value
	{
		^self.value
	}
	
	method = ass
	{
		^(self.key = ass key) and: [ self.value = ass value ]
	}
	
	method hash
	{
		^(self.key hash) + (self.value hash)
	}
}

class Character(Magnitude)
{
	## method basicSize
	## {
	## 	^0
	## }
}

class Number(Magnitude)
{
	method + aNumber
	{
		<primitive: #_integer_add>
		self primitiveFailed.
	}

	method - aNumber
	{
		<primitive: #_integer_sub>
		self primitiveFailed.
	}

	method * aNumber
	{
		<primitive: #_integer_mul>
		self primitiveFailed.
	}

	method quo: aNumber
	{
		<primitive: #_integer_quo>
		self primitiveFailed.
	}

	method rem: aNumber
	{
		<primitive: #_integer_rem>
		self primitiveFailed.
	}

	method // aNumber
	{
		<primitive: #_integer_quo2>
		self primitiveFailed.
	}

	method \\ aNumber
	{
		<primitive: #_integer_rem2>
		self primitiveFailed.
	}

	method = aNumber
	{
		<primitive: #_integer_eq>
		self primitiveFailed.
	}

	method ~= aNumber
	{
		<primitive: #_integer_ne>
		self primitiveFailed.
	}

	method < aNumber
	{
		<primitive: #_integer_lt>
		self primitiveFailed.
	}

	method > aNumber
	{
		<primitive: #_integer_gt>
		self primitiveFailed.
	}

	method <= aNumber
	{
		<primitive: #_integer_le>
		self primitiveFailed.
	}

	method >= aNumber
	{
		<primitive: #_integer_ge>
		self primitiveFailed.
	}

	method negated
	{
		<primitive: #_integer_negated>
		^0 - self.
	}

	method bitAt: index
	{
		<primitive: #_integer_bitat>
		^(self bitShift: index negated) bitAnd: 1.
	}

	method bitAnd: aNumber
	{
		<primitive: #_integer_bitand>
		self primitiveFailed.
	}

	method bitOr: aNumber
	{
		<primitive: #_integer_bitor>
		self primitiveFailed.
	}

	method bitXor: aNumber
	{
		<primitive: #_integer_bitxor>
		self primitiveFailed.
	}

	method bitInvert
	{
		<primitive: #_integer_bitinv>
		^-1 - self.
	}

	method bitShift: aNumber
	{
		(* positive number for left shift. 
		 * negative number for right shift *)

		<primitive: #_integer_bitshift>
		self primitiveFailed.
	}

	method asString
	{
		^self printStringRadix: 10
	}

	method printStringRadix: aNumber
	{
		<primitive: #_integer_inttostr>
		self primitiveFailed.
	}

	method to: end by: step do: aBlock
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

	method to: end do: aBlock
	{
		^self to: end by: 1 do: aBlock.
	}
	
	method priorTo: end by: step do: aBlock
	{
		| i |
		i := self.
		(step > 0) 
			ifTrue: [
				[ i < end ] whileTrue: [ 
					aBlock value: i.
					i := i + step.
				].
			]
			ifFalse: [
				[ i > end ] whileTrue: [
					aBlock value: i.
					i := i - step.
				].
			].
	}
	
	method priorTo: end do: aBlock
	{
		^self priorTo: end by: 1 do: aBlock.
	}

	method abs
	{
		self < 0 ifTrue: [^self negated].
		^self.
	}

	method sign
	{
		self < 0 ifTrue: [^-1].
		self > 0 ifTrue: [^1].
		^0.
	}
}

class Integer(Number)
{
	method timesRepeat: aBlock
	{
		1 to: self by: 1 do: [ :count | aBlock value ].
	}
}

class SmallInteger(Integer)
{
	## method basicSize
	## {
	## 	^0
	## }
	
	method asError
	{
		<primitive: #_smooi_as_error>
	}
	
	method asCharacter
	{
		<primitive: #_smooi_as_character>
	}
}

class(#liword) LargeInteger(Integer)
{
}

class(#liword) LargePositiveInteger(LargeInteger)
{
	method abs
	{
		^self.
	}

	method sign
	{
		^1.
	}
}

class(#liword) LargeNegativeInteger(LargeInteger)
{
	method abs
	{
		^self negated.
	}
	
	method sign
	{
		^-1.
	}
}
