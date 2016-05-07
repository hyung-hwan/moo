
#class(#pointer) Class(Apex)
{
	#dcl spec selfspec superclass subclasses name instvars classvars classinstvars pooldics instmthdic classmthdic.

	#method(#class) basicNew
	{
		## you must not instantiate a new class this way.
		self cannotInstantiate.
	}

	#method(#class) initialize
	{
		^self.
	}


	## ########################################################################
	## most of the following methods can actuall become class methods of Apex.
	## ########################################################################

	#method name
	{
		^self.name
	}

	#method superclass
	{
		^self.superclass
	}

	#method specNumInstVars
	{
		## shift right by 7 bits.
		## see stix-prv.h for details.
		^self.spec bitShift: -7
	}

	"#method inheritsFrom: aSuperclass
	{
		| c |
		c := self superclass.
		[c notNil] whileTrue: [
			[ c == aSuperclass ] ifTrue: [^true].
			c := c superclass.
		].
		^false
	}"
}
