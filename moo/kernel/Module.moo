class(#byte) Module(Object) 
{
	method(#class) _newInstSize
	{
		self subclassResponsibility: #_newInstSize
	}

	method(#class) new: size
	{
		## ignore the specified size
		^(super new: (self _newInstSize))
	}

	method(#class) new
	{
		^(super new: (self _newInstSize))
	}
}