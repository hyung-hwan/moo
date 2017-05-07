class Magnitude(Object)
{
}

class Association(Magnitude)
{
	var key, value.

	method(#class) key: key value: value
	{
		^self new key: key value: value
	}
	
	method key: key value: value
	{
		self.key := key.
		self.value := value.
	}
	
	method value: value
	{
		self.value := value
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

class(#limited) Character(Magnitude)
{
	## method basicSize
	## {
	## 	^0
	## }

	method(#primitive) asInteger.
}

class(#limited) Number(Magnitude)
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

	method to: end by: step do: block
	{
		| i |
		i := self.
		(*
		(step > 0) 
			ifTrue: [
				[ i <= end ] whileTrue: [ 
					block value: i.
					i := i + step.
				].
			]
			ifFalse: [
				[ i >= end ] whileTrue: [
					block value: i.
					i := i - step.
				].
			].
		*)
		if (step > 0)
		{
			while (i <= end)
			{
				block value: i.
				i := i + step.
			}
		}
		else
		{
			while ( i => end)
			{
				block value: i.
				i := i - step.
			}
		}
	}

	method to: end do: block
	{
		^self to: end by: 1 do: block.
	}
	
	method priorTo: end by: step do: block
	{
		| i |
		i := self.
		(*
		(step > 0) 
			ifTrue: [
				[ i < end ] whileTrue: [ 
					block value: i.
					i := i + step.
				].
			]
			ifFalse: [
				[ i > end ] whileTrue: [
					block value: i.
					i := i - step.
				].
			].
		*)
		if (step > 0)
		{
			while (i < end)
			{
				block value: i.
				i := i + step.
			}
		}
		else
		{
			while ( i > end)
			{
				block value: i.
				i := i - step.
			}
		}
	}
	
	method priorTo: end do: block
	{
		^self priorTo: end by: 1 do: block.
	}

	method abs
	{
		(*self < 0 ifTrue: [^self negated].
		^self.*)
		^if (self < 0) { self negated } else { self }
	}

	method sign
	{
		(* self < 0 ifTrue: [^-1].
		self > 0 ifTrue: [^1].
		^0.*)
		^if (self < 0) { -1 } elsif (self > 0) { 1 } else { 0 }
	}
}

class(#limited) Integer(Number)
{
	method timesRepeat: aBlock
	{
		## 1 to: self by: 1 do: [ :count | aBlock value ].
		| count |
		count := 0.
		while (count < self) { aBlock value. count := count + 1 }
	}
}

class(#limited) SmallInteger(Integer)
{
	## method basicSize
	## {
	## 	^0
	## }
	
	method(#primitive) asCharacter.
	method(#primitive) asError.
}

class(#liword,#limited) LargeInteger(Integer)
{
}

class(#liword,#immutable) LargePositiveInteger(LargeInteger)
{
	method abs { ^self }
	method sign { ^1 }
}

class(#liword,#immutable) LargeNegativeInteger(LargeInteger)
{
	method abs { ^self negated }
	method sign { ^-1 }
}
