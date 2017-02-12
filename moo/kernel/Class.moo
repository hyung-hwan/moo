##
## the Class object should be a variable-pointer object because
## it needs to accomodate class instance variables.
##
class(#pointer) Class(Apex)
{
	dcl spec selfspec superclass subclasses name instvars classvars classinstvars pooldics instmthdic classmthdic nsdic.

	method(#class) basicNew
	{
		## you must not instantiate a new class this way.
		self cannotInstantiate.
	}

	method(#class) initialize
	{
		^self.
	}

	(* most of the following methods can actually become class methods of Apex.
	 * if the instance varibles can be made accessible from the Apex class. *)
	 
	method name
	{
		^self.name
	}

	method superclass
	{
		^self.superclass
	}

	method specNumInstVars
	{
		## shift right by 7 bits.
		## see moo-prv.h for details.
		^self.spec bitShift: -7
	}

	(*method inheritsFrom: aSuperclass
	{
		| c |
		c := self superclass.
		[c notNil] whileTrue: [
			[ c == aSuperclass ] ifTrue: [^true].
			c := c superclass.
		].
		^false
	}*)
}
