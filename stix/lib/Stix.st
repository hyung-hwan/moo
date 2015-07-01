#class Stix(nil)
{
	#dcl(#class) sysdic.

	#method(#class) yourself
	{
		^self.
	}

	#method yourself
	{
		^self.
	}

	#method(#class) dump
	{
		<primitive: 0>
	}

	#method dump
	{
		<primitive: 0>
	}

	#method(#class) new
	{
		<primitive: 1>
	}

	#method(#class) new: anInteger
	{
		<primitive: 2>
	}

	#method basicSize
	{
		<primitive: 3>
		^0
	}


	#method basicAt: anInteger
	{
		<primitive: 4>
		## self error: 'out of range'.
	}

	#method basicAt: anInteger put: anObject
	{
		<primitive: 5>
		## self error: 'out of range'.
	}

	#method badReturnError
	{
		## TODO: implement this
	}

	#method mustBeBoolean
	{
		## TODO: implement this
	}

	#method doesNotUnderstand: aMessageSymbol
	{
		## TODO: implement this
	}

	#method error: anErrorString
	{
		anErrorString dump.
	}
}

#class Object(Stix)
{

}

#class NilObject(Stix)
{

}

#class(#pointer) Class(Stix)
{
	#dcl spec selfspec superclass subclasses name instvars classvars classinstvars instmthdic classmthdic.
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
	#method add: aNumber
	{
		<primitive: 7>
	}

	#method + aNumber
	{
		<primitive: 7>
	}

	#method - aNumber
	{
		<primitive: 8>
	}

	#method * aNumber
	{
		<primitive: 8>
	}

	#method = aNumber
	{
		<primitive: 10>
	}

	#method < aNumber
	{
		<primitive: 11>
	}

	#method > aNumber
	{
		<primitive: 12>
	}
}

#class SmallInteger(Number)
{
	
}

#class Boolean(Object)
{

}

#class True(Boolean)
{
	#method ifTrue: trueBlock ifFalse: falseBlock
	{
		^trueBlock value.
	}

	#method ifTrue: trueBlock
	{
		^trueBlock value.
	}

	#method ifFalse: falseBlock
	{
		^nil.
	}
}

#class False(Boolean)
{
	#method ifTrue: trueBlock ifFalse: falseBlock
	{
		^falseBlock value.
	}

	#method ifTrue: trueBlock
	{
		^nil.
	}

	#method ifFalse: falseBlock
	{
		^falseBlock value.
	}
}

#class Collection(Object)
{
}

#class(#byte) ByteArray(Collection)
{
	#method at: anInteger
	{
		^self basicAt: anInteger.
	}

	#method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}

}

#class(#pointer) Array(Collection)
{
	#method at: anInteger
	{
		^self basicAt: anInteger.
	}

	#method at: anInteger put: aValue
	{
		^self basicAt: anInteger put: aValue.
	}
}

#class(#character) String(Array)
{
}

#class(#character) Symbol(Array)
{
}


#class Set(Collection)
{
	#dcl tally bucket.
}

#class SymbolSet(Set)
{
}

#class Dictionary(Set)
{
}

#class SystemDictionary(Dictionary)
{
}

#class MethodDictionary(Dictionary)
{

}

#class(#pointer) Context(Stix)
{
}

#class(#pointer) MethodContext(Context)
{
	#dcl sender ip sp ntmprs method receiver home origin.

	#method pc
	{
		^ip
	}

	#method pc: anInteger
	{
		ip := anInteger.
		"sp := sp - 1."  "whould this always work??? "
	}

	#method sp
	{
		^sp.

	}
	#method sp: anInteger
	{
		sp := anInteger.
	}

	#method pc: aPC sp: aSP
	{
		ip := aPC.
		sp := aSP.
		##sp := sp - 1.
	}
}

#class(#pointer) BlockContext(Context)
{
	#dcl caller ip sp ntmprs nargs source  home origin.

	#method value
	{
		<primitive: 6>
	}

	#method value: a 
	{
		<primitive: 6>
	}

	#method value: a value: b
	{
		<primitive: 6>
	}

	#method value: a value: b value: c
	{
		<primitive: 6>
	}

	#method whileTrue: aBlock
	{
## http://stackoverflow.com/questions/2500483/is-there-a-way-in-a-message-only-language-to-define-a-whiletrue-message-without

## ----------------------------------------------------------------------------

##		^(self value) ifTrue: [aBlock value. self whileTrue: aBlock].

## ----------------------------------------------------------------------------

		## less block context before whileTrue: is recursively sent.
		## whileTrue: is sent in a method context.
##		(self value) ifFalse: [^nil].
##		aBlock value. 
##		self whileTrue: aBlock.

## ----------------------------------------------------------------------------

## ----------------------------------------------------------------------------
		| pc sp xsp |

		sp := thisContext sp.
		sp := sp - 1. "decrement sp by 1 becuase thisContext pushed above affects the sp method"
		pc := thisContext pc.
		self value ifFalse: [ ^nil "^self" ].
		aBlock value.
		##thisContext pc: pc - 3 sp: sp.
		##thisContext pc: pc + 2 sp: sp.   
		thisContext pc: pc + 1 sp: sp.   
		## this +2 or - 3 above is dependent on the byte code instruction size used for 'store'  
		## +2 to skip STORE_INTO_TEMP(pc) and POP_STACKTOP.
		## TODO: make it independent of the byte code size 

## ----------------------------------------------------------------------------

##        #<label>:
##		thisContext pc: #<label> sp: sp.
##
##		| pc |
##		pc := thisContext pc.
##		^self value ifTrue: [aBlock value. thisContext pc: pc]

## ----------------------------------------------------------------------------

##		self value ifTrue: [ aBlock value. thisContext restart. ].
	}

	#method pc
	{
		^ip
	}

	#method pc: anInteger
	{
		ip := anInteger.
	}
	
	#method sp
	{
		^sp
	}

	#method sp: anInteger
	{
		sp := anInteger.
	}

	#method restart
	{
		ip := source pc.
	}
}

#class(#pointer) CompiledMethod(Object)
{
	#dcl owner preamble ntmprs nargs code source.
}

#################################################################
## MAIN
#################################################################
