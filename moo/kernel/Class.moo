interface ClassInterface
{
	method name.
	method superclass.
}

//
// the Class object should be a variable-pointer object because
// it needs to accomodate class instance variables.
//
class(#pointer,#limited,#uncopyable) Class(Apex) [ClassInterface]
{
	var spec, selfspec, superclass, subclasses, name, modname.
	var instvars, classinstvars, classvars, pooldics.
	var instmthdic, classmthdic, nsup, nsdic.
	var trsize, trgc, initv, initv_ci.

	method(#class) initialize { ^self }

	/* most of the following methods can actually become class methods of Apex.
	 * if the instance varibles can be made accessible from the Apex class. */
	method name { ^self.name }
	method superclass { ^self.superclass }

	method specNumInstVars
	{
		// shift right by 8 bits.
		// see MOO_CLASS_SPEC_NAMED_INSTVARS in moo-prv.h for details.
		^self.spec bitShift: -8
	}

	/*method inheritsFrom: aSuperclass
	{
		| c |
		c := self superclass.
		[c notNil] whileTrue: [
			[ c == aSuperclass ] ifTrue: [^true].
			c := c superclass.
		].
		^false
	}*/

	method nsup { ^self.nsup }
	method nsdic { ^self.nsdic }
}

class(#pointer,#limited,#uncopyable) Interface(Apex)
{
	var name.
	var instmthdic, classmthdic, nsup.
}
