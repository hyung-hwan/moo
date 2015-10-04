#class Apex(nil)
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
		<primitive: #dump>
	}

	#method dump
	{
		<primitive: #dump>
	}

	#method(#class) new
	{
		## <primitive: 1>
		<primitive: #new>
	}

	#method(#class) new: anInteger
	{
		## <primitive: 2>
		<primitive: #newWithSize>
	}

	#method basicSize
	{
		## <primitive: 3>
		<primitive: #basicSize>
		^0
	}


	#method basicAt: anInteger
	{
		## <primitive: 4>
		<primitive: #basicAt>
		## self error: 'out of range'.
	}

	#method basicAt: anInteger put: anObject
	{
		## <primitive: 5>
		<primitive: #basicAtPut>
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

#class Object(Apex)
{

}

#class UndefinedObject(Apex)
{

}

#class(#pointer) Class(Apex)
{
	#dcl spec selfspec superclass subclasses name instvars classvars classinstvars pooldics instmthdic classmthdic.
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
		<primitive: #integerAdd>
	}

	#method + aNumber
	{
		<primitive: #integerAdd>
	}

	#method - aNumber
	{
		<primitive: #integerSub>
	}

	#method * aNumber
	{
		<primitive: #integerMul>
	}

	#method = aNumber
	{
		<primitive: #integerEQ>
	}

	#method < aNumber
	{
		<primitive: #integerLT>
	}

	#method > aNumber
	{
		<primitive: #integerGT>
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

#class Namespace(Set)
{
}

#class PoolDictionary(Set)
{
}

#class MethodDictionary(Dictionary)
{

}

#class(#pointer) Context(Apex)
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
		<primitive: #blockValue>
	}

	#method value: a 
	{
		<primitive: #blockValue>
	}

	#method value: a value: b
	{
		<primitive: #blockValue>
	}

	#method value: a value: b value: c
	{
		<primitive: #blockValue>
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
## f isNil ifTrue: [ self error: 'No such function' ].

		^self privateCall: f withSig: aString withArgs: anArray
	}

	#method privateOpen: aString
	{
		<primitive: #ffiOpen>
		## throw an exception here.
	}

	#method privateClose: aHandle
	{
		<primitive: #ffiClose>
	}

	#method privateCall: aSymbol withSig: aString withArgs: anArray
	{
		<primitive: #ffiCall>
	}

	#method privateGetSymbol: aString in: aHandle
	{
		<primitive: #ffiGetSym>
	}
}


