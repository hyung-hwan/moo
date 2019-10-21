class Boolean(Object)
{
	/* TODO: do i need to really define methods defined in True and False here?
	       and call subclassResponsibiltiy?" */
}

class(#final, #limited) True(Boolean)
{
	method not
	{
		^false
	}

	method & aBoolean
	{
		^aBoolean
	}

	method | aBoolean
	{
		^true
	}

	method and: aBlock
	{
		^aBlock value
	}

	method or: aBlock
	{
		^true
	}

	method ifTrue: trueBlock ifFalse: falseBlock
	{
		^trueBlock value.
	}

	method ifTrue: trueBlock
	{
		^trueBlock value.
	}

	method ifFalse: falseBlock
	{
		^nil.
	}
}

class(#final, #limited) False(Boolean)
{
	method not
	{
		^true
	}

	method & aBoolean
	{
		^false
	}

	method | aBoolean
	{
		^aBoolean
	}

	method and: aBlock
	{
		^false
	}

	method or: aBlock
	{
		^aBlock value
	}

	method ifTrue: trueBlock ifFalse: falseBlock
	{
		^falseBlock value.
	}

	method ifTrue: trueBlock
	{
		^nil.
	}

	method ifFalse: falseBlock
	{
		^falseBlock value.
	}
}
