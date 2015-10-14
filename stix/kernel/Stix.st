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

#include 'Collection.st'.
#include 'Collection-ByteArray.st'.
#include 'Collection-Array.st'.
#include 'Collection-Set.st'.


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



	"------ TODO: -------------------------------------"
	#method on: anError do: anExceptionBlock
	{
		"TODO: handle if anError is an ErrorSet .."
	}

	#method ensure: aBlock
	{
	}

	#method ifCurtailed: aBlock
	{
	}

	"------ TODO: -------------------------------------"
}

#class(#pointer) CompiledMethod(Object)
{
	#dcl owner preamble preamble_data_1 preamble_data_2 ntmprs nargs code source.
}

#class(#pointer) Process(Object)
{
	#dcl state.
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
		<primitive: #ffiOpen>
		^nil. ## TODO: Error signal: 'can not open'
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
		^nil.
	}
}


