class(#pointer) Context(Apex)
{
	var sender, ip, sp, ntmprs.

	method sender
	{
		^self.sender
	}

	method isDead
	{
		^self.ip < 0
	}

	method temporaryCount
	{
		^self.ntmprs
	}

/* ---------------------------------
method varargCount
{
method context,

	
	^do calculation...

for a block context, it must access homeContext first and call varargCount

	^self.home varargCount...
}

method varargAt: index
{
method context
	^do calculation...

block context...
	^self.home varargAt: index
}

---------------------------------- */
}

class(#pointer,#final,#limited) MethodContext(Context)
{
	var method, receiver, home, origin.

	method pc
	{
		^self.ip
	}

	method pcplus1
	{
		^self.ip + 1
	}

	method pc: anInteger
	{
		self.ip := anInteger.
	}

	// it is similar to the pc: method but it doesn't 
	// push the return value to the stack.
	method(#primitive) goto: pc.

	method sp
	{
		^self.sp.
	}

	method sp: new_sp
	{
		self.sp := new_sp
	}

	method pc: new_pc sp: new_sp
	{
		self.ip := new_pc.
		self.sp := new_sp.
	}

	method method
	{
		^self.method
	}

	method vargCount
	{
		^self basicSize - self class specNumInstVars - self.ntmprs
	}

	method vargAt: index
	{
		^self basicAt: (index + self class specNumInstVars + self.ntmprs)
	}
}

class(#pointer,#final,#limited) BlockContext(Context)
{
	var nargs, source, home, origin.

	method vargCount
	{
		^self.home vargCount
	}

	method vargAt: index
	{
		^self.home vargAt: index
	}

// TODO: how can i pass variadic arguments to newProcess
// method(#variadic) fork() -> how to pass them to newProcess???
	method fork 
	{
		// crate a new process in the runnable state
		^self newProcess resume.
	}

	// create a new process in the suspended state
	method(#variadic,#primitive) newProcess().

	// evaluate the block
	method(#variadic,#primitive) value().

	method value: a 
	{
		<primitive: #BlockContext_value>
		self primitiveFailed.
	}
	method value: a value: b
	{
		<primitive: #BlockContext_value>
		self primitiveFailed.
	}
	method value: a value: b value: c
	{
		<primitive: #BlockContext_value>
		self primitiveFailed.
	}
	method value: a value: b value: c value: d
	{
		<primitive: #BlockContext_value>
		self primitiveFailed.
	}
	method value: a value: b value: c value: d value: e
	{
		<primitive: #BlockContext_value>
		self primitiveFailed.
	}


	method ifTrue: aBlock
	{
		//^(self value) ifTrue: aBlock.
		^if (self value) { aBlock value }.
	}

	method ifFalse: aBlock
	{
		//^(self value) ifFalse: aBlock.
		^if (self value) { /* nothing */ } else { aBlock value }.
	}

	method ifTrue: trueBlock ifFalse: falseBlock
	{
		//^(self value) ifTrue: trueBlock ifFalse: falseBlock
		^if (self value) { trueBlock value } else { falseBlock value }.
	}

	method whileTrue: aBlock
	{
		/* --------------------------------------------------
		 * Naive recursive implementation
		 * --------------------------------------------------
		(self value) ifFalse: [^nil].
		aBlock value. 
		self whileTrue: aBlock.
		 * -------------------------------------------------- */

		/* --------------------------------------------------
		 * Non-recursive implementation
		 * --------------------------------------------------
		| pc sp |

		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil ].
		aBlock value.

		 * the pc: method leaves thisContext and pc in the stack after
		 * having changes the instruction poointer.
		 * as a result, the stack keeps growing. the goto method
		 * clears thisContext and pc off the stack unlike normal methods.
		 * thisContext pc: pc.
		thisContext goto: pc.
		 * -------------------------------------------------- *(

		/* --------------------------------------------------
		 * Imperative implementation - == true or ~~ false? match the VM implementation
		 * -------------------------------------------------- */
		while ((self value) ~~ false) { aBlock value }.
	}

	method whileTrue
	{
		/* --------------------------------------------------
		 * Naive recursive implementation
		 * --------------------------------------------------
		(self value) ifFalse: [^nil].
		self whileTrue.
		 * -------------------------------------------------- */

		/* --------------------------------------------------
		 * Non-recursive implementation
		 * --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifFalse: [ ^nil ].
		thisContext goto: pc.
		 * -------------------------------------------------- */
		 
		/* --------------------------------------------------
		 * Imperative implementation
		 * -------------------------------------------------- */
		while ((self value) ~~ false) { }.
	}

	method whileFalse: aBlock
	{
		/* --------------------------------------------------
		 * Naive recursive implementation
		 * --------------------------------------------------
		 (self value) ifTrue: [^nil].
		 aBlock value. 
		 self whileFalse: aBlock.
		 * -------------------------------------------------- */

		/* --------------------------------------------------
		 * Non-recursive implementation
		 * --------------------------------------------------
		 *  The assignment to 'pc' uses the POP_INTO_TEMPVAR_1.
		 *  It pops a value off the stack top, stores it to the second
		 *  temporary variable(aBlock is the first temporary variable).
		 *  It is a single byte instruction. 'pc' returned by 
		 *  'thisContext pcplus1'' should point to the instruction after
		 *  the POP_INTO_TEMPVAR_0 instruction.
		 *
		 *  It would need the increment of 2 if the pair of 
		 *  STORE_INTO_TEMPVAR_1 and POP_STACKTOP were used. 
		 *  This implementation is subject to the instructions chosen
		 *  by the compiler.
		 * --------------------------------------------------
		| pc |
		pc := thisContext pcplus1.
		(self value) ifTrue: [ ^nil ]. // ^self
		aBlock value.
		thisContext goto: pc.
		 * -------------------------------------------------- */

		/* --------------------------------------------------
		 * Imperative implementation
		 * -------------------------------------------------- */
		while ((self value) == false) { aBlock value }.
	}

	method whileFalse
	{
		/* --------------------------------------------------
		 * Naive recursive implementation
		 * --------------------------------------------------
		(self value) ifTrue: [^nil].
		self whileFalse.
		 * -------------------------------------------------- */

		/* --------------------------------------------------
		 * Non-recursive implementation
		 * -------------------------------------------------- 
		| pc |
		pc := thisContext pcplus1.
		(self value) ifTrue: [ ^nil ]. // ^self
		thisContext goto: pc.
		* -------------------------------------------------- */
		while ((self value) == false) { }.
	}

	method pc
	{
		^self.ip
	}

	method pc: anInteger
	{
		self.ip := anInteger.
	}
	
	method sp
	{
		^self.sp
	}

	method sp: anInteger
	{
		self.sp := anInteger.
	}

	method restart
	{
		ip := self.source pc.
	}
}


class(#pointer) CompiledMethod(Object)
{
	var owner, 
	    name,
	    preamble,
	    preamble_data_1,
	    preamble_data_2,
	    ntmprs,
	    nargs,
	    //code, <-- only if moo is built with MOO_USE_METHOD_TRAILER disable.
	    dbi_text_offset,
	    dbi_file_offset,
	    source_line,
	    dbi_method_offset.

	method preamble
	{
		^self.preamble
	}

	method preambleCode
	{
		/* TODO: make this a primtive for performance */
		^(self.preamble bitShift: -4) bitAnd: 16r1F.
	}

	method owner
	{
		^self.owner
	}

	method name
	{
		^self.name
	}

	method(#primitive) sourceText.
	method(#primitive) sourceFile.
	method(#primitive) ipSourceLine: ip.

	method sourceLine
	{
		^self.source_line
	}
}
