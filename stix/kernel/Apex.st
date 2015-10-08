#class Apex(nil)
{
	#dcl(#class) sysdic.

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method(#class) dump
	{
		<primitive: #dump>
	}

	#method dump
	{
		<primitive: #dump>
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method(#class) yourself
	{
		^self.
	}

	#method yourself
	{
		^self.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method(#class) new
	{
		<primitive: #new>
		self primitiveFailed.
	}

	#method(#class) new: anInteger
	{
		<primitive: #newWithSize>
		self primitiveFailed.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method class
	{
		<primitive: #class>
	}

	#method(#class) class
	{
		<primitive: #class>
		^Class
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method basicSize
	{
		<primitive: #basicSize>
		^0
	}

	#method basicAt: anInteger
	{
		<primitive: #basicAt>
		self error: 'out of range'.
	}

	#method basicAt: anInteger put: anObject
	{
		<primitive: #basicAtPut>
		self error: 'out of range'.
	}

	#method(#class) basicSize
	{
		<primitive: #basicSize>
		^0
	}

	#method(#class) basicAt: anInteger
	{
		<primitive: #basicAt>
		self error: 'out of range'.
	}

	#method(#class) basicAt: anInteger put: anObject
	{
		<primitive: #basicAtPut>
		self error: 'out of range'.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method == anObject
	{
		"check if the receiver is identical to anObject.
		 this doesn't compare the contents"
		<primitive: #identical>
	}

	#method ~~ anObject
	{
		<primitive: #notIdentical>
		^(self == anObject) not.
	}

	#method(#class) == anObject
	{
		"check if the receiver is identical to anObject.
		 this doesn't compare the contents"
		<primitive: #identical>
	}

	#method(#class) ~~ anObject
	{
		<primitive: #notIdentical>
		^(self == anObject) not.
	}

	## TODO: add = and ~= for equality check.

	#method isNil
	{
		"^self == nil."
		^false
	}

	#method notNil
	{
		"^(self == nil) not"
		"^self ~= nil."
		^true.
	}

	#method(#class) isNil
	{
		"^self == nil."
		^false
	}

	#method(#class) notNil
	{
		"^(self == nil) not"
		"^self ~= nil."
		^true.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------
	"
	#method(#class) respondsTo: selectorSymbol
	{
		TODO: find selectorSymbol in the class method dictionary...
	}

	#method respondsTo: selectorSymbol
	{
		TODO: find selectorSymbol in the method dictionary...
	}
	"

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method(#class) primitiveFailed
	{
		## TODO: implement this
		## PrimitiveFailureError signal.
		'primitive failed' dump.
	}

	#method primitiveFailed
	{
		self class primitiveFailed.
	}

	#method(#class) doesNotUnderstand: messageSymbol
	{
		## TODO: implement this
		## UnrecognizedMessage signal.
		'does not understand' dump.
	}

	#method doesNotUnderstand: messageSymbol
	{
		self class doesNotUnderstand: messageSymbol
	}

	#method(#class) error: msgText
	{
		## TODO: implement this
		## Error signal: msgText.
		msgText dump.
	}

	#method error: aString
	{
		self class error: aString.
	}

}
