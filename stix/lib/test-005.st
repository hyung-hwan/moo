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

	#method < aNumber
	{
		<primitive: 9>
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

##		^(self value) ifTrue: [aBlock value. self whileTrue: aBlock].

		| start |
		start := thisContext pc.
		self value ifFalse: [ ^nil "^self" ].
		aBlock value.
		thisContext pc: start.

##		| start |
##		start := thisContext pc.
##		^self value ifTrue: [aBlock value. thisContext pc: start]


## TODO: add restart method.
## self value ifTrue: [ aBlock value. thisContext restart. ].
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
}

#class(#pointer) CompiledMethod(Object)
{
	#dcl owner preamble ntmprs nargs code source.
}

#################################################################
## MAIN
#################################################################

## TODO: use #define to define a class or use #class to define a class.
##       use #extend to extend a class
##       using #class for both feels confusing.

#class Stix
{

}

#class SmallInteger
{
	#method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	#method inc
	{
		^self + 1.
	}
}

#class TestObject(Object)
{
	#dcl(#class) Q R.
	#dcl(#classinst) t1 t2.
}

#class MyObject(TestObject)
{
	#dcl(#class) C B A.

	#method getTrue
	{
		^true.
	}

	#method getTrue: anInteger
	{
		^ anInteger
	}
	#method getFalse
	{
		^false
	}


	#method yyy: aBlock
	{
		| a |
		a := aBlock value.
		^a + 99.
	}

	#method xxx: aBlock
	{
		| a |
		a := self yyy: aBlock.
		'KKKKKKKKKKKKKKKKKKKKKKKKKKKKK' dump.
		^a.
	}
	#method(#class) main2 
	{
		| a b c sum |

##		##(10 add: 20) dump.
##		(10 + 20) dump.
##
##		a := 10 + 20 + 30.
##		b := [:x :y | | t z | x := 20. b := 9. x := 10 + 20 ].
##
##		(b value: 10 value: 20) dump.
##		
##		thisContext basicSize dump.
##
##		(thisContext basicAt: (8 + 5)) dump.
##
##		^self.

		a := self new.
		##a yourself.
		##b := a getTrue; getFalse.
		##b := a getTrue; getFalse; getTrue: 20 + 10.
		##b := a getTrue; getFalse; getTrue: 20 + 10; getTrue: 90 + 20.
		##b := 3 + 5 getTrue: 20; getTrue: 8 + 1; getTrue: 20; yourself.

		b := 3 + 5 inc getTrue: 20 + (30 getTrue: 20; yourself); yourself.

		##b := [:q | q ] value: a getTrue.
		b dump.

		##^self.

## ############################################################
##		A := 99.
		[:x :y | R := y. ] value: 10 value: 6.
		R := R + 1.
		R dump.

		sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1))] ].
		##sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1)) + (sum value: (n - 2))] ].
		(sum value: R; value: 5) dump.

##sum := [ :n | sum value: 5 ].
##sum value: 5.

		#[ 1 2 3] dump.
		#[ 4 5 6] dump.
	#(abc:def: 2 'string is good' 3 4 (5 6) (7 (8 9)) 10) dump.
		#([] #[]) dump.


	a := #(abc:def: -2 'string is good' 3 #[2 3 4] 4 (5 6) (7 (8 [4 56] 'hello' 9)) 10 -93952 self true false nil thisContext super).
	a at: 3 put: 'hello world'; dump.


	a := self new.
	(a xxx: [888]) dump.
	20 dump.

	b := 0.
	[ b < 9 ] whileTrue: [ b dump. b := b + 1 ].

	S'hello \t\u78966\u8765\u3456\u2723\x20\123world\uD57C\uB85C\uC6B0' dump.
	C'\n' dump.
#abc:def: dump.

		##a := (11 < 10) ifTrue: [5] ifFalse: [20].
		##a dump.
	}

	#method(#class) main99
	{
		|a b c|

		self main2.
##		b := 0.
##		[ b < 5 ] whileTrue: [ b dump. b := b + 1 ].
	}

	#method(#class) getTen
	{
		^10
	}

	#method(#class) main
	{
		|a b sum |

		sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1))] ].
		(sum value: 5) dump.

		'-------------------------' dump.
		b := 0.
		[ b < 1000 ] whileTrue: [ b dump. b := b + 1 ].

		'-------------------------' dump.
		 a := #[4 5 6 7] at: 3. 
		(#[3 2 1] at: 3) dump.


		## thisContext value. "the message value must be unresolvable as thisContext is a method context"
		## [thisContext value] value.
		'-------------------------' dump.
		b := 0.
		[ b := b + 1. b dump. thisContext value] value.

		[self getTen] value dump.
	}
}


"
 [ a := 20.  b := [ a + 20 ].   b value. ] value
^                 ^               ^        ^              
p1                p3             p4        p2

--------------------------------------------------------------------------------
AC
--------------------------------------------------------------------------------
		mc1<active>
		   mc1->sender := fake_initial_context.
		   mc1->home := nil.
		   mc1->origin := mc1.

mc1		p1 -> bc1 is created based on mc1 (mc1 blockCopy:)
		   bc1->caller := nil
		   bc1->origin := mc1.
		   bc1->home := mc1. (the active context is a method context. so just use it as a home).
		   bc1->source := nil.
		   
mc1		p2 -> bc2 is shallow-copied of bc1. (bc1 value)
		   bc2->caller := mc1. (mc1 is the active context at p2 time)
		   bc2->origin := bc1->origin.
		   bc2->home := bc1->home.
		   bc2->source := bc1.

bc2		bc3 is created based on bc2. (bc2 blockCopy:)
		   bc3->caller := nil 
		   bc3->origin := bc2->origin
		   //bc3->home := bc2. 
		   bc3->home := bc2->source. (the active context is a block context. take from the block context's source */
		   bc3->source := nil.

bc2		bc4 is shallow-copied of bc3.   (bc3 value)
		   bc4->caller := bc2. (bc2 is the active context at p2 time)
		   bc4->origin := bc3->origin
		   bc4->home := bc3->home
		   bc4->source = bc3.

bc4.
--------------------------------------------------------------------------------

'home' is set when the context is created by blockCopy.
'caller' is set when the context is activated.
all 'origin' fields point to mc1 as a result.
self represents the receiver. that is bc->origin->receiver which is mc1->receiver.

--------------------------------------------------------------------------------

#method ifTrue: trueBlock
{
	^trueBlock value.
}

#method whileTrue: aBlock
{
	(self value) ifTrue: [aBlock value. self whileTrue: aBlock].
}

[ b < 10 ] whileTrue: [ b dump. b := b + 1 ].

"
