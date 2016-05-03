#class Apex(nil)
{
	#dcl(#class) sysdic.

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method(#class) dump
	{
		<primitive: #_dump>
	}

	#method dump
	{
		<primitive: #_dump>
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

	#method(#class) basicNew
	{
		<primitive: #_basic_new>
		self primitiveFailed.
	}

	#method(#class) basicNew: anInteger
	{
		<primitive: #_basic_new_with_size>
		self primitiveFailed.
	}

	#method(#class) ngcNew
	{
		<primitive: #_ngc_new>
		self primitiveFailed.
	}

	#method(#class) ngcNew: anInteger
	{
		<primitive: #_ngc_new_with_size>
		self primitiveFailed.
	}

	#method(#class) new
	{
		| x |
		x := self basicNew.
		x initialize. "TODO: assess if it's good to call 'initialize' from new."
		^x.
	}

	#method(#class) new: anInteger
	{
		| x |
		x := self basicNew: anInteger.
		x initialize. "TODO: assess if it's good to call 'initialize' from new."
		^x.
	}

	#method initialize
	{
		"a subclass may override this method."
		^self.
	}

	#method ngcDispose
	{
		<primitive: #_ngc_dispose>
		self primitiveFailed.
	}
	## -------------------------------------------------------
	## -------------------------------------------------------
	#method shallowCopy
	{
		<primitive: #_shallow_copy>
		self primitiveFailed.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method class
	{
		<primitive: #_class>
	}

	#method(#class) class
	{
		<primitive: #_class>
		^Class
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method basicSize
	{
		<primitive: #_basic_size>
		self primitiveFailed.
	}

	#method(#class) basicSize
	{
		<primitive: #_basic_size>
		self primitiveFailed.
	}

	#method basicAt: anInteger
	{
		<primitive: #_basic_at>
## TODO: chagne it to 'self outOfRangeError' or something.
		self error: 'out of range'.
	}

	#method basicAt: anInteger put: anObject
	{
		<primitive: #_basic_at_put>
		self error: 'out of range'.
	}

	#method(#class) basicAt: anInteger
	{
		<primitive: #_basic_at>
		self error: 'out of range'.
	}

	#method(#class) basicAt: anInteger put: anObject
	{
		<primitive: #_basic_at_put>
		self error: 'out of range'.
	}

	## -------------------------------------------------------
	## -------------------------------------------------------

	#method == anObject
	{
		"check if the receiver is identical to anObject.
		 this doesn't compare the contents"
		<primitive: #_identical>
	}

	#method ~~ anObject
	{
		<primitive: #_not_identical>
		^(self == anObject) not.
	}

	#method(#class) == anObject
	{
		"check if the receiver is identical to anObject.
		 this doesn't compare the contents"
		<primitive: #_identical>
	}

	#method(#class) ~~ anObject
	{
		<primitive: #_not_identical>
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

	## method(#class) primitiveFailed
	## {
	## 	this method will be added after Exception class has been defined.
	## }

	#method primitiveFailed
	{
		self class primitiveFailed.
	}

	#method(#class) doesNotUnderstand: messageSymbol
	{
		## TODO: implement this
		## UnrecognizedMessage signal.
		self class dump.
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

	#method cannotInstantiate
	{
		self class cannotInstantiate
	}
}
