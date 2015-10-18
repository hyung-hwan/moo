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

	#method fork
	{
		"crate a new process in the runnable state"
		^self newProcess resume.
	}

	#method newProcess
	{
		"create a new process in the suspended state"
		<primitive: #_block_new_process>
		self primitiveFailed.
	}

	#method newProcessWith: anArray
	{
		"create a new process in the suspended state passing the elements
		 of anArray as block arguments"
## TODO
	}

	#method value
	{
		<primitive: #_block_value>
	}

	#method value: a 
	{
		<primitive: #_block_value>
	}

	#method value: a value: b
	{
		<primitive: #_block_value>
	}

	#method value: a value: b value: c
	{
		<primitive: #_block_value>
	}

	#method ifTrue: aBlock
	{
		^(self value) ifTrue: aBlock.
	}

	#method ifFalse: aBlock
	{
		^(self value) ifFalse: aBlock.
	}

	#method ifTrue: trueBlock ifFalse: falseBlock
	{
		^(self value) ifTrue: trueBlock ifFalse: falseBlock
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

